/*
 * HEsp.cpp
 *
 *  Created on: Oct 6, 2016
 *      Author: nullifiedcat
 */

#include "ESP.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace shared { namespace esp {

CatVar local_esp(CV_SWITCH, "esp_local", "1", "ESP Local Player", "Shows local player ESP in thirdperson");
CatVar buildings(CV_SWITCH, "esp_buildings", "1", "Building ESP", "Show buildings");
CatVar enabled(CV_SWITCH, "esp_enabled", "0", "ESP", "Master ESP switch");
CatVar entity_info(CV_SWITCH, "esp_entity", "0", "Entity ESP", "Show entity info (debug)");
CatVar teammates(CV_SWITCH, "esp_teammates", "0", "ESP Teammates", "Teammate ESP");
CatVar item_esp(CV_SWITCH, "esp_item", "1", "Item ESP", "Master Item ESP switch (health packs, etc.)");
CatVar item_dropped_weapons(CV_SWITCH, "esp_item_weapons", "0", "Dropped weapons", "Show dropped weapons");
CatVar item_ammo_packs(CV_SWITCH, "esp_item_ammo", "0", "Ammo packs", "Show ammo packs");
CatVar item_health_packs(CV_SWITCH, "esp_item_health", "1", "Health packs", "Show health packs");
CatVar item_powerups(CV_SWITCH, "esp_item_powerups", "1", "Powerups", "Show powerups");
CatVar item_money(CV_SWITCH, "esp_money", "1", "MvM money", "Show MvM money");
CatVar item_money_red(CV_SWITCH, "esp_money_red", "1", "Red MvM money", "Show red MvM money");
CatVar entity_id(CV_SWITCH, "esp_entity_id", "1", "Entity ID", "Used with Entity ESP. Shows entityID");
CatVar tank(CV_SWITCH, "esp_show_tank", "1", "Show tank", "Show tank");
CatVar box_esp(CV_SWITCH, "esp_box", "1", "Box", "Draw 2D box with healthbar. fancy.");
CatVar show_distance(CV_SWITCH, "esp_distance", "1", "Distance ESP", "Show distance to target");
CatVar show_name(CV_SWITCH, "esp_name", "1", "Name ESP", "Show name");
CatVar show_class(CV_SWITCH, "esp_class", "1", "Class ESP", "Show class");
CatVar show_conditions(CV_SWITCH, "esp_conds", "1", "Conditions ESP", "Show conditions");
CatVar vischeck(CV_SWITCH, "esp_vischeck", "1", "VisCheck", "ESP visibility check - makes enemy info behind walls darker, disable this if you get FPS drops");
CatVar legit(CV_SWITCH, "esp_legit", "0", "Legit Mode", "Don't show invisible enemies");
CatVar show_health(CV_SWITCH, "esp_health_num", "1", "Health numbers", "Show health in numbers");
CatEnum proj_esp_enum({ "OFF", "ALL", "CRIT" });
CatVar proj_rockets(proj_esp_enum, "esp_proj_rockets", "1", "Rockets", "Rockets");
CatVar proj_arrows(proj_esp_enum, "esp_proj_arrows", "1", "Arrows", "Arrows");
CatVar proj_pipes(proj_esp_enum, "esp_proj_pipes", "1", "Pipes", "Pipebombs");
CatVar proj_stickies(proj_esp_enum, "esp_proj_stickies", "1", "Stickies", "Stickybombs");
CatVar proj_enemy(CV_SWITCH, "esp_proj_enemy", "1", "Only enemy projectiles", "Don't show friendly projectiles");
CatVar proj_esp(CV_SWITCH, "esp_proj", "1", "Projectile ESP", "Projectile ESP");
CatVar entity_model(CV_SWITCH, "esp_model_name", "0", "Model name ESP", "Model name esp (DEBUG ONLY)");
CatVar item_weapon_spawners(CV_SWITCH, "esp_weapon_spawners", "1", "Show weapon spawners", "TF2C deathmatch weapon spawners");
CatVar item_adrenaline(CV_SWITCH, "esp_item_adrenaline", "0", "Show Adrenaline", "TF2C adrenaline pills");

const std::string classes[] = {
	"Scout",
	"Sniper",
	"Soldier",
	"Demoman",
	"Medic",
	"Heavy",
	"Pyro",
	"Spy",
	"Engineer"
};

void CreateMove() {
	int limit = HIGHEST_ENTITY;
	if (!buildings && !proj_esp && !item_esp) limit = min(32, HIGHEST_ENTITY);
	for (int i = 0; i < limit; i++) {
		ProcessEntity(ENTITY(i));
		if (ENTITY(i)->m_nESPStrings) {
			ENTITY(i)->m_ESPColorFG = colors::EntityF(ENTITY(i));
			if (vischeck) {
				if (!ENTITY(i)->IsVisible()) ENTITY(i)->m_ESPColorFG = colors::Transparent(ENTITY(i)->m_ESPColorFG);
			}
		}
	}
}

void Draw() {
	int limit = HIGHEST_ENTITY;
	if (!buildings && !proj_esp && !item_esp) limit = min(32, HIGHEST_ENTITY);
	for (int i = 0; i < limit; i++) {
		ProcessEntityPT(ENTITY(i));
	}
}

void DrawBox(CachedEntity* ent, int clr, float widthFactor, float addHeight, bool healthbar, int health, int healthmax) {
	if (CE_BAD(ent)) return;
	bool cloak = ent->m_iClassID == g_pClassID->C_Player && IsPlayerInvisible(ent);
	Vector min, max;
	RAW_ENT(ent)->GetRenderBounds(min, max);
	Vector origin = RAW_ENT(ent)->GetAbsOrigin();
	Vector so;
	draw::WorldToScreen(origin, so);
	//if (!a) return;
	//logging::Info("%f %f", so.x, so.y);
	Vector omin, omax;
	omin = origin + Vector(0, 0, min.z);
	omax = origin + Vector(0, 0, max.z + addHeight);
	Vector smin, smax;
	bool a = draw::WorldToScreen(omin, smin);
	a = a && draw::WorldToScreen(omax, smax);
	if (!a) return;
	float height = abs(smax.y - smin.y);
	//logging::Info("height: %f", height);
	float width = height / widthFactor;
	//bool a = draw::WorldToScreen(omin, smin);
	//a = a && draw::WorldToScreen(omax, smax);
	//if (!a) return;
	//draw::DrawString(min(smin.x, smax.x), min(smin.y, smax.y), clr, false, "min");
	//draw::DrawString(max(smin.x, smax.x), max(smin.y, smax.y), clr, false, "max");
	//draw::DrawString((int)so.x, (int)so.y, draw::white, false, "origin");
	ent->m_ESPOrigin.x = so.x + width / 2 + 1;
	ent->m_ESPOrigin.y = so.y - height;
	unsigned char alpha = clr >> 24;
	float trf = (float)((float)alpha / 255.0f);
	int border = cloak ? colors::Create(160, 160, 160, alpha) : colors::Transparent(colors::black, trf);

	draw::OutlineRect(so.x - width / 2 - 1, so.y - 1 - height, width + 2, height + 2, border);
	draw::OutlineRect(so.x - width / 2, so.y - height, width, height, clr);
	draw::OutlineRect(so.x - width / 2 + 1, so.y + 1 - height, width - 2, height - 2, border);
	if (healthbar) {
		int hp = colors::Transparent(colors::Health(health, healthmax), trf);
		int hbh = (height) * min((float)health / (float)healthmax, 1.0f);
		draw::DrawRect(so.x - width / 2 - 7, so.y - 1 - height, 6, height + 2, border);
		draw::DrawRect(so.x - width / 2 - 6, so.y - hbh, 5, hbh, hp);
	}
	//draw::OutlineRect(min(smin.x, smax.x) - 1, min(smin.y, smax.y) - 1, max(smin.x, smax.x), max(smin.y, smax.y), draw::black);
	//draw::OutlineRect(min(smin.x, smax.x), min(smin.y, smax.y), max(smin.x, smax.x), max(smin.y, smax.y), clr);
}

void ProcessEntity(CachedEntity* ent) {
	if (!enabled) return;
	if (CE_BAD(ent)) return;

	//if (ent->IsVisible()) ent->AddESPString("VISIBLE");

	if (entity_info) {
		ent->AddESPString("%s [%i]", RAW_ENT(ent)->GetClientClass()->m_pNetworkName, ent->m_iClassID);
		if (entity_id) {
			ent->AddESPString("%i", ent->m_IDX);
		}
		//ent->AddESPString("Type: %i", ent->m_Type);
		if (entity_model) {
			const model_t* model = RAW_ENT(ent)->GetModel();
			if (model) ent->AddESPString("%s", g_IModelInfo->GetModelName(model));
		}
	}

	if (ent->m_Type == ENTITY_PROJECTILE && proj_esp && (ent->m_bEnemy || (teammates && !proj_enemy))) {
		if (ent->m_iClassID == g_pClassID->CTFProjectile_Rocket || ent->m_iClassID ==  g_pClassID->CTFProjectile_SentryRocket) {
			if (proj_rockets) {
				if ((int)proj_rockets != 2 || ent->m_bCritProjectile) {
					ent->AddESPString("[ ==> ]");
					if (show_distance) {
						ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
					}
				}
			}
		} else if (ent->m_iClassID == g_pClassID->CTFGrenadePipebombProjectile) {
			switch (CE_INT(ent, netvar.iPipeType)) {
			case 0:
				if (!proj_pipes) break;
				if ((int)proj_pipes == 2 && !ent->m_bCritProjectile) break;
				ent->AddESPString("[ (PP) ]");
				break;
			case 1:
				if (!proj_stickies) break;
				if ((int)proj_stickies == 2 && !ent->m_bCritProjectile) break;
				ent->AddESPString("[ {*} ]");
			}
			if (show_distance) {
				ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
			}
		} else if (ent->m_iClassID == g_pClassID->CTFProjectile_Arrow) {
			if ((int)proj_arrows != 2 || ent->m_bCritProjectile) {
				ent->AddESPString("[ >>---> ]");
				if (show_distance) {
					ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
				}
			}
		}
	}

	if (HL2DM) {
		if (item_esp && item_dropped_weapons) {
			if (CE_BYTE(ent, netvar.hOwner) == (unsigned char)-1) {
				int a = ent->m_nESPStrings;
				if (ent->m_iClassID == g_pClassID->CWeapon_SLAM) ent->AddESPString("SLAM");
				else if (ent->m_iClassID == g_pClassID->CWeapon357) ent->AddESPString(".357");
				else if (ent->m_iClassID == g_pClassID->CWeaponAR2) ent->AddESPString("AR2");
				else if (ent->m_iClassID == g_pClassID->CWeaponAlyxGun) ent->AddESPString("Alyx Gun");
				else if (ent->m_iClassID == g_pClassID->CWeaponAnnabelle) ent->AddESPString("Annabelle");
				else if (ent->m_iClassID == g_pClassID->CWeaponBinoculars) ent->AddESPString("Binoculars");
				else if (ent->m_iClassID == g_pClassID->CWeaponBugBait) ent->AddESPString("Bug Bait");
				else if (ent->m_iClassID == g_pClassID->CWeaponCrossbow) ent->AddESPString("Crossbow");
				else if (ent->m_iClassID == g_pClassID->CWeaponShotgun) ent->AddESPString("Shotgun");
				else if (ent->m_iClassID == g_pClassID->CWeaponSMG1) ent->AddESPString("SMG");
				else if (ent->m_iClassID == g_pClassID->CWeaponRPG) ent->AddESPString("RPG");
				if (a != ent->m_nESPStrings) {
					ent->m_ESPColorFG = colors::yellow;
					if (show_distance) {
						ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
					}
				}
			}
		}
	}

	if (ent->m_iClassID == g_pClassID->CTFTankBoss && tank) {
		ent->AddESPString("Tank");
	} else if (ent->m_iClassID == g_pClassID->CTFDroppedWeapon && item_esp && item_dropped_weapons) {
		ent->AddESPString("WEAPON %s", ent->m_pEntity->GetClientClass()->GetName());
		if (show_distance) {
			ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
		}
	} else if (ent->m_iClassID == g_pClassID->CCurrencyPack && item_money) {
		if (CE_BYTE(ent, netvar.bDistributed)) {
			if (item_money_red) {
				ent->AddESPString("$$$");
				ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
			}
		} else {
			ent->AddESPString("$$$");
			ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
		}
	} else if (ent->m_ItemType != ITEM_NONE && item_esp) {
		bool shown = false;
		if (item_health_packs && (ent->m_ItemType >= ITEM_HEALTH_SMALL && ent->m_ItemType <= ITEM_HEALTH_LARGE || ent->m_ItemType == ITEM_HL_BATTERY)) {
			if (ent->m_ItemType == ITEM_HEALTH_SMALL) ent->AddESPString("[+]");
			if (ent->m_ItemType == ITEM_HEALTH_MEDIUM) ent->AddESPString("[++]");
			if (ent->m_ItemType == ITEM_HEALTH_LARGE) ent->AddESPString("[+++]");
			if (ent->m_ItemType == ITEM_HL_BATTERY) ent->AddESPString("[Z]");
		} else if (item_adrenaline && ent->m_ItemType == ITEM_TF2C_PILL) {
			ent->AddESPString("[a]");
		} else if (item_ammo_packs && ent->m_ItemType >= ITEM_AMMO_SMALL && ent->m_ItemType <= ITEM_AMMO_LARGE) {
			if (ent->m_ItemType == ITEM_AMMO_SMALL) ent->AddESPString("{i}");
			if (ent->m_ItemType == ITEM_AMMO_MEDIUM) ent->AddESPString("{ii}");
			if (ent->m_ItemType == ITEM_AMMO_LARGE) ent->AddESPString("{iii}");
		} else if (item_powerups && ent->m_ItemType >= ITEM_POWERUP_FIRST && ent->m_ItemType <= ITEM_POWERUP_LAST) {
			ent->AddESPString("%s PICKUP", powerups[ent->m_ItemType - ITEM_POWERUP_FIRST]);
		} else if (item_weapon_spawners && ent->m_ItemType >= ITEM_TF2C_W_FIRST && ent->m_ItemType <= ITEM_TF2C_W_LAST) {
			ent->AddESPString("%s SPAWNER", tf2c_weapon_names[ent->m_ItemType - ITEM_TF2C_W_FIRST].c_str());
			if (CE_BYTE(ent, netvar.bRespawning)) ent->AddESPString("-- RESPAWNING --");
		}
		if (show_distance && shown) {
			ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
		}
	} else if (ent->m_Type == ENTITY_BUILDING && buildings) {
		if (!ent->m_bEnemy && !teammates) return;
		int level = CE_INT(ent, netvar.iUpgradeLevel);
		const char* name = (ent->m_iClassID == g_pClassID->CObjectTeleporter ? "Teleporter" : (ent->m_iClassID == g_pClassID->CObjectSentrygun ? "Sentry Gun" : "Dispenser"));
		if (legit && ent->m_iTeam != g_pLocalPlayer->team) {
			/*if (ent->m_lLastSeen > v_iLegitSeenTicks->GetInt()) {
				return;
			}*/
		}
		ent->AddESPString("LV %i %s", level, name);
		if (show_health) {
			ent->AddESPString("%i / %i HP", ent->m_iHealth, ent->m_iMaxHealth);
			ent->GetESPString(ent->m_nESPStrings - 1).m_bColored = true;
			if (vischeck) {
				ent->GetESPString(ent->m_nESPStrings - 1).m_nColor = colors::Transparent(colors::Health(ent->m_iHealth, ent->m_iMaxHealth), ent->IsVisible() ? 1.0 : 0.5f);
			} else {
				ent->GetESPString(ent->m_nESPStrings - 1).m_nColor = colors::Health(ent->m_iHealth, ent->m_iMaxHealth);
			}
		}
		if (show_distance) {
			ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
		}
		return;
	} else if (ent->m_Type == ENTITY_PLAYER && ent->m_bAlivePlayer) {
		if (!(local_esp && g_IInput->CAM_IsThirdPerson()) &&
			ent->m_IDX == g_IEngine->GetLocalPlayer()) return;
		int pclass = CE_INT(ent, netvar.iClass);
		player_info_t info;
		if (!g_IEngine->GetPlayerInfo(ent->m_IDX, &info)) return;
		powerup_type power = GetPowerupOnPlayer(ent);
		// If target is enemy, always show powerups, if player is teammate, show powerups
		// only if bTeammatePowerup or bTeammates is true
		if (legit && ent->m_iTeam != g_pLocalPlayer->team  && !GetRelation(ent)) {
			if (IsPlayerInvisible(ent)) return;
			/*if (ent->m_lLastSeen > (unsigned)v_iLegitSeenTicks->GetInt()) {
				return;
			}*/
		}
		if (power >= 0 && (ent->m_bEnemy || teammates)) {
			ent->AddESPString("HAS [%s]", powerups[power]);
		}
		if (ent->m_bEnemy || teammates || GetRelation(ent)) {
			if (show_name)
				ent->AddESPString("%s", info.name);
			if (show_class) {
				if (pclass > 0 && pclass < 10)
					ent->AddESPString("%s", classes[pclass - 1].c_str());
			}
			if (show_health) {
				ent->AddESPString("%i / %i HP", ent->m_iHealth, ent->m_iMaxHealth);
				ent->GetESPString(ent->m_nESPStrings - 1).m_bColored = true;
				if (vischeck) {
					ent->GetESPString(ent->m_nESPStrings - 1).m_nColor = colors::Transparent(colors::Health(ent->m_iHealth, ent->m_iMaxHealth), ent->IsVisible() ? 1.0 : 0.5f);
				} else {
					ent->GetESPString(ent->m_nESPStrings - 1).m_nColor = colors::Health(ent->m_iHealth, ent->m_iMaxHealth);
				}
			}
			if (show_conditions && TF) {
				if (IsPlayerInvisible(ent)) {
					ent->AddESPString("INVISIBLE");
				}
				if (IsPlayerInvulnerable(ent)) {
					ent->AddESPString("INVULNERABLE");
				}
				if (HasCondition(ent, TFCond_UberBulletResist)) {
					ent->AddESPString("VACCINATOR ACTIVE");
				}
				if (HasCondition(ent, TFCond_SmallBulletResist)) {
					ent->AddESPString("VACCINATOR PASSIVE");
				}
				if (IsPlayerCritBoosted(ent)) {
					ent->AddESPString("CRIT BOOSTED");
				}
			}
			if (show_distance) {
				ent->AddESPString("%im", (int)(ent->m_flDistance / 64 * 1.22f));
			}
		}
		return;
	}
}

void ProcessEntityPT(CachedEntity* ent) {
	if (!enabled) return;
	if (!box_esp) return;
	if (CE_BAD(ent)) return;
	if (!(local_esp && g_IInput->CAM_IsThirdPerson()) &&
		ent->m_IDX == g_IEngine->GetLocalPlayer()) return;
	int fg = ent->m_ESPColorFG;
	switch (ent->m_Type) {
	case ENTITY_PLAYER: {
		bool cloak = IsPlayerInvisible(ent);
		if (legit && ent->m_iTeam != g_pLocalPlayer->team && !GetRelation(ent)) {
			if (cloak) return;
			/*if (ent->m_lLastSeen > v_iLegitSeenTicks->GetInt()) {
				return;
			}*/
		}
		if (!ent->m_bEnemy && !teammates && !GetRelation(ent)) break;
		if (!ent->m_bAlivePlayer) break;

		DrawBox(ent, fg, 3.0f, -15.0f, true, CE_INT(ent, netvar.iHealth), ent->m_iMaxHealth);
	break;
	}
	case ENTITY_BUILDING: {
		if (legit && ent->m_iTeam != g_pLocalPlayer->team) {
			/*if (ent->m_lLastSeen > v_iLegitSeenTicks->GetInt()) {
				return;
			}*/
		}
		if (CE_INT(ent, netvar.iTeamNum) == g_pLocalPlayer->team && !teammates) break;
		DrawBox(ent, fg, 1.0f, 0.0f, true, CE_INT(ent, netvar.iBuildingHealth), CE_INT(ent, netvar.iBuildingMaxHealth));
	break;
	}
	}
}

}}}
