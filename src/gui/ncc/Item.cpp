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

Item::Item() {
	SetSize(220, 16);
	SetMaxSize(220, 16);
}

void Item::Draw(int x, int y) {
	const auto& size = GetSize();
	draw::DrawRect(x, y, size.first, size.second, IsHovered() ? color_bg_hover : color_bg);
}

}}
