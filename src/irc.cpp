#include "common.hpp"
#include <thread>
#include "IRCClient.h"
#include <atomic>
#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"
#include "irc.hpp"
#include "ucccccp.hpp"
#include "Settings.hpp"

namespace IRC
{
static std::atomic<bool> thread_shouldrun;
static std::atomic<bool> thread_running;
static settings::Bool enabled("irc.enabled", "true");
static settings::Bool anon("irc.anon", "true");
static settings::Bool authenticate("irc.auth", "true");
// To prevent possible crashes when IRC shuts down while sending msg
static std::mutex IRCConnection_mutex;
bool shouldauth;

struct IRCData
{
    std::string address                      = "cathook.irc.inkcat.net";
    int port                                 = 8080;
    std::string channel                      = "#cat_comms";
    std::string username                     = "You";
    std::string nickname                     = "You";
    std::unique_ptr<IRCClient> IRCConnection = nullptr;
};

static IRCData IRCData;

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
    if (!*anon)
    {
        IRCData.username = g_ISteamFriends->GetPersonaName();
        IRCData.nickname = g_ISteamFriends->GetPersonaName();
        IRCData.nickname.append(format("-", randomint()));
    }
    else
    {
        IRCData.username = "Anon";
        IRCData.nickname = "Anon";
        IRCData.nickname.append(format("-", randomint()));
    }
    std::replace(IRCData.nickname.begin(), IRCData.nickname.end(), ' ', '_');
    std::replace(IRCData.username.begin(), IRCData.username.end(), ' ', '_');

    IRCConnection_mutex.lock();
    IRCData.IRCConnection = std::make_unique<IRCClient>();
    IRCClient &client     = *IRCData.IRCConnection;
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
                    if (IRCData.IRCConnection &&
                        IRCData.IRCConnection->Connected())
                        IRCData.IRCConnection->SendIRC(
                            format("JOIN ", IRCData.channel));
                    logging::Info("IRC: Init complete.");
                });
                joinChannel.detach();
                IRCConnection_mutex.unlock();
                while (client.Connected() && thread_shouldrun.load())
                    client.ReceiveData();
            }
            else
                IRCConnection_mutex.unlock();
        }
        else
            IRCConnection_mutex.unlock();
    }
    else
        IRCConnection_mutex.unlock();
    logging::Info("Exiting IRC thread...");
    // Lock mutex twice twice tf
    IRCConnection_mutex.lock();
    IRCData.IRCConnection->SendIRC("QUIT");
    IRCData.IRCConnection->Disconnect();
    IRCData.IRCConnection.reset();
    thread_running.store(false);
    IRCConnection_mutex.unlock();
}

static Timer IRCRetry{};

static HookedFunction pt(HF_Paint, "IRC", 16, []() {
    if (!*enabled)
    {
        thread_shouldrun.store(false);
        return;
    }
    if (!thread_running.load() && IRCRetry.test_and_set(10000))
    {
        thread_shouldrun.store(true);
        thread_running.store(true);
        std::thread ircthread(IRCThread);
        ircthread.detach();
        logging::Info("IRC: Running.");
    }
});

bool sendraw(std::string &msg)
{
    std::lock_guard<std::mutex> lock(IRCConnection_mutex);
    if (IRCData.IRCConnection && IRCData.IRCConnection->Connected())
    {
        if (IRCData.IRCConnection->SendIRC(format(
                "PRIVMSG ", IRCData.channel, " :", ucccccp::encrypt(msg, 'B'))))
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
    sendraw(msg);
}

static CatCommand irc_send_cmd("irc_send_cmd", "Send cmd to IRC",
                               [](const CCommand &args) {
                                   IRCData.IRCConnection->SendIRC(args.ArgS());
                               });

static CatCommand irc_send("irc_send", "Send message to IRC",
                           [](const CCommand &args) {
                               std::string msg(args.ArgS());
                               sendmsg(msg, true);
                           });

static CatCommand irc_auth("irc_auth",
                           "Auth via IRC (Find users on same server)",
                           [](const CCommand &args) { auth(); });
static CatCommand irc_disconnect(
    "irc_disconnect",
    "Disconnect from IRC (Warning you might automatically reconnect)",
    [](const CCommand &args) {
        std::lock_guard<std::mutex> lock(IRCConnection_mutex);
        if (IRCData.IRCConnection)
        {
            // IRCData.IRCConnection->SendIRC("QUIT");
            IRCData.IRCConnection->Disconnect();
        }
    });

void rvarCallback(settings::VariableBase<bool> &var, bool after)
{
    std::lock_guard<std::mutex> lock(IRCConnection_mutex);
    if (IRCData.IRCConnection)
    {
        IRCData.IRCConnection->SendIRC("QUIT");
        IRCData.IRCConnection->Disconnect();
    }
}

static InitRoutine init([]() {
    enabled.installChangeCallback(rvarCallback);
    anon.installChangeCallback(rvarCallback);
});
} // namespace IRC
