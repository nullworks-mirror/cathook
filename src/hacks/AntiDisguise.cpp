/*
 * AntiDisguise.cpp
 *
 *  Created on: Nov 16, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

namespace hacks
{
namespace tf2
{
namespace antidisguise
{

static CatVar enabled(CV_SWITCH, "antidisguise", "0", "Remove spy disguise",
                      "Removes the disguise from spys\nUsefull for aimbot");
static CatVar no_invisibility(CV_SWITCH, "no_invis", "0", "Remove Invisibility",
                              "Useful with chams!");

void Draw()
{
    CachedEntity *ent;
    if (no_invisibility)
        for (int i = 0; i < 32 && i < HIGHEST_ENTITY; i++)
        {
            ent = ENTITY(i);
            if (CE_BAD(ent))
                continue;
            if (ent->m_Type == ENTITY_PLAYER)
            {
                if (CE_INT(ent, netvar.iClass) == tf_class::tf_spy)
                {
                    RemoveCondition<TFCond_Cloaked>(ent);
                    RemoveCondition<TFCond_CloakFlicker>(ent);
                }
            }
        }
    if (!enabled)
        return;
    for (int i = 0; i < 32 && i < HIGHEST_ENTITY; i++)
    {
        ent = ENTITY(i);
        if (CE_BAD(ent))
            continue;
        if (ent->m_Type == ENTITY_PLAYER)
        {
            if (CE_INT(ent, netvar.iClass) == tf_class::tf_spy)
            {
                RemoveCondition<TFCond_Disguised>(ent);
            }
        }
    }
}
}
}
}
