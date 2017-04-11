/*
 * playerlist.hpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#ifndef PLAYERLIST_HPP_
#define PLAYERLIST_HPP_

#include "common.h"

namespace playerlist {

constexpr int SERIALIZE_VERSION = 2;

enum class k_EState {
	DEFAULT,
	IGNORE,
	RAGE,
	STATE_LAST = RAGE
};

struct userdata {
	k_EState state { k_EState::DEFAULT };
	int color { 0 };
};

extern std::unordered_map<int, userdata> data;

void Save();
void Load();

userdata& AccessData(int steamid);
userdata& AccessData(CachedEntity* player);

}

#endif /* PLAYERLIST_HPP_ */
