/*
 * chatstack.cpp
 *
 *  Created on: Jan 24, 2017
 *      Author: nullifiedcat
 */

#include "chatstack.h"
#include "common.h"
#include "sdk.h"

namespace chat_stack {

void OnCreateMove() {
	if (last_say > g_GlobalVars->curtime) last_say = 0;
	if (g_GlobalVars->curtime - CHATSTACK_INTERVAL <= last_say) return;
	std::string message;
	if (!stack.empty()) {
		message = stack.top();
		stack.pop();
		if (message.size()) {
			g_IEngine->ServerCmd(format("say \"", message, '"').c_str());
			last_say = g_GlobalVars->curtime;
		}
	}
}

void Reset() {
	while (!stack.empty()) stack.pop();
	last_say = 0.0f;
}

std::stack<std::string> stack;
float last_say = 0.0f;

}
