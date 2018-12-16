/*
 * DominateSay.cpp
 *
 *	Created on: October 30, 2018
 */

#include <hacks/DominateSay.hpp>
#include <settings/Int.hpp>
#include "common.hpp"

static settings::Int dominatesay_mode{ "dominatesay.mode", "0" };
static settings::String filename{ "dominatesay.file", "dominatesay.txt" };

static CatCommand reload_command("dominatesay_reload", "Reload dominatesays", []() { hacks::shared::dominatesay::reload(); });

namespace hacks::shared::dominatesay
{

const std::string tf_classes_dominatesay[] = { "class", "scout", "sniper", "soldier", "demoman", "medic", "heavy", "pyro", "spy", "engineer" };

const std::string tf_teams_dominatesay[] = { "RED", "BLU" };

static std::string lastmsg{};

TextFile file{};

std::string ComposeDominateSay(IGameEvent *event)
{
    const std::vector<std::string> *source = nullptr;
    switch (*dominatesay_mode)
    {
    case 1:
        source = &file.lines;
        break;
    case 2:
        source = &builtin_default;
        break;
    case 3:
        source = &jp_anime;
        break;
    default:
        break;
    }
    if (!source || source->empty())
        return "";
    if (!event)
        return "";
    int vid  = event->GetInt("dominated");
    int kid  = event->GetInt("dominator");
    int dnum = event->GetInt("dominations");

    //	this is actually impossible but just in case.
    if (g_IEngine->GetPlayerForUserID(kid) != g_IEngine->GetLocalPlayer())
        return "";

    std::string msg = source->at(rand() % source->size());

    while (msg == lastmsg && source->size() > 1)
        msg = source->at(rand() % source->size());
    lastmsg = msg;
    player_info_s info{};

    g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(vid), &info);
    ReplaceSpecials(msg);

    CachedEntity *ent = ENTITY(g_IEngine->GetPlayerForUserID(vid));
    int clz           = g_pPlayerResource->GetClass(ent);

    ReplaceString(msg, "%class%", tf_classes_dominatesay[clz]);
    player_info_s infok{};
    g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(kid), &infok);

    ReplaceString(msg, "%dominum%", std::to_string(dnum));
    ReplaceString(msg, "%killer%", std::string(infok.name));
    ReplaceString(msg, "%team%", tf_teams_dominatesay[ent->m_iTeam() - 2]);
    ReplaceString(msg, "%myteam%", tf_teams_dominatesay[LOCAL_E->m_iTeam() - 2]);
    ReplaceString(msg, "%myclass%", tf_classes_dominatesay[g_pPlayerResource->GetClass(LOCAL_E)]);
    ReplaceString(msg, "%name%", std::string(info.name));
    return msg;
}

class DominateSayEventListener : public IGameEventListener2
{
    void FireGameEvent(IGameEvent *event) override
    {
        if (!dominatesay_mode)
            return;
        std::string message = hacks::shared::dominatesay::ComposeDominateSay(event);
        if (!message.empty())
            chat_stack::Say(message, false);
    }
};

static DominateSayEventListener listener{};

void reload()
{
    file.Load(*filename);
}

void init()
{
    g_IEventManager2->AddListener(&listener, (const char *) "player_domination", false);
}

void shutdown()
{
    g_IEventManager2->RemoveListener(&listener);
}

//	a much better default dominatesay would be appreciated.
const std::vector<std::string> builtin_default = {
    "dominating %name%! (%dominum% dominations)",
    "%name%, getting tapped?",
    "%killer% is dominating the server with %dominum% dominations!",
};

//	same goes to this one
const std::vector<std::string> jp_anime = {
    "Get d-dominated %name%-senpai >:3",
    "g- gomenasai! %name%-san!",
    "Wow! hey hey hey H~ ey !!! I found you again~~!",
    "%name%-san please don't get mad at me.. ><",
    //	https://youtu.be/sW3RT0tF020?t=207
    "kore kara mo douzo yoroshiku ne.",
    "konna watashi dakedo waratte yurushite ne.",
    "zutto taisetsu ni shite ne.",
    "eikyuu hoshou no watashi dakara.",
};

} // namespace hacks::shared::dominatesay
