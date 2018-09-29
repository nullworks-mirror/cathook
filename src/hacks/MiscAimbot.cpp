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
std::pair<CachedEntity *, Vector> FindBestEnt(bool teammate, bool Predict, bool zcheck)
{
    CachedEntity *bestent = nullptr;
    float bestscr         = FLT_MAX;
    Vector predicted{};
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || !(ent->m_bAlivePlayer()) ||
                (teammate && ent->m_iTeam() != LOCAL_E->m_iTeam()) || ent == LOCAL_E)
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
            bestent = ent;
            predicted = target;
            bestscr = scr;
        }
    }
    return {bestent, predicted};
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
        result = FindBestEnt(true, true, false);
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
static settings::Bool charge_aim{ "chargeaim.enable", "false"};
static HookedFunction ChargeAimbot(HookedFunctions_types::HF_CreateMove, "ChargeAim", 1, [](){
    if (!*charge_aim)
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    if (!HasCondition<TFCond_Charging>(LOCAL_E))
        return;
    std::pair<CachedEntity *, Vector> result{};
    result = FindBestEnt(false, false, true);
    CachedEntity *bestent = result.first;
    if (bestent && result.second.IsValid())
    {
        Vector tr = result.second - g_pLocalPlayer->v_Eye;
        Vector angles;
        VectorAngles(tr, angles);
        // Clamping is important
        fClampAngle(angles);
        current_user_cmd->viewangles = angles;
        current_user_cmd->buttons |= IN_ATTACK2;
        g_pLocalPlayer->bUseSilentAngles = true;
    }
});