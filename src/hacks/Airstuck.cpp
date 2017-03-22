/*
 * Airstuck.cpp
 *
 *  Created on: Nov 26, 2016
 *      Author: nullifiedcat
 */

#include "Airstuck.h"

#include "../common.h"
#include "../sdk.h"

#include "../netmessage.h"

namespace hacks { namespace shared { namespace airstuck {

CatVar stuck(CV_KEY, "airstuck", "0", "Airstuck key");
CatVar stuck_toggle(CV_SWITCH, "airstuck_toggle", "0", "Airstuck toggle", "Use this in bot instances/when using phlog to block cart!");

void SendNOP() {
	INetChannel* ch = (INetChannel*)g_IEngine->GetNetChannelInfo();
	NET_NOP packet;
	packet.SetNetChannel(ch);
	packet.SetReliable(false);
	ch->SendNetMsg(packet);
}

void CreateMove() {
	if (IsStuck()) {
		if (g_GlobalVars->tickcount % 60 == 0) {
			SendNOP();
		}
	}
}

bool IsStuck() {
	if (g_Settings.bInvalid || (g_pUserCmd->buttons & (IN_ATTACK | IN_ATTACK2))) return false;
	return (g_IInputSystem->IsButtonDown((ButtonCode_t)int(stuck)) || stuck_toggle);
}

void Reset() {
	stuck_toggle = false;
}

}}}
