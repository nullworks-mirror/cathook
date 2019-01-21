/*
 * prediction.cpp
 *
 *  Created on: Dec 5, 2016
 *      Author: nullifiedcat
 */
#include "common.hpp"
#if ENABLE_VISUALS
#include <glez/draw.hpp>
#endif
#include <settings/Bool.hpp>

static settings::Bool debug_enginepred{ "debug.engine-pred-others", "false" };
static settings::Bool debug_pp_extrapolate{ "debug.pp-extrapolate", "false" };
static settings::Bool debug_pp_rockettimeping{ "debug.pp-rocket-time-ping", "false" };

// TODO there is a Vector() object created each call.

Vector SimpleLatencyPrediction(CachedEntity *ent, int hb)
{
    if (!ent)
        return Vector();
    Vector result;
    GetHitbox(ent, hb, result);
    float latency = g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);
    result += CE_VECTOR(ent, netvar.vVelocity) * latency;
    return result;
}

float PlayerGravityMod(CachedEntity *player)
{
    //      int movetype = CE_INT(player, netvar.movetype);
    //      if (movetype == MOVETYPE_FLY || movetype == MOVETYPE_NOCLIP) return
    //      0.0f;
    if (HasCondition<TFCond_Parachute>(player))
        return 0.448f;
    return 1.0f;
}

bool PerformProjectilePrediction(CachedEntity *target, int hitbox)
{
    Vector src, vel, hit;
    ;
    // src = vfunc<Vector(*)(IClientEntity*)>(RAW_ENT(target),
    // 299)(RAW_ENT(target));

    return true;
}

std::vector<std::vector<Vector>> predicted_players{};

int predicted_player_count = 0;

void Prediction_CreateMove()
{
    static bool setup = false;
    if (!setup)
    {
        setup = true;
        predicted_players.resize(32);
    }
    if (!debug_enginepred)
        return;
    for (int i = 1; i < g_GlobalVars->maxClients; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_GOOD(ent))
        {
            Vector o = ent->m_vecOrigin();
            predicted_players[i].clear();
            for (int j = 0; j < 20; j++)
            {
                Vector r           = EnginePrediction(ent, 0.05f);
                ent->m_vecOrigin() = r;
                predicted_players[i].push_back(std::move(r));
            }
            ent->m_vecOrigin()    = o;
            CE_VECTOR(ent, 0x354) = o;
            // logging::Info("Predicted %d to be at [%.2f, %.2f, %.2f] vs [%.2f,
            // %.2f, %.2f]", i, r.x,r.y,r.z, o.x, o.y, o.z);
            predicted_player_count = i;
        }
    }
}
#if ENABLE_VISUALS
void Prediction_PaintTraverse()
{
    if (!debug_enginepred)
        return;
    for (int i = 1; i < predicted_player_count; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_GOOD(ent))
        {
            Vector previous_screen;
            if (!draw::WorldToScreen(ent->m_vecOrigin(), previous_screen))
                continue;
            rgba_t color = colors::FromRGBA8(255, 0, 0, 255);
            for (int j = 0; j < predicted_players[i].size(); j++)
            {
                Vector screen;
                if (draw::WorldToScreen(predicted_players[i][j], screen))
                {
                    draw::Line(screen.x, screen.y, previous_screen.x - screen.x, previous_screen.y - screen.y, color, 0.5f);
                    previous_screen = screen;
                }
                else
                {
                    break;
                }
                color.r -= 1.0f / 20.0f;
            }
        }
    }
}
#endif
Vector EnginePrediction(CachedEntity *entity, float time)
{
    Vector result      = entity->m_vecOrigin();
    IClientEntity *ent = RAW_ENT(entity);

    typedef void (*SetupMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, class IMoveHelper *, CMoveData *);
    typedef void (*FinishMoveFn)(IPrediction *, IClientEntity *, CUserCmd *, CMoveData *);

    void **predictionVtable  = *((void ***) g_IPrediction);
    SetupMoveFn oSetupMove   = (SetupMoveFn)(*(unsigned *) (predictionVtable + 19));
    FinishMoveFn oFinishMove = (FinishMoveFn)(*(unsigned *) (predictionVtable + 20));

    // CMoveData *pMoveData = (CMoveData*)(sharedobj::client->lmap->l_addr +
    // 0x1F69C0C);  CMoveData movedata {};
    auto object          = std::make_unique<char[]>(165);
    CMoveData *pMoveData = (CMoveData *) object.get();

    float frameTime = g_GlobalVars->frametime;
    float curTime   = g_GlobalVars->curtime;

    CUserCmd fakecmd{};

    memset(&fakecmd, 0, sizeof(CUserCmd));

    Vector vel;
    if (velocity::EstimateAbsVelocity)
        velocity::EstimateAbsVelocity(RAW_ENT(entity), vel);
    else
        vel = CE_VECTOR(entity, netvar.vVelocity);
    fakecmd.command_number = last_cmd_number;
    fakecmd.forwardmove    = vel.x;
    fakecmd.sidemove       = -vel.y;
    Vector oldangles       = CE_VECTOR(entity, netvar.m_angEyeAngles);
    static Vector zerov{ 0, 0, 0 };
    CE_VECTOR(entity, netvar.m_angEyeAngles) = zerov;

    CUserCmd *original_cmd = NET_VAR(ent, 4188, CUserCmd *);

    NET_VAR(ent, 4188, CUserCmd *) = &fakecmd;

    g_GlobalVars->curtime   = g_GlobalVars->interval_per_tick * NET_INT(ent, netvar.nTickBase);
    g_GlobalVars->frametime = time;

    Vector old_origin      = entity->m_vecOrigin();
    NET_VECTOR(ent, 0x354) = entity->m_vecOrigin();

    //*g_PredictionRandomSeed =
    // MD5_PseudoRandom(current_user_cmd->command_number) &
    // 0x7FFFFFFF;
    g_IGameMovement->StartTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));
    oSetupMove(g_IPrediction, ent, &fakecmd, NULL, pMoveData);
    g_IGameMovement->ProcessMovement(reinterpret_cast<CBasePlayer *>(ent), pMoveData);
    oFinishMove(g_IPrediction, ent, &fakecmd, pMoveData);
    g_IGameMovement->FinishTrackPredictionErrors(reinterpret_cast<CBasePlayer *>(ent));

    NET_VAR(entity, 4188, CUserCmd *) = original_cmd;

    g_GlobalVars->frametime = frameTime;
    g_GlobalVars->curtime   = curTime;

    result                                   = ent->GetAbsOrigin();
    NET_VECTOR(ent, 0x354)                   = old_origin;
    CE_VECTOR(entity, netvar.m_angEyeAngles) = oldangles;
    return result;
}

Vector ProjectilePrediction_Engine(CachedEntity *ent, int hb, float speed, float gravitymod, float entgmod /* ignored */)
{
    Vector origin = ent->m_vecOrigin();
    Vector hitbox;
    GetHitbox(ent, hb, hitbox);
    Vector hitbox_offset = hitbox - origin;

    if (speed == 0.0f)
        return Vector();
    Vector velocity;
    if (velocity::EstimateAbsVelocity)
        velocity::EstimateAbsVelocity(RAW_ENT(ent), velocity);
    else
        velocity = CE_VECTOR(ent, netvar.vVelocity);
    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(hitbox) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = origin;
    Vector current = origin;
    int maxsteps   = 40;
    bool onground  = false;
    if (ent->m_Type() == ENTITY_PLAYER)
    {
        if (CE_INT(ent, netvar.iFlags) & FL_ONGROUND)
            onground = true;
    }
    float steplength = ((float) (2 * range) / (float) maxsteps);
    for (int steps = 0; steps < maxsteps; steps++, currenttime += steplength)
    {
        ent->m_vecOrigin() = current;
        current            = EnginePrediction(ent, steplength);

        if (onground)
        {
            float toground = DistanceToGround(current);
            current.z -= toground;
        }

        float rockettime = g_pLocalPlayer->v_Eye.DistTo(current) / speed;
        if (fabs(rockettime - currenttime) < mindelta)
        {
            besttime = currenttime;
            bestpos  = current;
            mindelta = fabs(rockettime - currenttime);
        }
    }
    ent->m_vecOrigin()    = origin;
    CE_VECTOR(ent, 0x354) = origin;
    bestpos.z += (400 * besttime * besttime * gravitymod);
    // S = at^2/2 ; t = sqrt(2S/a)*/
    Vector result = bestpos + hitbox_offset;
    /*logging::Info("[Pred][%d] delta: %.2f   %.2f   %.2f", result.x - origin.x,
                  result.y - origin.y, result.z - origin.z);*/
    return result;
}
Vector BuildingPrediction(CachedEntity *building, Vector vec, float speed, float gravity)
{
    if (!vec.z || CE_BAD(building))
        return Vector();
    Vector result = vec;
    // if (not debug_pp_extrapolate) {
    //} else {
    //        result = SimpleLatencyPrediction(ent, hb);
    //
    //}

    if (speed == 0.0f)
        return Vector();
    trace::filter_no_player.SetSelf(RAW_ENT(building));
    float dtg = DistanceToGround(vec);
    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(result) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = result;
    int maxsteps   = 300;
    for (int steps = 0; steps < maxsteps; steps++, currenttime += ((float) (2 * range) / (float) maxsteps))
    {
        Vector curpos = result;
        if (dtg > 0.0f)
        {
            if (curpos.z < result.z - dtg)
                curpos.z = result.z - dtg;
        }
        float rockettime = g_pLocalPlayer->v_Eye.DistTo(curpos) / speed;
        if (debug_pp_rockettimeping)
            rockettime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
        if (fabs(rockettime - currenttime) < mindelta)
        {
            besttime = currenttime;
            bestpos  = curpos;
            mindelta = fabs(rockettime - currenttime);
        }
    }
    if (debug_pp_rockettimeping)
        besttime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
    bestpos.z += (400 * besttime * besttime * gravity);
    // S = at^2/2 ; t = sqrt(2S/a)*/
    return bestpos;
}
Vector ProjectilePrediction(CachedEntity *ent, int hb, float speed, float gravitymod, float entgmod)
{
    if (!ent)
        return Vector();
    Vector result;
    // if (not debug_pp_extrapolate) {
    GetHitbox(ent, hb, result);
    //} else {
    //        result = SimpleLatencyPrediction(ent, hb);
    //
    //}
    float latency = g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING) + g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_INCOMING);

    if (speed == 0.0f)
        return Vector();
    trace::filter_no_player.SetSelf(RAW_ENT(ent));
    float dtg = DistanceToGround(ent);
    Vector vel;
    if (velocity::EstimateAbsVelocity)
        velocity::EstimateAbsVelocity(RAW_ENT(ent), vel);
    else
        vel = CE_VECTOR(ent, netvar.vVelocity);
    // TODO ProjAim
    float medianTime  = g_pLocalPlayer->v_Eye.DistTo(result) / speed;
    float range       = 1.5f;
    float currenttime = medianTime - range;
    if (currenttime <= 0.0f)
        currenttime = 0.01f;
    float besttime = currenttime;
    float mindelta = 65536.0f;
    Vector bestpos = result;
    int maxsteps   = 300;
    for (int steps = 0; steps < maxsteps; steps++, currenttime += ((float) (2 * range) / (float) maxsteps))
    {
        Vector curpos = result;
        curpos += vel * currenttime;
        if (debug_pp_extrapolate)
        {
            curpos += vel * currenttime * latency;
        }
        if (dtg > 0.0f)
        {
            curpos.z -= currenttime * currenttime * 400.0f * entgmod;
            if (curpos.z < result.z - dtg)
                curpos.z = result.z - dtg;
        }
        float rockettime = g_pLocalPlayer->v_Eye.DistTo(curpos) / speed;
        if (debug_pp_rockettimeping)
            rockettime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
        if (fabs(rockettime - currenttime) < mindelta)
        {
            besttime = currenttime;
            bestpos  = curpos;
            mindelta = fabs(rockettime - currenttime);
        }
    }
    if (debug_pp_rockettimeping)
        besttime += g_IEngine->GetNetChannelInfo()->GetLatency(FLOW_OUTGOING);
    bestpos.z += (400 * besttime * besttime * gravitymod);
    // S = at^2/2 ; t = sqrt(2S/a)*/
    return bestpos;
}

float DistanceToGround(CachedEntity *ent)
{
    if (ent->m_Type() == ENTITY_PLAYER)
    {
        if (CE_INT(ent, netvar.iFlags) & FL_ONGROUND)
            return 0;
    }
    Vector origin = ent->m_vecOrigin();
    float v1      = DistanceToGround(origin + Vector(10.0f, 10.0f, 0.0f));
    float v2      = DistanceToGround(origin + Vector(-10.0f, 10.0f, 0.0f));
    float v3      = DistanceToGround(origin + Vector(10.0f, -10.0f, 0.0f));
    float v4      = DistanceToGround(origin + Vector(-10.0f, -10.0f, 0.0f));
    return MIN(v1, MIN(v2, MIN(v3, v4)));
}

float DistanceToGround(Vector origin)
{
    trace_t ground_trace;
    Ray_t ray;
    Vector endpos = origin;
    endpos.z -= 8192;
    ray.Init(origin, endpos);
    g_ITrace->TraceRay(ray, MASK_PLAYERSOLID, &trace::filter_no_player, &ground_trace);
    return 8192.0f * ground_trace.fraction;
}
