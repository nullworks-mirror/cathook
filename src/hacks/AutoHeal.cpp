/*
 * AutoHeal.cpp
 *
 *  Created on: Dec 3, 2016
 *      Author: nullifiedcat
 */

#include "AutoHeal.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace tf { namespace autoheal {

extern CatVar enabled(CV_SWITCH, "autoheal_enabled", "0", "AutoHeal", "Automatically heals nearby teammates");
extern CatVar silent(CV_SWITCH, "autoheal_silent", "1", "Silent AutoHeal", "Silent AutoHeal. Disable this to make ghetto followbot");
//extern CatVar target_only;

int m_iCurrentHealingTarget { -1 };
int m_iNewTarget { 0 };

void CreateMove() {
	if (!enabled) return;
	if (GetWeaponMode(g_pLocalPlayer->entity) != weapon_medigun) return;
	int old_target = m_iCurrentHealingTarget;
	m_iCurrentHealingTarget = BestTarget();
	if (m_iNewTarget > 0 && m_iNewTarget < 10) m_iNewTarget++;
	else m_iNewTarget = 0;
	bool new_target = (old_target != m_iCurrentHealingTarget);
	if (new_target) {
		m_iNewTarget = 1;
	}
	if (m_iCurrentHealingTarget == -1) return;
	CachedEntity* target = ENTITY(m_iCurrentHealingTarget);
	Vector out;
	GetHitbox(target, 7, out);
	AimAt(g_pLocalPlayer->v_Eye, out, g_pUserCmd);
	if (silent) g_pLocalPlayer->bUseSilentAngles = true;
	if (!m_iNewTarget && (g_GlobalVars->tickcount % 60)) g_pUserCmd->buttons |= IN_ATTACK;
	return;
}

int BestTarget() {
	int best = -1;
	int best_score = -65536;
	for (int i = 0; i < 32 && i < HIGHEST_ENTITY; i++) {
		int score = HealingPriority(i);
		if (score > best_score && score != -1) {
			best = i;
			best_score = score;
		}
	}
	return best;
}

int HealingPriority(int idx) {
	if (!CanHeal(idx)) return -1;
	CachedEntity* ent = ENTITY(idx);
	int priority = 0;
	int health = CE_INT(ent, netvar.iHealth);
	int maxhealth = g_pPlayerResource->GetMaxHealth(ent);
	int maxbuffedhealth = maxhealth * 1.5;
	int maxoverheal = maxbuffedhealth - maxhealth;
	int overheal = maxoverheal - (maxbuffedhealth - health);
	float overhealp = ((float)overheal / (float)maxoverheal);
	float healthp = ((float)health / (float)maxhealth);
	switch (GetRelation(ent)) {
	case relation::FRIEND:
		priority += 70 * (1 - healthp);
		priority += 15 * (1 - overhealp);
		break;
	case relation::BOT:
		priority += 100 * (1 - healthp);
		priority += 15 * (1 - overhealp);
		break;
	default:
		priority += 50 * (1 - healthp);
		priority += 10 * (1 - overhealp);
	}
	if (ipc::peer) {
		if (hacks::shared::followbot::bot && hacks::shared::followbot::following_idx == idx) {
			priority += 75;
		}
	}
	return priority;
}

bool CanHeal(int idx) {
	CachedEntity* ent = ENTITY(idx);
	if (!ent) return false;
	if (CE_BAD(ent)) return false;
	if (ent->m_Type != ENTITY_PLAYER) return false;
	if (g_IEngine->GetLocalPlayer() == idx) return false;
	if (!ent->m_bAlivePlayer) return false;
	if (ent->m_bEnemy) return false;
	if (ent->m_flDistance > 420) return false;
	// TODO visible any hitbox
	if (!IsEntityVisible(ent, 7)) return false;
	if (IsPlayerInvisible(ent)) return false;
	return true;
}

}}}
