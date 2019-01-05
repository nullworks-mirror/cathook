/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <settings/String.hpp>
#include "HookedMethods.hpp"
#include "PlayerTools.hpp"

static settings::String ipc_name{ "name.ipc", "" };
static settings::String force_name{ "name.custom", "" };
static settings::Int namesteal{ "name.namesteal", "0" };

static std::string stolen_name;

int getRng(int min, int max)
{
    static std::random_device rd;
    std::uniform_int_distribution<int> unif(min, max);
    static std::mt19937 rand_engine(rd());

    int x = unif(rand_engine);
    return x;
}

// Func to get a new entity to steal name from and returns true if a target has
// been found
bool StolenName()
{

    // Array to store potential namestealer targets with a bookkeeper to tell
    // how full it is
    int potential_targets[32];
    int potential_targets_length = 0;

    // Go through entities looking for potential targets
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);

        // Check if ent is a good target
        if (!ent)
            continue;
        if (ent == LOCAL_E)
            continue;
        if (ent->m_Type() != ENTITY_PLAYER)
            continue;
        if (ent->m_bEnemy())
            continue;

        // Check if name is current one
        player_info_s info;
        if (g_IEngine->GetPlayerInfo(ent->m_IDX, &info))
        {
            // Invisible character won't fit into name with max. length
            if (std::strlen(info.name) >= 31)
                continue;
            // Ignore Friendly
            if (player_tools::shouldTargetSteamId(info.friendsID) != player_tools::IgnoreReason::DO_NOT_IGNORE)
                continue;
            // If our name is the same as current, then change it
            if (stolen_name == info.name && *namesteal == 1)
                return true;

            // a ent without a name is no ent we need, contine for a different
            // one
        }
        else
            continue;

        // Save the ent to our array
        potential_targets[potential_targets_length] = i;
        potential_targets_length++;

        // With our maximum amount of players reached, dont search for anymore
        if (potential_targets_length >= 32)
            break;
    }

    // Checks to prevent crashes
    if (potential_targets_length == 0)
        return false;

    // Get random number that we can use with our array
    int target_random_num = getRng(0, potential_targets_length - 1);

    // Get a idx from our random array position
    int new_target = potential_targets[target_random_num];

    // Grab username of user
    player_info_s info;
    if (g_IEngine->GetPlayerInfo(new_target, &info))
    {

        // If our name is the same as current, than change it and return true
        stolen_name = std::string(info.name);
        return true;
    }

    // Didnt get playerinfo
    return false;
}
const char *GetNamestealName(CSteamID steam_id)
{
#if ENABLE_IPC
    if (ipc::peer)
    {
        std::string namestr(*ipc_name);
        if (namestr.length() > 3)
        {
            ReplaceString(namestr, "%%", std::to_string(ipc::peer->client_id));
            ReplaceSpecials(namestr);
            return namestr.c_str();
        }
    }
#endif

    // Check User settings if namesteal is allowed
    if (namesteal && steam_id == g_ISteamUser->GetSteamID())
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
                return format(stolen_name, "\e").c_str();
            }
        }
    }

    if ((*force_name).size() > 1 && steam_id == g_ISteamUser->GetSteamID())
    {
        auto new_name = force_name.toString();
        ReplaceSpecials(new_name);

        return new_name.c_str();
    }
    return nullptr;
}
namespace hooked_methods
{

DEFINE_HOOKED_METHOD(GetFriendPersonaName, const char *, ISteamFriends *this_, CSteamID steam_id)
{
    const char *new_name = GetNamestealName(steam_id);
    return (new_name ? new_name : original::GetFriendPersonaName(this_, steam_id));
}
static InitRoutine init([]() {
    namesteal.installChangeCallback([](settings::VariableBase<int> &var, int new_val) {
        if (new_val != 0)
        {
            const char *xd = GetNamestealName(g_ISteamUser->GetSteamID());
            if (CE_BAD(LOCAL_E) || !xd || !strcmp(LOCAL_E->player_info.name, xd))
                return;
            NET_SetConVar setname("name", xd);
            INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
            if (ch)
            {
                setname.SetNetChannel(ch);
                setname.SetReliable(false);
                ch->SendNetMsg(setname, false);
            }
        }
    });
});
static Timer set_name{};
static void cm()
{
    if (!namesteal)
        return;
    if (!set_name.test_and_set(300000))
        return;
    const char *name = GetNamestealName(g_ISteamUser->GetSteamID());
    if (CE_BAD(LOCAL_E) || !name)
        return;
    // Didn't change name - update timer a bit
    if (!strcmp(LOCAL_E->player_info.name, name))
    {
        set_name.last += std::chrono::seconds(170);
        return;
    }
    NET_SetConVar setname("name", name);
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch)
    {
        setname.SetNetChannel(ch);
        setname.SetReliable(false);
        ch->SendNetMsg(setname, false);
    }
}

static InitRoutine runinit([]() { EC::Register(EC::CreateMove, cm, "cm_namesteal", EC::late); });

} // namespace hooked_methods
