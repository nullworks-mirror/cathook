#pragma once

#include "common.hpp"

namespace nav
{
extern bool init;
extern bool ReadyForCommands;
extern std::vector<CNavArea> areas;
std::vector<Vector> findPath(Vector loc, Vector dest);
bool NavTo(Vector dest);
bool Prepare();
void CreateMove();
}
