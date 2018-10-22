#include "common.hpp"
#include "CNavFile.h"
#include <boost/thread/scoped_thread.hpp>
#include <micropather.h>

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
        for (auto i : center->m_connections)
        {
            CNavArea *neighbour = i.area;
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
static std::unique_ptr<CNavFile> navfile;
static std::mutex navfile_lock;

// Thread and status of thread
static std::atomic<thread_status> status;
static boost::scoped_thread thread;

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

void init()
{
    if (status == initing)
        return;
    status = initing;
    thread = boost::scoped_thread(initThread);
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
                           findClosestNavSquare_localAreas.end(), i) < 3)
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

static CatCommand nav_set("nav_set", "Debug nav find", []() {
    loc = g_pLocalPlayer->v_Origin;
});

} // namespace nav
