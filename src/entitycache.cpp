/*
 * entitycache.cpp
 *
 *  Created on: Nov 7, 2016
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include <time.h>

bool IsProjectileACrit(CachedEntity *ent)
{
    if (ent->m_bGrenadeProjectile())
        return CE_BYTE(ent, netvar.Grenade_bCritical);
    return CE_BYTE(ent, netvar.Rocket_bCritical);
}
// This method of const'ing the index is weird.
CachedEntity::CachedEntity()
    : m_IDX(int(((unsigned) this - (unsigned) &entity_cache::array) /
                sizeof(CachedEntity))),
      hitboxes(hitbox_cache::Get(unsigned(m_IDX)))
{
#if PROXY_ENTITY != true
    m_pEntity = nullptr;
#endif
    m_fLastUpdate = 0.0f;
}

void CachedEntity::Reset()
{
    m_bAnyHitboxVisible = false;
    m_bVisCheckComplete = false;
    m_lLastSeen         = 0;
    m_lSeenTicks        = 0;
    memset(&player_info, 0, sizeof(player_info_s));
    m_vecAcceleration.Zero();
    m_vecVOrigin.Zero();
    m_vecVelocity.Zero();
    m_fLastUpdate = 0;
}

CachedEntity::~CachedEntity()
{
}

static CatVar ve_window(CV_FLOAT, "debug_ve_window", "0", "VE Window");
static CatVar ve_smooth(CV_SWITCH, "debug_ve_smooth", "1", "VE Smoothing");
static CatVar ve_averager_size(CV_INT, "debug_ve_averaging", "8",
                               "VE Averaging");

void CachedEntity::Update()
{
    auto raw = RAW_ENT(this);

    if (!raw)
        return;
#if PROXY_ENTITY != true
    m_pEntity = g_IEntityList->GetClientEntity(idx);
    if (!m_pEntity)
    {
        return;
    }
#endif
    bool dormant               = raw->IsDormant();
    bool dormant_state_changed = dormant != was_dormant();

    m_lSeenTicks = 0;
    m_lLastSeen  = 0;

    hitboxes.Update();

    m_bVisCheckComplete = false;

    if (m_Type() == EntityType::ENTITY_PLAYER)
        g_IEngine->GetPlayerInfo(m_IDX, &player_info);
}

static CatVar fast_vischeck(CV_SWITCH, "fast_vischeck", "0", "Fast VisCheck",
                            "VisCheck only certain player hitboxes");

bool CachedEntity::IsVisible()
{
    static constexpr int optimal_hitboxes[] = {
        hitbox_t::head, hitbox_t::foot_L, hitbox_t::hand_R, hitbox_t::spine_1
    };
    static bool vischeck0, vischeck;

    PROF_SECTION(CE_IsVisible);
    if (m_bVisCheckComplete)
        return m_bAnyHitboxVisible;

    vischeck0 = IsEntityVectorVisible(this, m_vecOrigin());

    if (vischeck0)
    {
        m_bAnyHitboxVisible = true;
        m_bVisCheckComplete = true;
        return true;
    }

    if (m_Type() == ENTITY_PLAYER && fast_vischeck)
    {
        for (int i = 0; i < 4; i++)
        {
            if (hitboxes.VisibilityCheck(optimal_hitboxes[i]))
            {
                m_bAnyHitboxVisible = true;
                m_bVisCheckComplete = true;
                return true;
            }
        }
        m_bAnyHitboxVisible = false;
        m_bVisCheckComplete = true;
        return false;
    }

    for (int i = 0; i < hitboxes.m_nNumHitboxes; i++)
    {
        vischeck = false;
        vischeck = hitboxes.VisibilityCheck(i);
        if (vischeck)
        {
            m_bAnyHitboxVisible = true;
            m_bVisCheckComplete = true;
            return true;
        }
    }
    m_bAnyHitboxVisible = false;
    m_bVisCheckComplete = true;

    return false;
}

namespace entity_cache
{

CachedEntity array[MAX_ENTITIES]{};

void Update()
{
    max = g_IEntityList->GetHighestEntityIndex();
    if (max >= MAX_ENTITIES)
        max = MAX_ENTITIES - 1;
    for (int i = 0; i <= max; i++)
    {
        array[i].Update();
    }
}

void Invalidate()
{
    for (auto &ent : array)
    {
        // pMuch useless line!
        // ent.m_pEntity = nullptr;
        ent.Reset();
    }
}

int max = 0;
}
