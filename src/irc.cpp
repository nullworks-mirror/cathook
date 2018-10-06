#include "common.hpp"
#include <thread>
#include "IRCClient.h"
#include <atomic>
#include "irc.hpp"
#include "ucccccp.hpp"
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
void updateData(std::string commandandcontrol)
{
    std::string nick = g_ISteamFriends->GetPersonaName();   
    irc.UpdateData(nick, nick, *channel, commandandcontrol, *address, *port);
}
static HookedFunction paint(HookedFunctions_types::HF_Paint, "IRC", 16,
                            []() { irc.Update(); });

template<typename T>
void rvarCallback(settings::VariableBase<T> &var, T after)
{
    irc.Disconnect();
    updateData("");
    if (enabled)
        irc.Connect();
}

void rvarCallback_enabled(settings::VariableBase<bool> &var, bool after)
{
    irc.Disconnect();
    if (after)
        irc.Connect();
}

static InitRoutine init([]() {
    updateData("");
    enabled.installChangeCallback(rvarCallback_enabled);
    anon.installChangeCallback(rvarCallback<bool>);
    authenticate.installChangeCallback(rvarCallback<bool>);
    channel.installChangeCallback(rvarCallback<std::string>);
    address.installChangeCallback(rvarCallback<std::string>);
    port.installChangeCallback(rvarCallback<int>);

    irc.Connect();
});

static CatCommand send("irc_send", "Send message to IRC",
                       [](const CCommand &args) { irc.privmsg(args.ArgS()); });

} // namespace IRC
