/*
 * List.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "List.hpp"
#include "Item.hpp"
#include "ItemTitle.hpp"
#include "Menu.hpp"

#include "../../common.h"

namespace menu { namespace ncc {

List::List(std::string title) : open_sublist(nullptr), title(title), items {} {
	AddChild(new ItemTitle(title));
}

void List::Draw(int x, int y) {
	const auto& size = GetSize();
	draw::OutlineRect(x, y, size.first, size.second, GUIColor());
	for (int i = 1; i < ChildCount(); i++) {
		draw::DrawLine(x + 1, y + 15 * i, 220, 0, GUIColor());
	}
	CBaseContainer::Draw(x, y);
}

void List::Update() {
	CBaseContainer::Update();
}

void List::MoveChildren() {
	int accy = 2;
	for (int i = 0; i < ChildCount(); i++) {
		Item* item = dynamic_cast<Item*>(ChildByIndex(i));
		if (!item) throw std::runtime_error("Invalid cast in NCC-List!");
		item->SetOffset(1, i * 15 + 1);
		accy += 15;
	}
	SetSize(222, accy);
}

}}
