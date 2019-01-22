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

        // info is the person getting kicked,
        // info2 is the person calling the kick.
        player_info_s info{}, info2{};
        if (!g_IEngine->GetPlayerInfo(eid, &info) || !g_IEngine->GetPlayerInfo(caller, &info2))
            break;

        logging::Info("Vote called to kick %s [U:1:%u] for %s by %s [U:1:%u]",
            info.name, info.friendsID, reason, info2.name, info2.friendsID);
        if (eid == LOCAL_E->m_IDX)
            was_local_player = true;

        if (*vote_kickn || *vote_kicky)
        {
            using namespace playerlist;

            auto &pl = AccessData(info.friendsID);
            auto &pl_caller = AccessData(info2.friendsID);
            bool friendly_kicked = pl.state != k_EState::RAGE && pl.state != k_EState::DEFAULT;
            bool friendly_caller = pl_caller.state != k_EState::RAGE && pl_caller.state != k_EState::DEFAULT;

            if (*vote_kickn && friendly_kicked)
            {
                g_IEngine->ClientCmd_Unrestricted("vote option2");
            }
            else if (*vote_kicky && !friendly_kicked)
                g_IEngine->ClientCmd_Unrestricted("vote option1");
        }
        if (*party_say)
        {
            char formated_string[256];
            std::snprintf(formated_string, sizeof(formated_string),
                "[CAT] votekick called: %s => %s (%s)",
                info2.name, info.name, reason);
            re::CTFPartyClient::GTFPartyClient()->SendPartyChat(formated_string);
        }
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
