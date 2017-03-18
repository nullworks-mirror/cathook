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

CatVar stuck(CV_SWITCH, "airstuck", "0", "Airstuck active");

void SendNOP() {
	INetChannel* ch = (INetChannel*)g_IEngine->GetNetChannelInfo();
	NET_NOP packet;
	packet.SetNetChannel(ch);
	packet.SetReliable(false);
	ch->SendNetMsg(packet);
}

void CreateMove() {
	if (stuck) {
		if (g_GlobalVars->tickcount % 60 == 0) {
			SendNOP();
		}
	}
}

bool IsStuck() {
	if (g_pUserCmd->buttons & (IN_ATTACK | IN_ATTACK2)) return false;
	return stuck;
}

void Reset() {
	stuck = false;
}

}}}
