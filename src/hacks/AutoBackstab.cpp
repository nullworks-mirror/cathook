/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

namespace hacks
{
namespace tf2
{
namespace autobackstab
{

// pPaste, thanks to F1ssi0N
const Vector GetWorldSpaceCenter(CachedEntity *ent)
{
    Vector vMin, vMax;
    RAW_ENT(ent)->GetRenderBounds(vMin, vMax);
    Vector vWorldSpaceCenter = RAW_ENT(ent)->GetAbsOrigin();
    vWorldSpaceCenter.z += (vMin.z + vMax.z) / 2;
    return vWorldSpaceCenter;
}

static CatVar enabled(CV_SWITCH, "autobackstab", "0", "Auto Backstab",
                      "Does not depend on triggerbot!");
static CatVar value(CV_INT, "autobackstab_range", "110.0f",
                    "Set Detection Distance to this much");
bool found;
// TODO improve
void CreateMove()
{
    if (!enabled)
        return;
    if (!CE_GOOD(LOCAL_E))
        return;
    if (!LOCAL_E->m_bAlivePlayer)
        return;
    if (g_pLocalPlayer->weapon()->m_iClassID != CL_CLASS(CTFKnife))
        return;
    if (CE_BYTE(g_pLocalPlayer->weapon(), netvar.m_bReadyToBackstab))
    {
        g_pUserCmd->buttons |= IN_ATTACK;
    }
    else if (HasCondition<TFCond_Cloaked>(LOCAL_E))
    {
        found = false;
        if (!CE_GOOD(LOCAL_E))
            return;
        CachedEntity *ent;
        for (int i; i < 32; i++)
        {
            float scr          = 0;
            float scr_best     = 0;
            CachedEntity *pEnt = ENTITY(i);
            if (!CE_GOOD(pEnt))
                continue;
            if (pEnt->m_Type != ENTITY_PLAYER)
                continue;
            if (!pEnt->m_bAlivePlayer)
                continue;
            if (pEnt == LOCAL_E)
                continue;
            if (LOCAL_E->m_iTeam == pEnt->m_iTeam)
                continue;
            scr = 4096.0f - pEnt->m_vecOrigin.DistTo(LOCAL_E->m_vecOrigin);
            if ((scr > scr_best) &&
                LOCAL_E->m_vecOrigin.DistTo(pEnt->m_vecOrigin) < (int) value)
            {
                scr_best = scr;
                ent      = pEnt;
                found    = true;
            }
        }
        if (!found)
            return;
        if (!CE_GOOD(ent))
            return;

        Vector vecVictimForward;
        vecVictimForward.x = CE_FLOAT(ent, netvar.angEyeAngles);
        vecVictimForward.y = CE_FLOAT(ent, netvar.angEyeAngles + 4);
        vecVictimForward.z = 0.0f;
        vecVictimForward.NormalizeInPlace();

        // Get a vector from my origin to my targets origin
        Vector vecToTarget;

        vecToTarget   = GetWorldSpaceCenter(ent) - GetWorldSpaceCenter(LOCAL_E);
        vecToTarget.z = 0.0f;
        vecToTarget.NormalizeInPlace();

        float flDot = DotProduct(vecVictimForward, vecToTarget);

        if (flDot > -0.1)
            g_pUserCmd->buttons |= IN_ATTACK;
    }
}
}
}
}
