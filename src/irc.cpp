#include "common.hpp"
#include "core/logging.hpp"
#include <thread>
//#include "Thread.h"
#include "IRCClient.h"
#include <atomic>
#include "boost/random.hpp"
#include "boost/generator_iterator.hpp"

namespace IRC
{
static std::atomic<bool> thread_shouldrun;
static std::atomic<bool> thread_running;
static std::unique_ptr<IRCClient> IRCConnection;
static Timer heartbeat{};
static settings::Bool enabled("irc.enabled", "true");

static char address[] = "cathook.irc.inkcat.net";
static int port       = 8080;
static std::string channel("#cat_comms");

void handleMessage(IRCMessage message, IRCClient *client)
{
    std::string &cmd     = message.command;
    std::string &channel = message.parameters.at(0);
    std::string &msg     = message.parameters.at(1);
    std::string &usr     = message.prefix.nick;
    if (msg.empty() || usr.empty())
        return;
    if (g_Settings.bInvalid)
        logging::Info("[IRC] %s: %s", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[IRC] %s\x01: %s", 0xe05938, usr.c_str(),
                  msg.c_str());
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
    std::string user = g_ISteamFriends->GetPersonaName();
    std::string nick = g_ISteamFriends->GetPersonaName();
    nick.append(format("-", randomint()));

    IRCConnection     = std::make_unique<IRCClient>();
    IRCClient &client = *IRCConnection;
    client.HookIRCCommand("PRIVMSG", handleMessage);

    logging::Info("IRC: Initializing IRC");
    if (client.InitSocket())
    {
        logging::Info("IRC: Socket Initialized. Connecting to cathook IRC...");
        if (client.Connect(address, port))
        {
            logging::Info("IRC: Connected. Logging in...");
            if (client.Login(nick, user))
            {
                logging::Info("IRC: Logged in. Joining channel.");
                std::thread joinChannel([=]() {
                    std::this_thread::sleep_for(
                        std::chrono_literals::operator""s(3));
                    if (IRCConnection && IRCConnection->Connected())
                        IRCConnection->SendIRC(format("JOIN ", channel));
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
    thread_shouldrun.store(true);
    if (!thread_running.load())
    {
        thread_running.store(true);
        heartbeat.update();
        std::thread ircthread(IRCThread);
        ircthread.detach();
        logging::Info("IRC: Running.");
    }
    heartbeat.update();
});

bool sendmsg(std::string msg)
{
    if (IRCConnection && IRCConnection->Connected())
    {
        if (IRCConnection->SendIRC(format("PRIVMSG ", channel, " :", msg)))
        {
            return true;
        }
    }
    return false;
}

CatCommand irc_send_cmd("irc_send_cmd", "Send message to IRC",
                        [](const CCommand &args) {
                            IRCConnection->SendIRC(args.ArgS());
                            if (g_Settings.bInvalid)
                                logging::Info("[IRC] You: %s", args.ArgS());
                            else
                                PrintChat("\x07%06X[IRC] You\x01: %s", 0xe05938, args.ArgS());
                        });

CatCommand irc_send("irc_send", "Send message to IRC",
                    [](const CCommand &args) {
                        if (sendmsg(args.ArgS()))
                            logging::Info("IRC: Sent!");
                        else
                            logging::Info("IRC: Error!");
                    });
} // namespace IRC
