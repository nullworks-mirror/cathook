#include "common.hpp"
#include "navparser.hpp"
#include <thread>
#include "micropather.h"
#include <pwd.h>
#include <boost/functional/hash.hpp>
#include <boost/container/flat_set.hpp>
#include <chrono>

namespace nav
{

static settings::Bool enabled{ "misc.pathing", "true" };
// Whether or not to run vischecks at pathtime
static settings::Bool vischecks{ "misc.pathing.pathtime-vischecks", "true" };
static settings::Bool draw{ "misc.pathing.draw", "false" };
static settings::Bool look{ "misc.pathing.look-at-path", "false" };
static settings::Int stuck_time{ "misc.pathing.stuck-time", "4000" };

static std::vector<Vector> crumbs;

enum ignore_status : uint8_t
{
    // Status is unknown
    unknown = 0,
    // Something like Z check failed, these are unchanging
    const_ignored,
    // LOS between areas is given
    vischeck_success,
    // No LOS between areas
    vischeck_failed,
    // Failed to actually walk thru connection
    explicit_ignored,
    // Danger like sentry gun or sticky
    danger_found
};

void ResetPather();
void repath();
void DoSlowAim(Vector &input_angle);

struct ignoredata
{
    ignore_status status{ unknown };
    float stucktime{ 0.0f };
    Timer ignoreTimeout{};
};

CNavArea *getNavArea(Vector &vec)
{
    for (auto &i : navfile->m_areas)
    {
        if (vec == i.m_center)
            return &i;
    }
    return nullptr;
}

Vector GetClosestCornerToArea(CNavArea *CornerOf, CNavArea *Target)
{
    std::array<Vector, 4> corners;
    corners.at(0) = CornerOf->m_nwCorner;                                                             // NW
    corners.at(1) = CornerOf->m_seCorner;                                                             // SE
    corners.at(2) = Vector{ CornerOf->m_seCorner.x, CornerOf->m_nwCorner.y, CornerOf->m_nwCorner.z }; // NE
    corners.at(3) = Vector{ CornerOf->m_nwCorner.x, CornerOf->m_seCorner.y, CornerOf->m_seCorner.z }; // SW

    Vector bestVec{};
    float bestDist = FLT_MAX;

    for (size_t i = 0; i < corners.size(); i++)
    {
        float dist = corners.at(i).DistTo(Target->m_center);
        if (dist < bestDist)
        {
            bestVec  = corners.at(i);
            bestDist = dist;
        }
    }

    Vector bestVec2{};
    float bestDist2 = FLT_MAX;

    for (size_t i = 0; i < corners.size(); i++)
    {
        if (corners.at(i) == bestVec2)
            continue;
        float dist = corners.at(i).DistTo(Target->m_center);
        if (dist < bestDist2)
        {
            bestVec2  = corners.at(i);
            bestDist2 = dist;
        }
    }
    return (bestVec + bestVec2) / 2;
}

float getZBetweenAreas(CNavArea *start, CNavArea *end)
{
    float z1 = GetClosestCornerToArea(start, end).z;
    float z2 = GetClosestCornerToArea(end, start).z;

    return z2 - z1;
}

class ignoremanager
{
    static std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata, boost::hash<std::pair<CNavArea *, CNavArea *>>> ignores;
    static bool vischeck(CNavArea *begin, CNavArea *end)
    {
        Vector first  = begin->m_center;
        Vector second = end->m_center;
        first.z += 42;
        second.z += 42;
        return IsVectorVisible(first, second, true);
    }
    static ignore_status runIgnoreChecks(CNavArea *begin, CNavArea *end)
    {
        if (getZBetweenAreas(begin, end) > 42)
            return const_ignored;
        if (!vischecks)
            return vischeck_success;
        if (vischeck(begin, end))
            return vischeck_success;
        else
            return vischeck_failed;
    }
    static void updateDanger()
    {
        for (size_t i = 0; i < HIGHEST_ENTITY; i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent))
                continue;
            if (ent->m_iClassID() == CL_CLASS(CObjectSentrygun))
            {
                if (!ent->m_bEnemy())
                    continue;
                Vector loc = GetBuildingPosition(ent);
                for (auto &i : navfile->m_areas)
                {
                    Vector area = i.m_center;
                    area.z += 41.5f;
                    if (loc.DistTo(area) > 1100)
                        continue;
                    // Check if sentry can see us
                    if (!IsVectorVisible(loc, area, true))
                        continue;
                    ignoredata &data = ignores[{ &i, nullptr }];
                    data.status      = danger_found;
                    data.ignoreTimeout.update();
                }
            }
            else if (ent->m_iClassID() == CL_CLASS(CTFGrenadePipebombProjectile))
            {
                if (!ent->m_bEnemy())
                    continue;
                if (CE_INT(ent, netvar.iPipeType) == 1)
                    continue;
                Vector loc = ent->m_vecOrigin();
                for (auto &i : navfile->m_areas)
                {
                    Vector area = i.m_center;
                    if (loc.DistTo(area) > 130)
                        continue;
                    area.z += 41.5f;
                    // Check if sentry can see us
                    if (!IsVectorVisible(loc, area, true))
                        continue;
                    ignoredata &data = ignores[{ &i, nullptr }];
                    data.status      = danger_found;
                    data.ignoreTimeout.update();
                }
            }
        }
    }

    static void checkPath()
    {
        bool perform_repath = false;
        // Vischecks
        for (size_t i = 0; i < crumbs.size() - 1; i++)
        {
            CNavArea *begin = getNavArea(crumbs.at(i));
            CNavArea *end   = getNavArea(crumbs.at(i + 1));
            if (!begin || !end)
                continue;
            ignoredata &data = ignores[{ begin, end }];
            if (data.status == vischeck_failed)
                return;
            if (!vischeck(begin, end))
            {
                data.status = vischeck_failed;
                data.ignoreTimeout.update();
                perform_repath = true;
            }
            else if (ignores[{ end, nullptr }].status == danger_found)
            {
                perform_repath = true;
            }
        }
        if (perform_repath)
            repath();
    }

public:
    // 0 = Not ignored, 1 = low priority, 2 = ignored
    static int isIgnored(CNavArea *begin, CNavArea *end)
    {
        if (ignores[{ end, nullptr }].status == danger_found)
            return 2;
        ignore_status &status = ignores[{ begin, end }].status;
        if (status == unknown)
            status = runIgnoreChecks(begin, end);
        if (status == vischeck_success)
            return 0;
        else if (status == vischeck_failed)
            return 1;
        else
            return 2;
    }
    static bool addTime(CNavArea *begin, CNavArea *end, Timer &time)
    {
        using namespace std::chrono;
        // Check if connection is already known
        if (ignores.find({ begin, end }) == ignores.end())
        {
            ignores[{ begin, end }] = {};
        }
        ignoredata &connection = ignores[{ begin, end }];
        connection.stucktime += duration_cast<milliseconds>(system_clock::now() - time.last).count();
        if (connection.stucktime >= *stuck_time)
        {
            connection.status = explicit_ignored;
            connection.ignoreTimeout.update();
            logging::Info("Ignored Connection %i-%i", begin->m_id, end->m_id);
            return true;
        }
        return false;
    }
    static bool addTime(Vector &begin, Vector &end, Timer &time)
    {
        // todo: check nullptr
        CNavArea *begin_area = getNavArea(begin);
        CNavArea *end_area   = getNavArea(end);
        if (!begin_area || !end_area)
        {
            // We can't reach the destination vector. Destination vector might
            // be out of bounds/reach.
            crumbs.clear();
            return true;
        }
        return addTime(begin_area, end_area, time);
    }
    static void reset()
    {
        ignores.clear();
        ResetPather();
    }
    static void updateIgnores()
    {
        static Timer update{};
        static Timer last_pather_reset{};
        static bool reset_pather = false;
        if (!update.test_and_set(500))
            return;
        updateDanger();
        if (crumbs.empty())
        {
            for (auto &i : ignores)
            {
                switch (i.second.status)
                {
                case explicit_ignored:
                    if (i.second.ignoreTimeout.check(60000))
                    {
                        i.second.status    = unknown;
                        i.second.stucktime = 0;
                        reset_pather       = true;
                    }
                    break;
                case unknown:
                    break;
                case danger_found:
                    if (i.second.ignoreTimeout.check(20000))
                    {
                        i.second.status = unknown;
                        reset_pather    = true;
                    }
                    break;
                case vischeck_failed:
                case vischeck_success:
                default:
                    if (i.second.ignoreTimeout.check(30000))
                    {
                        i.second.status    = unknown;
                        i.second.stucktime = 0;
                        reset_pather       = true;
                    }
                    break;
                }
            }
        }
        else
            checkPath();
        if (reset_pather && last_pather_reset.test_and_set(10000))
        {
            reset_pather = false;
            ResetPather();
        }
    }
    static bool isSafe(CNavArea *area)
    {
        return !(ignores[{ area, nullptr }].status == danger_found);
    }
    ignoremanager() = delete;
};
std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata, boost::hash<std::pair<CNavArea *, CNavArea *>>> ignoremanager::ignores;

struct Graph : public micropather::Graph
{
    std::unique_ptr<micropather::MicroPather> pather;

    Graph()
    {
        pather = std::make_unique<micropather::MicroPather>(this, 3000, 6, true);
    }
    ~Graph() override
    {
    }
    void AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent) override
    {
        CNavArea *center = static_cast<CNavArea *>(state);
        for (auto &i : center->m_connections)
        {
            CNavArea *neighbour = i.area;
            int isIgnored       = ignoremanager::isIgnored(center, neighbour);
            if (isIgnored == 2)
                continue;
            float distance = center->m_center.DistTo(i.area->m_center);
            if (isIgnored == 1)
                distance += 50000;
            micropather::StateCost cost{ static_cast<void *>(neighbour), distance };
            adjacent->push_back(cost);
        }
    }
    float LeastCostEstimate(void *stateStart, void *stateEnd) override
    {
        CNavArea *start = static_cast<CNavArea *>(stateStart);
        CNavArea *end   = static_cast<CNavArea *>(stateEnd);
        return start->m_center.DistTo(end->m_center);
    }
    void PrintStateInfo(void *) override
    {
        // Uhh no
    }
};

// Navfile containing areas
std::unique_ptr<CNavFile> navfile;
// Status
std::atomic<init_status> status;

// See "Graph", does pathing and stuff I guess
static Graph Map;

void initThread()
{
    // Get NavFile location
    std::string lvlname(g_IEngine->GetLevelName());
    size_t dotpos = lvlname.find('.');
    lvlname       = lvlname.substr(0, dotpos);
    std::string lvldir("/home/");
    passwd *pwd = getpwuid(getuid());
    lvldir.append(pwd->pw_name);
    lvldir.append("/.steam/steam/steamapps/common/Team Fortress 2/tf/");
    lvldir.append(lvlname);
    lvldir.append(".nav");
    logging::Info(format("Pathing: Nav File location: ", lvldir).c_str());

    navfile = std::make_unique<CNavFile>(lvldir.c_str());

    if (!navfile->m_isOK)
    {
        navfile.reset();
        status = unavailable;
        return;
    }
    logging::Info("Pather: Initing with %i Areas", navfile->m_areas.size());
    status = on;
}

void init()
{
    ignoremanager::reset();
    status = initing;
    std::thread thread;
    thread = std::thread(initThread);
    thread.detach();
}

bool prepare()
{
    if (!enabled)
        return false;
    init_status fast_status = status;
    if (fast_status == on)
        return true;
    if (fast_status == off)
    {
        init();
    }
    return false;
}

// This prevents the bot from gettings completely stuck in some cases
static std::vector<CNavArea *> findClosestNavSquare_localAreas(6);
// Function for getting closest Area to player, aka "LocalNav"
CNavArea *findClosestNavSquare(Vector vec)
{
    if (findClosestNavSquare_localAreas.size() > 5)
        findClosestNavSquare_localAreas.erase(findClosestNavSquare_localAreas.begin());

    bool is_local = vec == g_pLocalPlayer->v_Origin;

    auto &areas = navfile->m_areas;
    std::vector<CNavArea *> overlapping;

    for (auto &i : areas)
    {
        // Check if we are within x and y bounds of an area
        if (i.IsOverlapping(vec))
        {
            // Make sure we're not stuck on the same area for too long
            if (std::count(findClosestNavSquare_localAreas.begin(), findClosestNavSquare_localAreas.end(), &i) < 3)
            {
                if (IsVectorVisible(vec, i.m_center, true))
                    overlapping.push_back(&i);
            }
        }
    }

    // If multiple candidates for LocalNav have been found, pick the closest
    float bestDist       = FLT_MAX;
    CNavArea *bestSquare = nullptr;
    for (auto &i : overlapping)
    {
        float dist = i->m_center.DistTo(vec);
        if (dist < bestDist)
        {
            bestDist   = dist;
            bestSquare = i;
        }
    }

    if (bestSquare != nullptr)
    {
        if (is_local)
            findClosestNavSquare_localAreas.push_back(bestSquare);
        return bestSquare;
    }
    // If no LocalNav was found, pick the closest available Area
    bestDist = FLT_MAX;
    for (auto &i : areas)
    {
        float dist = i.m_center.DistTo(vec);
        if (dist < bestDist)
        {
            if (std::count(findClosestNavSquare_localAreas.begin(), findClosestNavSquare_localAreas.end(), &i) < 3)
            {
                bestDist   = dist;
                bestSquare = &i;
            }
        }
    }
    if (is_local)
        findClosestNavSquare_localAreas.push_back(bestSquare);
    return bestSquare;
}

std::vector<Vector> findPath(Vector start, Vector end)
{
    if (status != on)
        return {};
    CNavArea *local = findClosestNavSquare(start);
    CNavArea *dest  = findClosestNavSquare(end);

    if (!local || !dest)
        return {};
    micropather::MPVector<void *> pathNodes;
    float cost;
    std::chrono::time_point begin_pathing = std::chrono::high_resolution_clock::now();
    int result                            = Map.pather->Solve(static_cast<void *>(local), static_cast<void *>(dest), &pathNodes, &cost);
    long long timetaken                   = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - begin_pathing).count();
    logging::Info("Pathing: Pather result: %i. Time taken (NS): %lld", result, timetaken);
    // If no result found, return empty Vector
    if (result == micropather::MicroPather::NO_SOLUTION)
        return std::vector<Vector>(0);
    // Convert (void *) CNavArea * to Vector
    std::vector<Vector> path;
    for (size_t i = 0; i < pathNodes.size(); i++)
    {
        path.push_back(static_cast<CNavArea *>(pathNodes[i])->m_center);
    }
    // Add our destination to the std::vector
    path.push_back(end);
    return path;
}

static Vector loc(0.0f, 0.0f, 0.0f);
static Vector last_area(0.0f, 0.0f, 0.0f);
bool ReadyForCommands = true;
static Timer inactivity{};
int curr_priority         = 0;
static bool ensureArrival = false;

bool navTo(Vector destination, int priority, bool should_repath, bool nav_to_local, bool is_repath)
{
    if (!prepare())
        return false;
    if (priority < curr_priority)
        return false;
    std::vector<Vector> path = findPath(g_pLocalPlayer->v_Origin, destination);
    if (path.empty())
    {
        crumbs.clear();
        return false;
    }
    auto crumb = crumbs.begin();
    if (crumb != crumbs.end())
    {
        if (ignoremanager::addTime(last_area, *crumb, inactivity))
            ResetPather();
    }
    auto path_it = path.begin();
    last_area    = *path_it;
    if (!nav_to_local)
    {
        path.erase(path_it);
        if (path.empty())
            return false;
    }
    inactivity.update();
    if (!is_repath)
    {
        findClosestNavSquare_localAreas.clear();
    }
    ensureArrival    = should_repath;
    ReadyForCommands = false;
    curr_priority    = priority;
    crumbs           = std::move(path);
    return true;
}

void repath()
{
    if (ensureArrival)
    {
        Vector last = crumbs.back();
        crumbs.clear();
        ResetPather();
        navTo(last, curr_priority, true, true, true);
    }
}

static Timer last_jump{};
// Main movement function, gets path from NavTo
static void cm()
{
    if (!enabled || status != on)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;
    if (!LOCAL_E->m_bAlivePlayer())
    {
        // Clear path if player dead
        crumbs.clear();
        return;
    }
    ignoremanager::updateIgnores();
    auto crumb = crumbs.begin();
    // Crumbs empty, prepare for next instruction
    if (crumb == crumbs.end())
    {
        curr_priority    = 0;
        ReadyForCommands = true;
        ensureArrival    = false;
        return;
    }
    ReadyForCommands = false;
    // Remove old crumbs
    if (g_pLocalPlayer->v_Origin.DistTo(Vector{ crumb->x, crumb->y, crumb->z }) < 50.0f)
    {
        last_area = *crumb;
        crumbs.erase(crumb);
        inactivity.update();
        crumb = crumbs.begin();
        if (crumb == crumbs.end())
            return;
    }
    if (look)
    {
        Vector next  = *crumb;
        next.z       = g_pLocalPlayer->v_Eye.z;
        Vector angle = GetAimAtAngles(g_pLocalPlayer->v_Eye, next);
        DoSlowAim(angle);
        current_user_cmd->viewangles = angle;
    }
    // Detect when jumping is necessary
    if ((!(g_pLocalPlayer->holding_sniper_rifle && g_pLocalPlayer->bZoomed) && crumb->z - g_pLocalPlayer->v_Origin.z > 18 && last_jump.test_and_set(200)) || (last_jump.test_and_set(200) && inactivity.check(3000)))
        current_user_cmd->buttons |= IN_JUMP;
    // If inactive for too long
    if (inactivity.check(*stuck_time))
    {
        // Ignore connection
        ignoremanager::addTime(last_area, *crumb, inactivity);
        repath();
        return;
    }
    // Walk to next crumb
    WalkTo(*crumb);
}

#if ENABLE_VISUALS
static void drawcrumbs()
{
    if (!enabled || !draw)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;
    if (!LOCAL_E->m_bAlivePlayer())
        return;
    if (crumbs.size() < 2)
        return;
    for (size_t i = 0; i < crumbs.size() - 1; i++)
    {
        Vector wts1, wts2;
        if (draw::WorldToScreen(crumbs[i], wts1) && draw::WorldToScreen(crumbs[i + 1], wts2))
        {
            draw::Line(wts1.x, wts1.y, wts2.x - wts1.x, wts2.y - wts1.y, colors::white, 0.3f);
        }
    }
    Vector wts;
    if (!draw::WorldToScreen(crumbs[0], wts))
        return;
    draw::Rectangle(wts.x - 4, wts.y - 4, 8, 8, colors::white);
    draw::RectangleOutlined(wts.x - 4, wts.y - 4, 7, 7, colors::white, 1.0f);
}
#endif

static InitRoutine runinit([]() {
    EC::Register(EC::CreateMove, cm, "cm_navparser", EC::average);
#if ENABLE_VISUALS
    EC::Register(EC::Draw, drawcrumbs, "draw_navparser", EC::average);
#endif
});

void ResetPather()
{
    Map.pather->Reset();
}

bool isSafe(CNavArea *area)
{
    return ignoremanager::isSafe(area);
}

static CatCommand nav_find("nav_find", "Debug nav find", []() {
    std::vector<Vector> path = findPath(g_pLocalPlayer->v_Origin, loc);
    if (path.empty())
    {
        logging::Info("Pathing: No path found");
        return;
    }
    std::string output = "Pathing: Path found! Path: ";
    for (int i = 0; i < path.size(); i++)
    {
        output.append(format(path.at(i).x, ",", format(path.at(i).y), " "));
    }
    logging::Info(output.c_str());
});

static CatCommand nav_set("nav_set", "Debug nav find", []() { loc = g_pLocalPlayer->v_Origin; });

static CatCommand nav_init("nav_init", "Debug nav init", []() {
    status = off;
    prepare();
});

static CatCommand nav_path("nav_path", "Debug nav path", []() { navTo(loc); });

static CatCommand nav_path_no_local("nav_path_no_local", "Debug nav path", []() { navTo(loc, 5, false, false); });

static CatCommand nav_reset_ignores("nav_reset_ignores", "Reset all ignores.", []() { ignoremanager::reset(); });

void DoSlowAim(Vector &input_angle)
{
    static float slow_change_dist_y{};
    static float slow_change_dist_p{};

    auto viewangles = current_user_cmd->viewangles;

    // Yaw
    if (viewangles.y != input_angle.y)
    {

        // Check if input angle and user angle are on opposing sides of yaw so
        // we can correct for that
        bool slow_opposing = false;
        if ((input_angle.y < -90 && viewangles.y > 90) || (input_angle.y > 90 && viewangles.y < -90))
            slow_opposing = true;

        // Direction
        bool slow_dir = false;
        if (slow_opposing)
        {
            if (input_angle.y > 90 && viewangles.y < -90)
                slow_dir = true;
        }
        else if (viewangles.y > input_angle.y)
            slow_dir = true;

        // Speed, check if opposing. We dont get a new distance due to the
        // opposing sides making the distance spike, so just cheap out and reuse
        // our last one.
        if (!slow_opposing)
            slow_change_dist_y = std::abs(viewangles.y - input_angle.y) / 5;

        // Move in the direction of the input angle
        if (slow_dir)
            input_angle.y = viewangles.y - slow_change_dist_y;
        else
            input_angle.y = viewangles.y + slow_change_dist_y;
    }

    // Pitch
    if (viewangles.x != input_angle.x)
    {
        // Get speed
        slow_change_dist_p = std::abs(viewangles.x - input_angle.x) / 5;

        // Move in the direction of the input angle
        if (viewangles.x > input_angle.x)
            input_angle.x = viewangles.x - slow_change_dist_p;
        else
            input_angle.x = viewangles.x + slow_change_dist_p;
    }

    // Clamp as we changed angles
    fClampAngle(input_angle);
}

void clearInstructions()
{
    crumbs.clear();
    curr_priority = 0;
}

} // namespace nav
