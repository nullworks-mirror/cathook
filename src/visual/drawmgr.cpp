/*
 * drawmgr.cpp
 *
 *  Created on: May 22, 2017
 *      Author: nullifiedcat
 */

#include <MiscTemporary.hpp>
#include <hacks/Misc.hpp>
#include <hacks/Aimbot.hpp>
#include <hacks/hacklist.hpp>
#include <glez/glez.hpp>
#include <glez/record.hpp>
#include <settings/Bool.hpp>
#include <settings/Float.hpp>
#include <menu/GuiInterface.hpp>
#include "common.hpp"
#include "visual/drawing.hpp"
#include "hack.hpp"
#include "menu/menu/Menu.hpp"
#include <glez/draw.hpp>

static settings::Bool info_text{ "hack-info.enable", "true" };
static settings::Bool info_text_min{ "hack-info.minimal", "false" };
static settings::Bool draw_snowflakes{ "visual.snowflakes", "true" };
static settings::Float snowflake_min_down{ "visual.snowflakes.fall-speed.min", "0.5f" };
static settings::Float snowflake_max_down{ "visual.snowflakes.fall-speed.max", "2.0f" };
static settings::Float snowflake_min_side{ "visual.snowflakes.sideways-speed.min", "-0.8f" };
static settings::Float snowflake_max_side{ "visual.snowflakes.sideways-speed.max", "0.8f" };

void render_cheat_visuals()
{
    {
        PROF_SECTION(BeginCheatVisuals);
        BeginCheatVisuals();
    }
    {
        PROF_SECTION(DrawCheatVisuals);
        DrawCheatVisuals();
    }
    {
        PROF_SECTION(EndCheatVisuals);
        EndCheatVisuals();
    }
}

glez::record::Record bufferA{};
glez::record::Record bufferB{};

glez::record::Record *buffers[] = { &bufferA, &bufferB };
int currentBuffer               = 0;

void BeginCheatVisuals()
{
    buffers[currentBuffer]->begin();
    ResetStrings();
}

std::mutex drawing_mutex;

struct snowflake
{
    Vector2D pos;
    Vector2D angle;
};

double getRandom(double lower_bound, double upper_bound)
{
    std::uniform_real_distribution<double> unif(lower_bound, upper_bound);
    static std::mt19937 rand_engine(std::time(nullptr));

    double x = unif(rand_engine);
    return x;
}

static std::vector<snowflake> snowflakes{};
static Timer snowflake_spawn{};
static Timer flake_update{};
void DrawCheatVisuals()
{
    /*#if RENDERING_ENGINE_OPENGL
        std::lock_guard<std::mutex> draw_lock(drawing_mutex);
    #endif*/
    {
        PROF_SECTION(DRAW_misc);
        hacks::shared::misc::DrawText();
    }
    {
        PROF_SECTION(DRAW_SNOWFLAKES);

        if (zerokernel::Menu::instance && !zerokernel::Menu::instance->isInGame())
        {
            int idx = 0;
            for (snowflake &flake : snowflakes)
            {
                static textures::sprite snowflake_sprite(256, 0, 16, 16, textures::atlas());
                snowflake_sprite.draw(flake.pos.x, flake.pos.y, 16, 16, colors::white);
                if (flake_update.check(33))
                {
                    flake.pos += flake.angle;
                    float new_down = getRandom(fminf(*snowflake_min_down, flake.angle.y-0.01f), fmaxf(*snowflake_max_down, flake.angle.x+0.01f));
                    float new_side = getRandom(fminf(*snowflake_min_side, flake.angle.x-0.01f), fmaxf(*snowflake_max_side, flake.angle.x+0.01f));
                    flake.angle = {new_side, new_down};
                    if (flake.pos.x > draw::width || flake.pos.x < 0 || flake.pos.y > draw::height)
                    snowflakes.erase(snowflakes.begin()+idx);
                }
                idx++;
            }
            flake_update.test_and_set(33);
            if (snowflake_spawn.test_and_set(900))
            {
                float down_speed = getRandom(*snowflake_min_down, *snowflake_max_down);
                float side_speed = getRandom(*snowflake_min_side, *snowflake_max_side);
                float start_pos = getRandom(draw::width*0.1f, draw::width*0.9f);
                snowflake new_flake{};
                new_flake.pos = {start_pos, 0.0f};
                new_flake.angle = {side_speed, down_speed};
                snowflakes.push_back(new_flake);
            }
        }
    }
    {
        PROF_SECTION(DRAW_info);
        std::string name_s, reason_s;
        PROF_SECTION(PT_info_text);
        if (info_text)
        {
            AddSideString("cathook by nullworks", colors::RainbowCurrent());
            if (!info_text_min)
            {
                AddSideString(hack::GetVersion(),
                              GUIColor());                  // github commit and date
                AddSideString(hack::GetType(), GUIColor()); //  Compile type
#if ENABLE_GUI
                AddSideString("Press 'INSERT' key to open/close cheat menu.", GUIColor());
                AddSideString("Use mouse to navigate in menu.", GUIColor());
#endif
                if (!g_IEngine->IsInGame()
#if ENABLE_GUI
/*
|| g_pGUI->Visible()
*/
#endif
                )
                {
                    // FIXME
                    /*name_s = *force_name;
                    if (name_s.length() < 3)
                        name_s = "*Not Set*";
                    reason_s   = disconnect_reason.GetString();
                    if (reason_s.length() < 3)
                        reason_s = "*Not Set*";
                    AddSideString(""); // foolish
                    AddSideString(format("Custom Name: ", name_s), GUIColor());
                    AddSideString(
                        format("Custom Disconnect Reason: ", reason_s),
                        GUIColor());*/
                }
            }
        }
    }
    if (spectator_target)
    {
        AddCenterString("Press SPACE to stop spectating");
    }
    {
        PROF_SECTION(DRAW_WRAPPER);
        HookTools::DRAW();
    }
    if (CE_GOOD(g_pLocalPlayer->entity) && !g_Settings.bInvalid)
    {
        PROF_SECTION(PT_total_hacks);
        {
            PROF_SECTION(DRAW_aimbot);
            hacks::shared::aimbot::DrawText();
        }
        IF_GAME(IsTF2())
        {
            PROF_SECTION(DRAW_skinchanger);
            hacks::tf2::skinchanger::DrawText();
        }
#ifndef FEATURE_RADAR_DISABLED
        IF_GAME(IsTF())
        {
            PROF_SECTION(DRAW_radar);
            hacks::tf::radar::Draw();
        }
#endif
        IF_GAME(IsTF())
        {
            PROF_SECTION(DRAW_autoreflect);
            hacks::tf::autoreflect::Draw();
        }
        IF_GAME(IsTF2())
        {
            PROF_SECTION(DRAW_backtracc);
            hacks::shared::backtrack::Draw();
        }
        IF_GAME(IsTF2())
        {
            PROF_SECTION(DRAW_lightesp);
            hacks::shared::lightesp::draw();
        }
        {
            PROF_SECTION(DRAW_walkbot);
            hacks::shared::walkbot::Draw();
        }
        IF_GAME(IsTF())
        {
            PROF_SECTION(PT_antidisguise);
            hacks::tf2::antidisguise::Draw();
        }
        IF_GAME(IsTF())
        {
            PROF_SECTION(PT_spyalert);
            hacks::tf::spyalert::Draw();
        }
        {
            PROF_SECTION(DRAW_esp);
            hacks::shared::esp::Draw();
        }
        IF_GAME(IsTF2())
        {
            criticals::draw();
        }
        hacks::tf2::autobackstab::Draw();
#ifndef FEATURE_FIDGET_SPINNER_ENABLED
        DrawSpinner();
#endif
        Prediction_PaintTraverse();
    }
    {
        PROF_SECTION(DRAW_strings);
        DrawStrings();
    }
#if ENABLE_GUI
    {
        PROF_SECTION(DRAW_GUI);
        gui::draw();
    }
#endif
}

void EndCheatVisuals()
{
    buffers[currentBuffer]->end();
    currentBuffer = !currentBuffer;
}

void DrawCache()
{
    buffers[!currentBuffer]->replay();
}
