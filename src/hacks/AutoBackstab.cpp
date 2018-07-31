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

bool canBackstab(CachedEntity *tar)
{
    float _viewangles =
        NET_VECTOR(RAW_ENT(tar), netvar.m_angEyeAngles).y;
    float viewangles =
        (_viewangles > 180) ? _viewangles - 360 : _viewangles;
    float scr = abs(g_pLocalPlayer->v_OrigViewangles.y - viewangles);
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
        if (target
                ->m_vecOrigin() /*target->hitboxes.GetHitbox(spine_3)->center*/
                .DistTo(g_pLocalPlayer->v_Eye) <= meleeRange)
        {
            if (CE_GOOD(besttarget))
            {
                if (target
                        ->m_vecOrigin() /*target->hitboxes.GetHitbox(spine_3)->center*/
                        .DistTo(g_pLocalPlayer->v_Eye) >
                    target
                        ->m_vecOrigin() /* besttarget->hitboxes.GetHitbox(spine_3)->center*/
                        .DistTo(g_pLocalPlayer->v_Eye))
                    besttarget = target;
            }
            else
            {
                besttarget = target;
            }
        }
    }
    if (CE_GOOD(besttarget) && canBackstab(besttarget))
    {
        g_pUserCmd->buttons |= IN_ATTACK;
        besttarget = nullptr;
    }
}
}
