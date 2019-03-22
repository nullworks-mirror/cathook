/*
 * AutoTaunt.cpp
 *
 *  Created on: Jul 27, 2017
 *      Author: nullifiedcat
 */

#include <settings/Float.hpp>
#include "common.hpp"
#include "hack.hpp"

static settings::Bool enable{ "autotaunt.enable", "false" };
static settings::Float chance{ "autotaunt.chance", "8" };
static settings::Float safety{ "autotaunt.safety-distance", "0" };

namespace hacks::tf::autotaunt
{

class AutoTauntListener : public IGameEventListener2
{
public:
    virtual void FireGameEvent(IGameEvent *event)
    {
        if (!enable)
        {
            return;
        }
        if (g_IEngine->GetPlayerForUserID(event->GetInt("attacker")) == g_IEngine->GetLocalPlayer())
        {
            bool nearby = false;
            for (int i = 1; i < g_IEngine->GetMaxClients(); i++)
            {
                auto ent = ENTITY(i);
                if (CE_GOOD(ent) && ent->m_flDistance() < *safety)
                {
                    nearby = true;
                    break;
                }
            }
            if (!nearby && RandomFloat(0, 100) <= float(chance))
            {
                hack::ExecuteCommand("taunt");
            }
        }
    }
};

AutoTauntListener listener;

InitRoutine init([]() {
    g_IEventManager2->AddListener(&listener, "player_death", false);
    EC::Register(EC::Shutdown, []() { g_IEventManager2->RemoveListener(&listener); }, "Shutdown_Autotaunt");
});
} // namespace hacks::tf::autotaunt
