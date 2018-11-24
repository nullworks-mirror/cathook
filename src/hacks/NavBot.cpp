#include "common.hpp"
#include "navparser.hpp"
#include "NavBot.hpp"

namespace hacks::tf2::NavBot
{
// -Rvars-
static settings::Bool enabled("navbot.enabled", "false");
static settings::Bool stay_near("navbot.stay-near", "true");
static settings::Bool heavy_mode("navbot.other-mode", "false");

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
constexpr std::pair<float, float> MIN_MAX_DIST_OTHER(200, 500);
constexpr std::pair<float, float> MIN_MAX_DIST_SNIPER(1000, 4000);

static void CreateMove()
{
    if (!init(false))
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (!nav::ReadyForCommands)
        wait_until_path.update();
    else
        current_task = task::none;

    // Try to near enemies to increase efficiency
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
        // Add all sniper spots to vector
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

namespace stayNearHelpers
{
// Check if the location is close enough/far enough and has a visual to target
static bool isValidNearPosition(Vector vec, Vector target, float min_distance,
                                float max_distance)
{
    vec.z += 20;
    target.z += 20;
    float dist = vec.DistTo(target);
    if (dist < min_distance || dist > max_distance)
        return false;
    if (!IsVectorVisible(vec, target, true))
        return false;
    return true;
}

// Returns true if began pathing
static bool stayNearPlayer(CachedEntity *ent, float min_distance,
                           float max_distance, CNavArea **result)
{
    // Find good near positions
    std::vector<CNavArea *> checked_areas;
    for (auto &area : nav::navfile->m_areas)
    {
        if (!nav::isSafe(&area))
            continue;
        if (isValidNearPosition(area.m_center, ent->m_vecOrigin(), min_distance,
                                max_distance))
            checked_areas.push_back(&area);
    }
    if (checked_areas.empty())
        return false;
    unsigned int size = 5;
    if (checked_areas.size() < size)
        size = checked_areas.size() / 2;
    std::partial_sort(checked_areas.begin(), checked_areas.begin() + size,
                      checked_areas.end(), [](CNavArea *a, CNavArea *b) {
                          return g_pLocalPlayer->v_Origin.DistTo(a->m_center) <
                                 g_pLocalPlayer->v_Origin.DistTo(b->m_center);
                      });

    // Try to path up to 10 times to different areas
    for (unsigned int attempts = 0; attempts < (size * 2); attempts++)
    {
        std::vector<CNavArea *>::iterator i;
        if (attempts >= size)
        {
            i = select_randomly(checked_areas.begin(), checked_areas.end());
        }
        else
        {
            i = checked_areas.begin() + attempts;
        }

        if (nav::navTo((*i.base())->m_center, 7, true, false))
        {
            *result      = *i.base();
            current_task = task::stay_near;
            return true;
        }
    }
    return false;
}

// Loop thru all players and find one we can path to
static bool stayNearPlayers(float min_distance, float max_distance,
                            CachedEntity **result_ent, CNavArea **result_area)
{
    logging::Info("Stay near players called!");
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy())
            continue;
        if (stayNearPlayer(ent, min_distance, max_distance, result_area))
        {
            *result_ent = ent;
            return true;
        }
    }
    return false;
}
} // namespace stayNearHelpers

// Main stay near function
static bool stayNear()
{
    static CachedEntity *lastTarget = nullptr;
    static CNavArea *result         = nullptr;

    // What distances do we have to use?
    const std::pair<float, float> *minMaxDist{};
    if (heavy_mode)
    {
        minMaxDist = &MIN_MAX_DIST_OTHER;
    }
    else
    {
        minMaxDist = &MIN_MAX_DIST_SNIPER;
    }

    if (current_task == task::stay_near)
    {
        // Do we already have a stay near target? Check if its still good.
        if (CE_BAD(lastTarget))
        {
            current_task = task::none;
            logging::Info("Player gone bad!");
            // Don't cancel if target gone CE_BAD. We might still find him.
        }
        else if (!lastTarget->m_bAlivePlayer() || !lastTarget->m_bEnemy())
        {
            nav::clearInstructions();
            current_task = task::none;
            logging::Info("Player gone bad!");
        }
        // Check if we still have LOS and are close enough/far enough
        else if (!stayNearHelpers::isValidNearPosition(
                     result->m_center, lastTarget->m_vecOrigin(),
                     minMaxDist->first, minMaxDist->second))
        {
            logging::Info("Location gone bad!");
            current_task = task::none;
            nav::clearInstructions();
        }
    }
    // Are we doing nothing? Check if our current location can still attack our
    // last target
    else if (current_task == task::none && CE_GOOD(lastTarget) &&
             lastTarget->m_bAlivePlayer() && lastTarget->m_bEnemy())
    {
        if (stayNearHelpers::isValidNearPosition(
                g_pLocalPlayer->v_Origin, lastTarget->m_vecOrigin(),
                minMaxDist->first, minMaxDist->second))
            return true;
    }

    if (current_task == task::stay_near)
    {
        return true;
    }
    else
    {
        // We're doing nothing? Do something!
        return stayNearHelpers::stayNearPlayers(
            minMaxDist->first, minMaxDist->second, &lastTarget, &result);
    }
}

static HookedFunction cm(HookedFunctions_types::HF_CreateMove, "NavBot", 16,
                         &CreateMove);

} // namespace hacks::tf2::NavBot
