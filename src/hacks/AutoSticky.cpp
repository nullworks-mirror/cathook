/*
 * AutoSticky.cpp
 *
 *  Created on: Dec 2, 2016
 *      Author: nullifiedcat
 */

#include "AutoSticky.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace tf { namespace autosticky {

CatVar enabled(CV_SWITCH, "sticky_enabled", "0", "AutoSticky", "Master AutoSticky switch");
CatVar buildings(CV_SWITCH, "sticky_buildings", "1", "Detonate buildings", "Stickies react to buildings");
CatVar distance(CV_INT, "sticky_distance", "200", "Distance", "Maximum distance to detonate");

bool ShouldDetonate(CachedEntity* bomb) {
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (ent->m_Type != ENTITY_PLAYER && (ent->m_Type != ENTITY_BUILDING || !buildings)) continue;
		if (ent->m_iTeam == CE_INT(bomb, netvar.iTeamNum)) continue;
		if (ent->m_Type == ENTITY_PLAYER) {
			if (CE_BYTE(ent, netvar.iLifeState) != LIFE_ALIVE) continue;
		}
		if (ent->m_vecOrigin.DistToSqr(bomb->m_vecOrigin) > SQR((float)distance)) continue;
		return true;
	}
	return false;
}

void CreateMove() {
	if (!enabled) return;
	if (g_pLocalPlayer->clazz != tf_demoman) return;
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (ent->m_iClassID != g_pClassID->CTFGrenadePipebombProjectile) continue;
		if (CE_INT(ent, netvar.iPipeType) != 1) continue;
		if ((CE_INT(ent, netvar.hThrower) & 0xFFF) != g_pLocalPlayer->entity->m_IDX) continue;
		if (ShouldDetonate(ent)) {
			g_pUserCmd->buttons |= IN_ATTACK2;
		}
	}
	return;
}

}}}
