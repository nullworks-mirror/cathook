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
#include "PlayerTools.hpp"

namespace hacks::tf2::autobackstab
{
namespace backtrack = hacks::shared::backtrack;
static settings::Bool enable{ "autobackstab.enable", "0" };
static settings::Bool silent{ "autobackstab.silent", "1" };
void testingFunc();
void AngleVectors2(const QAngle &angles, Vector *forward)
{
    float sp, sy, cp, cy;

    SinCos(DEG2RAD(angles[YAW]), &sy, &cy);
    SinCos(DEG2RAD(angles[PITCH]), &sp, &cp);

    forward->x = cp * cy;
    forward->y = cp * sy;
    forward->z = -sp;
}

// Not required anymore, keeping for future reference
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

// Function to find the closest hitbox to the v_Eye for a given ent
int ClosestDistanceHitbox(CachedEntity *target,
                          backtrack::BacktrackData btd = {})
{
    int closest        = -1;
    float closest_dist = 0.0f, dist = 0.0f;
    for (int i = pelvis; i < lowerArm_R; i++)
    {
        if (hacks::shared::backtrack::isBacktrackEnabled)
            dist = g_pLocalPlayer->v_Eye.DistTo(btd.hitboxes.at(i).center);
        else
            dist = g_pLocalPlayer->v_Eye.DistTo(
                target->hitboxes.GetHitbox(i)->center);
        if (dist < closest_dist || closest == -1)
        {
            closest      = i;
            closest_dist = dist;
        }
    }
    return closest;
}

bool unifiedCanBackstab(CachedEntity *tar, Vector &vecAngle,
                        Vector targetOrigin, Vector min, Vector max)
{
    // Get melee range
    float meleeRange = re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W));
    Vector tarAngle  = CE_VECTOR(tar, netvar.m_angEyeAngles);

    Vector head = g_pLocalPlayer->v_Eye;

    // Check if we are in range. Note: This has to be done in order to avoid
    // false positives even when "forward" is only "meleeRange" away from the
    // head.
    if (head.DistTo((min + max) / 2) > meleeRange)
        return false;

    Vector wsc_spy_to_victim = (targetOrigin - LOCAL_E->m_vecOrigin());
    wsc_spy_to_victim.z      = 0;
    wsc_spy_to_victim.NormalizeInPlace();

    Vector eye_spy;
    AngleVectors2(VectorToQAngle(vecAngle), &eye_spy);
    eye_spy.z = 0;
    eye_spy.NormalizeInPlace();

    Vector eye_victim;
    AngleVectors2(VectorToQAngle(tarAngle), &eye_victim);
    eye_victim.z = 0;
    eye_victim.NormalizeInPlace();

    // Get the min and max for the hitbox
    Vector minz(fminf(min.x, max.x), fminf(min.y, max.y), fminf(min.z, max.z));
    Vector maxz(fmaxf(min.x, max.x), fmaxf(min.y, max.y), fmaxf(min.z, max.z));

    Vector forward;
    AngleVectors2(VectorToQAngle(vecAngle), &forward);
    forward = forward * meleeRange + head;

    Vector hit;
    // Check if we our line is within the targets hitbox
    if (!hacks::shared::triggerbot::CheckLineBox(minz, maxz, head, forward,
                                                 hit))
        return false;

    if (DotProduct(wsc_spy_to_victim, eye_victim) <= 0.0f)
        return false;
    if (DotProduct(wsc_spy_to_victim, eye_spy) <= 0.5f)
        return false;
    if (DotProduct(eye_spy, eye_victim) <= -0.3f)
        return false;
    return true;
}

void CreateMove()
{
    //    testingFunc();
    if (!enable)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
        return;
    if (!CanShoot())
        return;
    CachedEntity *besttarget = nullptr;
    if (!backtrack::isBacktrackEnabled)
    {
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            CachedEntity *target = ENTITY(i);
            if (CE_BAD(target))
                continue;
            if (target == LOCAL_E || target->m_iTeam() == LOCAL_E->m_iTeam() ||
                !target->m_bAlivePlayer() || target->m_Type() != ENTITY_PLAYER)
                continue;
            if (player_tools::shouldTarget(target) !=
                player_tools::IgnoreReason::DO_NOT_IGNORE)
                continue;
            if (target->hitboxes.GetHitbox(spine_3)->center.DistTo(
                    g_pLocalPlayer->v_Eye) <= 200.0f)
            {
                if (CE_GOOD(besttarget))
                {
                    if (target->hitboxes.GetHitbox(spine_3)->center.DistTo(
                            g_pLocalPlayer->v_Eye) <
                        besttarget->hitboxes.GetHitbox(spine_3)->center.DistTo(
                            g_pLocalPlayer->v_Eye))
                        besttarget = target;
                }
                else
                {
                    besttarget = target;
                }
            }
        }
    }
    else
    {
        // Run if backtrack is enabled
        if (backtrack::iBestTarget == -1)
            return;
        CachedEntity *target = ENTITY(backtrack::iBestTarget);
        // Various valid entity checks
        if (CE_BAD(target))
            return;
        if (target == LOCAL_E || target->m_iTeam() == LOCAL_E->m_iTeam() ||
            !target->m_bAlivePlayer() || target->m_Type() != ENTITY_PLAYER)
            return;
        if (player_tools::shouldTarget(target) !=
            player_tools::IgnoreReason::DO_NOT_IGNORE)
            return;
        // Check if besttick distance is < 200.0f
        if (backtrack::headPositions[target->m_IDX][backtrack::BestTick]
                .hitboxes.at(spine_3)
                .center.DistTo(g_pLocalPlayer->v_Eye) < 200.0f)
            besttarget = target;
    }

    if (CE_GOOD(besttarget))
    {
        hacks::shared::anti_anti_aim::resolveEnt(besttarget->m_IDX);
        Vector angle = NET_VECTOR(RAW_ENT(LOCAL_E), 4104);
        if (!backtrack::isBacktrackEnabled)
        {
            IClientEntity *raw = RAW_ENT(besttarget);
            Vector origin      = besttarget->m_vecOrigin();
            Vector min         = raw->GetCollideable()->OBBMins() + origin;
            Vector max         = raw->GetCollideable()->OBBMaxs() + origin;

            for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 10.0f)
            {
                if (!unifiedCanBackstab(besttarget, angle, origin, min, max))
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
            int idx = besttarget->m_IDX;
            for (int j = 0; j < 66; j++)
            {
                auto i = backtrack::headPositions[idx][j];
                if (!backtrack::ValidTick(i, besttarget))
                    continue;

                for (angle.y = -180.0f; angle.y < 180.0f; angle.y += 20.0f)
                {
                    if (unifiedCanBackstab(besttarget, angle, i.entorigin,
                                           i.collidable.min, i.collidable.max))
                    {
                        current_user_cmd->tick_count = i.tickcount;
                        current_user_cmd->viewangles = angle;
                        current_user_cmd->buttons |= IN_ATTACK;
                        if (silent)
                            g_pLocalPlayer->bUseSilentAngles = true;
                        logging::Info("Success on tick %i", j);
                        return;
                    }
                }
            }
        }
    }
}
const Vector GetWorldSpaceCenter(CachedEntity *ent)
{
    Vector vMin, vMax;
    RAW_ENT(ent)->GetRenderBounds(vMin, vMax);
    Vector vWorldSpaceCenter = RAW_ENT(ent)->GetAbsOrigin();
    vWorldSpaceCenter.z += (vMin.z + vMax.z) / 2;
    return vWorldSpaceCenter;
}

static bool InBackstabAngleRange = false;
static bool LookingAtVic         = false;
static bool IsBehind             = false;

void Draw()
{
    //    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !LOCAL_E->m_bAlivePlayer())
    //        return;
    //    if (g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
    //        return;
    //    CachedEntity *besttarget = nullptr;
    //        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    //        {
    //            CachedEntity *target = ENTITY(i);
    //            if (CE_BAD(target))
    //                continue;
    //            if (target == LOCAL_E || target->m_iTeam() ==
    //            LOCAL_E->m_iTeam() ||
    //                !target->m_bAlivePlayer() || target->m_Type() !=
    //                ENTITY_PLAYER) continue;
    //            if (target->m_vecOrigin().DistTo(
    //                    g_pLocalPlayer->v_Eye) <= 200.0f)
    //            {
    //                if (CE_GOOD(besttarget))
    //                {
    //                    if (besttarget->m_vecOrigin().DistTo(
    //                            g_pLocalPlayer->v_Eye) <
    //                        besttarget->m_vecOrigin().DistTo(
    //                            g_pLocalPlayer->v_Eye))
    //                        besttarget = target;
    //                }
    //                else
    //                {
    //                    besttarget = target;
    //                }
    //            }
    //        }

    //    if (CE_BAD(besttarget))
    //        return;
    //    Vector angle = NET_VECTOR(RAW_ENT(LOCAL_E), netvar.m_angEyeAngles);
    //    Vector tarAngle = NET_VECTOR(RAW_ENT(besttarget),
    //    netvar.m_angEyeAngles);

    //    //bool IsBehind =  dot1 <=  0.0f;
    //    bool LookingAtVic = GetFov(angle, g_pLocalPlayer->v_Eye,
    //    GetWorldSpaceCenter(besttarget)) <= 60.0f; bool InBackstabAngleRange =
    //    fabsf(tarAngle.y - angle.y) <= 107.5f;

    //    rgba_t col1 = IsBehind ? colors::green : colors::red;
    //    rgba_t col2 = LookingAtVic ? colors::green : colors::red;
    //    rgba_t col3 = InBackstabAngleRange ? colors::green : colors::red;
    //    AddCenterString(format("Behind target" /*, dot1*/), col1);
    //    AddCenterString(format("Looking at Target" /*, dot2*/), col2);
    //    AddCenterString(format("In Angle Range" /*, dot3*/), col3);
}

void testingFunc()
{
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
        if (target->m_vecOrigin().DistTo(g_pLocalPlayer->v_Eye) <= 200.0f)
        {
            if (CE_GOOD(besttarget))
            {
                if (besttarget->m_vecOrigin().DistTo(g_pLocalPlayer->v_Eye) <
                    besttarget->m_vecOrigin().DistTo(g_pLocalPlayer->v_Eye))
                    besttarget = target;
            }
            else
            {
                besttarget = target;
            }
        }
    }

    if (CE_BAD(besttarget))
        return;
    Vector angle    = NET_VECTOR(RAW_ENT(LOCAL_E), 4104);
    Vector tarAngle = NET_VECTOR(RAW_ENT(besttarget), netvar.m_angEyeAngles);

    logging::Info("Loc: %f; Tar: %f", angle.y, tarAngle.y);

    Vector wsc_spy_to_victim =
        (besttarget->m_vecOrigin() - LOCAL_E->m_vecOrigin());
    wsc_spy_to_victim.z = 0;
    wsc_spy_to_victim.NormalizeInPlace();

    Vector eye_spy;
    AngleVectors2(VectorToQAngle(angle), &eye_spy);
    eye_spy.z = 0;
    eye_spy.NormalizeInPlace();

    Vector eye_victim;
    AngleVectors2(VectorToQAngle(tarAngle), &eye_victim);
    eye_victim.z = 0;
    eye_victim.NormalizeInPlace();

    IsBehind             = DotProduct(wsc_spy_to_victim, eye_victim) <= 0.0f;
    LookingAtVic         = DotProduct(wsc_spy_to_victim, eye_spy) <= 0.5f;
    InBackstabAngleRange = DotProduct(eye_spy, eye_victim) <= -0.3f;
}

} // namespace hacks::tf2::autobackstab
