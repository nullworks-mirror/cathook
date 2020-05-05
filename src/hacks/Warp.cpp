/*
 * Created on 16.4.2020
 * Author: BenCat07
 *
 * Copyright Nullworks 2020
 */

#include "common.hpp"
#if ENABLE_VISUALS
#include "drawing.hpp"
#endif
#include "MiscAimbot.hpp"

namespace hacks::tf2::warp
{
static settings::Boolean enabled{ "warp.enabled", "false" };
static settings::Boolean draw{ "warp.draw", "false" };
static settings::Button warp_key{ "warp.key", "<null>" };
static settings::Boolean charge_passively{ "warp.charge-passively", "true" };
static settings::Boolean charge_in_jump{ "warp.charge-passively.jump", "true" };
static settings::Boolean charge_no_input{ "warp.charge-passively.no-inputs", "false" };
static settings::Int warp_movement_ratio{ "warp.movement-ratio", "6" };
static settings::Boolean warp_demoknight{ "warp.demoknight", "false" };
static settings::Boolean warp_peek{ "warp.peek", "false" };
static settings::Boolean warp_on_damage{ "warp.on-hit", "false" };
static settings::Boolean warp_forward{ "warp.on-hit.forward", "false" };
static settings::Boolean warp_backwards{ "warp.on-hit.backwards", "false" };
static settings::Boolean warp_left{ "warp.on-hit.left", "true" };
static settings::Boolean warp_right{ "warp.on-hit.right", "true" };

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
    return warp_override ? warp_override : sv_max_dropped_packets_to_process->GetInt();
}

static bool should_warp = true;
static bool was_hurt    = false;

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
    // Don't attack while warping
    current_user_cmd->buttons &= ~IN_ATTACK;
    if (warp_amount <= 0)
    {
        was_hurt    = false;
        warp_amount = 0;
    }
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
        if ((warp_key.isKeyDown() || was_hurt) && warp_amount)
        {
            Warp();
            if (warp_amount < GetMaxWarpTicks())
                charged = false;
        }
    }
    should_charge = false;
}

static bool move_last_tick        = true;
static bool warp_last_tick        = false;
static bool should_warp_last_tick = false;
static bool was_hurt_last_tick    = false;
static int ground_ticks           = 0;
// Left and right by default
static std::vector<float> yaw_selections{ 90.0f, -90.0f };

enum charge_state
{
    ATTACK = 0,
    CHARGE,
    WARP,
    DONE
};

charge_state current_state = ATTACK;
void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    warp_override = 0;
    if (!warp_key.isKeyDown() && !was_hurt)
    {
        warp_last_tick = false;
        current_state  = ATTACK;

        Vector velocity{};
        velocity::EstimateAbsVelocity(RAW_ENT(LOCAL_E), velocity);

        if (!charge_in_jump)
        {
            if (CE_INT(LOCAL_E, netvar.iFlags) & FL_ONGROUND)
                ground_ticks++;
            else
                ground_ticks = 0;
        }

        // Bunch of checks, if they all pass we are standing still
        if ((ground_ticks > 1 || charge_in_jump) && (charge_no_input || velocity.IsZero()) && !HasCondition<TFCond_Charging>(LOCAL_E) && !current_user_cmd->forwardmove && !current_user_cmd->sidemove && !current_user_cmd->upmove && !(current_user_cmd->buttons & IN_JUMP) && !(current_user_cmd->buttons & (IN_ATTACK | IN_ATTACK2)))
        {
            if (!move_last_tick)
                should_charge = true;
            move_last_tick = false;

            return;
        }
        else if (charge_passively && (charge_in_jump || ground_ticks > 1) && !(current_user_cmd->buttons & (IN_ATTACK | IN_ATTACK2)))
        {
            // Use everxy xth tick for charging
            if (*warp_movement_ratio > 0 && !(tickcount % *warp_movement_ratio))
                should_charge = true;
            move_last_tick = true;
        }
    }
    // Warp when hurt
    else if (was_hurt)
    {
        // Store direction
        static float yaw = 0.0f;
        // Select yaw
        if (!was_hurt_last_tick)
        {
            yaw = 0.0f;
            if (yaw_selections.empty())
                return;
            // Select randomly
            yaw = yaw_selections[UniformRandomInt(0, yaw_selections.size() - 1)];
        }
        // The yaw we want to achieve
        float actual_yaw = DEG2RAD(yaw);
        if (g_pLocalPlayer->bUseSilentAngles)
            actual_yaw = DEG2RAD(yaw);

        // Set forward/sidemove
        current_user_cmd->forwardmove = cos(actual_yaw) * 450.0f;
        current_user_cmd->sidemove    = -sin(actual_yaw) * 450.0f;
    }
    // Demoknight Warp
    else if (warp_demoknight)
    {
        switch (current_state)
        {
        case ATTACK:
        {
            // Get charge meter (0 - 100 range)
            float charge_meter = re::CTFPlayerShared::GetChargeMeter(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E)));

            // If our charge meter is full
            if (charge_meter == 100.0f)
            {
                // Shield is 750 HU/s with no acceleration at all, convert to HU/tick
                float range = 750.0f * g_GlobalVars->interval_per_tick;
                // Then multiply by our warp ticks to get actual range
                range *= warp_amount;
                // Now add a bit of melee range aswell
                range += 100.0f;

                // Find an entity meeting the shield aim criteria in range
                std::pair<CachedEntity *, Vector> result = hacks::tf2::misc_aimbot::FindBestEnt(false, false, true, true, range);

                // We found a good entity within range
                if (result.first)
                {
                    // Force a crit
                    criticals::force_crit_this_tick = true;
                    current_user_cmd->buttons |= IN_ATTACK;
                    current_state = CHARGE;
                }
            }
            should_warp = false;

            break;
        }
        case CHARGE:
        {
            current_user_cmd->buttons |= IN_ATTACK2;
            current_state = WARP;
            should_warp   = false;
            break;
        }
        case WARP:
        {
            should_warp   = true;
            current_state = DONE;
            break;
        }
        default:
            break;
        }
    }
    // Warp peaking
    else if (warp_peek)
    {
        // We have Warp
        if (warp_amount)
        {
            // Warped last tick, time to reverse
            if (warp_last_tick)
            {
                // Wait 1 tick before warping back
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
    was_hurt_last_tick = was_hurt;
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

class WarpHurtListener : public IGameEventListener2
{
public:
    virtual void FireGameEvent(IGameEvent *event)
    { // Not enabled
        if (!enabled || !warp_on_damage)
            return;
        // We have no warp
        if (!warp_amount)
            return;
        // Store userids
        int victim   = event->GetInt("userid");
        int attacker = event->GetInt("attacker");
        player_info_s kinfo{};
        player_info_s vinfo{};

        // Check if both are valid (Attacker & victim)
        if (!g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(victim), &vinfo) || !g_IEngine->GetPlayerInfo(g_IEngine->GetPlayerForUserID(attacker), &kinfo))
            return;
        // Check if victim is local player
        if (g_IEngine->GetPlayerForUserID(victim) != g_pLocalPlayer->entity_idx)
            return;
        // Ignore projectiles for now
        if (GetWeaponMode(ENTITY(attacker)) == weapon_projectile)
            return;
        // We got hurt
        was_hurt = true;
    }
};

static WarpHurtListener listener;

void rvarCallback(settings::VariableBase<bool> &, bool)
{
    yaw_selections.clear();
    if (warp_forward)
        yaw_selections.push_back(0.0f);
    if (warp_backwards)
        yaw_selections.push_back(-180.0f);
    if (warp_left)
        yaw_selections.push_back(-90.0f);
    if (warp_right)
        yaw_selections.push_back(90.0f);
}

static InitRoutine init([]() {
    EC::Register(EC::LevelShutdown, LevelShutdown, "warp_levelshutdown");
    EC::Register(EC::CreateMove, CreateMove, "warp_createmove", EC::very_late);
    g_IEventManager2->AddListener(&listener, "player_hurt", false);
    EC::Register(
        EC::Shutdown, []() { g_IEventManager2->RemoveListener(&listener); }, "warp_shutdown");
    warp_forward.installChangeCallback(rvarCallback);
    warp_backwards.installChangeCallback(rvarCallback);
    warp_left.installChangeCallback(rvarCallback);
    warp_right.installChangeCallback(rvarCallback);

#if ENABLE_VISUALS
    EC::Register(EC::Draw, Draw, "warp_draw");
#endif
});
} // namespace hacks::tf2::warp
