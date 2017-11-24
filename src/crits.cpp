/*
 * crits.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <link.h>

CatVar crit_info(CV_SWITCH, "crit_info", "0", "Show crit info");
CatVar crit_key(CV_KEY, "crit_key", "0", "Crit Key");
CatVar crit_melee(CV_SWITCH, "crit_melee", "0", "Melee crits");

std::unordered_map<int, int> command_number_mod {};
int* g_PredictionRandomSeed = nullptr;

namespace criticals
{

int find_next_random_crit_for_weapon(IClientEntity *weapon)
{
    int tries = 0,
        number = g_pUserCmd->command_number,
        found = 0,
        seed,
        seed_md5;

    crithack_saved_state state;
    state.Save(weapon);

    while (!found && tries < 4096)
    {
        seed_md5 = MD5_PseudoRandom(number) & 0x7FFFFFFF;
        *g_PredictionRandomSeed = seed_md5;
        seed = seed_md5 ^ (LOCAL_E->m_IDX | (LOCAL_W->m_IDX << 8));
        found = re::C_TFWeaponBase::CalcIsAttackCritical(weapon);
        if (found)
            break;
        ++tries;
        ++number;
    }

    state.Load(weapon);
    if (found)
        return number;
    return 0;
}

void unfuck_bucket(IClientEntity *weapon)
{
    static bool changed;
    static float last_bucket;
    static int last_weapon;

    if (g_pUserCmd->command_number)
        changed = false;

    float& bucket = re::C_TFWeaponBase::crit_bucket_(weapon);

    if (bucket != last_bucket)
    {
        if (changed && weapon->entindex() == last_weapon)
        {
            bucket = last_bucket;
        }
        changed = true;
    }
    last_weapon = weapon->entindex();
    last_bucket = bucket;
}

struct cached_calculation_s
{
    int command_number;
    int weapon_entity;
};

cached_calculation_s cached_calculation {};

bool force_crit(IClientEntity *weapon)
{
    if (cached_calculation.weapon_entity == weapon->entindex())
        return bool(cached_calculation.command_number);

    int number = find_next_random_crit_for_weapon(weapon);

    cached_calculation.command_number = number;
    cached_calculation.weapon_entity = LOCAL_W->m_IDX;
    return false;
}

void create_move()
{
    if (!random_crits_enabled())
        return;
    if (CE_BAD(LOCAL_W))
        return;
    IClientEntity *weapon = RAW_ENT(LOCAL_W);
    if (!re::C_TFWeaponBase::IsBaseCombatWeapon(weapon))
        return;
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon))
        return;
    unfuck_bucket(weapon);
    if (crit_key.KeyDown())
    {
        force_crit(weapon);
    }
}

bool random_crits_enabled() {
    static ConVar* tf_weapon_criticals = g_ICvar->FindVar("tf_weapon_criticals");
    return tf_weapon_criticals->GetBool();
}

}

void crithack_saved_state::Load(IClientEntity* entity) {
	*(float*)((uintptr_t)entity + 2612) = bucket;
	*(float*)((uintptr_t)entity + 2831) = unknown2831;
	*(int*)((uintptr_t)entity + 2868) = seed;
	*(float*)((uintptr_t)entity + 2872) = time;
	*(int*)((uintptr_t)entity + 2616) = unknown2616;
	*(int*)((uintptr_t)entity + 2620) = unknown2620;
	 *(float*)((uintptr_t)entity + 2856) = unknown2856;
	*(float*)((uintptr_t)entity + 2860) = unknown2860;
}

void crithack_saved_state::Save(IClientEntity* entity) {
	bucket = *(float*)((uintptr_t)entity + 2612);
	unknown2831 = *(float*)((uintptr_t)entity + 2831);
	seed = *(int*)((uintptr_t)entity + 2868);
	time = *(float*)((uintptr_t)entity + 2872);
	unknown2616 = *(int*)((uintptr_t)entity + 2616);
	unknown2620 = *(int*)((uintptr_t)entity + 2620);
	unknown2856 = *(float*)((uintptr_t)entity + 2856);
	unknown2860 = *(float*)((uintptr_t)entity + 2860);
}
