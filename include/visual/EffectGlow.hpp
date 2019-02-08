/*
 * EffectGlow.hpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"
#include "core/sdk.hpp"

namespace effect_glow
{

class EffectGlow : public IScreenSpaceEffect
{
public:
    virtual void Init();
    virtual void Shutdown()
    {
        if (init)
        {
            mat_unlit_z.Shutdown();
            init = false;
        }
    }

    inline virtual void SetParameters(KeyValues *params)
    {
    }

    virtual void Render(int x, int y, int w, int h);

    inline virtual void Enable(bool bEnable)
    {
        enabled = bEnable;
    }
    inline virtual bool IsEnabled()
    {
        return enabled;
    }

    void DrawEntity(IClientEntity *entity);
    rgba_t GlowColor(IClientEntity *entity);
    bool ShouldRenderGlow(IClientEntity *entity);

public:
    bool init{ false };
    bool drawing{ false };
    bool enabled;
    CMaterialReference mat_unlit_z;
    IMaterial *mat_halo;
    IMaterial *mat_glow;
    ITexture *mat_fullframe;
};

extern EffectGlow g_EffectGlow;
extern CScreenSpaceEffectRegistration *g_pEffectGlow;
} // namespace effect_glow
