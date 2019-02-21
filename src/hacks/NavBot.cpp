#include "common.hpp"
#include "navparser.hpp"
#include "NavBot.hpp"
#include "PlayerTools.hpp"
#include "Aimbot.hpp"
#include "FollowBot.hpp"

namespace hacks::tf2::NavBot
{
// -Rvars-
static settings::Bool enabled("navbot.enabled", "false");
static settings::Bool stay_near("navbot.stay-near", "true");
static settings::Bool heavy_mode("navbot.other-mode", "false");
static settings::Bool get_health("navbot.get-health-and-ammo", "true");
static settings::Float jump_distance("navbot.autojump.trigger-distance", "300");
static settings::Bool autojump("navbot.autojump.enabled", "false");
static settings::Bool primary_only("navbot.primary-only", "true");

// -Forward declarations-
bool init(bool first_cm);
static bool navToSniperSpot();
static bool stayNear();
static bool getHealthAndAmmo();
static void autoJump();
static void updateSlot();
using task::current_task;

// -Variables-
static std::vector<std::pair<CNavArea *, Vector>> sniper_spots;
// How long should the bot wait until pathing again?
static Timer wait_until_path{};
// What is the bot currently doing
namespace task
{
task current_task;
}
constexpr bot_class_config DIST_OTHER{ 100.0f, 200.0f, 300.0f };
constexpr bot_class_config DIST_SNIPER{ 1000.0f, 1500.0f, 3000.0f };

static void CreateMove()
{
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || !LOCAL_E->m_bAlivePlayer())
        return;
    if (!init(false))
        return;
    if (!nav::ReadyForCommands || current_task == task::followbot)
        wait_until_path.update();
    else
        current_task = task::none;

    if (autojump)
        autoJump();
    if (primary_only)
        updateSlot();

    if (get_health)
        if (getHealthAndAmmo())
            return;
    // Try to stay near enemies to increase efficiency
    if ((stay_near || heavy_mode) && current_task != task::followbot)
        if (stayNear())
            return;
    // We don't have anything else to do. Just nav to sniper spots.
    if (navToSniperSpot())
        return;
    // Uhh... Just stand arround I guess?
}

bool init(bool first_cm)
{
    static bool inited = false;
    if (first_cm)
        inited = false;
    if (!enabled)
        return false;
    if (!nav::prepare())
        return false;
    if (!inited)
    {
        sniper_spots.clear();
        // Add all sniper spots to vector
        for (auto &area : nav::navfile->m_areas)
        {
            for (auto hide : area.m_hidingSpots)
                if (hide.IsGoodSniperSpot() || hide.IsIdealSniperSpot() || hide.IsExposed())
                    sniper_spots.emplace_back(&area, hide.m_pos);
        }
        inited = true;
    }
    return true;
}

static bool navToSniperSpot()
{
    // Don't path if you already have commands. But also don't error out.
    if (!nav::ReadyForCommands || current_task != task::none)
        return true;
    // Wait arround a bit before pathing again
    if (!wait_until_path.check(2000))
        return false;
    // Max 10 attempts
    for (int attempts = 0; attempts < 10; attempts++)
    {
        // Get a random sniper spot
        auto random = select_randomly(sniper_spots.begin(), sniper_spots.end());
        // Check if spot is considered safe (no sentry, no sticky)
        if (!nav::isSafe(random.base()->first))
            continue;
        // Try to nav there
        if (nav::navTo(random.base()->second, 5, true, true, false))
        {
            current_task = task::sniper_spot;
            return true;
        }
    }
    return false;
}

static std::pair<CachedEntity *, float> getNearestPlayerDistance()
{
    float distance         = FLT_MAX;
    CachedEntity *best_ent = nullptr;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_GOOD(ent) && ent->m_bAlivePlayer() && ent->m_bEnemy() && g_pLocalPlayer->v_Origin.DistTo(ent->m_vecOrigin()) < distance && player_tools::shouldTarget(ent) && (!hacks::shared::aimbot::ignore_cloak || !IsPlayerInvisible(ent)) && VisCheckEntFromEnt(LOCAL_E, ent))
        {
            distance = g_pLocalPlayer->v_Origin.DistTo(ent->m_vecOrigin());
            best_ent = ent;
        }
    }
    return { best_ent, distance };
}

namespace stayNearHelpers
{
// Check if the location is close enough/far enough and has a visual to target
static bool isValidNearPosition(Vector vec, Vector target, const bot_class_config &config)
{
    vec.z += 40;
    target.z += 40;
    float dist = vec.DistTo(target);
    if (dist < config.min || dist > config.max)
        return false;
    if (!IsVectorVisible(vec, target, true))
        return false;
    return true;
}

// Returns true if began pathing
static bool stayNearPlayer(CachedEntity *&ent, const bot_class_config &config, CNavArea *&result)
{
    // Get some valid areas
    std::vector<CNavArea *> areas;
    for (auto &area : nav::navfile->m_areas)
    {
        if (!isValidNearPosition(area.m_center, ent->m_vecOrigin(), config))
            continue;
        areas.push_back(&area);
    }
    if (areas.empty())
        return false;

    const Vector ent_orig = ent->m_vecOrigin();
    // Area dist to target should be as close as possible to config.preferred
    std::sort(areas.begin(), areas.end(), [&](CNavArea *a, CNavArea *b) { return std::abs(a->m_center.DistTo(ent_orig) - config.preferred) < std::abs(b->m_center.DistTo(ent_orig) - config.preferred); });

    size_t size = 20;
    if (areas.size() < size)
        size = areas.size();

    // Get some areas that are close to the player
    std::vector<CNavArea *> preferred_areas(areas.begin(), areas.end());
    preferred_areas.resize(size / 2);
    if (preferred_areas.empty())
        return false;
    std::sort(preferred_areas.begin(), preferred_areas.end(), [](CNavArea *a, CNavArea *b) { return a->m_center.DistTo(g_pLocalPlayer->v_Origin) < b->m_center.DistTo(g_pLocalPlayer->v_Origin); });

    preferred_areas.resize(size / 4);
    if (preferred_areas.empty())
        return false;
    for (auto &i : preferred_areas)
    {
        if (nav::navTo(i->m_center, 7, true, false))
        {
            result       = i;
            current_task = task::stay_near;
            return true;
        }
    }

    for (size_t attempts = 0; attempts < size / 4; attempts++)
    {
        auto it = select_randomly(areas.begin(), areas.end());
        if (nav::navTo((*it.base())->m_center, 7, true, false))
        {
            result       = *it.base();
            current_task = task::stay_near;
            return true;
        }
    }
    return false;
}

// Loop thru all players and find one we can path to
static bool stayNearPlayers(const bot_class_config &config, CachedEntity *&result_ent, CNavArea *&result_area)
{
    std::vector<CachedEntity *> players;
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy() || !player_tools::shouldTarget(ent) || (hacks::shared::aimbot::ignore_cloak && IsPlayerInvisible(ent)))
            continue;
        players.push_back(ent);
    }
    if (players.empty())
        return false;
    std::sort(players.begin(), players.end(), [](CachedEntity *a, CachedEntity *b) { return a->m_vecOrigin().DistTo(g_pLocalPlayer->v_Origin) < b->m_vecOrigin().DistTo(g_pLocalPlayer->v_Origin); });
    for (auto player : players)
    {
        if (stayNearPlayer(player, config, result_area))
        {
            result_ent = player;
            return true;
        }
    }
    return false;
}
} // namespace stayNearHelpers

// Main stay near function
static bool stayNear()
{
    static CachedEntity *last_target = nullptr;
    static CNavArea *last_area       = nullptr;

    // What distances do we have to use?
    const bot_class_config *config;
    if (heavy_mode)
    {
        config = &DIST_OTHER;
    }
    else
    {
        config = &DIST_SNIPER;
    }

    // Check if someone is too close to us and then target them instead
    std::pair<CachedEntity *, float> nearest = getNearestPlayerDistance();
    if (nearest.first && nearest.first != last_target && nearest.second < config->min)
        if (stayNearHelpers::stayNearPlayer(nearest.first, *config, last_area))
        {
            last_target = nearest.first;
            return true;
        }

    if (current_task == task::stay_near)
    {
        static Timer invalid_area_time{};
        static Timer invalid_target_time{};
        // Do we already have a stay near target? Check if its still good.
        if (CE_GOOD(last_target))
            invalid_target_time.update();
        else
            invalid_area_time.update();
        // Check if we still have LOS and are close enough/far enough
        if (CE_GOOD(last_target) && stayNearHelpers::isValidNearPosition(last_area->m_center, last_target->m_vecOrigin(), *config))
            invalid_area_time.update();

        if (CE_GOOD(last_target) && (!last_target->m_bAlivePlayer() || !last_target->m_bEnemy() || !player_tools::shouldTarget(last_target) || (hacks::shared::aimbot::ignore_cloak && IsPlayerInvisible(last_target))))
        {
            nav::clearInstructions();
            current_task = task::none;
        }
        else if (invalid_area_time.test_and_set(300))
        {
            current_task = task::none;
        }
        else if (invalid_target_time.test_and_set(5000))
        {
            current_task = task::none;
        }
    }
    // Are we doing nothing? Check if our current location can still attack our
    // last target
    if (current_task == task::none && CE_GOOD(last_target) && last_target->m_bAlivePlayer() && last_target->m_bEnemy())
    {
        if (stayNearHelpers::isValidNearPosition(g_pLocalPlayer->v_Origin, last_target->m_vecOrigin(), *config))
            return true;
        // If not, can we try pathing to our last target again?
        if (stayNearHelpers::stayNearPlayer(last_target, *config, last_area))
            return true;
        last_target = nullptr;
    }

    static Timer wait_until_stay_near{};
    if (current_task == task::stay_near)
    {
        return true;
    }
    else if (wait_until_stay_near.test_and_set(1000))
    {
        // We're doing nothing? Do something!
        return stayNearHelpers::stayNearPlayers(*config, last_target, last_area);
    }
    return false;
}

static inline bool hasLowAmmo()
{
    if (CE_BAD(LOCAL_W))
        return false;
    int *weapon_list = (int *) ((uint64_t)(RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
    if (!weapon_list)
        return false;
    if (g_pLocalPlayer->holding_sniper_rifle && CE_INT(LOCAL_E, netvar.m_iAmmo + 4) <= 5)
        return true;
    for (int i = 0; weapon_list[i]; i++)
    {
        int handle = weapon_list[i];
        int eid    = handle & 0xFFF;
        if (eid >= 32 && eid <= HIGHEST_ENTITY)
        {
            IClientEntity *weapon = g_IEntityList->GetClientEntity(eid);
            if (weapon and re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon) && re::C_TFWeaponBase::UsesPrimaryAmmo(weapon) && !re::C_TFWeaponBase::HasPrimaryAmmo(weapon))
                return true;
        }
    }
    return false;
}

static bool getHealthAndAmmo()
{
    static Timer health_ammo_timer{};
    if (!health_ammo_timer.test_and_set(2000))
        return false;
    if (current_task == task::health && static_cast<float>(LOCAL_E->m_iHealth()) / LOCAL_E->m_iMaxHealth() >= 0.64f)
    {
        nav::clearInstructions();
        current_task = task::none;
    }
    if (current_task == task::health)
        return true;

    if (static_cast<float>(LOCAL_E->m_iHealth()) / LOCAL_E->m_iMaxHealth() < 0.64f)
    {
        std::vector<Vector> healthpacks;
        for (int i = 1; i < HIGHEST_ENTITY; i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent))
                continue;
            if (ent->m_ItemType() != ITEM_HEALTH_SMALL && ent->m_ItemType() != ITEM_HEALTH_MEDIUM && ent->m_ItemType() != ITEM_HEALTH_LARGE)
                continue;
            healthpacks.push_back(ent->m_vecOrigin());
        }
        std::sort(healthpacks.begin(), healthpacks.end(), [](Vector &a, Vector &b) { return g_pLocalPlayer->v_Origin.DistTo(a) < g_pLocalPlayer->v_Origin.DistTo(b); });
        for (auto &pack : healthpacks)
        {
            if (nav::navTo(pack, 10, true, false))
            {
                current_task = task::health;
                return true;
            }
        }
    }

    if (current_task == task::ammo && !hasLowAmmo())
    {
        nav::clearInstructions();
        current_task = task::none;
        return false;
    }
    if (current_task == task::ammo)
        return true;
    if (hasLowAmmo())
    {
        std::vector<Vector> ammopacks;
        for (int i = 1; i < HIGHEST_ENTITY; i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent))
                continue;
            if (ent->m_ItemType() != ITEM_AMMO_SMALL && ent->m_ItemType() != ITEM_AMMO_MEDIUM && ent->m_ItemType() != ITEM_AMMO_LARGE)
                continue;
            ammopacks.push_back(ent->m_vecOrigin());
        }
        std::sort(ammopacks.begin(), ammopacks.end(), [](Vector &a, Vector &b) { return g_pLocalPlayer->v_Origin.DistTo(a) < g_pLocalPlayer->v_Origin.DistTo(b); });
        for (auto &pack : ammopacks)
        {
            if (nav::navTo(pack, 9, true, false))
            {
                current_task = task::ammo;
                return true;
            }
        }
    }
    return false;
}

static void autoJump()
{
    static Timer last_jump{};
    if (!last_jump.test_and_set(200))
        return;

    if (getNearestPlayerDistance().second <= *jump_distance)
        current_user_cmd->buttons |= IN_JUMP;
}

enum slots
{
    primary   = 1,
    secondary = 2,
    melee     = 3
};
static int GetBestSlot()
{

    switch (g_pLocalPlayer->clazz)
    {
    case tf_scout:
        return primary;
    case tf_heavy:
        return primary;
    case tf_medic:
        return secondary;
    default:
    {
        float nearest_dist = getNearestPlayerDistance().second;
        if (nearest_dist > 400)
            return primary;
        else
            return secondary;
    }
    }
    return primary;
}

static void updateSlot()
{
    static Timer slot_timer{};
    if (!slot_timer.test_and_set(300))
        return;
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W) && !g_pLocalPlayer->life_state)
    {
        IClientEntity *weapon = RAW_ENT(LOCAL_W);
        // IsBaseCombatWeapon()
        if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon))
        {
            int slot    = re::C_BaseCombatWeapon::GetSlot(weapon);
            int newslot = GetBestSlot();
            if (slot != newslot - 1)
                g_IEngine->ClientCmd_Unrestricted(format("slot", newslot).c_str());
        }
    }
}

static InitRoutine runinit([]() { EC::Register(EC::CreateMove, CreateMove, "navbot", EC::early); });

void change(settings::VariableBase<bool> &, bool)
{
    nav::clearInstructions();
}

static InitRoutine routine([]() { enabled.installChangeCallback(change); });

} // namespace hacks::tf2::NavBot
