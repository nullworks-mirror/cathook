/*
 * Menu.cpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include "menu/ncc/Menu.hpp"
#include "menu/ncc/List.hpp"
#include "menu/ncc/ItemSublist.hpp"
#include "menu/ncc/Tooltip.hpp"

namespace menu { namespace ncc {

unsigned long font_title = 0;
unsigned long font_item  = 0;

CatVar scale(CV_FLOAT, "gui_ncc_scale", "1", "NCC GUI Scale", "Defines scale of NCC gui", 0.5f, 4.0f);
CatVar font_family(fonts::family_enum, "gui_ncc_font_family", "3", "NCC Font Family", "Defines font family for NCC menu");
CatVar font_title_family(fonts::family_enum, "gui_ncc_font_title_family", "4", "NCC Title Family", "Defines font family for NCC menu titles");

Tooltip* tooltip = nullptr;;
Root* root = nullptr;

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

bool init_done = false;

void ChangeCallback(IConVar* var, const char* pszOldValue, float flOldValue) {
	if (init_done)
		RefreshFonts();
}

void Init() {
	root = new Root();
	root->Setup();
	scale.InstallChangeCallback([](IConVar* var, const char* pszOldValue, float flOldValue) {
		if (init_done) RefreshFonts();
		logging::Info("Scale Changed");
		root->HandleCustomEvent(KeyValues::AutoDelete("scale_update"));
	});
	font_family.InstallChangeCallback(ChangeCallback);
	font_title_family.InstallChangeCallback(ChangeCallback);
	init_done = true;
	RefreshFonts();
}

void RefreshFonts() {
	font_title = g_ISurface->CreateFont();
	font_item  = g_ISurface->CreateFont();
	g_ISurface->SetFontGlyphSet(font_title, fonts::fonts.at(_clamp(0, (int)(fonts::fonts.size() - 1), (int)font_title_family)).c_str(), psize_font_title * (float)scale, 0, 0, 0, 0x0);
	g_ISurface->SetFontGlyphSet(font_item, fonts::fonts.at(_clamp(0, (int)(fonts::fonts.size() - 1), (int)font_family)).c_str(), psize_font_item * (float)scale, 0, 0, 0, 0x0);
	root->HandleCustomEvent(KeyValues::AutoDelete("font_update"));
}

static const std::string list_hl2dm = R"(
"Cat Hook"
"Aim Bot" [
	"Aim Bot Menu"
	"aimbot_enabled"
	"aimbot_aimkey"
	"aimbot_aimkey_mode"
	"aimbot_autoshoot"
	"aimbot_hitboxmode"
	"aimbot_fov"
	"aimbot_prioritymode"
	"aimbot_charge"
    "aimbot_teammates"
	"Preferences" [
		"Aim Bot Preferences"
		"aimbot_silent"
		"aimbot_hitbox"
		"aimbot_only_when_can_shoot"
		"aimbot_enable_attack_only"
		"aimbot_maxrange"
		"aimbot_interp"
		"aimbot_slow"
		"aimbot_slow_smooth"
		"aimbot_slow_autoshoot"
	]
]
 
"Trigger Bot" [
	"Trigger Bot Menu"
	"trigger_enabled"
	"trigger_accuracy"
	"trigger_range"
	"trigger_hitbox"
]
 


"Visuals" [
	"Visuals Menu"
	"ESP" [
		"ESP Menu"
		"esp_enabled"
		"font_esp_family"
		"font_esp_height"
		"esp_name"
		"esp_distance"
		"esp_box"
		"esp_box_text_position"
		"esp_box_nodraw"
		"esp_box_expand"
		"3D Box" [
			"3D Box Settings"
			"esp_3d_box"
			"esp_3d_box_thick"
			"esp_3d_box_health"
			"esp_3d_box_expand"
			"esp_3d_box_smoothing"
			"esp_3d_box_expand_size"
			"esp_3d_box_healthbar"
		]
		"esp_legit"
		"esp_health_num"
		"esp_weapon_spawners"
		"esp_model_name"
		"esp_weapon"
		"esp_vischeck"
		"esp_entity_id"
		"esp_followbot_id"
		"esp_teammates"
		"esp_entity"
		"esp_local"
		"Items" [
			"Item ESP Menu"
			"esp_item"
			"esp_item_adrenaline"
			"esp_item_powerups"
			"esp_item_health"
			"esp_item_ammo"
			"esp_item_weapons"
		]
	]
	"Chams" [
		"Chams Menu"
		"chams_enable"
		"chams_health"
		"chams_players"
		"chams_teammates"
		"chams_flat"
		"chams_weapons"
		"chams_medkits"
		"chams_ammo"
	]

	"Glow" [
		"Glow Menu"
		"glow_enable"
		"glow_solid_when"
		"glow_blur_scale"
		"glow_health"
		"glow_players"
		"glow_teammates"
		"glow_medkits"
		"glow_ammo"
	]
	"GUI" [
		"GUI Settings"
		"logo"
		"gui_bg_particles"
		"gui_bg_particles_type"
		"gui_bg_particles_chance"
		"gui_bg_particles_pack_size"
		"gui_bg_particles_safe_zone"
		"gui_bg_particles_gravity"
		"gui_bg_particles_jittering"
		"gui_bg_particles_jittering_chance"
		"gui_bg_particles_wind"
		"gui_bg_visible"
		"gui_color_b"
		"gui_color_g"
		"gui_color_r"
		"gui_rainbow"
		"fast_outline"
		"gui_bounds"
		"gui_visible"
	]
	"Miscellaneous" [
		"Miscellaneous"
		"pure_bypass"
		"no_arms"
		"fov"
		"clean_screenshots"
		"logo"
	]
]
 
"Anti-/Anti-Aim" [
	"Anti-Aim Menu"
	"aa_enabled"
	"aa_pitch"
	"aa_pitch_mode"
	"aa_yaw"
	"aa_yaw_mode"
	"aa_spin"
	"aa_roll"
	"aa_no_clamp"
	"resolver"
	"Anti-Anti-Anti-Aim" [
		"Anti-Anti-Anti-Aim Menu"
		"aa_aaaa_enabled"
		"aa_aaaa_interval"
		"aa_aaaa_interval_low"
		"aa_aaaa_interval_high"
		"aa_aaaa_mode"
		"aa_aaaa_flip_key"
	]
]

"Chat" [
	"Chat Options Menu"
	"chat_newlines"
	"clean_chat"
	"killsay"
	"spam"
	"spam_random"
]
 
"Follow Bot" [
	"Follow Bot Settings"
	"fb_bot"
	"fb_mimic_slot"
	"fb_always_medigun"
	"fb_autoclass"
	"fb_follow_distance"
]

"Miscellaneous" [
	"Miscellaneous Settings"
	"bhop_enabled"
	"fast_vischeck"
	"anti_afk"
	"rollspeedhack"
	"info"
	"Debug" [
		"Debug Menu"
		"software_cursor_mode"
		"enabled"
		"no_visuals"
		"debug_info"
		"log"
	]
]
)";

static const std::string list_tf2 = R"(
"Cat Hook"
"Aim Bot" [
    "Aim Bot Menu"
    "aimbot_enabled"
    "aimbot_aimkey"
    "aimbot_aimkey_mode"
    "aimbot_autoshoot"
    "aimbot_hitboxmode"
    "aimbot_fov"
    "aimbot_prioritymode"
    "aimbot_charge"
    "Ignore" [
        "Ignore/Respect Menu"
        "ignore_taunting"
        "aimbot_ignore_hoovy"
        "aimbot_teammates"
    ]
    "Auto Heal" [
        "Auto Heal Menu"
        "autoheal_enabled"
        "autoheal_uber"
        "autoheal_uber_health"
        "autoheal_silent"
        "autoheal_share_uber"
    ]
    "Preferences" [
        "Aim Bot Preferences"
        "aimbot_silent"
        "aimbot_hitbox"
        "aimbot_zoomed"
        "aimbot_only_when_can_shoot"
        "aimbot_maxrange"
        "aimbot_slow"
        "Projectile Aimbot" [
            "Projectile Aimbot Tweaks"
            "aimbot_projectile"
            "aimbot_proj_gravity"
            "aimbot_proj_speed"
            "aimbot_huntsman_charge"
        ]
    ]
]
)";

List& MainList() {
	static List* main = List::FromString(IsTF2() ? list_tf2 : list_hl2dm);
	return *main;
}

}}
