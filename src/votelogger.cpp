/*
 * votelogger.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <boost/algorithm/string.hpp>
#include <settings/Bool.hpp>

static settings::Bool enable{ "vote-log.enable", "false" };
static settings::Bool requeue{ "vote-log.requeue", "false" };

namespace votelogger
{

Timer antikick{};
bool active = false;

const std::string tf_classes[] = { "class",   "scout",   "sniper", "soldier",
                                   "demoman", "medic",   "heavy",  "pyro",
                                   "spy",     "engineer" };
void user_message(bf_read &buffer, int type)
{

    bool islocalplayer = false;
    if (!enable)
        return;
    switch (type)
    {
    case 45:
        // Call Vote Failed
        break;
    case 46:
    {
        islocalplayer        = false;
        unsigned char caller = buffer.ReadByte();
        // unknown
        buffer.ReadByte();
        char reason[64];
        char name[64];
        buffer.ReadString(reason, 64, false, nullptr);
        buffer.ReadString(name, 64, false, nullptr);
        unsigned char eid = buffer.ReadByte();
        buffer.Seek(0);
        eid >>= 1;

        unsigned steamID = 0;
        player_info_s info;
        if (g_IEngine->GetPlayerInfo(eid, &info))
            steamID = info.friendsID;
        if (eid == LOCAL_E->m_IDX ||
            playerlist::AccessData(steamID).state ==
                playerlist::k_EState::FRIEND)
        {
            islocalplayer = true;
        }

        logging::Info("Vote called to kick %s [U:1:%u] for %s", name, steamID,
                      reason);
        break;
    }
    case 47:
        logging::Info("Vote passed");
        if (islocalplayer && requeue)
            tfmm::queue_start();
        break;
    case 48:
        logging::Info("Vote failed");
        break;
    case 49:
        logging::Info("VoteSetup?");
        break;
    }
}
}
