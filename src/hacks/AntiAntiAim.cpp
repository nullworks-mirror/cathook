/*
  Created on 29.07.18.
*/

#include <hacks/AntiAntiAim.hpp>
#include <core/interfaces.hpp>
#include <core/sdk.hpp>
#include <globals.h>
#include <core/netvars.hpp>
#include <settings/Bool.hpp>
#include <localplayer.hpp>
#include <entitycache.hpp>

static settings::Bool enable{ "anti-anti-aim.enable", "false" };

void hacks::shared::anti_anti_aim::createMove()
{
    if (!enable)
        return;
    if (CE_BAD(LOCAL_E))
        return;

    IClientEntity *entity{ nullptr };
    for (int i = 0; i < g_IEngine->GetMaxClients(); i++)
    {
        if (i == g_IEngine->GetLocalPlayer())
            continue;
        entity = g_IEntityList->GetClientEntity(i);
        if (entity && !entity->IsDormant() && !NET_BYTE(entity, netvar.iLifeState))
        {
            float quotat = 0;
            float quotaf = 0;
            if (!g_Settings.brute.choke[i].empty())
                for (auto it : g_Settings.brute.choke[i])
                {
                    if (it)
                        quotat++;
                    else
                        quotaf++;
                }
            float quota    = quotat / quotaf;
            Vector &angles = NET_VECTOR(entity, netvar.m_angEyeAngles);
            static bool brutepitch = false;
            if (g_Settings.brute.brutenum[i] > 5)
            {
                g_Settings.brute.brutenum[i] = 0;
                brutepitch                   = !brutepitch;
            }
            angles.y = fmod(angles.y + 180.0f, 360.0f);
            if (angles.y < 0)
                angles.y += 360.0f;
            angles.y -= 180.0f;
            if (quota < 0.8f)
                switch (g_Settings.brute.brutenum[i])
                {
                case 0:
                    break;
                case 1:
                    angles.y += 180.0f;
                    break;
                case 2:
                    angles.y -= 90.0f;
                    break;
                case 3:
                    angles.y += 90.0f;
                    break;
                case 4:
                    angles.y -= 180.0f;
                    break;
                case 5:
                    angles.y = 0.0f;
                    break;
                }
            if (brutepitch || quota < 0.8f)
                switch (g_Settings.brute.brutenum[i] % 4)
                {
                case 0:
                    break;
                case 1:
                    angles.x = -89.0f;
                    break;
                case 2:
                    angles.x = 89.0f;
                    break;
                case 3:
                    angles.x = 0.0f;
                    break;
                }
        }
    }
}

