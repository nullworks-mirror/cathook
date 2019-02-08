/*
 * EffectGlow.cpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#include <visual/EffectGlow.hpp>
#include <MiscTemporary.hpp>
#include <hacks/Aimbot.hpp>
#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool enable{ "glow.enable", "false" };
static settings::Bool health{ "glow.health", "false" };
static settings::Bool teammates{ "glow.show.teammates", "false" };
static settings::Bool players{ "glow.show.players", "true" };
static settings::Bool medkits{ "glow.show.medkits", "false" };
static settings::Bool ammobox{ "glow.show.ammoboxes", "false" };
static settings::Bool buildings{ "glow.show.buildings", "true" };
static settings::Bool stickies{ "glow.show.stickies", "true" };
static settings::Bool teammate_buildings{ "glow.show.teammate-buildings", "false" };
static settings::Bool show_powerups{ "glow.show.powerups", "true" };
static settings::Bool weapons_white{ "glow.white-weapons", "true" };
static settings::Bool glowself{ "glow.self", "true" };
static settings::Bool rainbow{ "glow.self-rainbow", "true" };

CScreenSpaceEffectRegistration *CScreenSpaceEffectRegistration::s_pHead = NULL;
IScreenSpaceEffectManager *g_pScreenSpaceEffects                        = nullptr;
CScreenSpaceEffectRegistration **g_ppScreenSpaceRegistrationHead        = nullptr;
CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration(const char *pName, IScreenSpaceEffect *pEffect)
{
    logging::Info("Creating new effect '%s', head: 0x%08x", pName, *g_ppScreenSpaceRegistrationHead);
    m_pEffectName                    = pName;
    m_pEffect                        = pEffect;
    m_pNext                          = *g_ppScreenSpaceRegistrationHead;
    *g_ppScreenSpaceRegistrationHead = this;
    logging::Info("New head: 0x%08x", *g_ppScreenSpaceRegistrationHead);
}

namespace effect_glow
{

struct ShaderStencilState_t
{
    bool m_bEnable;
    StencilOperation_t m_FailOp;
    StencilOperation_t m_ZFailOp;
    StencilOperation_t m_PassOp;
    StencilComparisonFunction_t m_CompareFunc;
    int m_nReferenceValue;
    uint32 m_nTestMask;
    uint32 m_nWriteMask;

    ShaderStencilState_t()
    {
        Reset();
    }

    inline void Reset()
    {
        m_bEnable = false;
        m_PassOp = m_FailOp = m_ZFailOp = STENCILOPERATION_KEEP;
        m_CompareFunc                   = STENCILCOMPARISONFUNCTION_ALWAYS;
        m_nReferenceValue               = 0;
        m_nTestMask = m_nWriteMask = 0xFFFFFFFF;
    }

    inline void SetStencilState(CMatRenderContextPtr &pRenderContext) const
    {
        pRenderContext->SetStencilEnable(m_bEnable);
        pRenderContext->SetStencilFailOperation(m_FailOp);
        pRenderContext->SetStencilZFailOperation(m_ZFailOp);
        pRenderContext->SetStencilPassOperation(m_PassOp);
        pRenderContext->SetStencilCompareFunction(m_CompareFunc);
        pRenderContext->SetStencilReferenceValue(m_nReferenceValue);
        pRenderContext->SetStencilTestMask(m_nTestMask);
        pRenderContext->SetStencilWriteMask(m_nWriteMask);
    }
};

static ShaderStencilState_t SS_First{};
static ShaderStencilState_t SS_Last{};

CatCommand fix_black_glow("fix_black_glow", "Fix Black Glow", []() {
    effect_glow::g_EffectGlow.Shutdown();
    effect_glow::g_EffectGlow.Init();
});

void EffectGlow::Init()
{
    if (init)
        return;
    logging::Info("Init Glow...");
    {
        SS_First.m_bEnable         = true;
        SS_First.m_PassOp          = STENCILOPERATION_REPLACE;
        SS_First.m_nWriteMask      = 1;
        SS_First.m_nReferenceValue = 1;
    }
    {
        SS_Last.m_bEnable     = true;
        SS_Last.m_nWriteMask  = 0; // We're not changing stencil
        SS_Last.m_nTestMask   = 255;
        SS_Last.m_CompareFunc = STENCILCOMPARISONFUNCTION_EQUAL;
    }
    {
        KeyValues *kv = new KeyValues("UnlitGeneric");
        kv->SetString("$basetexture", "vgui/white_additive");
        kv->SetInt("$ignorez", 1);
        mat_unlit_z.Init("__cathook_glow_unlit_z", kv);
    }
    IF_GAME(IsTF2())
    {
        mat_halo = g_IMaterialSystem->FindMaterial("dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, false);
        mat_halo->FindVar("$C0_X", NULL, false)->SetFloatValue(1.0f);
        mat_glow      = g_IMaterialSystem->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, false);
        mat_fullframe = g_IMaterialSystem->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);
    }
    else
    {
        mat_halo = g_IMaterialSystemHL->FindMaterial("dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, false);
        mat_halo->FindVar("$C0_X", NULL, false)->SetFloatValue(1.0f);
        mat_glow      = g_IMaterialSystemHL->FindMaterial("dev/glow_color", TEXTURE_GROUP_OTHER, false);
        mat_fullframe = g_IMaterialSystemHL->FindTexture("_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET);
    }
    logging::Info("Init done!");
    init = true;
}

rgba_t EffectGlow::GlowColor(IClientEntity *entity)
{
    static CachedEntity *ent;
    static IClientEntity *owner;

    ent = ENTITY(entity->entindex());
    if (CE_BAD(ent))
        return colors::white;
    if (ent == hacks::shared::aimbot::CurrentTarget())
        return colors::pink;
    if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(entity))
    {
        owner = re::C_TFWeaponBase::GetOwnerViaInterface(entity);
        if (owner)
        {
            return GlowColor(owner);
        }
    }
    switch (ent->m_Type())
    {
    case ENTITY_BUILDING:
        if (health)
        {
            return colors::Health(ent->m_iHealth(), ent->m_iMaxHealth());
        }
        break;
    case ENTITY_PLAYER:
        if (ent->m_IDX == LOCAL_E->m_IDX)
        {
            if (rainbow)
                return colors::RainbowCurrent();
            else if (LOCAL_E->m_iTeam() == TEAM_BLU)
                return colors::blu;
            else
                return colors::red;
        }
        if (health && playerlist::IsDefault(ent))
        {
            return colors::Health(ent->m_iHealth(), ent->m_iMaxHealth());
        }
        break;
    }

    return colors::EntityF(ent);
}

bool EffectGlow::ShouldRenderGlow(IClientEntity *entity)
{
    static CachedEntity *ent;
    if (entity->entindex() < 0)
        return false;
    ent = ENTITY(entity->entindex());
    if (CE_BAD(ent))
        return false;
    if (ent->m_IDX == LOCAL_E->m_IDX && !glowself)
        return false;
    switch (ent->m_Type())
    {
    case ENTITY_BUILDING:
        if (!buildings)
            return false;
        if (!ent->m_bEnemy() && !(teammate_buildings || teammates))
            return false;
        return true;
    case ENTITY_PLAYER:
        if (!players)
            return false;
        if (!teammates && !ent->m_bEnemy() && playerlist::IsDefault(ent))
            return false;
        if (CE_BYTE(ent, netvar.iLifeState) != LIFE_ALIVE)
            return false;
        return true;
        break;
    case ENTITY_PROJECTILE:
        if (!ent->m_bEnemy())
            return false;
        if (stickies && ent->m_iClassID() == CL_CLASS(CTFGrenadePipebombProjectile))
        {
            return true;
        }
        break;
    case ENTITY_GENERIC:
        const auto &type = ent->m_ItemType();
        if (type >= ITEM_HEALTH_SMALL && type <= ITEM_HEALTH_LARGE)
        {
            return *medkits;
        }
        else if (type >= ITEM_AMMO_SMALL && type <= ITEM_AMMO_LARGE)
        {
            return *ammobox;
        }
        else if (type >= ITEM_POWERUP_FIRST && type <= ITEM_POWERUP_LAST)
        {
            return *show_powerups; // lol? whoever made it return const char *powerups is psmart
        }
        break;
    }
    return false;
}

void EffectGlow::DrawEntity(IClientEntity *entity)
{
    static IClientEntity *attach;
    static int passes;
    passes = 0;

    entity->DrawModel(1);
    attach = g_IEntityList->GetClientEntity(*(int *) ((uintptr_t) entity + netvar.m_Collision - 24) & 0xFFF);
    while (attach && passes++ < 32)
    {
        if (attach->ShouldDraw())
        {
            if (weapons_white && entity->GetClientClass()->m_ClassID == RCC_PLAYER && re::C_BaseCombatWeapon::IsBaseCombatWeapon(attach))
            {
                rgba_t mod_original;
                g_IVRenderView->GetColorModulation(mod_original.rgba);
                g_IVRenderView->SetColorModulation(colors::white);
                attach->DrawModel(1);
                g_IVRenderView->SetColorModulation(mod_original.rgba);
            }
            else
                attach->DrawModel(1);
        }
        attach = g_IEntityList->GetClientEntity(*(int *) ((uintptr_t) attach + netvar.m_Collision - 20) & 0xFFF);
    }
}

void EffectGlow::Render(int x, int y, int w, int h)
{
    if (!enable)
        return;
    if (!init)
        Init();
    if (!isHackActive() || (g_IEngine->IsTakingScreenshot() && clean_screenshots) || g_Settings.bInvalid)
        return;
    CMatRenderContextPtr ptr(GET_RENDER_CONTEXT);

    ptr->ClearColor4ub(0, 0, 0, 255);
    ptr->PushRenderTargetAndViewport(mat_fullframe);
    ptr->ClearBuffers(true, true);
    ptr->OverrideAlphaWriteEnable(true, true);
    ptr->PopRenderTargetAndViewport();
    ptr->ClearStencilBufferRectangle(0, 0, w, h, 0);
    ptr->DepthRange(0.0f, 0.01f);

    SS_First.SetStencilState(ptr);

    drawing = true;
    for (int i = 1; i <= HIGHEST_ENTITY; i++)
    {
        IClientEntity *ent = g_IEntityList->GetClientEntity(i);
        if (ent && !ent->IsDormant() && ShouldRenderGlow(ent))
        {
            ptr->PushRenderTargetAndViewport(mat_fullframe);
            g_IVRenderView->SetColorModulation(GlowColor(ent));
            g_IVModelRender->ForcedMaterialOverride(mat_glow);
            DrawEntity(ent);
            ptr->PopRenderTargetAndViewport();

            g_IVRenderView->SetBlend(0.0f);
            g_IVModelRender->ForcedMaterialOverride(mat_unlit_z);
            DrawEntity(ent);
            g_IVRenderView->SetBlend(1.0f);
        }
    }
    drawing = false;

    SS_Last.SetStencilState(ptr);

    g_IVModelRender->ForcedMaterialOverride(NULL);

    ptr->DepthRange(0.0f, 1.0f);
    ptr->DrawScreenSpaceRectangle(mat_halo, x, y, w, h, 0, 0, w, h, w, h);
    ptr->SetStencilEnable(false);
}

EffectGlow g_EffectGlow;
CScreenSpaceEffectRegistration *g_pEffectGlow = nullptr;
} // namespace effect_glow
