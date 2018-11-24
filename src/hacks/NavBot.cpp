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
constexpr bot_class_config DIST_OTHER{ 100.0f, 200.0f, 300.0f };
constexpr bot_class_config DIST_SNIPER{ 1000.0f, 1500.0f, 3000.0f };

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
static bool isValidNearPosition(Vector vec, Vector target,
                                const bot_class_config &config)
{
    vec.z += 20;
    target.z += 20;
    float dist = vec.DistTo(target);
    if (dist < config.min || dist > config.max)
        return false;
    if (!IsVectorVisible(vec, target, true))
        return false;
    return true;
}

// Returns true if began pathing
static bool stayNearPlayer(CachedEntity *ent, const bot_class_config &config,
                           CNavArea **result)
{
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
    std::sort(areas.begin(), areas.end(), [&](CNavArea *a, CNavArea *b) {
        return std::abs(a->m_center.DistTo(ent_orig) - config.preferred) <
               std::abs(b->m_center.DistTo(ent_orig) - config.preferred);
    });

    size_t size = 20;
    if (areas.size() < size)
        size = areas.size();
    std::vector<CNavArea *> preferred_areas(areas.begin(),
                                            areas.begin() + size / 2);
    std::sort(preferred_areas.begin(), preferred_areas.end(),
              [](CNavArea *a, CNavArea *b) {
                  return a->m_center.DistTo(g_pLocalPlayer->v_Origin) <
                         b->m_center.DistTo(g_pLocalPlayer->v_Origin);
              });

    // Try to path up to 10 times to different areas
    for (size_t attempts = 0; attempts < size / 2; attempts++)
    {
        std::vector<CNavArea *>::iterator it;
        if (attempts <= size / 4)
        {
            it = preferred_areas.begin() + attempts;
        }
        else
        {
            it = select_randomly(areas.begin(), areas.end());
        }

        if (nav::navTo((*it.base())->m_center, 7, true, false))
        {
            *result      = *it.base();
            current_task = task::stay_near;
            return true;
        }
    }
    return false;
}

// Loop thru all players and find one we can path to
static bool stayNearPlayers(const bot_class_config &config,
                            CachedEntity **result_ent, CNavArea **result_area)
{
    logging::Info("Stay near players called!");
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer() || !ent->m_bEnemy())
            continue;
        if (stayNearPlayer(ent, config, result_area))
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
    const bot_class_config *config;
    if (heavy_mode)
    {
        config = &DIST_OTHER;
    }
    else
    {
        config = &DIST_SNIPER;
    }

    if (current_task == task::stay_near)
    {
        // Do we already have a stay near target? Check if its still good.
        if (CE_BAD(lastTarget))
        {
            current_task = task::none;
        }
        else if (!lastTarget->m_bAlivePlayer() || !lastTarget->m_bEnemy())
        {
            nav::clearInstructions();
            current_task = task::none;
        }
        // Check if we still have LOS and are close enough/far enough
        else if (!stayNearHelpers::isValidNearPosition(
                     result->m_center, lastTarget->m_vecOrigin(), *config))
        {
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
                g_pLocalPlayer->v_Origin, lastTarget->m_vecOrigin(), *config))
            return true;
    }

    if (current_task == task::stay_near)
    {
        return true;
    }
    else
    {
        // We're doing nothing? Do something!
        return stayNearHelpers::stayNearPlayers(*config, &lastTarget, &result);
    }
}

static HookedFunction cm(HookedFunctions_types::HF_CreateMove, "NavBot", 16,
                         &CreateMove);

} // namespace hacks::tf2::NavBot
