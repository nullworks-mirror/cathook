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

template<typename T>
void SplitName(std::vector<T> &ret, const T &name, int num)
{
    T tmp;
    int chars = 0;
    for (char i : name)
    {
        if (i == ' ')
            continue;

        tmp.push_back(std::tolower(i));
        ++chars;
        if (chars == num + 1) {
            chars = 0;
            ret.push_back(tmp);
            tmp.clear();
        }
    }
    if (tmp.size() > 2)
        ret.push_back(tmp);
}

DEFINE_HOOKED_METHOD(DispatchUserMessage, bool, void *this_, int type,
                     bf_read &buf)
{
    if (!isHackActive())
        return original::DispatchUserMessage(this_, type, buf);

    int s, i, j;
    char c;
    const char *buf_data = reinterpret_cast<const char *>(buf.m_pData);

    /* Delayed print of name and message, censored by chat_filter
     * TO DO: Document type 47
     */
    if (retrun && type != 47 && gitgud.test_and_set(300))
    {
        PrintChat("\x07%06X%s\x01: %s", 0xe05938, lastname.c_str(),
                  lastfilter.c_str());
        retrun = false;
    }
    std::string data;
    switch (type) {

    case 12:
        if (hacks::shared::catbot::anti_motd && hacks::shared::catbot::catbotmode)
        {
            data = std::string(buf_data);
            if (data.find("class_") != data.npos)
                return false;
        }
        break;
    case 5:
        if (*anti_votekick && buf.GetNumBytesLeft() > 35)
        {
            data = std::string(buf_data);
            logging::Info("%s", data.c_str());
            if (data.find("TeamChangeP") != data.npos && CE_GOOD(LOCAL_E))
                g_IEngine->ClientCmd_Unrestricted("cat_disconnect;wait 100;cat_mm_join");
            buf.Seek(0);
        }
        break;
    case 4:
        s = buf.GetNumBytesLeft();
        if (s >= 256)
            break;

        for (i = 0; i < s; i++) {
            c = buf_data[i];
            if (clean_chat && i > 1)
                if (c == '\n' || c == '\r')
                    continue;

            data.push_back(c);
        }
        /* First byte is player ENT index
         * Second byte is unindentified (equals to 0x01)
         */
        const char *p = data.c_str() + 2;
        std::string event(p), name((p += event.size() + 1)), message(p + name.size() + 1);
        if (chat_filter_enable && data[0] == LOCAL_E->m_IDX &&
            event == "#TF_Name_Change")
        {
            chat_stack::Say("." + clear, false);
        }
        else if (chat_filter_enable && data[0] != LOCAL_E->m_IDX &&
            event.find("TF_Chat") == 0)
        {
            player_info_s info{};
            g_IEngine->GetPlayerInfo(LOCAL_E->m_IDX, &info);
            std::string name1 = info.name, claz;

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
                "hax",  "vac",    "ban",   "bot",  "report", "kick",
                claz
            };
            SplitName(res, name1, 2);
            SplitName(res, name1, 3);

            std::string message2(message);
            boost::to_lower(message2);

            const char *toreplace[] = { " ", "4", "3", "0", "6", "5", "7" };
            const char *replacewith[] = { "",  "a", "e", "o", "g", "s", "t" };

            for (int i = 0; i < 7; i++)
                boost::replace_all(message2, toreplace[i], replacewith[i]);

            for (auto filter : res)
                if (boost::contains(message2, filter))
                {
                    chat_stack::Say("." + clear, true);
                    retrun     = true;
                    lastfilter = message;
                    lastname   = format(name);
                    gitgud.update();
                    break;
                }
        }
        if (crypt_chat && message.find("!!B") == 0 && ucccccp::validate(message))
        {
            std::string msg = ucccccp::decrypt(message);
#if !LAGBOT_MODE
            CachedEntity *ent = ENTITY(data[0]);
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
        chatlog::LogMessage(data[0], message);
        buf = bf_read(data.c_str(), data.size());
        buf.Seek(0);
        break;

    }
    if (dispatch_log)
    {
        logging::Info("D> %i", type);
        std::ostringstream str;
        while (buf.GetNumBytesLeft())
            str << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(buf.ReadByte()) << ' ';

        std::string msg(str.str());
        logging::Info("MESSAGE %d, DATA = [ %s ] strings listed below", type, msg.c_str());
        buf.Seek(0);

        i = 0;
        msg.clear();
        while (buf.GetNumBytesLeft()) {
            if ((c = buf.ReadByte()))
                msg.push_back(c);
            else {
                logging::Info("[%d] %s", i++, msg.c_str());
                msg.clear();
            }
        }
        buf.Seek(0);
    }
    votelogger::dispatchUserMessage(buf, type);
    return original::DispatchUserMessage(this_, type, buf);
}
} // namespace hooked_methods
