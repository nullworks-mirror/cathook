/*
 * entitycache.h
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#ifndef ENTITYCACHE_H_
#define ENTITYCACHE_H_

#include "enums.h"
#include "itemtypes.h"
#include "fixsdk.h"
#include <mathlib/vector.h>

struct matrix3x4_t;

class IClientEntity;
struct player_info_s;
struct model_t;
struct mstudiohitboxset_t;
struct mstudiobbox_t;

#define MAX_STRINGS 16
#define MAX_ENTITIES 2048

#define PROXY_ENTITY true

#if PROXY_ENTITY == true
#define RAW_ENT(ce) ((ce) ? (ce)->InternalEntity() : nullptr)
#else
#define RAW_ENT(ce) ce->m_pEntity
#endif

#define CE_VAR(entity, offset, type) \
	NET_VAR(RAW_ENT(entity), offset, type)

#define CE_INT(entity, offset) CE_VAR(entity, offset, int)
#define CE_FLOAT(entity, offset) CE_VAR(entity, offset, float)
#define CE_BYTE(entity, offset) CE_VAR(entity, offset, unsigned char)
#define CE_VECTOR(entity, offset) CE_VAR(entity, offset, Vector)

#define CE_GOOD(entity) (!g_Settings.bInvalid && dynamic_cast<CachedEntity*>(entity) && RAW_ENT(entity) && !RAW_ENT(entity)->IsDormant())
#define CE_BAD(entity) (!CE_GOOD(entity))

#define IDX_GOOD(idx) (idx >= 0 && idx <= HIGHEST_ENTITY && idx < MAX_ENTITIES)
#define IDX_BAD(idx) !IDX_GOOD(idx)

#define HIGHEST_ENTITY (entity_cache::max)
#define ENTITY(idx) (&entity_cache::Get(idx))

struct CachedHitbox {
	Vector min;
	Vector max;
	Vector center;
	mstudiobbox_t* bbox;
};

#define CACHE_MAX_HITBOXES 64

class EntityHitboxCache {
public:
	EntityHitboxCache(CachedEntity* parent);
	~EntityHitboxCache();

	CachedHitbox* GetHitbox(int id);
	void Update();
	void InvalidateCache();
	bool VisibilityCheck(int id);
	void Init();
	int GetNumHitboxes();

	bool* m_VisCheckValidationFlags;
	bool* m_VisCheck;

	int m_nNumHitboxes;
	model_t* m_pLastModel;
	bool m_bModelSet;
	mstudiohitboxset_t* m_pHitboxSet;
	bool m_bInit;
	bool m_bSuccess;
	CachedEntity* m_pParentEntity;
	bool* m_CacheValidationFlags;
	CachedHitbox* m_CacheInternal;
};

class CachedEntity {
public:
	CachedEntity();
	~CachedEntity();

	void Update();

	// Entity fields start here
	EntityType m_Type { ENTITY_GENERIC };

	int m_iClassID { 0 };
	float m_flDistance { 0.0f };

	bool m_bCritProjectile { false };
	bool m_bGrenadeProjectile { false };

	bool m_bAnyHitboxVisible { false };
	bool m_bVisCheckComplete { false };
	bool IsVisible();

	Vector m_vecOrigin { 0 };

	k_EItemType m_ItemType { ITEM_NONE };
	int  m_iTeam { 0 };
	bool m_bAlivePlayer { false };
	bool m_bEnemy { false };
	int m_iMaxHealth { 0 };
	int m_iHealth { 0 };

	unsigned long m_lSeenTicks { 0 };
	unsigned long m_lLastSeen { 0 };

	player_info_s* m_pPlayerInfo { nullptr };
	matrix3x4_t* m_Bones { nullptr };
	bool m_bBonesSetup { false };
	matrix3x4_t* GetBones();

	// Players, Buildings, Stickies


	// Entity fields end here.

	EntityHitboxCache* m_pHitboxCache { nullptr };
	const int m_IDX;
	IClientEntity* InternalEntity();
	Vector m_vecVOrigin { 0 };
	Vector m_vecVelocity { 0 };
	Vector m_vecAcceleration { 0 };
	float m_fLastUpdate { 0.0f };
#if PROXY_ENTITY != true || 1 // FIXME??
	IClientEntity* m_pEntity { nullptr };
#endif
};

namespace entity_cache {

extern CachedEntity array[MAX_ENTITIES];
inline CachedEntity& Get(int idx) {
	if (idx < 0 || idx >= 2048) throw std::out_of_range("Entity index out of range!");
	return array[idx];
}
void Update();
void Invalidate();
extern int max;

}

#endif /* ENTITYCACHE_H_ */
