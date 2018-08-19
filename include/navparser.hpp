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
std::vector<Vector> findPath(Vector loc, Vector dest, int &id_loc, int &id_dest);
bool NavTo(Vector dest, bool navToLocalCenter = true, bool persistent = true, int instructionPriority = 5);
void clearInstructions();
int findClosestNavSquare(Vector vec);
bool Prepare();
void CreateMove();
void Draw();
} // namespace nav
