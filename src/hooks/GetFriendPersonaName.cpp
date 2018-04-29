/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(GetFriendPersonaName, const char *, ISteamFriends *this_,
                     CSteamID steam_id)
{
    static const GetFriendPersonaName_t original =
            (GetFriendPersonaName_t) hooks::steamfriends.GetMethod(
                    offsets::GetFriendPersonaName());

#if ENABLE_IPC
    if (ipc::peer)
    {
        static std::string namestr(ipc_name.GetString());
        namestr.assign(ipc_name.GetString());
        if (namestr.length() > 3)
        {
            ReplaceString(namestr, "%%", std::to_string(ipc::peer->client_id));
            return namestr.c_str();
        }
    }
#endif

    // Check User settings if namesteal is allowed
    if (namesteal && steamID == g_ISteamUser->GetSteamID())
    {

        // We dont want to steal names while not in-game as there are no targets
        // to steal from. We want to be on a team as well to get teammates names
        if (g_IEngine->IsInGame() && g_pLocalPlayer->team)
        {

            // Check if we have a username to steal, func automaticly steals a
            // name in it.
            if (StolenName())
            {

                // Return the name that has changed from the func above
                return format(stolen_name, "\x0F").c_str();
            }
        }
    }

    if ((strlen(force_name.GetString()) > 1) &&
        steamID == g_ISteamUser->GetSteamID())
    {

        return force_name_newlined;
    }
    return original::GetFriendPersonaName(this_, steam_id);
}
}