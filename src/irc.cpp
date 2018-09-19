#include "common.hpp"
#include "core/logging.hpp"
#include <thread>
#include "IRCClient.h"
#include <atomic>
#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"
#include "checksum_md5.h"
#include "irc.hpp"

namespace IRC
{
static std::atomic<bool> thread_shouldrun;
static std::atomic<bool> thread_running;
static std::unique_ptr<IRCClient> IRCConnection;
static Timer heartbeat{};
static settings::Bool enabled("irc.enabled", "true");

struct IRCData
{
    std::string address  = "cathook.irc.inkcat.net";
    int port             = 8080;
    std::string channel  = "#cat_comms";
    std::string username = "You";
    std::string nickname = "You";
};

IRCData IRCData;

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
    std::string &msg     = message.parameters.at(1);
    std::string &usr     = message.prefix.nick;
    // Handle privmsg (Message to #channel)
    if (cmd == "PRIVMSG")
    {
        if (msg.empty() || usr.empty())
            return;
        // Handle messages
        if (msg.find("msg") == 0)
        {
            printmsgcopy(usr, msg.substr(3));
            return;
        }

        // Handle auth requests
        if (msg.find("auth") == 0)
        {
            // Check if we are in a game
            if (g_Settings.bInvalid)
                return;
            bool isreply;
            std::string steamidhash;
            if (msg.find("authrep"))
                isreply = true;
            if (isreply)
                steamidhash = msg.substr(7);
            else
                steamidhash = msg.substr(4);
            // Get steamid hash from string
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
                logging::Info("%s | %s", steamidhash.c_str(), tarhash.c_str());
                if (tarhash == steamidhash)
                {
                    logging::Info("Match found!");
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
                logging::Info("No match!");
            }
        }
    }
}

int randomint()
{
    boost::mt19937 rng{ static_cast<std::uint32_t>(std::time(nullptr)) };
    boost::uniform_int<> oneto10000(1, 10000);
    boost::variate_generator<boost::mt19937, boost::uniform_int<> > dice(
        rng, oneto10000);
    return dice();
}

void IRCThread()
{
    IRCData.username = g_ISteamFriends->GetPersonaName();
    IRCData.nickname = g_ISteamFriends->GetPersonaName();
    IRCData.nickname.append(format("-", randomint()));
    std::replace(IRCData.nickname.begin(), IRCData.nickname.end(), ' ', '_');
    std::replace(IRCData.username.begin(), IRCData.username.end(), ' ', '_');

    IRCConnection     = std::make_unique<IRCClient>();
    IRCClient &client = *IRCConnection;
    client.HookIRCCommand("PRIVMSG", handleMessage);

    logging::Info("IRC: Initializing IRC");
    if (client.InitSocket())
    {
        logging::Info("IRC: Socket Initialized. Connecting to cathook IRC...");
        if (client.Connect(IRCData.address.c_str(), IRCData.port))
        {
            logging::Info("IRC: Connected. Logging in...");
            if (client.Login(IRCData.nickname, IRCData.username))
            {
                logging::Info("IRC: Logged in. Joining channel.");
                std::thread joinChannel([=]() {
                    std::this_thread::sleep_for(
                        std::chrono_literals::operator""s(3));
                    if (IRCConnection && IRCConnection->Connected())
                        IRCConnection->SendIRC(
                            format("JOIN ", IRCData.channel));
                    logging::Info("IRC: Init complete.");
                });
                joinChannel.detach();
                while (client.Connected() && thread_shouldrun.load())
                {
                    if (heartbeat.check(5000))
                    {
                        thread_shouldrun.store(false);
                        break;
                    }
                    client.ReceiveData();
                }
            }
        }
    }
    logging::Info("Exiting IRC thread...");
    IRCConnection->SendIRC("QUIT");
    IRCConnection->Disconnect();
    IRCConnection.reset();
    thread_running.store(false);
}

static HookedFunction pt(HF_Draw, "IRC", 16, []() {
    if (!*enabled)
    {
        thread_shouldrun.store(false);
        return;
    }
    if (!thread_running.load())
    {
        thread_shouldrun.store(true);
        thread_running.store(true);
        heartbeat.update();
        std::thread ircthread(IRCThread);
        ircthread.detach();
        logging::Info("IRC: Running.");
    }
    heartbeat.update();
});

bool sendraw(std::string &msg)
{
    if (IRCConnection && IRCConnection->Connected())
    {
        if (IRCConnection->SendIRC(
                format("PRIVMSG ", IRCData.channel, " :", msg)))
            return true;
    }
    return false;
}

bool sendmsg(std::string &msg, bool loopback)
{
    std::string raw = "msg" + msg;
    if (sendraw(raw))
    {
        if (loopback)
        {
            printmsg(IRCData.nickname, msg);
        }
        return true;
    }
    if (loopback)
        printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

void auth(bool reply)
{
    if (g_Settings.bInvalid)
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
    sendraw(msg);
}

CatCommand irc_send_cmd("irc_send_cmd", "Send cmd to IRC",
                        [](const CCommand &args) {
                            IRCConnection->SendIRC(args.ArgS());
                        });

CatCommand irc_send("irc_send", "Send message to IRC",
                    [](const CCommand &args) {
                        std::string msg(args.ArgS());
                        sendmsg(msg, true);
                    });

CatCommand irc_auth("irc_auth", "Send message to IRC",
                    [](const CCommand &args) { auth(); });
} // namespace IRC
