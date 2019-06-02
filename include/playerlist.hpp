/*
 * playerlist.hpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

namespace playerlist
{

constexpr int SERIALIZE_VERSION = 3;

enum class k_EState
{
    DEFAULT = 0,
    FRIEND,
    RAGE,
    IPC,
    DEVELOPER,
    CAT,
    STATE_LAST = CAT
};
#if ENABLE_VISUALS
extern rgba_t k_Colors[];
static_assert(sizeof(rgba_t) == sizeof(float) * 4, "player list is going to be incompatible with no visual version");
#endif
const std::string k_Names[]    = { "DEFAULT", "FRIEND", "RAGE", "IPC", "DEVELOPER", "CAT" };
const char *const k_pszNames[] = { "DEFAULT", "FRIEND", "RAGE", "IPC", "DEVELOPER", "CAT" };

struct userdata
{
    k_EState state{ k_EState::DEFAULT };
#if ENABLE_VISUALS
    rgba_t color{ 0, 0, 0, 0 };
#else
    char color[16]{};
#endif
    float inventory_value{ 0 };
    unsigned deaths_to{ 0 };
    unsigned kills{ 0 };
};

extern std::unordered_map<unsigned, userdata> data;

void Save();
void Load();

constexpr bool IsFriendly(k_EState state)
{
    return state == k_EState::DEVELOPER || state == k_EState::FRIEND || state == k_EState::IPC;
}
#if ENABLE_VISUALS
rgba_t Color(unsigned steamid);
rgba_t Color(CachedEntity *player);
#endif
userdata &AccessData(unsigned steamid);
userdata &AccessData(CachedEntity *player);
bool IsDefault(unsigned steamid);
bool IsDefault(CachedEntity *player);
} // namespace playerlist
