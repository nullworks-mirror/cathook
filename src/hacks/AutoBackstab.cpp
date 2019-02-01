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
        std::sort(yangles.begin(), yangles.end());
        newangle.y = (yangles.at(0) + yangles.at(yangles.size() - 1)) / 2.0f;
        current_user_cmd->buttons |= IN_ATTACK;
        current_user_cmd->viewangles     = newangle;
        g_pLocalPlayer->bUseSilentAngles = true;
        return;
    }
}

// Checks if point a in box with two corner points
bool in_box(Vector a, Vector min, Vector max)
{
    return (a.x > min.x && a.y > min.y && a.z > min.z && a.x < max.x && a.y < max.y && a.z < max.z);
}

std::pair<int, float> ReturnBestTickAndAimAng(CachedEntity *ent)
{
    auto &btd      = hacks::shared::backtrack::headPositions[ent->m_IDX];
    int best_tick  = -1;
    float aim_ang  = -360.0f;
    float best_scr = FLT_MAX;
    std::vector<float> yangles;
    for (int ii = 0; ii < 66; ii++)
    {
        auto &i         = btd[ii];
        Vector newangle = g_pLocalPlayer->v_OrigViewangles;
        if (!hacks::shared::backtrack::ValidTick(i, ent))
            continue;
        Vector distcheck = i.entorigin;
        if (distcheck.DistTo(g_pLocalPlayer->v_Eye) < best_scr)
        {
            Vector bbx_min = i.collidable.min;
            Vector bbx_max = i.collidable.max;
            // Shrink Bounding box for better hitrate
            bbx_min += (bbx_max - bbx_min) * 0.05f;
            bbx_max -= (bbx_max - bbx_min) * 0.05f;
            Vector hit;

            std::vector<float> yangles_tmp;
            for (newangle.y = -180.0f; newangle.y < 180.0f; newangle.y += 5.0f)
            {
                Vector aim_at = GetForwardVector(g_pLocalPlayer->v_Eye, GetAimAtAngles(g_pLocalPlayer->v_Eye, distcheck), re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W)));
                if (hacks::shared::triggerbot::CheckLineBox(bbx_min, bbx_max, g_pLocalPlayer->v_Eye, aim_at, hit))
                {
                    if (!angleCheck(ent, i.entorigin, newangle))
                        continue;
                    yangles_tmp.push_back(newangle.y);
                }
            }
            if (yangles_tmp.size())
            {
                yangles.clear();
                best_scr  = distcheck.DistTo(g_pLocalPlayer->v_Eye);
                best_tick = ii;
                yangles   = yangles_tmp;
            }
        }
    }
    if (best_tick != -1)
    {
        std::sort(yangles.begin(), yangles.end());
        aim_ang = (yangles.at(0) + 180.0f + yangles.at(yangles.size() - 1) + 180.0f) / 2.0f - 180.0f;
    }
    return { best_tick, aim_ang };
}

static void doBacktrackStab()
{
    CachedEntity *ent;
    if (hacks::shared::backtrack::iBestTarget < 1)
        return;
    ent = ENTITY(hacks::shared::backtrack::iBestTarget);
    if (!ent || !ent->m_bEnemy() || !player_tools::shouldTarget(ent))
        return;

    std::pair<int, float> Aim_Data = ReturnBestTickAndAimAng(ent);
    if (Aim_Data.first != -1)
    {
        Vector newangle              = g_pLocalPlayer->v_OrigViewangles;
        newangle.y                   = Aim_Data.second;
        current_user_cmd->tick_count = hacks::shared::backtrack::headPositions[ent->m_IDX][Aim_Data.first].tickcount;
        current_user_cmd->viewangles = newangle;
        current_user_cmd->buttons |= IN_ATTACK;
        // g_pLocalPlayer->bUseSilentAngles = true;
        return;
    }
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent) || !ent->m_bAlivePlayer())
            continue;
        std::pair<int, float> Aim_Data = ReturnBestTickAndAimAng(ent);
        if (Aim_Data.first != -1)
        {
            Vector newangle              = g_pLocalPlayer->v_OrigViewangles;
            newangle.y                   = Aim_Data.second;
            current_user_cmd->tick_count = hacks::shared::backtrack::headPositions[ent->m_IDX][Aim_Data.first].tickcount;
            current_user_cmd->viewangles = newangle;
            current_user_cmd->buttons |= IN_ATTACK;
            // g_pLocalPlayer->bUseSilentAngles = true;
            return;
        }
    }
}

void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E) || g_pLocalPlayer->life_state || g_pLocalPlayer->clazz != tf_spy || CE_BAD(LOCAL_W) || GetWeaponMode() != weapon_melee || !CanShoot())
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
