/*
 * chatstack.cpp
 *
 *  Created on: Jan 24, 2017
 *      Author: nullifiedcat
 */

#include "chatstack.h"
#include "common.h"
#include "sdk.h"

ChatStack::ChatStack() {
	m_Buffer = new char[256 * CHATSTACK_SIZE];
	m_nStackDepth = 0;
}

ChatStack::~ChatStack() {
	delete [] m_Buffer;
}

void ChatStack::OnCreateMove() {
	if (m_nStackDepth <= 0) return;
	if (m_fLastSay > g_GlobalVars->curtime) m_fLastSay = 0;
	if (g_GlobalVars->curtime - CHATSTACK_INTERVAL <= m_fLastSay) return;
	const char* msg = Pop();
	const char* cmd = strfmt("say \"%s\"", msg);
	g_IEngine->ServerCmd(cmd);
	m_fLastSay = g_GlobalVars->curtime;
	delete [] cmd;
}

void ChatStack::Push(const char* message) {
	if (m_nStackDepth >= CHATSTACK_SIZE) return;
	strncpy(&m_Buffer[m_nStackDepth++ * 256], message, 255);
}

const char* ChatStack::Pop() {
	if (m_nStackDepth <= 0) return 0;
	return (const char*)&m_Buffer[(--m_nStackDepth) * 256];
}


void ChatStack::Reset() {
	m_nStackDepth = 0;
}

ChatStack* g_pChatStack = 0;
