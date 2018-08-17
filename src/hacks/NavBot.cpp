//
// Created by bencat07 on 17.08.18.
//
#include "common.hpp"
#include "navparser.hpp"
#include "NavBot.hpp"

namespace hacks::tf2::NavBot
{
static settings::Bool enable("navbot.enable", "false");
static settings::Bool primary_only("navbot.primary-only", "true");

bool HasLowAmmo()
{
    int *weapon_list =
        (int *) ((unsigned) (RAW_ENT(LOCAL_E)) + netvar.hMyWeapons);
    for (int i = 0; weapon_list[i]; i++)
    {
        int handle = weapon_list[i];
        int eid    = handle & 0xFFF;
        if (eid >= 32 && eid <= HIGHEST_ENTITY)
        {
            IClientEntity *weapon = g_IEntityList->GetClientEntity(eid);
            if (weapon and
                re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon) and
                re::C_TFWeaponBase::UsesPrimaryAmmo(weapon) and
                not re::C_TFWeaponBase::HasPrimaryAmmo(weapon))
                return true;
        }
    }
    return false;
}

bool HasLowHealth()
{
    return float(LOCAL_E->m_iHealth()) / float(LOCAL_E->m_iMaxHealth()) < 0.45;
}

CachedEntity *nearestHealth()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CBaseAnimating))
            continue;
        if (ent->m_ItemType() != ITEM_HEALTH_SMALL &&
            ent->m_ItemType() != ITEM_HEALTH_MEDIUM &&
            ent->m_ItemType() != ITEM_HEALTH_LARGE)
            continue;
        if (ent->m_flDistance() < bestscr)
        {
            bestscr = ent->m_flDistance();
            bestent = ent;
        }
    }
    return bestent;
}
CachedEntity *nearestAmmo()
{
    float bestscr         = FLT_MAX;
    CachedEntity *bestent = nullptr;
    for (int i = 0; i < HIGHEST_ENTITY; i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_BAD(ent) || ent->m_iClassID() != CL_CLASS(CBaseAnimating))
            continue;
        if (ent->m_ItemType() != ITEM_AMMO_SMALL &&
            ent->m_ItemType() != ITEM_AMMO_MEDIUM &&
            ent->m_ItemType() != ITEM_AMMO_LARGE)
            continue;
        if (ent->m_flDistance() < bestscr)
        {
            bestscr = ent->m_flDistance();
            bestent = ent;
        }
    }
    return bestent;
}
std::vector<Vector> sniper_spots;
void Init()
{
    sniper_spots.clear();
    for (auto area : nav::areas)
        for (auto hide : area.m_hidingSpots)
            if (hide.IsGoodSniperSpot() || hide.IsIdealSniperSpot() || hide.IsExposed())
                sniper_spots.push_back(hide.m_pos);

}
Timer slot_timer{};
void UpdateSlot()
{
    if (!slot_timer.test_and_set(1000))
        return;
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W) && !g_pLocalPlayer->life_state)
    {
        IClientEntity *weapon = RAW_ENT(LOCAL_W);
        // IsBaseCombatWeapon()
        if (re::C_BaseCombatWeapon::IsBaseCombatWeapon(weapon))
        {
            int slot = re::C_BaseCombatWeapon::GetSlot(weapon);
            if (slot != 0)
                g_IEngine->ClientCmd(format("slot", 1).c_str());

        }
    }
}
Timer cdr{};
Timer cd2{};
Timer cd3{};
void CreateMove()
{
    if (!nav::Prepare() || !enable)
        return;
    if (CE_BAD(LOCAL_E) || !LOCAL_E->m_bAlivePlayer())
        return;
    UpdateSlot();
    if (HasLowHealth() && cdr.test_and_set(5000))
    {
        CachedEntity *med = nearestHealth();
        if (CE_GOOD(med))
        {
            nav::NavTo(med->m_vecOrigin());
            return;
        }
    }
    if (HasLowAmmo() && cdr.test_and_set(5000))
    {
        CachedEntity *ammo = nearestAmmo();
        if (CE_GOOD(ammo))
        {
            nav::NavTo(ammo->m_vecOrigin());
            return;
        }
    }
    if (!nav::ReadyForCommands)
        cd3.update();
    if (nav::ReadyForCommands && cd3.test_and_set(5000))
    {
        Vector random_spot;
        if (sniper_spots.empty())
        {
            if (cd2.test_and_set(5000))
                Init();
            return;
        }
        int rng = rand() % sniper_spots.size();
        random_spot = sniper_spots.at(rng);
        if (random_spot.z)
            nav::NavTo(random_spot);
    }
}
} // namespace hacks::tf2::NavBot