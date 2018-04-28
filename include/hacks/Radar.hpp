/*
 * Radar.hpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "visual/atlas.hpp"
#include "common.hpp"

namespace hacks
{
namespace tf
{
namespace radar
{

extern CatVar size;
extern CatVar zoom;
extern CatVar radar_enabled;
extern CatVar radar_x;
extern CatVar radar_y;

void Init();
std::pair<int, int> WorldToRadar(int x, int y);
void Draw();
}
}
}
