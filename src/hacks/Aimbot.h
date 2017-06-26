/*
 * HAimbot.h
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#ifndef HAIMBOT_H_
#define HAIMBOT_H_

#include "../common.h"

class ConVar;
class IClientEntity;

namespace hacks { namespace shared { namespace aimbot {

enum class EAimKeyMode {
	DISABLED,
	PRESS_TO_ENABLE,
	PRESS_TO_DISABLE,
	PRESS_TO_TOGGLE
};

struct AimbotCalculatedData_s {
	unsigned long predict_tick { 0 };
	Vector aim_position { 0 };
	unsigned long vcheck_tick { 0 };
	bool visible { false };
	float fov { 0 };
	int hitbox { 0 };
};

	
extern bool foundTarget;

const Vector& PredictEntity(CachedEntity* entity);
bool VischeckPredictedEntity(CachedEntity* entity);

void CreateMove();
void PaintTraverse();
void Reset();

extern int target_eid;

float EffectiveTargetingRange();

bool UpdateAimkey();
CachedEntity* CurrentTarget();
bool IsTargetStateGood(CachedEntity* entity);
void Aim(CachedEntity* entity);
bool ShouldAim();
bool CanAutoShoot();
void slowAim(Vector &inputAngle, Vector userAngle);
int BestHitbox(CachedEntity* target);
int ClosestHitbox(CachedEntity* target);

}}}

#endif /* HAIMBOT_H_ */

