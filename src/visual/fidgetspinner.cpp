/*
 * fidgetspinner.cpp
 *
 *  Created on: Jul 4, 2017
 *      Author: nullifiedcat
 */

#include "visual/fidgetspinner.hpp"
#include "common.hpp"

#include <math.h>

#ifndef FEATURE_FIDGET_SPINNER_ENABLED

static CatVar enable_spinner(CV_SWITCH, "fidgetspinner", "0", "Fidget Spinner",
                             "Part of Cathook Autism Awareness program");
CatVar v9mode(CV_SWITCH, "nullcore_mode", "0", "Nullcore mode",
              "Part of Cathook Autism Awareness program");

float spinning_speed = 0.0f;
float angle          = 0;

// DEBUG
/*CatCommand add_spinner_speed("fidgetspinner_debug_speedup", "Add speed", []()
{ spinning_speed += 100.0f;
});*/

class SpinnerListener : public IGameEventListener
{
public:
    virtual void FireGameEvent(KeyValues *event)
    {
        std::string name(event->GetName());
        if (name == "player_death")
        {
            int attacker = event->GetInt("attacker");
            int eid      = g_IEngine->GetPlayerForUserID(attacker);
            if (eid == g_IEngine->GetLocalPlayer())
            {
                spinning_speed += 300.0f;
                // logging::Info("Spinning %.2f", spinning_speed);
            }
        }
    }
};

SpinnerListener listener;

void InitSpinner()
{
    g_IGameEventManager->AddListener(&listener, false);
}

static CatVar spinner_speed_cap(CV_FLOAT, "fidgetspinner_speed_cap", "30",
                                "Speed cap");
static CatVar spinner_speed_scale(CV_FLOAT, "fidgetspinner_speed_scale", "0.03",
                                  "Speed scale");
static CatVar spinner_decay_speed(CV_FLOAT, "fidgetspinner_decay_speed", "0.1",
                                  "Decay speed");
static CatVar spinner_scale(CV_FLOAT, "fidgetspinner_scale", "32",
                            "Spinner Size");
static CatVar spinner_min_speed(CV_FLOAT, "fidgetspinner_min_speed", "2",
                                "Spinner Min Speed");

draw_api::texture_handle_t text{ GLEZ_TEXTURE_INVALID };

void DrawSpinner()
{
    if (not enable_spinner)
        return;
    spinning_speed -= (spinning_speed > 150.0f)
                          ? float(spinner_decay_speed)
                          : float(spinner_decay_speed) / 2.0f;
    if (spinning_speed < float(spinner_min_speed))
        spinning_speed = float(spinner_min_speed);
    if (spinning_speed > 1000)
        spinning_speed = 1000;
    float real_speed   = 0;
    const float speed_cap(spinner_speed_cap);
    if (spinning_speed < 250)
        real_speed = speed_cap * (spinning_speed / 250.0f);
    else if (spinning_speed < 500)
        real_speed =
            speed_cap - (speed_cap - 10) * ((spinning_speed - 250.0f) / 250.0f);
    else if (spinning_speed < 750)
        real_speed =
            10 + (speed_cap - 20) * ((spinning_speed - 500.0f) / 250.0f);
    else
        real_speed =
            (speed_cap - 10) + 10 * ((spinning_speed - 750.0f) / 250.0f);
    const float speed_scale(spinner_speed_scale);
    const float size(spinner_scale);
    angle += speed_scale * real_speed;
    int state = min(3, int(spinning_speed / 250));

    const glez_rgba_t color = glez_rgba(255, 255, 255, 255);

    if (text.handle == GLEZ_TEXTURE_INVALID)
        text = draw_api::create_texture(DATA_PATH "/textures/atlas.png");

    draw_api::draw_rect_textured(draw::width / 2, draw::height / 2, size, size,
                                 colors::white, text, 0 + 64 * state,
                                 (3 + (v9mode ? 1 : 0)) * 64, 64, 64, angle);
    if (angle > PI * 4)
        angle -= PI * 4;
}

#endif
