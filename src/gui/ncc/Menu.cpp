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
#include "Tooltip.hpp"

namespace menu { namespace ncc {

unsigned long font_title = 0;
unsigned long font_item  = 0;

Tooltip* tooltip = 0;

void ShowTooltip(const std::string& text) {
	tooltip->Show();
	tooltip->SetText(text);
}

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
	aimbot_list->FillWithCatVars({
		"aimbot_enabled", "aimbot_fov", "aimbot_aimkey", "aimbot_aimkey_mode", "aimbot_hitboxmode", "aimbot_hitbox",
		"aimbot_silent", "aimbot_prioritymode", "aimbot_autoshoot", "aimbot_zoomed", "aimbot_teammates", "aimbot_buildings", "aimbot_respect_cloak",
		"aimbot_projectile"
	});
	List* aimbot_list_projectile = new List("Projectile Aimbot");
	aimbot_list_projectile->FillWithCatVars({
		"aimbot_huntsman_charge", "aimbot_full_auto_huntsman",
		"aimbot_proj_fovpred", "aimbot_proj_vispred", "aimbot_proj_gravity", "aimbot_proj_speed"
	});
	aimbot_list->AddChild(new ItemSublist("Projectile Aimbot Tweaks", aimbot_list_projectile));
	aimbot_list->FillWithCatVars({
		"aimbot_only_when_can_shoot", "aimbot_enable_attack_only", "aimbot_maxrange", "aimbot_interp"
	});
	//aimbot_list->FillWithCatVars(FindCatVars("aimbot_"));
	MainList().AddChild(new ItemSublist("Aim Bot", aimbot_list));
	List* triggerbot_list = new List("Trigger Bot Menu");
	triggerbot_list->FillWithCatVars(FindCatVars("trigger_"));
	MainList().AddChild(new ItemSublist("Trigger Bot", triggerbot_list));
	//MainList().AddChild(new ItemSublist("Accuracy", nullptr));
	// Fake Lag

	List* esp_list = new List("ESP Menu");
	esp_list->FillWithCatVars(FindCatVars("esp_"));
	MainList().AddChild(new ItemSublist("ESP", esp_list));
	MainList().Show();
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
