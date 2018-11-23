#include "common.hpp"
#include "navparser.hpp"
#include "NavBot.hpp"

namespace hacks::tf2::NavBot
{
// -Rvars-
static settings::Bool enabled("navbot.enabled", "false");
static settings::Bool stay_near("navbot.stay_near", "true");
static settings::Bool heavy_mode("navbot.heavy_mode", "false");

// -Forward declarations-
bool init(bool first_cm);
static bool navToSniperSpot();
static bool stayNear();

// -Variables-
static std::vector<std::pair<CNavArea *, Vector>> sniper_spots;
// How long should the bot wait until pathing again?
static Timer wait_until_path{};
// What is the bot currently doing
static task::task current_task;
constexpr std::pair<float, float> MAX_MIN_DIST_OTHER(1000, 200);
constexpr std::pair<float, float> MAX_MIN_DIST_SNIPER(1000, 200);

static void CreateMove()
{
    if (!init(false))
        return;
    if (!nav::ReadyForCommands)
        wait_until_path.update();
    else
        current_task = task::none;

    if (stay_near)
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
        for (auto &area : nav::navfile->m_areas)
        {
            for (auto hide : area.m_hidingSpots)
                if (hide.IsGoodSniperSpot() || hide.IsIdealSniperSpot() ||
                    hide.IsExposed())
                    sniper_spots.emplace_back(&area, hide.m_pos);
        }
    }
    return true;
}

static bool navToSniperSpot()
{
    // Don't path if you already have commands. But also don't error out.
    if (!nav::ReadyForCommands)
        return true;
    // Wait arround a bit before pathing again
    if (wait_until_path.check(2000))
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

namespace stayNearHelpers
{
static bool isValidNearPosition(Vector vec, Vector target,
                                          float min_distance,
                                          float max_distance)
{
    vec.z += 20;
    target += 20;
    float dist = vec.DistTo(target);
    if (dist < min_distance || dist > max_distance)
        return false;
    if (!IsVectorVisible(vec, target, true))
        return false;
    return true;
}

static bool stayNearPlayer(CachedEntity *ent, float min_distance,
                                     float max_distance, Vector &result)
{
    std::vector<CNavArea *> checked_areas;
    for (auto &area : nav::navfile->m_areas)
    {
        if (isValidNearPosition(area.m_center, ent->m_vecOrigin(), min_distance,
                                max_distance))
            checked_areas.push_back(&area);
    }
    if (checked_areas.empty())
        return false;
    for (int attempts = 0; attempts < 10; attempts++)
    {
        auto random =
            select_randomly(checked_areas.begin(), checked_areas.end());
        if (nav::navTo((*random.base())->m_center, 7, true, false))
        {
            result = (*random.base())->m_center;
            current_task = task::stay_near;
            return true;
        }
    }
    return false;
}

static bool stayNearPlayers(float min_distance, float max_distance, CachedEntity *ent, Vector &result)
{
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy())
            continue;
        if (stayNearPlayer(ent, min_distance, max_distance, result))
            return true;
    }
    return false;
}
} // namespace stayNear

static bool stayNear()
{
    static CachedEntity *lastTarget = nullptr;
    static Vector result{};

    const std::pair<float, float> *minMaxDist{};
    if (heavy_mode)
    {
        minMaxDist = &MAX_MIN_DIST_OTHER;
    }
    else
    {
        minMaxDist = &MAX_MIN_DIST_SNIPER;
    }

    if (current_task == task::stay_near)
    {
        if (CE_BAD(lastTarget) || !stayNearHelpers::isValidNearPosition(result, lastTarget->m_vecOrigin(), minMaxDist->first, minMaxDist->second))
        {
            current_task = task::none;
        }
    }
    if (current_task == task::stay_near)
    {
        return true;
    }
    else
    {
        return stayNearHelpers::stayNearPlayers(minMaxDist->first, minMaxDist->second, lastTarget, result);
    }
}

static HookedFunction cm(HookedFunctions_types::HF_CreateMove, "NavBot", 16,
                         &CreateMove);

} // namespace hacks::tf2::NavBot
