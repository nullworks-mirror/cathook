#include <boost/functional/hash.hpp>
#include "common.hpp"
#include "CNavFile.h"
#include <thread>
#include "micropather.h"
#include <pwd.h>
#include "settings/Bool.hpp"

namespace nav
{

static settings::Bool enabled{ "misc.pathing", "true" };

enum thread_status
{
    off = 0,
    unavailable,
    initing,
    on
};

std::vector<Vector> crumbs;
std::unique_ptr<CNavFile> navfile;
void ResetPather();

struct ignoredata
{
    bool isignored{ false };
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

class ignoremanager
{
public:
    static bool isIgnored(CNavArea *begin, CNavArea *end)
    {
        if (ignores().find({ begin, end }) == ignores().end())
            return false;
        if (ignores()[{ begin, end }].isignored)
        {
            logging::Info("Ignored Connection %i-&%", begin->m_id, end->m_id);
            return true;
        }
        else
            return false;
    }
    static void addTime(CNavArea *begin, CNavArea *end, Timer &time)
    {
        // Check if connection is already known
        if (ignores().find({ begin, end }) == ignores().end())
        {
            ignores()[{ begin, end }] = {};
        }
        ignoredata &connection = ignores()[{ begin, end }];
        if (connection.isignored)
            // Wtf
            return;
        connection.stucktime +=
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - time.last)
                .count();
        if (connection.stucktime > 5000)
        {
            connection.isignored = true;
            connection.ignoreTimeout.update();
            logging::Info("Ignored Connection &i-&i", begin->m_id, end->m_id);
            ResetPather();
        }
    }
    static void addTime(Vector &begin, Vector &end, Timer &time)
    {
        // todo: check nullptr
        addTime(getNavArea(begin), getNavArea(end), time);
    }
    static std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata,
    boost::hash<std::pair<CNavArea *, CNavArea *>>> &ignores()
    {
        static std::unordered_map<std::pair<CNavArea *, CNavArea *>, ignoredata,
                                  boost::hash<std::pair<CNavArea *, CNavArea *>>>
            data;
        return data;
    }
    ignoremanager() = delete;
};

struct Graph : public micropather::Graph
{
    std::unique_ptr<micropather::MicroPather> pather;

    Graph()
    {
        pather =
            std::make_unique<micropather::MicroPather>(this, 3000, 6, true);
    }
    ~Graph()
    {
    }
    void AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent)
    {
        CNavArea *center = static_cast<CNavArea *>(state);
        for (auto &i : center->m_connections)
        {
            CNavArea *neighbour = i.area;
            if (ignoremanager::isIgnored(center, neighbour))
                continue;
            micropather::StateCost cost{ static_cast<void *>(neighbour),
                                         center->m_center.DistTo(
                                             i.area->m_center) };
            adjacent->push_back(cost);
        }
    }
    float LeastCostEstimate(void *stateStart, void *stateEnd)
    {
        CNavArea *start = static_cast<CNavArea *>(stateStart);
        CNavArea *end   = static_cast<CNavArea *>(stateEnd);
        return start->m_center.DistTo(end->m_center);
    }
    void PrintStateInfo(void *)
    {
        // Uhh no
    }
};

// Navfile containing areas and its lock

std::mutex navfile_lock;
// Thread and status of thread
static std::atomic<thread_status> status;
static std::thread thread;

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

    std::lock_guard<std::mutex> lock(navfile_lock);
    navfile = std::make_unique<CNavFile>(lvldir.c_str());

    if (!navfile->m_isOK)
    {
        navfile.reset();
        status = off;
    }
    logging::Info("Pather: Initing with %i Areas", navfile->m_areas.size());
    status = on;
}

// todo: lowercase
void init()
{
    if (status == initing)
        return;
    status = initing;
    thread = std::thread(initThread);
}

bool prepare()
{
    if (!enabled)
        return false;
    thread_status fast_status = status;
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
    std::lock_guard<std::mutex> lock(navfile_lock);
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
    path.push_back(dest->m_center);
    return path;
}

static Vector loc(0.0f, 0.0f, 0.0f);
static Vector last_area(0.0f, 0.0f, 0.0f);
int priority              = 0;
bool ReadyForCommands     = true;
static bool ensureArrival = false;
static Timer inactivity{};

bool navTo(Vector destination)
{
    std::vector<Vector> path = findPath(g_pLocalPlayer->v_Origin, destination);
    if (!prepare())
        return false;
    if (path.empty())
        return true;
    inactivity.update();
    findClosestNavSquare_localAreas.clear();
    crumbs.clear();
    crumbs = std::move(path);
    return false;
}

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
        // Crumbs empty, prepare for next instruction
        if (crumbs.empty())
        {
            priority         = 0;
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
        if (inactivity.check(5000))
        {
            if (crumbs.size() > 1)
                ignoremanager::addTime(last_area, crumbs.at(0), inactivity);
            crumbs.clear();
        }
        if (crumbs.empty())
            return;
        // Detect when jumping is necessary
        //        if ((crumbs.at(0).z - g_pLocalPlayer->v_Origin.z > 18 &&
        //             lastJump.test_and_set(200)) ||
        //            (lastJump.test_and_set(200) && inactivity.check(2000)))
        //            current_user_cmd->buttons |= IN_JUMP;
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

static CatCommand nav_init("nav_init", "Debug nav find", []() { prepare(); });

static CatCommand nav_path("nav_path", "Debug nav path", []() { navTo(loc); });

void yeet()
{
}

} // namespace nav
