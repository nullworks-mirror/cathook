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
	FRIEND,
	RAGE,
	IPC,
	DEVELOPER,
	STATE_LAST = DEVELOPER
};


constexpr int k_Colors[] = { 0, colors::Create(99, 226, 161, 255), colors::Create(226, 204, 99, 255), colors::Create(232, 134, 6, 255), 0 };
const std::string k_Names[] = { "DEFAULT", "FRIEND", "RAGE", "IPC", "DEVELOPER" };

struct userdata {
	k_EState state { k_EState::DEFAULT };
	int color { 0 };
};

extern std::unordered_map<unsigned, userdata> data;

void Save();
void Load();

void DoNotKillMe();

constexpr bool IsFriendly(k_EState state) {
	return state == k_EState::DEVELOPER || state == k_EState::FRIEND || state == k_EState::IPC;
}

int Color(unsigned steamid);
int Color(CachedEntity* player);
userdata& AccessData(unsigned steamid);
userdata& AccessData(CachedEntity* player);
bool IsDefault(unsigned steamid);
bool IsDefault(CachedEntity* player);

}

#endif /* PLAYERLIST_HPP_ */
