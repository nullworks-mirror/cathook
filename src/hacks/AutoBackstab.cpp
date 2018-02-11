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

static CatVar enabled(CV_SWITCH, "autobackstab", "0", "Auto Backstab",
                      "Does not depend on triggerbot!");
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
            if (!pEnt->m_bAlivePlayer)
                continue;
            if (!pEnt->m_Type == ENTITY_PLAYER)
                continue;
            if (pEnt == LOCAL_E)
                continue;
            if (LOCAL_E->m_iTeam == pEnt->m_iTeam)
                continue;
            scr = 4096.0f - pEnt->m_vecOrigin.DistTo(LOCAL_E->m_vecOrigin);
            if (scr > scr_best)
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
        float ent_eye = CE_FLOAT(ent, netvar.m_angEyeAngles + 4);
        float min     = CE_FLOAT(LOCAL_E, netvar.m_angEyeAngles + 4) - 50.0f;
        float max = CE_FLOAT(LOCAL_E, netvar.m_angEyeAngles + 4) + 50.0f;
        if (ent_eye >= min && ent_eye <= max)
        {
            if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 100)
                g_pUserCmd->buttons |= IN_ATTACK;
        }
    }
}
}
}
}
