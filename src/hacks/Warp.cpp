/*
 * Created on 16.4.2020
 * Author: BenCat07
 *
 * Copyright Nullworks 2020
 */

#include "init.hpp"
#include "sdk.hpp"
#include "interfaces.hpp"
#include "settings/Bool.hpp"
#include "offsets.hpp"
#include "globals.h"
#include "Warp.hpp"
#include "HookTools.hpp"
#include "usercmd.hpp"
#include "localplayer.hpp"
#include "netvars.hpp"
#include "entitycache.hpp"
#include "conditions.hpp"
#include "velocity.hpp"
#if ENABLE_VISUALS
#include "drawing.hpp"
#endif

namespace hacks::tf2::warp
{
static settings::Boolean enabled{ "warp.enabled", "false" };
static settings::Boolean draw{ "warp.draw", "false" };
static settings::Button warp_key{ "warp.key", "<null>" };
static settings::Int warp_movement_ratio{ "warp.movement-ratio", "6" };
static settings::Boolean warp_peek{ "warp.peek", "false" };

// Draw control
static settings::Int size{ "warp.bar-size", "100" };
static settings::Int bar_x{ "warp.bar-x", "50" };
static settings::Int bar_y{ "warp.bar-y", "200" };

static bool should_charge = false;
static int warp_amount    = 0;
static int warp_override  = 0;
static bool charged       = false;

// Taken from MrSteyk
class clc_move_proto
{
public:
    int VTable;
    char reliable;
    int netchan;
    int field_C;
    int field_10;
    int m_nBackupCommands;
    int m_nNewCommands;
    int m_nLength;
    bf_write m_DataIn;
    bf_write m_DataOut;
};

int GetWarpAmount()
{
    static auto sv_max_dropped_packets_to_process = g_ICvar->FindVar("sv_max_dropped_packets_to_process");
    return warp_override ? *warp_override : sv_max_dropped_packets_to_process->GetInt();
}
bool should_warp = true;

// Warping part
void Warp()
{
    auto ch = g_IEngine->GetNetChannelInfo();
    if (!ch)
        return;
    if (!should_warp)
    {
        should_warp = true;
        return;
    }
    int &m_nOutSequenceNr = *(int *) ((uintptr_t) ch + offsets::m_nOutSequenceNr());

    // Has to be from the cvar
    m_nOutSequenceNr += GetWarpAmount();
    warp_amount -= GetWarpAmount();
    if (warp_amount < 0)
        warp_amount = 0;
}

int GetMaxWarpTicks()
{
    static auto usercmd_cvar = g_ICvar->FindVar("sv_maxusrcmdprocessticks");
    int ticks                = usercmd_cvar->GetInt();
    // No limit set
    if (!ticks)
        ticks = INT_MAX;
    return ticks;
}

void SendNetMessage(INetMessage &msg)
{
    if (!enabled)
        return;

    // Credits to MrSteyk for this btw
    if (msg.GetGroup() == 0xA)
    {
        // Charge
        if (should_charge && !charged)
        {
            int ticks    = GetMaxWarpTicks();
            auto movemsg = (clc_move_proto *) &msg;
            // Just null it :shrug:
            movemsg->m_nBackupCommands = 0;
            movemsg->m_nNewCommands    = 0;
            movemsg->m_DataOut.Reset();
            movemsg->m_DataOut.m_nDataBits  = 0;
            movemsg->m_DataOut.m_nDataBytes = 0;
            movemsg->m_DataOut.m_iCurBit    = 0;

            warp_amount++;
            if (warp_amount >= ticks)
            {
                warp_amount = ticks;
                charged     = true;
            }
        }
        // Warp
        if (warp_key.isKeyDown() && warp_amount)
        {
            Warp();
            if (warp_amount < GetWarpAmount())
                charged = false;
        }
    }
    should_charge = false;
}

static bool move_last_tick        = true;
static bool warp_last_tick        = false;
static bool should_warp_last_tick = false;
void CreateMove()
{
    if (!enabled)
        return;
    warp_override = 0;
    if (!warp_key.isKeyDown())
    {
        warp_last_tick = false;
        Vector velocity{};
        velocity::EstimateAbsVelocity(RAW_ENT(LOCAL_E), velocity);

        // Bunch of checks, if they all pass we are standing still
        if (velocity.IsZero() && !HasCondition<TFCond_Charging>(LOCAL_E) && !current_user_cmd->forwardmove && !current_user_cmd->sidemove && !current_user_cmd->upmove && (CE_INT(LOCAL_E, netvar.iFlags) & FL_ONGROUND) && !(current_user_cmd->buttons & (IN_ATTACK | IN_ATTACK2)))
        {

            if (!move_last_tick)
                should_charge = true;
            move_last_tick = false;

            return;
        }
        else if (!(current_user_cmd->buttons & (IN_ATTACK | IN_ATTACK2)))
        {
            // Use everxy xth tick for charging
            if (!(tickcount % *warp_movement_ratio))
                should_charge = true;
            move_last_tick = true;
        }
    }
    // Warp peaking
    else if (warp_peek)
    {
        // We Have warp
        if (warp_amount)
        {
            // Warped last tick, time to reverse
            if (warp_last_tick)
            {
                // Wait 1 tick before tping back
                if (should_warp && !should_warp_last_tick)
                {
                    should_warp_last_tick = true;
                    should_warp           = false;
                }
                else
                    should_warp_last_tick = false;

                // Inverse direction
                current_user_cmd->forwardmove = -current_user_cmd->forwardmove;
                current_user_cmd->sidemove    = -current_user_cmd->sidemove;
            }
            else
                warp_override = warp_amount / 2;
            warp_last_tick = true;
        }
        // Prevent movement so you don't overshoot when you don't want to
        else
        {
            current_user_cmd->forwardmove = 0.0f;
            current_user_cmd->sidemove    = 0.0f;
        }
    }
}

#if ENABLE_VISUALS
void Draw()
{
    if (!enabled || !draw)
        return;
    if (!g_IEngine->IsInGame())
        return;
    if (CE_BAD(LOCAL_E))
        return;

    float charge_percent = (float) warp_amount / (float) GetMaxWarpTicks();
    // Draw background
    static rgba_t background_color = colors::FromRGBA8(96, 96, 96, 150);
    float bar_bg_x_size            = *size * 2.0f;
    float bar_bg_y_size            = *size / 5.0f;
    draw::Rectangle(*bar_x - 5.0f, *bar_y - 5.0f, bar_bg_x_size + 10.0f, bar_bg_y_size + 10.0f, background_color);
    // Draw bar
    rgba_t color_bar = colors::orange;
    if (GetMaxWarpTicks() == warp_amount)
        color_bar = colors::green;
    color_bar.a = 100 / 255.0f;
    draw::Rectangle(*bar_x, *bar_y, *size * 2.0f * charge_percent, *size / 5.0f, color_bar);
}
#endif

void LevelShutdown()
{
    charged     = false;
    warp_amount = 0;
}

static InitRoutine init([]() {
    EC::Register(EC::LevelShutdown, LevelShutdown, "warp_levelshutdown");
    EC::Register(EC::CreateMove, CreateMove, "warp_createmove", EC::late);
#if ENABLE_VISUALS
    EC::Register(EC::Draw, Draw, "warp_draw");
#endif
});
} // namespace hacks::tf2::warp
