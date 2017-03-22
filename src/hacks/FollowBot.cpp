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
float idle_time { 0 };
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

// I've spent 2 days on writing this method.
// I couldn't succeed.
// Just removed everything and put movement fix code from createmove here.
std::pair<float, float> ComputeMove(const Vector& a, const Vector& b) {
	Vector diff = (b - a);
	if (diff.Length() == 0) return { 0, 0 };
	const float x = diff.x;
	const float y = diff.y;
	Vector vsilent(x, y, 0);
	float speed = sqrt(vsilent.x * vsilent.x + vsilent.y * vsilent.y);
	Vector ang;
	VectorAngles(vsilent, ang);
	float yaw = DEG2RAD(ang.y - g_pUserCmd->viewangles.y);
	return { cos(yaw) * 450, -sin(yaw) * 450 };
}

// I've removed that too early.
void PrintDebug() {

	const Vector& a = LOCAL_E->m_vecOrigin;
	const Vector& b = last_direction;

	Vector diff = (b - a);
	if (diff.Length() == 0) return;
	AddSideString(format("dx: ", diff.x));
	AddSideString(format("dy: ", diff.y));
	//float v_cos = diff.x / diff.Length();
	//float rad = acos(v_cos);
	//if (diff.y < 0) rad = 2 * PI - rad;
	//AddSideString(format("angle: ", rad / PI, " PI"));
	float yan = g_Settings.last_angles.y;
	float yaw = DEG2RAD(yan);
	AddSideString(format("yaw:  ", yaw / PI, " PI"));
	//float rad_diff = yaw - rad;
	//AddSideString(format("diff: ", rad_diff / PI, " PI"));

	auto move = ComputeMove(a, b);
	AddSideString(format("forward: ", move.first));
	AddSideString(format("side:    ", move.second));
}

void WalkTo(const Vector& vector) {
	if (CE_VECTOR(LOCAL_E, netvar.vVelocity).IsZero(1.0f)) {
		if (!idle_time) idle_time = g_GlobalVars->curtime;
		if (LOCAL_E->m_vecOrigin.DistTo(vector) > 200.0f) {
			if (LOCAL_E->m_vecOrigin.DistTo(vector) > 500.0f) {
				if (g_pLocalPlayer->bZoomed) g_pUserCmd->buttons |= IN_ATTACK2;
			}
			if (g_GlobalVars->curtime - idle_time > 2.0f) {
				if (!g_pLocalPlayer->bZoomed)
					g_pUserCmd->buttons |= IN_JUMP;
			}
		} else {
			idle_time = 0;
		}
	}
	auto result = ComputeMove(LOCAL_E->m_vecOrigin, last_direction);

	g_pUserCmd->forwardmove = result.first;
	g_pUserCmd->sidemove = result.second;
}

void DoWalking() {
	if (!bot) return;
	following_idx = 0;
	for (int i = 1; i < 32 && i < HIGHEST_ENTITY; i++) {
		CachedEntity* ent = ENTITY(i);
		if (CE_BAD(ent)) continue;
		if (!ent->m_pPlayerInfo) continue;
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
		if (found_entity->m_vecOrigin.DistTo(LOCAL_E->m_vecOrigin) > 200.0f) WalkTo(found_entity->m_vecOrigin);
		last_direction = found_entity->m_vecOrigin;
		if (HasCondition(found_entity, TFCond_Zoomed)) {
			if (!g_pLocalPlayer->bZoomed) g_pUserCmd->buttons |= IN_ATTACK2;
		}
	}
}

}}}
