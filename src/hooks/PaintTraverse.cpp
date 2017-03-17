/*
 * PaintTraverse.cpp
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#include "PaintTraverse.h"
#include "../common.h"
#include "../hack.h"
#include "hookedmethods.h"
#include "../gui/GUI.h"
#include "../segvcatch/segvcatch.h"
#include "../profiler.h"

CatVar clean_screenshots(CV_SWITCH, "clean_screenshots", "1", "Clean screenshots", "Don't draw visuals while taking a screenshot");
CatVar disable_visuals(CV_SWITCH, "no_visuals", "0", "Disable ALL drawing", "Completely hides cathook");
CatVar no_zoom(CV_SWITCH, "no_zoom", "1", "Disable scope", "Disables black scope overlay");
CatVar logo(CV_SWITCH, "logo", "1", "Show logo", "Show cathook text in top left corner");

void PaintTraverse_hook(void* p, unsigned int vp, bool fr, bool ar) {
#if DEBUG_SEGV == true
	if (!segvcatch::handler_fpe || !segvcatch::handler_segv) {
		segvcatch::init_segv();
		segvcatch::init_fpe();
	}
#endif
	SEGV_BEGIN;
	static unsigned long panel_focus = 0;
	static unsigned long panel_scope = 0;
	static unsigned long panel_top = 0;
	static bool draw_flag = false;
	bool call_default = true;
	if (cathook && panel_scope && no_zoom && vp == panel_scope) call_default = false;
	if (cathook) {
		bool vis = gui_visible;
		g_ISurface->SetCursorAlwaysVisible(vis);
	}


	if (call_default) SAFE_CALL(((PaintTraverse_t*)hooks::hkPanel->GetMethod(hooks::offPaintTraverse))(p, vp, fr, ar));
	PROF_SECTION(PaintTraverse);
	if (vp == panel_top) draw_flag = true;
	if (!cathook) return;
	// Because of single-multi thread shit I'm gonna put this thing riiiight here.

	static bool autoexec_done = false;
	if (!autoexec_done) {
		g_IEngine->ExecuteClientCmd("exec cat_autoexec");
		g_IEngine->ExecuteClientCmd("cat_killsay_reload");
		g_IEngine->ExecuteClientCmd("cat_spam_reload");
		autoexec_done = true;
	}

	if (!panel_top) {
		const char* name = g_IPanel->GetName(vp);
		if (strlen(name) > 4) {
			if (name[0] == 'M' && name[3] == 'S') {
				panel_top = vp;
			}
			if (name[0] == 'F' && name[5] == 'O') {
				panel_focus = vp;
			}
		}
	}
	if (!panel_scope) {
		if (!strcmp(g_IPanel->GetName(vp), "HudScope")) {
			panel_scope = vp;
		}
	}
	if (!g_IEngine->IsInGame()) {
		g_Settings.bInvalid = true;
	}

	if (disable_visuals) return;
	if (clean_screenshots && g_IEngine->IsTakingScreenshot()) return;

	ResetStrings();

	if (vp != panel_focus) return;
	if (!draw_flag) return;
	draw_flag = false;

	if (logo) {
		AddSideString(colors::RainbowCurrent(), "cathook by d4rkc4t");
	}
	if (CE_GOOD(g_pLocalPlayer->entity) && !g_Settings.bInvalid) {
		if (TF) SAFE_CALL(hacks::tf2::antidisguise::Draw());
		SAFE_CALL(HACK_DRAW(Misc));
		SAFE_CALL(HACK_DRAW(ESP));
		if (TF) SAFE_CALL(hacks::tf::spyalert::Draw());
		Vector screen;
		for (int i = 0; i < HIGHEST_ENTITY; i++) {
			CachedEntity* ce = gEntityCache.GetEntity(i);
			if (CE_BAD(ce)) continue;
			if (ce->m_ESPOrigin.IsZero(1.0f))
				if (!draw::EntityCenterToScreen(ce, screen)) continue;
			for (int j = 0; j < ce->m_nESPStrings; j++) {
				ESPStringCompound str = ce->GetESPString(j);
				int color = str.m_bColored ? str.m_nColor : ce->m_ESPColorFG;
				if (!ce->m_ESPOrigin.IsZero(1.0)) {
					draw::String(fonts::ESP, ce->m_ESPOrigin.x, ce->m_ESPOrigin.y, color, 2, str.m_String);
					ce->m_ESPOrigin.y += 12;
				} else {
					auto l = draw::GetStringLength(fonts::ESP, std::string(str.m_String));
					draw::String(fonts::ESP, screen.x - l.first / 2, screen.y, color, 2, str.m_String);
					screen.y += 11;
				}
			}
		}
	}

#if GUI_ENABLED == true
		g_pGUI->Update();
#endif

	DrawStrings();
	SEGV_END;
}

