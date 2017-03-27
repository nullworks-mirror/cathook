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
	Item("ncc_item_sublist"), title(title), list(list) {
}

void ItemSublist::SetParent(IWidget* widget) {
	Item::SetParent(widget);
	List* listp = dynamic_cast<List*>(widget);
	if (!listp) throw std::runtime_error("ItemSublist parent isnt List");
	listp->AddChild(list);
}

bool ItemSublist::IsHovered() {
	List* parent = dynamic_cast<List*>(GetParent());
	if (!parent) throw std::runtime_error("Sublist parent can't be casted to List!");
	return Item::IsHovered() || (dynamic_cast<List*>(parent->open_sublist) == list && !parent->open_sublist->ShouldClose());
}

void ItemSublist::Update() {
	if (!IsHovered()) {
		List* parent = dynamic_cast<List*>(GetParent());
		if (!parent) throw std::runtime_error("Sublist parent can't be casted to List!");
		if (dynamic_cast<List*>(parent->open_sublist) == list) {
			parent->OpenSublist(nullptr, 0);
		}
	}
}

void ItemSublist::Draw(int x, int y) {
	Item::Draw(x, y);
	List* parent = dynamic_cast<List*>(GetParent());
	if (!parent) throw std::runtime_error("Sublist parent can't be casted to List!");
	const auto& size = GetSize();
	if (parent->open_sublist == list)
		draw::DrawRect(x, y, size.first, size.second, colors::Transparent(GUIColor(), 0.5f));
	draw::String(font_item, x + 2, y, colors::white, 2, format((IsHovered() ? "[-] " : "[+] "), title));
}

void ItemSublist::OnKeyPress(ButtonCode_t code, bool repeated) {
	Item::OnKeyPress(code, repeated);
}

void ItemSublist::OnMouseEnter() {
	Item::OnMouseEnter();
	List* parent = dynamic_cast<List*>(GetParent());
	if (!parent) throw std::runtime_error("Sublist parent can't be casted to List!");
	parent->OpenSublist(list, GetOffset().second - 1);
}

void ItemSublist::OnMouseLeave() {
	Item::OnMouseLeave();
	List* parent = dynamic_cast<List*>(GetParent());
	if (!parent) throw std::runtime_error("Sublist parent can't be casted to List!");
	if (dynamic_cast<List*>(parent->open_sublist)) {
		if (parent->open_sublist->ShouldClose()) {
//			parent->OpenSublist(nullptr, 0);
		}
	}
}

}}
