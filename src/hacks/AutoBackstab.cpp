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
    else if (HasCondition<TFCond_Cloaked>(LOCAL_E)) {
    	CachedEntity* ent;
		for (int i; i < 32; i++) {
			float scr = 0;
			float scr_best = 0;
			CachedEntity* pEnt = ENTITY(i);
			if (!CE_GOOD(pEnt))
				continue;
			if (!pEnt->m_Type == ENTITY_PLAYER)
				continue;
			if (pEnt == LOCAL_E)
				continue;
			if (LOCAL_E->m_iTeam == pEnt->m_iTeam)
				continue;
			scr = 4096.0f
					- pEnt->m_vecOrigin.DistTo(
							LOCAL_E->m_vecOrigin);
			if (scr > scr_best) {
				scr_best = scr;
				ent = pEnt;
			}
		}
    	if (!CE_GOOD(ent))
    		return;
    	if (CE_FLOAT(LOCAL_E, netvar.angEyeAngles + 4) + 35.0f >= CE_FLOAT(ent, netvar.angEyeAngles + 4) || CE_FLOAT(LOCAL_E, netvar.angEyeAngles + 4) - 35.0f <= CE_FLOAT(ent, netvar.angEyeAngles + 4))
    		{
    		if (LOCAL_E->m_vecOrigin.DistTo(ent->m_vecOrigin) <= 67) {
    			g_pUserCmd->buttons |= IN_ATTACK;
    		}
    	}
    }
}
}
}
}
