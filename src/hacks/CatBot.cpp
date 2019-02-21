/*
 * CatBot.cpp
 *
 *  Created on: Dec 30, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "CatBot.hpp"
#include "common.hpp"
#include "hack.hpp"
#include "PlayerTools.hpp"

static settings::Bool auto_disguise{ "misc.autodisguise", "true" };

static settings::Int abandon_if_bots_gte{ "cat-bot.abandon-if.bots-gte", "0" };
static settings::Int abandon_if_ipc_bots_gte{ "cat-bot.abandon-if.ipc-bots-gte", "0" };
static settings::Int abandon_if_humans_lte{ "cat-bot.abandon-if.humans-lte", "0" };
static settings::Int abandon_if_players_lte{ "cat-bot.abandon-if.players-lte", "0" };
static settings::Int mark_human_threshold{ "cat-bot.mark-human-after-kills", "2" };

static settings::Bool micspam{ "cat-bot.micspam.enable", "false" };
static settings::Int micspam_on{ "cat-bot.micspam.interval-on", "3" };
static settings::Int micspam_off{ "cat-bot.micspam.interval-off", "60" };

static settings::Bool auto_crouch{ "cat-bot.auto-crouch", "false" };
static settings::Bool always_crouch{ "cat-bot.always-crouch", "false" };
static settings::Bool random_votekicks{ "cat-bot.votekicks", "false" };
static settings::Bool autoReport{ "cat-bot.autoreport", "true" };

namespace hacks::shared::catbot
{
settings::Bool catbotmode{ "cat-bot.enable", "false" };
settings::Bool anti_motd{ "cat-bot.anti-motd", "false" };

struct catbot_user_state
{
    int treacherous_kills{ 0 };
};

static std::unordered_map<unsigned, catbot_user_state> human_detecting_map{};

bool is_a_catbot(unsigned steamID)
{
    auto it = human_detecting_map.find(steamID);
    if (it == human_detecting_map.end())
        return false;

    // if (!(*it).second.has_bot_name)
    //	return false;

    if ((*it).second.treacherous_kills <= int(mark_human_threshold))
    {
        return true;
    }

    return false;
}

void on_killed_by(int userid)
{
    CachedEntity *player = ENTITY(g_IEngine->GetPlayerForUserID(userid));

    if (CE_BAD(player))
        return;

    unsigned steamID = player->player_info.friendsID;

    if (human_detecting_map.find(steamID) == human_detecting_map.end())
        return;

    // if (human_detecting_map[steamID].has_bot_name)
    human_detecting_map[steamID].treacherous_kills++;
    logging::Info("Treacherous kill #%d: %s [U:1:%u]", human_detecting_map[steamID].treacherous_kills, player->player_info.name, player->player_info.friendsID);
}

void do_random_votekick()
{
    std::vector<int> targets;
    player_info_s local_info;

    if (CE_BAD(LOCAL_E) || !g_IEngine->GetPlayerInfo(LOCAL_E->m_IDX, &local_info))
        return;
    for (int i = 1; i <= g_GlobalVars->maxClients; ++i)
    {
        player_info_s info;
        if (!g_IEngine->GetPlayerInfo(i, &info) || !info.friendsID)
            continue;
        if (g_pPlayerResource->GetTeam(i) != g_pLocalPlayer->team)
            continue;
        if (is_a_catbot(info.friendsID))
            continue;
        if (info.friendsID == local_info.friendsID)
            continue;
        auto &pl = playerlist::AccessData(info.friendsID);
        if (pl.state != playerlist::k_EState::RAGE && pl.state != playerlist::k_EState::DEFAULT)
            continue;

        targets.push_back(info.userID);
    }

    if (targets.empty())
        return;

    int target = targets[rand() % targets.size()];
    player_info_s info;
    if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(target), &info))
        return;
    hack::ExecuteCommand("callvote kick \"" + std::to_string(target) + " cheating\"");
}

void update_catbot_list()
{
    for (int i = 1; i < g_GlobalVars->maxClients; ++i)
    {
        player_info_s info;
        if (!g_IEngine->GetPlayerInfo(i, &info))
            continue;

        info.name[31] = 0;
        if (strcasestr(info.name, "cat-bot") || strcasestr(info.name, "just disable vac tf") || strcasestr(info.name, "raul.garcia") || strcasestr(info.name, "zCat") || strcasestr(info.name, "lagger bot") || strcasestr(info.name, "zLag-bot") || strcasestr(info.name, "crash-bot") || strcasestr(info.name, "reichstagbot"))
        {
            if (human_detecting_map.find(info.friendsID) == human_detecting_map.end())
            {
                logging::Info("Found bot %s [U:1:%u]", info.name, info.friendsID);
                human_detecting_map.insert(std::make_pair(info.friendsID, catbot_user_state{ 0 }));
            }
        }
    }
}

class CatBotEventListener : public IGameEventListener2
{
    void FireGameEvent(IGameEvent *event) override
    {
        if (!catbotmode)
            return;

        int killer_id = g_IEngine->GetPlayerForUserID(event->GetInt("attacker"));
        int victim_id = g_IEngine->GetPlayerForUserID(event->GetInt("userid"));

        if (victim_id == g_IEngine->GetLocalPlayer())
        {
            on_killed_by(killer_id);
            return;
        }
    }
};

CatBotEventListener &listener()
{
    static CatBotEventListener object{};
    return object;
}

static Timer timer_votekicks{};
static Timer timer_catbot_list{};
static Timer timer_abandon{};

int count_bots{ 0 };

bool should_ignore_player(CachedEntity *player)
{
    if (CE_BAD(player))
        return false;

    return is_a_catbot(player->player_info.friendsID);
}

#if ENABLE_IPC
void update_ipc_data(ipc::user_data_s &data)
{
    data.ingame.bot_count = count_bots;
}
#endif

Timer level_init_timer{};

Timer micspam_on_timer{};
Timer micspam_off_timer{};
static bool patched_report;
static std::atomic_bool can_report = false;
static std::vector<unsigned> to_report;
void reportall()
{
    can_report = false;
    if (!patched_report)
    {
        static BytePatch patch(gSignatures.GetClientSignature, "73 ? 80 7D ? ? 74 ? F3 0F 10 0D", 0x2F, { 0x89, 0xe0 });
        patch.Patch();
        patched_report = true;
    }
    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        // We only want a nullptr check since dormant entities are still on the
        // server
        if (!ent)
            continue;

        // Pointer comparison is fine
        if (ent == LOCAL_E)
            continue;
        player_info_s info;
        if (g_IEngine->GetPlayerInfo(i, &info) && info.friendsID)
        {
            if (!player_tools::shouldTargetSteamId(info.friendsID))
                continue;
            to_report.push_back(info.friendsID);
        }
    }
    can_report = true;
}

CatCommand report("report_all", "Report all players", []() { reportall(); });
CatCommand report_uid("report_steamid", "Report with steamid", [](const CCommand &args) {
    if (args.ArgC() < 1)
        return;
    unsigned steamid = 0;
    try
    {
        steamid = std::stoi(args.Arg(1));
    }
    catch (std::invalid_argument)
    {
        logging::Info("Report machine broke");
        return;
    }
    if (!steamid)
    {
        logging::Info("Report machine broke");
        return;
    }
    typedef uint64_t (*ReportPlayer_t)(uint64_t, int);
    static uintptr_t addr1                = gSignatures.GetClientSignature("55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
    static ReportPlayer_t ReportPlayer_fn = ReportPlayer_t(addr1);
    if (!addr1)
        return;
    CSteamID id(steamid, EUniverse::k_EUniversePublic, EAccountType::k_EAccountTypeIndividual);
    ReportPlayer_fn(id.ConvertToUint64(), 1);
});

Timer crouchcdr{};
void smart_crouch()
{
    if (g_Settings.bInvalid)
        return;
    if (!current_user_cmd)
        return;
    if (*always_crouch)
    {
        current_user_cmd->buttons |= IN_DUCK;
        if (crouchcdr.test_and_set(10000))
            current_user_cmd->buttons &= ~IN_DUCK;
        return;
    }
    bool foundtar      = false;
    static bool crouch = false;
    if (crouchcdr.test_and_set(2000))
    {
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            auto ent = ENTITY(i);
            if (CE_BAD(ent) || ent->m_Type() != ENTITY_PLAYER || ent->m_iTeam() == LOCAL_E->m_iTeam() || !(ent->hitboxes.GetHitbox(0)) || !(ent->m_bAlivePlayer()) || !player_tools::shouldTarget(ent) || should_ignore_player(ent))
                continue;
            bool failedvis = false;
            for (int j = 0; j < 18; j++)
                if (IsVectorVisible(g_pLocalPlayer->v_Eye, ent->hitboxes.GetHitbox(j)->center))
                    failedvis = true;
            if (failedvis)
                continue;
            for (int j = 0; j < 18; j++)
            {
                if (!LOCAL_E->hitboxes.GetHitbox(j))
                    continue;
                // Check if they see my hitboxes
                if (!IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->center) && !IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->min) && !IsVectorVisible(ent->hitboxes.GetHitbox(0)->center, LOCAL_E->hitboxes.GetHitbox(j)->max))
                    continue;
                foundtar = true;
                crouch   = true;
            }
        }
        if (!foundtar && crouch)
            crouch = false;
    }
    if (crouch)
        current_user_cmd->buttons |= IN_DUCK;
}

CatCommand print_ammo("debug_print_ammo", "debug", []() {
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer() || CE_BAD(LOCAL_W))
        return;
    logging::Info("Current slot: %d", re::C_BaseCombatWeapon::GetSlot(RAW_ENT(LOCAL_W)));
    for (int i = 0; i < 10; i++)
        logging::Info("Ammo Table %d: %d", i, CE_INT(LOCAL_E, netvar.m_iAmmo + i * 4));
});
static Timer disguise{};
static Timer report_timer{};
static std::string health = "Health: 0/0";
static std::string ammo   = "Ammo: 0/0";
static int max_ammo;
static CachedEntity *local_w;
// TODO: add more stuffs
static void cm()
{
    if (!*catbotmode)
        return;

    if (CE_GOOD(LOCAL_E))
    {
        if (LOCAL_W != local_w)
        {
            local_w  = LOCAL_W;
            max_ammo = 0;
        }
        float max_hp  = g_pPlayerResource->GetMaxHealth(LOCAL_E);
        float curr_hp = CE_INT(LOCAL_E, netvar.iHealth);
        int ammo0     = CE_INT(LOCAL_E, netvar.m_iClip2);
        int ammo2     = CE_INT(LOCAL_E, netvar.m_iClip1);
        if (ammo0 + ammo2 > max_ammo)
            max_ammo = ammo0 + ammo2;
        health = format("Health: ", curr_hp, "/", max_hp);
        ammo   = format("Ammo: ", ammo0 + ammo2, "/", max_ammo);
    }
    if (g_Settings.bInvalid)
        return;

    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

    if (*auto_crouch)
        smart_crouch();

    //
    static const int classes[3]{ tf_spy, tf_sniper, tf_pyro };
    if (*auto_disguise && g_pPlayerResource->GetClass(LOCAL_E) == tf_spy && !HasCondition<TFCond_Disguised>(LOCAL_E) && disguise.test_and_set(3000))
    {
        int teamtodisguise = (LOCAL_E->m_iTeam() == TEAM_RED) ? TEAM_RED - 1 : TEAM_BLU - 1;
        int classtojoin    = classes[rand() % 3];
        g_IEngine->ClientCmd_Unrestricted(format("disguise ", classtojoin, " ", teamtodisguise).c_str());
    }
    if (*autoReport && report_timer.test_and_set(60000))
        reportall();
}

static Timer autojointeam{};
static Timer report_timer2{};
void update()
{
    if (g_Settings.bInvalid)
        return;

    if (can_report)
    {
        typedef uint64_t (*ReportPlayer_t)(uint64_t, int);
        static uintptr_t addr1                = gSignatures.GetClientSignature("55 89 E5 57 56 53 81 EC ? ? ? ? 8B 5D ? 8B 7D ? 89 D8");
        static ReportPlayer_t ReportPlayer_fn = ReportPlayer_t(addr1);
        if (!addr1)
            return;
        if (report_timer2.test_and_set(400))
        {
            if (to_report.empty())
                can_report = false;
            else
            {
                auto rep = to_report.back();
                to_report.pop_back();
                CSteamID id(rep, EUniverse::k_EUniversePublic, EAccountType::k_EAccountTypeIndividual);
                ReportPlayer_fn(id.ConvertToUint64(), 1);
            }
        }
    }
    if (!catbotmode)
        return;

    if (CE_BAD(LOCAL_E))
        return;

    if (LOCAL_E->m_bAlivePlayer())
        autojointeam.update();
    if (autojointeam.test_and_set(60000) && !LOCAL_E->m_bAlivePlayer())
    {
        hack::command_stack().push("autoteam");
        hack::command_stack().push("join_class sniper");
    }
    if (micspam)
    {
        if (micspam_on && micspam_on_timer.test_and_set(*micspam_on * 1000))
        {
            g_IEngine->ExecuteClientCmd("+voicerecord");
            g_IEngine->ExecuteClientCmd("cat_set voice_maxgain 10000");
        }
        if (micspam_off && micspam_off_timer.test_and_set(*micspam_off * 1000))
            g_IEngine->ExecuteClientCmd("-voicerecord");
    }

    if (random_votekicks && timer_votekicks.test_and_set(5000))
        do_random_votekick();
    if (timer_catbot_list.test_and_set(3000))
        update_catbot_list();
    if (timer_abandon.test_and_set(2000) && level_init_timer.check(13000))
    {
        count_bots      = 0;
        int count_ipc   = 0;
        int count_total = 0;

        for (int i = 1; i <= g_GlobalVars->maxClients; ++i)
        {
            if (g_IEntityList->GetClientEntity(i))
                ++count_total;
            else
                continue;

            player_info_s info{};
            if (!g_IEngine->GetPlayerInfo(i, &info))
                continue;

            if (is_a_catbot(info.friendsID))
                ++count_bots;

            if (playerlist::AccessData(info.friendsID).state == playerlist::k_EState::IPC)
                ++count_ipc;
        }

        if (abandon_if_bots_gte)
        {
            if (count_bots >= int(abandon_if_bots_gte))
            {
                logging::Info("Abandoning because there are %d bots in game, "
                              "and abandon_if_bots_gte is %d.",
                              count_bots, int(abandon_if_bots_gte));
                tfmm::abandon();
                return;
            }
        }
        if (abandon_if_ipc_bots_gte)
        {
            if (count_ipc >= int(abandon_if_ipc_bots_gte))
            {
                logging::Info("Abandoning because there are %d local players "
                              "in game, and abandon_if_ipc_bots_gte is %d.",
                              count_ipc, int(abandon_if_ipc_bots_gte));
                tfmm::abandon();
                return;
            }
        }
        if (abandon_if_humans_lte)
        {
            if (count_total - count_bots <= int(abandon_if_humans_lte))
            {
                logging::Info("Abandoning because there are %d non-bots in "
                              "game, and abandon_if_humans_lte is %d.",
                              count_total - count_bots, int(abandon_if_humans_lte));
                tfmm::abandon();
                return;
            }
        }
        if (abandon_if_players_lte)
        {
            if (count_total <= int(abandon_if_players_lte))
            {
                logging::Info("Abandoning because there are %d total players "
                              "in game, and abandon_if_players_lte is %d.",
                              count_total, int(abandon_if_players_lte));
                tfmm::abandon();
                return;
            }
        }
    }
}

void init()
{
    g_IEventManager2->AddListener(&listener(), "player_death", false);
}

void level_init()
{
    level_init_timer.update();
}

#if ENABLE_VISUALS
static void draw()
{
    if (!catbotmode || !anti_motd)
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    AddCenterString(health, colors::green);
    AddCenterString(ammo, colors::yellow);
}
#endif

static InitRoutine runinit([]() {
    EC::Register(EC::CreateMove, cm, "cm_catbot", EC::average);
    EC::Register(EC::Paint, update, "paint_catbot", EC::average);
#if ENABLE_VISUALS
    EC::Register(EC::Draw, draw, "draw_catbot", EC::average);
#endif
    init();
});
} // namespace hacks::shared::catbot
