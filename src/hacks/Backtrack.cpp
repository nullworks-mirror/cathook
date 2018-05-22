/*
 * Backtrack.cpp
 *
 *  Created on: May 15, 2018
 *      Author: bencat07
 */

#include "common.hpp"
#include "Backtrack.hpp"
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

std::deque<CIncomingSequence> sequences;
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
    int amt = 0;
    for (auto &seq : sequences)
        amt++;
    amt = 0;
    for (auto &seq : sequences)
    {
        amt++;
        if (g_GlobalVars->realtime - seq.curtime >= Latency / 1000.0f)
        {
            ch->m_nInReliableState = seq.inreliablestate;
            ch->m_nInSequenceNr    = seq.sequencenr;
            break;
        }
    }
}
//=======================================================================
inline float distance_point_to_line(Vector Point, Vector LineOrigin, Vector Dir)
{
    auto PointDir = Point - LineOrigin;

    auto TempOffset =
        PointDir.Dot(Dir) / (Dir.x * Dir.x + Dir.y * Dir.y + Dir.z * Dir.z);
    if (TempOffset < 0.000001f)
        return FLT_MAX;

    auto PerpendicularPoint = LineOrigin + (Dir * TempOffset);

    return (Point - PerpendicularPoint).Length();
}

inline Vector angle_vector(Vector meme)
{
    auto sy = sin(meme.y / 180.f * static_cast<float>(PI));
    auto cy = cos(meme.y / 180.f * static_cast<float>(PI));

    auto sp = sin(meme.x / 180.f * static_cast<float>(PI));
    auto cp = cos(meme.x / 180.f * static_cast<float>(PI));

    return Vector(cp * cy, cp * sy, -sp);
}
//=======================================================================
void Init()
{
    for (int i = 0; i < 32; i++)
        for (int j = 0; j < 66; j++)
            headPositions[i][j] =
                BacktrackData{ 0, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
}
bool disabled = true;
int BestTick = 0;
void Run()
{
    int ticks = latency ? (float) latency / 15 : 12;
    if (!enable)
    {
        if (!disabled)
            Init();
        disabled = true;
        return;
    }
    disabled        = true;
    CUserCmd *cmd   = g_pUserCmd;
    int iBestTarget = -1;
    float bestFov   = 99999;

    if (CE_BAD(LOCAL_E))
        return;

    for (int i = 1; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *pEntity = ENTITY(i);

        if (CE_BAD(pEntity) || !pEntity->m_bAlivePlayer)
        {
            if (headPositions[i][0].hitboxpos.x)
                for (int j              = 0; j < 66; j++)
                    headPositions[i][j] = BacktrackData{
                        0, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }
                    };
            continue;
        }
        if (pEntity->m_iTeam == LOCAL_E->m_iTeam)
            continue;
        if (pEntity->m_Type != ENTITY_PLAYER)
            continue;
        if (!pEntity->hitboxes.GetHitbox(0))
            continue;
        Vector hitboxpos = pEntity->hitboxes.GetHitbox(0)->center;
        Vector min       = pEntity->hitboxes.GetHitbox(0)->min;
        Vector max       = pEntity->hitboxes.GetHitbox(0)->max;
        headPositions[i][cmd->command_number % ticks] =
            BacktrackData{ cmd->tick_count, hitboxpos, min, max };
        Vector ViewDir = angle_vector(cmd->viewangles);
        float FOVDistance =
            distance_point_to_line(hitboxpos, g_pLocalPlayer->v_Eye, ViewDir);

        if (bestFov > FOVDistance)
        {
            bestFov     = FOVDistance;
            iBestTarget = i;
        }

        if (iBestTarget != -1)
        {
            int bestTick      = 0;
            float tempFOV = 9999;
            float bestFOV = 30;
            Vector lowestDistTicks(180, 180, 0);

            for (int t = 0; t < 12; ++t)
            {
                Vector ViewDir = angle_vector(cmd->viewangles);
                tempFOV        = distance_point_to_line(
                    headPositions[iBestTarget][t].hitboxpos,
                    g_pLocalPlayer->v_Eye, ViewDir);
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
    int ticks = latency ? (float) latency / 15 : 12;
    for (int i = 0; i < 32; i++)
        for (int j = 0; j < ticks; j++)
        {
            auto hbpos    = headPositions[i][j].hitboxpos;
            auto tickount = headPositions[i][j].tickcount;
            if (!hbpos.x && !hbpos.y && !hbpos.z)
                continue;
            Vector out;
            rgba_t color =
                colors::FromHSL(fabs(sin(j / 2.0f)) * 360.0f, 0.85f, 0.9f);
            if (draw::WorldToScreen(hbpos, out))
            {
                draw_api::draw_rect(out.x, out.y, 3, 3, color);
            }
        }
#endif
}
}
}
}
