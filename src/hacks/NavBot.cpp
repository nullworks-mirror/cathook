#include "common.hpp"
#include "NavBot.hpp"
#include "Backtrack.hpp"
#include "PlayerTools.hpp"
#include "navparser.hpp"
#include "settings/Bool.hpp"
#include "settings/Float.hpp"
#include "HookedMethods.hpp"

// Rvars
static settings::Bool enable{ "navbot.enable", "false" };
static settings::Bool sniper_mode{ "navbot.sniper-mode", "true" };
static settings::Bool heavy_mode{ "navbot.heavy-mode", "false" };
static settings::Bool scout_mode{ "navbot.scout-mode", "false" };
static settings::Float jump_trigger{ "navbot.jump-distance", "500.0f" };
static settings::Bool spy_mode{ "navbot.spy-mode", "false" };
static settings::Bool stay_near{ "navbot.stay-near", "true" };
static settings::Bool path_health{ "navbot.path-health", "true" };
static settings::Bool path_ammo{ "navbot.path-ammo", "true" };
static settings::Bool pick_optimal_slot{ "navbot.best-slot", "true" };
static settings::Bool take_tele{ "navbot.take-teleporter", "false" };

// Timers
static Timer general_cooldown{};
static Timer init_cooldown{};
static Timer path_health_cooldown{};
static Timer path_ammo_cooldown{};
static Timer refresh_nearest_target{};
static Timer refresh_nearest_valid_vector{};
static Timer nav_to_nearest_enemy_cooldown{};
static Timer slot_timer{};
static Timer non_sniper_sniper_nav_cooldown{};
static Timer jump_cooldown{};
static Timer teleporter_cooldown{};
static Timer teleporter_find_cooldown{};
static Timer nav_timeout{};
static Timer stay_near_timeout{};
static Timer near_nav_timer{};

// Vectors
static std::vector<Vector *> default_spots{};
static std::vector<Vector *> preferred_spots{};

// Unordered_maps
static std::unordered_map<int, int> preferred_spots_priority{};

static bool inited = false;

namespace hacks::shared::NavBot
{

// Main Functions
void Init(bool from_LevelInit)
{
    if (!*enable)
    {
        inited = false;
        return;
    }
    if (from_LevelInit)
    {
        inited = false;
        default_spots.clear();
        preferred_spots.clear();
        preferred_spots_priority.clear();
    }
    if (!nav::prepare())
        return;
    if (!from_LevelInit)
    {
        default_spots.clear();
        preferred_spots.clear();
        preferred_spots_priority.clear();
    }
    inited = true;
    for (auto &i : nav::navfile.get()->m_areas)
    {
        if (!i.m_hidingSpots.empty())
            for (auto &j : i.m_hidingSpots)
                default_spots.push_back(&j.m_pos);
        if (i.m_attributeFlags & NAV_MESH_NO_HOSTAGES)
            preferred_spots.push_back(&i.m_center);
    }
}

static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "NavBot", 18, []() {
        // Master Switch

        if (!*enable)
            return;

        // init pls
        if (!inited)
        {
            if (init_cooldown.test_and_set(10000))
                Init(false);
            return;
        }
        // no nav file loaded/inited
        if (!nav::prepare())
            return;
        // Timeout boys
        if (!nav_timeout.check(2000))
            return;

        if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
            return;
        // Health and ammo have highest priority, but the stay near priorities has to be this high to override anything else
        if (nav::curr_priority == 1337 && ((HasLowAmmo() && *path_ammo) || (HasLowHealth() && *path_health)))
            nav::clearInstructions();

        // Get health if low
        if (path_health && HasLowHealth() &&
            path_health_cooldown.test_and_set(500))
        {
            CachedEntity *health = nearestHealth();
            if (CE_GOOD(health))
                nav::navTo(health->m_vecOrigin(), 7, false, false);
        }

        // Get Ammo if low on ammo
        if (path_ammo && HasLowAmmo() && path_ammo_cooldown.test_and_set(500))
        {
            CachedEntity *ammo = nearestAmmo();
            if (CE_GOOD(ammo))
                nav::navTo(ammo->m_vecOrigin(), 6, false, false);
        }
        // Stop pathing for ammo/Health if problem resolved
        if ((!HasLowAmmo() && nav::curr_priority == 6) || ( nav::curr_priority == 7 && !HasLowHealth()))
            nav::clearInstructions();

        // Take Teleporter
        if (*take_tele && nav::curr_priority != 6 && nav::curr_priority != 7 && teleporter_cooldown.test_and_set(200))
        {
            CachedEntity *ent = nearestTeleporter();
            if (CE_GOOD(ent) && ent->m_flDistance() <= 300.0f)
                if (nav::navTo(ent->m_vecOrigin(), 4, false, false))
                    return;

        }
        // If Zoning enabled then zone enemy
        if (stay_near && nav_to_nearest_enemy_cooldown.test_and_set(*spy_mode ? 100 : 1000) && !*spy_mode)
            NavToNearestEnemy();

        // Prevent path spam on sniper bots
        if (CanPath())
        {
            if (*sniper_mode)
                NavToSniperSpot(5);
            else if ((*heavy_mode || scout_mode) && nav_to_nearest_enemy_cooldown.test_and_set(*spy_mode ? 100 : 1000))
            {
                if (!NavToNearestEnemy() && non_sniper_sniper_nav_cooldown.test_and_set(10000))
                    NavToSniperSpot(5);
            }
            else if (*spy_mode && nav_to_nearest_enemy_cooldown.test_and_set(100))
                    if (!NavToBacktrackTick(5) && non_sniper_sniper_nav_cooldown.test_and_set(10000))
                        NavToSniperSpot(5);
        }
        if (*pick_optimal_slot)
            UpdateSlot();
        if (*scout_mode)
            Jump();
    });

// Helpers

bool CanPath()
{
    if (nav::curr_priority == 4 || nav::curr_priority == 6 || nav::curr_priority == 7)
        return false;
    if ((*heavy_mode || *spy_mode || *scout_mode) && general_cooldown.test_and_set(100))
        return true;
    else if (sniper_mode)
    {
        if (nav::ReadyForCommands && general_cooldown.test_and_set(100))
            return true;
        return false;
    }
    return false;
}

bool HasLowHealth()
{
    return float(LOCAL_E->m_iHealth()) / float(LOCAL_E->m_iMaxHealth()) < 0.64;
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

static int last_target = -1;
std::pair<CachedEntity *, int> nearestEnemy()
{
    if (refresh_nearest_target.test_and_set(10000))
    {
        CachedEntity *best_tar = nullptr;
        float best_dist        = FLT_MAX;
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy() ||
                ent == LOCAL_E ||
                player_tools::shouldTarget(ent) !=
                    player_tools::IgnoreReason::DO_NOT_IGNORE)
                continue;
            if (ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin()) < best_dist)
            {
                best_dist = ent->m_vecOrigin().DistTo(LOCAL_E->m_vecOrigin());
                best_tar  = ent;
            }
        }
        if (CE_GOOD(best_tar))
        {
            last_target = best_tar->m_IDX;
            return {best_tar, best_tar->m_IDX};
        }
        else
        {
            last_target = -1;
            return {nullptr, last_target};
        }
    }
    else
    {
        if (last_target == -1)
            return {nullptr, last_target};
        else
        {
            CachedEntity *ent = ENTITY(last_target);
            if (((CE_GOOD(ent) && ent->m_bAlivePlayer()) && (!ent->m_bEnemy() ||
                ent == LOCAL_E ||
                player_tools::shouldTarget(ent) !=
                    player_tools::IgnoreReason::DO_NOT_IGNORE)) || (CE_GOOD(ent) && !ent->m_bAlivePlayer()))
            {
                last_target = -1;
                return {nullptr, last_target};
            }
            else
                return {ent, last_target};
        }
    }
}

static Vector cached_vector{0.0f, 0.0f, 0.0f};
static Vector empty{0.0f, 0.0f, 0.0f};

static int last_tar = -1;
Vector GetClosestValidByDist(CachedEntity *ent, int idx, float mindist, float maxdist, bool near)
{
    if (refresh_nearest_valid_vector.test_and_set(*spy_mode ? 100 : 2000) && ( idx == -1 || idx != last_tar || CE_GOOD(ent)))
    {
        Vector best_area{0.0f, 0.0f, 0.0f};
        float best_dist = near ? FLT_MAX : 0.0f;
        for (auto &i : nav::navfile->m_areas)
        {
            if (!nav::isSafe(&i))
                continue;
            Vector center = i.m_center;
            float dist    = center.DistTo(ent->m_vecOrigin());

            // Check if wihin the range specified
            if (dist > maxdist || dist < mindist)
                continue;

            // We want to be standing the closest to them possible, besides as a sniper
            if ((near && dist > best_dist) || (!near && dist < best_dist))
                continue;

            // Anti stuck in ground stuff
            Vector zcheck = center;
            zcheck.z += 48;

            // Anti stuck in ground
            Vector zent = ent->m_vecOrigin();
            zent += 48;
            if (!IsVectorVisible(zcheck, zent, false))
                continue;
            best_area = i.m_center;
            best_dist = dist;
        }
        if (best_area.IsValid() && best_area.z )
        {
            cached_vector = best_area;
            last_tar = ent->m_IDX;
            return best_area;
        }
        last_tar = -1;
        return empty;
    }
    // We Want a bit of caching
    else
        return cached_vector;
}

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

void Jump()
{
    std::pair<CachedEntity *, int> enemy_pair{nullptr, -1};
    enemy_pair = nearestEnemy();
    CachedEntity *ent = enemy_pair.first;
    if (CE_BAD(ent))
        return;
    if (ent->m_flDistance() < *jump_trigger && jump_cooldown.test_and_set(200))
        current_user_cmd->buttons |= IN_JUMP;
}

static int teleporter = -1;
CachedEntity *nearestTeleporter()
{
    if (teleporter_find_cooldown.test_and_set(1000))
    {
        float closest = FLT_MAX;
        CachedEntity *tele = nullptr;
        for (int i = 0; i < HIGHEST_ENTITY; i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent) || ent->m_bEnemy() || ent->m_iClassID() != CL_CLASS(CObjectTeleporter))
                continue;
            if (CE_FLOAT(ent, netvar.m_flTeleYawToExit) && CE_FLOAT(ent, netvar.m_flTeleRechargeTime) && CE_FLOAT(ent, netvar.m_flTeleRechargeTime) < g_GlobalVars->curtime)
            {
                float score = ent->m_flDistance();
                if (score < closest)
                {
                    tele = ent;
                    closest = score;
                    nav_timeout.update();
                }
            }
        }
        if (tele)
            teleporter = tele->m_IDX;
        return tele;
    }
    else
    {
        if (teleporter == -1)
            return nullptr;
        CachedEntity *ent = ENTITY(teleporter);
        if (CE_BAD(ent) || ent->m_bEnemy() || ent->m_iClassID() != CL_CLASS(CObjectTeleporter))
        {
            teleporter = -1;
            return nullptr;
        }
        return ent;
    }
    // Wtf Call the police
    return nullptr;
}
CNavArea *GetNavArea(Vector loc)
{
    float best_scr = FLT_MAX;
    CNavArea *to_ret = nullptr;
    for (auto &i : nav::navfile->m_areas)
    {
        float score = i.m_center.DistTo(loc);
        if (score < best_scr)
        {
            best_scr = score;
            to_ret = &i;
        }
    }
    return to_ret;
}

// Navigation
bool NavToSniperSpot(int priority)
{
    // Already pathing currently and priority is below the wanted, so just
    // return
    if (priority < nav::curr_priority)
        return true;
    // Preferred yay or nay?
    bool use_preferred = preferred_spots.empty() ? false : true;
    if (!use_preferred && default_spots.empty())
        return false;

    std::vector<Vector *> *sniper_spots =
        use_preferred ? &preferred_spots : &default_spots;

    // Wtf Nullptr!
    if (!sniper_spots)
        return false;
    if (use_preferred)
    {
        // Store Lowest Matches for later, will be useful
        std::vector<unsigned> matches{};

        // Priority INT_MAX, not like you'll exceed it anyways lol
        int lowest_priority = INT_MAX;
        for (unsigned i = 0; i < sniper_spots->size(); i++)
        {
            if (preferred_spots_priority[i] < lowest_priority)
            {
                lowest_priority = preferred_spots_priority[i];
                matches.clear();
                matches.push_back(i);
            }
            else if (preferred_spots_priority[i] == lowest_priority)
                matches.push_back(i);
        }
        if (!matches.empty())
        {
            // Cant exceed.club
            float min_dist = FLT_MAX;

            // Best Spot to nav to
            int best_spot = -1;

            for (auto idx : matches)
            {
                Vector *match = sniper_spots->at(matches.at(0));

                // Score of the match
                float score = match->DistTo(LOCAL_E->m_vecOrigin());

                if (score < min_dist)
                {
                    min_dist  = score;
                    best_spot = idx;
                }
            }

            // return if no spot found
            if (best_spot == -1)
                return false;
            // Make spot less important
            preferred_spots_priority[best_spot]++;
            // Nav to Spot
            return nav::navTo(*sniper_spots->at(best_spot), priority);
        }
    }
    else
    {
        // Get Random Sniper Spot
        unsigned index      = unsigned(std::rand()) % sniper_spots->size();
        Vector *random_spot = sniper_spots->at(index);

        // Nav to Spot
        return nav::navTo(*random_spot, priority);
    }
}
bool CanNavToNearestEnemy()
{
    if (HasLowAmmo() || HasLowHealth() || near_nav_timer.test_and_set(*spy_mode ? 100 : 1000))
        return false;
    return true;
}
bool NavToNearestEnemy()
{
    if (!CanNavToNearestEnemy())
        return false;
    std::pair<CachedEntity *, int> enemy_pair{nullptr, -1};
    enemy_pair = nearestEnemy();
    CachedEntity *ent = enemy_pair.first;
    int ent_idx = enemy_pair.second;
    if ((CE_BAD(ent) && ent_idx == -1) || (CE_GOOD(ent) && !ent->m_bAlivePlayer()))
        return false;
    if (nav::curr_priority == 6 || nav::curr_priority == 7  || nav::curr_priority == 4)
        return false;
    float min_dist = 800.0f;
    float max_dist = 4000.0f;
    bool near = *sniper_mode ? false : true;
    if (*heavy_mode || *scout_mode)
    {
        min_dist = 100.0f;
        max_dist = 1000.0f;
    }
    Vector to_nav = GetClosestValidByDist(ent, ent_idx, min_dist, max_dist, near);
    if (to_nav.z)
        return nav::navTo(to_nav, 1337, false, false); 
    else if (CE_GOOD(ent) && ent->m_bAlivePlayer() && nav::isSafe(GetNavArea(ent->m_vecOrigin())))
        return nav::navTo(ent->m_vecOrigin(), 1337, false, false);
    return false;
}

static bool first_unready = true;
bool NavToBacktrackTick(int priority)
{
    CachedEntity *ent = nearestEnemy().first;
    if (CE_BAD(ent))
        return false;
    if (first_unready && !nav::ReadyForCommands && *sniper_mode && nav::curr_priority == 1337)
    {
        first_unready = false;
        stay_near_timeout.update();
    }
    else if (nav::ReadyForCommands)
        first_unready = true;
    if (stay_near_timeout.test_and_set(5000) && !nav::ReadyForCommands && *sniper_mode && nav::curr_priority == 1337)
        return true;
    // Health and ammo are more important
    if (nav::curr_priority == 6 || nav::curr_priority == 7)
        return false;
    // Just backtrack data
    auto unsorted_ticks = hacks::shared::backtrack::
        headPositions[ent->m_IDX];
    // Vector needed for later
    std::vector<hacks::shared::backtrack::BacktrackData>
        sorted_ticks;

    // Only use good ticks
    for (int i = 0; i < 66; i++)
    {
        if (hacks::shared::backtrack::ValidTick(
                unsorted_ticks[i], ent))
            sorted_ticks.push_back(unsorted_ticks[i]);
    }
    // Nav to Ent origin if everything falls flat
    if (sorted_ticks.empty())
    {
        if (nav::navTo(ent->m_vecOrigin(), priority, false, false))
            return true;
        return false;
    }
    // Sort by tickcount
    std::sort(
        sorted_ticks.begin(), sorted_ticks.end(),
        [](const hacks::shared::backtrack::BacktrackData
               &a,
           const hacks::shared::backtrack::BacktrackData
               &b) {
            return a.tickcount > b.tickcount;
    });

    // Get the 5th tick and path to it, better than pathing to the last tick since the bot may just lag behind and never reach it
    if (!sorted_ticks[5].tickcount ||
        !nav::navTo(sorted_ticks[5].entorigin, priority, false,
                   false))
        if (!nav::navTo(ent->m_vecOrigin(), priority, false))
            return false;
    return true;
}

} // namespace hacks::shared::NavBot
