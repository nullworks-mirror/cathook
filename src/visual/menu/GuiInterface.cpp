/*
  Created on 29.07.18.
*/
#include <menu/GuiInterface.hpp>

#include <menu/menu/Menu.hpp>
#include <drawing.hpp>
#include <menu/menu/special/SettingsManagerList.hpp>

static settings::Button open_gui_button{ "visual.open-gui-button", "Insert" };

static bool init_done{ false };

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
    // FIXME add playerlist

    zerokernel::Menu::instance->update();
    zerokernel::Menu::instance->setInGame(true);
}

static CatCommand reload("gui_reload", "Reload", []() {
    load();
});

void gui::init()
{
    zerokernel::Menu::init(draw::width, draw::height);

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

