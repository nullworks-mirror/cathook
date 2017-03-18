/*
 * AutoReflect.cpp
 *
 *  Created on: Nov 18, 2016
 *      Author: nullifiedcat
 */

#include "AutoReflect.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace tf { namespace autoreflect {

CatVar enabled(CV_SWITCH, "reflect_enabled", "0", "AutoReflect", "Master AutoReflect switch");
CatVar idle_only(CV_SWITCH, "reflect_only_idle", "0", "Only when not shooting", "Don't AutoReflect if you're holding M1");
CatVar stickies(CV_SWITCH, "reflect_stickybombs", "0", "Reflect stickies", "Reflect Stickybombs");
CatVar max_distance(CV_INT, "reflect_distance", "200", "Distance", "Maximum distance to reflect at", true, 300.0f);

void CreateMove() {
	if (!enabled) return;
	if (g_pLocalPlayer->weapon()->m_iClassID != g_pClassID->CTFFlameThrower) return;
	if (idle_only && (g_pUserCmd->buttons & IN_ATTACK)) return;

	CachedEntity* closest = 0;
	float closest_dist = 0.0f;
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (!ShouldReflect(ent)) continue;
		//if (ent->Var<Vector>(eoffsets.vVelocity).IsZero(1.0f)) continue;
		float dist = ent->m_vecOrigin.DistToSqr(g_pLocalPlayer->v_Origin);
		if (dist < closest_dist || !closest) {
			closest = ent;
			closest_dist = dist;
		}
	}
	if (CE_BAD(closest)) return;
	if (closest_dist == 0 || closest_dist > SQR((int)max_distance)) return;

	Vector tr = (closest->m_vecOrigin - g_pLocalPlayer->v_Eye);
	Vector angles;
	fVectorAngles(tr, angles);
	fClampAngle(angles);
	g_pUserCmd->viewangles = angles;
	g_pLocalPlayer->bUseSilentAngles = true;
	g_pUserCmd->buttons |= IN_ATTACK2;
	return;
}

bool ShouldReflect(CachedEntity* ent) {
	if (CE_BAD(ent)) return false;
	if (ent->m_Type != ENTITY_PROJECTILE) return false;
	if (CE_INT(ent, netvar.iTeamNum) == g_pLocalPlayer->team) return false;
	// If projectile is already deflected, don't deflect it again.
	if (CE_INT(ent, (ent->m_bGrenadeProjectile ?
			/* NetVar for grenades */ netvar.Grenade_iDeflected :
			/* For rockets */ netvar.Rocket_iDeflected))) return false;
	if (ent->m_iClassID == g_pClassID->CTFGrenadePipebombProjectile) {
		if (CE_INT(ent, netvar.iPipeType) == 1) {
			if (!stickies) return false;
		}
	}
	return true;
}

}}}
