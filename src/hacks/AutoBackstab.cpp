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
#include "hacks/AntiAntiAim.hpp"

namespace hacks::tf2::autobackstab
{
static settings::Bool enable{ "autobackstab.enable", "0" };
static settings::Bool silent{ "autobackstab.silent", "1" };

const static float headOffset = 2.5f;

bool rangeCheck(Vector hitboxLoc)
{
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    Vector center = (LOCAL_E->hitboxes.GetHitbox(upperArm_L)->center + LOCAL_E->hitboxes.GetHitbox(upperArm_R)->center) / 2;
    center.z = g_pLocalPlayer->v_Eye.z;
    return (center.DistTo(hitboxLoc) <= meleeRange + headOffset);
}

Vector rotateVector(Vector center, float radianAngle, Vector p)
{
    float s = sin(radianAngle);
    float c = cos(radianAngle);

    // translate point back to origin:
    p.x -= center.x;
    p.y -= center.y;

    // rotate point
    //  float xnew = p.x * c - p.y * s;
    //  float ynew = p.x * s + p.y * c;
    Vector vecNew{ p.x * c - p.y * s, p.x * s + p.y * c, 0 };

    // translate point back:
    p.x = vecNew.x + center.x;
    p.y = vecNew.y + center.y;
    return p;
}

//void updateHeadOffset()
//{
//    Vector center = (LOCAL_E->hitboxes.GetHitbox(upperArm_L)->center + LOCAL_E->hitboxes.GetHitbox(upperArm_R)->center) / 2;
//    Vector eye = g_pLocalPlayer->v_Eye;
//    center.z = eye.z;
//    float currOffset = eye.DistTo(center);
//    if (currOffset > headOffset)
//    {
//        headOffset = currOffset;
//    }
//    if (removeMe.test_and_set(2000))
//        logging::Info("%f", currOffset);
//}

// Function to find the closest hitbox to the v_Eye for a given ent
int ClosestDistanceHitbox(CachedEntity *target)
{
    int closest        = -1;
    float closest_dist = 0.0f, dist = 0.0f;

    for (int i = spine_0; i < spine_3; i++)
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
            loc.z = g_pLocalPlayer->v_Eye.z;
            if (rangeCheck(hitboxLoc))
                return true;
        }
    }
    return false;
}

bool canBacktrackStab(hacks::shared::backtrack::BacktrackData &i,
                      Vector vecAngle, Vector loc, Vector hitboxLoc, float targetAngle)
{
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    if (fabsf(vecAngle.y - targetAngle) >= 45)
        return false;
    if (!rangeCheck(hitboxLoc))
        return false;

    auto min = i.spineMin;
    auto max = i.spineMax;
    if (!min.x && !max.x)
        return false;

    // Get the min and max for the hitbox
    Vector minz(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
    Vector maxz(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));

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
    forward   = forward * (meleeRange + headOffset) + loc;

    Vector hit;
    if (hacks::shared::triggerbot::CheckLineBox(
            minz, maxz, g_pLocalPlayer->v_Eye, forward, hit))
        return true;
    return false;
}

bool unifiedCanBackstab(Vector &vecAngle, Vector min, Vector max, Vector hitboxLoc, CachedEntity *besttarget)
{
    // Get melee range
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    if (fabsf(vecAngle.y - NET_VECTOR(RAW_ENT(besttarget), netvar.m_angEyeAngles).y) >= 45)
        return false;
    if (!min.x && !max.x)
        return false;

    //Calculate head position
    Vector currEye = g_pLocalPlayer->v_Eye;
    float rotateRadians = atan2(currEye.x,currEye.y);
    Vector center = (LOCAL_E->hitboxes.GetHitbox(upperArm_L)->center + LOCAL_E->hitboxes.GetHitbox(upperArm_R)->center) / 2;
    center.z = currEye.z;
    Vector head = rotateVector(center, rotateRadians, currEye);

    // Check if we are in range. Note: This has to be done in order to avoid
    // false positives even when "forward" is only "meleeRange" away from the
    // head.
    if (head.DistTo(hitboxLoc) > meleeRange)
        return false;

    //Calculate head x angle
    Vector tr = (hitboxLoc - head);
    Vector xAngle;
    VectorAngles(tr, xAngle);
    fClampAngle(xAngle);
    vecAngle.x = xAngle.x;

    // Get the min and max for the hitbox
    Vector minz(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
    Vector maxz(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));

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
    forward   = forward * meleeRange + head;

    Vector hit;
    // Check if we our line is within the targets hitbox
    if (hacks::shared::triggerbot::CheckLineBox(
            minz, maxz, head, forward, hit))
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
        hacks::shared::anti_anti_aim::resolveEnt(besttarget->m_IDX);
        Vector angle = NET_VECTOR(RAW_ENT(LOCAL_E), netvar.m_angEyeAngles);
        if (!hacks::shared::backtrack::isBacktrackEnabled())
        {
            for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 10.0f)
            {
                Vector hitboxLoc =
                    besttarget->hitboxes
                        .GetHitbox(ClosestDistanceHitbox(besttarget))
                        ->center;

                if (!unifiedCanBackstab(angle, besttarget->hitboxes.GetHitbox(ClosestDistanceHitbox(besttarget))->min, besttarget->hitboxes.GetHitbox(ClosestDistanceHitbox(besttarget))->max, hitboxLoc, besttarget))
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
            for (auto i : hacks::shared::backtrack::headPositions[idx])
            {
                bool good_tick = false;
                for (int j = 0; j < hacks::shared::backtrack::getTicks2(); ++j)
                    if (tickcnt ==
                            hacks::shared::backtrack::sorted_ticks[j].tick &&
                        hacks::shared::backtrack::sorted_ticks[j].tickcount !=
                            INT_MAX)
                        good_tick = true;
                tickcnt++;
                if (!good_tick)
                    continue;

                for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 20.0f)
                {
                    if (unifiedCanBackstab(angle, i.spineMin, i.spineMax, i.spine, besttarget))
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
