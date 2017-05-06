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

enum class EAimbotState {
	DISABLED, 	// Aimbot is disabled completely.
	ENABLED,    // Aimbot is enabled, but other checks weren't performed yet
	INACTIVE, 	// Aimkey isn't down, minigun isn't spun up, you are dead, etc.
	ACTIVE,   	// Aimbot is enabled and active, but target isn't found yet.
	TARGET_FOUND, // Target is found, but you aren't aiming yet.
	AIMING 		// Aimbot is messing with your UserCmd
};


enum class EAimKeyMode {
	DISABLED,
	PRESS_TO_ENABLE,
	PRESS_TO_DISABLE,
	PRESS_TO_TOGGLE
};

enum class EAimbotLocalState {
	// Aimbot can activate
	GOOD = 0,
	// Aimbot won't activate
	AIMKEY_RELEASED,
	TAUNTING, // Local player is taunting
	CLOAKED, // Local player is cloaked, you can't shoot
	DEAD_RINGER_OUT, // You can't shoot with DR out
	USE_BUTTON, // You are holding use button
	ACTION_SLOT_ITEM, // You are using cat_use_action_slot_item_server (Grappling Hook)
	CARRYING_BUILDING, // You are carrying a building
	NOT_ZOOMED, // Zoomed only is enabled and player isn't zoomed in (ignored if attacking)
	CANT_SHOOT,
	MINIGUN_IDLE, // Minigun is idle
	MINIGUN_BUTTON_RELEASED, // Mouse not pressed
	AMBASSADOR_COOLDOWN,
	SNIPER_RIFLE_DELAY,
	CRIT_HACK_LOCKS_ATTACK,
	DISABLED_FOR_THIS_WEAPON, // You are holding Jarate/PDA/something like that
	NOT_ATTACKING // Attack Only is enabled, but mouse1 isn't held
};

enum class EAimbotTargetState {
	// Target is valid, aimbot can aim at it
	GOOD = 0,
	// Aimbot won't aim at the target
	LOCAL, // Aimbot shouldn't aim at local player
	TAUNTING,
	FRIENDLY, // Managed by playerlist
	HOOVY,
	NOT_ENOUGH_CHARGE,
	INVULNERABLE,
	INVISIBLE,
	VACCINATED,
	DEAD,
	TEAMMATE,
	OUT_OF_RANGE,
	HITBOX_ERROR,
	VCHECK_FAILED,
	FOV_CHECK_FAILED,
	BUILDING_AIMBOT_DISABLED,
	INVALID_ENTITY,
	IMPOSSIBLE_ERROR
};

struct AimbotCalculatedData_s {
	unsigned long predict_tick { 0.0f };
	Vector aim_position { 0 };
	unsigned long vcheck_tick { 0 };
	bool visible { false };
	float fov { 0 };
	int hitbox { 0 };
};

const Vector& PredictEntity(CachedEntity* entity);
bool VischeckPredictedEntity(CachedEntity* entity);

void CreateMove();
void Reset();

extern EAimbotState state;
extern int target_eid;

float EffectiveTargetingRange();
float EffectiveShootingRange();

bool UpdateAimkey();
CachedEntity* CurrentTarget();
EAimbotTargetState TargetState(CachedEntity* entity);
bool Aim(CachedEntity* entity);
EAimbotLocalState ShouldAim();
int BestHitbox(CachedEntity* target);

}}}

#endif /* HAIMBOT_H_ */
