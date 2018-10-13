//
// Created by bencat07 on 28.09.18.
//

#include "common.hpp"
#include <settings/Bool.hpp>
#include <settings/Int.hpp>
#include <settings/Key.hpp>
static settings::Bool enable{ "sandwichaim.enable", "false" };
static settings::Button aimkey{ "sandwichaim.aimkey", "<null>" };
static settings::Int aimkey_mode{ "sandwichaim.aimkey-mode", "0" };

float sandwich_speed = 350.0f;
float grav           = 0.25f;
int prevent = -1;
std::pair<CachedEntity *, Vector> FindBestEnt(bool teammate, bool Predict,
                                              bool zcheck)
{
    CachedEntity *bestent = nullptr;
    float bestscr         = FLT_MAX;
    Vector predicted{};
    for (int i = 0; i < 1; i++)
    {
        if (prevent != -1)
        {
            auto ent = ENTITY(prevent);
            if (CE_BAD(ent) || !ent->m_bAlivePlayer() || (teammate && ent->m_iTeam() != LOCAL_E->m_iTeam()) || ent == LOCAL_E)
                continue;
            if (!teammate && ent->m_iTeam() == LOCAL_E->m_ItemType())
            continue;
            if (!ent->hitboxes.GetHitbox(1))
                continue;
            Vector target{};
            if (Predict)
                target = ProjectilePrediction(ent, 1, sandwich_speed, grav,
                                              PlayerGravityMod(ent));
            else
                target = ent->hitboxes.GetHitbox(1)->center;
            if (!IsEntityVectorVisible(ent, target))
                continue;
            if (zcheck && (ent->m_vecOrigin().z - LOCAL_E->m_vecOrigin().z) > 80.0f)
                continue;
            float scr = ent->m_flDistance();
            if (g_pPlayerResource->GetClass(ent) == tf_medic)
                scr *= 0.1f;
            if (scr < bestscr)
            {
                bestent   = ent;
                predicted = target;
                bestscr   = scr;
                prevent = ent->m_IDX;
            }
        }
        if (bestent && predicted.z)
            return { bestent, predicted };
    }
    prevent = -1;
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !(ent->m_bAlivePlayer()) ||
            (teammate && ent->m_iTeam() != LOCAL_E->m_iTeam()) ||
            ent == LOCAL_E)
            continue;
        if (!teammate && ent->m_iTeam() == LOCAL_E->m_iTeam())
            continue;
        if (!ent->hitboxes.GetHitbox(1))
            continue;
        Vector target{};
        if (Predict)
            target = ProjectilePrediction(ent, 1, sandwich_speed, grav,
                                          PlayerGravityMod(ent));
        else
            target = ent->hitboxes.GetHitbox(1)->center;
        if (!IsEntityVectorVisible(ent, target))
            continue;
        if (zcheck && (ent->m_vecOrigin().z - LOCAL_E->m_vecOrigin().z) > 80.0f)
            continue;
        float scr = ent->m_flDistance();
        if (g_pPlayerResource->GetClass(ent) == tf_medic)
            scr *= 0.1f;
        if (scr < bestscr)
        {
            bestent   = ent;
            predicted = target;
            bestscr   = scr;
            prevent = ent->m_IDX;
        }
    }
    return { bestent, predicted };
}
static float slow_change_dist_y{};
static float slow_change_dist_p{};
void DoSlowAim(Vector &input_angle)
{

    auto viewangles = current_user_cmd->viewangles;

    // Yaw
    if (viewangles.y != input_angle.y)
    {

        // Check if input angle and user angle are on opposing sides of yaw so
        // we can correct for that
        bool slow_opposing = false;
        if ((input_angle.y < -90 && viewangles.y > 90) ||
            (input_angle.y > 90 && viewangles.y < -90))
            slow_opposing = true;

        // Direction
        bool slow_dir = false;
        if (slow_opposing)
        {
            if (input_angle.y > 90 && viewangles.y < -90)
                slow_dir = true;
        }
        else if (viewangles.y > input_angle.y)
            slow_dir = true;

        // Speed, check if opposing. We dont get a new distance due to the
        // opposing sides making the distance spike, so just cheap out and reuse
        // our last one.
        if (!slow_opposing)
            slow_change_dist_y =
                std::abs(viewangles.y - input_angle.y) / 5;

        // Move in the direction of the input angle
        if (slow_dir)
            input_angle.y = viewangles.y - slow_change_dist_y;
        else
            input_angle.y = viewangles.y + slow_change_dist_y;
    }

    // Pitch
    if (viewangles.x != input_angle.x)
    {
        // Get speed
        slow_change_dist_p =
            std::abs(viewangles.x - input_angle.x) / 5;

        // Move in the direction of the input angle
        if (viewangles.x > input_angle.x)
            input_angle.x = viewangles.x - slow_change_dist_p;
        else
            input_angle.x = viewangles.x + slow_change_dist_p;
    }

    // Clamp as we changed angles
    fClampAngle(input_angle);
}

static HookedFunction
    SandwichAim(HookedFunctions_types::HF_CreateMove, "SandwichAim", 1, []() {
        if (!*enable)
            return;
        if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
            return;
        if (aimkey)
        {
            switch (*aimkey_mode)
            {
            case 1:
                if (!aimkey.isKeyDown())
                    return;
                break;
            case 2:
                if (aimkey.isKeyDown())
                    return;
                break;
            default:
                break;
            }
        }
        if (LOCAL_W->m_iClassID() != CL_CLASS(CTFLunchBox))
            return;
        Vector Predict;
        CachedEntity *bestent = nullptr;
        std::pair<CachedEntity *, Vector> result{};
        result  = FindBestEnt(true, true, false);
        bestent = result.first;
        Predict = result.second;
        if (bestent)
        {
            Vector tr = Predict - g_pLocalPlayer->v_Eye;
            Vector angles;
            VectorAngles(tr, angles);
            // Clamping is important
            fClampAngle(angles);
            current_user_cmd->viewangles = angles;
            current_user_cmd->buttons |= IN_ATTACK2;
            g_pLocalPlayer->bUseSilentAngles = true;
        }
    });
static settings::Bool charge_aim{ "chargeaim.enable", "false" };
static HookedFunction
    ChargeAimbot(HookedFunctions_types::HF_CreateMove, "ChargeAim", 1, []() {
        if (!*charge_aim)
            return;
        if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
            return;
        if (!HasCondition<TFCond_Charging>(LOCAL_E))
            return;
        std::pair<CachedEntity *, Vector> result{};
        result                = FindBestEnt(false, false, true);
        CachedEntity *bestent = result.first;
        if (bestent && result.second.IsValid())
        {
            Vector tr = result.second - g_pLocalPlayer->v_Eye;
            Vector angles;
            VectorAngles(tr, angles);
            // Clamping is important
            fClampAngle(angles);
            DoSlowAim(angles);
            current_user_cmd->viewangles = angles;
        }
    });
