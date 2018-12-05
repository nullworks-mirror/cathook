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
#include "CatBot.hpp"

static settings::Bool clean_chat{ "chat.clean", "false" };
static settings::Bool dispatch_log{ "debug.log-dispatch-user-msg", "false" };
static settings::Bool chat_filter_enable{ "chat.censor.enable", "false" };
static settings::Bool identify{ "chat.identify", "false" };
static settings::Bool answerIdentify{ "chat.identify.answer", "false" };
static settings::Bool anti_votekick{ "cat-bot.anti-autobalance", "false" };

static bool retrun = false;
static Timer sendmsg{};
static Timer gitgud{};

// Using repeated char causes crash on some systems. Suboptimal solution.
const static std::string clear(
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
    "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
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

    if (type == 12 && hacks::shared::catbot::anti_motd && hacks::shared::catbot::catbotmode)
    {
        std::string message_name;
        while (buf.GetNumBytesLeft())
            message_name.push_back(buf.ReadByte());
        buf.Seek(0);
        if (message_name.find("class_") != message_name.npos)
            return false;
    }
    if (retrun && type != 47 && gitgud.test_and_set(300))
    {
        PrintChat("\x07%06X%s\x01: %s", 0xe05938, lastname.c_str(),
                  lastfilter.c_str());
        retrun = false;
    }
    int loop_index, s, i, j;
    char c;
    if (type == 5 && *anti_votekick)
        if (buf.GetNumBytesLeft() > 35)
        {
            std::string message_name;
            while (buf.GetNumBytesLeft())
                message_name.push_back(buf.ReadByte());
            logging::Info("%s", message_name.c_str());
            if (message_name.find("TeamChangeP") != message_name.npos && CE_GOOD(LOCAL_E))
                g_IEngine->ClientCmd_Unrestricted("cat_disconnect;wait 100;cat_mm_join");
            buf.Seek(0);
        }

    std::string cleaned_data;
    if (type == 4)
    {
        loop_index = 0;
        s          = buf.GetNumBytesLeft();
        if (s < 256)
        {
            std::string data;
            for (i = 0; i < s; i++)
            {
                char to_app = buf.ReadByte();
                data.push_back(to_app);
            }
            data = data.substr(0, data.size()-1);
            for (auto i : data)
            {
                if (clean_chat)
                {
                    if ((i == '\n' || i == '\r'))
                    {

                    }
                    else
                        cleaned_data.push_back(i);
                }
            }
            if (!*clean_chat)
                cleaned_data = data;
            j = 0;
            std::string name{};
            std::string message{};
            for (i = 0; i < 3; i++)
            {
                while ((c = cleaned_data[j++]) && (loop_index < s) && (loop_index < cleaned_data.size()))
                {
                    loop_index++;
                    if (i == 1)
                        name.push_back(c);
                    if (i == 2)
                        message.push_back(c);
                }
            }
            if (chat_filter_enable && cleaned_data[0] != LOCAL_E->m_IDX)
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
                        chat_stack::Say("." + clear, true);
                        retrun     = true;
                        lastfilter = message;
                        lastname   = format(name);
                        gitgud.update();
                    }
            }
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
                        CachedEntity *ent = ENTITY(cleaned_data[0]);
                        if (msg != "Attempt at ucccccping and failing" &&
                            msg != "Unsupported version" && ent != LOCAL_E)
                        {
                            auto &state = playerlist::AccessData(ent).state;
                            if (state == playerlist::k_EState::DEFAULT)
                            {
                                state = playerlist::k_EState::CAT;
                            }
                        }
#endif
                        PrintChat("\x07%06X%s\x01: %s", 0xe05938, name.c_str(),
                                  msg.c_str());
                    }
                }
            }
            chatlog::LogMessage(cleaned_data[0], message);
            buf = bf_read(cleaned_data.c_str(), cleaned_data.size());
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
