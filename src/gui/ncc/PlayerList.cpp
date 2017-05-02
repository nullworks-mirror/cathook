/*
 * PlayerList.cpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#include "Menu.hpp"

namespace menu { namespace ncc {

PlayerList::PlayerList() : CBaseContainer() {
	for (int i = 0; i <= 32; i++) {
		AddChild(new PlayerListEntry(i));
	}
}

void PlayerList::Draw(int x, int y) {
	if (g_Settings.bInvalid) return;
	const auto& size = GetSize();
	draw::DrawRect(x, y, size.first, size.second, colors::Create(0, 0, 0, 77));
	draw::OutlineRect(x, y, size.first, size.second, GUIColor());
	for (int i = 0; i < Props()->GetInt("vischildren"); i++) {
		draw::DrawLine(x, y + i * 17, size_table_width(), 0, GUIColor());
	}
	int accum = 0;
	for (int i = 0; i < sizeof(size_table) / sizeof(int); i++) {
		draw::DrawLine(x + accum, y, 0, size.second, GUIColor());
		accum += size_table[i] + 1;
	}
	CBaseContainer::Draw(x, y);
}

void PlayerList::OnKeyPress(ButtonCode_t key, bool repeat) {
	if (GetHoveredChild()) GetHoveredChild()->OnKeyPress(key, repeat);
}

void PlayerList::Update() {
	if (g_Settings.bInvalid) return;
	if (IsPressed()) {
		auto offset = GetOffset();
		offset.first += g_pGUI->mouse_dx;
		offset.second += g_pGUI->mouse_dy;
		SetOffset(offset.first, offset.second);
	}
	CBaseContainer::Update();
}

void PlayerList::MoveChildren() {
	int visible = 0;
	for (int i = 0; i < ChildCount(); i++) {
		ChildByIndex(i)->SetOffset(0, 1 + visible * 17);
		if (ChildByIndex(i)->IsVisible()) visible++;
	}
	SetSize(size_table_width(), visible * 17 + 1);
	Props()->SetInt("vischildren", visible);
}

}}
