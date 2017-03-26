/*
 * Menu.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "../../common.h"

#include "Menu.hpp"
#include "List.hpp"
#include "ItemSublist.hpp"

namespace menu { namespace ncc {

unsigned long font_title = 0;
unsigned long font_item  = 0;

std::vector<CatVar*> FindCatVars(const std::string name) {
	std::vector<CatVar*> result = {};
	for (auto var : CatVarList()) {
		if (var->name.find(name) == 0) result.push_back(var);
	}
	return result;
}

void Init() {
	font_title = g_ISurface->CreateFont();
	font_item  = g_ISurface->CreateFont();
	g_ISurface->SetFontGlyphSet(font_title, "Verdana Bold", 14, 0, 0, 0, 0x0);
	g_ISurface->SetFontGlyphSet(font_item, "Verdana", 12, 0, 0, 0, 0x0);
	// TODO add stuff
	List* aimbot_list = new List("Aim Bot Menu");
	aimbot_list->FillWithCatVars(FindCatVars("aimbot_"));
	MainList().AddChild(new ItemSublist("Aim Bot", aimbot_list));
	List* triggerbot_list = new List("Trigger Bot Menu");
	triggerbot_list->FillWithCatVars(FindCatVars("trigger_"));
	MainList().AddChild(new ItemSublist("Trigger Bot", triggerbot_list));
	//MainList().AddChild(new ItemSublist("Accuracy", nullptr));
	// Fake Lag

	List* esp_list = new List("ESP Menu");
	esp_list->FillWithCatVars(FindCatVars("esp_"));
	MainList().AddChild(new ItemSublist("ESP", esp_list));
	// Radar
	/*MainList().AddChild(new ItemSublist("Bunny Hop", nullptr));
	MainList().AddChild(new ItemSublist("Air Stuck", nullptr));
	// Speed Hack
	MainList().AddChild(new ItemSublist("Anti-Aim", nullptr));
	// Name Stealer
	MainList().AddChild(new ItemSublist("Chat Spam", nullptr));
	MainList().AddChild(new ItemSublist("Misc", nullptr));
	// Info Panel
	MainList().AddChild(new ItemSublist("Ignore Settings", nullptr));
	MainList().AddChild(new ItemSublist("Cheat Settings", nullptr));*/
}

List& MainList() {
	static List object("Cat Hook");
	return object;
}

}}
