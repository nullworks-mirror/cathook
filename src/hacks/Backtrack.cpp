/*
 * Backtrack.cpp
 *
 *  Created on: May 15, 2018
 *      Author: bencat07
 */

#include "common.hpp"
#include "Backtrack.hpp"
#include <boost/circular_buffer.hpp>

namespace hacks
{
namespace shared
{
namespace backtrack
{
CatVar enable(CV_SWITCH, "backtrack", "0", "Enable backtrack",
              "For legit play only as of right now.");
CatVar draw_bt(CV_SWITCH, "backtrack_draw", "0", "Draw",
               "Draw backtrack ticks");
CatVar latency(CV_FLOAT, "backtrack_latency", "0", "fake lantency",
               "Set fake latency to this many ms");
BacktrackData headPositions[32][66];
int lastincomingsequencenumber = 0;

circular_buf sequences{ 2048 };
void UpdateIncomingSequences()
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (ch)
    {
        int m_nInSequenceNr = ch->m_nInSequenceNr;
        int instate         = ch->m_nInReliableState;
        if (m_nInSequenceNr > lastincomingsequencenumber)
        {
            lastincomingsequencenumber = m_nInSequenceNr;
            sequences.push_front(CIncomingSequence(instate, m_nInSequenceNr,
                                                   g_GlobalVars->realtime));
        }

        if (sequences.size() > 2048)
            sequences.pop_back();
    }
}
void AddLatencyToNetchan(INetChannel *ch, float Latency)
{
    if (Latency > 200.0f)
        Latency -= ch->GetLatency(MAX_FLOWS);
    for (auto &seq : sequences)
    {
        if (g_GlobalVars->realtime - seq.curtime > Latency / 1000.0f)
        {
            ch->m_nInReliableState = seq.inreliablestate;
            ch->m_nInSequenceNr    = seq.sequencenr;
            break;
        }
    }
}
bool installed = false;
int ticks      = 12;
void Init()
{
    for (int i = 0; i < 32; i++)
        for (int j              = 0; j < 66; j++)
            headPositions[i][j] = BacktrackData{
                0, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
            };
    if (!installed)
    {
        latency.InstallChangeCallback(
            [](IConVar *var, const char *pszOldValue, float flOldValue) {
                ticks = max(min(int((float) latency) / 15, 65), 12);
            });
        installed = true;
    }
}
bool disabled   = true;
int BestTick    = 0;
int iBestTarget = -1;
void Run()
{
    if (!enable)
    {
        if (!disabled)
            Init();
        disabled = true;
        return;
    }
    disabled      = true;
    CUserCmd *cmd = g_pUserCmd;
    float bestFov = 99999;

    if (CE_BAD(LOCAL_E))
        return;

    for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *pEntity = ENTITY(i);

        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer())
        {
            for (BacktrackData &btd : headPositions[i])
                btd = BacktrackData{
                    0, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
                };
            continue;
        }
        if (pEntity->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        if (pEntity->m_Type != ENTITY_PLAYER)
            continue;
        if (!pEntity->hitboxes.GetHitbox(0))
            continue;
        Vector hitboxpos = pEntity->hitboxes.GetHitbox(0)->center;
        Vector min       = pEntity->hitboxes.GetHitbox(0)->min;
        Vector max       = pEntity->hitboxes.GetHitbox(0)->max;
        headPositions[i][cmd->command_number % ticks + 1] =
            BacktrackData{ cmd->tick_count, hitboxpos, min, max,
                           pEntity->m_vecOrigin() };
        float FOVDistance = GetFov(g_pLocalPlayer->v_OrigViewangles,
                                   g_pLocalPlayer->v_Eye, hitboxpos);
        iBestTarget = -1;
        if (bestFov > FOVDistance && FOVDistance < 10.0f)
        {
            bestFov     = FOVDistance;
            iBestTarget = i;
        }
        if (iBestTarget != -1 && (g_pUserCmd->buttons & IN_ATTACK ||
                                  g_pUserCmd->buttons & IN_ATTACK2))
        {
            int bestTick  = 0;
            float tempFOV = 9999;
            float bestFOV = 40.0f;
            for (int t = 0; t < ticks; ++t)
            {
                if (GetWeaponMode() == weapon_melee)
                    if (g_pLocalPlayer->v_Eye.DistTo(
                            headPositions[iBestTarget][t].hitboxpos) > 90.0f)
                        return;
                tempFOV = GetFov(g_pLocalPlayer->v_OrigViewangles,
                                 g_pLocalPlayer->v_Eye,
                                 headPositions[iBestTarget][t].hitboxpos);
                if (bestFOV > tempFOV)
                    bestTick = t, bestFOV = tempFOV;
            }

            BestTick = bestTick;
            if (cmd->buttons & IN_ATTACK)
                cmd->tick_count = headPositions[i][bestTick].tickcount;
        }
    }
}
void Draw()
{
#if ENABLE_VISUALS
    if (!enable)
        return;
    if (!draw_bt)
        return;
    for (int i = 0; i < 32; i++)
        for (int j = 0; j < ticks; j++)
        {
            auto hbpos    = headPositions[i][j].hitboxpos;
            auto tickount = headPositions[i][j].tickcount;
            auto min      = headPositions[i][j].min;
            auto max      = headPositions[i][j].max;
            if (!hbpos.x && !hbpos.y && !hbpos.z)
                continue;
            float size = 0.0f;
            if (abs(max.x - min.x) > abs(max.y - min.y))
                size = abs(max.x - min.x);
            else
                size = abs(max.y - min.y);
            Vector out;
            if (i == iBestTarget && j == BestTick)
            {
                if (draw::WorldToScreen(hbpos, out))
                    draw_api::draw_rect(out.x, out.y, size / 2, size / 2,
                                        colors::red);
            }
            else if (draw::WorldToScreen(hbpos, out))
                draw_api::draw_rect(out.x, out.y, size / 4, size / 4,
                                    colors::green);
        }
#endif
}
}
}
}
