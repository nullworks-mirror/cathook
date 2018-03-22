/*
 * CBaseButton.cpp
 *
 *  Created on: Jan 26, 2017
 *      Author: nullifiedcat
 */

#include "menu/CBaseButton.h"

#include "common.hpp"
#include "sdk.hpp"

CBaseButton::CBaseButton(std::string name, IWidget* parent, std::string text, ButtonCallbackFn_t callback) : CTextLabel(name, parent, text) {
	SetPadding(BUTTON_PADDING_W, BUTTON_PADDING_H);
	SetText(text);
	if (callback) SetCallback(callback);
}

void CBaseButton::SetCallback(ButtonCallbackFn_t callback) {
	m_pCallback = callback;
}

void CBaseButton::Draw(int x, int y) {
	int textcolor = NCGUIColor();
	auto size = GetSize();
	if (IsPressed()) {
		draw::DrawRect(x, y, size.first, size.second, NCGUIColor());
		textcolor = colorsint::white;
	}
	draw::OutlineRect(x, y, size.first, size.second, NCGUIColor());
	draw::String(fonts::MENU, x + Props()->GetInt("padding_x"), y + Props()->GetInt("padding_y"), textcolor, 1, GetText());
}

void CBaseButton::OnMousePress() {
	CBaseWidget::OnMousePress();
	if (m_pCallback) {
		m_pCallback(this);
	}
}
