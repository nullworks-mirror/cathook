/*
 * AutoBackstab.cpp
 *
 *  Created on: Apr 14, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"
#include "hacks/Backtrack.hpp"
#include "hacks/Aimbot.hpp"
#include "hacks/Trigger.hpp"

static settings::Bool enable{ "autobackstab.enable", "0" };
static settings::Bool silent{ "autobackstab.silent", "1" };

namespace hacks::tf2::autobackstab
{
// Function to find the closest hitbox to the v_Eye for a given ent
int ClosestDistanceHitbox(CachedEntity *target)
{
    int closest        = -1;
    float closest_dist = 0.0f, dist = 0.0f;

    for (int i = 0; i < target->hitboxes.GetNumHitboxes(); i++)
    {
        dist =
            g_pLocalPlayer->v_Eye.DistTo(target->hitboxes.GetHitbox(i)->center);
        if (dist < closest_dist || closest == -1)
        {
            closest      = i;
            closest_dist = dist;
        }
    }
    return closest;
}

// pPaste, thanks to F1ssi0N
const Vector GetWorldSpaceCenter(CachedEntity *ent)
{
    Vector vMin, vMax;
    RAW_ENT(ent)->GetRenderBounds(vMin, vMax);
    Vector vWorldSpaceCenter = RAW_ENT(ent)->GetAbsOrigin();
    vWorldSpaceCenter.z += (vMin.z + vMax.z) / 2;
    return vWorldSpaceCenter;
}

std::pair<Vector, Vector> GetHitboxBounds(CachedEntity *it, int hitbox)
{
    std::pair<Vector, Vector> result(it->hitboxes.GetHitbox(hitbox)->min,
                                     it->hitboxes.GetHitbox(hitbox)->max);
    return result;
}

void traceEntity(int *result_eindex, Vector *result_pos, QAngle angle,
                 Vector loc, float meleeRange)
{
    Ray_t ray;
    Vector forward;
    float sp, sy, cp, cy;
    trace_t trace;

    trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
    sy        = sinf(DEG2RAD(angle[1]));
    cy        = cosf(DEG2RAD(angle[1]));
    sp        = sinf(DEG2RAD(angle[0]));
    cp        = cosf(DEG2RAD(angle[0]));
    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
    forward   = forward * meleeRange + loc;
    ray.Init(loc, forward);
    g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
    if (result_pos)
        *result_pos = trace.endpos;
    if (result_eindex)
    {
        *result_eindex = 0;
    }
    if (trace.m_pEnt && result_eindex)
    {
        *result_eindex = ((IClientEntity *) (trace.m_pEnt))->entindex();
    }
}

bool canBackstab(CachedEntity *tar, Vector angle, Vector loc, Vector hitboxLoc)
{
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    Vector targetAngle = NET_VECTOR(RAW_ENT(tar), netvar.m_angEyeAngles);
    if (fabsf(angle.y - targetAngle.y) < 45)
    {
        int IDX;
        Vector hitLoc;
        traceEntity(&IDX, &hitLoc, QAngle(angle.x, angle.y, angle.z), loc, meleeRange);
        if (IDX == tar->m_IDX)
        {
            if (loc.DistTo(hitboxLoc) <= meleeRange)
                return true;
        }
    }
    return false;
}

bool canBacktrackStab(hacks::shared::backtrack::BacktrackData &i,
                      Vector vecAngle, Vector loc, Vector hitboxLoc)
{
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    float targetAngle = i.viewangles;
    if (fabsf(vecAngle.y - targetAngle) >= 45)
        return false;
    if (loc.DistTo(hitboxLoc) > meleeRange)
        return false;

    auto min = i.spineMin;
    auto max = i.spineMax;
    if (!min.x && !max.x)
        return false;

    // Get the min and max for the hitbox
    Vector minz(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
    Vector maxz(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));

//    if (!IsVectorVisible(g_pLocalPlayer->v_Eye, minz, true) &&
//        !IsVectorVisible(g_pLocalPlayer->v_Eye, maxz, true))
//        return false;

    Vector forward;
    float sp, sy, cp, cy;
    QAngle angle = VectorToQAngle(vecAngle);

    // Use math to get a vector in front of the player
    sy        = sinf(DEG2RAD(angle[1]));
    cy        = cosf(DEG2RAD(angle[1]));
    sp        = sinf(DEG2RAD(angle[0]));
    cp        = cosf(DEG2RAD(angle[0]));
    forward.x = cp * cy;
    forward.y = cp * sy;
    forward.z = -sp;
    forward   = forward * meleeRange + loc;

    Vector hit;
    if (hacks::shared::triggerbot::CheckLineBox(
            minz, maxz, g_pLocalPlayer->v_Eye, forward, hit))
        return true;
    return false;
}

void CreateMove()
{
    if (!enable)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
        return;
    CachedEntity *besttarget = nullptr;

    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *target = ENTITY(i);
        if (CE_BAD(target))
            continue;
        if (target == LOCAL_E || target->m_iTeam() == LOCAL_E->m_iTeam() ||
            !target->m_bAlivePlayer() || target->m_Type() != ENTITY_PLAYER)
            continue;
        if (target->hitboxes.GetHitbox(spine_2)->center.DistTo(
                g_pLocalPlayer->v_Eye) <= 300.0f)
        {
            if (CE_GOOD(besttarget))
            {
                if (target->hitboxes.GetHitbox(spine_2)->center.DistTo(
                        g_pLocalPlayer->v_Eye) <
                    besttarget->hitboxes.GetHitbox(spine_2)->center.DistTo(
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
        Vector angle = NET_VECTOR(RAW_ENT(LOCAL_E), netvar.m_angEyeAngles);
        if (!hacks::shared::backtrack::isBacktrackEnabled())
        {
            for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 10.0f)
            {
                Vector hitboxLoc =
                    besttarget->hitboxes
                        .GetHitbox(ClosestDistanceHitbox(besttarget))
                        ->center;
                // Get angles
                Vector tr = (hitboxLoc - g_pLocalPlayer->v_Eye);
                Vector xAngle;
                VectorAngles(tr, xAngle);
                // Clamping is important
                fClampAngle(xAngle);
                angle.x = xAngle.x;
                if (!canBackstab(besttarget, angle, g_pLocalPlayer->v_Eye,
                                 hitboxLoc))
                    continue;
                current_user_cmd->viewangles = angle;
                current_user_cmd->buttons |= IN_ATTACK;
                besttarget = nullptr;
                if (silent)
                    g_pLocalPlayer->bUseSilentAngles = true;
                return;
            }
        }
        else
        {
            int idx     = besttarget->m_IDX;
            int tickcnt = 0;
            int tickus =
                (float(hacks::shared::backtrack::getLatency()) > 800.0f ||
                 float(hacks::shared::backtrack::getLatency()) < 200.0f)
                    ? 12
                    : 24;
            for (auto i : hacks::shared::backtrack::headPositions[idx])
            {
                bool good_tick = false;
                for (int j = 0; j < tickus; ++j)
                    if (tickcnt ==
                            hacks::shared::backtrack::sorted_ticks[j].tick &&
                        hacks::shared::backtrack::sorted_ticks[j].tickcount !=
                            INT_MAX)
                        good_tick = true;
                tickcnt++;
                if (!good_tick)
                    continue;

                // Get angles
                Vector tr = (i.spine - g_pLocalPlayer->v_Eye);
                Vector xAngle;
                VectorAngles(tr, xAngle);
                // Clamping is important
                fClampAngle(xAngle);
                angle.x = xAngle.x;

                for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 40.0f)
                {
                    if (canBacktrackStab(i, angle, g_pLocalPlayer->v_Eye,
                                         i.spine))
                    {
                        current_user_cmd->tick_count = i.tickcount;
                        current_user_cmd->viewangles = angle;
                        current_user_cmd->buttons |= IN_ATTACK;
                        if (silent)
                            g_pLocalPlayer->bUseSilentAngles = true;
                    }
                }
            }
        }
    }
}
} // namespace hacks::tf2::autobackstab
