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

List::List(std::string title) : open_sublist(nullptr), title(title), got_mouse(false), CBaseContainer("ncc_list") {
	AddChild(new ItemTitle(title));
	Hide();
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

void List::OnKeyPress(ButtonCode_t key, bool repeat) {
	if (GetHoveredChild()) GetHoveredChild()->OnKeyPress(key, repeat);
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

/*IWidget* List::ChildByPoint(int x, int y) {
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
}*/

void List::Draw(int x, int y) {
	//const auto& size = GetSize();
	draw::OutlineRect(x, y, 222, Props()->GetInt("items") * 15 + 2, GUIColor());
	for (int i = 1; i < Props()->GetInt("items"); i++) {
		draw::DrawLine(x + 1, y + 15 * i, 220, 0, GUIColor());
	}
	//CBaseContainer::Draw(x, y);
	for (int i = 0; i < ChildCount(); i++) {
		Item* item = dynamic_cast<Item*>(ChildByIndex(i));
		if (!item) {
			if (ChildByIndex(i)->GetName().find("ncc_list") == 0) continue;
			throw std::runtime_error("Invalid cast in NCC-List!");
		}
		const auto& offset = item->GetOffset();
		item->Draw(x + offset.first, y + offset.second);
	}
	if (dynamic_cast<List*>(open_sublist)) {
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
}

void List::MoveChildren() {
	int accy = 2;
	int j = 0;
	for (int i = 0; i < ChildCount(); i++) {
		Item* item = dynamic_cast<Item*>(ChildByIndex(i));
		if (!item) {
			if (ChildByIndex(i)->GetName().find("ncc_list") == 0) continue;
			throw std::runtime_error("Invalid cast in NCC-List!");
		}
		item->SetOffset(1, j * 15 + 1);
		accy += 15;
		j++;
	}
	Props()->SetInt("items", j);
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
