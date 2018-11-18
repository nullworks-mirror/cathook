//
// Created by bencat07 on 17.08.18.
//
#include <hacks/Backtrack.hpp>
#include <boost/functional/hash.hpp>
#include "common.hpp"
#include "navparser.hpp"
#include "FollowBot.hpp"
#include "NavBot.hpp"
#include "PlayerTools.hpp"

namespace hacks::tf2::NavBot
{
static settings::Bool enable{ "navbot.enable", "false" };
static settings::Bool spy_mode{ "navbot.spy-mode", "false" };
static settings::Bool heavy_mode{ "navbot.heavy-mode", "false" };
static settings::Bool scout_mode{ "navbot.scout-mode", "false" };
static settings::Bool primary_only{ "navbot.primary-only", "true" };
static settings::Int jump_distance{ "navbot.jump-distance", "500" };

static settings::Bool target_sentry{ "navbot.target-sentry", "true" };
static settings::Bool stay_near{ "navbot.stay-near", "false" };
static settings::Bool take_tele{ "navbot.take-teleporters", "true" };
void rvarCallback(settings::VariableBase<bool> &var, bool after)
{
    if (!after)
        return;
    if (&var == &spy_mode)
    {
        scout_mode = false;
        heavy_mode = false;
    }
    else if (&var == &heavy_mode)
    {
        spy_mode = false;
        scout_mode = false;
    }
    else if (&var == &scout_mode)
    {
        spy_mode = false;
        heavy_mode = false;
    }
}
static InitRoutine init([]() {
    spy_mode.installChangeCallback(rvarCallback);
    scout_mode.installChangeCallback(rvarCallback);
    heavy_mode.installChangeCallback(rvarCallback);
});
bool HasLowAmmo()
{
    int *weapon_list =
        (int *) ((uint64_t)(RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
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
CNavArea *GetClosestToNav(Vector vec)
{
    CNavArea *target = nullptr;
    float bestscr = FLT_MAX;
    for (auto &i : nav::navfile->m_areas)
        if (i.m_center.DistTo(vec) < bestscr)
        {
            target = &i;
            bestscr = i.m_center.DistTo(vec);
        }
    return target;
}
CachedEntity *NearestEnemy()
{
    if (last_tar != -1 && CE_GOOD(ENTITY(last_tar)) &&
        ENTITY(last_tar)->m_bAlivePlayer() &&
        ENTITY(last_tar)->m_iTeam() != LOCAL_E->m_iTeam() && nav::isSafe(GetClosestToNav(ENTITY(last_tar)->m_vecOrigin())))
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

bool IsGood(CachedEntity *ent, Vector vec, float mindist, float maxdist, bool nearest)
{
    bool entchecks = false;
    if (ent)
        entchecks = true;
    if (!nav::isSafe(GetClosestToNav(vec)))
        return false;
    Vector tmp_vec = vec;
    tmp_vec.z += 48.0f;
    if (entchecks)
    {
        Vector ent_orig = ent->m_vecOrigin();
        float min_tol = mindist;
        float max_tol = maxdist;
        if (nearest)
            max_tol /= 2;
        else
            min_tol *= 2;
        ent_orig += 48.0f;
        if (ent_orig.DistTo(tmp_vec) < min_tol || ent_orig.DistTo(vec) > max_tol)
            return false;
        if (!IsVectorVisible(tmp_vec, ent_orig, true))
            return false;
    }
    return true;
}

CNavArea *FindNearestValidByDist(Vector vec, float mindist, float maxdist,
                                 bool nearest)
{
    float best_scr      = nearest ? FLT_MAX : 0.0f;
    CNavArea *best_area = nullptr;
    Vector tmp_vec = vec;
    tmp_vec.z += 48.0f;
    for (auto &i : nav::navfile->m_areas)
    {
        float score = i.m_center.DistTo(vec);
        if (score > maxdist || score < mindist)
            continue;
        Vector tovischeck = i.m_center;
        tovischeck.z += 48.0f;
        if (!IsVectorVisible(tmp_vec, tovischeck, true))
            continue;
        if (!nav::isSafe(&i))
            continue;
        if (nearest)
        {
            if (score < best_scr)
            {
                best_scr  = score;
                best_area = &i;
            }
        }
        else
        {
            if (score > best_scr)
            {
                best_scr  = score;
                best_area = &i;
            }
        }
    }
    return best_area;
}

static Timer ammo_health_cooldown{};
static Timer init_timer{};
static Timer nav_cooldown{};
static Timer engi_spot_cd{};
static Timer nav_enemy_cd{};
static Timer jump_cooldown{};
static std::vector<Vector> preferred_sniper_spots;
static std::vector<Vector> sniper_spots;
static std::vector<Vector> nest_spots;
Vector Best_Spot{ 0.0f, 0.0f, 0.0f };
void Init()
{
    sniper_spots.clear();
    preferred_sniper_spots.clear();
    nest_spots.clear();
    for (auto &area : nav::navfile->m_areas)
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
static std::unordered_map<int, bool> disabled_spot{};
static std::unordered_map<int, Timer> disabled_cooldown{};
std::unordered_map<int, int> priority_spots;
void initonce()
{
    priority_spots.clear();
    ammo_health_cooldown.update();
    init_timer.update();
    nav_cooldown.update();
    engi_spot_cd.update();
    sniper_spots.clear();
    preferred_sniper_spots.clear();
    disabled_cooldown.clear();
    disabled_spot.clear();
    return;
}

static Timer spot_timer{};
#define retnull                                                                \
    {                                                                          \
        Best_Spot = { 0.0f, 0.0f, 0.0f };                                      \
        return;                                                                \
    }

struct ent_info
{
    Vector origin{};
    int team{};
    int clazz{};
    int idx{};
};

void UpdateBestSpot()
{
    if (!spot_timer.test_and_set(10000))
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        retnull;
    int ent_count = 0;
    std::unordered_map<int, ent_info> stored_pos;
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer())
            continue;
        stored_pos[i].origin = ent->m_vecOrigin();
        stored_pos[i].team   = ent->m_iTeam();
        stored_pos[i].clazz  = g_pPlayerResource->GetClass(ent);
        stored_pos[i].idx    = i;
    }
    // Get dist from each ent to another
    std::unordered_map<std::pair<int, int>, float,
                       boost::hash<std::pair<int, int>>>
        distances;
    for (auto &i : stored_pos)
        for (auto &j : stored_pos)
            distances[{ i.second.idx, j.second.idx }] =
                i.second.origin.DistTo(j.second.origin);
    Vector prefferred_spot{};
    float score_max    = FLT_MAX;
    bool must_be_enemy = false;
    for (auto &i : distances)
    {
        // Mark self :thinking:
        if (stored_pos[i.first.first].idx == LOCAL_E->m_IDX ||
            stored_pos[i.first.second].idx == LOCAL_E->m_IDX)
            continue;
        // Not for us
        if (stored_pos[i.first.first].team != stored_pos[i.first.second].team)
            continue;
        Vector pos1 = stored_pos[i.first.first].origin;
        Vector pos2 = stored_pos[i.first.second].origin;

        // Bot please don't get stuck midair thanks
        if (fabsf(pos1.z - pos2.z) > 70)
            continue;

        // Get Pos inbetween the two coords.
        Vector estimated_pos = (pos1 + pos2) / 2;
        bool enemy = stored_pos[i.first.first].team != LOCAL_E->m_iTeam();
        if (must_be_enemy && !enemy)
            continue;
        // Prevent lag thru vischecks
        if (must_be_enemy && !enemy)
            if ((!i.second || (i.second > score_max && !score_max)))
                continue;
        CNavArea *issafe = GetClosestToNav(estimated_pos);
        if (!issafe || !nav::isSafe(issafe))
            continue;
        // Vischecks.club
        if (!IsVectorVisible(pos1, estimated_pos, true) ||
            !IsVectorVisible(pos2, estimated_pos, true))
            continue;

        // Distance check thing
        if ((i.second && (i.second < score_max || !score_max )) || (enemy && !must_be_enemy))
        {
            score_max = i.second;
            if (enemy)
                must_be_enemy = true;
            prefferred_spot = estimated_pos;
        }
    }
    if (!prefferred_spot.IsZero())
        Best_Spot = prefferred_spot;
    else
        retnull;

}
static CatCommand update_spot_debug{"navbot_debug_spotupdate", "debug", [](){
        UpdateBestSpot();
}};
static CatCommand nav_best_spot{"navbot_debug_navbest", "debug", [](){
        nav::navTo(Best_Spot);
}};
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
            if (*spy_mode)
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
    static CNavArea *last_area = nullptr;
    CachedEntity *Sentry = nearestSentry();
    if (CE_BAD(Sentry))
        return false;
    CNavArea *area = FindNearestValidByDist(GetBuildingPosition(Sentry),
                                            1100.0f, 2000.0f, false);
    if (!area || (area == last_area && !nav::ReadyForCommands))
        return false;
    if (nav::navTo(area->m_center, priority, true, false))
        return true;
    return false;
}
static Vector lastgoal{ 0, 0, 0 };
int lastent = -1;

bool NavToEnemy()
{
    bool toret = true;
    static CNavArea *last_area = nullptr;
    if (*stay_near || *heavy_mode || *scout_mode)
    {
        if (lastent != -1)
        {
            CachedEntity *ent = ENTITY(lastent);
            if (CE_BAD(ent) || !ent->m_bAlivePlayer() ||
                ent->m_iTeam() == LOCAL_E->m_iTeam())
            {
                lastent = -1;
                if (lastgoal.x > 1.0f || lastgoal.x < -1.0f)
                {
                    if (nav::ReadyForCommands || nav::curr_priority != 1337)
                        toret = nav::navTo(lastgoal, 1337, true, false);
                    lastgoal = {};
                    return toret;
                }
            }
            else
            {
                CNavArea *area = nullptr;
                float mindist = 300.0f;
                float maxdist = 3000.0f;
                bool nearest = false;
                if (*heavy_mode || *scout_mode)
                {
                    mindist = 200.0f;
                    maxdist = 1000.0f;
                    nearest = true;
                }
                area = FindNearestValidByDist(ent->m_vecOrigin(), mindist, maxdist,
                                              nearest);
                if (area)
                {
                    if ((area == last_area || (last_area && IsGood(ent, last_area->m_center, mindist, maxdist, nearest))) && !nav::ReadyForCommands)
                        return true;
                    toret = nav::navTo(area->m_center, 1337, true, false);
                    lastgoal = area->m_center;
                    last_area = area;
                    lastent  = ent->m_IDX;
                    return toret;
                }
                else if ((lastgoal.x > 1.0f || lastgoal.x < -1.0f) &&
                         lastgoal.DistTo(LOCAL_E->m_vecOrigin()) > 200.0f)
                {
                    if (nav::ReadyForCommands || nav::curr_priority != 1337)
                        toret = nav::navTo(lastgoal, 1337, true, false);
                    lastgoal = { 0, 0, 0 };
                    return toret;
                }
                else
                {
                    lastgoal = { 0, 0, 0 };
                    lastent  = -1;
                }
            }
        }

        auto ent = NearestEnemy();
        if (CE_GOOD(ent))
        {
            CNavArea *area = nullptr;
            float mindist = 200.0f;
            float maxdist = 3000.0f;
            bool nearest = false;
            if (*heavy_mode || *scout_mode)
            {
                mindist = 200.0f;
                maxdist = 1000.0f;
                nearest = true;
            }
            area = FindNearestValidByDist(ent->m_vecOrigin(), mindist, maxdist,
                                          nearest);
            if (area)
            {
                if ((area == last_area || (last_area && IsGood(ent, last_area->m_center, mindist, maxdist, nearest))) && !nav::ReadyForCommands)
                    return true;
                toret = nav::navTo(area->m_center, 1337, true, false);
                lastgoal = area->m_center;
                lastent  = ent->m_IDX;
                last_area = area;
                return toret;
            }
            else if ((lastgoal.x > 1.0f || lastgoal.x < -1.0f) &&
                     lastgoal.DistTo(LOCAL_E->m_vecOrigin()) > 200.0f)
            {
                if (nav::ReadyForCommands || nav::curr_priority != 1337)
                    toret = nav::navTo(lastgoal, 1337, true, false);
                lastgoal = { 0, 0, 0 };
                return toret;
            }
            else
            {
                lastgoal = { 0, 0, 0 };
                lastent  = -1;
            }
        }
        else if ((lastgoal.x > 1.0f || lastgoal.x < -1.0f) &&
                 lastgoal.DistTo(LOCAL_E->m_vecOrigin()) > 200.0f)
        {
            if (nav::ReadyForCommands || nav::curr_priority != 1337)
                toret = nav::navTo(lastgoal, 1337, true, false);
            lastgoal = { 0, 0, 0 };
            return toret;
        }
        else
        {
            lastgoal = { 0, 0, 0 };
            lastent  = -1;
        }
    }
    return false;
}
static Timer nav_to_spot{};
bool NavToSniperSpot(int priority)
{
    Vector random_spot{};
    if (!sniper_spots.size() && !preferred_sniper_spots.size())
        return false;
    if (!nav::ReadyForCommands)
        return false;
    if (!nav_to_spot.test_and_set(100))
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
            if (disabled_spot[i])
            {
                if (!disabled_cooldown[i].test_and_set(5000))
                    continue;
                else
                    disabled_spot[i] = false;
            }
            if ((priority_spots[i] < lowest_priority))
                lowest_priority = priority_spots[i];
        }
        for (int i = 0; i < snip_spot.size(); i++)
        {
            if (disabled_spot[i])
            {
                if (!disabled_cooldown[i].test_and_set(5000))
                    continue;
                else
                    disabled_spot[i] = false;
            }
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
        {
            snip_spot   = sniper_spots;
            int rng     = rand() % snip_spot.size();
            random_spot = snip_spot.at(rng);
            if (random_spot.z)
            {
                nav_to_spot.update();
                return nav::navTo(random_spot, false, true, priority);
            }
            return false;
        }
        random_spot = snip_spot.at(best_spot);
        if (random_spot.z)
            toret = nav::navTo(random_spot, false, true, priority);
        if (!toret)
        {
            disabled_spot[best_spot] = true;
            disabled_cooldown[best_spot].update();
        }
        priority_spots[best_spot]++;
    }
    else if (!snip_spot.empty())
    {
        int rng     = rand() % snip_spot.size();
        random_spot = snip_spot.at(rng);
        if (random_spot.z)
        {
            nav_to_spot.update();
            toret = nav::navTo(random_spot, priority, true, false);
        }
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
        "%d %u %d %d %f %f %d %f %f %f", CE_INT(ent, netvar.m_iObjectType),
        CE_BYTE(ent, netvar.m_bBuilding), CE_INT(ent, netvar.m_iTeleState),
        CE_INT(ent, netvar.m_bMatchBuilding),
        CE_FLOAT(ent, netvar.m_flTeleRechargeTime),
        CE_FLOAT(ent, netvar.m_flTeleCurrentRechargeDuration),
        CE_INT(ent, netvar.m_iTeleTimesUsed),
        CE_FLOAT(ent, netvar.m_flTeleYawToExit), g_GlobalVars->curtime,
        g_GlobalVars->curtime * g_GlobalVars->interval_per_tick);
});

static CatCommand debug_ammo("navbot_debug_ammo", "debug", []() {
    if (CE_BAD(LOCAL_W) || CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    logging::Info("Clip size: %d %d", CE_INT(LOCAL_W, netvar.m_iClip1),
                  CE_INT(LOCAL_W, netvar.m_iClip2));
    for (int i = 0; i < 8; i++)
        logging::Info("Ammo Table IDX %d: %d", i,
                      CE_INT(LOCAL_E, netvar.m_iAmmo + 4 * i));
});

static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "NavBot", 16, []() {
        if (!enable || !nav::prepare())
            return;
        if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || CE_BAD(LOCAL_W))
            return;
        if (primary_only)
            UpdateSlot();
        UpdateBestSpot();
        if (*scout_mode && jump_cooldown.test_and_set(200))
        {
            auto ent = NearestEnemy();
            if (CE_GOOD(ent))
            {
                if (ent->m_flDistance() < *jump_distance)
                    current_user_cmd->buttons |= IN_JUMP;
            }
        }
        if (HasLowHealth() && ammo_health_cooldown.test_and_set(5000))
        {
            CachedEntity *med = nearestHealth();
            if (CE_GOOD(med))
            {
                if (nav::curr_priority == 1337)
                    nav::clearInstructions();
                Vector tonav = med->m_vecOrigin();
                tonav.z += 15.0f;
                nav::navTo(tonav, 7, true, true);
            }
        }
        if (HasLowAmmo() && ammo_health_cooldown.test_and_set(5000))
        {
            CachedEntity *ammo = nearestAmmo();
            if (CE_GOOD(ammo))
            {
                if (nav::curr_priority == 1337)
                    nav::clearInstructions();
                Vector tonav = ammo->m_vecOrigin();
                tonav.z += 15.0f;
                nav::navTo(tonav, 6, true, true);
            }
        }
        if ((!HasLowHealth() && nav::curr_priority == 7) ||
            (!HasLowAmmo() && nav::curr_priority == 6))
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
                        CE_INT(ent, netvar.m_iTeleState) == 2 &&
                        CE_FLOAT(ent, netvar.m_flTeleRechargeTime) <
                            g_GlobalVars->curtime)
                    {
                        waittime = 1000;
                        nav_cooldown.update();
                        if (nav::curr_priority == 1337)
                            nav::clearInstructions();
                        nav::navTo(GetBuildingPosition(ent), 5, false, false);
                    }
            }
        }
        if ((*stay_near || *heavy_mode || *scout_mode) && nav_enemy_cd.test_and_set(100) && !HasLowAmmo() &&
            !HasLowHealth())
        {
            if (NavToEnemy())
                return;
            else if (nav::ReadyForCommands)
                NavToSniperSpot(5);
        }
        if (enable)
        {
            if (!nav::ReadyForCommands && !spy_mode && !heavy_mode &&
                !scout_mode)
                nav_cooldown.update();
            if (target_sentry && NavToSentry(3))
                return;
            bool isready = (spy_mode || heavy_mode || scout_mode)
                               ? true
                               : nav::ReadyForCommands;
            if (isready && nav_cooldown.test_and_set(waittime))
            {
                waittime =
                    /*(spy_mode || heavy_mode || engi_mode) ? 100 : 2000*/ 0;
                if (!spy_mode && !heavy_mode && !scout_mode)
                {
                    nav_cooldown.update();
                    if (init_timer.test_and_set(5000))
                        Init();
                    if (!NavToSniperSpot(5))
                        waittime = 1;
                }
                else
                {
                    CachedEntity *tar = NearestEnemy();
                    if (CE_BAD(tar) && last_tar == -1 && nav::ReadyForCommands)
                    {
                        if (init_timer.test_and_set(5000))
                            Init();
                        if (!NavToSniperSpot(4))
                            waittime = 1;
                    }
                    if (CE_GOOD(tar) && *spy_mode)
                    {
                        if (!spy_mode ||
                            !hacks::shared::backtrack::isBacktrackEnabled)
                        {
                            if (!nav::navTo(tar->m_vecOrigin(), 5, false, false))
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
                                if (!nav::navTo(tar->m_vecOrigin(), 5, false,
                                                false))
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
                                !nav::navTo(sorted_ticks[5].entorigin, false,
                                           false))
                                if (!nav::navTo(tar->m_vecOrigin(), 5, true,
                                                false))
                                    last_tar = -1;
                        }
                    }
                }
            }
        }
    });
} // namespace hacks::tf2::NavBot
