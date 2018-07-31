/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hacks/Backtrack.hpp"
namespace hacks::tf2::autobackstab
{
static CatVar enabled(CV_SWITCH, "autobackstab", "0", "Auto Backstab",
                      "Does not depend on triggerbot!");
static CatVar silent(CV_SWITCH, "autobackstab_silent", "1", "Silent");

bool canBackstab(CachedEntity *tar, float angleY)
{
    float _viewangles = NET_VECTOR(RAW_ENT(tar), netvar.m_angEyeAngles).y;
    float viewangles  = (_viewangles > 180) ? _viewangles - 360 : _viewangles;
    float scr         = abs(angleY - viewangles);
    return (scr <= 90.0f);
}

void CreateMove()
{
    if (!enabled)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
        return;
    // Get melee range of knife
    int meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    CachedEntity *besttarget = nullptr;
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *target = ENTITY(i);
        if (CE_BAD(target))
            continue;
        if (target == LOCAL_E || target->m_iTeam() == LOCAL_E->m_iTeam() ||
            !target->m_bAlivePlayer() || target->m_Type() != ENTITY_PLAYER)
            continue;
        if (target->hitboxes.GetHitbox(spine_3)->center.DistTo(
                g_pLocalPlayer->v_Eye) <= meleeRange)
        {
            if (CE_GOOD(besttarget))
            {
                if (target->hitboxes.GetHitbox(spine_3)->center.DistTo(
                        g_pLocalPlayer->v_Eye) >
                    besttarget->hitboxes.GetHitbox(spine_3)->center.DistTo(
                        g_pLocalPlayer->v_Eye))
                    besttarget = target;
            }
            else
            {
                besttarget = target;
            }
        }
    }
    if (CE_GOOD(besttarget))
    {
        if (canBackstab(besttarget, g_pLocalPlayer->v_OrigViewangles.y))
        {
        g_pUserCmd->buttons |= IN_ATTACK;
        besttarget = nullptr;
        return;
        }
        else
        {
            for (float i = -180.0f; i < 180.0f; i += 30.0f)
            {
                if (canBackstab(besttarget, i))
                {
                    g_pUserCmd->viewangles.y = i;
                    g_pUserCmd->buttons |= IN_ATTACK;
                    besttarget = nullptr;
                    if (silent)
                        g_pLocalPlayer->bUseSilentAngles = true;
                    return;
                }
            }
        }
    }
}
}
