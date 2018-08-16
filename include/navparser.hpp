#pragma once

#include "common.hpp"

namespace nav
{
extern bool init;
extern bool pathfinding;
std::vector<Vector> findPath(Vector loc, Vector dest);
bool Prepare();
void CreateMove();
}
