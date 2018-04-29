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

static CatVar medal_flip(CV_SWITCH, "medal_flip", "0", "Infinite Medal Flip",
                         "");

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

static CatVar no_arms(CV_SWITCH, "no_arms", "0", "No Arms",
                      "Removes arms from first person");
static CatVar no_hats(CV_SWITCH, "no_hats", "0", "No Hats",
                      "Removes non-stock hats");
float last_say = 0.0f;

CatVar override_fov_zoomed(CV_FLOAT, "fov_zoomed", "0", "FOV override (zoomed)",
                           "Overrides FOV with this value when zoomed in "
                           "(default FOV when zoomed is 20)");
CatVar override_fov(CV_FLOAT, "fov", "0", "FOV override",
                    "Overrides FOV with this value");

CatVar freecam(CV_KEY, "debug_freecam", "0", "Freecam");
int spectator_target{ 0 };

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
static CatVar crypt_chat(
    CV_SWITCH, "chat_crypto", "1", "Crypto chat",
    "Start message with !! and it will be only visible to cathook users");
static CatVar chat_filter(CV_STRING, "chat_censor", "", "Censor words",
                          "Spam Chat with newlines if the chosen words are "
                          "said, seperate with commas");
static CatVar chat_filter_enabled(CV_SWITCH, "chat_censor_enabled", "0",
                                  "Enable censor", "Censor Words in chat");
static CatVar server_crash_key(CV_KEY, "crash_server", "0", "Server crash key",
                               "hold key and wait...");

static CatVar die_if_vac(CV_SWITCH, "die_if_vac", "0", "Die if VAC banned");

static CatVar resolver(CV_SWITCH, "resolver", "0", "Resolve angles");

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

CatVar nightmode(CV_SWITCH, "nightmode", "0", "Enable nightmode", "");

static CatVar clean_chat(CV_SWITCH, "clean_chat", "0", "Clean chat",
                         "Removes newlines from chat");
static CatVar dispatch_log(CV_SWITCH, "debug_log_usermessages", "0",
                           "Log dispatched user messages");
std::string clear = "";
std::string lastfilter{};
std::string lastname{};
static bool retrun = false;

static Timer sendmsg{};
static Timer gitgud{};

const char *skynum[] = { "sky_tf2_04",
                         "sky_upward",
                         "sky_dustbowl_01",
                         "sky_goldrush_01",
                         "sky_granary_01",
                         "sky_well_01",
                         "sky_gravel_01",
                         "sky_badlands_01",
                         "sky_hydro_01",
                         "sky_night_01",
                         "sky_nightfall_01",
                         "sky_trainyard_01",
                         "sky_stormfront_01",
                         "sky_morningsnow_01",
                         "sky_alpinestorm_01",
                         "sky_harvest_01",
                         "sky_harvest_night_01",
                         "sky_halloween",
                         "sky_halloween_night_01",
                         "sky_halloween_night2014_01",
                         "sky_island_01",
                         "sky_jungle_01",
                         "sky_invasion2fort_01",
                         "sky_well_02",
                         "sky_outpost_01",
                         "sky_coastal_01",
                         "sky_rainbow_01",
                         "sky_badlands_pyroland_01",
                         "sky_pyroland_01",
                         "sky_pyroland_02",
                         "sky_pyroland_03" };
CatEnum skys({ "sky_tf2_04",
               "sky_upward",
               "sky_dustbowl_01",
               "sky_goldrush_01",
               "sky_granary_01",
               "sky_well_01",
               "sky_gravel_01",
               "sky_badlands_01",
               "sky_hydro_01",
               "sky_night_01",
               "sky_nightfall_01",
               "sky_trainyard_01",
               "sky_stormfront_01",
               "sky_morningsnow_01",
               "sky_alpinestorm_01",
               "sky_harvest_01",
               "sky_harvest_night_01",
               "sky_halloween",
               "sky_halloween_night_01",
               "sky_halloween_night2014_01",
               "sky_island_01",
               "sky_jungle_01",
               "sky_invasion2fort_01",
               "sky_well_02",
               "sky_outpost_01",
               "sky_coastal_01",
               "sky_rainbow_01",
               "sky_badlands_pyroland_01",
               "sky_pyroland_01",
               "sky_pyroland_02",
               "sky_pyroland_03" });
static CatVar
    skybox_changer(skys, "skybox_changer", "0", "Change Skybox to this skybox",
                   "Change Skybox to this skybox, only changes on map load");
static CatVar halloween_mode(CV_SWITCH, "halloween_mode", "0",
                             "Forced Halloween mode",
                             "forced tf_forced_holiday 2");
