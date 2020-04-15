#include "common.hpp"
#include "crits.hpp"
#include "netadr.h"

std::unordered_map<int, int> command_number_mod{};

namespace criticals
{
settings::Boolean enabled{ "crit.enabled", "false" };
static settings::Boolean draw{ "crit.draw-info", "false" };
settings::Boolean melee{ "crit.melee", "false" };
static settings::Button crit_key{ "crit.key", "<null>" };
static settings::Boolean old_mode{ "crit.old-mode", "false" };

// How much is added to bucket per shot?
float added_per_shot = 0.0f;
class weapon_info
{
public:
    float crit_bucket{};
    unsigned int weapon_seed{};
    unsigned unknown1{};
    unsigned unknown2{};
    bool unknown3{};
    float unknown4{};
    int crit_attempts{};
    int crit_count{};
    float observed_crit_chance{};
    bool unknown7{};
    weapon_info()
    {
    }
    void Load(IClientEntity *weapon)
    {
        crit_bucket          = *(float *) ((uintptr_t) weapon + 0xa38);
        weapon_seed          = *(unsigned int *) ((uintptr_t) weapon + 0xb3c);
        unknown1             = *(unsigned int *) ((uintptr_t) weapon + 0xb30);
        unknown2             = *(unsigned int *) ((uintptr_t) weapon + 0xb34);
        unknown3             = *(bool *) ((uintptr_t) weapon + 0xb17);
        unknown4             = *(float *) ((uintptr_t) weapon + 0xb40);
        crit_attempts        = *(int *) ((uintptr_t) weapon + 0xa3c);
        crit_count           = *(int *) ((uintptr_t) weapon + 0xa40);
        observed_crit_chance = *(float *) ((uintptr_t) weapon + 0xbfc);
        unknown7             = *(bool *) ((uintptr_t) weapon + 0xb18);
    }
    weapon_info(IClientEntity *weapon)
    {
        Load(weapon);
    }
    void restore_data(IClientEntity *weapon)
    {
        *(float *) ((uintptr_t) weapon + 0xa38)        = crit_bucket;
        *(unsigned int *) ((uintptr_t) weapon + 0xb3c) = weapon_seed;
        *(unsigned int *) ((uintptr_t) weapon + 0xb30) = unknown1;
        *(unsigned int *) ((uintptr_t) weapon + 0xb34) = unknown2;
        *(bool *) ((uintptr_t) weapon + 0xb17)         = unknown3;
        *(float *) ((uintptr_t) weapon + 0xb40)        = unknown4;
        *(int *) ((uintptr_t) weapon + 0xa3c)          = crit_attempts;
        *(int *) ((uintptr_t) weapon + 0xa40)          = crit_count;
        *(float *) ((uintptr_t) weapon + 0xbfc)        = observed_crit_chance;
        *(bool *) ((uintptr_t) weapon + 0xb18)         = unknown7;
    }
};

float GetWithdrawMult(IClientEntity *wep)
{
    weapon_info info(wep);
    // Increment call count
    int call_count = info.crit_count + 1;
    // How many times there was a check for crits
    int crit_checks  = info.crit_attempts + 1;
    float flMultiply = 0.5;

    if (g_pLocalPlayer->weapon_mode != weapon_melee)
    {
        float flTmp = min(max((((float) call_count / (float) crit_checks) - 0.1) * 1.111, 0.0), 1.0);
        flMultiply  = (flTmp * 2.0) + 1.0;
    }

    float flToRemove = flMultiply * 3.0;
    return flToRemove;
}

static CatCommand debug_print_mult("debug_print_mult", "print Withdraw multiplier", []() {
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;
    logging::Info("Multiplier: %f", GetWithdrawMult(RAW_ENT(LOCAL_W)));
});

static Timer bucket_cap_refresh{};
float GetBucketCap()
{
    static float return_value;
    static ConVar *tf_weapon_criticals = g_ICvar->FindVar("tf_weapon_criticals_bucket_cap");
    if (bucket_cap_refresh.test_and_set(5000))
        return_value = tf_weapon_criticals->GetFloat();
    return return_value;
}

// Add damage parameter adds the same value that AddToCritbucket would add
bool IsAllowedToWithdrawFromCritBucket(IClientEntity *wep, float flDamage, bool add_damage = true)
{
    weapon_info info(wep);
    if (add_damage)
    {
        info.crit_bucket += flDamage;
        if (info.crit_bucket > GetBucketCap())
            info.crit_bucket = GetBucketCap();
    }
    float flToRemove = GetWithdrawMult(wep) * flDamage;
    // Can remove
    if (flToRemove <= info.crit_bucket)
        return true;

    return false;
}

// As opposed to the function above this does actual writing
void SimulateNormalShot(IClientEntity *wep, float flDamage)
{
    weapon_info info(wep);
    info.crit_bucket += flDamage;
    if (info.crit_bucket > GetBucketCap())
        info.crit_bucket = GetBucketCap();
    // Write other values important for iteration
    info.crit_attempts++;
    info.restore_data(wep);
}

bool CanWeaponCrit(IClientEntity *wep)
{
    // Are we allowed to even use that much?
    if (g_pLocalPlayer->weapon_mode != weapon_melee && !IsAllowedToWithdrawFromCritBucket(wep, added_per_shot))
        return false;
    return true;
}

// Calculate shots until crit
int ShotsUntilCrit(IClientEntity *wep)
{
    weapon_info info(wep);
    // How many shots until we can crit
    int shots = 0;
    // Predicting 100 shots should be fine
    for (shots = 0; shots < 100; shots++)
    {
        if (IsAllowedToWithdrawFromCritBucket(wep, added_per_shot, true))
            break;
        // Do calculations
        SimulateNormalShot(wep, added_per_shot);
    }
    // Restore variables
    info.restore_data(wep);
    return shots;
}

typedef float (*AttribHookFloat_t)(float, const char *, IClientEntity *, void *, bool);

// Calculate Crit penalty
float CalcCritPenalty(IClientEntity *wep)
{
    // Need this to get crit chance from weapon
    static uintptr_t AttribHookFloat            = gSignatures.GetClientSignature("55 89 E5 57 56 53 83 EC 6C C7 45 ? 00 00 00 00 A1 ? ? ? ? C7 45 ? 00 00 00 00 8B 75 ? 85 C0 0F 84 ? ? ? ? 8D 55 ? 89 04 24 31 DB 89 54 24");
    static AttribHookFloat_t AttribHookFloat_fn = AttribHookFloat_t(AttribHookFloat);

    // Player specific Multiplier
    float crit_mult = re::CTFPlayerShared::GetCritMult((re::CTFPlayerShared *) (((uintptr_t) RAW_ENT(LOCAL_E)) + 0x5f3));

    // Weapon specific Multiplier
    static std::string query = "mult_crit_chance";
    float flMultCritChance   = AttribHookFloat_fn(crit_mult * 0.02, query.c_str(), wep, 0, 1);

    return flMultCritChance;
}

std::pair<float, float> CritMultInfo(IClientEntity *wep)
{
    float cur_crit        = CalcCritPenalty(wep);
    float observed_chance = CE_FLOAT(LOCAL_W, netvar.flObservedCritChance);
    float needed_chance   = cur_crit + 0.1f;
    return std::pair<float, float>(observed_chance, needed_chance);
}

CatCommand debug_observed("debug_observed_crit", "debug", []() {
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W))
        logging::Info("%f", CE_FLOAT(LOCAL_W, netvar.flObservedCritChance));
});

CatCommand debug_damage("debug_print_damage_until_crit", "debug", []() {
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W))
    {
        float penalty = CalcCritPenalty(RAW_ENT(LOCAL_W));

        float observed_chance = CE_FLOAT(LOCAL_W, netvar.flObservedCritChance);

        rgba_t color = colors::green;
        if (penalty + 0.1f < observed_chance)
            color = colors::red;

        Color valve_color(color.r * 255.0f, color.g * 255.0f, color.b * 255.0f, color.a * 255.0f);
        g_ICvar->ConsoleColorPrintf(valve_color, "%f\n", observed_chance);
    }
});

CatCommand debug_shots("debug_print_shots_until_crit", "debug", []() {
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W))
        logging::Info("%d", ShotsUntilCrit(RAW_ENT(LOCAL_W)));
});

int next_crit_tick()
{
    auto wep = RAW_ENT(LOCAL_W);

    int old_seed = MD5_PseudoRandom(current_late_user_cmd->command_number) & 0x7FFFFFFF;
    // Try 4096 times
    for (int i = 0; i < 4096; i++)
    {
        int cmd_number = current_late_user_cmd->command_number + i;
        // Set random seed
        *g_PredictionRandomSeed = MD5_PseudoRandom(cmd_number) & 0x7FFFFFFF;
        // Save weapon state to not break anything
        weapon_info info(wep);
        bool is_crit = re::C_TFWeaponBase::CalcIsAttackCritical(wep);
        // Restore state
        info.restore_data(wep);
        // Is a crit
        if (is_crit)
        {
            *g_PredictionRandomSeed = old_seed;
            return cmd_number;
        }
    }
    *g_PredictionRandomSeed = old_seed;
    return -1;
}

static Timer random_crit_refresh{};
bool random_crits_enabled()
{
    static bool return_value;
    static ConVar *tf_weapon_criticals = g_ICvar->FindVar("tf_weapon_criticals");
    if (random_crit_refresh.test_and_set(3000))
        return_value = tf_weapon_criticals->GetBool();
    return return_value;
}

static int force_ticks = 0;

bool IsEnabled()
{
    // Random crits on
    if (!random_crits_enabled())
        return false;
    // Weapon specific checks
    if (force_ticks || ((melee && g_pLocalPlayer->weapon_mode == weapon_melee) || (enabled && g_pLocalPlayer->weapon_mode != weapon_melee && (!crit_key || crit_key.isKeyDown()))))
        return true;
    return false;
}
// clang-format on

static int idx = 0;
static std::vector<int> crit_cmds;

void force_crit()
{
    if (!IsEnabled())
        return;
    if (!old_mode && g_pLocalPlayer->weapon_mode != weapon_melee && LOCAL_W->m_iClassID() != CL_CLASS(CTFPipebombLauncher))
    {
        if (crit_cmds.size())
        {
            if (idx > crit_cmds.size())
                idx = 0;
            // Magic crit cmds
            current_late_user_cmd->command_number = crit_cmds[idx];
            current_late_user_cmd->random_seed    = MD5_PseudoRandom(current_late_user_cmd->command_number) & 0x7FFFFFFF;
            idx++;
        }
    }
    else
    {
        int next_crit = next_crit_tick();
        if (next_crit != -1)
        {
            // We can just force to nearest crit for melee, and sticky launchers apparently
            if (g_pLocalPlayer->weapon_mode == weapon_melee || LOCAL_W->m_iClassID() == CL_CLASS(CTFPipebombLauncher))
            {
                if (LOCAL_W->m_iClassID() == CL_CLASS(CTFPipebombLauncher))
                {
                    if (!force_ticks)
                        force_ticks = 3;
                    force_ticks--;
                }
                current_late_user_cmd->command_number = next_crit;
                current_late_user_cmd->random_seed    = MD5_PseudoRandom(next_crit) & 0x7FFFFFFF;
            }
            // No crit, don't attack
            else if (current_late_user_cmd->command_number != next_crit)
                current_user_cmd->buttons &= ~IN_ATTACK;
        }
    }
}

void update_cmds()
{
    // Melee weapons do not matter
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
        return;
    auto weapon = RAW_ENT(LOCAL_W);
    static float last_bucket;
    static int last_weapon;

    float &bucket = re::C_TFWeaponBase::crit_bucket_(weapon);

    int lowest_value = INT_MAX;
    int cur_cmdnum   = current_late_user_cmd->command_number;
    bool old_cmds    = false;
    for (auto &cmd : crit_cmds)
        if (cmd < cur_cmdnum)
            old_cmds = true;
    if (weapon->entindex() != last_weapon || old_cmds)
    {
        // Used later
        int xor_dat = (weapon->entindex() << 8 | LOCAL_E->m_IDX);
        // Clear old data
        crit_cmds.clear();
        added_per_shot = 0.0f;

        // 30.000.000 seems like a good cap
        for (int i = cur_cmdnum, j = 5; i <= cur_cmdnum + 30000000 + 200 && j > 0; i++)
        {
            // Reverse seed
            int iSeed    = MD5_PseudoRandom(i) & 0x7fffffff;
            int tempSeed = iSeed ^ (xor_dat);
            RandomSeed(tempSeed);
            int iResult = RandomInt(0, 9999);
            // Returns something low? Store it.
            if (iResult <= lowest_value)
            {
                if (i > cur_cmdnum + 200)
                {
                    // Found lower value
                    if (iResult < lowest_value)
                    {
                        crit_cmds.clear();
                        lowest_value = iResult;
                        j            = 5;
                    }
                    crit_cmds.push_back(i);
                    if (added_per_shot == 0.0f)
                    {
                        int backup_seed = *g_PredictionRandomSeed;
                        // Set random seed
                        *g_PredictionRandomSeed = MD5_PseudoRandom(i) & 0x7FFFFFFF;
                        // Save weapon state to not break anything
                        weapon_info info(weapon);
                        // We modify and write using this
                        weapon_info write(weapon);

                        // Set Values so they don't get in the way
                        write.crit_bucket          = GetBucketCap();
                        write.crit_attempts        = 100000000;
                        write.crit_count           = 0;
                        write.observed_crit_chance = 0.0f;

                        // Write onto weapon
                        write.restore_data(weapon);

                        bool is_crit = re::C_TFWeaponBase::CalcIsAttackCritical(weapon);

                        if (is_crit)
                        {
                            weapon_info new_info(weapon);
                            // How much is Subtracted per shot
                            added_per_shot = (GetBucketCap() - new_info.crit_bucket) / 3.0f;
                        }

                        // Restore original state
                        info.restore_data(weapon);
                        // Adjust with multiplier

                        *g_PredictionRandomSeed = backup_seed;
                    }
                    j--;
                }
            }
        }
    }
    last_weapon = weapon->entindex();
    last_bucket = bucket;
}

// Fix bucket on non-local servers
void unfuck_bucket(IClientEntity *weapon)
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (!ch)
        return;
    auto addr = ch->GetRemoteAddress();
    // Local server needs no fixing
    if (addr.type == NA_LOOPBACK)
        return;
    // Melee doesn't either
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
        return;

    static float last_bucket;
    static int last_weapon;
    static unsigned last_tick;
    static float last_fire_time;

    weapon_info info(weapon);
    float bucket    = info.crit_bucket;
    float fire_time = NET_FLOAT(weapon, netvar.flLastFireTime);
    if (weapon->entindex() == last_weapon)
    {
        if (bucket != last_bucket && last_bucket != 0)
        {
            if (last_tick == tickcount)
            {
                bucket           = last_bucket;
                info.crit_bucket = bucket;
                info.restore_data(weapon);
                return;
            }

            // We want to adjust it ourselves
            if (bucket > last_bucket)
                bucket = last_bucket;
        }

        // We shot, add to bucket
        if (last_fire_time != fire_time)
        {
            bucket += added_per_shot;
            // Don't overshoot
            if (bucket > GetBucketCap())
                bucket = GetBucketCap();
        }
    }

    last_fire_time = fire_time;
    last_tick      = tickcount;
    last_weapon    = weapon->entindex();
    last_bucket    = bucket;

    info.crit_bucket = bucket;
    info.restore_data(weapon);
}

void CreateMove()
{
    if (!enabled && !melee && !draw)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

    unfuck_bucket(RAW_ENT(LOCAL_W));
    // Update cmds
    update_cmds();
    if (!enabled && !melee)
        return;
    if (!force_ticks && (!(melee && g_pLocalPlayer->weapon_mode == weapon_melee) && crit_key && !crit_key.isKeyDown()))
        return;
    if (!current_late_user_cmd->command_number)
        return;

    IClientEntity *weapon = RAW_ENT(LOCAL_W);
    if (!re::C_TFWeaponBase::IsBaseCombatWeapon(weapon))
        return;
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon))
        return;
    if (!re::C_TFWeaponBase::CanFireCriticalShot(weapon, false, nullptr))
        return;

    if (force_ticks || (CanShoot() && current_late_user_cmd->buttons & IN_ATTACK))
    {
        // Are we even allowed to crit?
        if (CanWeaponCrit(RAW_ENT(LOCAL_W)))
            force_crit();
    }
}

// Storage
static int last_crit_tick   = -1;
static int last_bucket      = 0;
static int shots_until_crit = 0;
static int last_wep         = 0;
static std::pair<float, float> crit_mult_info;
void Draw()
{
    if (!draw)
        return;
    if (!g_IEngine->GetNetChannelInfo())
        last_crit_tick = -1;
    if (CE_GOOD(LOCAL_E) && CE_GOOD(LOCAL_W))
    {
        auto wep     = RAW_ENT(LOCAL_W);
        float bucket = weapon_info(wep).crit_bucket;

        // Reset because too old
        if (wep->entindex() != last_wep || last_crit_tick - current_late_user_cmd->command_number < 0 || (last_crit_tick - current_late_user_cmd->command_number) * g_GlobalVars->interval_per_tick > 30)
            last_crit_tick = next_crit_tick();

        // Used by multiple things
        bool can_crit = CanWeaponCrit(wep);

        if (bucket != last_bucket || wep->entindex() != last_wep)
        {
            // Recalculate shots until crit
            if (!can_crit)
                shots_until_crit = ShotsUntilCrit(wep);
            // Recalc crit multiplier info
            crit_mult_info = CritMultInfo(wep);
        }

        // Display for when crithack is active/can't be active
        if (IsEnabled() && last_crit_tick != -1)
            AddCenterString("Forcing Crits!", colors::red);

        // Mark if the weapon can randomly crit or not
        if (!re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr))
        {
            AddCenterString("Weapon cannot randomly crit.", colors::red);
            return;
        }

        if (!can_crit)
            AddCenterString("Shots until crit: " + std::to_string(shots_until_crit), colors::orange);

        // Mark bucket as ready/not ready
        auto color = colors::red;
        if (can_crit)
            color = colors::green;
        AddCenterString("Crit Bucket: " + std::to_string(bucket), color);

        // Print crit Multiplier information

        // Crit chance is low enough
        if (crit_mult_info.first < crit_mult_info.second)
            color = colors::green;
        // Cannot crit
        else
            color = colors::red;
        AddCenterString("Crit Penalty: " + std::to_string(crit_mult_info.first), color);

        // Time until crit
        if (old_mode && can_crit && last_crit_tick != -1)
        {
            // Ticks / Ticks per second
            float time = (last_crit_tick - current_late_user_cmd->command_number) * g_GlobalVars->interval_per_tick;
            AddCenterString("Crit in " + std::to_string(time) + "s", colors::orange);
        }

        // Update
        last_bucket = bucket;
        last_wep    = wep->entindex();
    }
}

void LevelShutdown()
{
    last_crit_tick = -1;
}

static InitRoutine init([]() {
    EC::Register(EC::CreateMoveLate, CreateMove, "crit_cm");
    EC::Register(EC::Draw, Draw, "crit_draw");
    EC::Register(EC::LevelShutdown, LevelShutdown, "crit_lvlshutdown");
});
} // namespace criticals
