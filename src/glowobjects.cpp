/*
 * glowobjects.cpp
 *
 *  Created on: Jan 16, 2017
 *      Author: nullifiedcat
 */

#include "glowobjects.h"
#include "common.h"

static CatEnum glow_color_scheme_enum({"ESP", "HEALTH"});
static CatVar glow_color_scheme(glow_color_scheme_enum, "glow_old_color_scheme", "1", "Colors", "Doesn't need a description");
// Doesn't work - has to be registered manually.
static CatVar glow_ammo_boxes(CV_SWITCH, "glow_old_ammo_boxes", "0", "Ammo", "Render glow on ammoboxes");
static CatVar glow_health_packs(CV_SWITCH, "glow_old_health_packs", "0", "Health", "Render glow on medkits");
static CatVar glow_teammates(CV_SWITCH, "glow_old_teammates", "0", "Teammates", "Render glow on teammates");
static CatVar glow_teammate_buildings(CV_SWITCH, "glow_old_teammate_buildings", "0", "Teammate buildings", "Render glow on teammates buildings");
static CatVar glow_buildings(CV_SWITCH, "glow_old_buildings", "1", "Buildings", "Render glow on buildings");
static CatVar glow_stickies(CV_SWITCH, "glow_old_stickies", "0", "Stickies", "Render glow on stickybombs");
static CatVar glow_players(CV_SWITCH, "glow_old_players", "1", "Players", "Render glow on player models");

int CGlowObjectManager::EnableGlow(IClientEntity* entity, int color) {
	int idx = GlowHandle(entity);
	if (idx != -1) {
		logging::Info("[WARNING] Glow handle exists!!!");
		return idx;
	}
	if (m_nFirstFreeSlot == END_OF_FREE_LIST) {
		idx = m_GlowObjectDefinitions.AddToTail();
	} else {
		idx = m_nFirstFreeSlot;
		m_nFirstFreeSlot = m_GlowObjectDefinitions[idx].m_nNextFreeSlot;
	}
	g_CathookManagedGlowObjects.insert(idx);
	GlowObjectDefinition_t& def = m_GlowObjectDefinitions[idx];
	def.m_bRenderWhenOccluded = true;
	def.m_bRenderWhenUnoccluded = true;
	def.m_flGlowAlpha = 1.0f;
	def.m_hEntity = CBaseHandle();
	def.m_hEntity.Set(entity);
	unsigned char _b = (color >> 16) & 0xFF;
	unsigned char _g = (color >> 8)  & 0xFF;
	unsigned char _r = (color) & 0xFF;
	def.m_vGlowColor = Vector((float)_r / 255.0f, (float)_g / 255.0f, (float)_b / 255.0f);
	def.m_nNextFreeSlot = ENTRY_IN_USE;
	def.m_nSplitScreenSlot = -1;
	return idx;
}

void CGlowObjectManager::DisableGlow(int idx) {
	const auto& it = g_CathookManagedGlowObjects.find(idx);
	if (it == g_CathookManagedGlowObjects.end()) {
		return;
	}
	g_CathookManagedGlowObjects.erase(it);
	m_GlowObjectDefinitions[idx].m_nNextFreeSlot = m_nFirstFreeSlot;
	m_GlowObjectDefinitions[idx].m_hEntity = NULL;
	m_nFirstFreeSlot = idx;
}

int CGlowObjectManager::GlowHandle(IClientEntity* entity) const {
	for (int i = 0; i < m_GlowObjectDefinitions.Count(); i++) {
		if (m_GlowObjectDefinitions[i].m_nNextFreeSlot == ENTRY_IN_USE && (m_GlowObjectDefinitions[i].m_hEntity.m_Index & 0xFFF) == entity->entindex()) return i;
	}
	return -1;
}

int GetEntityGlowColor(int idx) {
	if (idx < 0 || idx > g_IEntityList->GetHighestEntityIndex()) return colors::white;
	CachedEntity* ent = ENTITY(idx);
	if (CE_BAD(ent)) return colors::white;
	switch (ent->m_Type) {
	case ENTITY_BUILDING:
		if (!ent->m_bEnemy && !(glow_teammates || glow_teammate_buildings)) {
			return 0;
		}
		if ((int)glow_color_scheme == 1) {
			return colors::Health(ent->m_iHealth, ent->m_iMaxHealth);
		}
		break;
	case ENTITY_PLAYER:
		if (!glow_players) return 0;
		if (!ent->m_bEnemy && !glow_teammates) return 0;
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
		if (!glow_players) return false;
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
	case ENTITY_GENERIC:
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
		break;
	}
	return false;
}

std::set<int> g_CathookManagedGlowObjects {};

CGlowObjectManager* g_GlowObjectManager = 0;
