/*
 * CMenuListEntry.cpp
 *
 *  Created on: Feb 3, 2017
 *      Author: nullifiedcat
 */

#include "menu/CMenuListEntry.h"
#include "menu/CMenuList.h"

#include "common.hpp"
#include "sdk.hpp"

CMenuListEntry::CMenuListEntry(std::string name, CMenuList* parent, std::string entry) : CBaseButton(name, parent, entry) {
	Props()->SetString("entry", entry.c_str());
}

void CMenuListEntry::SetMaxSize(int x, int y) {
	CBaseWidget::SetMaxSize(x, y);
	SetSize(x, y);
}

bool CMenuListEntry::IsSelected() {
	return (dynamic_cast<CMenuList*>(GetParent())->m_pSelected == this);
}

void CMenuListEntry::Draw(int x, int y) {
	auto texts = draw::GetStringLength(fonts::MENU_BIG, GetText());
	auto size = GetSize();
	if (IsSelected()) {
		draw::DrawLine(x, y, size.first, 0, GUIColor2());
		draw::DrawLine(x, y + size.second, size.first, 0, GUIColor2());
		draw::DrawLine(x, y, 0, size.second, GUIColor2());
	} else {
		draw::OutlineRect(x, y, size.first, size.second, GUIColor2());
	}
	if (IsHovered()) {
		draw::DrawRect(x, y, size.first, size.second, colors2::Transparent(GUIColor2(), 0.25));
	}
	draw::String(fonts::MENU_BIG, x + (size.first - texts.first) / 2, y + (size.second - texts.second) / 2, IsSelected() ? colors2::white : GUIColor2(), 1, GetText());
}
