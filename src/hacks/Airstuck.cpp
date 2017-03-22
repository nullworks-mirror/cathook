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

void SendNOP() {
	INetChannel* ch = (INetChannel*)g_IEngine->GetNetChannelInfo();
	NET_NOP packet;
	packet.SetNetChannel(ch);
	packet.SetReliable(false);
	ch->SendNetMsg(packet);
}

void CreateMove() {
	if (g_IInputSystem->IsButtonDown((ButtonCode_t)int(stuck))) {
		if (g_GlobalVars->tickcount % 60 == 0) {
			SendNOP();
		}
	}
}

bool IsStuck() {
	if (g_pUserCmd->buttons & (IN_ATTACK | IN_ATTACK2)) return false;
	return g_IInputSystem->IsButtonDown((ButtonCode_t)int(stuck));
}

void Reset() {

}

}}}
