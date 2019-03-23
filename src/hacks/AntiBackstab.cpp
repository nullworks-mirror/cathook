/*
 * AntiBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"
#include "hack.hpp"

static settings::Bool enable{ "antibackstab.enable", "0" };
static settings::Float distance{ "antibackstab.distance", "200" };
static settings::Bool silent{ "antibackstab.silent", "1" };
static settings::Float angle{ "antibackstab.angle", "107.5" };
static settings::Bool sayno{ "antibackstab.nope", "0" };

namespace hacks::tf2::antibackstab
{
bool noaa = false;

void SayNope()
{
    static float last_say = 0.0f;
    if (g_GlobalVars->curtime < last_say)
        last_say = 0.0f;
    if (g_GlobalVars->curtime - last_say < 1.5f)
        return;
    hack::ExecuteCommand("voicemenu 0 7");
    last_say = g_GlobalVars->curtime;
}

float GetAngle(CachedEntity *spy)
{
    float yaw, yaw2, anglediff;
    Vector diff;
    yaw             = g_pLocalPlayer->v_OrigViewangles.y;
    const Vector &A = LOCAL_E->m_vecOrigin();
    const Vector &B = spy->m_vecOrigin();
    diff            = (A - B);
    yaw2            = acos(diff.x / diff.Length()) * 180.0f / PI;
    if (diff.y < 0)
        yaw2 = -yaw2;
    anglediff = yaw - yaw2;
    if (anglediff > 180)
        anglediff -= 360;
    if (anglediff < -180)
        anglediff += 360;
    // logging::Info("Angle: %.2f | %.2f | %.2f | %.2f", yaw, yaw2, anglediff,
    // yaw - yaw2);
    return anglediff;
}

CachedEntity *ClosestSpy()
{
    CachedEntity *closest, *ent;
    float closest_dist, dist;

    closest      = nullptr;
    closest_dist = 0.0f;

    for (int i = 1; i < 32 && i < g_IEntityList->GetHighestEntityIndex(); i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent))
            continue;
        if (CE_BYTE(ent, netvar.iLifeState))
            continue;
        bool ispyro  = false;
        bool isheavy = false;
        if (CE_INT(ent, netvar.iClass) != tf_class::tf_spy)
        {
            if (CE_INT(ent, netvar.iClass) != tf_class::tf_pyro && CE_INT(ent, netvar.iClass) != tf_class::tf_heavy)
                continue;
            int idx = CE_INT(ent, netvar.hActiveWeapon) & 0xFFF;
            if (IDX_BAD(idx))
                continue;
            CachedEntity *pyro_weapon = ENTITY(idx);
            if (CE_BAD(pyro_weapon))
                continue;
            int widx = CE_INT(pyro_weapon, netvar.iItemDefinitionIndex);
            if (widx != 40 && widx != 1146 && widx != 656)
                continue;
            if (widx == 656)
                isheavy = true;
            ispyro = true;
        }
        if (CE_INT(ent, netvar.iTeamNum) == g_pLocalPlayer->team)
            continue;
        if (IsPlayerInvisible(ent))
            continue;
        dist = ent->m_flDistance();
        if (fabs(GetAngle(ent)) > (float) angle || (ispyro && !isheavy && fabs(GetAngle(ent)) > 90.0f) || (isheavy && fabs(GetAngle(ent)) > 132.0f))
        {
            break;
            // logging::Info("Backstab???");
        }
        if ((((!ispyro && dist < (float) distance)) || (ispyro && !isheavy && dist < 314.0f) || (isheavy && dist < 120.0f)) && (dist < closest_dist || !closest_dist))
        {
            closest_dist = dist;
            closest      = ent;
        }
    }
    return closest;
}

void CreateMove()
{
    CachedEntity *spy;
    Vector diff;

    if (!enable || CE_BAD(LOCAL_E))
        return;
    spy = ClosestSpy();
    if (spy)
    {
        noaa = true;
        if (current_user_cmd->buttons & IN_ATTACK)
            return;
        const Vector &A = LOCAL_E->m_vecOrigin();
        const Vector &B = spy->m_vecOrigin();
        diff            = (A - B);
        if (diff.y < 0 || CE_INT(spy, netvar.iClass) == tf_class::tf_heavy)
            current_user_cmd->viewangles.x = 180.0f;
        if (silent)
            g_pLocalPlayer->bUseSilentAngles = true;
        if (sayno)
            SayNope();
    }
    else
        noaa = false;
}

static InitRoutine EC([]() { EC::Register(EC::CreateMove, CreateMove, "antibackstab", EC::late); });
} // namespace hacks::tf2::antibackstab
