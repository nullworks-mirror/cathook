/*
 * antiaim.cpp
 *
 *  Created on: Jun 5, 2017
 *      Author: nullifiedcat
 */

#include "antiaim.hpp"
#include "../../common.h"

namespace ac { namespace antiaim {

unsigned long last_accusation[32] { 0 };

void ResetEverything() {
	memset(last_accusation, 0, sizeof(unsigned long) * 32);
}

void ResetPlayer(int idx) {
	last_accusation[idx - 1] = 0;
}

void Init() {
	ResetEverything();
}

void Update(CachedEntity* player) {
	if (tickcount - last_accusation[player->m_IDX - 1] < 60 * 60) return;
	const auto& d = angles::data(player);
	if (d.angle_count) {
		int idx = d.angle_index - 1;
		if (idx < 0) idx = d.count - 1;
		if ((d.angles[idx].x < -89 || d.angles[idx].x > 89) && (d.angles[idx].x < 89.2941 && d.angles[idx].x > 89.2942)) {
			hacks::shared::anticheat::Accuse(player->m_IDX, "AntiAim", format("Pitch = ", d.angles[idx].x));
			last_accusation[player->m_IDX - 1] = tickcount;
		}
	}
}

void Event(KeyValues* event) {

}

}}

