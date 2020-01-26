#include "config.h"
#if ENABLE_NULLNEXUS
#include "common.hpp"
#include "libnullnexus/nullnexus.hpp"
#include "nullnexus.hpp"
#if ENABLE_VISUALS
#include "colors.hpp"
#include "MiscTemporary.hpp"
#endif

namespace nullnexus
{
static settings::Boolean enabled("nullnexus.enabled", "true");
static settings::Boolean anon("nullnexus.user.anon", "false");
static settings::String address("nullnexus.host", "localhost");
static settings::String port("nullnexus.port", "3000");
static settings::String endpoint("nullnexus.endpoint", "/client/v1");
#if ENABLE_VISUALS
static settings::Rgba colour("nullnexus.user.colour");
#endif

static NullNexus nexus;

void printmsg(std::string &usr, std::string &msg, int colour = 0xff9340)
{
#if !ENFORCE_STREAM_SAFETY && ENABLE_VISUALS
    if (msg.size() > 128 || usr.size() > 32)
    {
        logging::Info("Nullnexus: Message too large.");
        return;
    }
    if (g_Settings.bInvalid)
        g_ICvar->ConsoleColorPrintf(MENU_COLOR, "[Nullnexus] %s: %s\n", usr.c_str(), msg.c_str());
    else
        PrintChat(false, "\x07%06X[\x07%06XNullnexus\x07%06X] \x07%06X%s\x01: %s", 0x5e3252, 0xba3d9a, 0x5e3252, colour, usr.c_str(), msg.c_str());
#endif
}
void printmsgcopy(std::string usr, std::string msg)
{
    printmsg(usr, msg);
}

namespace handlers
{
void message(std::string usr, std::string msg, int colour)
{
    printmsg(usr, msg, colour);
}
} // namespace handlers

void updateData()
{
    std::optional<std::string> username = std::nullopt;
    std::optional<int> newcolour        = std::nullopt;
    if (!*anon)
        username = g_ISteamFriends->GetPersonaName();
    if ((*colour).r || (*colour).g || (*colour).b)
    {
        int r     = (*colour).r * 255;
        int g     = (*colour).g * 255;
        int b     = (*colour).b * 255;
        newcolour = (r << 16) + (g << 8) + b;
    }
    nexus.changeSettings(username, newcolour);
}

bool sendmsg(std::string &msg)
{
    if (nexus.sendChat(msg))
        return true;
    printmsgcopy("Cathook", "Error! Couldn't send message.");
    return false;
}

template <typename T> void rvarCallback(settings::VariableBase<T> &, T)
{
    std::thread reload([]() {
        std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
        updateData();
        nexus.connect(*enabled, *address, *port, *endpoint);
    });
    reload.detach();
}

template <typename T> void rvarDataCallback(settings::VariableBase<T> &, T)
{
    std::thread reload([]() {
        std::this_thread::sleep_for(std::chrono_literals::operator""ms(500));
        updateData();
    });
    reload.detach();
}

static InitRoutine init([]() {
    updateData();
    enabled.installChangeCallback(rvarCallback<bool>);
    address.installChangeCallback(rvarCallback<std::string>);
    port.installChangeCallback(rvarCallback<std::string>);
    endpoint.installChangeCallback(rvarCallback<std::string>);

    anon.installChangeCallback(rvarDataCallback<bool>);
    colour.installChangeCallback(rvarDataCallback<rgba_t>);

    updateData();
    nexus.setHandlerChat(handlers::message);
    nexus.connect(*enabled, *address, *port, *endpoint);

    EC::Register(
        EC::Shutdown, []() { nexus.connect(false); }, "shutdown_nullnexus");
});
static CatCommand nullnexus_send("nullnexus_send", "Send message to IRC", [](const CCommand &args) {
    std::string msg(args.ArgS());
    sendmsg(msg);
});
} // namespace nullnexus
#endif
