/*
 * hack.cpp
 *
 *  Created on: Oct 3, 2016
 *      Author: nullifiedcat
 */

#include "hack.h"

#include "beforecheaders.h"
#include <vector>
#include <map>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sys/prctl.h>
//#include <cstring>
#include <unistd.h>
#include <link.h>
#include <unordered_map>
#include <cstring>
#include <memory>
#include "segvcatch/segvcatch.h"
#include <csignal>
#include <sys/sysinfo.h>
#include "aftercheaders.h"

#include <steam/isteamuser.h>
// All Hacks
#include "hacks/hacklist.h"

#include "common.h"
#include "sharedobj.h"
#include "hooks.h"
#include "netmessage.h"
#include "profiler.h"
#include "cvwrapper.h"

#define STRINGIFY(x) #x
#define TO_STRING(x) STRINGIFY(x)

#include "ftrender.hpp"
#include "hooks/hookedmethods.h"

#include "sdk.h"
#include "vfunc.h"
#include "copypasted/CSignature.h"
#include "copypasted/Netvar.h"
#include "CDumper.h"
#include <KeyValues.h>

/*
 *  Credits to josh33901 aka F1ssi0N for butifel F1Public and Darkstorm 2015 Linux
 */

bool hack::shutdown = false;

const std::string& hack::GetVersion() {
	static std::string version("Unknown Version");
	static bool version_set = false;
	if (version_set) return version;
#if defined(GIT_COMMIT_HASH) && defined(GIT_COMMIT_DATE)
		version = "Version: #" GIT_COMMIT_HASH " " GIT_COMMIT_DATE;
#endif
	version_set = true;
	return version;
}

const std::string& hack::GetType() {
	static std::string version("Unknown Type");
	static bool version_set = false;
	if (version_set) return version;
	version = "";
#if not defined(IPC_ENABLED)
	version += " NOIPC";
#endif
#if not ENABLE_GUI
		version += " NOGUI";
#else
	version += " IMGUI";
#endif

#ifndef DYNAMIC_CLASSES

#ifdef BUILD_GAME
		version += " GAME " TO_STRING(BUILD_GAME);
#else
		version += " UNIVERSAL";
#endif

#else
		version += " DYNAMIC";
#endif
		
	version = version.substr(1);
	version_set = true;
	return version;
}

std::mutex hack::command_stack_mutex;
std::stack<std::string>& hack::command_stack() {
	static std::stack<std::string> stack;
	return stack;
}

class AdvancedEventListener : public IGameEventListener {
public:
	virtual void FireGameEvent( KeyValues * event) {
		const char* name = event->GetName();
		if (!strcmp(name, "player_connect_client")) {
			PrintChat("\x07%06X%s\x01 \x07%06X%s\x01 joining", 0xa06ba0, event->GetString("name"), 0x914e65, event->GetString("networkid"));
		} else if (!strcmp(name, "player_activate")) {
			int uid = event->GetInt("userid");
			int entity = g_IEngine->GetPlayerForUserID(uid);
			player_info_s info;
			if (g_IEngine->GetPlayerInfo(entity, &info)) {
				PrintChat("\x07%06X%s\x01 connected", 0xa06ba0, info.name);
			}
		} else if (!strcmp(name, "player_disconnect")) {
			CachedEntity* player = ENTITY(g_IEngine->GetPlayerForUserID(event->GetInt("userid")));
			PrintChat("\x07%06X%s\x01 \x07%06X%s\x01 disconnected", colors::chat::team(player->m_iTeam), event->GetString("name"), 0x914e65, event->GetString("networkid"));
		} else if (!strcmp(name, "player_team")) {
			if (event->GetBool("disconnect") != 1) {
				int oteam = event->GetInt("oldteam");
				int nteam = event->GetInt("team");
				const char* oteam_s = teamname(oteam);
				const char* nteam_s = teamname(nteam);
				logging::Info("%d -> %d", oteam, nteam);
				PrintChat("\x07%06X%s\x01 changed team (\x07%06X%s\x01 -> \x07%06X%s\x01)", 0xa06ba0, event->GetString("name"), colors::chat::team(oteam), oteam_s, colors::chat::team(nteam), nteam_s);
			}
		}
	}
};

AdvancedEventListener adv_event_listener {};

void hack::ExecuteCommand(const std::string command) {
	std::lock_guard<std::mutex> guard(hack::command_stack_mutex);
	hack::command_stack().push(command);
}

ConCommand* hack::c_Cat = 0;

void hack::CC_Cat(const CCommand& args) {
	g_ICvar->ConsoleColorPrintf(Color(255, 255, 255, 255), "kathook");
	g_ICvar->ConsoleColorPrintf(Color(  0,   0, 255, 255), " by ");
	g_ICvar->ConsoleColorPrintf(Color(255,   0,   0, 255), "nullifiedcat\n");
	/*int white = colors::white, blu = colors::blu, red = colors::red;
	g_ICvar->ConsoleColorPrintf(*reinterpret_cast<Color*>(&white), "kathook");
	g_ICvar->ConsoleColorPrintf(*reinterpret_cast<Color*>(&blu), " by ");
	g_ICvar->ConsoleColorPrintf(*reinterpret_cast<Color*>(&red), "nullifiedcat\n");
	g_ICvar->ConsoleColorPrintf(*reinterpret_cast<Color*>(&white), GetVersion().c_str());
	g_ICvar->ConsoleColorPrintf(*reinterpret_cast<Color*>(&white), "\n");*/
}

void hack::Initialize() {
	logging::Info("Initializing...");
	srand(time(0));
	prctl(PR_SET_DUMPABLE,0,42,42,42);
	sharedobj::LoadAllSharedObjects();
	CreateInterfaces();
	CDumper dumper;
	dumper.SaveDump();
	logging::Info("Is TF2? %d", IsTF2());
	logging::Info("Is TF2C? %d", IsTF2C());
	logging::Info("Is HL2DM? %d", IsHL2DM());
	logging::Info("Is CSS? %d", IsCSS());
	logging::Info("Is TF? %d", IsTF());
	InitClassTable();
	IF_GAME (IsTF2()) {
		uintptr_t mmmf = (gSignatures.GetClientSignature("C7 44 24 04 09 00 00 00 BB ? ? ? ? C7 04 24 00 00 00 00 E8 ? ? ? ? BA ? ? ? ? 85 C0 B8 ? ? ? ? 0F 44 DA") + 37);
		if (mmmf) {
			unsigned char patch1[] = { 0x89, 0xD3, 0x90 };
			unsigned char patch2[] = { 0x89, 0xC2, 0x90 };
			Patch((void*)mmmf, (void*)patch1, 3);
			Patch((void*)(mmmf + 8), (void*)patch2, 3);
		}
		/*uintptr_t canInspectSig = (gSignatures.GetClientSignature("55 0F 57 C0 89 E5 83 EC 48 8B 45 08 F3 0F 11 04 24 F3 0F 11 45 E8 C7 44 24 10 01 00 00 00 C7 44 24 0C 00 00 00 00 89 44 24 08 C7 44 24 ? ? ? ? ? E8 ? ? ? ? F3 0F 10 45 E8 D9 5D E4 F3 0F 10 4D E4 C9 0F 2F C8 0F 95 C0 C3") + 72);
		if (canInspectSig) {
			unsigned char patch[] = { 0xB0, 0x01, 0x90 };
			Patch((void*)canInspectSig, (void*)patch, 3);
		}*/
	}
	BeginConVars();
	hack::c_Cat = CreateConCommand(CON_NAME, &hack::CC_Cat, "Info");
	g_Settings.Init();
	EndConVars();
	draw::Initialize();
#if ENABLE_GUI
	g_pGUI = new CatGUI();
	g_pGUI->Setup();
#endif
	gNetvars.init();
	InitNetVars();
	g_pLocalPlayer = new LocalPlayer();
	g_pPlayerResource = new TFPlayerResource();

	/*
	 * TIME FOR HOOKING! wow
	 */
	hooks::panel.Set(g_IPanel);
	hooks::panel.HookMethod((void*)PaintTraverse_hook, offsets::PaintTraverse());
	hooks::panel.Apply();
	uintptr_t* clientMode = 0;
	// Bad way to get clientmode.
	// FIXME [MP]?
	while(!(clientMode = **(uintptr_t***)((uintptr_t)((*(void***)g_IBaseClient)[10]) + 1))) {
		sleep(1);
	}
	hooks::clientmode.Set((void*)clientMode);
	hooks::clientmode.HookMethod((void*)CreateMove_hook, offsets::CreateMove());
	hooks::clientmode.HookMethod((void*)OverrideView_hook, offsets::OverrideView());
	hooks::clientmode.HookMethod((void*)LevelInit_hook, offsets::LevelInit());
	hooks::clientmode.HookMethod((void*)LevelShutdown_hook, offsets::LevelShutdown());
	hooks::clientmode.Apply();
	hooks::clientmode4.Set((void*)(clientMode), 4);
	hooks::clientmode4.HookMethod((void*)FireGameEvent_hook, offsets::FireGameEvent());
	hooks::clientmode4.Apply();
	hooks::client.Set(g_IBaseClient);
	hooks::client.HookMethod((void*)FrameStageNotify_hook, offsets::FrameStageNotify());
	hooks::client.HookMethod((void*)DispatchUserMessage_hook, offsets::DispatchUserMessage());
	hooks::client.HookMethod((void*)IN_KeyEvent_hook, offsets::IN_KeyEvent());
	hooks::client.Apply();
	hooks::input.Set(g_IInput);
	hooks::input.HookMethod((void*)GetUserCmd_hook, offsets::GetUserCmd());
	hooks::input.Apply();
	hooks::modelrender.Set(g_IVModelRender);
	hooks::modelrender.HookMethod((void*)DrawModelExecute_hook, offsets::DrawModelExecute());
	hooks::modelrender.Apply();
	hooks::steamfriends.Set(g_ISteamFriends);
	hooks::steamfriends.HookMethod((void*)GetFriendPersonaName_hook, offsets::GetFriendPersonaName());
	hooks::steamfriends.Apply();
	//logging::Info("After hacking: %s", g_ISteamFriends->GetPersonaName());
	// Sadly, it doesn't work as expected :(
	/*hooks::hkBaseClientState = new hooks::VMTHook();
	hooks::hkBaseClientState->Init((void*)g_IBaseClientState, 0);
	hooks::hkBaseClientState->HookMethod((void*)GetClientName_hook, hooks::offGetClientName);
	hooks::hkBaseClientState->Apply();*/
	//hooks::hkBaseClientState8 = new hooks::VMTHook();
	//hooks::hkBaseClientState8->Init((void*)g_IBaseClientState, 8);
	//hooks::hkBaseClientState8->HookMethod((void*)ProcessSetConVar_hook, hooks::offProcessSetConVar);
	//hooks::hkBaseClientState8->HookMethod((void*)ProcessGetCvarValue_hook, hooks::offProcessGetCvarValue);
	//hooks::hkBaseClientState8->Apply();

	// FIXME [MP]
	InitStrings();
	hacks::shared::killsay::Init();
#if ENABLE_GUI
	// cat_reloadscheme to load imgui
	hack::command_stack().push("cat_reloadscheme");
#endif
	hack::command_stack().push("exec cat_autoexec");
	hack::command_stack().push("cat_killsay_reload");
	hack::command_stack().push("cat_spam_reload");
	logging::Info("Hooked!");
	velocity::Init();
	playerlist::Load();
	if (g_ppScreenSpaceRegistrationHead && g_pScreenSpaceEffects) {
		effect_chams::g_pEffectChams = new CScreenSpaceEffectRegistration("_cathook_chams", &effect_chams::g_EffectChams);
		g_pScreenSpaceEffects->EnableScreenSpaceEffect("_cathook_chams");
		effect_chams::g_EffectChams.Init();
		effect_glow::g_pEffectGlow = new CScreenSpaceEffectRegistration("_cathook_glow", &effect_glow::g_EffectGlow);
		g_pScreenSpaceEffects->EnableScreenSpaceEffect("_cathook_glow");
	}
	//for (CScreenSpaceEffectRegistration* reg = *g_ppScreenSpaceRegistrationHead; reg; reg = reg->m_pNext) {
	//	logging::Info("%s", reg->m_pEffectName);
	//}
	logging::Info("SSE enabled..");
	DoSDLHooking();
	logging::Info("SDL hooking done");
	g_IGameEventManager->AddListener(&adv_event_listener, false);
	hacks::shared::anticheat::Init();

}

void hack::Think() {
	usleep(250000);
}

void hack::Shutdown() {
	if (hack::shutdown) return;
	hack::shutdown = true;
	playerlist::Save();
	DoSDLUnhooking();
	logging::Info("Unregistering convars..");
	ConVar_Unregister();
	logging::Info("Shutting down killsay...");
	hacks::shared::killsay::Shutdown();
	logging::Info("Success..");
}
