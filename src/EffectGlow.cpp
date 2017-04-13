/*
 * EffectGlow.cpp
 *
 *  Created on: Apr 13, 2017
 *      Author: nullifiedcat
 */

#include "common.h"
#include "EffectGlow.hpp"

IMaterialSystem* materials = nullptr;

// FIXME move to sdk
CScreenSpaceEffectRegistration *CScreenSpaceEffectRegistration::s_pHead = NULL;
IScreenSpaceEffectManager* g_pScreenSpaceEffects = nullptr;
CScreenSpaceEffectRegistration** g_ppScreenSpaceRegistrationHead = nullptr;
CScreenSpaceEffectRegistration::CScreenSpaceEffectRegistration( const char *pName, IScreenSpaceEffect *pEffect )
{
	logging::Info("Creating new effect '%s', head: 0x%08x", pName, *g_ppScreenSpaceRegistrationHead);
	m_pEffectName = pName;
	m_pEffect = pEffect;
	m_pNext = *g_ppScreenSpaceRegistrationHead;
	*g_ppScreenSpaceRegistrationHead = this;
	logging::Info("New head: 0x%08x", *g_ppScreenSpaceRegistrationHead);
}

static CTextureReference tr_cathook_rt;

/*IMaterial* GetRenderTarget() {
	if (!tr_cathook_rt) {
		tr_cathook_rt.InitRenderTarget(256, 256, RT_SIZE_FULL_FRAME_BUFFER, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false, "cathook_rt");
	}
	return tr_cathook_rt.;
}*/

IMaterial* GetGlowMaterial() {
	return vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/glow_color", TEXTURE_GROUP_OTHER, true, 0);
	/*static IMaterial* material = nullptr;
	if (!material) {
		KeyValues* kv = new KeyValues("UnlitGeneric");
		kv->SetString("$basetexture", "_rt_FullFrameFB");
		kv->SetInt("$ignorez", 1);
		material = g_IMaterialSystem->CreateMaterial("cathook/glow", kv);
	}
	return material;*/
}

void EffectGlow::Render(int x, int y, int w, int h) {
	if (!materials) materials = g_IMaterialSystem;
	CMatRenderContextPtr ptr(vfunc<IMatRenderContext*(*)(IMaterialSystem*)>(g_IMaterialSystem, 100, 0)(g_IMaterialSystem));
	//ITexture* rt_original = ptr->GetRenderTarget();
	ITexture *pRtFullFrame = NULL;
	pRtFullFrame = vfunc<ITexture*(*)(IMaterialSystem*, const char*, const char*, bool, int)>(g_IMaterialSystem, 81)(g_IMaterialSystem, 	"_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET, 1, 0);//materials->FindTexture( "_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET );
	logging::Info("%s", pRtFullFrame->GetName());
	ptr->SetRenderTarget(pRtFullFrame);
	ptr->Viewport(0, 0, 1920, 1080);
	ptr->PushRenderTargetAndViewport();
	Vector orig_modulation;
	g_IVRenderView->GetColorModulation(orig_modulation.Base());
	Vector red(1.0f, 0.1f, 0.1f);
	g_IVRenderView->SetColorModulation(red.Base());
	g_IStudioRender->ForcedMaterialOverride(GetGlowMaterial());
	g_IVRenderView->SetBlend(1.0f);
	for (int i = 1; i < 8; i++) {
		IClientEntity* ent = g_IEntityList->GetClientEntity(i);
		if (!ent->IsDormant() && NET_BYTE(ent, netvar.iLifeState) == LIFE_ALIVE) {
			logging::Info("Drawing %d", i);
			ent->DrawModel(1);
		}
	}
	logging::Info("Setting modulation");
	g_IVRenderView->SetColorModulation(orig_modulation.Base());
	logging::Info("z");
	logging::Info("%i", g_IStudioRender->GetNumAmbientLightSamples());
	g_IStudioRender->ForcedMaterialOverride(0);
	logging::Info("x");
	ptr->PopRenderTargetAndViewport();
	logging::Info("Done...?");
	//ptr->SetRenderTarget(rt_original);
}

EffectGlow g_EffectGlow;
CScreenSpaceEffectRegistration* g_pEffectGlow = nullptr;
