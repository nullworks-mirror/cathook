#include "common.hpp"
#include "navparser.hpp"
#include <thread>
#include "micropather.h"
#include <pwd.h>
#include <boost/functional/hash.hpp>

namespace nav
{

static settings::Bool enabled{ "misc.pathing", "true" };

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
    explicit_ignored
};

void ResetPather();
void repath();

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
    corners.at(0) = CornerOf->m_nwCorner; // NW
    corners.at(1) = CornerOf->m_seCorner; // SE
    corners.at(2) = Vector{ CornerOf->m_seCorner.x, CornerOf->m_nwCorner.y,
                            CornerOf->m_nwCorner.z }; // NE
    corners.at(3) = Vector{ CornerOf->m_nwCorner.x, CornerOf->m_seCorner.y,
                            CornerOf->m_seCorner.z }; // SW

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
    static std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata,
                              boost::hash<std::pair<CNavArea *, CNavArea *>>>
        ignores;
    static bool vischeck(CNavArea *begin, CNavArea *end)
    {
        Vector first  = begin->m_center;
        Vector second = end->m_center;
        first.z += 42;
        second.z += 42;
        return IsVectorVisible(first, second, false);
    }
    static ignore_status runIgnoreChecks(CNavArea *begin, CNavArea *end)
    {
        if (getZBetweenAreas(begin, end) > 42)
            return const_ignored;
        if (vischeck(begin, end))
            return vischeck_success;
        else
            return vischeck_failed;
    }
    static void checkPath()
    {
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
                repath();
                return;
            }
        }
    }

public:
    // 0 = Not ignored, 1 = low priority, 2 = ignored
    static int isIgnored(CNavArea *begin, CNavArea *end)
    {
        //
        ignore_status &status = ignores[{ begin, end }].status;
        if (status == unknown)
            status = runIgnoreChecks(begin, end);
        if (status == const_ignored || status == explicit_ignored)
            return 2;
        else if (status == vischeck_failed)
            return 1;
        else
            return 0;
    }
    static void addTime(CNavArea *begin, CNavArea *end, Timer &time)
    {
        // Check if connection is already known
        if (ignores.find({ begin, end }) == ignores.end())
        {
            ignores[{ begin, end }] = {};
        }
        ignoredata &connection = ignores[{ begin, end }];
        connection.stucktime +=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - time.last)
                .count();
        if (connection.stucktime > 3999)
        {
            connection.status = explicit_ignored;
            connection.ignoreTimeout.update();
            logging::Info("Ignored Connection %i-%i", begin->m_id, end->m_id);
        }
    }
    static void addTime(Vector &begin, Vector &end, Timer &time)
    {
        // todo: check nullptr
        CNavArea *begin_area = getNavArea(begin);
        CNavArea *end_area   = getNavArea(end);
        if (!begin_area || !end_area)
        {
            // We can't reach the destination vector. Destination vector might
            // be out of bounds/reach.
            crumbs.clear();
            return;
        }
        addTime(begin_area, end_area, time);
    }
    static void reset()
    {
        ignores.clear();
        ResetPather();
    }
    static void updateIgnores()
    {
        static Timer update{};
        if (!update.test_and_set(500))
            return;
        if (crumbs.empty())
        {
            for (auto &i : ignores)
            {
                switch (i.second.status)
                {
                case vischeck_failed:
                case vischeck_success:
                    if (i.second.ignoreTimeout.check(30000))
                    {
                        i.second.status    = unknown;
                        i.second.stucktime = 0;
                    }
                    break;
                case explicit_ignored:
                    if (i.second.ignoreTimeout.check(60000))
                    {
                        i.second.status    = unknown;
                        i.second.stucktime = 0;
                    }
                    break;
                default:
                    break;
                }
            }
        }
        else
            checkPath();
    }
    ignoremanager() = delete;
};
std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata,
                   boost::hash<std::pair<CNavArea *, CNavArea *>>>
    ignoremanager::ignores;

struct Graph : public micropather::Graph
{
    std::unique_ptr<micropather::MicroPather> pather;

    Graph()
    {
        pather =
            std::make_unique<micropather::MicroPather>(this, 3000, 6, true);
    }
    ~Graph() override
    {
    }
    void AdjacentCost(void *state,
                      MP_VECTOR<micropather::StateCost> *adjacent) override
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
            micropather::StateCost cost{ static_cast<void *>(neighbour),
                                         distance };
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
        findClosestNavSquare_localAreas.erase(
            findClosestNavSquare_localAreas.begin());

    auto &areas = navfile->m_areas;
    std::vector<CNavArea *> overlapping;

    for (auto &i : areas)
    {
        // Check if we are within x and y bounds of an area
        if (i.IsOverlapping(vec))
        {
            // Make sure we're not stuck on the same area for too long
            if (std::count(findClosestNavSquare_localAreas.begin(),
                           findClosestNavSquare_localAreas.end(), &i) < 3)
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
        if (vec == g_pLocalPlayer->v_Origin)
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
            if (std::count(findClosestNavSquare_localAreas.begin(),
                           findClosestNavSquare_localAreas.end(), &i) < 3)
            {
                bestDist   = dist;
                bestSquare = &i;
            }
        }
    }
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

    int result =
        Map.pather->Solve(static_cast<void *>(local), static_cast<void *>(dest),
                          &pathNodes, &cost);
    logging::Info(format("Pathing: Pather result: ", result).c_str());
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

bool navTo(Vector destination, int priority, bool should_repath,
           bool nav_to_local, bool is_repath)
{
    if (!prepare())
        return false;
    if (priority < curr_priority)
        return false;
    std::vector<Vector> path = findPath(g_pLocalPlayer->v_Origin, destination);
    if (path.empty())
        return false;
    last_area = path.at(0);
    if (!nav_to_local)
    {
        path.erase(path.begin());
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
    crumbs.clear();
    crumbs = std::move(path);
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
static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "NavParser", 17, []() {
        if (!enabled || status != on)
            return;
        if (CE_BAD(LOCAL_E))
            return;
        if (!LOCAL_E->m_bAlivePlayer())
        {
            // Clear path if player dead
            crumbs.clear();
            return;
        }
        ignoremanager::updateIgnores();
        // Crumbs empty, prepare for next instruction
        if (crumbs.empty())
        {
            curr_priority    = 0;
            ReadyForCommands = true;
            ensureArrival    = false;
            return;
        }
        ReadyForCommands = false;
        // Remove old crumbs
        if (g_pLocalPlayer->v_Origin.DistTo(Vector{
                crumbs.at(0).x, crumbs.at(0).y, crumbs.at(0).z }) < 50.0f)
        {
            last_area = crumbs.at(0);
            crumbs.erase(crumbs.begin());
            inactivity.update();
        }
        if (crumbs.empty())
            return;
        // Detect when jumping is necessary
        if ((!(g_pLocalPlayer->holding_sniper_rifle &&
               g_pLocalPlayer->bZoomed) &&
             crumbs.at(0).z - g_pLocalPlayer->v_Origin.z > 18 &&
             last_jump.test_and_set(200)) ||
            (last_jump.test_and_set(200) && inactivity.check(3000)))
            current_user_cmd->buttons |= IN_JUMP;
        // If inactive for too long
        if (inactivity.check(4000))
        {
            // Ignore connection
            ignoremanager::addTime(last_area, crumbs.at(0), inactivity);
            repath();
            return;
        }
        // Walk to next crumb
        WalkTo(crumbs.at(0));
    });

void ResetPather()
{
    Map.pather->Reset();
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

static CatCommand nav_set("nav_set", "Debug nav find",
                          []() { loc = g_pLocalPlayer->v_Origin; });

static CatCommand nav_init("nav_init", "Debug nav init", []() {
    status = off;
    prepare();
});

static CatCommand nav_path("nav_path", "Debug nav path", []() { navTo(loc); });

static CatCommand nav_reset_ignores("nav_reset_ignores", "Reset all ignores.",
                                    []() { ignoremanager::reset(); });

void clearInstructions()
{
    crumbs.clear();
}

} // namespace nav
