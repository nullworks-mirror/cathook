#pragma once

#include "common.hpp"
#include "micropather.h"
#include "pwd.h"
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

struct inactivityTracker
{
    // Map for storing inactivity per connection
    std::map<std::pair<int, int>, unsigned int> inactives;

    void reset()
    {
        // inactives.clear();
    }
    void addTime(std::pair<int, int> connection, Timer &timer,
                 bool &resetPather)
    {
        if (inactives.find(connection) == inactives.end())
        {
            inactives[connection] = 0;
        }
        inactives[connection] =
            inactives[connection] +
            (std::chrono::duration_cast<std::chrono::milliseconds>(
                 std::chrono::system_clock::now() - timer.last)
                 .count());
        if (inactives[connection] >= 5000)
            resetPather = true;
    }
    void addTime(std::pair<Vector, Vector> connection, Timer &timer,
                 bool &resetPather)
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
            return;

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
            return;

        for (auto i : currnode->m_connections)
        {
            if (i.area->m_id == nextnode->m_id)
            {
                addTime(std::pair{ currnode->m_id, nextnode->m_id }, timer,
                        resetPather);
                return;
            }
        }
    }
    unsigned int getTime(std::pair<int, int> connection)
    {
        return inactives[connection];
    }
};

} // namespace nav
