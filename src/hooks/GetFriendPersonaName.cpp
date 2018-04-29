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
    return original::GetFriendPersonaName(this_, steam_id);
}
}