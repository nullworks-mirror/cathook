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

// BUGBUG TODO this struct is outdated
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

extern CatVar crit_info;
extern CatVar crit_key;
extern CatVar crit_melee;

namespace criticals
{

void create_move();
bool random_crits_enabled();

}

#include <beforecheaders.hpp>
#include <unordered_map>
#include <aftercheaders.hpp>

extern int* g_PredictionRandomSeed;
extern std::unordered_map<int, int> command_number_mod;

#endif /* CRITS_HPP_ */
