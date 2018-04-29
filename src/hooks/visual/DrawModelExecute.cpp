/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(DrawModelExecute, void, IVModelRender *this_,
                     const DrawModelState_t &state,
                     const ModelRenderInfo_t &info, matrix3x4_t *bone)
{
    static const DrawModelExecute_t original =
            (DrawModelExecute_t) hooks::modelrender.GetMethod(
                    offsets::DrawModelExecute());
    static const char *name;
    static std::string sname;
    static IClientUnknown *unk;
    static IClientEntity *ent;

    if (!cathook ||
        !(spectator_target || no_arms || no_hats ||
          (clean_screenshots && g_IEngine->IsTakingScreenshot())))
    {
        original(_this, state, info, matrix);
        return;
    }

    PROF_SECTION(DrawModelExecute);

    if (no_arms || no_hats)
    {
        if (info.pModel)
        {
            name = g_IModelInfo->GetModelName(info.pModel);
            if (name)
            {
                sname = name;
                if (no_arms && sname.find("arms") != std::string::npos)
                {
                    return;
                }
                else if (no_hats &&
                         sname.find("player/items") != std::string::npos)
                {
                    return;
                }
            }
        }
    }

    unk = info.pRenderable->GetIClientUnknown();
    if (unk)
    {
        ent = unk->GetIClientEntity();
        if (ent)
        {
            if (ent->entindex() == spectator_target)
            {
                return;
            }
        }
        if (ent && !effect_chams::g_EffectChams.drawing &&
            effect_chams::g_EffectChams.ShouldRenderChams(ent))
        {
            return;
        }
    }

    return original::DrawModelExecute(this_, state, info, bone);
}
}