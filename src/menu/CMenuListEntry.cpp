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
		draw::DrawLine(x, y, size.first, 0, NCGUIColor());
		draw::DrawLine(x, y + size.second, size.first, 0, NCGUIColor());
		draw::DrawLine(x, y, 0, size.second, NCGUIColor());
	} else {
		draw::OutlineRect(x, y, size.first, size.second, NCGUIColor());
	}
	if (IsHovered()) {
		draw::DrawRect(x, y, size.first, size.second, colorsint::Transparent(NCGUIColor(), 0.25));
	}
	draw::String(fonts::MENU_BIG, x + (size.first - texts.first) / 2, y + (size.second - texts.second) / 2, IsSelected() ? colorsint::white : NCGUIColor(), 1, GetText());
}
