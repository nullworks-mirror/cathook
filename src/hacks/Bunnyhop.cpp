/*
 * HBunnyhop.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: nullifiedcat
 */

#include "Bunnyhop.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace shared { namespace bunnyhop {

CatVar enabled(CV_SWITCH, "bhop_enabled", "0", "Bunnyhop", "Enable Bunnyhop. All extra features like autojump and perfect jump limit were temporary removed.");

int iTicksFlying { 0 };
int iTicksLastJump { 0 };

void CreateMove() {
	if (!enabled) return;
	//if (HasCondition(g_pLocalPlayer->entity, TFCond_GrapplingHook)) return;
	int flags = CE_INT(g_pLocalPlayer->entity, netvar.iFlags);

	if (CE_INT(g_pLocalPlayer->entity, netvar.movetype) == MOVETYPE_FLY) return;

	bool ground = (flags & (1 << 0));
	bool jump = (g_pUserCmd->buttons & IN_JUMP);

	if (ground) {
		iTicksFlying = 0;
	} else {
		iTicksFlying++;
	}

	if (!ground && jump) {
		if (iTicksLastJump++ >= 9) g_pUserCmd->buttons = g_pUserCmd->buttons &~ IN_JUMP;
	}
	if (!jump) iTicksLastJump = 0;
	return;
}

}}}
