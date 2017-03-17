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

class GlobalSettings {
public:
	void Init();
// TODO
//	CatVar* bMaxPerformance;
	CatVar* bHackEnabled;
	CatVar* bIgnoreTaunting;
//	CatVar* bProfiler;
//	CatVar* bNoFlinch;
	CatVar* bSendPackets;
	CatVar* sDisconnectMsg;
	CatVar* bShowAntiAim;
	CatVar* bThirdperson;
	CatVar* bDebugLog;
	Vector last_angles;
	CatVar* bFastOutline;
	CatVar* kRollSpeedhack;
	CatVar* bFastVischeck;
	bool bInvalid;
};

class CUserCmd;
extern CUserCmd* g_pUserCmd;
extern const char* g_pszTFPath;

extern GlobalSettings g_Settings;

#endif /* GLOBALS_H_ */
