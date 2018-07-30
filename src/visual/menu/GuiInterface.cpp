/*
  Created on 29.07.18.
*/
#include <menu/GuiInterface.hpp>

#include <menu/menu/Menu.hpp>
#include <drawing.hpp>
#include <menu/menu/special/SettingsManagerList.hpp>
#include <menu/menu/special/PlayerListController.hpp>

static settings::Button open_gui_button{ "visual.open-gui-button", "Insert" };

static bool init_done{ false };

static std::unique_ptr<zerokernel::special::PlayerListController> controller{ nullptr };

static zerokernel::special::PlayerListData createPlayerListData(int userid)
{
    zerokernel::special::PlayerListData data{};
    auto idx = g_IEngine->GetPlayerForUserID(userid);
    player_info_s info{};
    g_IEngine->GetPlayerInfo(idx, &info);
    data.classId = g_pPlayerResource->getClass(idx);
    data.teamId = g_pPlayerResource->getTeam(idx);
    data.dead = !g_pPlayerResource->isAlive(idx);
    data.steam = info.friendsID;
    strncpy(data.name, info.name, 31);
    return data;
}

class PlayerListEventListener: public IGameEventListener
{
public:
    void FireGameEvent(KeyValues *event) override
    {
        if (!controller)
            return;

        auto userid = event->GetInt("userid");
        if (!userid)
            return;

        std::string name = event->GetName();
        if (name == "player_connect_client")
        {
            controller->addPlayer(userid, createPlayerListData(userid));
        }
        else if (name == "player_disconnect")
        {
            controller->removePlayer(userid);
        }
        else if (name == "player_team")
        {
            controller->updatePlayerTeam(userid, event->GetInt("team"));
        }
        else if (name == "player_changeclass")
        {
            controller->updatePlayerClass(userid, event->GetInt("class"));
        }
        else if (name == "player_changename")
        {
            controller->updatePlayerName(userid, event->GetString("newname"));
        }
        else if (name == "player_death")
        {
            controller->updatePlayerLifeState(userid, false);
        }
        else if (name == "player_spawn")
        {
            controller->updatePlayerLifeState(userid, true);
        }
    }
};

static PlayerListEventListener listener{};

static void initPlayerlist()
{
    auto pl = dynamic_cast<zerokernel::Table *>(zerokernel::Menu::instance->wm->getElementById("special-player-list"));
    if (pl)
    {
        controller = std::make_unique<zerokernel::special::PlayerListController>(*pl);
        controller->setKickButtonCallback([](int uid) {
            // TODO
        });
        controller->setOpenSteamCallback([](unsigned steam) {
            CSteamID id{};
            id.SetFromUint64((0b1000100000000000000000001 << 32) | steam);
            g_ISteamFriends->ActivateGameOverlayToUser("steamid", id);
        });
    }
}

static void load()
{
    zerokernel::Menu::instance->loadFromFile(DATA_PATH "/menu", "menu.xml");

    zerokernel::Container *sv = dynamic_cast<zerokernel::Container *>(zerokernel::Menu::instance->wm->getElementById("special-variables"));
    if (sv)
    {
        zerokernel::special::SettingsManagerList list(*sv);
        list.construct();
        printf("SV found\n");
    }
    initPlayerlist();

    zerokernel::Menu::instance->update();
    zerokernel::Menu::instance->setInGame(true);
}

static CatCommand reload("gui_reload", "Reload", []() {
    load();
});



void gui::init()
{
    zerokernel::Menu::init(draw::width, draw::height);
    g_IGameEventManager->AddListener(&listener, false);

    load();

    init_done = true;
}

void gui::draw()
{
    if (!init_done)
        return;

    zerokernel::Menu::instance->update();
    zerokernel::Menu::instance->render();
}

bool gui::handleSdlEvent(SDL_Event *event)
{
    if (event->type == SDL_KEYDOWN)
    {
        logging::Info("%d %d\n", event->key.keysym.scancode, (*open_gui_button).scan);
        if (event->key.keysym.scancode == (*open_gui_button).scan)
        {
            logging::Info("GUI open button pressed");
            zerokernel::Menu::instance->setInGame(!zerokernel::Menu::instance->isInGame());
            return true;
        }
    }

    return zerokernel::Menu::instance->handleSdlEvent(event) && !zerokernel::Menu::instance->isInGame();
}

void gui::onLevelLoad()
{
    if (controller)
    {
        controller->removeAll();
    }
}

