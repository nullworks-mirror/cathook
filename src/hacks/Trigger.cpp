/*
 * HTrigger.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#include "../beforecheaders.h"
#include <memory> // std::unique_ptr
#include "../aftercheaders.h"

#include "Trigger.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace shared { namespace triggerbot {

CatVar enabled(CV_SWITCH, "trigger_enabled", "0", "Enable", "Master Triggerbot switch");
CatVar respect_cloak(CV_SWITCH, "trigger_respect_cloak", "1", "Respect cloak", "Don't shoot at cloaked spies");
CatVar zoomed_only(CV_SWITCH, "trigger_zoomed", "1", "Zoomed only", "Don't shoot if you aren't zoomed in");
CatEnum hitbox_enum({
	"ANY", "HEAD", "PELVIS", "SPINE 0", "SPINE 1", "SPINE 2", "SPINE 3", "UPPER ARM L", "LOWER ARM L",
	"HAND L", "UPPER ARM R", "LOWER ARM R", "HAND R", "HIP L", "KNEE L", "FOOT L", "HIP R",
	"KNEE R", "FOOT R"
}, -1);
CatVar hitbox(hitbox_enum, "trigger_hitbox", "-1", "Hitbox", "Triggerbot hitbox. Only useful settings are ANY and HEAD. Use ANY for scatter or any other shotgun-based weapon, HEAD for ambassador/sniper rifle");
CatVar allow_bodyshot(CV_SWITCH, "trigger_bodyshot", "1", "Bodyshot", "Triggerbot will bodyshot enemies if you have enough charge to 1tap them");
CatVar finishing_hit(CV_SWITCH, "trigger_finish", "1", "Noscope weak enemies", "If enemy has <50 HP, noscope them");
CatVar max_range(CV_INT, "trigger_range", "0", "Max range", "Triggerbot won't shoot if enemy is too far away", true, 4096.0f);
CatVar buildings(CV_SWITCH, "trigger_buildings", "1", "Trigger at buildings", "Shoot buildings");
CatVar ignore_vaccinator(CV_SWITCH, "trigger_respect_vaccinator", "1", "Respect vaccinator", "Don't shoot at bullet-vaccinated enemies");
CatVar ambassador(CV_SWITCH, "trigger_ambassador", "1", "Smart Ambassador", "Don't shoot if your ambassador can't headshot yet (Keep that enabled!)");
CatVar accuracy(CV_INT, "trigger_accuracy", "0", "Improve accuracy", "Improves triggerbot accuracy when aiming for specific hitbox");

trace_t trace_object;

// TEMPORARY CODE.
// TODO

bool GetIntersection(float fDst1, float fDst2, Vector P1, Vector P2, Vector& Hit) {
    if ((fDst1 * fDst2) >= 0.0f) return false;
    if (fDst1 == fDst2) return false;
    Hit = P1 + (P2 - P1) * (-fDst1 / (fDst2 - fDst1));
    return true;
}

bool InBox(Vector Hit, Vector B1, Vector B2, int Axis) {
    if (Axis == 1 && Hit.z > B1.z && Hit.z < B2.z && Hit.y > B1.y && Hit.y < B2.y) return true;
    if (Axis == 2 && Hit.z > B1.z && Hit.z < B2.z && Hit.x > B1.x && Hit.x < B2.x) return true;
    if (Axis == 3 && Hit.x > B1.x && Hit.x < B2.x && Hit.y > B1.y && Hit.y < B2.y) return true;
    return false;
}

bool CheckLineBox(Vector B1, Vector B2, Vector L1, Vector L2, Vector& Hit) {
    if (L2.x < B1.x && L1.x < B1.x) return false;
    if (L2.x > B2.x && L1.x > B2.x) return false;
    if (L2.y < B1.y && L1.y < B1.y) return false;
    if (L2.y > B2.y && L1.y > B2.y) return false;
    if (L2.z < B1.z && L1.z < B1.z) return false;
    if (L2.z > B2.z && L1.z > B2.z) return false;
    if (L1.x > B1.x && L1.x < B2.x &&
        L1.y > B1.y && L1.y < B2.y &&
        L1.z > B1.z && L1.z < B2.z)
    {
        Hit = L1;
        return true;
    }
    if ((GetIntersection(L1.x - B1.x, L2.x - B1.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
      || (GetIntersection(L1.y - B1.y, L2.y - B1.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
      || (GetIntersection(L1.z - B1.z, L2.z - B1.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3))
      || (GetIntersection(L1.x - B2.x, L2.x - B2.x, L1, L2, Hit) && InBox(Hit, B1, B2, 1))
      || (GetIntersection(L1.y - B2.y, L2.y - B2.y, L1, L2, Hit) && InBox(Hit, B1, B2, 2))
      || (GetIntersection(L1.z - B2.z, L2.z - B2.z, L1, L2, Hit) && InBox(Hit, B1, B2, 3)))
        return true;

    return false;
}


void CreateMove() {
	if (!enabled) return;
	if (GetWeaponMode() != weapon_hitscan) return;
	if (ambassador) {
		if (IsAmbassador(g_pLocalPlayer->weapon())) {
			if ((g_GlobalVars->curtime - CE_FLOAT(g_pLocalPlayer->weapon(), netvar.flLastFireTime)) <= 1.0) {
				return;
			}
		}
	}
	Ray_t ray;
	trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	Vector forward;
	float sp, sy, cp, cy;
	sy = sinf(DEG2RAD(g_pUserCmd->viewangles[1])); // yaw
	cy = cosf(DEG2RAD(g_pUserCmd->viewangles[1]));

	sp = sinf(DEG2RAD(g_pUserCmd->viewangles[0])); // pitch
	cp = cosf(DEG2RAD(g_pUserCmd->viewangles[0]));

	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
	forward = forward * 8192.0f + g_pLocalPlayer->v_Eye;
	ray.Init(g_pLocalPlayer->v_Eye, forward);
	g_ITrace->TraceRay(ray, 0x4200400B, &trace::filter_default, &trace_object);

	IClientEntity* raw_entity = (IClientEntity*)(trace_object.m_pEnt);
	if (!raw_entity) return;
	CachedEntity* entity = ENTITY(raw_entity->entindex());
	if (!entity->m_bEnemy) return;

	bool isPlayer = false;
	switch (entity->m_Type) {
	case EntityType::ENTITY_PLAYER:
		isPlayer = true; break;
	case EntityType::ENTITY_BUILDING:
		if (!buildings) return;
		break;
	default:
		return;
	};

	Vector enemy_pos = entity->m_vecOrigin;
	Vector my_pos = g_pLocalPlayer->entity->m_vecOrigin;
	if (max_range) {
		if (entity->m_flDistance > (float)max_range) return;
	}
	if (!isPlayer) {
		g_pUserCmd->buttons |= IN_ATTACK;
		return;
	}
	if (HasCondition<TFCond_UberBulletResist>(entity) && ignore_vaccinator) return;
	if (playerlist::IsFriendly(playerlist::AccessData(entity).state)) return;
	if (IsPlayerInvulnerable(entity)) return;
	if (respect_cloak && (IsPlayerInvisible(entity))) return;
	int health = CE_INT(entity, netvar.iHealth);
	bool do_bodyshot = false;
	if (g_pLocalPlayer->clazz == tf_class::tf_sniper) {
		// If sniper..
		if (health <= 50 && finishing_hit) {
			do_bodyshot = true;
		}
		// If we need charge...
		if (!allow_bodyshot) {
			float bdmg = CE_FLOAT(g_pLocalPlayer->weapon(), netvar.flChargedDamage);
			if (CanHeadshot() && (bdmg) >= health) {
				do_bodyshot = true;
			}
		}

	}
	if (!do_bodyshot && (g_pLocalPlayer->clazz == tf_class::tf_sniper) && zoomed_only &&
		!((g_pLocalPlayer->bZoomed) && CanHeadshot())) {
		return;
	}

	CachedHitbox* hb = entity->hitboxes.GetHitbox(trace_object.hitbox);

	if ((int)hitbox >= 0 && !do_bodyshot) {
		if (trace_object.hitbox != (int)hitbox) return;
	}

	if (accuracy && hb) {
		Vector minz(min(hb->min.x, hb->max.x), min(hb->min.y, hb->max.y), min(hb->min.z, hb->max.z));
		Vector maxz(max(hb->min.x, hb->max.x), max(hb->min.y, hb->max.y), max(hb->min.z, hb->max.z));
		Vector size = maxz - minz;
		Vector smod = size * 0.05f * (int)accuracy;
		minz += smod;
		maxz -= smod;
		Vector hit;
		if (CheckLineBox(minz, maxz, g_pLocalPlayer->v_Eye, forward, hit)) {
			g_pUserCmd->buttons |= IN_ATTACK;
		}
	} else {
		g_pUserCmd->buttons |= IN_ATTACK;
	}
}

void Draw() {

}

}}}
