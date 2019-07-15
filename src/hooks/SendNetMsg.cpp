/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <ucccccp.hpp>
#include <MiscTemporary.hpp>
#include <settings/Int.hpp>
#include "HookedMethods.hpp"
#include <MiscTemporary.hpp>
#include "irc.hpp"
#include "e8call.hpp"

static settings::Int newlines_msg{ "chat.prefix-newlines", "0" };
static settings::Boolean log_sent{ "debug.log-sent-chat", "false" };
extern settings::Boolean identify;
static Timer identify_timer{};

namespace hacks::shared::catbot
{
void SendNetMsg(INetMessage &msg);
}
namespace hooked_methods
{
std::vector<KeyValues *> Iterate(KeyValues *event, int depth)
{
    std::vector<KeyValues *> peer_list = { event };
    for (int i = 0; i < depth; i++)
    {
        for (auto ev : peer_list)
            for (KeyValues *dat2 = ev; dat2 != NULL; dat2 = dat2->m_pPeer)
                if (std::find(peer_list.begin(), peer_list.end(), dat2) == peer_list.end())
                    peer_list.push_back(dat2);
        for (auto ev : peer_list)
            for (KeyValues *dat2 = ev; dat2 != NULL; dat2 = dat2->m_pSub)
                if (std::find(peer_list.begin(), peer_list.end(), dat2) == peer_list.end())
                    peer_list.push_back(dat2);
    }
    return peer_list;
}

void ParseKeyValue(KeyValues *event)
{
    // loop through all our peers
    std::vector<KeyValues *> peer_list = Iterate(event, 10);
    logging::Info("Data for %s:", event->GetName());
    for (auto dat : peer_list)
    {
        auto name = dat->GetName();
        logging::Info("%s", name);
        switch (dat->m_iDataType)
        {
        case KeyValues::types_t::TYPE_NONE:
        {
            logging::Info("%s is typeless", name);
            break;
        }
        case KeyValues::types_t::TYPE_STRING:
        {
            if (dat->m_sValue && *(dat->m_sValue))
            {
                logging::Info("%s is String: %s", name, dat->m_sValue);
            }
            else
            {
                logging::Info("%s is String: %s", name, "");
            }
            break;
        }
        case KeyValues::types_t::TYPE_WSTRING:
        {
            break;
        }

        case KeyValues::types_t::TYPE_INT:
        {
            logging::Info("%s is int: %d", name, dat->m_iValue);
            break;
        }

        case KeyValues::types_t::TYPE_UINT64:
        {
            logging::Info("%s is double: %f", name, *(double *) dat->m_sValue);
            break;
        }

        case KeyValues::types_t::TYPE_FLOAT:
        {
            logging::Info("%s is float: %f", name, dat->m_flValue);
            break;
        }
        case KeyValues::types_t::TYPE_COLOR:
        {
            logging::Info("%s is Color: { %u %u %u %u}", name, dat->m_Color[0], dat->m_Color[1], dat->m_Color[2], dat->m_Color[3]);
            break;
        }
        case KeyValues::types_t::TYPE_PTR:
        {
            logging::Info("%s is Pointer: %x", name, dat->m_pValue);
            break;
        }

        default:
            break;
        }
    }
}

static InitRoutine run_identify([]() {
    EC::Register(
        EC::CreateMove,
        []() {
            // 5 minutes between each identify seems ok?
            if (!*identify || CE_BAD(LOCAL_E) || !identify_timer.test_and_set(1000 * 60 * 5))
                return;
            chat_stack::Say(ucccccp::encrypt("meow", 'B'));
        },
        "sendnetmsg_createmove");
});

DEFINE_HOOKED_METHOD(SendNetMsg, bool, INetChannel *this_, INetMessage &msg, bool force_reliable, bool voice)
{
    if (!isHackActive())
        return original::SendNetMsg(this_, msg, force_reliable, voice);
    size_t say_idx, say_team_idx;
    int offset;
    std::string newlines{};
    NET_StringCmd stringcmd;
    // net_StringCmd
    if (msg.GetType() == 4 && (newlines_msg || crypt_chat))
    {
        std::string str(msg.ToString());
        say_idx      = str.find("net_StringCmd: \"say \"");
        say_team_idx = str.find("net_StringCmd: \"say_team \"");
        if (!say_idx || !say_team_idx)
        {
            offset    = say_idx ? 26 : 21;
            bool crpt = false;

            // Only allow !! and !!! if crypto_chat is on
            if (crypt_chat)
            {
                // Artifical Scope
                {
                    std::string msg(str.substr(offset));
                    msg = msg.substr(0, msg.length() - 2);
                    if (msg.find("!!!") == 0 || msg.find("!!") == 0)
                    {
                        int sub_val = 2;
                        if (msg.find("!!!") == 0)
                            sub_val = 3;
                        if (ucccccp::decrypt(msg.substr(sub_val)) != "Unsupported version" && ucccccp::decrypt(msg.substr(sub_val)) != "Attempt at ucccccping and failing")
                        {
                            // Message is sent over IRC.
                            std::string substrmsg(msg.substr(sub_val));
#if ENABLE_IRC
                            IRC::sendmsg(substrmsg, true);
#endif
                            // Do not send message over normal chat.
                            return false;
                        }
                    }
                }
            }
            if (!crpt && *newlines_msg > 0)
            {
                // TODO move out? update in a value change callback?
                newlines = std::string(*newlines_msg, '\n');
                str.insert(offset, newlines);
            }
            str = str.substr(16, str.length() - 17);
            // if (queue_messages && !chat_stack::CanSend()) {
            stringcmd.m_szCommand = str.c_str();
            return original::SendNetMsg(this_, stringcmd, force_reliable, voice);
            //}
        }
    }
    static ConVar *sv_player_usercommand_timeout = g_ICvar->FindVar("sv_player_usercommand_timeout");
    static float lastcmd                         = 0.0f;
    if (lastcmd > g_GlobalVars->absoluteframetime)
    {
        lastcmd = g_GlobalVars->absoluteframetime;
    }
    // SoonTM
    /*
    if (msg.GetType() == 16)
    {
        std::string message(msg.GetName());
        if (message.find("MVM_Upgrade"))
        {
            CLC_CmdKeyValues keyval = *(CLC_CmdKeyValues *) (&msg);
            KeyValues *kv           = keyval.kv;
            while (kv)
            {
                if (kv->GetInt("count", 1333) != 1333)
                {
                    logging::Info("Upgrade: %d", kv->GetInt("upgrade"));
                    logging::Info("Itemslot: %d", kv->GetInt("itemslot"));
                    logging::Info("Count: %d", kv->GetInt("count"));
                    kv->SetInt("free", 1);
                }
                kv = kv->GetNextKey();
            }
        }
    }*/
    if (!strcmp(msg.GetName(), "clc_CmdKeyValues"))
        hacks::shared::catbot::SendNetMsg(msg);
    if (log_sent && msg.GetType() != 3 && msg.GetType() != 9)
    {
        if (!strcmp(msg.GetName(), "clc_CmdKeyValues"))
            if ((KeyValues *) (((unsigned *) &msg)[4]))
                ParseKeyValue((KeyValues *) (((unsigned *) &msg)[4]));
        logging::Info("=> %s [%i] %s", msg.GetName(), msg.GetType(), msg.ToString());
        unsigned char buf[4096];
        bf_write buffer("cathook_debug_buffer", buf, 4096);
        logging::Info("Writing %i", msg.WriteToBuffer(buffer));
        std::string bytes    = "";
        constexpr char h2c[] = "0123456789abcdef";
        for (int i = 0; i < buffer.GetNumBytesWritten(); i++)
        {
            bytes += format(h2c[(buf[i] & 0xF0) >> 4], h2c[(buf[i] & 0xF)], ' ');
            // bytes += format((unsigned short) buf[i], ' ');
        }
        logging::Info("%i bytes => %s", buffer.GetNumBytesWritten(), bytes.c_str());
    }
    return original::SendNetMsg(this_, msg, force_reliable, voice);
}
} // namespace hooked_methods
