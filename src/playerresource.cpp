/*
 * playerresource.cpp
 *
 *  Created on: Nov 13, 2016
 *      Author: nullifiedcat
 */

#include "common.h"

void TFPlayerResource::Update() {
	entity = 0;
	for (int i = 0; i < HIGHEST_ENTITY; i++) {
		IClientEntity* ent = g_IEntityList->GetClientEntity(i);
		if (ent && ent->GetClientClass()->m_ClassID == (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) {
			entity = i;
			return;
		}
	}
}

int TFPlayerResource::GetMaxHealth(CachedEntity* player) {
	if (HL2DM) return 100;
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	int idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iMaxHealth + 4 * idx);
}

int TFPlayerResource::GetMaxBuffedHealth(CachedEntity* player) {
	if (!TF2) return GetMaxHealth(player);
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	int idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iMaxBuffedHealth + 4 * idx);
}

int TFPlayerResource::GetTeam(int idx) {
	if (idx >= 64 || idx < 0) return 0;
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	return *(int*)((unsigned)ent + netvar.res_iTeam + 4 * idx);
}

int TFPlayerResource::GetClass(CachedEntity* player) {
	IClientEntity* ent = g_IEntityList->GetClientEntity(entity);
	if (!ent || ent->GetClientClass()->m_ClassID != (TF ? g_pClassID->CTFPlayerResource : g_pClassID->CPlayerResource)) return 0;
	int idx = player->m_IDX;
	if (idx >= 64 || idx < 0) return 0;
	return *(int*)((unsigned)ent + netvar.res_iPlayerClass + 4 * idx);
}


TFPlayerResource* g_pPlayerResource = 0;
