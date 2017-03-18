/*
 * SpyAlert.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */

#include "SpyAlert.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace tf { namespace spyalert {

CatVar enabled(CV_SWITCH, "spyalert_enabled", "0", "Enable", "Master SpyAlert switch");
CatVar distance_warning(CV_FLOAT, "spyalert_warning", "500.0", "Warning distance", "Distance where yellow warning shows");
CatVar distance_backstab(CV_FLOAT, "spyalert_backstab", "200.0", "Backstab distance", "Distance where red warning shows");

void Draw() {
	if (!enabled) return;
	for (int i = 0; i < HIGHEST_ENTITY && i < 64; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (CE_BYTE(ent, netvar.iLifeState)) continue;
		if (CE_INT(ent, netvar.iClass) != tf_class::tf_spy) continue;
		if (CE_INT(ent, netvar.iTeamNum) == g_pLocalPlayer->team) continue;
		float distance = ent->m_flDistance;
		if (distance < (float)distance_backstab) {
			AddCenterString(format("BACKSTAB WARNING! ", (int)(distance / 64 * 1.22f), 'm'), colors::red);
		} else if (distance < (float)distance_warning) {
			AddCenterString(format("Incoming spy! ", (int)(distance / 64 * 1.22f), 'm'), colors::yellow);
		}
	}
}

}}}
