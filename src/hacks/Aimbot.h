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

// Variable used to tell when the aimbot has found a target
extern bool foundTarget;

const Vector& PredictEntity(CachedEntity* entity);
bool VischeckPredictedEntity(CachedEntity* entity);

// Functions called by other functions for when certian game calls are run
void CreateMove();
void DrawText();
void Reset();

// Used by esp to set their color
extern int target_eid;

float EffectiveTargetingRange();


CachedEntity* CurrentTarget();
bool ShouldAim();
CachedEntity* RetrieveBestTarget();
bool IsTargetStateGood(CachedEntity* entity);
void Aim(CachedEntity* entity);
bool CanAutoShoot();
int BestHitbox(CachedEntity* target);
int ClosestHitbox(CachedEntity* target);
void slowAim(Vector &inputAngle, Vector userAngle);
bool UpdateAimkey();
bool GetCanAim(int mode);

}}}

#endif /* HAIMBOT_H_ */

