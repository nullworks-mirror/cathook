/*
 * others.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"
#include "../netmessage.h"
#include "../hack.h"
#include "hookedmethods.h"

bool CanPacket_hook(void* thisptr) {
	SEGV_BEGIN;
	return g_Settings.bSendPackets->GetBool() && ((CanPacket_t*)hooks::hkNetChannel->GetMethod(hooks::offCanPacket))(thisptr);
	SEGV_END;
	return false;
}

int IN_KeyEvent_hook(void* thisptr, int eventcode, int keynum, const char* pszCurrentBinding) {
	SEGV_BEGIN;
	if (g_pGUI->ConsumesKey((ButtonCode_t)keynum)) {
		return 0;
	}
	return ((IN_KeyEvent_t*)hooks::hkClient->GetMethod(hooks::offKeyEvent))(thisptr, eventcode, keynum, pszCurrentBinding);
	SEGV_END;
	return 0;
}

bool SendNetMsg_hook(void* thisptr, INetMessage& msg, bool bForceReliable = false, bool bVoice = false) {
	SEGV_BEGIN;

	//logging::Info("Sending NetMsg! %i", msg.GetType());
	if (hacks::shared::airstuck::IsStuck() && cathook && !g_Settings.bInvalid) {
		switch (msg.GetType()) {
		case net_NOP:
		case net_SignonState:
		case net_StringCmd:
			break;
		default:
			return false;
		}
	}
	return ((SendNetMsg_t*)hooks::hkNetChannel->GetMethod(hooks::offSendNetMsg))(thisptr, msg, bForceReliable, bVoice);
	SEGV_END;
	return false;
}

CatVar disconnect_reason(CV_STRING, "disconnect_reason", "", "Disconnect reason", "A custom disconnect reason");

void Shutdown_hook(void* thisptr, const char* reason) {
	SEGV_BEGIN;
	if (cathook && (disconnect_reason.convar_parent->m_StringLength > 3) && strstr(reason, "user")) {
		((Shutdown_t*)hooks::hkNetChannel->GetMethod(hooks::offShutdown))(thisptr, disconnect_reason.GetString());
	} else {
		((Shutdown_t*)hooks::hkNetChannel->GetMethod(hooks::offShutdown))(thisptr, reason);
	}
	SEGV_END;
}

void FrameStageNotify_hook(void* thisptr, int stage) {
	SEGV_BEGIN;
	//logging::Info("FrameStageNotify %i", stage);
	// Ambassador to festive ambassador changer. simple.
	if (!g_IEngine->IsInGame()) g_Settings.bInvalid = true;
	//logging::Info("fsi begin");// TODO dbg
	SVDBG("FSN %i", __LINE__);
	// TODO hack FSN hook
	if (TF && cathook && !g_Settings.bInvalid && stage == FRAME_RENDER_START) {
		SVDBG("FSN %i", __LINE__);
		if (g_Settings.bThirdperson->GetBool() && !g_pLocalPlayer->life_state && CE_GOOD(g_pLocalPlayer->entity)) {
			SVDBG("FSN %i", __LINE__);
			CE_INT(g_pLocalPlayer->entity, netvar.nForceTauntCam) = 1;
		}
		SVDBG("FSN %i", __LINE__);
		if (stage == 5 && g_Settings.bShowAntiAim->GetBool() && g_IInput->CAM_IsThirdPerson()) {
			SVDBG("FSN %i", __LINE__);
			if (CE_GOOD(g_pLocalPlayer->entity)) {
				CE_FLOAT(g_pLocalPlayer->entity, netvar.deadflag + 4) = g_Settings.last_angles.x;
				CE_FLOAT(g_pLocalPlayer->entity, netvar.deadflag + 8) = g_Settings.last_angles.y;
			}
			SVDBG("FSN %i", __LINE__);
		}
	}
	SVDBG("FSN %i", __LINE__);
	SAFE_CALL(((FrameStageNotify_t*)hooks::hkClient->GetMethod(hooks::offFrameStageNotify))(thisptr, stage));
	SEGV_END;
}

CatVar override_fov_zoomed(CV_FLOAT, "fov_zoomed", "0", "FOV override (zoomed)", "Overrides FOV with this value when zoomed in (default FOV when zoomed is 20)");
CatVar override_fov(CV_FLOAT, "fov", "0", "FOV override", "Overrides FOV with this value");

void OverrideView_hook(void* thisptr, CViewSetup* setup) {
	SEGV_BEGIN;
	((OverrideView_t*)hooks::hkClientMode->GetMethod(hooks::offOverrideView))(thisptr, setup);
	if (!cathook) return;
	bool zoomed = g_pLocalPlayer->bZoomed;
	if (zoomed && override_fov_zoomed) {
		setup->fov = override_fov_zoomed;
	} else {
		if (override_fov) {
			setup->fov = override_fov;
		}
	}
	SEGV_END;
}

bool DispatchUserMessage_hook(void* thisptr, int type, bf_read& buf) {
	SEGV_BEGIN;
	if (g_phMisc->v_bCleanChat->GetBool()) {
		if (type == 4) {
			int s = buf.GetNumBytesLeft();
			char* data = new char[s];
			for (int i = 0; i < s; i++)
				data[i] = buf.ReadByte();
			int j = 0;
			for (int i = 0; i < 3; i++) {
				while (char c = data[j++]) {
					if (c == '\n' && (i == 1 || i == 2)) data[j - 1] = ' ';
				}
			}
			buf = bf_read(data, s);
			buf.Seek(0);
		}
	}
	return ((DispatchUserMessage_t*)hooks::hkClient->GetMethod(hooks::offFrameStageNotify + 1))(thisptr, type, buf);
	SEGV_END; return false;
}

void LevelInit_hook(void* thisptr, const char* newmap) {
	((LevelInit_t*) hooks::hkClientMode->GetMethod(hooks::offLevelInit))(thisptr, newmap);
	g_IEngine->ExecuteClientCmd("exec cat_matchexec");
	hacks::shared::aimbot::Reset();
	hacks::shared::airstuck::Reset();
	LEVEL_INIT(Bunnyhop);
	LEVEL_INIT(ESP);
//	LEVEL_SHUTDOWN(FollowBot);
	LEVEL_INIT(Misc);
	//if (TF) LEVEL_INIT(SpyAlert);
	chat_stack::Reset();
	hacks::shared::spam::Reset();
}

bool CanInspect_hook(IClientEntity*) { return true; }

void LevelShutdown_hook(void* thisptr) {
	((LevelShutdown_t*) hooks::hkClientMode->GetMethod(hooks::offLevelShutdown))(thisptr);
	g_Settings.bInvalid = true;
	hacks::shared::aimbot::Reset();
	hacks::shared::airstuck::Reset();
	LEVEL_SHUTDOWN(Bunnyhop);
	LEVEL_SHUTDOWN(ESP);
//	LEVEL_SHUTDOWN(FollowBot);
	LEVEL_SHUTDOWN(Misc);
	chat_stack::Reset();
	hacks::shared::spam::Reset();
}

