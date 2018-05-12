/*
 * CreateMove.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hack.hpp"
#include "MiscTemporary.hpp"

#include <link.h>
#include <hacks/hacklist.hpp>

#include "HookedMethods.hpp"

class CMoveData;
namespace engine_prediction
{

void RunEnginePrediction(IClientEntity *ent, CUserCmd *ucmd)
{
    if (!ent)
        return;

    typedef void (*SetupMoveFn)(IPrediction *, IClientEntity *, CUserCmd *,
                                class IMoveHelper *, CMoveData *);
    typedef void (*FinishMoveFn)(IPrediction *, IClientEntity *, CUserCmd *,
                                 CMoveData *);

    void **predictionVtable = *((void ***) g_IPrediction);
    SetupMoveFn oSetupMove =
        (SetupMoveFn)(*(unsigned *) (predictionVtable + 19));
    FinishMoveFn oFinishMove =
        (FinishMoveFn)(*(unsigned *) (predictionVtable + 20));

    // CMoveData *pMoveData = (CMoveData*)(sharedobj::client->lmap->l_addr +
    // 0x1F69C0C);  CMoveData movedata {};
    char object[165];
    CMoveData *pMoveData = (CMoveData *) object;

    float frameTime = g_GlobalVars->frametime;
    float curTime   = g_GlobalVars->curtime;
    CUserCmd defaultCmd;
    if (ucmd == NULL)
    {
        ucmd = &defaultCmd;
    }

    NET_VAR(ent, 4188, CUserCmd *) = ucmd;

    g_GlobalVars->curtime =
        g_GlobalVars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
    g_GlobalVars->frametime = g_GlobalVars->interval_per_tick;

    *g_PredictionRandomSeed =
        MD5_PseudoRandom(g_pUserCmd->command_number) & 0x7FFFFFFF;
    g_IGameMovement->StartTrackPredictionErrors(
        reinterpret_cast<CBasePlayer *>(ent));
    oSetupMove(g_IPrediction, ent, ucmd, NULL, pMoveData);
    g_IGameMovement->ProcessMovement(reinterpret_cast<CBasePlayer *>(ent),
                                     pMoveData);
    oFinishMove(g_IPrediction, ent, ucmd, pMoveData);
    g_IGameMovement->FinishTrackPredictionErrors(
        reinterpret_cast<CBasePlayer *>(ent));

    NET_VAR(ent, 4188, CUserCmd *) = nullptr;

    g_GlobalVars->frametime = frameTime;
    g_GlobalVars->curtime   = curTime;

    return;
}
}

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(CreateMove, bool, void *this_, float input_sample_time,
                     CUserCmd *cmd)
{
    uintptr_t **fp;
    __asm__("mov %%ebp, %0" : "=r"(fp));
    bSendPackets = reinterpret_cast<bool *>(**fp - 8);

    g_Settings.is_create_move = true;
    bool time_replaced, ret, speedapplied;
    float curtime_old, servertime, speed, yaw;
    Vector vsilent, ang;

    tickcount++;
    g_pUserCmd = cmd;

    IF_GAME(IsTF2C())
    {
        if (CE_GOOD(LOCAL_W) && minigun_jump &&
            LOCAL_W->m_iClassID == CL_CLASS(CTFMinigun))
        {
            CE_INT(LOCAL_W, netvar.iWeaponState) = 0;
        }
    }

    ret = original::CreateMove(this_, input_sample_time, cmd);

    PROF_SECTION(CreateMove);

    if (!cmd)
    {
        g_Settings.is_create_move = false;
        return ret;
    }

    if (!cathook)
    {
        g_Settings.is_create_move = false;
        return ret;
    }

    if (!g_IEngine->IsInGame())
    {
        g_Settings.bInvalid       = true;
        g_Settings.is_create_move = false;
        return true;
    }

    //	PROF_BEGIN();

    if (g_pUserCmd && g_pUserCmd->command_number)
        last_cmd_number = g_pUserCmd->command_number;

    /**bSendPackets = true;
    if (hacks::shared::lagexploit::ExploitActive()) {
        *bSendPackets = ((g_pUserCmd->command_number % 4) == 0);
        //logging::Info("%d", *bSendPackets);
    }*/

    // logging::Info("canpacket: %i", ch->CanPacket());
    // if (!cmd) return ret;

    time_replaced = false;
    curtime_old   = g_GlobalVars->curtime;

    if (nolerp)
    {
        g_pUserCmd->tick_count += 1;
        if (sv_client_min_interp_ratio->GetInt() != -1)
        {
            // sv_client_min_interp_ratio->m_nFlags = 0;
            sv_client_min_interp_ratio->SetValue(-1);
        }
        if (cl_interp->m_fValue != 0)
        {
            cl_interp->SetValue(0);
            cl_interp->m_fValue = 0.0f;
            cl_interp->m_nValue = 0;
        }
        if (cl_interp_ratio->GetInt() != 0)
            cl_interp_ratio->SetValue(0);
        // if (cl_interpolate->GetInt() != 0) cl_interpolate->SetValue(0);
    }

    if (!g_Settings.bInvalid && CE_GOOD(g_pLocalPlayer->entity))
    {
        servertime = (float) CE_INT(g_pLocalPlayer->entity, netvar.nTickBase) *
                     g_GlobalVars->interval_per_tick;
        g_GlobalVars->curtime = servertime;
        time_replaced         = true;
    }
    if (g_Settings.bInvalid)
    {
        entity_cache::Invalidate();
    }

    // Disabled because this causes EXTREME aimbot inaccuracy
    // if (!cmd->command_number) return ret;
    //	PROF_BEGIN();
    {
        PROF_SECTION(EntityCache);
        entity_cache::Update();
    }
    //	PROF_END("Entity Cache updating");
    {
        PROF_SECTION(CM_PlayerResource);
        g_pPlayerResource->Update();
    }
    {
        PROF_SECTION(CM_LocalPlayer);
        g_pLocalPlayer->Update();
    }
    g_Settings.bInvalid = false;

    hacks::shared::autojoin::Update();

#if ENABLE_IPC
    static int team_joining_state  = 0;
    static float last_jointeam_try = 0;
    CachedEntity *found_entity, *ent;

    if (hacks::shared::followbot::followbot)
    {
        hacks::shared::followbot::WorldTick();
        if (g_GlobalVars->curtime < last_jointeam_try)
        {
            team_joining_state = 0;
            last_jointeam_try  = 0.0f;
        }

        if (!g_pLocalPlayer->team || (g_pLocalPlayer->team == TEAM_SPEC))
        {
            // if (!team_joining_state) logging::Info("Bad team, trying to
            // join...");
            team_joining_state = 1;
        }
        else
        {
            if (team_joining_state)
            {
                logging::Info("Trying to change CLASS");
                g_IEngine->ExecuteClientCmd(
                    format("join_class ", joinclass.GetString()).c_str());
            }
            team_joining_state = 0;
        }

        if (team_joining_state)
        {
            found_entity = nullptr;
            for (int i = 1; i < 32 && i < HIGHEST_ENTITY; i++)
            {
                ent = ENTITY(i);
                if (CE_BAD(ent))
                    continue;
                if (ent->player_info.friendsID ==
                    (int) hacks::shared::followbot::follow_steam)
                {
                    found_entity = ent;
                    break;
                }
            }

            if (found_entity && CE_GOOD(found_entity))
            {
                if (jointeam &&
                    (g_GlobalVars->curtime - last_jointeam_try) > 1.0f)
                {
                    last_jointeam_try = g_GlobalVars->curtime;
                    switch (CE_INT(found_entity, netvar.iTeamNum))
                    {
                    case TEAM_RED:
                        logging::Info("Trying to join team RED");
                        g_IEngine->ExecuteClientCmd("jointeam red");
                        break;
                    case TEAM_BLU:
                        logging::Info("Trying to join team BLUE");
                        g_IEngine->ExecuteClientCmd("jointeam blue");
                        break;
                    }
                }
            }
        }
    }
#endif
    if (CE_GOOD(g_pLocalPlayer->entity))
    {
        IF_GAME(IsTF2())
        {
            UpdateHoovyList();
        }
        g_pLocalPlayer->v_OrigViewangles = cmd->viewangles;
#if ENABLE_VISUALS
        {
            PROF_SECTION(CM_esp);
            hacks::shared::esp::CreateMove();
        }
#endif
        *bSendPackets = true;
        if (!g_pLocalPlayer->life_state && CE_GOOD(g_pLocalPlayer->weapon()))
        {
            {
                PROF_SECTION(CM_walkbot);
                hacks::shared::walkbot::Move();
            }
            // Walkbot can leave game.
            if (!g_IEngine->IsInGame())
            {
                g_Settings.is_create_move = false;
                return ret;
            }
            IF_GAME(IsTF())
            {
                PROF_SECTION(CM_uberspam);
                hacks::tf::uberspam::CreateMove();
            }
            IF_GAME(IsTF2())
            {
                PROF_SECTION(CM_antibackstab);
                hacks::tf2::antibackstab::CreateMove();
            }
            IF_GAME(IsTF2())
            {
                PROF_SECTION(CM_noisemaker);
                hacks::tf2::noisemaker::CreateMove();
            }
            {
                PROF_SECTION(CM_deadringer);
                hacks::shared::deadringer::CreateMove();
            }
            {
                PROF_SECTION(CM_bunnyhop);
                hacks::shared::bunnyhop::CreateMove();
            }
            if (engine_pred)
                engine_prediction::RunEnginePrediction(RAW_ENT(LOCAL_E),
                                                       g_pUserCmd);
            {
                PROF_SECTION(CM_aimbot);
                hacks::shared::aimbot::CreateMove();
            }
            static int attackticks = 0;
            if (g_pUserCmd->buttons & IN_ATTACK)
                ++attackticks;
            else
                attackticks = 0;
            if (semiauto)
            {
                if (g_pUserCmd->buttons & IN_ATTACK)
                {
                    if (attackticks % int(semiauto) < int(semiauto) - 1)
                    {
                        g_pUserCmd->buttons &= ~IN_ATTACK;
                    }
                }
            }
            {
                PROF_SECTION(CM_antiaim);
                hacks::shared::antiaim::ProcessUserCmd(cmd);
            }
            IF_GAME(IsTF())
            {
                PROF_SECTION(CM_autosticky);
                hacks::tf::autosticky::CreateMove();
            }
            IF_GAME(IsTF())
            {
                PROF_SECTION(CM_autodetonator);
                hacks::tf::autodetonator::CreateMove();
            }
            IF_GAME(IsTF())
            {
                PROF_SECTION(CM_autoreflect);
                hacks::tf::autoreflect::CreateMove();
            }
            {
                PROF_SECTION(CM_triggerbot);
                hacks::shared::triggerbot::CreateMove();
            }
            IF_GAME(IsTF())
            {
                PROF_SECTION(CM_autoheal);
                hacks::tf::autoheal::CreateMove();
            }
            IF_GAME(IsTF2())
            {
                PROF_SECTION(CM_autobackstab);
                hacks::tf2::autobackstab::CreateMove();
            }
            if (debug_projectiles)
                projectile_logging::Update();
            Prediction_CreateMove();
        }
        {
            PROF_SECTION(CM_misc);
            hacks::shared::misc::CreateMove();
        }
        {
            PROF_SECTION(CM_crits);
            criticals::create_move();
        }
        {
            PROF_SECTION(CM_spam);
            hacks::shared::spam::CreateMove();
        }
        {
            PROF_SECTION(CM_AC);
            angles::Update();
            hacks::shared::anticheat::CreateMove();
        }
    }
    if (time_replaced)
        g_GlobalVars->curtime = curtime_old;
    g_Settings.bInvalid       = false;
    {
        PROF_SECTION(CM_chat_stack);
        chat_stack::OnCreateMove();
    }
    {
        PROF_SECTION(CM_healarrow);
        hacks::tf2::healarrow::CreateMove();
    }
    {
        PROF_SECTION(CM_lagexploit);
        hacks::shared::lagexploit::CreateMove();
    }

// TODO Auto Steam Friend

#if ENABLE_IPC
    {
        PROF_SECTION(CM_playerlist);
        static Timer ipc_update_timer{};
        //	playerlist::DoNotKillMe();
        if (ipc_update_timer.test_and_set(1000 * 10))
        {
            ipc::UpdatePlayerlist();
        }
    }
#endif

    if (CE_GOOD(g_pLocalPlayer->entity))
    {
        static int fakelag_queue = 0;
        if (fakelag_amount)
        {
            if (fakelag_queue == int(fakelag_amount) ||
                (g_pUserCmd->buttons & IN_ATTACK))
            {
                *bSendPackets = true;
            }
            else if (fakelag_queue < int(fakelag_amount))
            {
                *bSendPackets = false;
            }
            else
            {
                fakelag_queue = 0;
            }
            fakelag_queue++;
        }
        speedapplied = false;
        if (roll_speedhack &&
            g_IInputSystem->IsButtonDown(
                (ButtonCode_t)((int) roll_speedhack)) &&
            !(cmd->buttons & IN_ATTACK))
        {
            speed = cmd->forwardmove;
            if (fabs(speed) > 0.0f)
            {
                cmd->forwardmove  = -speed;
                cmd->sidemove     = 0.0f;
                cmd->viewangles.y = g_pLocalPlayer->v_OrigViewangles.y;
                cmd->viewangles.y -= 180.0f;
                if (cmd->viewangles.y < -180.0f)
                    cmd->viewangles.y += 360.0f;
                cmd->viewangles.z                = 90.0f;
                g_pLocalPlayer->bUseSilentAngles = true;
                speedapplied                     = true;
            }
        }

        if (g_pLocalPlayer->bUseSilentAngles)
        {
            if (!speedapplied)
            {
                vsilent.x = cmd->forwardmove;
                vsilent.y = cmd->sidemove;
                vsilent.z = cmd->upmove;
                speed     = sqrt(vsilent.x * vsilent.x + vsilent.y * vsilent.y);
                VectorAngles(vsilent, ang);
                yaw = DEG2RAD(ang.y - g_pLocalPlayer->v_OrigViewangles.y +
                              cmd->viewangles.y);
                cmd->forwardmove = cos(yaw) * speed;
                cmd->sidemove    = sin(yaw) * speed;
            }

            ret = false;
        }
        if (cmd)
            g_Settings.last_angles = cmd->viewangles;
    }
    NET_StringCmd senddata(serverlag_string.GetString());
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    senddata.SetNetChannel(ch);
    senddata.SetReliable(false);
    if (servercrash && DelayTimer.check((int) delay * 1000))
    {
        for (int i = 0; i < 7800; i += sizeof(serverlag_string.GetString()))
            ch->SendNetMsg(senddata);
        ch->Transmit();
    }
    if (serverlag_amount || votelogger::antikick_ticks)
    {
        float latency =
            g_IEngine->GetNetChannelInfo()->GetAvgPackets(FLOW_INCOMING);
        logging::Info("%f", latency);
        if (latency > 200 && adjust)
            serverlag_amount = (int) serverlag_amount + 1;
        if (votelogger::antikick_ticks)
            votelogger::antikick_ticks--;
        if (votelogger::antikick_ticks)
        {
            for (int i = 0; i < 7800; i += sizeof(serverlag_string.GetString()))
                ch->SendNetMsg(senddata, false);
            ch->Transmit();
        }
        else if (!votelogger::antikick_ticks &&
                 DelayTimer.check((int) delay * 1000))
        {
            for (int i = 0; i < (int) serverlag_amount; i++)
                ch->SendNetMsg(senddata, false);
            ch->Transmit();
        }
    }

    //	PROF_END("CreateMove");
    if (!(cmd->buttons & IN_ATTACK))
    {
        // LoadSavedState();
    }
    g_pLocalPlayer->bAttackLastTick = (cmd->buttons & IN_ATTACK);
    g_Settings.is_create_move       = false;
    return ret;
}
}

/*float o_curtime;
float o_frametime;

void Start() {
    g_IGameMovement->StartTrackPredictionErrors((CBasePlayer*)(RAW_ENT(LOCAL_E)));

    IClientEntity* player = RAW_ENT(LOCAL_E);
    // CPredictableId::ResetInstanceCounters();
    *(reinterpret_cast<CUserCmd*>(reinterpret_cast<uintptr_t>(player) + 1047)) =
g_pUserCmd; o_curtime = g_GlobalVars->curtime; o_frametime =
g_GlobalVars->frametime; *g_PredictionRandomSeed =
MD5_PseudoRandom(g_pUserCmd->command_number) & 0x7FFFFFFF; g_GlobalVars->curtime
= CE_INT(LOCAL_E, netvar.nTickBase) * g_GlobalVars->interval_per_tick;
    g_GlobalVars->frametime = g_GlobalVars->interval_per_tick;

    CMoveData data;

}

void End() {
    *g_PredictionRandomSeed = -1;
    g_GlobalVars->curtime = o_curtime;
    g_GlobalVars->frametime = o_frametime;
}*/
