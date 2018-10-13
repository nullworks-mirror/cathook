//
// Created by bencat07 on 17.08.18.
//
#include <hacks/Backtrack.hpp>
#include "common.hpp"
#include "navparser.hpp"
#include "FollowBot.hpp"
#include "NavBot.hpp"
#include "PlayerTools.hpp"

namespace hacks::tf2::NavBot
{
static settings::Bool enable("navbot.enable", "false");
static settings::Bool spy_mode("navbot.spy-mode", "false");
static settings::Bool heavy_mode("navbot.heavy-mode", "false");
static settings::Bool engi_mode("navbot.engi-mode", "false");
static settings::Bool primary_only("navbot.primary-only", "true");

static settings::Bool target_sentry{ "navbot.target-sentry", "true" };
static settings::Bool stay_near{ "navbot.stay-near", "false" };
static settings::Bool take_tele{ "navbot.take-teleporters", "true" };
static settings::Bool enable_fb{ "navbot.medbot", "false" };
static settings::Bool roambot{ "navbot.roaming", "true" };
static settings::Float follow_activation{ "navbot.max-range", "1000" };
static settings::Bool mimic_slot{ "navbot.mimic-slot", "false" };
static settings::Bool always_medigun{ "navbot.always-medigun", "false" };
static settings::Bool sync_taunt{ "navbot.taunt-sync", "false" };
static settings::Bool change_tar{ "navbot.change-roaming-target", "false" };
static settings::Bool autojump{ "navbot.jump-if-stuck", "true" };
static settings::Bool afk{ "navbot.switch-afk", "true" };
static settings::Int afktime{ "navbot.afk-time", "15000" };

unsigned steamid = 0x0;
CatCommand follow_steam("navbot_steam", "Follow Steam Id",
                        [](const CCommand &args) {
                            if (args.ArgC() < 1)
                            {
                                steamid = 0x0;
                                return;
                            }
                            try {
                                steamid = std::stoul(args.Arg(1));
                            } catch (std::invalid_argument) {
                                return;
                            }
                        });
Timer lastTaunt{}; // time since taunt was last executed, used to avoid kicks
Timer lastJump{};
std::array<Timer, 32> afkTicks; // for how many ms the player hasn't been moving

void checkAFK()
{
    for (int i = 0; i < g_GlobalVars->maxClients; i++)
    {
        auto entity = ENTITY(i);
        if (CE_BAD(entity))
            continue;
        if (!CE_VECTOR(entity, netvar.vVelocity).IsZero(60.0f))
            afkTicks[i].update();
    }
}

bool HasLowAmmo()
{
    int *weapon_list =
        (int *) ((unsigned) (RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
    if (g_pLocalPlayer->holding_sniper_rifle &&
        CE_INT(LOCAL_E, netvar.m_iAmmo + 4) <= 5)
        return true;
    for (int i = 0; weapon_list[i]; i++)
    {
        int handle = weapon_list[i];
        int eid    = handle & 0xFFF;
        if (eid >= 32 && eid <= HIGHEST_ENTITY)
        {
            IClientEntity *weapon = g_IEntityList->GetClientEntity(eid);
            if (weapon and re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon) &&
                re::C_TFWeaponBase::UsesPrimaryAmmo(weapon) &&
                !re::C_TFWeaponBase::HasPrimaryAmmo(weapon))
                return true;
        }
    }
    return false;
}

bool HasLowHealth()
{
    return float(LOCAL_E->m_iHealth()) / float(LOCAL_E->m_iMaxHealth()) < 0.64;
}

CachedEntity *nearestSentry()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CObjectSentrygun) ||
            ent->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        if (ent->m_flDistance() < bestscr)
        {
            bestscr = ent->m_flDistance();
            bestent = ent;
        }
    }
    return bestent;
}
CachedEntity *nearestHealth()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CBaseAnimating))
            continue;
        if (ent->m_ItemType() != ITEM_HEALTH_SMALL &&
            ent->m_ItemType() != ITEM_HEALTH_MEDIUM &&
            ent->m_ItemType() != ITEM_HEALTH_LARGE)
            continue;
        if (ent->m_flDistance() < bestscr)
        {
            bestscr = ent->m_flDistance();
            bestent = ent;
        }
    }
    return bestent;
}
CachedEntity *nearestAmmo()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CBaseAnimating))
            continue;
        if (ent->m_ItemType() != ITEM_AMMO_SMALL &&
            ent->m_ItemType() != ITEM_AMMO_MEDIUM &&
            ent->m_ItemType() != ITEM_AMMO_LARGE)
            continue;
        if (ent->m_flDistance() < bestscr)
        {
            bestscr = ent->m_flDistance();
            bestent = ent;
        }
    }
    return bestent;
}
int last_tar = -1;
CachedEntity *NearestEnemy()
{
    if (last_tar != -1 && CE_GOOD(ENTITY(last_tar)) &&
        ENTITY(last_tar)->m_bAlivePlayer() &&
        ENTITY(last_tar)->m_iTeam() != LOCAL_E->m_iTeam())
        return ENTITY(last_tar);
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_Type() != ENTITY_PLAYER)
            continue;
        if (player_tools::shouldTarget(ent) !=
            player_tools::IgnoreReason::DO_NOT_IGNORE)
            continue;
        if (ent == LOCAL_E || !ent->m_bAlivePlayer() ||
            ent->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        float scr = ent->m_flDistance();
        if (g_pPlayerResource->GetClass(ent) == tf_engineer ||
            g_pPlayerResource->GetClass(ent) == tf_heavy)
            scr *= 5.0f;
        if (g_pPlayerResource->GetClass(ent) == tf_pyro)
            scr *= 7.0f;
        if (scr < bestscr)
        {
            bestscr = scr;
            bestent = ent;
        }
    }
    if (CE_BAD(bestent))
        last_tar = -1;
    else
        last_tar = bestent->m_IDX;
    return bestent;
}
Timer ammo_health_cooldown{};
Timer init_timer{};
Timer nav_cooldown{};
Timer engi_spot_cd{};
Timer nav_enemy_cd{};
std::vector<Vector> preferred_sniper_spots;
std::vector<Vector> sniper_spots;
std::vector<Vector> nest_spots;
void Init()
{
    sniper_spots.clear();
    preferred_sniper_spots.clear();
    nest_spots.clear();
    for (auto area : nav::areas)
    {
        if (area.m_attributeFlags & NAV_MESH_NO_HOSTAGES)
            preferred_sniper_spots.push_back(area.m_center);
        if (area.m_attributeFlags & NAV_MESH_RUN)
            nest_spots.push_back(area.m_center);
        for (auto hide : area.m_hidingSpots)
            if (hide.IsGoodSniperSpot() || hide.IsIdealSniperSpot() ||
                hide.IsExposed())
                sniper_spots.push_back(hide.m_pos);
    }
    logging::Info("Sniper spots: %d, Manual Sniper Spots: %d, Sentry Spots: %d",
                  sniper_spots.size(), preferred_sniper_spots.size(),
                  nest_spots.size());
}

std::unordered_map<int, int> priority_spots;
void initonce()
{
    for (int i = 0; i < afkTicks.size(); i++)
        afkTicks[i].update();
    priority_spots.clear();
    ammo_health_cooldown.update();
    init_timer.update();
    nav_cooldown.update();
    engi_spot_cd.update();
    sniper_spots.clear();
    preferred_sniper_spots.clear();
    return;
}

Timer slot_timer{};
void UpdateSlot()
{
    if (!slot_timer.test_and_set(1000))
        return;
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W) && !g_pLocalPlayer->life_state)
    {
        IClientEntity *weapon = RAW_ENT(LOCAL_W);
        // IsBaseCombatWeapon()
        if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon))
        {
            int slot    = re::C_BaseCombatWeapon::GetSlot(weapon);
            int newslot = 1;
            if (*spy_mode || *engi_mode)
                newslot = 3;
            if (slot != newslot - 1)
                g_IEngine->ClientCmd_Unrestricted(
                    format("slot", newslot).c_str());
        }
    }
}
enum BuildingNum
{
    DISPENSER = 0,
    TELEPORT_ENT,
    SENTRY,
    TELEPORT_EXT,
};
std::vector<int> GetBuildings()
{
    float bestscr = FLT_MAX;
    std::vector<int> buildings;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent))
            continue;
        if (ent->m_Type() != ENTITY_BUILDING)
            continue;
        if ((CE_INT(ent, netvar.m_hBuilder) & 0xFFF) !=
            g_pLocalPlayer->entity_idx)
            continue;
        if (ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin()) < bestscr)
        {
            buildings.push_back(i);
            bestscr = ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin());
        }
    }
    return buildings;
}
int cost[4] = { 100, 50, 130, 50 };
int GetBestBuilding(int metal)
{
    bool hasSentry    = false;
    bool hasDispenser = false;
    if (!GetBuildings().empty())
        for (auto build : GetBuildings())
        {
            CachedEntity *building = ENTITY(build);
            if (building->m_iClassID() == CL_CLASS(CObjectSentrygun))
                hasSentry = true;
            if (building->m_iClassID() == CL_CLASS(CObjectDispenser))
                hasDispenser = true;
        }
    if (metal >= cost[SENTRY] && !hasSentry)
        return SENTRY;
    else if (metal >= cost[DISPENSER] && !hasDispenser)
        return DISPENSER;
    if (hasSentry && hasDispenser)
        return 3;
    return -1;
}
int GetClosestBuilding()
{
    float bestscr    = FLT_MAX;
    int BestBuilding = -1;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent))
            continue;
        if (ent->m_Type() != ENTITY_BUILDING)
            continue;
        if ((CE_INT(ent, netvar.m_hBuilder) & 0xFFF) !=
            g_pLocalPlayer->entity_idx)
            continue;
        if (ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin()) < bestscr)
        {
            BestBuilding = i;
            bestscr      = ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin());
        }
    }
    return BestBuilding;
}
int GetClosestTeleporter()
{
    float bestscr    = FLT_MAX;
    int BestBuilding = -1;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent))
            continue;
        if (ent->m_iClassID() != CL_CLASS(CObjectTeleporter))
            continue;
        if (ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin()) < bestscr)
        {
            BestBuilding = i;
            bestscr      = ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin());
        }
    }
    return BestBuilding;
}
bool NavToSentry(int priority)
{
    CachedEntity *Sentry = nearestSentry();
    if (CE_BAD(Sentry))
        return false;
    int num = nav::FindNearestValid(GetBuildingPosition(Sentry));
    if (num == -1)
        return false;
    auto area = nav::areas[num];
    if (nav::NavTo(area.m_center, false, true, priority))
        return true;
    return false;
}
bool NavToEnemy()
{
    if (*stay_near)
    {
        static Vector lastgoal{};
        CachedEntity *ent = NearestEnemy();
        if (CE_GOOD(ent))
        {
            int nearestvalid{};
            if (!*heavy_mode)
                nearestvalid =
                    nav::FindNearestValidbyDist(ent->m_vecOrigin(), 2000, 6000);
            else
                nearestvalid =
                    nav::FindNearestValidbyDist(ent->m_vecOrigin(), 200, 1000);
            if (nearestvalid != -1)
            {
                auto area = nav::areas[nearestvalid];
                nav::NavTo(area.m_center, false, true, 1337);
                lastgoal = area.m_center;
                return true;
            }
        }
        else if (lastgoal.z && LOCAL_E->m_vecOrigin().DistTo(lastgoal) > 200.0f)
        {
            nav::NavTo(lastgoal, false, true, 1337);
            return true;
        }
        else
            lastgoal = {};
    }
    return false;
}
bool NavToSniperSpot(int priority)
{
    Vector random_spot{};
    if (!sniper_spots.size() && !preferred_sniper_spots.size())
        return false;
    bool use_preferred = !preferred_sniper_spots.empty();
    auto snip_spot     = use_preferred ? preferred_sniper_spots : sniper_spots;
    bool toret         = false;
    if (use_preferred)
    {
        int best_spot       = -1;
        float maxscr        = FLT_MAX;
        int lowest_priority = 9999;
        for (int i = 0; i < snip_spot.size(); i++)
        {
            if ((priority_spots[i] < lowest_priority))
                lowest_priority = priority_spots[i];
        }
        for (int i = 0; i < snip_spot.size(); i++)
        {
            if ((priority_spots[i] > lowest_priority))
                continue;
            float scr = snip_spot[i].DistTo(g_pLocalPlayer->v_Eye);
            if (scr < maxscr)
            {
                maxscr    = scr;
                best_spot = i;
            }
        }

        if (best_spot == -1)
            return false;
        random_spot = snip_spot.at(best_spot);
        if (random_spot.z)
            toret = nav::NavTo(random_spot, false, true, priority);
        priority_spots[best_spot]++;
    }
    else if (!snip_spot.empty())
    {
        int rng     = rand() % snip_spot.size();
        random_spot = snip_spot.at(rng);
        if (random_spot.z)
            toret = nav::NavTo(random_spot, false, true, priority);
    }
    return toret;
}
CatCommand debug_tele("navbot_debug", "debug", []() {
    int idx = GetClosestBuilding();
    if (idx == -1)
        return;
    CachedEntity *ent = ENTITY(idx);
    if (CE_BAD(ent))
        return;
    logging::Info(
        "%d %d %d %f %f %d %f %f %f", CE_INT(ent, netvar.m_iObjectType),
        CE_INT(ent, netvar.m_bBuilding), CE_INT(ent, netvar.m_iTeleState),
        CE_FLOAT(ent, netvar.m_flTeleRechargeTime),
        CE_FLOAT(ent, netvar.m_flTeleCurrentRechargeDuration),
        CE_INT(ent, netvar.m_iTeleTimesUsed),
        CE_FLOAT(ent, netvar.m_flTeleYawToExit), g_GlobalVars->curtime,
        g_GlobalVars->curtime * g_GlobalVars->interval_per_tick);
});

int follow_target = 0;
static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "NavBot", 16, []() {
        if ((!enable && !enable_fb) || !nav::Prepare())
            return;
        if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
            return;
        if (primary_only && enable)
            UpdateSlot();
        if (*stay_near && nav_enemy_cd.test_and_set(1000) &&
            (!HasLowAmmo()) & (!HasLowHealth()))
            NavToEnemy();
        if (HasLowHealth() && ammo_health_cooldown.test_and_set(5000))
        {
            CachedEntity *med = nearestHealth();
            if (CE_GOOD(med))
            {
                if (nav::priority == 1337)
                    nav::clearInstructions();
                nav::NavTo(med->m_vecOrigin(), true, true, 7);
            }
        }
        if (HasLowAmmo() && ammo_health_cooldown.test_and_set(5000))
        {
            CachedEntity *ammo = nearestAmmo();
            if (CE_GOOD(ammo))
            {
                if (nav::priority == 1337)
                    nav::clearInstructions();
                nav::NavTo(ammo->m_vecOrigin(), true, true, 6);
            }
        }
        if ((!HasLowHealth() && nav::priority == 7) ||
            (!HasLowAmmo() && nav::priority == 6))
            nav::clearInstructions();
        static int waittime =
            /*(spy_mode || heavy_mode || engi_mode) ? 100 : 2000*/ 0;
        if (*take_tele)
        {
            int idx = GetClosestTeleporter();
            if (idx != -1)
            {
                CachedEntity *ent = ENTITY(idx);
                if (CE_GOOD(ent) && ent->m_flDistance() < 300.0f)
                    if (CE_FLOAT(ent, netvar.m_flTeleYawToExit) &&
                        CE_FLOAT(ent, netvar.m_flTeleRechargeTime) <
                            g_GlobalVars->curtime)
                    {
                        waittime = 1000;
                        nav_cooldown.update();
                        if (nav::priority == 1337)
                            nav::clearInstructions();
                        nav::NavTo(GetBuildingPosition(ent), false, false);
                    }
            }
        }
        if (enable)
        {
            if (!nav::ReadyForCommands && !spy_mode && !heavy_mode &&
                !engi_mode)
                nav_cooldown.update();
            if (target_sentry && NavToSentry(3))
                return;
            bool isready = (spy_mode || heavy_mode || engi_mode)
                               ? true
                               : nav::ReadyForCommands;
            if (isready && nav_cooldown.test_and_set(waittime))
            {
                waittime =
                    /*(spy_mode || heavy_mode || engi_mode) ? 100 : 2000*/ 0;
                if (!spy_mode && !heavy_mode && !engi_mode)
                {
                    nav_cooldown.update();
                    if (init_timer.test_and_set(5000))
                        Init();
                    if (!NavToSniperSpot(5))
                        waittime = 1;
                }
                else if (!engi_mode)
                {
                    CachedEntity *tar = NearestEnemy();
                    if (CE_BAD(tar) && last_tar == -1 && nav::ReadyForCommands)
                    {
                        if (init_timer.test_and_set(5000))
                            Init();
                        if (!NavToSniperSpot(4))
                            waittime = 1;
                    }
                    if (CE_GOOD(tar))
                    {
                        if (!spy_mode ||
                            !hacks::shared::backtrack::isBacktrackEnabled)
                        {
                            if (!nav::NavTo(tar->m_vecOrigin(), false))
                                last_tar = -1;
                        }
                        else
                        {
                            auto unsorted_ticks = hacks::shared::backtrack::
                                headPositions[tar->m_IDX];
                            std::vector<hacks::shared::backtrack::BacktrackData>
                                sorted_ticks;
                            for (int i = 0; i < 66; i++)
                            {
                                if (hacks::shared::backtrack::ValidTick(
                                        unsorted_ticks[i], tar))
                                    sorted_ticks.push_back(unsorted_ticks[i]);
                            }
                            if (sorted_ticks.empty())
                            {
                                if (!nav::NavTo(tar->m_vecOrigin(), false))
                                    last_tar = -1;
                                return;
                            }
                            std::sort(
                                sorted_ticks.begin(), sorted_ticks.end(),
                                [](const hacks::shared::backtrack::BacktrackData
                                       &a,
                                   const hacks::shared::backtrack::BacktrackData
                                       &b) {
                                    return a.tickcount > b.tickcount;
                                });

                            if (!sorted_ticks[5].tickcount ||
                                nav::NavTo(sorted_ticks[5].entorigin, false,
                                           false))
                                if (!nav::NavTo(tar->m_vecOrigin(), false))
                                    last_tar = -1;
                        }
                    }
                }
                // Engi Mode
                else
                {
                    // Init things
                    if (init_timer.test_and_set(5000))
                        Init();
                    // If No spots set just return
                    if (nest_spots.empty())
                        return;
                    // Get Metal (offset of MAX metal is +8 and current metal
                    // +12)
                    int metal = CE_INT(LOCAL_E, netvar.m_iAmmo + 12);
                    // Best spot storage
                    static Vector best_spot{};
                    // Get Best spot based on distance
                    if (engi_spot_cd.test_and_set(10000))
                    {
                        float bestscr = FLT_MAX;
                        for (auto spot : nest_spots)
                        {
                            if (spot.DistTo(LOCAL_E->m_vecOrigin()) < bestscr)
                            {
                                bestscr   = spot.DistTo(LOCAL_E->m_vecOrigin());
                                best_spot = spot;
                            }
                        }
                    }
                    if (nav::priority == 1)
                    {
                        CachedEntity *ammo = nearestAmmo();
                        if (CE_GOOD(ammo))
                        {
                            nav::NavTo(ammo->m_vecOrigin(), false, true);
                            return;
                        }
                    }
                    // If Near The best spot and ready for commands
                    if (best_spot.DistTo(LOCAL_E->m_vecOrigin()) < 200.0f &&
                        (nav::ReadyForCommands || nav::priority == 1))
                    {
                        // Get the closest Building
                        int ClosestBuilding = GetClosestBuilding();
                        // If A Building was found
                        if (ClosestBuilding != -1)
                        {
                            CachedEntity *ent = ENTITY(ClosestBuilding);
                            // If we have more than 25 metal and the building is
                            // damaged or not fully upgraded hit it with the
                            // wrench
                            if (metal > 25 &&
                                (CE_INT(ent, netvar.iUpgradeLevel) < 3 ||
                                 CE_INT(ent, netvar.iBuildingHealth) <
                                     CE_INT(ent, netvar.iBuildingMaxHealth)))
                            {
                                auto collide = RAW_ENT(ent)->GetCollideable();
                                Vector min =
                                    ent->m_vecOrigin() + collide->OBBMins();
                                Vector max =
                                    ent->m_vecOrigin() + collide->OBBMaxs();
                                // Distance check
                                if (min.DistTo(g_pLocalPlayer->v_Eye) >
                                        re::C_TFWeaponBaseMelee::GetSwingRange(
                                            RAW_ENT(LOCAL_W)) &&
                                    max.DistTo(g_pLocalPlayer->v_Eye) >
                                        re::C_TFWeaponBaseMelee::GetSwingRange(
                                            RAW_ENT(LOCAL_W)) &&
                                    GetBuildingPosition(ent).DistTo(
                                        g_pLocalPlayer->v_Eye) >
                                        re::C_TFWeaponBaseMelee::GetSwingRange(
                                            RAW_ENT(LOCAL_W)))
                                {
                                    float minf =
                                        min.DistTo(g_pLocalPlayer->v_Eye);
                                    float maxf =
                                        max.DistTo(g_pLocalPlayer->v_Eye);
                                    float center =
                                        GetBuildingPosition(ent).DistTo(
                                            g_pLocalPlayer->v_Eye);
                                    float closest =
                                        fminf(minf, fminf(maxf, center));
                                    Vector tonav =
                                        (minf == closest)
                                            ? min
                                            : (maxf == closest)
                                                  ? max
                                                  : GetBuildingPosition(ent);
                                    nav::NavTo(tonav, false, false);
                                }
                                Vector tr = GetBuildingPosition(ent) -
                                            g_pLocalPlayer->v_Eye;
                                Vector angles;
                                VectorAngles(tr, angles);
                                // Clamping is important
                                fClampAngle(angles);
                                current_user_cmd->viewangles = angles;
                                current_user_cmd->buttons |= IN_ATTACK;
                                g_pLocalPlayer->bUseSilentAngles = true;
                                return;
                            }
                        }
                        // Get A building, Sentry > Dispenser
                        int tobuild = GetBestBuilding(metal);
                        // If not enough metal then Find ammo
                        if (tobuild == -1)
                        {
                            CachedEntity *ammo = nearestAmmo();
                            if (CE_GOOD(ammo))
                            {
                                nav::NavTo(ammo->m_vecOrigin(), false, true);
                                return;
                            }
                            // Ammo is dormant, go and find it!
                            else if (sniper_spots.size() &&
                                     nav::ReadyForCommands)
                            {
                                if (init_timer.test_and_set(5000))
                                    Init();
                                if (!NavToSniperSpot(1))
                                    waittime = 1;
                                return;
                            }
                        }
                        // Build Building
                        else if (tobuild != 3)
                        {
                            // Make ENgi look slightly down
                            current_user_cmd->viewangles.x = 20.0f;
                            // Build buildings in a 360° angle around player
                            current_user_cmd->viewangles.y =
                                90.0f * (tobuild + 1);
                            // Build new one
                            g_IEngine->ServerCmd(
                                format("build ", tobuild).c_str(), true);
                            current_user_cmd->buttons |= IN_ATTACK;
                            g_pLocalPlayer->bUseSilentAngles = true;
                        }
                    }
                    // If not near best spot then navigate to it
                    else if (nav::ReadyForCommands)
                        nav::NavTo(best_spot, false, true);
                }
            }
        }
        else if (enable_fb)
        {
            // We need a local player to control
            if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
            {
                follow_target = 0;
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }

            if (afk)
                checkAFK();

            // Still good check
            if (follow_target)
                if (CE_BAD(ENTITY(follow_target)))
                    follow_target = 0;

            if (!follow_target)
                nav::NavTo(LOCAL_E->m_vecOrigin(), true,
                           false); // no target == no path
            // Target Selection
            if (steamid)
            {
                // Find a target with the steam id, as it is prioritized
                auto ent_count = HIGHEST_ENTITY;
                for (int i = 0; i < ent_count; i++)
                {
                    auto entity = ENTITY(i);
                    if (CE_BAD(entity)) // Exist + dormant
                        continue;
                    if (i == follow_target)
                        break;
                    if (entity->m_Type() != ENTITY_PLAYER)
                        continue;
                    if (steamid !=
                        entity->player_info.friendsID) // steamid check
                        continue;

                    if (!entity->m_bAlivePlayer()) // Dont follow dead players
                        continue;
                    follow_target = entity->m_IDX;
                    break;
                }
            }
            // If we dont have a follow target from that, we look again for
            // someone else who is suitable
            if ((!follow_target || change_tar ||
                 (hacks::shared::followbot::ClassPriority(
                      ENTITY(follow_target)) < 6 &&
                  ENTITY(follow_target)->player_info.friendsID != steamid)) &&
                roambot)
            {
                // Try to get a new target
                auto ent_count = g_IEngine->GetMaxClients();
                for (int i = 0; i < ent_count; i++)
                {
                    auto entity = ENTITY(i);
                    if (CE_BAD(entity)) // Exist + dormant
                        continue;
                    if (entity->m_Type() != ENTITY_PLAYER)
                        continue;
                    if (entity == LOCAL_E) // Follow self lol
                        continue;
                    if (entity->m_bEnemy())
                        continue;
                    if (afk && afkTicks[i].check(
                                   int(afktime))) // don't follow target that
                                                  // was determined afk
                        continue;
                    if (IsPlayerDisguised(entity) || IsPlayerInvisible(entity))
                        continue;
                    if (!entity->m_bAlivePlayer()) // Dont follow dead players
                        continue;
                    if (follow_activation &&
                        entity->m_flDistance() > (float) follow_activation)
                        continue;
                    const model_t *model =
                        ENTITY(follow_target)->InternalEntity()->GetModel();
                    // FIXME follow cart/point
                    /*if (followcart && model &&
                        (lagexploit::pointarr[0] || lagexploit::pointarr[1] ||
                         lagexploit::pointarr[2] || lagexploit::pointarr[3] ||
                         lagexploit::pointarr[4]) &&
                        (model == lagexploit::pointarr[0] ||
                         model == lagexploit::pointarr[1] ||
                         model == lagexploit::pointarr[2] ||
                         model == lagexploit::pointarr[3] ||
                         model == lagexploit::pointarr[4]))
                        follow_target = entity->m_IDX;*/
                    if (entity->m_Type() != ENTITY_PLAYER)
                        continue;
                    // favor closer entitys
                    if (follow_target &&
                        ENTITY(follow_target)->m_flDistance() <
                            entity->m_flDistance()) // favor closer entitys
                        continue;
                    // check if new target has a higher priority than current
                    // target
                    if (hacks::shared::followbot::ClassPriority(
                            ENTITY(follow_target)) >=
                        hacks::shared::followbot::ClassPriority(ENTITY(i)))
                        continue;
                    // ooooo, a target
                    follow_target = i;
                    afkTicks[i].update(); // set afk time to 0
                }
            }
            // last check for entity before we continue
            if (!follow_target)
            {
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }

            CachedEntity *followtar = ENTITY(follow_target);
            // wtf is this needed
            if (CE_BAD(followtar) || !followtar->m_bAlivePlayer())
            {
                follow_target = 0;
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }
            // Check if we are following a disguised/spy
            if (IsPlayerDisguised(followtar) || IsPlayerInvisible(followtar))
            {
                follow_target = 0;
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }
            // check if target is afk
            if (afk)
            {
                if (afkTicks[follow_target].check(int(afktime)))
                {
                    follow_target = 0;
                    nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                    return;
                }
            }

            // Update timer on new target
            static Timer idle_time{};
            if (nav::ReadyForCommands)
                idle_time.update();

            // If the player is close enough, we dont need to follow the path
            auto tar_orig       = followtar->m_vecOrigin();
            auto loc_orig       = LOCAL_E->m_vecOrigin();
            auto dist_to_target = loc_orig.DistTo(tar_orig);
            if (!CE_VECTOR(followtar, netvar.vVelocity).IsZero(20.0f))
                idle_time.update();

            // Tauntsync
            if (sync_taunt && HasCondition<TFCond_Taunting>(followtar) &&
                lastTaunt.test_and_set(1000))
                g_IEngine->ClientCmd("taunt");

            // Check for jump
            if (autojump && lastJump.check(1000) && idle_time.check(2000))
            {
                current_user_cmd->buttons |= IN_JUMP;
                lastJump.update();
            }
            // Check if still moving. 70 HU = Sniper Zoomed Speed
            if (idle_time.check(3000) &&
                CE_VECTOR(g_pLocalPlayer->entity, netvar.vVelocity)
                    .IsZero(60.0f))
            {
                follow_target = 0;
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }
            // Basic idle check
            if (idle_time.test_and_set(5000))
            {
                follow_target = 0;
                nav::NavTo(LOCAL_E->m_vecOrigin(), true, false, 4);
                return;
            }

            static float last_slot_check = 0.0f;
            if (g_GlobalVars->curtime < last_slot_check)
                last_slot_check = 0.0f;
            if (follow_target && (always_medigun || mimic_slot) &&
                (g_GlobalVars->curtime - last_slot_check > 1.0f) &&
                !g_pLocalPlayer->life_state &&
                !CE_BYTE(ENTITY(follow_target), netvar.iLifeState))
            {

                // We are checking our slot so reset the timer
                last_slot_check = g_GlobalVars->curtime;

                // Get the follow targets active weapon
                int owner_weapon_eid =
                    (CE_INT(ENTITY(follow_target), netvar.hActiveWeapon) &
                     0xFFF);
                IClientEntity *owner_weapon =
                    g_IEntityList->GetClientEntity(owner_weapon_eid);

                // If both the follow targets and the local players weapons arnt
                // null or
                // dormant
                if (owner_weapon && CE_GOOD(g_pLocalPlayer->weapon()))
                {

                    // IsBaseCombatWeapon()
                    if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(
                            RAW_ENT(g_pLocalPlayer->weapon())) &&
                        re::C_BaseCombatWeapon::IsBaseCombatWeapon(
                            owner_weapon))
                    {

                        // Get the players slot numbers and store in some vars
                        int my_slot = re::C_BaseCombatWeapon::GetSlot(
                            RAW_ENT(g_pLocalPlayer->weapon()));
                        int owner_slot =
                            re::C_BaseCombatWeapon::GetSlot(owner_weapon);

                        // If the local player is a medic and user settings
                        // allow, then keep the medigun out
                        if (g_pLocalPlayer->clazz == tf_medic && always_medigun)
                        {
                            if (my_slot != 1)
                            {
                                g_IEngine->ExecuteClientCmd("slot2");
                            }

                            // Else we attemt to keep our weapon mimiced with
                            // our follow target
                        }
                        else
                        {
                            if (my_slot != owner_slot)
                            {
                                g_IEngine->ExecuteClientCmd(
                                    format("slot", owner_slot + 1).c_str());
                            }
                        }
                    }
                }
            }
            nav::NavTo(tar_orig, false, true, 5);
        }
    });
} // namespace hacks::tf2::NavBot
