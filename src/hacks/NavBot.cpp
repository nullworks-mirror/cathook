//
// Created by bencat07 on 17.08.18.
//
#include "common.hpp"
#include <glez/draw.hpp>
#include <glez/color.hpp>
#include "navparser.hpp"
#include "FollowBot.hpp"
#include "NavBot.hpp"

namespace hacks::tf2::NavBot
{
static settings::Bool enable("navbot.enable", "false");
static settings::Bool spy_mode("navbot.spy-mode", "false");
static settings::Bool heavy_mode("navbot.heavy-mode", "false");
static settings::Bool primary_only("navbot.primary-only", "true");

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
                            steamid = atol(args.Arg(1));
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
        {
            afkTicks[i].update();
        }
    }
}

bool HasLowAmmo()
{
    int *weapon_list =
        (int *) ((unsigned) (RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
    for (int i = 0; weapon_list[i]; i++)
    {
        int handle = weapon_list[i];
        int eid    = handle & 0xFFF;
        if (eid >= 32 && eid <= HIGHEST_ENTITY)
        {
            IClientEntity *weapon = g_IEntityList->GetClientEntity(eid);
            if (weapon and
                re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon) and
                re::C_TFWeaponBase::UsesPrimaryAmmo(weapon) and
                not re::C_TFWeaponBase::HasPrimaryAmmo(weapon))
                return true;
        }
    }
    return false;
}

bool HasLowHealth()
{
    return float(LOCAL_E->m_iHealth()) / float(LOCAL_E->m_iMaxHealth()) < 0.64;
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
CachedEntity *NearestEnemy()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_Type() != ENTITY_PLAYER)
            continue;
        if (ent == LOCAL_E || !ent->m_bAlivePlayer() ||
            ent->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        float scr = ent->m_flDistance();
        if (g_pPlayerResource->GetClass(ent) == tf_engineer)
            scr *= 5.0f;
        if (g_pPlayerResource->GetClass(ent) == tf_pyro)
            scr *= 7.0f;
        if (scr < bestscr)
        {
            bestscr = scr;
            bestent = ent;
        }
    }
    return bestent;
}
Timer cdr{};
Timer cd2{};
Timer cd3{};
std::vector<Vector> sniper_spots;
void Init()
{
    sniper_spots.clear();
    for (auto area : nav::areas)
        for (auto hide : area.m_hidingSpots)
            if (hide.IsGoodSniperSpot() || hide.IsIdealSniperSpot() ||
                hide.IsExposed())
                sniper_spots.push_back(hide.m_pos);
    logging::Info("%d", sniper_spots.size());
}
void initonce()
{
    for (int i = 0; i < afkTicks.size(); i++)
        afkTicks[i].update();
    cdr.update();
    cd2.update();
    cd3.update();
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
            if (spy_mode)
                newslot = 3;
            if (slot != newslot - 1)
                g_IEngine->ClientCmd_Unrestricted(
                    format("slot", newslot).c_str());
        }
    }
}
int follow_target = 0;
void CreateMove()
{
    if ((!enable && !enable_fb) || !nav::Prepare())
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (primary_only && enable)
        UpdateSlot();
    if (HasLowHealth() && cdr.test_and_set(5000))
    {
        CachedEntity *med = nearestHealth();
        if (CE_GOOD(med))
        {
            nav::NavTo(med->m_vecOrigin(), true, true, 7);
        }
    }
    if (HasLowAmmo() && cdr.test_and_set(5000))
    {
        CachedEntity *ammo = nearestAmmo();
        if (CE_GOOD(ammo))
        {
            nav::NavTo(ammo->m_vecOrigin(), true, true, 6);
        }
    }
    if (enable)
    {
        if (!nav::ReadyForCommands && !spy_mode && !heavy_mode)
            cd3.update();
        bool isready = (spy_mode || heavy_mode) ? 1 : nav::ReadyForCommands;
        int waittime = (spy_mode || heavy_mode) ? 100 : 5000;
        if (isready && cd3.test_and_set(waittime))
        {
            if (!spy_mode && !heavy_mode)
            {
                cd3.update();
                Vector random_spot;
                if (cd2.test_and_set(5000))
                    Init();
                if (!sniper_spots.size())
                    return;
                int rng     = rand() % sniper_spots.size();
                random_spot = sniper_spots.at(rng);
                if (random_spot.z)
                    nav::NavTo(random_spot, true, true);
            }
            else if (cdr.check(5000))
            {
                CachedEntity *tar = NearestEnemy();
                if (CE_BAD(tar))
                {
                    Vector random_spot;
                    if (cd2.test_and_set(5000))
                        Init();
                    if (!sniper_spots.size())
                        return;
                    int rng     = rand() % sniper_spots.size();
                    random_spot = sniper_spots.at(rng);
                    if (random_spot.z)
                        nav::NavTo(random_spot, false);
                    return;
                }
                nav::NavTo(tar->m_vecOrigin(), false);
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
                if (steamid != entity->player_info.friendsID) // steamid check
                    continue;

                if (!entity->m_bAlivePlayer()) // Dont follow dead players
                    continue;
                follow_target = entity->m_IDX;
                break;
            }
        }
        // If we dont have a follow target from that, we look again for someone
        // else who is suitable
        if ((!follow_target || change_tar ||
             (hacks::shared::followbot::ClassPriority(ENTITY(follow_target)) <
                  6 &&
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
                if (afk &&
                    afkTicks[i].check(int(afktime))) // don't follow target that
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
                // check if new target has a higher priority than current target
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
            CE_VECTOR(g_pLocalPlayer->entity, netvar.vVelocity).IsZero(60.0f))
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
                (CE_INT(ENTITY(follow_target), netvar.hActiveWeapon) & 0xFFF);
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
                    re::C_BaseCombatWeapon::IsBaseCombatWeapon(owner_weapon))
                {

                    // Get the players slot numbers and store in some vars
                    int my_slot = re::C_BaseCombatWeapon::GetSlot(
                        RAW_ENT(g_pLocalPlayer->weapon()));
                    int owner_slot =
                        re::C_BaseCombatWeapon::GetSlot(owner_weapon);

                    // If the local player is a medic and user settings allow,
                    // then
                    // keep the medigun out
                    if (g_pLocalPlayer->clazz == tf_medic && always_medigun)
                    {
                        if (my_slot != 1)
                        {
                            g_IEngine->ExecuteClientCmd("slot2");
                        }

                        // Else we attemt to keep our weapon mimiced with our
                        // follow
                        // target
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
}
} // namespace hacks::tf2::NavBot
