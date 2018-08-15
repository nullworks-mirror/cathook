#include "common.hpp"
#include "micropather.h"
namespace nav
{
CNavFile navfile(nullptr);
CNavArea LocalNav;
std::vector<CNavArea> areas;
// std::vector<CNavArea> SniperAreas;

struct MAP : public micropather::Graph
{
    float LeastCostEstimate(void *stateStart, void *stateEnd)
    {
        int *start = (int *) (stateStart);
        int *end   = (int *) (stateEnd);
        return areas.at(*start).m_center.DistTo(areas.at(*end).m_center);
    }
    void AdjacentCost(void *state,
                              MP_VECTOR<micropather::StateCost> *adjacent)
    {
        int *area        = (int *) (state);
        auto &neighbours = areas.at(*area).m_connections;
        for (auto i : neighbours)
        {
            adjacent->push_back(micropather::StateCost{
                (void *) (i.area->m_id),
                i.area->m_center.DistTo(areas.at(*area).m_center) });
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
    //    std::string lvlname(g_IEngine->GetLevelName());
    //    int dotpos = lvlname.find('.');
    //    lvlname  = lvlname.substr(0, dotpos);

    //    std::string lvldir("/home/elite/Schreibtisch/tf2/maps/");
    //    lvldir.append(lvlname);
    //    lvldir.append(".nav");
    // FIXME temp
    std::string lvldir = "/home/elite/Schreibtisch/tf2/maps/cp_dustbowl.nav";

    for (auto &it : areas)
        it = {};
    navfile = CNavFile(lvldir.c_str());
    if (!navfile.m_isOK)
        logging::Info("Invalid Nav File");
    else
    {
        areas.empty();
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
    micropather::MicroPather pather(&TF2MAP, 5000, 8, true);
    logging::Info("Solving");
    pather.Solve((void *) (&id_loc), (void *) (&id_dest), &pathNodes, &cost);
    logging::Info("Converting to vector");
    std::vector<Vector> path;
    for (int i = 0; i < pathNodes.size(); i++)
    {
        path.push_back(areas.at(*(int *) (pathNodes[i])).m_center);
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
    }
    std::string output = "Pathing: Path found! Path: ";
    for (int i = 0; i < path.size(); i++)
    {
        output.append(format(path.at(i).x, ",", format(path.at(i).y), " "));
    }
    logging::Info(output.c_str());
});

//    Timer cd{};
//    void CreateMove()
//    {
//        if (navfile.m_isOK)
//        {
//            if (cd.test_and_set(300)) {
//                for (auto i : navfile.m_areas) {
//            Vector vec = LOCAL_E->m_vecOrigin();
//                    if (i.Contains(vec)) {
//                        LocalNav = i;
//                        break;
//                    }
//                }
//            }
//            if (SniperAreas.size()) {
//                auto res =
//                areas[LocalNav.m_id]->FindPath(areas[SniperAreas[0].m_id]);
//                for (auto r : res)
//                    logging::Info("%f, %f, %f", r->pos.x, r->pos.y, r->pos.z);
//            }
//        }
//    }
} // namespace nav
