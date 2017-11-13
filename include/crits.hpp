/*
 * crits.h
 *
 *  Created on: Feb 25, 2017
 *      Author: nullifiedcat
 */

#ifndef CRITS_HPP_
#define CRITS_HPP_

class CUserCmd;
class IClientEntity;

struct crithack_saved_state {
	float bucket; // 2612
	bool unknown2831;
	int seed; // 2868
	float time; // 2872
	int unknown2616;
	int unknown2620;
	float unknown2856;
	float unknown2860;
	void Save(IClientEntity* entity);
	void Load(IClientEntity* entity);
};

extern bool weapon_can_crit_last;

bool CritKeyDown();
bool AllowAttacking();
bool RandomCrits();
bool WeaponCanCrit();
bool IsAttackACrit(CUserCmd* cmd);
void ResetCritHack();
void LoadSavedState();
void ModifyCommandNumber();

#include <beforecheaders.hpp>
#include <unordered_map>
#include <aftercheaders.hpp>
class CatVar;
extern CatVar experimental_crit_hack;

extern int* g_PredictionRandomSeed;
extern std::unordered_map<int, int> command_number_mod;

//bool CalcIsAttackCritical(IClientEntity* weapon);


#endif /* CRITS_HPP_ */
