/*
 * others.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "ucccccp.hpp"
#include "hack.hpp"
#include "hitrate.hpp"
#include "chatlog.hpp"
#include "netmessage.hpp"
#include <boost/algorithm/string.hpp>

#if ENABLE_VISUALS

// This hook isn't used yet!
/*int C_TFPlayer__DrawModel_hook(IClientEntity *_this, int flags)
{
    float old_invis = *(float *) ((uintptr_t) _this + 79u);
    if (no_invisibility)
    {
        if (old_invis < 1.0f)
        {
            *(float *) ((uintptr_t) _this + 79u) = 0.5f;
        }
    }

    *(float *) ((uintptr_t) _this + 79u) = old_invis;
}*/


float last_say = 0.0f;



CatCommand spectate("spectate", "Spectate", [](const CCommand &args) {
    if (args.ArgC() < 1)
    {
        spectator_target = 0;
        return;
    }
    int id = atoi(args.Arg(1));
    if (!id)
        spectator_target = 0;
    else
    {
        spectator_target = g_IEngine->GetPlayerForUserID(id);
    }
});

#endif

static CatVar log_sent(CV_SWITCH, "debug_log_sent_messages", "0",
                       "Log sent messages");

static CatCommand plus_use_action_slot_item_server(
    "+cat_use_action_slot_item_server", "use_action_slot_item_server", []() {
        KeyValues *kv = new KeyValues("+use_action_slot_item_server");
        g_pLocalPlayer->using_action_slot_item = true;
        g_IEngine->ServerCmdKeyValues(kv);
    });

static CatCommand minus_use_action_slot_item_server(
    "-cat_use_action_slot_item_server", "use_action_slot_item_server", []() {
        KeyValues *kv = new KeyValues("-use_action_slot_item_server");
        g_pLocalPlayer->using_action_slot_item = false;
        g_IEngine->ServerCmdKeyValues(kv);
    });

static CatVar newlines_msg(CV_INT, "chat_newlines", "0", "Prefix newlines",
                           "Add # newlines before each your message", 0, 24);
// TODO replace \\n with \n
// TODO name \\n = \n
// static CatVar queue_messages(CV_SWITCH, "chat_queue", "0", "Queue messages",
// "Use this if you want to use spam/killsay and still be able to chat normally
// (without having your msgs eaten by valve cooldown)");

static CatVar airstuck(CV_KEY, "airstuck", "0", "Airstuck", "");
static CatVar server_crash_key(CV_KEY, "crash_server", "0", "Server crash key",
                               "hold key and wait...");

static CatVar die_if_vac(CV_SWITCH, "die_if_vac", "0", "Die if VAC banned");


CatEnum namesteal_enum({ "OFF", "PASSIVE", "ACTIVE" });
CatVar namesteal(namesteal_enum, "name_stealer", "0", "Name Stealer",
                 "Attemt to steal your teammates names. Usefull for avoiding "
                 "kicks\nPassive only changes when the name stolen is no "
                 "longer the best name to use\nActive Attemps to change the "
                 "name whenever possible");

static std::string stolen_name;

// Func to get a new entity to steal name from and returns true if a target has
// been found
bool StolenName()
{

    // Array to store potential namestealer targets with a bookkeeper to tell
    // how full it is
    int potential_targets[32];
    int potential_targets_length = 0;

    // Go through entities looking for potential targets
    for (int i = 1; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);

        // Check if ent is a good target
        if (!ent)
            continue;
        if (ent == LOCAL_E)
            continue;
        if (!ent->m_Type == ENTITY_PLAYER)
            continue;
        if (ent->m_bEnemy)
            continue;

        // Check if name is current one
        player_info_s info;
        if (g_IEngine->GetPlayerInfo(ent->m_IDX, &info))
        {

            // If our name is the same as current, than change it
            if (std::string(info.name) == stolen_name)
            {
                // Since we found the ent we stole our name from and it is still
                // good, if user settings are passive, then we return true and
                // dont alter our name
                if ((int) namesteal == 1)
                {
                    return true;
                    // Otherwise we continue to change our name to something
                    // else
                }
                else
                    continue;
            }

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
    int target_random_num =
        floor(RandFloatRange(0, potential_targets_length - 0.1F));

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

static CatVar ipc_name(CV_STRING, "name_ipc", "", "IPC Name");



