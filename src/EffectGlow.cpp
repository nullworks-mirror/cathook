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

/*IMaterial* GetRenderTarget() {
	if (!tr_cathook_rt) {
		tr_cathook_rt.InitRenderTarget(256, 256, RT_SIZE_FULL_FRAME_BUFFER, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false, "cathook_rt");
	}
	return tr_cathook_rt.;
}*/

IMaterial* GetGlowMaterial() {
	return vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/glow_color", TEXTURE_GROUP_OTHER, true, 0);
}

static CatVar glow_experimental(CV_SWITCH, "glow_experimental", "0", "Experimental Glow");

void EffectGlow::Init() {
	logging::Info("Init EffectGlow...");
	if (!materials) materials = g_IMaterialSystem;
	logging::Info("materials: 0x%08x", materials);
	rt_A.InitRenderTarget(1920/2, 1080/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "__cathook_glow_rta");
	rt_B.InitRenderTarget(1920/2, 1080/2, RT_SIZE_DEFAULT, IMAGE_FORMAT_RGBA8888, MATERIAL_RT_DEPTH_SEPARATE, false, "__cathook_glow_rtb");
	logging::Info("Textures init!");
	//rt_B.InitRenderTarget(256, 256, RT_SIZE_FULL_FRAME_BUFFER, IMAGE_FORMAT_ARGB8888, MATERIAL_RT_DEPTH_NONE, false, "__cathook_glow_rtB");
	KeyValues *kv2 = new KeyValues( "VertexLitGeneric" );
	kv2->SetString( "$basetexture", "vgui/white" );
	kv2->SetInt( "$selfillum", 1 );
	kv2->SetString( "$selfillummask", "vgui/white" );
	kv2->SetInt( "$vertexalpha", 1 );
	kv2->SetInt( "$model", 1 );
	glow_material.Init( "__cathook_glow_mat_color", TEXTURE_GROUP_CLIENT_EFFECTS, kv2 );
	glow_material->Refresh();
	KeyValues* kv = new KeyValues("UnlitGeneric");
	kv->SetString("$basetexture", "_rt_FullFrameFB");
	kv->SetInt("$additive", 1);
	result_material.Init("__cathook_glow_mat", TEXTURE_GROUP_CLIENT_EFFECTS, kv);
	result_material->Refresh();
	logging::Info("Material init!");
	//dev_glow_color.Init(vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/glow_color", TEXTURE_GROUP_OTHER, true, 0));
	dev_bloomdadd.Init(vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/bloomadd", TEXTURE_GROUP_OTHER, true, 0));
	dev_blurfilterx.Init(vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/blurfilterx", TEXTURE_GROUP_OTHER, true, 0));
	dev_blurfiltery.Init(vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/blurfiltery", TEXTURE_GROUP_OTHER, true, 0));
	dev_halo_add_to_screen.Init(vfunc<IMaterial*(*)(IMaterialSystem*, const char*, const char*, bool, const char*)>(g_IMaterialSystem, 73)(g_IMaterialSystem, "dev/halo_add_to_screen", TEXTURE_GROUP_OTHER, true, 0));

	logging::Info("Init done!");
	init = true;
}

void EffectGlow::BeginRenderGlow() {
	CMatRenderContextPtr ptr(vfunc<IMatRenderContext*(*)(IMaterialSystem*)>(g_IMaterialSystem, 100, 0)(g_IMaterialSystem));
	ptr->PushRenderTargetAndViewport(rt_A);
	g_IVModelRender->SuppressEngineLighting(true);
	g_IVRenderView->GetColorModulation(orig_modulation);
	static Vector red(1.0f, 0.1f, 0.1f);
	g_IVRenderView->SetColorModulation(red.Base());
	g_IStudioRender->ForcedMaterialOverride(glow_material);
	g_IVRenderView->SetBlend(1.0f);
}

void EffectGlow::EndRenderGlow() {
	CMatRenderContextPtr ptr(vfunc<IMatRenderContext*(*)(IMaterialSystem*)>(g_IMaterialSystem, 100, 0)(g_IMaterialSystem));
	g_IVRenderView->SetColorModulation(orig_modulation);
	g_IStudioRender->ForcedMaterialOverride(nullptr);
	g_IVModelRender->SuppressEngineLighting(false);
	ptr->PopRenderTargetAndViewport();
}

void EffectGlow::RenderGlow(int idx) {
	CMatRenderContextPtr ptr(vfunc<IMatRenderContext*(*)(IMaterialSystem*)>(g_IMaterialSystem, 100, 0)(g_IMaterialSystem));
	ptr->PushRenderTargetAndViewport( rt_A );

	g_IVModelRender->SuppressEngineLighting( true );

	// Set the glow tint since selfillum trumps color modulation
	IMaterialVar *var = glow_material->FindVar( "$selfillumtint", NULL, false );
	static float color[4] = { 1.0f, 0.0f, 0.0f, 1.0f };
	var->SetVecValue( color, 4 ); // Fixed compilation error
	var = glow_material->FindVar( "$alpha", NULL, false );
	var->SetFloatValue( color[3] ); // Fixed compilation error

	g_IVModelRender->ForcedMaterialOverride( glow_material );
		IClientEntity* ent = g_IEntityList->GetClientEntity(idx);
		if (ent) ent->DrawModel(1);
	g_IVModelRender->ForcedMaterialOverride( NULL );

	g_IVModelRender->SuppressEngineLighting( false );

	ptr->PopRenderTargetAndViewport();


	/*IClientEntity* ent = g_IEntityList->GetClientEntity(idx);
	if (ent) {
		ent->DrawModel(1);
	}*/
}

void EffectGlow::Render(int x, int y, int w, int h) {
	if (!init) Init();
	if (!glow_experimental) return;
	CMatRenderContextPtr ptr(vfunc<IMatRenderContext*(*)(IMaterialSystem*)>(g_IMaterialSystem, 100, 0)(g_IMaterialSystem));

	//ptr->Viewport(x, y, w, h);

	ITexture* rt = ptr->GetRenderTarget();

	IMaterialVar* var;
	var = dev_blurfilterx->FindVar("$basetexture", nullptr);
	var->SetTextureValue(rt_A);
	var = dev_blurfiltery->FindVar("$basetexture", nullptr);
	var->SetTextureValue(rt_B);
	var = result_material->FindVar("$basetexture", nullptr);
	var->SetTextureValue(rt_A);
	//var = dev_blurfilterx->FindVar("$bloomamount", nullptr);
	//var->SetFloatValue(10);
	var = dev_blurfiltery->FindVar("$bloomamount", nullptr);
	var->SetFloatValue(10);

	ptr->ClearColor4ub(0, 0, 0, 255);
	ptr->PushRenderTargetAndViewport(rt_A);
		ptr->ClearBuffers(true, true);
	ptr->PopRenderTargetAndViewport();
	ptr->PushRenderTargetAndViewport(rt_B);
		ptr->ClearBuffers(true, true);
	ptr->PopRenderTargetAndViewport();

	ptr->ClearStencilBufferRectangle( 0, 0, 1920, 1080, 0 );

	BeginRenderGlow();
	for (int i = 1; i < 32; i++) {
		IClientEntity* ent = g_IEntityList->GetClientEntity(i);
		if (ent && !ent->IsDormant() && NET_BYTE(ent, netvar.iLifeState) == LIFE_ALIVE) {
			//BeginRenderGlow();
			RenderGlow(i);
			//EndRenderGlow();
		}
	}

	ptr->PushRenderTargetAndViewport( rt_B );
	ptr->Viewport(x, y, w, h);
	ptr->DrawScreenSpaceQuad( dev_blurfilterx );
	ptr->PopRenderTargetAndViewport();

	ptr->PushRenderTargetAndViewport( rt_A );
	ptr->Viewport(x, y, w, h);
	ptr->DrawScreenSpaceQuad( dev_blurfiltery );
	ptr->PopRenderTargetAndViewport();

	ptr->SetRenderTarget(rt);
	ptr->Viewport(x, y, w, h);

	ptr->DrawScreenSpaceQuad(result_material);
	var = result_material->FindVar("$basetexture", nullptr);
	//var->SetTextureValue(rt_B);
	//ptr->DrawScreenSpaceQuad(result_material);
}

EffectGlow g_EffectGlow;
CScreenSpaceEffectRegistration* g_pEffectGlow = nullptr;
