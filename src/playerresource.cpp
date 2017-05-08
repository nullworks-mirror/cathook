/*
 * playerresource.cpp
 *
 *  Created on: Nov 13, 2016
 *      Author: nullifiedcat
 */

#include "common.h"

void TFPlayerResource::Update() {
	static IClientEntity *ent;

	entity = 0;
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		ent = g_IEntityList->GetClientEntity(i);
		if (ent && ent->GetClientClass()->m_ClassID == (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) {
			entity = i;
			return;
		}
	}
}

int TFPlayerResource::GetMaxHealth(CachedEntity* player) {
	static IClientEntity *ent;
	static int idx;

	if (HL2DM) return 100;
	ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iMaxHealth + 4 * idx);
}

int TFPlayerResource::GetMaxBuffedHealth(CachedEntity* player) {
	static IClientEntity *ent;
	static int idx;

	if (!TF2) return GetMaxHealth(player);
	ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iMaxBuffedHealth + 4 * idx);
}

int TFPlayerResource::GetTeam(int idx) {
	static IClientEntity *ent;

	if (idx >= 64 || idx < 0) return 0;
	ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	return *(int*)((unsigned)ent + netvar.res_iTeam + 4 * idx);
}

int TFPlayerResource::GetClass(CachedEntity* player) {
	static IClientEntity *ent;
	static int idx;

	ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iPlayerClass + 4 * idx);
}


TFPlayerResource* g_pPlayerResource = 0;
