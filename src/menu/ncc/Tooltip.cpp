/*
 * Tooltip.cpp
 *
 *  Created on: Mar 27, 2017
 *      Author: nullifiedcat
 */

#include "menu/ncc/Tooltip.hpp"
#include "menu/CTextLabel.h"
#include "common.hpp"

namespace menu { namespace ncc {

Tooltip::Tooltip() : CTextLabel("ncc_tooltip") {
	SetZIndex(999);
	SetPadding(3, 2);
	SetMaxSize(220, -1);
	SetAutoSize(false);
	SetSize(220, -1);
	Props()->SetInt("font", font_item);
}

void Tooltip::HandleCustomEvent(KeyValues* event) {
	if (!strcmp(event->GetName(), "scale_update")) {
		SetMaxSize(Item::psize_x * (float)scale, -1);
		SetSize(Item::psize_x * (float)scale, -1);
		SetText(GetText()); // To update word wrapping.
	} else if (!strcmp(event->GetName(), "font_update")) {
		Props()->SetInt("font", font_item);
	}
}

void Tooltip::Draw(int x, int  y) {
	const auto& size = GetSize();
	int originx = x;
	int originy = y;
	if (originx + size.first > draw::width) originx -= size.first;
	if (originx + size.second > draw::height) originy -= size.second;
	static int bgcolor = colorsint::Create(0, 0, 0, 77); //colorsint::Create(70, 86, 47, 28);
	static int fgcolor = colorsint::Create(200, 200, 190, 255);
	draw::DrawRect(x, y, size.first, size.second, bgcolor);
	draw::OutlineRect(x, y, size.first, size.second, NCGUIColor());
	draw::String(font_item, x + Props()->GetInt("padding_x"), y + Props()->GetInt("padding_y"), fgcolor, 2, GetText());
}

}}
