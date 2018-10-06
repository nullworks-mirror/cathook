#include "common.hpp"
#include <thread>
#include <atomic>
#include "irc.hpp"
#include "Settings.hpp"
#include "ChIRC.hpp"
#include <random>

namespace IRC
{
static settings::Bool enabled("irc.enabled", "true");
static settings::Bool anon("irc.anon", "true");
static settings::Bool authenticate("irc.auth", "true");
static settings::String channel("irc.channel", "#cat_comms");
static settings::String address("irc.address", "cathook.irc.inkcat.net");
static settings::Int port("irc.port", "8080");

static ChIRC::ChIRC irc;

void printmsg(std::string &usr, std::string &msg)
{
    if (msg.size() > 256 || usr.size() > 256)
    {
        logging::Info("IRC: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        logging::Info("[IRC] %s: %s", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[IRC] %s\x01: %s", 0xe05938, usr.c_str(),
                  msg.c_str());
}
void printmsgcopy(std::string usr, std::string msg)
{
    if (msg.size() > 256 || usr.size() > 256)
    {
        logging::Info("IRC: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        logging::Info("[IRC] %s: %s", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[IRC] %s\x01: %s", 0xe05938, usr.c_str(),
                  msg.c_str());
}

void handleMessage(IRCMessage message, IRCClient *client)
{
    std::string &cmd     = message.command;
    std::string &channel = message.parameters.at(0);
    std::string &rawmsg  = message.parameters.at(1);
    std::string &usr     = message.prefix.nick;
    if (!ucccccp::validate(rawmsg))
        return;
    std::string msg(ucccccp::decrypt(rawmsg));
    if (msg == "Attempt at ucccccping and failing" ||
        msg == "Unsupported version")
        return;

    // Handle privmsg (Message to #channel)
    if (cmd == "PRIVMSG")
    {
        if (msg.empty() || usr.empty())
            return;
        // Handle messages
        if (msg.find("msg") == 0)
        {
            std::string toprint = msg.substr(3);
            if (toprint.empty())
                return;
            printmsg(usr, toprint);
            return;
        }

        // Handle auth requests
        if (msg.find("auth") == 0)
        {
            // Check if we are in a game
            if (g_Settings.bInvalid)
                return;
            bool isreply = false;
            std::string steamidhash;
            if (msg.find("authrep") == 0)
                isreply = true;
            // Get steamid hash from string
            if (isreply)
                steamidhash = msg.substr(7);
            else
                steamidhash = msg.substr(4);

            for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
            {
                if (i == g_pLocalPlayer->entity_idx)
                    continue;
                player_info_s pinfo;
                // Get playerinfo and check if player on server
                if (!g_IEngine->GetPlayerInfo(i, &pinfo))
                    continue;
                auto tarsteamid = pinfo.friendsID;
                MD5Value_t result;
                // Hash steamid
                MD5_ProcessSingleBuffer(&tarsteamid, sizeof(tarsteamid),
                                        result);
                // Get bits of hash and store in string
                std::string tarhash;
                for (auto i : result.bits)
                {
                    for (int j = 0; j < 8; j++)
                        tarhash.append(std::to_string((i >> j) & 1));
                }
                // Check if steamid of sender == steamid we currently check
                // (using hashes)
                if (tarhash == steamidhash)
                {
                    // Use actual steamid to set cat status
                    auto &playerlistdata = playerlist::AccessData(tarsteamid);
                    if (playerlistdata.state == playerlist::k_EState::DEFAULT)
                    {
                        playerlistdata.state = playerlist::k_EState::CAT;
                    }
                    // Avoid replying to a reply
                    if (isreply)
                        // We are done here. Steamid duplicates don't exist.
                        return;
                    // If message is not a reply, reply.
                    auth(true);
                    // We are done here. Steamid duplicates don't exist.
                    return;
                }
            }
        }
    }
}

void updateData(std::string commandandcontrol)
{
    std::string nick("Anon");
    if (!*anon)
        nick = g_ISteamFriends->GetPersonaName();
    irc.UpdateData(nick, nick, *channel, commandandcontrol, *address, *port);
}

bool sendmsg(std::string &msg, bool loopback)
{
    std::string raw = "msg" + msg;
    if (irc.privmsg(raw))
    {
        if (loopback)
        {
            printmsgcopy(irc.getData().nick, msg);
        }
        return true;
    }
    if (loopback)
        printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

void auth(bool reply)
{
    if (g_Settings.bInvalid && !g_Settings.is_create_move)
        return;
    if (!*authenticate)
        return;
    MD5Value_t result;
    MD5_ProcessSingleBuffer(&LOCAL_E->player_info.friendsID, sizeof(uint32),
                            result);
    std::string msg = "auth";
    if (reply)
        msg.append("rep");
    for (auto i : result.bits)
    {
        for (int j = 0; j < 8; j++)
            msg.append(std::to_string((i >> j) & 1));
    }
    irc.privmsg(msg);
}

static bool restarting{ false };

static HookedFunction paint(HookedFunctions_types::HF_Paint, "IRC", 16, []() {
    if (!restarting)
        irc.Update();
});

template <typename T> void rvarCallback(settings::VariableBase<T> &var, T after)
{
    if (!restarting)
    {
        restarting = true;
        std::thread reload([]() {
            std::this_thread::sleep_for(
                std::chrono_literals::operator""ms(500));
            irc.Disconnect();
            updateData("");
            if (enabled)
                irc.Connect();
            restarting = false;
        });
        reload.detach();
    }
}

static InitRoutine init([]() {
    updateData("");
    enabled.installChangeCallback(rvarCallback<bool>);
    anon.installChangeCallback(rvarCallback<bool>);
    authenticate.installChangeCallback(rvarCallback<bool>);
    channel.installChangeCallback(rvarCallback<std::string>);
    address.installChangeCallback(rvarCallback<std::string>);
    port.installChangeCallback(rvarCallback<int>);

    irc.installCallback("PRIVMSG", handleMessage);
    irc.Connect();
});

static CatCommand irc_send_cmd("irc_send_cmd", "Send cmd to IRC",
                               [](const CCommand &args) {
                                   irc.sendraw(args.ArgS());
                               });

static CatCommand irc_send("irc_send", "Send message to IRC",
                           [](const CCommand &args) {
                               std::string msg(args.ArgS());
                               sendmsg(msg, true);
                           });

static CatCommand irc_auth("irc_auth",
                           "Auth via IRC (Find users on same server)",
                           []() { auth(); });

} // namespace IRC
