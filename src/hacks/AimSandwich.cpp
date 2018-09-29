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
float grav           = 0.5f;
static HookedFunction
    CreateMove(HookedFunctions_types::HF_CreateMove, "SandwichAim", 1, []() {
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
        float bestscr         = FLT_MAX;
        for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
        {
            CachedEntity *ent = ENTITY(i);
            if (CE_BAD(ent) || !(ent->m_bAlivePlayer()) ||
                ent->m_iTeam() != LOCAL_E->m_iTeam() || ent == LOCAL_E)
                continue;
            Vector target = ProjectilePrediction(ent, 1, sandwich_speed, grav,
                                                 PlayerGravityMod(ent));
            if (!IsEntityVectorVisible(ent, target))
                continue;
            float scr = ent->m_flDistance();
            if (scr < bestscr)
            {
                bestent = ent;
                Predict = target;
                bestscr = scr;
            }
        }
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