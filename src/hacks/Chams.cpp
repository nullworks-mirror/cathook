/*
 * Chams.cpp
 *
 *  Created on: Apr 15, 2017
 *      Author: nullifiedcat
 */

#include "../common.h"

namespace hacks { namespace shared { namespace chams {

static CatVar enable(CV_SWITCH, "chams_enable", "0", "Enable");

static bool init = false;

CMaterialReference mat_unlit;
CMaterialReference mat_unlit_z;
CMaterialReference mat_lit;
CMaterialReference mat_lit_z;

void Init() {
	if (!materials) materials = g_IMaterialSystem;
	{
		KeyValues* kv = new KeyValues("UnlitGeneric");
		kv->SetString("$basetexture", "vgui/white_additive");
		kv->SetInt("$ignorez", 0);
		mat_unlit.Init("__cathook_chams_unlit", kv);
	}
	{
		KeyValues* kv = new KeyValues("UnlitGeneric");
		kv->SetString("$basetexture", "vgui/white_additive");
		kv->SetInt("$ignorez", 1);
		mat_unlit_z.Init("__cathook_chams_unlit_z", kv);
	}
	{
		KeyValues* kv = new KeyValues("VertexLitGeneric");
		kv->SetString("$basetexture", "vgui/white_additive");
		kv->SetInt("$ignorez", 0);
		kv->SetInt("$halflambert", 1);
		mat_lit.Init("__cathook_chams_lit", kv);
	}
	{
		KeyValues* kv = new KeyValues("VertexLitGeneric");
		kv->SetString("$basetexture", "vgui/white_additive");
		kv->SetInt("$ignorez", 1);
		kv->SetInt("$halflambert", 1);
		mat_lit_z.Init("__cathook_chams_lit_z", kv);
	}
	init = true;
}

void DrawModelExecute(IVModelRender* _this, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* matrix) {
	if (!enable) return;
	if (!init) {
		Init();
	}
	//std::string name(g_IModelInfo->GetModelName(info.pModel));
	IClientUnknown* unknown = info.pRenderable->GetIClientUnknown();
	if (unknown) {
		IClientEntity* entity = unknown->GetIClientEntity();
		//logging::Info("entity: 0x%08x", entity);
		if (entity && !entity->IsDormant()) {
			//logging::Info("lit player");
			// IsBaseCombatWeapon
			if (vfunc<bool(*)(IClientEntity*)>(entity, 0xBE, 0)(entity)) {
				IClientEntity* owner = vfunc<IClientEntity*(*)(IClientEntity*)>(entity, 0x1C3, 0)(entity);
				if (owner) {
					int color = colors::EntityF(ENTITY(owner->entindex()));
					unsigned char _b = (color >> 16) & 0xFF;
					unsigned char _g = (color >> 8)  & 0xFF;
					unsigned char _r = (color) & 0xFF;
					float color_1[] = { (float)_r / 255.0f, (float)_g / 255.0f, (float)_b / 255.0f };
					float color_2[] = { color_1[0] * 0.6f, color_1[1] * 0.6f, color_1[2] * 0.6f };
					mat_unlit_z->AlphaModulate(1.0f);
					g_IVRenderView->SetColorModulation(color_1);
					g_IVModelRender->ForcedMaterialOverride(mat_lit_z);
					((DrawModelExecute_t)(hooks::hkIVModelRender->GetMethod(hooks::offDrawModelExecute)))(_this, state, info, matrix);
					mat_unlit->AlphaModulate(1.0f);
					g_IVRenderView->SetColorModulation(color_2);
					g_IVModelRender->ForcedMaterialOverride(mat_lit);
				}
			} else if (entity->GetClientClass()->m_ClassID == g_pClassID->C_Player) {
				int color = colors::EntityF(ENTITY(entity->entindex()));
				unsigned char _b = (color >> 16) & 0xFF;
				unsigned char _g = (color >> 8)  & 0xFF;
				unsigned char _r = (color) & 0xFF;
				float color_1[] = { (float)_r / 255.0f, (float)_g / 255.0f, (float)_b / 255.0f };
				float color_2[] = { color_1[0] * 0.6f, color_1[1] * 0.6f, color_1[2] * 0.6f };
				mat_unlit_z->AlphaModulate(1.0f);
				g_IVRenderView->SetColorModulation(color_1);
				g_IVModelRender->ForcedMaterialOverride(mat_lit_z);
				((DrawModelExecute_t)(hooks::hkIVModelRender->GetMethod(hooks::offDrawModelExecute)))(_this, state, info, matrix);
				mat_unlit->AlphaModulate(1.0f);
				g_IVRenderView->SetColorModulation(color_2);
				g_IVModelRender->ForcedMaterialOverride(mat_lit);
			}


		}
	}
}

}}}
