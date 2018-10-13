#include "navparser.hpp"

namespace nav
{
static CNavFile navfile(nullptr);
// Vector containing areas on map
std::vector<CNavArea> areas;
bool init               = false;
static bool pathfinding = true;
bool ReadyForCommands   = false;
static settings::Bool enabled{ "misc.pathing", "true" };
static settings::Bool draw{ "misc.pathing.draw", "false" };
static std::atomic<bool> threadingFinished;

static std::unique_ptr<MAP> TF2MAP;

// Function to get place in Vector by connection ID
// Todo: find an alternative for this, maybe a map for storing ptrs to the
// std::vector?
int FindInVector(size_t id)
{
    for (int i = 0; i < areas.size(); i++)
    {
        if (areas.at(i).m_id == id)
            return i;
    }
    return -1;
}
static int bestarea = -1;
Timer reselect{};
int FindNearestValidbyDist(Vector vec, float mindist, float maxdist, bool closest)
{
    if (reselect.test_and_set(500))
    {
        float bestscr = FLT_MAX;
            bestarea = -1;
            for (int ar = 0; ar < areas.size(); ar++)
            {
                Vector area = areas[ar].m_center;
                area.z += 72.0f;
                float scr = area.DistTo(vec);
                if (scr > maxdist || scr < mindist)
                    continue;
                if (closest)
                {
                    if (scr > bestscr)
                        continue;

                }
                else if (scr < bestscr)
                    continue;
                if (IsVectorVisible(vec, area, false))
                {
                    bestscr  = scr;
                    bestarea = ar;
                }
            }
    }
    return bestarea;
}
int FindNearestValid(Vector vec)
{
    if (reselect.test_and_set(500))
    {
        float bestscr = FLT_MAX;
        if (bestarea != -1)
        {
            bool success = false;
            Vector area  = areas[bestarea].m_center;
            area.z += 72.0f;
            if (IsPlayerDisguised(LOCAL_E) ||
                !TF2MAP->inactiveTracker.sentryAreas[bestarea])
            {
                float scr = area.DistTo(vec);
                if (scr < 2000.0f)
                    if (IsVectorVisible(vec, area, false))
                        success = true;
            }
            if (!success)
                bestarea = -1;
        }
        else
            for (int ar = 0; ar < areas.size(); ar++)
            {
                Vector area = areas[ar].m_center;
                area.z += 72.0f;
                if (!IsPlayerDisguised(LOCAL_E) &&
                    TF2MAP->inactiveTracker.sentryAreas[ar])
                    continue;
                float scr = area.DistTo(vec);
                if (scr > 2000.0f)
                    continue;
                if (scr > bestscr)
                    continue;
                if (IsVectorVisible(vec, area, false))
                {
                    bestscr  = scr;
                    bestarea = ar;
                }
            }
    }
    return bestarea;
}
void Init()
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

    areas.clear();
    // Load the NavFile
    navfile = CNavFile(lvldir.c_str());
    if (!navfile.m_isOK)
        logging::Info("Pathing: Invalid Nav File");
    else
    {
        size_t size = navfile.m_areas.size();
        logging::Info(
            format("Pathing: Number of areas to index:", size).c_str());
        areas.reserve(size);
        // Add areas from CNavFile to our Vector
        for (auto i : navfile.m_areas)
            areas.push_back(i);
        // Don't reserve more than 7000 states
        if (size > 7000)
            size = 7000;
        // Initiate "Map", contains micropather object
        TF2MAP = std::make_unique<MAP>(size);
        TF2MAP->inactiveTracker.reset();
    }
    if (!areas.empty())
        pathfinding = true;
    threadingFinished.store(true);
}

static std::string lastmap;
// Function that decides if pathing is possible, and inits pathing if necessary
bool Prepare()
{
    if (!enabled)
        return false;
    if (!init)
    {
        // Don't reinit if same map
        if (lastmap == g_IEngine->GetLevelName())
        {
            init = true;
        }
        else
        {
            lastmap     = g_IEngine->GetLevelName();
            pathfinding = false;
            init        = true;
            threadingFinished.store(false);
            // Parsing CNavFile takes time, run it in a seperate thread
            std::thread initer(Init);
            // We need to either detach or join to avoid std::terminate
            initer.detach();
        }
    }
    if (!pathfinding || !threadingFinished.load())
        return false;
    return true;
}

// This prevents the bot from gettings completely stuck in some cases
static std::vector<int> findClosestNavSquare_localAreas(6);
// Function for getting closest Area to player, aka "LocalNav"
int findClosestNavSquare(Vector vec)
{
    if (findClosestNavSquare_localAreas.size() > 5)
        findClosestNavSquare_localAreas.erase(
            findClosestNavSquare_localAreas.begin());
    std::vector<std::pair<int, CNavArea *>> overlapping;
    for (size_t i = 0; i < areas.size(); i++)
    {
        // Check if we are within x and y bounds of an area
        if (areas.at(i).IsOverlapping(vec))
        {
            // Make sure we're not stuck on the same area for too long
            if (std::count(findClosestNavSquare_localAreas.begin(),
                           findClosestNavSquare_localAreas.end(), i) < 3)
                overlapping.emplace_back(i, &areas.at(i));
        }
    }

    // If multiple candidates for LocalNav have been found, pick the closest
    float bestDist = FLT_MAX;
    int bestSquare = -1;
    for (size_t i = 0; i < overlapping.size(); i++)
    {
        float dist = overlapping.at(i).second->m_center.DistTo(vec);
        if (dist < bestDist)
        {
            bestDist   = dist;
            bestSquare = overlapping.at(i).first;
        }
    }
    if (bestSquare != -1)
    {
        if (vec == g_pLocalPlayer->v_Origin)
            findClosestNavSquare_localAreas.push_back(bestSquare);
        return bestSquare;
    }
    // If no LocalNav was found, pick the closest available Area
    for (size_t i = 0; i < areas.size(); i++)
    {
        float dist = areas.at(i).m_center.DistTo(vec);
        if (dist < bestDist)
        {
            if (std::count(findClosestNavSquare_localAreas.begin(),
                           findClosestNavSquare_localAreas.end(), i) < 3)
            {
                bestDist   = dist;
                bestSquare = i;
            }
        }
    }
    if (vec == g_pLocalPlayer->v_Origin)
        findClosestNavSquare_localAreas.push_back(bestSquare);
    return bestSquare;
}

std::vector<Vector> findPath(Vector loc, Vector dest, int &id_loc, int &id_dest)
{
    if (areas.empty())
        return std::vector<Vector>(0);
    // Get areas of Vector loc and dest
    id_loc  = findClosestNavSquare(loc);
    id_dest = findClosestNavSquare(dest);
    if (id_loc == -1 || id_dest == -1)
        return std::vector<Vector>(0);
    float cost;
    micropather::MPVector<void *> pathNodes;
    // Find a solution to get to location
    int result = TF2MAP->pather->Solve(static_cast<void *>(&areas.at(id_loc)),
                                       static_cast<void *>(&areas.at(id_dest)),
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
    path.push_back(dest);
    return path;
}

// Timer for measuring inactivity, aka time between not reaching a crumb
static Timer inactivity{};
// Time since our last Jump
static Timer lastJump{};
// std::vector containing our path
static std::vector<Vector> crumbs;
// Bot will keep trying to get to the target even if it fails a few times
static bool ensureArrival;
// Priority value for current instructions, only higher or equal priorites can
// overwrite it
int priority               = 0;
static Vector lastArea     = { 0.0f, 0.0f, 0.0f };
static int persistentTries = 0;

// dest = Destination, navToLocalCenter = Should bot travel to local center
// first before resuming pathing activity? (Increases accuracy) persistent =
// ensureArrival above, instructionPriority = priority above
bool NavTo(Vector dest, bool navToLocalCenter, bool persistent,
           int instructionPriority)
{
    if (CE_BAD(LOCAL_E))
        return false;
    if (!Prepare())
        return false;
    // Only allow instructions to overwrite others if their priority is higher
    if (instructionPriority < priority)
        return false;
    int locNav = 0, tarNav = 0;
    auto path = findPath(g_pLocalPlayer->v_Origin, dest, locNav, tarNav);
    if (path.empty())
        return false;
    if (!crumbs.empty())
    {
        bool reset = false;
        TF2MAP->inactiveTracker.AddTime({ lastArea, crumbs.at(0) }, inactivity,
                                        reset);
        if (reset)
            TF2MAP->pather->Reset();
    }
    crumbs.clear();
    crumbs = std::move(path);
    if (crumbs.empty())
        return false;
    lastArea = crumbs.at(0);
    if (!navToLocalCenter && crumbs.size() > 1)
        crumbs.erase(crumbs.begin());
    ensureArrival = persistent;
    findClosestNavSquare_localAreas.clear();
    priority = instructionPriority;
    inactivity.update();
    persistentTries = 0;
    return true;
}

void clearInstructions()
{
    crumbs.clear();
}

static Timer ignoreReset{};
static Timer patherReset{};
static Timer sentryUpdate{};
static Timer sentryClear{};
static Timer sentryCheck{};
// Function for removing ignores
void ignoreManagerCM()
{
    if (!TF2MAP || !TF2MAP->pather)
        return;
    if (ignoreReset.test_and_set(120000))
        TF2MAP->inactiveTracker.reset();
    if (patherReset.test_and_set(30000))
        TF2MAP->pather->Reset();
    if (sentryClear.test_and_set(20000))
        TF2MAP->inactiveTracker.ClearSentries();
    if (sentryUpdate.test_and_set(500))
        TF2MAP->inactiveTracker.AddSentries();
}

void Repath()
{
    if (ensureArrival && persistentTries < 10)
    {
        logging::Info("Pathing: NavBot inactive for too long. Ignoring "
                      "connection and finding another path...");
        // Throwaway int
        int i1 = 0, i2 = 0;
        // Find a new path
        TF2MAP->pather->Reset();
        crumbs = findPath(g_pLocalPlayer->v_Origin, crumbs.back(), i1, i2);
        persistentTries++;
    }
    else
    {
        logging::Info(
            "Pathing: NavBot inactive for too long. Canceling tasks and "
            "ignoring connection...");
        // Wait for new instructions
        TF2MAP->pather->Reset();
        crumbs.clear();
    }
}

// Main movement function, gets path from NavTo
static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "NavParser", 17, []() {
        if (!enabled || !threadingFinished.load())
            return;
        if (CE_BAD(LOCAL_E))
            return;
        if (!LOCAL_E->m_bAlivePlayer())
        {
            // Clear path if player dead
            crumbs.clear();
            return;
        }
        ignoreManagerCM();
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
            lastArea = crumbs.at(0);
            crumbs.erase(crumbs.begin());
            inactivity.update();
        }
        if (crumbs.empty())
            return;
        // Detect when jumping is necessary
        if ((crumbs.at(0).z - g_pLocalPlayer->v_Origin.z > 18 &&
             lastJump.test_and_set(200)) ||
            (lastJump.test_and_set(200) && inactivity.check(2000)))
            current_user_cmd->buttons |= IN_JUMP;
        // Check if were dealing with a type 2 connection
        if (inactivity.check(3000) &&
            TF2MAP->inactiveTracker.CheckType2({ lastArea, crumbs.at(0) }))
        {
            logging::Info("Pathing: Type 2 connection detected!");
            TF2MAP->pather->Reset();
            Repath();
            inactivity.update();
            return;
        }
        // Check for new sentries
        if (sentryCheck.test_and_set(500) &&
            TF2MAP->inactiveTracker.ShouldCancelPath(crumbs))
        {
            logging::Info("Pathing: New Sentry found!");
            TF2MAP->pather->Reset();
            Repath();
            return;
        }
        // If inactive for too long
        if (inactivity.check(3000))
        {
            // Ignore connection
            bool resetPather = false;
            TF2MAP->inactiveTracker.AddTime({ lastArea, crumbs.at(0) },
                                            inactivity, resetPather);
            if (resetPather)
                TF2MAP->pather->Reset();
            Repath();
            inactivity.update();
            return;
        }
        // Walk to next crumb
        WalkTo(crumbs.at(0));
    });

void Draw()
{
#if ENABLE_VISUALS
    if (!enabled || !draw)
        return;
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E))
        return;
    if (!LOCAL_E->m_bAlivePlayer())
        return;
    if (crumbs.size() < 2)
        return;
    for (size_t i = 0; i < crumbs.size() - 1; i++)
    {
        Vector wts1, wts2;
        if (draw::WorldToScreen(crumbs[i], wts1) &&
            draw::WorldToScreen(crumbs[i + 1], wts2))
        {
            glez::draw::line(wts1.x, wts1.y, wts2.x - wts1.x, wts2.y - wts1.y,
                             colors::white, 0.3f);
        }
    }
    Vector wts;
    if (!draw::WorldToScreen(crumbs[0], wts))
        return;
    glez::draw::rect(wts.x - 4, wts.y - 4, 8, 8, colors::white);
    glez::draw::rect_outline(wts.x - 4, wts.y - 4, 7, 7, colors::white, 1.0f);
#endif
}

CatCommand navinit("nav_init", "Debug nav init", [](const CCommand &args) {
    lastmap = "";
    init    = false;
    Prepare();
});

Vector loc;

CatCommand navset("nav_set", "Debug nav set",
                  [](const CCommand &args) { loc = LOCAL_E->m_vecOrigin(); });
CatCommand navprint("nav_print", "Debug nav print", [](const CCommand &args) {
    logging::Info(
        format(findClosestNavSquare(g_pLocalPlayer->v_Origin)).c_str());
});

CatCommand navfind("nav_find", "Debug nav find", [](const CCommand &args) {
    int i1 = 0, i2 = 0;
    std::vector<Vector> path = findPath(g_pLocalPlayer->v_Origin, loc, i1, i2);
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
    if (NavTo(loc, true, true, 50 + priority))
    {
        logging::Info("Pathing: Success! Walking to path...");
    }
    else
    {
        logging::Info("Pathing: Failed!");
    }
});

// Clang format pls
CatCommand navpathnolocal("nav_path_nolocal", "Debug nav path",
                          [](const CCommand &args) {
                              if (NavTo(loc, false, true, 50 + priority))
                              {
                                  logging::Info(
                                      "Pathing: Success! Walking to path...");
                              }
                              else
                              {
                                  logging::Info("Pathing: Failed!");
                              }
                          });

} // namespace nav
