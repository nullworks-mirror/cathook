/*
 * ItemSublist.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "ItemSublist.hpp"
#include "Menu.hpp"

#include "../../common.h"

namespace menu { namespace ncc {

ItemSublist::ItemSublist(std::string title, List* list) :
	Item(), title(title), list(list){
}

void ItemSublist::Draw(int x, int y) {
	Item::Draw(x, y);
	draw::String(font_item, x + 2, y + 2, colors::white, 2, format((IsHovered() ? "[-] " : "[+] "), title));
}

void ItemSublist::OnKeyPress(ButtonCode_t code, bool repeated) {
	Item::OnKeyPress(code, repeated);
}

void ItemSublist::OnMouseEnter() {
	Item::OnMouseEnter();
}

void ItemSublist::OnMouseLeave() {
	Item::OnMouseLeave();
}

}}
