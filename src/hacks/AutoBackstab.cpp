/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace tf2 { namespace autobackstab {

static CatVar enabled(CV_SWITCH, "autobackstab", "0", "Auto Backstab", "Does not depend on triggerbot!");

/*float GetAngle(CachedEntity* target) {
	const Vector& A = target->m_vecOrigin;
	const Vector& B = LOCAL_E->m_vecOrigin;
	const float yaw = CE_FLOAT(target, netvar.m_angEyeAngles + 4);
	const Vector diff = (A - B);
	float yaw2 = acos(diff.x / diff.Length()) * 180.0f / PI;
	if (diff.y < 0) yaw2 = -yaw2;
	float anglediff = yaw - yaw2;
	if (anglediff > 180) anglediff -= 360;
	if (anglediff < -180) anglediff += 360;
	//logging::Info("Angle: %.2f | %.2f | %.2f | %.2f", yaw, yaw2, anglediff, yaw - yaw2);
	return anglediff;
}*/

// TODO improve
void CreateMove() {
	// lmao thanks valve
	if (!enabled) return;
	if (g_pLocalPlayer->weapon()->m_iClassID != g_pClassID->CTFKnife) return;
	if (CE_BYTE(g_pLocalPlayer->weapon(), netvar.m_bReadyToBackstab)) {
		g_pUserCmd->buttons |= IN_ATTACK;
		return;
	}

	/*if (g_pLocalPlayer->weapon()->m_iClassID != g_pClassID->CTFKnife) return;
	for (int i = 1; i < g_IEntityList->GetHighestEntityIndex() && i < 32; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (!ent->m_bEnemy) continue;
		if (CE_BYTE(ent, netvar.iLifeState) != LIFE_ALIVE) continue;
		if (IsPlayerInvulnerable(ent)) continue;
		if (ent->m_flDistance > 75) continue;
		if (fabs(GetAngle(ent)) < 100) g_pUserCmd->buttons |= IN_ATTACK;
	}*/
}

}}}
