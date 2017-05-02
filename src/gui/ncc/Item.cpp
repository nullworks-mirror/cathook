/*
 * Item.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "Item.hpp"
#include "Menu.hpp"

#include "../../common.h"

namespace menu { namespace ncc {

Item::Item(std::string name) : CBaseWidget(name, nullptr) {
	SetSize(220, 15);
	SetMaxSize(220, 15);
}

void Item::Draw(int x, int y) {
	const auto& size = GetSize();
	//draw::DrawRect(x, y, size.first, size.second, colors::red);
	draw::DrawRect(x, y, size.first, size.second, colors::Create(0, 0, 0, 77));
	if (IsHovered()) {
		draw::DrawRect(x, y, size.first, size.second, colors::Transparent(GUIColor(), 0.32f));
	}
}

}}
