/*
 * globals.cpp
 *
 *  Created on: Nov 25, 2016
 *      Author: nullifiedcat
 */

#include "common.h"
#include "sdk.h"
#include "copypasted/CSignature.h"

EstimateAbsVelocity_t* EstimateAbsVelocity = 0;

int g_AppID = 0;

void ThirdpersonCallback(IConVar* var, const char* pOldValue, float flOldValue) {
	if (force_thirdperson.convar_parent && !force_thirdperson) {
		if (g_pLocalPlayer && g_pLocalPlayer->entity)
			CE_INT(g_pLocalPlayer->entity, netvar.nForceTauntCam) = 0;
	}
}

unsigned long tickcount = 0;
bool need_name_change = true;
CatVar force_name(CV_STRING, "name", "", "Force name");
CatVar cathook(CV_SWITCH, "enabled", "1", "CatHook enabled", "Disabling this completely disables cathook (can be re-enabled)");
CatVar ignore_taunting(CV_SWITCH, "ignore_taunting", "1", "Ignore taunting", "Aimbot/Triggerbot won't attack taunting enemies");
CatVar send_packets(CV_SWITCH, "sendpackets", "1", "Send packets", "Internal use");
CatVar show_antiaim(CV_SWITCH, "thirdperson_angles", "1", "Real TP angles", "You can see your own AA/Aimbot angles in thirdperson");
CatVar force_thirdperson(CV_SWITCH, "thirdperson", "0", "Thirdperson", "Enable thirdperson view");
CatVar console_logging(CV_SWITCH, "log", "1", "Debug Log", "Disable this if you don't need cathook messages in your console");
//CatVar fast_outline(CV_SWITCH, "fastoutline", "0", "Low quality outline", "Might increase performance when there is a lot of ESP text to draw");
CatVar roll_speedhack(CV_KEY, "rollspeedhack", "0", "Roll Speedhack", "Roll speedhack key");

void GlobalSettings::Init() {
	EstimateAbsVelocity = (EstimateAbsVelocity_t*)gSignatures.GetClientSignature("55 89 E5 56 53 83 EC 30 8B 5D 08 8B 75 0C E8 4D 2E 01 00 39 D8 74 69 0F B6 05 24 3F 00 02 81 C3 B8 02 00 00 C6 05 24 3F 00 02 01 88 45 F0 A1 20 3F 00 02 89 45 F4 A1 28 3F 00 02 89 45 EC 8D 45 EC A3 28 3F 00 02 A1 14 C8 F6 01 8B 40 0C 89 74 24 04 89 1C 24 89 44 24 08 E8 A2 41 00 00 0F B6 45 F0 A2 24 3F 00 02 8B 45 F4 A3 20 3F 00 02 8B 45 EC A3 28 3F 00 02 83 C4 30 5B 5E 5D C3");
	force_thirdperson.OnRegister([](CatVar* var) {
		var->convar_parent->InstallChangeCallback(ThirdpersonCallback);
	});
	bInvalid = true;
}

CUserCmd* g_pUserCmd = nullptr;
const char* g_pszTFPath = 0;

GlobalSettings g_Settings;
