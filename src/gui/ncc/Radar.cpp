/*
 * Radar.cpp
 *
 *  Created on: Mar 28, 2017
 *      Author: nullifiedcat
 */

#include "Radar.hpp"
#include "../../common.h"

namespace menu { namespace ncc {

Radar::Radar() : CBaseWidget("ncc_radar") {}

std::pair<int, int> Radar::GetSize() {
	return { (int)hacks::tf::radar::size, (int)hacks::tf::radar::size };
}

void Radar::Update() {
	if (!hacks::tf::radar::radar_enabled) Hide();
	else Show();
	SetOffset((int)hacks::tf::radar::radar_x, (int)hacks::tf::radar::radar_y);
	if (IsPressed()) {
		hacks::tf::radar::radar_x = (int)hacks::tf::radar::radar_x + g_pGUI->mouse_dx;
		hacks::tf::radar::radar_y = (int)hacks::tf::radar::radar_y + g_pGUI->mouse_dy;
	}
}

}}
