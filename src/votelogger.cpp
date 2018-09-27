/*
 * votelogger.cpp
 *
 *  Created on: Dec 31, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <boost/algorithm/string.hpp>
#include <settings/Bool.hpp>

static settings::Bool requeue{ "hack.requeue-on-kick", "false" };
static settings::Bool vote_kicky{ "hack.autovote.yes", "false" };
static settings::Bool vote_kickn{ "hack.autovote.no", "false" };
static settings::Bool party_say{ "hack.partysay", "true" };

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
        int team	=	buffer.ReadByte();
        int caller      =	buffer.ReadByte();	//	client id of init apparently
        char reason[64];
        char name[64];
        buffer.ReadString(reason, 64, false, nullptr);
        buffer.ReadString(name, 64, false, nullptr);
        auto eid = (unsigned char) buffer.ReadByte();
        buffer.Seek(0);
        eid >>= 1;

        unsigned steamID = 0;
        player_info_s info{}, info2{};	//	info is the person getting kicked, info2 is the person calling the kick.;
        if (g_IEngine->GetPlayerInfo(eid, &info))
            steamID = info.friendsID;
        if (eid == LOCAL_E->m_IDX)
            was_local_player = true;
        if (*vote_kickn)
            if (playerlist::AccessData(info.friendsID).state !=
                    playerlist::k_EState::RAGE &&
                playerlist::AccessData(info.friendsID).state !=
                    playerlist::k_EState::DEFAULT)
                g_IEngine->ClientCmd_Unrestricted("vote option2");
        if (*vote_kicky)
            if (playerlist::AccessData(info.friendsID).state ==
                    playerlist::k_EState::RAGE ||
                playerlist::AccessData(info.friendsID).state ==
                    playerlist::k_EState::DEFAULT)
                g_IEngine->ClientCmd_Unrestricted("vote option1");
        if (*party_say)
        {
        	g_IEngine->GetPlayerInfo(caller, &info2);
        	g_IEngine->ExecuteClientCmd(format("say_party [cat] votekick called: ",
        			boost::replace_all_copy((std::string)info2.name, ";", ";"),
					" => ",
        			boost::replace_all_copy((std::string)info.name, ";", ";"),
        			" (",reason,")").c_str());
        	//	because tf2 is stupid and doesn't have escape characters, i'll use the greek question mark instead.
        	//	big brain.
        }
        logging::Info("Vote called to kick %s [U:1:%u] for %s", name, steamID,
                      reason);
        break;
    }
    case 47:
        logging::Info("Vote passed");
        if (was_local_player && requeue)
            tfmm::startQueue();
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
