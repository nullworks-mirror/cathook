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
std::atomic<bool> thread_shouldrun;
std::atomic<bool> thread_running;
std::unique_ptr<IRCClient> IRCConnection;
Timer heartbeat{};
settings::Bool enabled("irc.enabled", "true");

void handleMessage(IRCMessage message, IRCClient *client)
{
    std::string &cmd     = message.command;
    std::string &channel = message.parameters.at(0);
    std::string &msg     = message.parameters.at(1);
    std::string &usr     = message.prefix.nick;
    if (msg.empty() || usr.empty())
        return;
    logging::Info("passed");
    if (g_Settings.bInvalid)
        logging::Info("[IRC] %s: %s", usr.c_str(), msg.c_str());
    else
        PrintChat("\x07%06X[IRC] %s\x01: %s", 0xe05938, usr.c_str(),
                  msg.c_str());
}

int randomint() {
      boost::mt19937 rng;
      boost::uniform_int<> one_to_six( 1, 10000 );
      boost::variate_generator< boost::mt19937, boost::uniform_int<> >
                    dice(rng, one_to_six);
      return dice();
}

void IRCThread()
{
    char address[] = "cathook.irc.inkcat.net";
    int port       = 8080;
    std::atomic<bool> thread_shouldrun;
    std::string user = g_ISteamFriends->GetPersonaName();
//    user.append(format("-", getpid()));
    std::string nick = g_ISteamFriends->GetPersonaName();
    nick.append(format("-", randomint()));
    std::string channel("#cat_talk");

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

namespace irc
{
bool sendmsg(std::string msg, std::string = "pixelz")
{
    if (!IRCConnection || IRCConnection->Connected())
    {
        if (IRCConnection->SendIRC(format("PRIVMSG #pixelz :", msg)))
        {
            ;
            return true;
        }
    }
    return false;
}
} // namespace irc

CatCommand irc_send("irc_send", "Send message to IRC",
                    [](const CCommand &args) {
                        IRCConnection->SendIRC(args.ArgS());
                    });

CatCommand irc_send_msg("irc_send_msg", "Send message to IRC",
                        [](const CCommand &args) {
                            if (irc::sendmsg(args.ArgS()))
                                logging::Info("IRC: Sent!");
                            else
                                logging::Info("IRC: Error!");
                        });
}
