/*
 * HAimbot.h
 *
 *  Created on: Oct 8, 2016
 *      Author: nullifiedcat
 */

#ifndef HAIMBOT_H_
#define HAIMBOT_H_

#include "IHack.h"
#include "../enums.h"

class ConVar;
class IClientEntity;
class Vector;

namespace hacks { namespace shared { namespace aimbot {

struct AimKeyMode_t {
	enum {
		DISABLED,
		PRESS_TO_ENABLE,
		PRESS_TO_DISABLE,
		PRESS_TO_TOGGLE
	};
};

void CreateMove();
void Reset();

extern CachedEntity* target_highest;

int ShouldTarget(CachedEntity* entity);
bool Aim(CachedEntity* entity, CUserCmd* cmd);
bool ShouldAim(CUserCmd* cmd);
int BestHitbox(CachedEntity* target);

extern CatVar aimkey;
extern CatVar aimkey_mode;
extern CatVar hitbox_mode;
extern CatVar enabled;
extern CatVar fov;
extern CatVar hitbox;
extern CatVar lerp;
extern CatVar autoshoot;
extern CatVar silent;
extern CatVar zoomed_only;
extern CatVar teammates;
extern CatVar huntsman_autoshoot;
extern CatVar max_range;
//CatVar* v_iMaxAutoshootRange; // TODO IMPLEMENT
extern CatVar respect_cloak;
extern CatVar attack_only;
extern CatVar projectile_aimbot;
extern CatVar proj_speed;
extern CatVar proj_gravity;
extern CatVar buildings;
extern CatVar only_can_shoot;
extern CatVar priority_mode;
extern CatVar proj_visibility;
extern CatVar proj_fov;

}}}

#endif /* HAIMBOT_H_ */
