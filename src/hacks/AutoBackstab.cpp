/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "PlayerTools.hpp"

namespace hacks::tf2::autobackstab
{
static settings::Bool enabled("autobackstab.enabled", "false");
static settings::Int mode("autobackstab.mode", "0");

static bool angleCheck(CachedEntity *target, Vector localAngle)
{
    Vector tarAngle = CE_VECTOR(target, netvar.m_angEyeAngles);

    Vector wsc_spy_to_victim = (target->m_vecOrigin() - LOCAL_E->m_vecOrigin());
    wsc_spy_to_victim.z      = 0;
    wsc_spy_to_victim.NormalizeInPlace();

    Vector eye_spy;
    AngleVectors2(VectorToQAngle(localAngle), &eye_spy);
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
    if (index == 0 || index > g_IEngine->GetMaxClients())
        return;
    if (angleCheck(ENTITY(index), g_pLocalPlayer->v_OrigViewangles))
    {
        current_user_cmd->buttons |= IN_ATTACK;
    }
}

static void doRageBackstab()
{
    float swingrange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    Vector newangle = g_pLocalPlayer->v_OrigViewangles;

    for (newangle.y = -180.0f; newangle.y < 180.0f; newangle.y += 10.0f)
    {
        trace_t trace;
        Ray_t ray;
        trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
        ray.Init(g_pLocalPlayer->v_Eye, GetForwardVector(g_pLocalPlayer->v_Eye, newangle, swingrange));
        g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
        if (trace.m_pEnt)
        {
            int index = reinterpret_cast<IClientEntity *>(trace.m_pEnt)->entindex();
            logging::Info("Hit: %i");
            if (index == 0 || index > g_IEngine->GetMaxClients())
                continue;
            if (angleCheck(ENTITY(index), newangle))
            {
                current_user_cmd->buttons |= IN_ATTACK;
                current_user_cmd->viewangles = newangle;
                g_pLocalPlayer->bUseSilentAngles = true;
                return;
            }
        }
    }
}

void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E) || g_pLocalPlayer->life_state || g_pLocalPlayer->clazz != tf_spy || CE_BAD(LOCAL_W) || GetWeaponMode() != weapon_melee)
        return;
    switch (*mode)
    {
    case 0:
        doLegitBackstab();
        break;
    case 1:
        doRageBackstab();
        break;
    default:
        break;
    }
}
} // namespace hacks::tf2::autobackstab
