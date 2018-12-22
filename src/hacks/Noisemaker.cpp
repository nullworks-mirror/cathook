/*
 * Noisemaker.cpp
 *
 *  Created on: Feb 2, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool enable{ "noisemaker-spam.enable", "false" };

namespace hacks::tf2::noisemaker
{

static void CreateMove()
{
    if (enable && CE_GOOD(LOCAL_E) && LOCAL_E->m_bAlivePlayer())
    {
        if (g_GlobalVars->framecount % 100 == 0)
        {
            KeyValues *kv = new KeyValues("+use_action_slot_item_server");
            g_IEngine->ServerCmdKeyValues(kv);
            KeyValues *kv2 = new KeyValues("-use_action_slot_item_server");
            g_IEngine->ServerCmdKeyValues(kv2);
        }
    }
}
static InitRoutine EC([]() { EC::Register<EC::CreateMove>(CreateMove, "Noisemaker", EC::average); });
} // namespace hacks::tf2::noisemaker
