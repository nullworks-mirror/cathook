#pragma once

#include "common.hpp"
#include "micropather.h"
#include "pwd.h"
#include <thread>
#include <boost/functional/hash.hpp>
#if ENABLE_VISUALS
#include <glez/draw.hpp>
#endif

namespace nav
{
extern bool init;
extern bool ReadyForCommands;
extern int priority;
extern std::vector<CNavArea> areas;
std::vector<Vector> findPath(Vector loc, Vector dest, int &id_loc,
                             int &id_dest);
bool NavTo(Vector dest, bool navToLocalCenter = true, bool persistent = true,
           int instructionPriority = 5);
void clearInstructions();
int findClosestNavSquare(Vector vec);
bool Prepare();
void CreateMove();
void Draw();

size_t FindInVector(size_t id);

class inactivityTracker
{
    // Map for storing inactivity per connection
    std::unordered_map<std::pair<int, int>, std::pair<int, unsigned int>,
                       boost::hash<std::pair<int, int>>>
        inactives;
    std::unordered_map<int, bool> sentryAreas;
    std::vector<Vector> sentries{};

    bool vischeckConnection(std::pair<int, int> &connection)
    {
        Vector begin = areas.at(FindInVector(connection.first)).m_center;
        Vector end   = areas.at(FindInVector(connection.second)).m_center;
        begin.z += 72;
        end.z += 72;
        bool result = IsVectorVisible(begin, end, false);
        return result;
    }

    std::pair<int, int> VectorToId(std::pair<Vector, Vector> &connection)
    {
        CNavArea *currnode = nullptr;
        for (size_t i = 0; i < areas.size(); i++)
        {
            if (areas.at(i).m_center == connection.first)
            {
                currnode = &areas.at(i);
                break;
            }
        }
        if (!currnode)
            return { -1, -1 };

        CNavArea *nextnode = nullptr;
        for (size_t i = 0; i < areas.size(); i++)
        {
            if (areas.at(i).m_center == connection.second)
            {
                nextnode = &areas.at(i);
                break;
            }
        }
        if (!nextnode)
            return { -1, -1 };

        for (auto i : currnode->m_connections)
        {
            if (i.area->m_id == nextnode->m_id)
            {
                return { currnode->m_id, nextnode->m_id };
            }
        }
    }

public:
    void reset()
    {
        for (auto i : inactives)
        {
            // What is this tf
            i.second.second = 0;
        }
    }
    bool ShouldCancelPath(std::vector<Vector> crumbs)
    {
        for (auto sentry : sentries)
            for (auto crumb : crumbs)
            {
                if (crumb.DistTo(sentry) > 1100)
                    continue;
                if (!IsVectorVisible(crumb, sentry, true))
                    continue;
                return true;
            }
         return false;
    }
    void updateSentries()
    {
        sentryAreas.clear();
        sentries.clear();
        for (int i = 0; i < HIGHEST_ENTITY; i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CObjectSentrygun) || ent->m_iTeam() == LOCAL_E->m_iTeam())
                continue;
            Vector sentryloc = GetBuildingPosition(ent);
            sentries.push_back(sentryloc);
            for (auto i : areas)
            {
                Vector area = i.m_center;
                area.z += 83.0f;
                if (area.DistTo(sentryloc) > 1100.0f)
                    continue;
                if (!IsVectorVisible(area, sentryloc, true))
                    continue;
                sentryAreas[i.m_id] = true;
            }
        }
    }
    bool IsIgnored(std::pair<int, int> connection)
    {
        if (sentryAreas[connection.first] ||
            sentryAreas[connection.second])
        {
            logging::Info("Ignored a connection due to sentry gun coverage");
            return true;
        }
        if (inactives.find(connection) == inactives.end())
            return false;
        auto &pair = inactives.at(connection);
        if (pair.second >= 5000)
        {
            pair.first = 1;
            return true;
        }
        if (pair.first == 2 && !vischeckConnection(connection))
        {
            logging::Info(
                "Ignored a connection due to type 2 connection type.");
            return true;
        }
        return false;
    }

    void AddTime(std::pair<int, int> connection, Timer &timer,
                 bool &resetPather)
    {
        if (inactives.find(connection) == inactives.end())
        {
            inactives[connection] = { 0, 0 };
        }
        auto &pair = inactives.at(connection);

        unsigned int newTime =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - timer.last)
                .count();
        if (pair.first == 2 && !vischeckConnection(connection))
            newTime = (newTime > 2500 ? 2500 : newTime);
        pair.second = pair.second + newTime;
        if (pair.second >= 5000)
            resetPather = true;
    }
    void AddTime(std::pair<Vector, Vector> connection, Timer &timer,
                 bool &resetPather)
    {
        auto pair = VectorToId(connection);
        if (pair.first == -1 || pair.second == -1)
            return;
        AddTime(pair, timer, resetPather);
    }
    bool CheckType2(std::pair<int, int> connection)
    {
        // Fix calls vischeckConnection too often
        if (inactives.find(connection) == inactives.end())
        {
            inactives[connection] = { 0, 0 };
        }
        auto pair = inactives.at(connection);
        switch (pair.first)
        {
        case 0:
            if (!vischeckConnection(connection))
            {
                inactives[connection].first = 2;
                return true;
            }
        case 1:
        case 2:
            return false;
        }
    }
    bool CheckType2(std::pair<Vector, Vector> connection)
    {
        auto pair = VectorToId(connection);
        if (pair.first == -1 || pair.second == -1)
            return false;
        return CheckType2(pair);
    }
}; // namespace nav

struct MAP : public micropather::Graph
{
    std::unique_ptr<micropather::MicroPather> pather;
    // Maps already utilize dynamic allocation and we don't need a custom
    // constructor
    inactivityTracker inactiveTracker;
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

    float GetZBetweenAreas(CNavArea *start, CNavArea *end)
    {
        float z1 = GetClosestCornerToArea(start, end).z;
        float z2 = GetClosestCornerToArea(end, start).z;

        return z2 - z1;
    }
    // Function required by MicroPather for getting an estimated cost
    float LeastCostEstimate(void *stateStart, void *stateEnd)
    {
        CNavArea *start = static_cast<CNavArea *>(stateStart);
        CNavArea *end   = static_cast<CNavArea *>(stateEnd);
        float dist      = start->m_center.DistTo(end->m_center);
        return dist;
    }
    // Function required by MicroPather to retrieve neighbours and their
    // associated costs.
    void AdjacentCost(void *state, MP_VECTOR<micropather::StateCost> *adjacent)
    {
        CNavArea *area   = static_cast<CNavArea *>(state);
        auto &neighbours = area->m_connections;
        for (auto i : neighbours)
        {
            if (GetZBetweenAreas(area, i.area) > 42)
                continue;
            if (inactiveTracker.IsIgnored(
                    std::pair{ area->m_id, i.area->m_id }))
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
        CNavArea *area = static_cast<CNavArea *>(state);
        logging::Info(format(area->m_center.x, " ", area->m_center.y, " ",
                             area->m_center.z)
                          .c_str());
    }
    MAP(size_t size)
    {
        pather =
            std::make_unique<micropather::MicroPather>(this, size, 6, true);
    }

    ~MAP()
    {
    }
};

} // namespace nav
