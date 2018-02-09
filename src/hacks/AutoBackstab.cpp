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
}
}
}
}
