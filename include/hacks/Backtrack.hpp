/*
 * Backtrack.hpp
 *
 *  Created on: May 15, 2018
 *      Author: bencat07
 */

#pragma once
#include "common.hpp"

namespace hacks {
namespace shared {
namespace backtrack {
struct BacktrackData
{
	int tickcount;
	Vector hitboxpos;
};
void Init();
void Run();
void Draw();
extern BacktrackData headPositions[24][12];
}
}
}
