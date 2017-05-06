/*
 * globals.h
 *
 *  Created on: Nov 16, 2016
 *      Author: nullifiedcat
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_

class Vector;
class ConVar;
class CatVar;

typedef void(EstimateAbsVelocity_t)(IClientEntity*, Vector&);
extern EstimateAbsVelocity_t* EstimateAbsVelocity;

extern int g_AppID;
extern unsigned long tickcount;

extern CatVar cathook; // Master switch
extern CatVar ignore_taunting;
extern CatVar send_packets;
extern CatVar show_antiaim;
extern CatVar force_thirdperson;
extern CatVar console_logging;
extern CatVar fast_outline;
extern CatVar roll_speedhack;
extern CatVar force_name;
extern bool need_name_change;

class GlobalSettings {
public:
	void Init();
	bool bInvalid { true };
	Vector last_angles;
};

class CUserCmd;
extern CUserCmd* g_pUserCmd;
extern const char* g_pszTFPath;

extern GlobalSettings g_Settings;

#endif /* GLOBALS_H_ */
