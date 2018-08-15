#include "common.hpp"
#include "micropather.h"
#include "pwd.h"
namespace nav
{
static CNavFile navfile(nullptr);
// Todo: CNavArea* to navfile
static std::vector<CNavArea> areas;
// std::vector<CNavArea> SniperAreas;

// Todo fix
int FindInVector(int id)
{
    for (int i = 0; i < areas.size(); i++)
    {
        if (areas.at(i).m_id == id)
            return i;
    }
}

struct MAP : public micropather::Graph
{
    float LeastCostEstimate(void *stateStart, void *stateEnd)
    {
        CNavArea *start = static_cast<CNavArea *>(stateStart);
        CNavArea *end   = static_cast<CNavArea *>(stateEnd);
        float dist      = start->m_center.DistTo(end->m_center);
        return dist;
    }
    void AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent)
    {
        CNavArea *area   = static_cast<CNavArea *>(state);
        auto &neighbours = area->m_connections;
        for (auto i : neighbours)
        {
            micropather::StateCost cost;
            cost.state =
                static_cast<void *>(&areas.at(FindInVector(i.area->m_id)));
            cost.cost = area->m_center.DistTo(i.area->m_center);
            adjacent->push_back(cost);
        }
    }
    void PrintStateInfo(void *state)
    {
    }

    MAP()
    {
        logging::Info("Init Map");
    }

    virtual ~MAP()
    {
    }
};

void Init()
{
    // TODO: Improve performance
    std::string lvlname(g_IEngine->GetLevelName());
    int dotpos = lvlname.find('.');
    lvlname    = lvlname.substr(0, dotpos);

    std::string lvldir("/home/");
    passwd *pwd     = getpwuid(getuid());
    lvldir.append(pwd->pw_name);
    lvldir.append("/.steam/steam/steamapps/common/Team Fortress 2/tf/");
    lvldir.append(lvlname);
    lvldir.append(".nav");
    logging::Info(lvldir.c_str());

    areas.empty();
    navfile = CNavFile(lvldir.c_str());
    if (!navfile.m_isOK)
        logging::Info("Invalid Nav File");
    else
    {
        areas.reserve(navfile.m_areas.size());
        for (auto i : navfile.m_areas)
            areas.push_back(i);
    }
}

int findClosestNavSquare(Vector vec)
{
    float bestDist = 999999.0f;
    int bestSquare = -1;
    for (int i = 0; i < areas.size(); i++)
    {
        float dist = areas.at(i).m_center.DistTo(vec);
        if (dist < bestDist)
        {
            bestDist   = dist;
            bestSquare = i;
        }
    }
    return bestSquare;
}

std::vector<Vector> findPath(Vector loc, Vector dest)
{
    logging::Info("Starting findPath");
    if (areas.empty())
        return std::vector<Vector>(0);
    logging::Info("Finding closest Squares");
    int id_loc  = findClosestNavSquare(loc);
    int id_dest = findClosestNavSquare(dest);
    logging::Info("Initiating path_Nodes");
    micropather::MPVector<void *> pathNodes;
    float cost;
    logging::Info("Initiating map");
    MAP TF2MAP;
    logging::Info("Initiating pather");
    // Todo: Make MicroPather a member of TF2MAP
    micropather::MicroPather pather(&TF2MAP, areas.size(), 8, true);
    logging::Info("Solving");
    int result = pather.Solve(static_cast<void *>(&areas.at(id_loc)),
                              static_cast<void *>(&areas.at(id_dest)),
                              &pathNodes, &cost);
    logging::Info(format("Result:", result).c_str());
    logging::Info("Converting to vector");
    std::vector<Vector> path;
    for (int i = 0; i < pathNodes.size(); i++)
    {
        path.push_back(static_cast<CNavArea *>(pathNodes[i])->m_center);
    }
    return path;
}

CatCommand navinit("nav_init", "Debug nav init",
                   [](const CCommand &args) { Init(); });

Vector loc;

CatCommand navset("nav_set", "Debug nav set",
                  [](const CCommand &args) { loc = g_pLocalPlayer->v_Origin; });

CatCommand navfind("nav_find", "Debug nav find", [](const CCommand &args) {
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

} // namespace nav
