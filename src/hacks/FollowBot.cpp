/*
 * FollowBot.cpp
 *
 *  Created on: Mar 20, 2017
 *      Author: nullifiedcat
 */

#include "FollowBot.h"

#include "../common.h"

namespace hacks { namespace shared { namespace followbot {

unsigned follow_steamid { 0 };
Vector last_direction;
float lost_time { 0 };
int following_idx { 0 };

CatCommand move_to_crosshair("fb_move_to_point", "Moves a bot (or all bots) to crosshair", [](const CCommand& args) {
	logging::Info("not yet implemented.");
});
CatCommand follow("fb_follow", "Follows you (or player with SteamID specified)", [](const CCommand& args) {
	follow_steamid = strtol(args.Arg(1), nullptr, 10);
});
CatCommand follow_entity("fb_follow_entity", "Follows entity with specified entity ID", [](const CCommand& args) {
	logging::Info("not yet implemented.");
});
CatVar bot(CV_SWITCH, "fb_bot", "0", "This player is a bot", "Set to 1 in followbots' configs");

void WalkTo(const Vector& vector) {
	if (CE_VECTOR(LOCAL_E, netvar.vVelocity).IsZero(1.0f)) {
		if (LOCAL_E->m_vecOrigin.DistTo(vector) > 150.0f) {
			if (!g_pLocalPlayer->bZoomed)
				g_pUserCmd->buttons |= IN_JUMP;
		}
	}
	const Vector& current = LOCAL_E->m_vecOrigin;
	Vector diff = (current - vector);
	float dot = current.Dot(vector);
	float cos = dot / (current.Length() * vector.Length());
	float rad = acos(cos);
	if (diff.y < 0) rad = -rad;
	float cur_rad = DEG2RAD(g_pUserCmd->viewangles.y);
	float rad_diff = cur_rad - rad;
	g_pUserCmd->forwardmove = std::cos(rad_diff) * 450.0f;
	g_pUserCmd->sidemove = sin(rad_diff) * 450.0f;
}

void DoWalking() {
	following_idx = 0;
	for (int i = 1; i < 32 && i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (ent->m_pPlayerInfo->friendsID == follow_steamid) {
			following_idx = i;
			break;
		}
	}
	CachedEntity* found_entity = ENTITY(following_idx);
	if (!found_entity->IsVisible()) {
		if (!lost_time) {
			lost_time = g_GlobalVars->curtime;
		}
		if (g_GlobalVars->curtime - lost_time < 2.0f) {
			WalkTo(last_direction);
		}
	} else {
		lost_time = 0;
		WalkTo(found_entity->m_vecOrigin);
	}
}

}}}
