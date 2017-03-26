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
#include "ItemVariable.hpp"

#include "../../common.h"

namespace menu { namespace ncc {

List::List(std::string title) : open_sublist(nullptr), title(title), got_mouse(false), CBaseContainer() {
	AddChild(new ItemTitle(title));
}

void List::Show() {
	CBaseContainer::Show();
	got_mouse = false;
}

void List::FillWithCatVars(std::vector<CatVar*> vec) {
	for (auto var : vec) {
		AddChild(new ItemVariable(*var));
	}
}


void List::OpenSublist(List* sublist, int dy) {
	if (open_sublist) open_sublist->Hide();
	open_sublist = sublist;
	if (sublist) {
		sublist->SetOffset(221, dy);
		sublist->Show();
	}
}

bool List::ShouldClose() {
	if (open_sublist) {
		if (!open_sublist->ShouldClose()) return false;
	}
	return !IsHovered() && !got_mouse;
}

IWidget* List::ChildByPoint(int x, int y) {
	IWidget* rr = CBaseContainer::ChildByPoint(x, y);
	if (rr) return rr;
	List* list = dynamic_cast<List*>(open_sublist);
	if (list) {
		auto co = list->GetOffset();
		auto cs = list->GetSize();
		if (x >= co.first && x <= co.first + cs.first &&
			y >= co.second && y <= co.second + cs.second) {
			return list;
		}
	}
	return nullptr;
}

void List::Draw(int x, int y) {
	//const auto& size = GetSize();
	draw::OutlineRect(x, y, 222, ChildCount() * 15 + 2, GUIColor());
	for (int i = 1; i < ChildCount(); i++) {
		draw::DrawLine(x + 1, y + 15 * i, 220, 0, GUIColor());
	}
	//CBaseContainer::Draw(x, y);
	for (int i = 0; i < ChildCount(); i++) {
		Item* item = dynamic_cast<Item*>(ChildByIndex(i));
		if (!item) throw std::runtime_error("Invalid cast in NCC-List!");
		const auto& offset = item->GetOffset();
		item->Draw(x + offset.first, y + offset.second);
	}
	if (open_sublist) {
		const auto& offset = open_sublist->GetOffset();
		open_sublist->Draw(x + offset.first, y + offset.second);
	}
}

void List::OnMouseEnter() {
	CBaseContainer::OnMouseEnter();
	got_mouse = true;
}

void List::Update() {
	CBaseContainer::Update();
	if (open_sublist)
		open_sublist->Update();
}

void List::MoveChildren() {
	int accy = 2;
	for (int i = 0; i < ChildCount(); i++) {
		Item* item = dynamic_cast<Item*>(ChildByIndex(i));
		if (!item) throw std::runtime_error("Invalid cast in NCC-List!");
		item->SetOffset(1, i * 15 + 1);
		accy += 15;
	}
	List* list = dynamic_cast<List*>(open_sublist);
	if (list) {
		const auto& size = list->GetSize();
		const auto& offset = list->GetOffset();
		SetSize(222 + size.first, max(accy, offset.second + size.second));
	} else {
		SetSize(222, accy);
	}
}

}}
