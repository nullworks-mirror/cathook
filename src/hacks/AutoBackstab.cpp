/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "PlayerTools.hpp"
#include "Trigger.hpp"
#include "Backtrack.hpp"

namespace hacks::tf2::autobackstab
{
static settings::Bool enabled("autobackstab.enabled", "false");
static settings::Int mode("autobackstab.mode", "0");

static bool angleCheck(CachedEntity *target, std::optional<Vector> target_pos, Vector local_angle)
{
    Vector tarAngle = CE_VECTOR(target, netvar.m_angEyeAngles);

    Vector wsc_spy_to_victim;
    if (target_pos)
        wsc_spy_to_victim = *target_pos - LOCAL_E->m_vecOrigin();
    else
        wsc_spy_to_victim = target->m_vecOrigin() - LOCAL_E->m_vecOrigin();
    wsc_spy_to_victim.z = 0;
    wsc_spy_to_victim.NormalizeInPlace();

    Vector eye_spy;
    AngleVectors2(VectorToQAngle(local_angle), &eye_spy);
    eye_spy.z = 0;
    eye_spy.NormalizeInPlace();

    Vector eye_victim;
    AngleVectors2(VectorToQAngle(tarAngle), &eye_victim);
    eye_victim.z = 0;
    eye_victim.NormalizeInPlace();

    if (DotProduct(wsc_spy_to_victim, eye_victim) <= 0.0f)
        return false;
    if (DotProduct(wsc_spy_to_victim, eye_spy) <= 0.5f)
        return false;
    if (DotProduct(eye_spy, eye_victim) <= -0.3f)
        return false;
    return true;
}

static void doLegitBackstab()
{
    trace_t trace;
    if (!re::C_TFWeaponBaseMelee::DoSwingTrace(RAW_ENT(LOCAL_W), &trace))
        return;
    if (!trace.m_pEnt)
        return;
    int index = reinterpret_cast<IClientEntity *>(trace.m_pEnt)->entindex();
    auto ent  = ENTITY(index);
    if (index == 0 || index > g_IEngine->GetMaxClients() || !ent->m_bEnemy() || !player_tools::shouldTarget(ent))
        return;
    if (angleCheck(ENTITY(index), std::nullopt, g_pLocalPlayer->v_OrigViewangles))
    {
        current_user_cmd->buttons |= IN_ATTACK;
    }
}

static void doRageBackstab()
{
    float swingrange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    Vector newangle  = g_pLocalPlayer->v_OrigViewangles;
    std::vector<float> yangles;
    for (newangle.y = -180.0f; newangle.y < 180.0f; newangle.y += 5.0f)
    {
        trace_t trace;
        Ray_t ray;
        trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
        ray.Init(g_pLocalPlayer->v_Eye, GetForwardVector(g_pLocalPlayer->v_Eye, newangle, swingrange));
        g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
        if (trace.m_pEnt)
        {
            int index = reinterpret_cast<IClientEntity *>(trace.m_pEnt)->entindex();
            auto ent  = ENTITY(index);
            if (index == 0 || index > g_IEngine->GetMaxClients() || !ent->m_bEnemy() || !player_tools::shouldTarget(ent))
                continue;
            if (angleCheck(ent, std::nullopt, newangle))
            {
                yangles.push_back(newangle.y);
            }
        }
    }
    if (!yangles.empty())
    {
        newangle.y = yangles.at(std::floor((float) yangles.size() / 2));
        current_user_cmd->buttons |= IN_ATTACK;
        current_user_cmd->viewangles     = newangle;
        g_pLocalPlayer->bUseSilentAngles = true;
        return;
    }
}

static void doBacktrackStab()
{
    float swingrange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    CachedEntity *ent;
    if (hacks::shared::backtrack::iBestTarget < 1)
        return;
    ent = ENTITY(hacks::shared::backtrack::iBestTarget);
    if (!ent->m_bEnemy() || !player_tools::shouldTarget(ent))
        return;
    auto &btd       = hacks::shared::backtrack::headPositions[ent->m_IDX];
    Vector newangle = g_pLocalPlayer->v_OrigViewangles;
    std::vector<float> yangles;
    float best_scr = FLT_MAX;
    hacks::shared::backtrack::BacktrackData *best_tick;
    for (int ii = 0; ii < 66; ii++)
    {
        std::vector<float> yangles_tmp;
        auto &i = btd[ii];

        Vector distcheck = i.entorigin;
        distcheck.z      = g_pLocalPlayer->v_Eye.z;
        if (distcheck.DistTo(g_pLocalPlayer->v_Eye) < best_scr && distcheck.DistTo(g_pLocalPlayer->v_Eye) > 20.0f)
        {
            for (newangle.y = -180.0f; newangle.y < 180.0f; newangle.y += 5.0f)
            {
                if (!hacks::shared::backtrack::ValidTick(i, ent))
                    continue;
                if (!angleCheck(ent, i.entorigin, newangle))
                    continue;

                Vector &min = i.collidable.min;
                Vector &max = i.collidable.max;

                // Get the min and max for the hitbox
                Vector minz(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
                Vector maxz(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));

                // Shrink the hitbox here
                Vector size = maxz - minz;
                Vector smod = { size.x * 0.20f, size.y * 0.20f, 0 };

                // Save the changes to the vectors
                minz += smod;
                maxz -= smod;
                maxz.z += 20.0f;

                Vector hit;
                if (hacks::shared::triggerbot::CheckLineBox(minz, maxz, g_pLocalPlayer->v_Eye, GetForwardVector(g_pLocalPlayer->v_Eye, newangle, swingrange), hit))
                    yangles_tmp.push_back(newangle.y);
            }
            if (!yangles_tmp.empty())
            {
                yangles.clear();
                best_scr  = distcheck.DistTo(g_pLocalPlayer->v_Eye);
                best_tick = &i;
                yangles   = yangles_tmp;
            }
        }
    }
    if (!yangles.empty() && best_tick)
    {
        newangle.y                   = yangles.at(std::floor((float) yangles.size() / 2));
        current_user_cmd->tick_count = best_tick->tickcount;
        current_user_cmd->viewangles = newangle;
        current_user_cmd->buttons |= IN_ATTACK;
        g_pLocalPlayer->bUseSilentAngles = true;
    }
    if (!*bSendPackets)
        *bSendPackets = true;
}

void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E) || g_pLocalPlayer->life_state || g_pLocalPlayer->clazz != tf_spy || CE_BAD(LOCAL_W) || GetWeaponMode() != weapon_melee || !CanShoot())
        return;
    if (!CanShoot())
        return;
    switch (*mode)
    {
    case 0:
        doLegitBackstab();
        break;
    case 1:
        doRageBackstab();
        break;
    case 2:
        if (hacks::shared::backtrack::isBacktrackEnabled)
            doBacktrackStab();
    default:
        break;
    }
}

static InitRoutine EC([]() { EC::Register(EC::CreateMove, CreateMove, "autobackstab", EC::average); });
} // namespace hacks::tf2::autobackstab
