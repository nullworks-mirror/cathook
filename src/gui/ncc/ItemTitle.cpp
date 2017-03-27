/*
 * ItemTitle.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "ItemTitle.hpp"
#include "Menu.hpp"

#include "../../common.h"

namespace menu { namespace ncc {

ItemTitle::ItemTitle(std::string title) : Item("ncc_list_title"), title(title) {}

void ItemTitle::Draw(int x, int y) {
	Item::Draw(x, y);
	const std::string str = format(">>> ", title, " <<<");
	const auto& size = draw::GetStringLength(font_title, str);
	draw::String(font_title, x + (110 - size.first / 2), y, colors::white, 2, str);
}

}}
