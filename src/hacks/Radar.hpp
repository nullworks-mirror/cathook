/*
 * Radar.hpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#ifndef HACKS_RADAR_HPP_
#define HACKS_RADAR_HPP_

#include "../resource.hpp"
#include "../common.h"

extern unsigned char _binary_scout_start;
extern unsigned char _binary_scout_blue_start;
extern unsigned char _binary_soldier_start;
extern unsigned char _binary_soldier_blue_start;
extern unsigned char _binary_pyro_start;
extern unsigned char _binary_pyro_blue_start;
extern unsigned char _binary_demoman_start;
extern unsigned char _binary_demoman_blue_start;
extern unsigned char _binary_heavy_start;
extern unsigned char _binary_heavy_blue_start;
extern unsigned char _binary_engineer_start;
extern unsigned char _binary_engineer_blue_start;
extern unsigned char _binary_medic_start;
extern unsigned char _binary_medic_blue_start;
extern unsigned char _binary_sniper_start;
extern unsigned char _binary_sniper_blue_start;
extern unsigned char _binary_spy_start;
extern unsigned char _binary_spy_blue_start;

extern unsigned char _binary_dispenser_start;

namespace hacks { namespace tf { namespace radar {

extern Texture textures[2][9];
extern Texture buildings[1];

extern CatVar size;
extern CatVar zoom;
extern CatVar radar_enabled;
extern CatVar radar_x;
extern CatVar radar_y;


void Init();
std::pair<int, int> WorldToRadar(int x, int y);
void Draw();

}}}

#endif /* HACKS_RADAR_HPP_ */
