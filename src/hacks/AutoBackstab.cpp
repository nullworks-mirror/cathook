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
static CatVar value(CV_INT, "autobackstab_range", "75.0f",
                    "Set Detection Distance to this much");
bool found;
// TODO improve
void CreateMove()
{
    if (!enabled)
        return;
    if (!CE_GOOD(LOCAL_E))
        return;
    if (!LOCAL_E->m_bAlivePlayer())
        return;
    if (g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
        return;
    if (!hacks::shared::backtrack::enable &&
        CE_BYTE(g_pLocalPlayer->weapon(), netvar.m_bReadyToBackstab))
        g_pUserCmd->buttons |= IN_ATTACK;
    else
    {
        if (!hacks::shared::backtrack::enable)
            return;
        if (hacks::shared::backtrack::iBestTarget == -1)
            return;
        int iBestTarget = hacks::shared::backtrack::iBestTarget;
        int tickcnt     = 0;
        for (auto i : hacks::shared::backtrack::headPositions[iBestTarget])
        {
            bool good_tick = false;
            for (int j = 0; j < 12; ++j)
                if (tickcnt == hacks::shared::backtrack::sorted_ticks[j].tick)
                    good_tick = true;
            tickcnt++;
            if (!good_tick)
                continue;

            float scr = abs(g_pLocalPlayer->v_OrigViewangles.y - i.viewangles);

            if (scr <= 90.0f &&
                i.origin.DistTo(g_pLocalPlayer->v_Eye) <=
                    re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W)))
            {
                CachedEntity *tar = ENTITY(iBestTarget);
                // ok just in case
                if (CE_BAD(tar))
                    continue;
                Vector &angles = NET_VECTOR(tar, netvar.m_angEyeAngles);
                float &simtime = NET_FLOAT(tar, netvar.m_flSimulationTime);
                angles.y       = i.viewangles;
                simtime        = i.simtime;
                g_pUserCmd->tick_count = i.tickcount;
                g_pUserCmd->buttons |= IN_ATTACK;
                break;
            }
        }
    }
}
}
