/*
 * crits.h
 *
 *  Created on: Feb 25, 2017
 *      Author: nullifiedcat
 */

#ifndef CRITS_H_
#define CRITS_H_

class CUserCmd;
class IClientEntity;

bool AllowAttacking();
bool RandomCrits();
bool WeaponCanCrit();
bool IsAttackACrit(CUserCmd* cmd);
void ResetCritHack();
void LoadSavedState();

#include "beforecheaders.h"
#include <unordered_map>
#include "aftercheaders.h"

extern int* g_PredictionRandomSeed;
extern std::unordered_map<int, int> command_number_mod;

//bool CalcIsAttackCritical(IClientEntity* weapon);


#endif /* CRITS_H_ */
