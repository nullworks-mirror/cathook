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

static CatVar glow_enabled(CV_SWITCH, "glow_old_enabled", "0", "Enable", "Make sure to enable glow_outline_effect_enable in tf2 settings");
static CatVar glow_alpha(CV_FLOAT, "glow_old_alpha", "1", "Alpha", "Glow Transparency", 0.0f, 1.0f);

void GlowFrameStageNotify(int stage) {
	static ConVar* glow_outline_effect = g_ICvar->FindVar("glow_outline_effect_enable");
	if (glow_outline_effect->GetBool()) {
		PROF_SECTION(FSN_outline);
		if (glow_enabled) {
			for (int i = 0; i < g_GlowObjectManager->m_GlowObjectDefinitions.Size(); i++) {
				GlowObjectDefinition_t& glowobject = g_GlowObjectManager->m_GlowObjectDefinitions[i];
				if (glowobject.m_nNextFreeSlot != ENTRY_IN_USE)
					continue;
				int color = GetEntityGlowColor(glowobject.m_hEntity.m_Index & 0xFFF);
				if (color == 0) {
					glowobject.m_flGlowAlpha = 0.0f;
				} else {
					glowobject.m_flGlowAlpha = (float)glow_alpha;
				}
				unsigned char _b = (color >> 16) & 0xFF;
				unsigned char _g = (color >> 8)  & 0xFF;
				unsigned char _r = (color) & 0xFF;
				glowobject.m_vGlowColor.x = (float)_r / 255.0f;
				glowobject.m_vGlowColor.y = (float)_g / 255.0f;
				glowobject.m_vGlowColor.z = (float)_b / 255.0f;
			}
		}
		// Remove glow from dead entities
		for (int i = 0; i < g_GlowObjectManager->m_GlowObjectDefinitions.Count(); i++) {
			if (g_GlowObjectManager->m_GlowObjectDefinitions[i].m_nNextFreeSlot != ENTRY_IN_USE) continue;
			IClientEntity* ent = (IClientEntity*)g_IEntityList->GetClientEntityFromHandle(g_GlowObjectManager->m_GlowObjectDefinitions[i].m_hEntity);
			if (ent && ent->IsDormant()) {
				g_GlowObjectManager->DisableGlow(i);
			} else if (ent && ent->GetClientClass()->m_ClassID == RCC_PLAYER) {
				if (NET_BYTE(ent, netvar.iLifeState) != LIFE_ALIVE) {
					g_GlowObjectManager->DisableGlow(i);
				}
			}
		}
		if (glow_enabled) {
			for (int i = 1; i < g_IEntityList->GetHighestEntityIndex(); i++) {
				IClientEntity* entity = g_IEntityList->GetClientEntity(i);
				if (!entity || i == g_IEngine->GetLocalPlayer() || entity->IsDormant())
					continue;
				if (!CanEntityEvenGlow(i)) continue;
				int clazz = entity->GetClientClass()->m_ClassID;
				int current_handle = g_GlowObjectManager->GlowHandle(entity);
				bool shouldglow = ShouldEntityGlow(i);
				if (current_handle != -1) {
					if (!shouldglow) {
						g_GlowObjectManager->DisableGlow(current_handle);
					}
				} else {
					if (shouldglow) {
						g_GlowObjectManager->EnableGlow(entity, colors::white);
					}
				}
			}
		}
	}
}

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
	return (classid == CL_CLASS(CTFAmmoPack) || classid == CL_CLASS(CTFPlayer) || classid == CL_CLASS(CTFGrenadePipebombProjectile) || classid == CL_CLASS(CObjectDispenser) || classid == CL_CLASS(CObjectSentrygun) || classid == CL_CLASS(CObjectTeleporter) || classid == CL_CLASS(CBaseAnimating));
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
		if (glow_stickies && ent->m_iClassID == CL_CLASS(CTFGrenadePipebombProjectile)) {
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
