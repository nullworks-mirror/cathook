/*
 * crits.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include <link.h>

CatVar crit_hack_next(CV_SWITCH, "crit_hack_next", "0", "Next crit info");
CatVar crit_info(CV_SWITCH, "crit_info", "0", "Show crit info"); // TODO separate
CatVar crit_hack(CV_KEY, "crit_hack", "0", "Crit Key");
CatVar crit_melee(CV_SWITCH, "crit_melee", "0", "Melee crits");
CatVar crit_suppress(CV_SWITCH, "crit_suppress", "0", "Disable random crits", "Can help saving crit bucket for forced crits");
CatVar experimental_crit_hack(CV_KEY, "crit_hack_experimental", "0", "Unstable Crit Hack", "Experimental crit hack, use this **OR** old crit hack, do not use both!\nNEEDS NEXT CRIT INFO TO BE ACTIVE!");

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

bool force_crit()
{

    return false;
}

}

bool CritKeyDown() {
	return g_IInputSystem->IsButtonDown(static_cast<ButtonCode_t>((int)hacks::shared::misc::crit_hack));// || g_IInputSystem->IsButtonDown(static_cast<ButtonCode_t>((int)experimental_crit_hack));
}

bool AllowAttacking() {
	return true;
	/*
        if (!(CritKeyDown() || ((GetWeaponMode() == weapon_melee) && hacks::shared::misc::crit_melee)) && !hacks::shared::misc::crit_suppress) return true;
	bool crit = IsAttackACrit(g_pUserCmd);
	LoadSavedState();
	if (hacks::shared::misc::crit_suppress && !(CritKeyDown() || ((GetWeaponMode() == weapon_melee) && hacks::shared::misc::crit_melee))) {
		if (crit && !IsPlayerCritBoosted(LOCAL_E)) {
			return false;
		}
	} else if ((CritKeyDown() || ((GetWeaponMode() == weapon_melee) && hacks::shared::misc::crit_melee)) && RandomCrits() && WeaponCanCrit() && (g_pLocalPlayer->weapon()->m_iClassID != CL_CLASS(CTFKnife))) {
		if (!crit) return false;
	}
	return true;*/
}

void ModifyCommandNumber() {

}

bool RandomCrits() {
	static ConVar* tf_weapon_criticals = g_ICvar->FindVar("tf_weapon_criticals");
	return tf_weapon_criticals->GetBool();
}

bool weapon_can_crit_last = false;

bool WeaponCanCrit() {
	IF_GAME (!IsTF()) return false;
	if (CE_BAD(LOCAL_W)) return false;
	IClientEntity* weapon = RAW_ENT(LOCAL_W);
	weapon_can_crit_last = vfunc<bool(*)(IClientEntity*)>(weapon, 190, 0)(weapon) && vfunc<bool(*)(IClientEntity*)>(weapon, 465 + 21, 0)(weapon);
	return weapon_can_crit_last;
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

static crithack_saved_state state;
static bool state_saved { false };
void LoadSavedState() {
	// TODO TF2C Crit Hack
	IF_GAME (!IsTF2()) return;
	if (!state_saved) return;
	if (CE_GOOD(LOCAL_W)) {
		IClientEntity* weapon = RAW_ENT(LOCAL_W);
		state.Load(weapon);
	}
}
void ResetCritHack() {
	state_saved = false;
}

bool IsAttackACrit(CUserCmd* cmd) {
	// TODO TF2C Crit Hack
	IF_GAME (!IsTF2()) return false;
	if (CE_GOOD(LOCAL_W)) {
		IClientEntity* weapon = RAW_ENT(LOCAL_W);
		/*if (TF2C) {
			if (vfunc<bool(*)(IClientEntity*)>(weapon, 1824 / 4, 0)(weapon)) {
				static uintptr_t CalcIsAttackCritical_s = gSignatures.GetClientSignature("55 89 E5 56 53 83 EC 10 8B 5D 08 89 1C 24 E8 ? ? ? ? 85 C0 89 C6 74 59 8B 00 89 34 24 FF 90 E0 02 00 00 84 C0 74 4A A1 ? ? ? ? 8B 40 04 3B 83 A8 09 00 00 74 3A");
				typedef void(*CalcIsAttackCritical_t)(IClientEntity*);
				CalcIsAttackCritical_t CIACFn = (CalcIsAttackCritical_t)(CalcIsAttackCritical_s);
				*(float*)((uintptr_t)weapon + 2468ul) = 0.0f;
				int tries = 0;
				static int lcmdn = 0;
				if (*(bool*)((uintptr_t)RAW_ENT(LOCAL_W) + 2454ul)) return true;
				static int& seed = *(int*)(sharedobj::client->lmap->l_addr + 0x00D53F68ul);
				bool cmds = false;
				seed = MD5_PseudoRandom(cmd->command_number) & 0x7fffffff;
				RandomSeed(seed);
				CIACFn(RAW_ENT(LOCAL_W));
				return *(bool*)((uintptr_t)RAW_ENT(LOCAL_W) + 2454ul);
			}
		} else if (TF2) */
		{
			if (vfunc<bool(*)(IClientEntity*)>(weapon, 1944 / 4, 0)(weapon)) {
				static uintptr_t CalcIsAttackCritical_s = gSignatures.GetClientSignature("55 89 E5 83 EC 28 89 5D F4 8B 5D 08 89 75 F8 89 7D FC 89 1C 24 E8 ? ? ? ? 85 C0 89 C6 74 60 8B 00 89 34 24 FF 90 E0 02 00 00 84 C0 74 51 A1 ? ? ? ? 8B 40 04");
				typedef void(*CalcIsAttackCritical_t)(IClientEntity*);
				CalcIsAttackCritical_t CIACFn = (CalcIsAttackCritical_t)(CalcIsAttackCritical_s);
				if (cmd->command_number) {
					int tries = 0;
					static int cmdn = 0;
					bool chc = false;
					int md5seed = MD5_PseudoRandom(cmd->command_number) & 0x7fffffff;
					int rseed = md5seed;
					//float bucket = *(float*)((uintptr_t)RAW_ENT(LOCAL_W) + 2612u);

					*g_PredictionRandomSeed = md5seed;
					int c = LOCAL_W->m_IDX << 8;
					int b = LOCAL_E->m_IDX;
					rseed = rseed ^ (b | c);
					RandomSeed(rseed);
					if (GetWeaponMode() == weapon_melee) {
						*(float*)((uintptr_t)RAW_ENT(LOCAL_W) + 2612u) = 1000.0f;
					}
					state.Save(weapon);

					state_saved = true;
					//float saved_time = *(float*)(weapon + 2872ul);
					//*(float*)(weapon + 2872ul) = 0.0f;
					bool crits = vfunc<bool(*)(IClientEntity*)>(weapon, 1836 / 4, 0)(weapon);
					//*(float*)(weapon + 2872ul) = saved_time;
					//if (!crits) *(float*)((uintptr_t)RAW_ENT(LOCAL_W) + 2612u) = bucket;
					return crits;
				}
			}
		}
	}
	return false;
}
