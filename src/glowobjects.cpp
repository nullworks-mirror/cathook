/*
 * glowobjects.cpp
 *
 *  Created on: Jan 16, 2017
 *      Author: nullifiedcat
 */

#include "glowobjects.h"
#include "common.h"

static CatEnum glow_color_scheme_enum({"ESP", "HEALTH"});
static CatVar glow_color_scheme(glow_color_scheme_enum, "glow_color_scheme", "1", "Colors", "Doesn't need a description");
// Doesn't work - has to be registered manually.
//static CatVar glow_ammo_boxes(CV_SWITCH, "glow_ammo_boxes", "0", "Ammo");
//static CatVar glow_health_packs(CV_SWITCH, "glow_health_packs", "0", "Health");
static CatVar glow_teammates(CV_SWITCH, "glow_teammates", "0", "Teammates");
static CatVar glow_teammate_buildings(CV_SWITCH, "glow_teammate_buildings", "0", "Teammate buildings");
static CatVar glow_buildings(CV_SWITCH, "glow_buildings", "1", "Buildings");
static CatVar glow_stickies(CV_SWITCH, "glow_stickies", "0", "Stickies");

int GetEntityGlowColor(int idx) {
	if (idx < 0 || idx > g_IEntityList->GetHighestEntityIndex()) return colors::white;
	CachedEntity* ent = ENTITY(idx);
	if (CE_BAD(ent)) return colors::white;
	switch (ent->m_Type) {
	case ENTITY_BUILDING:
	case ENTITY_PLAYER:
		if ((int)glow_color_scheme == 1) {
			return colors::Health(ent->m_iHealth, ent->m_iMaxHealth);
		}
		break;
	}
	return colors::EntityF(ent);
}

bool CanEntityEvenGlow(int idx) {
	IClientEntity* entity = g_IEntityList->GetClientEntity(idx);
	if (!entity) return false;
	int classid = entity->GetClientClass()->m_ClassID;
	return (classid == g_pClassID->CTFAmmoPack || classid == g_pClassID->CTFPlayer || classid == g_pClassID->CTFGrenadePipebombProjectile || classid == g_pClassID->CObjectDispenser || classid == g_pClassID->CObjectSentrygun || classid == g_pClassID->CObjectTeleporter || classid == g_pClassID->CBaseAnimating);
}

bool ShouldEntityGlow(int idx) {
	CachedEntity* ent = ENTITY(idx);
	if (CE_BAD(ent)) return false;
	switch (ent->m_Type) {
	case ENTITY_BUILDING:
		if (!glow_buildings) return false;
		if (!ent->m_bEnemy && !(glow_teammate_buildings || glow_teammates)) return false;
		return true;
	case ENTITY_PLAYER:
		if (!glow_teammates && !ent->m_bEnemy) return false;
		if (CE_BYTE(ent, netvar.iLifeState) != LIFE_ALIVE) return false;
		return true;
		break;
	case ENTITY_PROJECTILE:
		if (!ent->m_bEnemy) return false;
		if (glow_stickies && ent->m_iClassID == g_pClassID->CTFGrenadePipebombProjectile) {
			return true;
		}
		break;
	/*case ENTITY_GENERIC:
		switch (ent->m_ItemType) {
		case ITEM_HEALTH_LARGE:
		case ITEM_HEALTH_MEDIUM:
		case ITEM_HEALTH_SMALL:
			return glow_health_packs;
		case ITEM_AMMO_LARGE:
		case ITEM_AMMO_MEDIUM:
		case ITEM_AMMO_SMALL:
			return glow_ammo_boxes;
		}
		break;*/
	}
	return false;
}

CGlowObjectManager* g_GlowObjectManager = 0;
