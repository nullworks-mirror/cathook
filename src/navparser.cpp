#include "common.hpp"
#include "micropather.h"
#include "pwd.h"
#include "navparser.hpp"

namespace nav
{
static CNavFile navfile(nullptr);
std::vector<CNavArea> areas;
// std::vector<CNavArea> SniperAreas;
bool init        = false;
bool pathfinding = true;
bool ReadyForCommands = false;
static settings::Bool enabled{ "misc.pathing", "true" };

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
    Vector GetClosestCornerToArea(CNavArea *CornerOf, CNavArea *Target)
    {
        std::array<Vector, 4> corners;
        corners.at(0) = CornerOf->m_nwCorner; // NW
        corners.at(1) = CornerOf->m_seCorner; // SE
        corners.at(2) = Vector{ CornerOf->m_seCorner.x, CornerOf->m_nwCorner.y,
                                CornerOf->m_nwCorner.z }; // NE
        corners.at(3) = Vector{ CornerOf->m_nwCorner.x, CornerOf->m_seCorner.y,
                                CornerOf->m_seCorner.z }; // SW

        Vector bestVec;
        float bestDist = FLT_MAX;

        for (int i = 0; i < corners.size(); i++)
        {
            float dist = corners.at(i).DistTo(Target->m_center);
            if (dist < bestDist)
            {
                bestVec  = corners.at(i);
                bestDist = dist;
            }
        }

        Vector bestVec2;
        float bestDist2 = FLT_MAX;

        for (int i = 0; i < corners.size(); i++)
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

    float GetZBetweenAreas(CNavArea *start, CNavArea *end)
    {
        float z1 = GetClosestCornerToArea(start, end).z;
        float z2 = GetClosestCornerToArea(end, start).z;

        return z2 - z1;
    }
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
            if (GetZBetweenAreas(area, i.area) > 42)
                continue;
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
    }

    ~MAP()
    {
    }
};

std::unique_ptr<MAP> TF2MAP;
std::unique_ptr<micropather::MicroPather> pather;

void Init()
{
    // TODO: Improve performance
    std::string lvlname(g_IEngine->GetLevelName());
    int dotpos = lvlname.find('.');
    lvlname    = lvlname.substr(0, dotpos);

    std::string lvldir("/home/");
    passwd *pwd = getpwuid(getuid());
    lvldir.append(pwd->pw_name);
    lvldir.append("/.steam/steam/steamapps/common/Team Fortress 2/tf/");
    lvldir.append(lvlname);
    lvldir.append(".nav");
    logging::Info(format("Pathing: Nav File location: ", lvldir).c_str());

    areas.clear();
    navfile = CNavFile(lvldir.c_str());
    if (!navfile.m_isOK)
        logging::Info("Pathing: Invalid Nav File");
    else
    {
        int size = navfile.m_areas.size();
        logging::Info(
            format("Pathing: Number of areas to index:", size).c_str());
        areas.reserve(size);
        for (auto i : navfile.m_areas)
            areas.push_back(i);
        if (size > 7000)
            size = 7000;
        TF2MAP = std::make_unique<MAP>();
        pather = std::make_unique<micropather::MicroPather>(TF2MAP.get(), size,
                                                            6, true);
    }
    pathfinding = true;
}

bool Prepare()
{
    if (!enabled)
        return false;
    if (!init)
    {
        pathfinding = false;
        init        = true;
        Init();
    }
    if (!pathfinding)
        return false;
    return true;
}

int findClosestNavSquare(Vector vec)
{
    float bestDist = FLT_MAX;
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
    if (areas.empty())
        return std::vector<Vector>(0);
    int id_loc  = findClosestNavSquare(loc);
    int id_dest = findClosestNavSquare(dest);
    if (id_loc == -1 || id_dest == -1)
        return std::vector<Vector>(0);
    micropather::MPVector<void *> pathNodes;
    // MAP TF2MAP;
    // micropather::MicroPather pather(&TF2MAP, areas.size(), 8, true);
    float cost;
    int result = pather->Solve(static_cast<void *>(&areas.at(id_loc)),
                               static_cast<void *>(&areas.at(id_dest)),
                               &pathNodes, &cost);
    logging::Info(format(result).c_str());
    if (result)
        return std::vector<Vector>(0);
    std::vector<Vector> path;
    for (int i = 0; i < pathNodes.size(); i++)
    {
        path.push_back(static_cast<CNavArea *>(pathNodes[i])->m_center);
    }
    path.push_back(dest);
    return path;
}

static Timer inactivity{};
Timer lastJump{};
static std::vector<Vector> crumbs;

bool NavTo(Vector dest)
{
    if (CE_BAD(LOCAL_E))
        return false;
    if (!Prepare())
        return false;
    auto path = findPath(g_pLocalPlayer->v_Origin, dest);
    if (path.empty())
        return false;
    crumbs.clear();
    crumbs = std::move(path);
    inactivity.update();
    return true;
}

void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E))
        return;
    if (crumbs.empty())
    {
        ReadyForCommands = true;
        return;
    }
    ReadyForCommands = false;
    if (g_pLocalPlayer->v_Origin.DistTo(crumbs.at(0)) < 30.0f)
    {
        crumbs.erase(crumbs.begin());
        inactivity.update();
    }
    if (crumbs.empty())
        return;
    if (crumbs.at(0).z - g_pLocalPlayer->v_Origin.z > 18 &&
        lastJump.test_and_set(200))
        current_user_cmd->buttons |= IN_JUMP;
    if (inactivity.test_and_set(5000))
    {
        logging::Info("NavBot inactive for too long. Canceling tasks...");
        crumbs.clear();
        return;
    }
    WalkTo(crumbs.at(0));
}

CatCommand navinit("nav_init", "Debug nav init",
                   [](const CCommand &args) { Prepare(); });

Vector loc;

CatCommand navset("nav_set", "Debug nav set",
                  [](const CCommand &args) { loc = LOCAL_E->m_vecOrigin(); });

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

CatCommand navpath("nav_path", "Debug nav path", [](const CCommand &args) {
    if (NavTo(loc))
    {
        logging::Info("Pathing: Success! Walking to path...");
    }
    else
    {
        logging::Info("Pathing: Failed!");
    }
});

} // namespace nav
