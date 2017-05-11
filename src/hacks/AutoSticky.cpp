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
CatVar distance(CV_INT, "sticky_distance", "150", "Distance", "Maximum distance to detonate");
CatVar visCheck(CV_SWITCH, "sticky_visable", "1", "Vis check", "Simple check to see if the sticky will hit someone");
//CatVar legit(CV_SWITCH, "sticky_legit", "0", "Legit", "Stickys only detonate when you see them");

std::vector<CachedEntity*> bombs;
std::vector<CachedEntity*> targets;

bool IsBomb(CachedEntity* ent) {
	if (ent->m_iClassID != g_pClassID->CTFGrenadePipebombProjectile) return false;
	if (CE_INT(ent, netvar.iPipeType) != 1) return false;
	if ((CE_INT(ent, netvar.hThrower) & 0xFFF) != g_pLocalPlayer->entity->m_IDX) return false;
	return true;
}

bool IsTarget(CachedEntity* ent) {
	if (!ent->m_bEnemy) return false;
	if (ent->m_Type == ENTITY_PLAYER) {
		if (CE_BYTE(ent, netvar.iLifeState)) return false;
		return true;
	} else if (ent->m_Type == ENTITY_BUILDING) {
		return buildings;
	}
	return false;
}

bool stickyVisable(CachedEntity* targetTrace, CachedEntity* bombTrace) {
    static trace_t trace;
    trace::filter_default.SetSelf(RAW_ENT(bombTrace));
    Ray_t ray;
    ray.Init(bombTrace->m_vecOrigin, RAW_ENT(targetTrace)->GetCollideable()->GetCollisionOrigin());
    g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_default, &trace);
    //deboog1 = bombTrace.DistToSqr(trace->endpos);
    if (trace.m_pEnt) {
        if (((IClientEntity*)(trace.m_pEnt))->entindex() == targetTrace->m_IDX) return true;
	}
    return false;
}

void CreateMove() {
	if (!enabled) return;
	if (g_pLocalPlayer->clazz != tf_demoman) return;
	bombs.clear();
	targets.clear();
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (IsBomb(ent)) {
			bombs.push_back(ent);
		} else if (IsTarget(ent)) {
			targets.push_back(ent);
		}
	}
	for (auto bomb : bombs) {
		for (auto target : targets) {
			if (bomb->m_vecOrigin.DistToSqr(target->m_vecOrigin) < ((float)distance * (float)distance)) {
                if (visCheck) {
                    if (stickyVisable(target, bomb)) {
                        logging::Info("Det"); 
                        g_pUserCmd->buttons |= IN_ATTACK2;
                        /*if (legit) {
                            if (stickyVisable(g_pLocalPlayer->entity, bomb)) g_pUserCmd->buttons |= IN_ATTACK2;
                        } else g_pUserCmd->buttons |= IN_ATTACK2;
                        */
                    }
                } else g_pUserCmd->buttons |= IN_ATTACK2;

            return;
			}
		}
	}
	return;
}

}}}
