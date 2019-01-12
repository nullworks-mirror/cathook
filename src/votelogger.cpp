/*
 * votelogger.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <boost/algorithm/string.hpp>
#include <settings/Bool.hpp>

static settings::Bool vote_kicky{ "votelogger.autovote.yes", "false" };
static settings::Bool vote_kickn{ "votelogger.autovote.no", "false" };
static settings::Bool party_say{ "votelogger.partysay", "true" };

namespace votelogger
{

static bool was_local_player{ false };

void dispatchUserMessage(bf_read &buffer, int type)
{
    switch (type)
    {
    case 45:
        // Call Vote Failed
        break;
    case 46:
    {
        // TODO: Add always vote no/vote no on friends. Cvar is "vote option2"
        was_local_player = false;
        int team         = buffer.ReadByte();
        int caller       = buffer.ReadByte();
        char reason[64];
        char name[64];
        buffer.ReadString(reason, 64, false, nullptr);
        buffer.ReadString(name, 64, false, nullptr);
        auto eid = (unsigned char) buffer.ReadByte();
        buffer.Seek(0);
        eid >>= 1;

        unsigned steamID = 0;
        // info is the person getting kicked,
        // info2 is the person calling the kick.
        player_info_s info{}, info2{};
        if (!g_IEngine->GetPlayerInfo(eid, &info))
            break;
        steamID = info.friendsID;
        if (eid == LOCAL_E->m_IDX)
            was_local_player = true;
        if (*vote_kickn || *vote_kicky)
        {
            auto &pl = playerlist::AccessData(info.friendsID);
            if (*vote_kickn && pl.state != playerlist::k_EState::RAGE && pl.state != playerlist::k_EState::DEFAULT)
                g_IEngine->ClientCmd_Unrestricted("vote option2");
            else if (*vote_kicky && (pl.state == playerlist::k_EState::RAGE || pl.state == playerlist::k_EState::DEFAULT))
                g_IEngine->ClientCmd_Unrestricted("vote option1");
        }
        if (*party_say && g_IEngine->GetPlayerInfo(caller, &info2))
        {
            char formated_string[512];
            // because tf2 is stupid and doesn't have escape characters,
            // use the greek question marks instead. big brain.
            std::string kicked_name(info.name), caller_name(info2.name);
            /* ';' (0x3B) regular replaced with unicode analog ';' (0xCD 0xBE)
             * to prevent exploits (by crafting name such that it executes command)
             * and output message properly
             * TO DO: Saner way to accomplish same */
            ReplaceString(kicked_name, ";", ";");
            ReplaceString(caller_name, ";", ";");
            std::snprintf(formated_string, sizeof(formated_string),
                "say_party [CAT] votekick called: %s => %s (%s)",
                caller_name.c_str(), kicked_name.c_str(), reason);
            g_IEngine->ExecuteClientCmd(formated_string);
        }
        logging::Info("Vote called to kick %s [U:1:%u] for %s", name, steamID, reason);
        break;
    }
    case 47:
        logging::Info("Vote passed");
        // if (was_local_player && requeue)
        //    tfmm::startQueue();
        break;
    case 48:
        logging::Info("Vote failed");
        break;
    case 49:
        logging::Info("VoteSetup?");
        break;
    default:
        break;
    }
}
} // namespace votelogger
