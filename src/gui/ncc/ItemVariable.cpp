/*
 * ItemVariable.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "ItemVariable.hpp"
#include "Item.hpp"
#include "../../common.h"

namespace menu { namespace ncc {

ItemVariable::ItemVariable(CatVar& variable) : Item("ncc_item_variable_" + variable.name), catvar(variable) {

}

void ItemVariable::Update() {
	Item::Update();
	if (catvar.desc_long.length() && IsHovered() && catvar.desc_long != "no description")
		ShowTooltip(catvar.desc_long);
}

void ItemVariable::Change(float amount) {
	if (!amount) return;
	switch (catvar.type) {
	case CV_SWITCH: {
		catvar = !catvar;
	} break;
	case CV_ENUM:
	case CV_INT: {
		catvar = (int)catvar + (int)amount;
		if (catvar.restricted && (int)catvar > catvar.max) catvar = (int)catvar.max;
		if (catvar.restricted && (int)catvar < catvar.min) catvar = (int)catvar.min;
	} break;
	case CV_FLOAT: {
		catvar = (float)catvar + amount;
		if (catvar.restricted && (float)catvar > catvar.max) catvar = (float)catvar.max;
		if (catvar.restricted && (float)catvar < catvar.min) catvar = (float)catvar.min;
	} break;
	}
}

bool ItemVariable::ConsumesKey(ButtonCode_t key) {
	if (capturing) return true;
	if (key == ButtonCode_t::MOUSE_WHEEL_DOWN || key == ButtonCode_t::MOUSE_WHEEL_UP || key == ButtonCode_t::MOUSE_FIRST) return true;
	return false;
}

void ItemVariable::OnMousePress() {
	if (catvar.type == CV_KEY)
		capturing = true;
	if (catvar.type == CV_SWITCH)
		catvar = !catvar;
}

void ItemVariable::OnFocusLose() {
	capturing = false;
}

void ItemVariable::OnKeyPress(ButtonCode_t key, bool repeat) {
	if (capturing) {
		catvar = (int)key;
		capturing = false;
		return;
	}

	float change = 0.0f;

	switch (catvar.type) {
	case CV_ENUM:
	case CV_SWITCH:
		change = 1.0f; break;
	case CV_INT:
	case CV_FLOAT: {
		if (catvar.restricted) {
			change = float(catvar.max - catvar.min) / 50.0f;
		} else {
			change = 1.0f;
		}
	}
	}

	if (change < 1.0f && catvar.type == CV_INT) change = 1.0f;

	if ((catvar.type == CV_SWITCH && key == ButtonCode_t::MOUSE_FIRST) || key == ButtonCode_t::MOUSE_WHEEL_UP) {
		Change(change);
	} else if (key == ButtonCode_t::MOUSE_WHEEL_DOWN) {
		Change(-change);
	}
}

void ItemVariable::Draw(int x, int y) {
	Item::Draw(x, y);
	std::string val = "[UNDEFINED]";
	switch (catvar.type) {
	case CV_SWITCH: {
		val = catvar ? "ON" : "OFF";
	} break;
	case CV_INT: {
		val = std::to_string((int)catvar);
	} break;
	case CV_FLOAT: {
		val = std::to_string((float)catvar);
	} break;
	case CV_ENUM: {
		val = catvar.enum_type->Name((int)catvar);
	} break;
	case CV_STRING: {
		val = std::string(catvar.GetString());
	} break;
	case CV_KEY: {
		if (capturing) {
			val = "[PRESS A KEY]";
		} else {
			if (catvar) {
				const char* name = g_IInputSystem->ButtonCodeToString((ButtonCode_t)((int)catvar));
				if (name) {
					val = std::string(name);
				}
			} else {
				val = "[CLICK TO SET]";
			}
		}
	} break;
	}
	draw::String(menu::ncc::font_item, x + 2, y, colors::white, 2, format(catvar.desc_short, ": ", val));
}

}}
