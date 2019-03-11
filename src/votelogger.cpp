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
static settings::Bool vote_rage_vote{ "votelogger.autovote.no.rage", "false" };
static settings::Bool party_say{ "votelogger.partysay", "true" };

namespace votelogger
{

static bool was_local_player{ false };

static void vote_rage_back()
{
    static Timer attempt_vote_time;
    char cmd[40];
    player_info_s info;
    std::vector<int> targets;

    if (!g_IEngine->IsInGame() || !attempt_vote_time.test_and_set(1000))
        return;

    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        auto ent = ENTITY(i);
        // TO DO: m_bEnemy check only when you can't vote off players from the opposite team
        if (CE_BAD(ent) || ent == LOCAL_E || ent->m_Type() != ENTITY_PLAYER || ent->m_bEnemy())
            continue;

        if (!g_IEngine->GetPlayerInfo(ent->m_IDX, &info))
            continue;

        auto &pl = playerlist::AccessData(info.friendsID);
        if (pl.state == playerlist::k_EState::RAGE)
            targets.emplace_back(info.userID);
    }
    if (targets.empty())
        return;

    std::snprintf(cmd, sizeof(cmd), "callvote kick \"%d cheating\"", targets[UniformRandomInt(0, targets.size() - 1)]);
    g_IEngine->ClientCmd_Unrestricted(cmd);
}

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

        logging::Info("Vote called to kick %s [U:1:%u] for %s by %s [U:1:%u]", info.name, info.friendsID, reason, info2.name, info2.friendsID);
        if (eid == LOCAL_E->m_IDX)
            was_local_player = true;

        if (*vote_kickn || *vote_kicky)
        {
            using namespace playerlist;

            auto &pl             = AccessData(info.friendsID);
            auto &pl_caller      = AccessData(info2.friendsID);
            bool friendly_kicked = pl.state != k_EState::RAGE && pl.state != k_EState::DEFAULT;
            bool friendly_caller = pl_caller.state != k_EState::RAGE && pl_caller.state != k_EState::DEFAULT;

            if (*vote_kickn && friendly_kicked)
            {
                g_IEngine->ClientCmd_Unrestricted("vote option2");
                if (*vote_rage_vote && !friendly_caller)
                    pl_caller.state = k_EState::RAGE;
            }
            else if (*vote_kicky && !friendly_kicked)
                g_IEngine->ClientCmd_Unrestricted("vote option1");
        }
        if (*party_say)
        {
            char formated_string[256];
            std::snprintf(formated_string, sizeof(formated_string), "[CAT] votekick called: %s => %s (%s)", info2.name, info.name, reason);
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

static void setup_vote_rage()
{
    EC::Register(EC::CreateMove, vote_rage_back, "vote_rage_back");
}

static void reset_vote_rage()
{
    EC::Unregister(EC::CreateMove, "vote_rage_back");
}

static InitRoutine init([]() {
    if (*vote_rage_vote)
        setup_vote_rage();

    vote_rage_vote.installChangeCallback([](settings::VariableBase<bool> &var, bool new_val) {
        if (new_val)
            setup_vote_rage();
        else
            reset_vote_rage();
    });
});
} // namespace votelogger
