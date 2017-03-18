/*
 * KillSay.cpp
 *
 *  Created on: Jan 19, 2017
 *      Author: nullifiedcat
 */

#include "KillSay.h"
#include "../common.h"
#include "../sdk.h"
#include <pwd.h>

void KillSayEventListener::FireGameEvent(IGameEvent* event) {
	if (!hacks::shared::killsay::enabled) return;
	std::string message = hacks::shared::killsay::ComposeKillSay(event);
	if (message.size()) {
		chat_stack::stack.push(message);
	}
}

namespace hacks { namespace shared { namespace killsay {

const std::string tf_classes_killsay[] = {
	"class",
	"scout",
	"sniper",
	"soldier",
	"demoman",
	"medic",
	"heavy",
	"pyro",
	"spy",
	"engineer"
};

const std::string tf_teams_killsay[] = {
	"RED",
	"BLU"
};

TextFile file {};

std::string ComposeKillSay(IGameEvent* event) {
	if (file.LineCount() == 0) return 0;
	if (!event) return 0;
	int vid = event->GetInt("userid");
	int kid = event->GetInt("attacker");
	if (kid == vid) return 0;
	if (g_IEngine->GetPlayerForUserID(kid) != g_IEngine->GetLocalPlayer()) return 0;
	std::string msg = file.Line(rand() % file.LineCount());
	player_info_s info;
	g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(vid), &info);
	ReplaceString(msg, "%name%", std::string(info.name));
	CachedEntity* ent = ENTITY(g_IEngine->GetPlayerForUserID(vid));
	int clz = g_pPlayerResource->GetClass(ent);
	ReplaceString(msg, "%class%", tf_classes_killsay[clz]);
	player_info_s infok;
	g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(kid), &infok);
	ReplaceString(msg, "%killer%", std::string(infok.name));
	ReplaceString(msg, "%team%", tf_teams_killsay[ent->m_iTeam - 2]);
	ReplaceString(msg, "%myteam%", tf_teams_killsay[LOCAL_E->m_iTeam - 2]);
	ReplaceString(msg, "%myclass%", tf_classes_killsay[g_pPlayerResource->GetClass(LOCAL_E)]);
	ReplaceString(msg, "\\n", "\n");
	return msg;
}

CatVar enabled(CV_SWITCH, "killsay", "0", "KillSay", "Enable KillSay");
CatVar filename(CV_STRING, "killsay_file", "killsays.txt", "Killsay file (~/.cathook/)", "Killsay file name. Should be located in ~/.cathook folder.");
CatCommand reload("killsay_reload", "Reload killsays", Reload);

KillSayEventListener& getListener() {
	static KillSayEventListener listener;
	return listener;
}

void Reload() {
	file.Load(std::string(filename.GetString()));
}

void Init() {
	g_IEventManager2->AddListener(&getListener(), (const char*)"player_death", false);
}

void Shutdown() {
	g_IEventManager2->RemoveListener(&getListener());
}

}}}
