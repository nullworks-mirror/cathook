/*
 * Logo.cpp
 *
 *  Created on: Apr 28, 2017
 *      Author: nullifiedcat
 */

#include "Menu.hpp"

namespace menu { namespace ncc {

static CatEnum logo_enum({"NEVER", "MENU", "ALWAYS"});
CatVar logo(logo_enum, "logo", "1", "Show logo", "Show cathook logo when");

Logo::Logo() : CBaseWidget("nc_logo"), texture(&_binary_logo_start, 576, 288) {
	SetSize(576, 288);
}

bool Logo::AlwaysVisible() {
	return (int)logo == 2;
}

void Logo::Draw(int x, int y) {
	if (logo) {
		if (!texture.id) texture.Load();
		texture.Draw(x, y, 576, 288, GUIColor());
	}
}

void Logo::Update() {
	if (IsPressed()) {
		auto offset = GetOffset();
		offset.first += g_pGUI->mouse_dx;
		offset.second += g_pGUI->mouse_dy;
		SetOffset(offset.first, offset.second);
	}
}

}}
