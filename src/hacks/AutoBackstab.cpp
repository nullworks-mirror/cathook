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
        float min     = CE_FLOAT(LOCAL_E, netvar.m_angEyeAngles + 4) - 45.0f;
        float min2;
        if (min < 1.0f)
            min2  = 360.0f - min;
        float max = CE_FLOAT(LOCAL_E, netvar.m_angEyeAngles + 4) + 45.0f;
        float max2;
        if (max > 360.0f)
            max2 = max - 360.0f;
        if (min2 || max2)
        {
            if (min2 && min2 > max)
            {
                if (ent_eye >= max && ent_eye <= min2)
                {
                    if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 80)
                        g_pUserCmd->buttons |= IN_ATTACK;
                }
            }
            else if (min2 && min2 < max)
            {
                if (ent_eye >= min2 && ent_eye <= max)
                    if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 80)
                        g_pUserCmd->buttons |= IN_ATTACK;
            }
            else if (max2 && max2 < min)
            {
                if (ent_eye >= max2 && ent_eye <= min)
                    if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 80)
                        g_pUserCmd->buttons |= IN_ATTACK;
            }
            else if (max && max2 > min)
            {
                if (ent_eye >= min && ent_eye >= min)
                    if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 80)
                        g_pUserCmd->buttons |= IN_ATTACK;
            }
        }
        else if (ent_eye >= min && ent_eye <= max)
        {
            if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 80)
                g_pUserCmd->buttons |= IN_ATTACK;
        }
    }
}
}
}
}
