/*
  Created on 29.07.18.
*/
#include "config.h"
#include <EventLogging.hpp>
#include <settings/Bool.hpp>
#include <helpers.hpp>
#if ENABLE_VISUALS
#include <colors.hpp>
#include <init.hpp>
#include "KeyValues.h"

static settings::Bool enable{ "chat.log-events", "false" };
static settings::Bool event_hurt{ "chat.log-events.hurt", "false" };
static settings::Bool event_connect{ "chat.log-events.joining", "true" };
static settings::Bool event_activate{ "chat.log-events.connect", "true" };
static settings::Bool event_disconnect{ "chat.log-events.disconnect", "true" };
static settings::Bool event_team{ "chat.log-events.team", "true" };
static settings::Bool event_death{ "chat.log-events.death", "true" };
static settings::Bool event_spawn{ "chat.log-events.spawn", "true" };
static settings::Bool event_changeclass{ "chat.log-events.changeclass", "true" };
static settings::Bool event_vote{ "chat.log-events.vote", "false" };
static settings::Bool debug_events{ "debug.log-events", "false" };

static void handlePlayerConnectClient(KeyValues *kv)
{
    PrintChat("\x07%06X%s\x01 \x07%06X%s\x01 joining", 0xa06ba0, kv->GetString("name"), 0x914e65, kv->GetString("networkid"));
}

static void handlePlayerActivate(KeyValues *kv)
{
    int uid    = kv->GetInt("userid");
    int entity = g_IEngine->GetPlayerForUserID(uid);
    player_info_s info{};
    if (g_IEngine->GetPlayerInfo(entity, &info))
        PrintChat("\x07%06X%s\x01 connected", 0xa06ba0, info.name);
}

static void handlePlayerDisconnect(KeyValues *kv)
{
    CachedEntity *player = ENTITY(g_IEngine->GetPlayerForUserID(kv->GetInt("userid")));
    if (player == nullptr || RAW_ENT(player) == nullptr)
        return;
    PrintChat("\x07%06X%s\x01 \x07%06X%s\x01 disconnected", colors::chat::team(player->m_iTeam()), kv->GetString("name"), 0x914e65, kv->GetString("networkid"));
}

static void handlePlayerTeam(KeyValues *kv)
{
    if (kv->GetBool("disconnect"))
        return;

    int oteam           = kv->GetInt("oldteam");
    int nteam           = kv->GetInt("team");
    const char *oteam_s = teamname(oteam);
    const char *nteam_s = teamname(nteam);
    PrintChat("\x07%06X%s\x01 changed team (\x07%06X%s\x01 -> "
              "\x07%06X%s\x01)",
              0xa06ba0, kv->GetString("name"), colors::chat::team(oteam), oteam_s, colors::chat::team(nteam), nteam_s);
}

static void handlePlayerHurt(KeyValues *kv)
{
    int victim   = kv->GetInt("userid");
    int attacker = kv->GetInt("attacker");
    int health   = kv->GetInt("health");
    player_info_s kinfo{};
    player_info_s vinfo{};
    if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(victim), &vinfo) || !g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(attacker), &kinfo))
        return;
    CachedEntity *vic = ENTITY(g_IEngine->GetPlayerForUserID(victim));
    CachedEntity *att = ENTITY(g_IEngine->GetPlayerForUserID(attacker));

    if (vic == nullptr || att == nullptr || RAW_ENT(vic) == nullptr || RAW_ENT(att) == nullptr)
        return;

    PrintChat("\x07%06X%s\x01 hurt \x07%06X%s\x01 down to \x07%06X%d\x01hp", colors::chat::team(att->m_iTeam()), kinfo.name, colors::chat::team(vic->m_iTeam()), vinfo.name, 0x2aaf18, health);
}

static void handlePlayerDeath(KeyValues *kv)
{
    int victim   = kv->GetInt("userid");
    int attacker = kv->GetInt("attacker");
    player_info_s kinfo{};
    player_info_s vinfo{};
    if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(victim), &vinfo) || !g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(attacker), &kinfo))
        return;
    CachedEntity *vic = ENTITY(g_IEngine->GetPlayerForUserID(victim));
    CachedEntity *att = ENTITY(g_IEngine->GetPlayerForUserID(attacker));

    if (vic == nullptr || att == nullptr || RAW_ENT(vic) == nullptr || RAW_ENT(att) == nullptr)
        return;

    PrintChat("\x07%06X%s\x01 killed \x07%06X%s\x01", colors::chat::team(att->m_iTeam()), kinfo.name, colors::chat::team(vic->m_iTeam()), vinfo.name);
}

static void handlePlayerSpawn(KeyValues *kv)
{
    int id = kv->GetInt("userid");
    player_info_s info{};
    if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(id), &info))
        return;
    CachedEntity *player = ENTITY(g_IEngine->GetPlayerForUserID(id));
    if (player == nullptr || RAW_ENT(player) == nullptr)
        return;
    PrintChat("\x07%06X%s\x01 (re)spawned", colors::chat::team(player->m_iTeam()), info.name);
}

static void handlePlayerChangeClass(KeyValues *kv)
{
    int id = kv->GetInt("userid");
    if (id > 33 || id < 0)
        return;
    player_info_s info{};
    if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(id), &info))
        return;
    CachedEntity *player = ENTITY(g_IEngine->GetPlayerForUserID(id));
    if (player == nullptr || RAW_ENT(player) == nullptr)
        return;
    PrintChat("\x07%06X%s\x01 changed to \x07%06X%s\x01", colors::chat::team(player->m_iTeam()), info.name, 0xa06ba0, classname(kv->GetInt("class")));
}

static void handleVoteCast(KeyValues *kv)
{
    int vote_option = kv->GetInt("vote_option");
    int team        = kv->GetInt("team");
    int idx         = kv->GetInt("entityid");
    player_info_s info{};
    const char *team_s = teamname(team);
    if (g_IEngine->GetPlayerInfo(idx, &info))
        PrintChat("\x07%06X%s\x01 Voted \x07%06X%d\x01 on team \x07%06X%s\x01", colors::chat::team(team), info.name, colors::chat::team(team), vote_option, colors::chat::team(team), team_s);
}

class LoggingEventListener : public IGameEventListener
{
public:
    void FireGameEvent(KeyValues *event) override
    {
        if (debug_events)
        {
            // loop through all our peers
            for (KeyValues *dat2 = event; dat2 != NULL; dat2 = dat2->m_pPeer)
            {
                for (KeyValues *dat = dat2; dat != NULL; dat = dat->m_pSub)
                {
                    auto data_type = dat->m_iDataType;
                    auto name      = dat->GetName();
                    logging::Info("%s: %u", name, data_type);
                    switch (dat->m_iDataType)
                    {
                    case KeyValues::types_t::TYPE_NONE:
                    {
                        logging::Info("KeyValue is typeless");
                        break;
                    }
                    case KeyValues::types_t::TYPE_STRING:
                    {
                        if (dat->m_sValue && *(dat->m_sValue))
                        {
                            logging::Info("KeyValue is String: %s", dat->m_sValue);
                        }
                        else
                        {
                            logging::Info("KeyValue is String: %s", "");
                        }
                        break;
                    }
                    case KeyValues::types_t::TYPE_WSTRING:
                    {
                        Assert(!"TYPE_WSTRING");
                        break;
                    }

                    case KeyValues::types_t::TYPE_INT:
                    {
                        logging::Info("KeyValue is int: %d", dat->m_iValue);
                        break;
                    }

                    case KeyValues::types_t::TYPE_UINT64:
                    {
                        logging::Info("KeyValue is double: %f", *(double *) dat->m_sValue);
                        break;
                    }

                    case KeyValues::types_t::TYPE_FLOAT:
                    {
                        logging::Info("KeyValue is float: %f", dat->m_flValue);
                        break;
                    }
                    case KeyValues::types_t::TYPE_COLOR:
                    {
                        logging::Info("KeyValue is Color: { %u %u %u %u}", dat->m_Color[0], dat->m_Color[1], dat->m_Color[2], dat->m_Color[3]);
                        break;
                    }
                    case KeyValues::types_t::TYPE_PTR:
                    {
                        logging::Info("KeyValue is Pointer: %x", dat->m_pValue);
                        break;
                    }

                    default:
                        break;
                    }
                }
            }
        }
        if (!enable)
            return;
        const char *name = event->GetName();
        if (!strcmp(name, "player_connect_client") && event_connect)
            handlePlayerConnectClient(event);
        else if (!strcmp(name, "player_activate") && event_activate)
            handlePlayerActivate(event);
        else if (!strcmp(name, "player_disconnect") && event_disconnect)
            handlePlayerDisconnect(event);
        else if (!strcmp(name, "player_team") && event_team)
            handlePlayerTeam(event);
        else if (!strcmp(name, "player_hurt") && event_hurt)
            handlePlayerHurt(event);
        else if (!strcmp(name, "player_death") && event_death)
            handlePlayerDeath(event);
        else if (!strcmp(name, "player_spawn") && event_spawn)
            handlePlayerSpawn(event);
        else if (!strcmp(name, "player_changeclass") && event_changeclass)
            handlePlayerChangeClass(event);
        else if (!strcmp(name, "vote_cast") && event_vote)
            handleVoteCast(event);
    }
};

static LoggingEventListener listener{};

InitRoutine init([]() { g_IGameEventManager->AddListener(&listener, false); });

bool event_logging::isEnabled()
{
    return *enable;
}
#endif
