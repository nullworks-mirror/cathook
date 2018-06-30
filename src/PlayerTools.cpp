/*
  Created on 23.06.18.
*/

#include <core/cvwrapper.hpp>
#include <unordered_map>
#include <hoovy.hpp>
#include <playerlist.hpp>
#include <online/Online.hpp>
#include "PlayerTools.hpp"
#include "entitycache.hpp"

static std::unordered_map<unsigned, unsigned> betrayal_list{};

static CatCommand forgive_all("pt_forgive_all", "Clear betrayal list",
                              []() { betrayal_list.clear(); });

namespace settings
{

static CatVar online_notarget(CV_SWITCH, "pt_ignore_notarget", "1",
                              "Ignore notarget",
                              "Ignore online players with notarget role");
static CatVar hoovy(CV_SWITCH, "pt_ignore_hoovy", "1", "Ignore hoovy");
static CatVar online_friendly_software(CV_SWITCH, "pt_ignore_friendly_software",
                                       "1", "Ignore friendly software",
                                       "Ignore CO-compatible software");
static CatVar online_only_verified(CV_SWITCH, "pt_ignore_only_verified", "0",
                                   "Only ignore verified",
                                   "If online checks are enabled, only apply "
                                   "ignore if SteamID is verified (not "
                                   "recommended right now)");
static CatVar online_anonymous(CV_SWITCH, "pt_ignore_anonymous", "1",
                               "Ignore anonymous",
                               "Apply ignore checks to anonymous accounts too");
static CatVar betrayal_limit(
    CV_INT, "pt_betrayal_limit", "3", "Betrayal limit",
    "Stop ignoring a player after N kills while you ignored them");
static CatVar taunting(CV_SWITCH, "pt_ignore_taunting", "1", "Ignore taunting",
                       "Don't shoot taunting players");
}

namespace player_tools
{

IgnoreReason shouldTargetSteamId(unsigned id)
{
    if (id == 0)
        return IgnoreReason::DO_NOT_IGNORE;

    if (settings::betrayal_limit)
    {
        if (betrayal_list[id] > int(settings::betrayal_limit))
            return IgnoreReason::DO_NOT_IGNORE;
    }

    auto &pl = playerlist::AccessData(id);
    if (playerlist::IsFriendly(pl.state))
        return IgnoreReason::LOCAL_PLAYER_LIST;

    auto *co = online::getUserData(id);
    if (co)
    {
        bool check_verified =
            !settings::online_only_verified || co->is_steamid_verified;
        bool check_anonymous = settings::online_anonymous || !co->is_anonymous;

        if (check_verified && check_anonymous)
        {
            if (settings::online_notarget && co->no_target)
                return IgnoreReason::ONLINE_NO_TARGET;
            if (settings::online_friendly_software &&
                co->is_using_friendly_software)
                return IgnoreReason::ONLINE_FRIENDLY_SOFTWARE;
        }
        // Always check developer status, no exceptions
        if (co->is_developer)
            return IgnoreReason::DEVELOPER;
    }

    return IgnoreReason::DO_NOT_IGNORE;
}
IgnoreReason shouldTarget(CachedEntity *entity)
{
    if (entity->m_Type() == ENTITY_PLAYER)
    {
        if (settings::hoovy && IsHoovy(entity))
            return IgnoreReason::IS_HOOVY;
        if (settings::taunting && HasCondition<TFCond_Taunting>(entity))
            return IgnoreReason::IS_TAUNTING;

        return shouldTargetSteamId(entity->player_info.friendsID);
    }

    return IgnoreReason::DO_NOT_IGNORE;
}

bool shouldAlwaysRenderEspSteamId(unsigned id)
{
    if (id == 0)
        return false;

    auto &pl = playerlist::AccessData(id);
    if (pl.state != playerlist::k_EState::DEFAULT)
        return true;

    auto *co = online::getUserData(id);
    if (co)
        return true;

    return false;
}
bool shouldAlwaysRenderEsp(CachedEntity *entity)
{
    if (entity->m_Type() == ENTITY_PLAYER)
    {
        return shouldAlwaysRenderEspSteamId(entity->player_info.friendsID);
    }

    return false;
}

#if ENABLE_VISUALS
std::optional<colors::rgba_t> forceEspColorSteamId(unsigned id)
{
    if (id == 0)
        return std::nullopt;

    auto pl = playerlist::Color(id);
    if (pl != colors::empty)
        return std::optional<colors::rgba_t>{ pl };

    auto *co = online::getUserData(id);
    if (co)
    {
        if (co->has_color)
            return std::optional<colors::rgba_t>{ co->color };
        if (co->rainbow)
            return std::optional<colors::rgba_t>{ colors::RainbowCurrent() };
    }

    return std::nullopt;
}
std::optional<colors::rgba_t> forceEspColor(CachedEntity *entity)
{
    if (entity->m_Type() == ENTITY_PLAYER)
    {
        return forceEspColorSteamId(entity->player_info.friendsID);
    }

    return std::nullopt;
}
#endif

void onKilledBy(unsigned id)
{
    auto reason = shouldTargetSteamId(id);
    if (reason != IgnoreReason::DO_NOT_IGNORE)
    {
        // We ignored the gamer, but they still shot us
        if (betrayal_list.find(id) == betrayal_list.end())
            betrayal_list[id] = 0;
        betrayal_list[id]++;
    }
}

void onKilledBy(CachedEntity *entity)
{
    onKilledBy(entity->player_info.friendsID);
}
}