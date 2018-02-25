/*
 * Events.cpp
 *
 *  Created on: Feb 24, 2018
 *      Author: bencat07
 */
#include "common.hpp"

namespace hacks
{
namespace shared
{
namespace event
{
std::array<int, 2048> data;
void CreateMove()
{
    if (!event_log)
        return;
    for (int i; i < g_IEngine->GetMaxClients(); ++i)
    {
        CachedEntity *ent = ENTITY(i);
        if (!CE_GOOD(ent))
            continue;
        player_info_s info;
        g_IEngine->GetPlayerInfo(ent->m_IDX, &info);
        if (!data[i])
            data[i] = CE_INT(ent, netvar.iClass);

        if (data[i] != CE_INT(ent, netvar.iClass))
        {
        	if (ent->m_iTeam == TEAM_RED) {
            PrintChat("\x07%06X%s\x01 changed from \"\x07%06X%s\x01\" to "
                      "\"\x07%06X%s\x01\"",
					  0xb8383b, info.name, 0x6b9ca0, inttochar(data[i]),
                      0x6ba072, inttochar(CE_INT(ent, netvar.iClass)));
        	}
        	else if (ent->m_iTeam == TEAM_BLU) {
                PrintChat("\x07%06X%s\x01 changed from \"\x07%06X%s\x01\" to "
                          "\"\x07%06X%s\x01\"",
						  0x5885a2, info.name, 0x6b9ca0, inttochar(data[i]),
                          0x6ba072, inttochar(CE_INT(ent, netvar.iClass)));
            }
            data[i] = CE_INT(ent, netvar.iClass);

        }
    }
}
char *inttochar(int i)
{
    switch (i)
    {
    case tf_scout:
        return "Scout";
        break;
    case tf_sniper:
        return "Sniper";
        break;
    case tf_soldier:
        return "Soldier";
        break;
    case tf_demoman:
        return "Demoman";
        break;
    case tf_medic:
        return "Medic";
        break;
    case tf_heavy:
        return "Heavy";
        break;
    case tf_pyro:
        return "Pyro";
        break;
    case tf_spy:
        return "Spy";
        break;
    case tf_engineer:
        return "Engineer";
        break;
    default:
        return "Invalid";
        break;
    }
}
}
}
}
