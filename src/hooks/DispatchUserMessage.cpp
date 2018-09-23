/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <chatlog.hpp>
#include <ucccccp.hpp>
#include <boost/algorithm/string.hpp>
#include <MiscTemporary.hpp>
#include <hacks/AntiAim.hpp>
#include <settings/Bool.hpp>
#include "HookedMethods.hpp"

static settings::Bool clean_chat{ "chat.clean", "false" };
static settings::Bool dispatch_log{ "debug.log-dispatch-user-msg", "false" };
static settings::Bool chat_filter_enable{ "chat.censor.enable", "false" };
static settings::Bool identify{ "chat.identify", "false" };
static settings::Bool answerIdentify{ "chat.identify.answer", "false" };

static bool retrun = false;
static Timer sendmsg{};
static Timer gitgud{};

std::string clear( 200, '\n' );
std::string lastfilter{};
std::string lastname{};

namespace hooked_methods
{
std::vector<std::string> SplitName(std::string name, int num)
{
    std::string tmp{};
    std::vector<std::string> name2{};
    int chars = 0;
    for (char i : name)
    {
        if (i == ' ')
            continue;
        if (chars == num)
        {
            chars = 0;
            tmp += i;
            name2.push_back(tmp);
            tmp = "";
        }
        else if (chars < num)
        {
            chars++;
            tmp += i;
        }
    }
    if (tmp.size() > 2)
        name2.push_back(tmp);
    for (auto &i : name2)
        boost::to_lower(i);
    return name2;
}
DEFINE_HOOKED_METHOD(DispatchUserMessage, bool, void *this_, int type,
                     bf_read &buf)
{
    if (!isHackActive())
        return original::DispatchUserMessage(this_, type, buf);
    if (retrun && gitgud.test_and_set(300))
    {
        PrintChat("\x07%06X%s\x01: %s", 0xe05938, lastname.c_str(),
                  lastfilter.c_str());
        retrun = false;
    }
    int loop_index, s, i, j;
    char *data, c;
    if (type == 5)
        if (buf.GetNumBytesLeft() > 35)
        {
            std::string message_name{};
            for (int i = 0; i < buf.GetNumBytesLeft(); i++)
            {
                int byte = buf.ReadByte();
                if (byte == 0)
                    break;
                message_name.push_back(byte);
            }
            if (message_name.find("TF_Autobalance_TeamChangePending") !=
                std::string::npos)
                logging::Info("test, %d %d", int(message_name[0]),
                              (CE_GOOD(LOCAL_E) ? LOCAL_E->m_IDX : -1));
        }
    if (type == 4)
    {
        loop_index = 0;
        s          = buf.GetNumBytesLeft();
        if (s < 256)
        {
            data = (char *) alloca(s);
            for (i = 0; i < s; i++)
                data[i] = buf.ReadByte();
            j = 0;
            std::string name{};
            std::string message{};
            for (i = 0; i < 3; i++)
            {
                int starcount = 0;
                while ((c = data[j++]) && (loop_index < s))
                {
                    loop_index++;
                    if (clean_chat)
                        if ((c == '\n' || c == '\r') && (i == 1 || i == 2))
                        {
                            data[j - 1] = '*';
                            starcount++;
                        }
                    if (i == 1)
                        name.push_back(c);
                    if (i == 2)
                        message.push_back(c);
                }
            }
            if (chat_filter_enable && data[0] != LOCAL_E->m_IDX)
            {
                player_info_s info{};
                g_IEngine->GetPlayerInfo(LOCAL_E->m_IDX, &info);
                std::string name1 = info.name;
                std::vector<std::string> name2{};
                std::string claz{};

                switch (g_pLocalPlayer->clazz)
                {
                case tf_scout:
                    claz = "scout";
                    break;
                case tf_soldier:
                    claz = "soldier";
                    break;
                case tf_pyro:
                    claz = "pyro";
                    break;
                case tf_demoman:
                    claz = "demo";
                    break;
                case tf_engineer:
                    claz = "engi";
                    break;
                case tf_heavy:
                    claz = "heavy";
                    break;
                case tf_medic:
                    claz = "med";
                    break;
                case tf_sniper:
                    claz = "sniper";
                    break;
                case tf_spy:
                    claz = "spy";
                    break;
                default:
                    break;
                }

                std::vector<std::string> res = {
                    "skid", "script", "cheat", "hak",   "hac",  "f1",
                    "hax",  "vac",    "ban",   "lmao",  "bot",  "report",
                    "cat",  "insta",  "revv",  "brass", "kick", claz
                };
                name2 = SplitName(name1, 2);
                for (auto i : name2)
                    res.push_back(i);
                name2 = SplitName(name1, 3);
                for (auto i : name2)
                    res.push_back(i);
                std::string message2 = message;
                std::vector<std::string> toreplace{ " ", "4", "3", "0",
                                                    "6", "5", "7" };
                std::vector<std::string> replacewith{ "",  "a", "e", "o",
                                                      "g", "s", "t" };
                boost::to_lower(message2);

                for (int i = 0; i < toreplace.size(); i++)
                    boost::replace_all(message2, toreplace[i], replacewith[i]);
                bool filtered = false;
                for (auto filter : res)
                    if (boost::contains(message2, filter) && !filtered)
                    {
                        filtered = true;
                        chat_stack::Say(". " + clear, true);
                        retrun     = true;
                        lastfilter = format(filter);
                        lastname   = format(name);
                        gitgud.update();
                    }
            }
#if !LAGBOT_MODE
            if (*identify && sendmsg.test_and_set(300000))
                chat_stack::Say("!!meow");
#endif
            if (crypt_chat)
            {
                if (message.find("!!B") == 0)
                {
                    if (ucccccp::validate(message))
                    {
                        std::string msg = ucccccp::decrypt(message);
#if !LAGBOT_MODE
                        //                        if (ucccccp::decrypt(message)
                        //                        == "meow" &&
                        //                            hacks::shared::antiaim::communicate
                        //                            && data[0] !=
                        //                            LOCAL_E->m_IDX &&
                        //                            playerlist::AccessData(ENTITY(data[0])).state
                        //                            !=
                        //                                playerlist::k_EState::CAT)
                        //                        {
                        //                            playerlist::AccessData(ENTITY(data[0])).state
                        //                            =
                        //                                playerlist::k_EState::CAT;
                        //                            chat_stack::Say("!!meow");
                        //                        }
                        CachedEntity *ent = ENTITY(data[0]);
                        if (msg != "Attempt at ucccccping and failing" &&
                            msg != "Unsupported version" && ent != LOCAL_E)
                        {
                            auto &state = playerlist::AccessData(ent).state;
                            if (state == playerlist::k_EState::DEFAULT)
                            {
                                state = playerlist::k_EState::CAT;
                                if (*answerIdentify &&
                                    sendmsg.test_and_set(5000))
                                    chat_stack::Say("!!meow");
                            }
                            else if (state == playerlist::k_EState::CAT)
                            {
                                if (*answerIdentify &&
                                    sendmsg.test_and_set(60000))
                                    chat_stack::Say("!!meow");
                            }
                        }
#endif
                        PrintChat("\x07%06X%s\x01: %s", 0xe05938, name.c_str(),
                                  msg.c_str());
                    }
                }
            }
            chatlog::LogMessage(data[0], message);
            buf = bf_read(data, s);
            buf.Seek(0);
        }
    }
    if (dispatch_log)
    {
        logging::Info("D> %i", type);
        std::ostringstream str{};
        while (buf.GetNumBytesLeft())
        {
            unsigned char byte = buf.ReadByte();
            str << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(byte) << ' ';
        }
        logging::Info("MESSAGE %d, DATA = [ %s ]", type, str.str().c_str());
        buf.Seek(0);
    }
    votelogger::dispatchUserMessage(buf, type);
    return original::DispatchUserMessage(this_, type, buf);
}
} // namespace hooked_methods
