/*
 * crits.cpp
 *
 *  Created on: Feb 25, 2017
 *      Author: nullifiedcat
 */

#include <settings/Bool.hpp>
#include "common.hpp"

static settings::Bool crit_info{ "crit.info", "false" };
static settings::Button crit_key{ "crit.key", "<null>" };
static settings::Bool crit_melee{ "crit.melee", "false" };
static settings::Bool crit_legiter{ "crit.force-gameplay", "true" };
static settings::Bool crit_experimental{ "crit.experimental", "false" };
static settings::Bool crit_experimental_melee{ "crit.experimental.melee", "false" };

std::unordered_map<int, int> command_number_mod{};

int *g_PredictionRandomSeed = nullptr;

namespace criticals
{
CatCommand test("crit_debug_print", "debug", []() {
    if (CE_BAD(LOCAL_E))
        return;
    if (CE_BAD(LOCAL_W))
        return;
    unsigned unk1           = *(unsigned *) (RAW_ENT(LOCAL_W) + 2832);
    unsigned unk2           = *(unsigned *) (RAW_ENT(LOCAL_W) + 2820);
    unsigned char CritSlots = *(unsigned char *) (unk1 + (unk2 << 6) + 1844);
    int CritSlots2          = *(unsigned *) (unk1 + (unk2 << 6) + 1788);
    unsigned CritSlots3     = *(unsigned *) (unk1 + (unk2 << 6) + 1788);
    int CritSlots4          = *(int *) (unk1 + (unk2 << 6) + 1788);
    logging::Info("%u %d %d %u %d", unk1, int(CritSlots), CritSlots2,
                  CritSlots3, CritSlots4);
});
int find_next_random_crit_for_weapon(IClientEntity *weapon)
{
    int tries = 0, number = current_user_cmd->command_number, found = 0,
        seed_backup;

    seed_backup = *g_PredictionRandomSeed;
    while (tries < 4096)
    {
        *g_PredictionRandomSeed = MD5_PseudoRandom(number) & 0x7FFFFFFF;
        found = re::C_TFWeaponBase::CalcIsAttackCritical(weapon);
        if (found)
            break;
        ++tries;
        ++number;
    }

    *g_PredictionRandomSeed = seed_backup;
    if (found)
        return number;
    return 0;
}

void unfuck_bucket(IClientEntity *weapon)
{
    static bool changed;
    static float last_bucket;
    static int last_weapon;

    if (current_user_cmd->command_number)
        changed = false;

    float &bucket = re::C_TFWeaponBase::crit_bucket_(weapon);

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
    int init_command;
    int weapon_entity;
};

cached_calculation_s cached_calculation{};

static int number                = 0;
static int lastnumber            = 0;
static int lastusercmd           = 0;
static const model_t *lastweapon = nullptr;

void force_crit(IClientEntity *weapon, bool really_force)
{
    auto command_number = current_user_cmd->command_number;

    if (lastnumber < command_number || lastweapon != weapon->GetModel() ||
        lastnumber - command_number > 1000)
    {
        /*if ((command_number &&
             (cached_calculation.command_number < command_number)))
            cached_calculation.weapon_entity = -1;
        if (cached_calculation.weapon_entity == weapon->entindex())
            return;*/

        number = find_next_random_crit_for_weapon(weapon);
    }
    else
        number = lastnumber;
    // logging::Info("Found critical: %d -> %d", command_number,
    //              number);

    if (really_force)
    {
        if (number && number != command_number)
            command_number_mod[command_number] = number;

        cached_calculation.command_number = number;
        cached_calculation.weapon_entity  = LOCAL_W->m_IDX;
    }
    if (number && number != command_number && number != lastnumber)
        current_user_cmd->buttons &= ~IN_ATTACK;

    lastweapon = weapon->GetModel();
    lastnumber = number;
    return;
}

void create_move()
{
    if (!crit_key && !crit_melee)
        return;
    if (!random_crits_enabled())
        return;
    if (CE_BAD(LOCAL_W))
        return;
    if (current_user_cmd->command_number)
        lastusercmd = current_user_cmd->command_number;
    IClientEntity *weapon = RAW_ENT(LOCAL_W);
    if (!re::C_TFWeaponBase::IsBaseCombatWeapon(weapon))
        return;
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon))
        return;
    unfuck_bucket(weapon);
    if (!current_user_cmd->command_number ||
        !(current_user_cmd->buttons & IN_ATTACK) ||
        (crit_legiter && !CanShoot()))
    {
        return;
    }
    if (crit_melee && GetWeaponMode() == weapon_melee &&
         g_pLocalPlayer->weapon()->m_iClassID() != CL_CLASS(CTFKnife))
    {
        force_crit(weapon, crit_experimental && crit_experimental_melee);
    }
    else if (crit_key && crit_key.isKeyDown())
    {
        force_crit(weapon, bool(crit_experimental));
    }
}

bool random_crits_enabled()
{
    static ConVar *tf_weapon_criticals =
        g_ICvar->FindVar("tf_weapon_criticals");
    return tf_weapon_criticals->GetBool();
}

#if ENABLE_VISUALS
void draw()
{
    if (CE_BAD(LOCAL_W))
        return;
    IClientEntity *weapon = RAW_ENT(LOCAL_W);
    if (!weapon)
        return;
    if (!re::C_TFWeaponBase::IsBaseCombatWeapon(weapon))
        return;
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon))
        return;
    if (crit_info && CE_GOOD(LOCAL_W))
    {
        if (crit_key.isKeyDown())
        {
            AddCenterString("FORCED CRITS!", colors::red);
        }
        IF_GAME(IsTF2())
        {
            if (!random_crits_enabled())
                AddCenterString("Random crits are disabled", colors::yellow);
            else
            {
                if (!re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W),
                                                             false, nullptr))
                    AddCenterString("Weapon can't randomly crit",
                                    colors::yellow);
                else if (lastusercmd)
                {
                    if (number > lastusercmd)
                    {
                        float nextcrit =
                            ((float) number - (float) lastusercmd) / (float) 90;
                        if (nextcrit > 0.0f)
                        {
                            AddCenterString(
                                format("Time to next crit: ", nextcrit, "s"),
                                colors::orange);
                        }
                    }
                    AddCenterString("Weapon can randomly crit");
                }
            }
            AddCenterString(format("Bucket: ", re::C_TFWeaponBase::crit_bucket_(
                                                   RAW_ENT(LOCAL_W))));
        }
    }
}
#endif
} // namespace criticals
