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
static float added_per_shot = 0.0f;
// Needed to calculate observed crit chance properly
static int cached_damage    = 0;
static int crit_damage      = 0;
static int shot_weapon_mode = 0;

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

static float getWithdrawMult(IClientEntity *wep)
{
    weapon_info info(wep);
    // Increment call count
    int call_count = info.crit_count + 1;
    // How many times there was a check for crits
    int crit_checks  = info.crit_attempts + 1;
    float flMultiply = 0.5;

    if (g_pLocalPlayer->weapon_mode != weapon_melee)
    {

        float flTmp = std::clamp((((float) call_count / (float) crit_checks) - 0.1f) * 1.111f, 0.0f, 1.0f);
        flMultiply  = (flTmp * 2.0) + 1.0;
    }

    float flToRemove = flMultiply * 3.0;
    return flToRemove;
}

static float getBucketCap()
{
    static ConVar *tf_weapon_criticals_bucket_cap = g_ICvar->FindVar("tf_weapon_criticals_bucket_cap");
    return tf_weapon_criticals_bucket_cap->GetFloat();
}

// This simply checks if we can withdraw the specified damage from the bucket or not.
// add_damage parameter simply specifies if we also kind of simulate the damage that gets added before
// the function gets ran in the game.
static bool isAllowedToWithdrawFromBucket(IClientEntity *wep, float flDamage, bool add_damage = true)
{
    weapon_info info(wep);
    if (add_damage)
    {
        info.crit_bucket += flDamage;
        if (info.crit_bucket > getBucketCap())
            info.crit_bucket = getBucketCap();
    }
    float flToRemove = getWithdrawMult(wep) * flDamage;
    // Can remove
    if (flToRemove <= info.crit_bucket)
        return true;

    return false;
}

// This simulates a shot in all the important aspects, like increating crit attempts, bucket, etc
static void simulateNormalShot(IClientEntity *wep, float flDamage)
{
    weapon_info info(wep);
    info.crit_bucket += flDamage;
    if (info.crit_bucket > getBucketCap())
        info.crit_bucket = getBucketCap();
    // Write other values important for iteration
    info.crit_attempts++;
    info.restore_data(wep);
}

// This is just a convenient wrapper which will most likely be inlined
static bool canWeaponWithdraw(IClientEntity *wep)
{
    // Check
    if (!isAllowedToWithdrawFromBucket(wep, added_per_shot))
        return false;
    return true;
}

// Calculate shots until crit
static int shotsUntilCrit(IClientEntity *wep)
{
    weapon_info info(wep);
    // How many shots until we can crit
    int shots = 0;
    // Predicting 100 shots should be fine
    for (shots = 0; shots < 100; shots++)
    {
        if (isAllowedToWithdrawFromBucket(wep, added_per_shot, true))
            break;
        // Do calculations
        simulateNormalShot(wep, added_per_shot);
    }
    // Restore variables
    info.restore_data(wep);
    return shots;
}

// Calculate a weapon and player specific variable that determines how
// High your observed crit chance is allowed to be
// (this + 0.1f >= observed_chance)
static float getCritCap(IClientEntity *wep)
{
    typedef float (*AttribHookFloat_t)(float, const char *, IClientEntity *, void *, bool);

    // Need this to get crit Multiplier from weapon
    static uintptr_t AttribHookFloat = gSignatures.GetClientSignature("55 89 E5 57 56 53 83 EC 6C C7 45 ? 00 00 00 00 A1 ? ? ? ? C7 45 ? 00 00 00 00 8B 75 ? 85 C0 0F 84 ? ? ? ? 8D 55 ? 89 04 24 31 DB 89 54 24");
    static auto AttribHookFloat_fn   = AttribHookFloat_t(AttribHookFloat);

    // Player specific Multiplier
    float crit_mult = re::CTFPlayerShared::GetCritMult(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E)));

    // Weapon specific Multiplier
    float flMultCritChance = AttribHookFloat_fn(crit_mult * 0.02, "mult_crit_chance", wep, 0, 1);

    return flMultCritChance;
}

// Server gives us garbage so let's just calc our own
static float getObservedCritChane()
{
    if (!cached_damage)
        return 0.0f;
    // Same is used by server
    float normalized_damage = (float) crit_damage / 3.0f;
    return normalized_damage / (normalized_damage + (float) (cached_damage - crit_damage));
}

// This returns two floats, first one being our current crit chance, second is what we need to be at/be lower than
static std::pair<float, float> critMultInfo(IClientEntity *wep)
{
    float cur_crit        = getCritCap(wep);
    float observed_chance = CE_FLOAT(LOCAL_W, netvar.flObservedCritChance);
    float needed_chance   = cur_crit + 0.1f;
    return std::pair<float, float>(observed_chance, needed_chance);
}

// How much damage we need until we can crit
static int damageUntilToCrit(IClientEntity *wep)
{
    // First check if we even need to deal damage at all
    auto crit_info = critMultInfo(wep);
    if (crit_info.first <= crit_info.second || g_pLocalPlayer->weapon_mode == weapon_melee)
        return 0;

    float target_chance = critMultInfo(wep).second;
    // Formula taken from TotallyNotElite
    int damage = std::ceil(crit_damage * (2.0f * target_chance + 1.0f) / (3.0f * target_chance));
    return damage - cached_damage;
}

// Calculate next tick we can crit at
static int nextCritTick()
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
    // Reset
    *g_PredictionRandomSeed = old_seed;
    return -1;
}

static bool randomCritEnabled()
{
    static ConVar *tf_weapon_criticals = g_ICvar->FindVar("tf_weapon_criticals");
    return tf_weapon_criticals->GetBool();
}

// These are used when we want to force a crit regardless of states (e.g. for delayed weapons like sticky launchers)
static int force_ticks = 0;

// Is the hack enabled?
static bool isEnabled()
{
    // No crits without random crits
    if (!randomCritEnabled())
        return false;
    // Check if
    // - forced crits
    // - melee enabled and holding melee
    // - master switch enabled and not using a melee + (button check)
    if (force_ticks || ((melee && g_pLocalPlayer->weapon_mode == weapon_melee) || (enabled && g_pLocalPlayer->weapon_mode != weapon_melee && (!crit_key || crit_key.isKeyDown()))))
        return true;
    return false;
}

// We cycle between the crit cmds so we want to store where we are currently at
static std::vector<int> crit_cmds;

// We need to store a bunch of data for when we kill someone with a crit
struct player_status
{
    int health{};
    bool was_jarated{};
    bool was_markedfordeath{};
};
static std::array<player_status, 33> player_status_list{};

// Main function that forces a crit
void force_crit()
{
    // Crithack should not run
    if (!isEnabled())
        return;

    // New mode stuff (well when not using melee nor using pipe launcher)
    if (!old_mode && g_pLocalPlayer->weapon_mode != weapon_melee && LOCAL_W->m_iClassID() != CL_CLASS(CTFPipebombLauncher))
    {
        // We have valid crit command numbers
        if (crit_cmds.size())
        {
            static int current_index = 0;
            if (current_index > crit_cmds.size())
                current_index = 0;

            // Magic crit cmds get used to force a crit
            current_late_user_cmd->command_number = crit_cmds[current_index];
            current_late_user_cmd->random_seed    = MD5_PseudoRandom(current_late_user_cmd->command_number) & 0x7FFFFFFF;
            current_index++;
        }
    }
    // Old mode stuff (and melee/sticky launcher)
    else
    {
        // get the next tick we can crit at
        int next_crit = nextCritTick();
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
            // For everything else, wait for the crit cmd
            else if (current_late_user_cmd->command_number != next_crit)
                current_user_cmd->buttons &= ~IN_ATTACK;
        }
    }
}

// Update the magic crit commands numbers
// Basically, we can send any command number to the server, and as long as its in the future it will get
// used for crit calculations.
// Since we know how the random seed is made, and how the crits are predicted,
// We simply replicate the same behaviour and find out, "Which command numbers would be the best?"
// The answer is, ones that make RandomInt return 0, or something very low, as that will have the highest
// Chance of working.

static void updateCmds()
{
    // Melee weapons don't use this, return
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
        return;

    auto weapon = RAW_ENT(LOCAL_W);
    static int last_weapon;

    // Current command number
    int cur_cmdnum = current_late_user_cmd->command_number;

    // Are the cmds too old?
    bool old_cmds = false;
    for (auto &cmd : crit_cmds)
        if (cmd < cur_cmdnum)
            old_cmds = true;

    // Did we switch weapons or the commands are too old?
    if (weapon->entindex() != last_weapon || old_cmds)
    {
        // Used later for the seed
        int xor_dat = (weapon->entindex() << 8 | LOCAL_E->m_IDX);

        // Clear old data
        crit_cmds.clear();
        added_per_shot = 0.0f;

        // 100000 should be fine performance wise, as they are very spread out
        // j indicates the amount to store at max
        for (int i = cur_cmdnum + 200, j = 3; i <= cur_cmdnum + 100000 + 200 && j > 0; i++)
        {
            // Manually make seed
            int iSeed = MD5_PseudoRandom(i) & 0x7fffffff;
            // Replicate XOR behaviour used by game
            int tempSeed = iSeed ^ (xor_dat);
            RandomSeed(tempSeed);

            // The result of the crithack, 0 == best chance
            int iResult = RandomInt(0, 9999);

            // If it's 0, store it. Also do not take too close cmds as they will
            // have to be replaced very soon anyways.
            if (iResult == 0 && i > cur_cmdnum + 200)
            {
                // Add to magic crit array
                crit_cmds.push_back(i);

                // We haven't calculated the amount added to the bucket yet
                if (added_per_shot == 0.0f)
                {
                    int backup_seed = *g_PredictionRandomSeed;
                    // Set random seed ( client only )
                    *g_PredictionRandomSeed = MD5_PseudoRandom(i) & 0x7FFFFFFF;

                    // Save weapon state to not break anything
                    weapon_info info(weapon);

                    // We modify and write using this
                    weapon_info write(weapon);

                    // Set Values so they don't get in the way and always subtract damage * 3.0f
                    write.crit_bucket          = getBucketCap();
                    write.crit_attempts        = 100000000;
                    write.crit_count           = 0;
                    write.observed_crit_chance = 0.0f;

                    // Write onto weapon
                    write.restore_data(weapon);

                    // Is it a crit?
                    bool is_crit = re::C_TFWeaponBase::CalcIsAttackCritical(weapon);

                    // If so, store it
                    if (is_crit)
                    {
                        weapon_info new_info(weapon);

                        // (old - new) * damage_multiplier = damage
                        added_per_shot = (getBucketCap() - new_info.crit_bucket) / 3.0f;
                    }

                    // Restore original state
                    info.restore_data(weapon);

                    // Reset seed
                    *g_PredictionRandomSeed = backup_seed;
                }
                // We found a cmd, store it
                j--;
            }
        }
    }
    last_weapon = weapon->entindex();
}

// Fix observed crit chance
static void fixObservedCritchance(IClientEntity *weapon)
{
    weapon_info info(weapon);
    info.observed_crit_chance = getObservedCritChane();
    info.restore_data(weapon);
}

// Fix bucket on non-local servers
static void fixBucket(IClientEntity *weapon)
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
    // We also need to fix the observed crit chance in here
    fixObservedCritchance(weapon);

    float bucket    = info.crit_bucket;
    float fire_time = NET_FLOAT(weapon, netvar.flLastFireTime);

    // Same weapon as last time
    if (weapon->entindex() == last_weapon)
    {
        // Amount in bucket changed
        if (bucket != last_bucket && last_bucket != 0)
        {
            // Two changes in one tick does not work, grab last bucket and return
            if (last_tick == tickcount)
            {
                bucket           = last_bucket;
                info.crit_bucket = bucket;
                info.restore_data(weapon);
                return;
            }

            // If increased, reset it so we can modify it ourselves
            if (bucket > last_bucket)
                bucket = last_bucket;
        }

        // We shot, add to bucket
        if (last_fire_time < fire_time)
        {
            bucket += added_per_shot;

            // Update the weapon we shot with, important for projectile crits later
            shot_weapon_mode = g_pLocalPlayer->weapon_mode;
            // Don't overshoot
            if (bucket > getBucketCap())
                bucket = getBucketCap();
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
    // We need to update player states regardless, else we can't sync the observed crit chance
    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_VALID(ent) && g_pPlayerResource->GetHealth(ent))
        {
            auto &status              = player_status_list[i];
            status.health             = g_pPlayerResource->GetHealth(ent);
            status.was_jarated        = HasCondition<TFCond_Jarated>(ent);
            status.was_markedfordeath = HasCondition<TFCond_MarkedForDeath>(ent) || HasCondition<TFCond_MarkedForDeathSilent>(ent);
        }
    }

    if (!enabled && !melee && !draw)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

    // Fix our bucket
    fixBucket(RAW_ENT(LOCAL_W));

    // Update magic crit commands
    updateCmds();
    if (!enabled && !melee)
        return;
    if (!force_ticks && (!(melee && g_pLocalPlayer->weapon_mode == weapon_melee) && crit_key && !crit_key.isKeyDown()))
        return;
    if (!current_late_user_cmd->command_number)
        return;

    // Is weapon elligible for crits?
    IClientEntity *weapon = RAW_ENT(LOCAL_W);
    if (!re::C_TFWeaponBase::IsBaseCombatWeapon(weapon))
        return;
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon))
        return;
    if (!re::C_TFWeaponBase::CanFireCriticalShot(weapon, false, nullptr))
        return;

    // Should we force crits?
    if (force_ticks || (CanShoot() && current_late_user_cmd->buttons & IN_ATTACK))
    {
        // Can we crit?
        if (canWeaponWithdraw(RAW_ENT(LOCAL_W)))
            force_crit();
    }
}

// Storage
static int last_crit_tick   = -1;
static int last_bucket      = 0;
static int shots_until_crit = 0;
static int last_wep         = 0;

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

        // Fix observed crit chance
        fixObservedCritchance(wep);

        // Reset because too old
        if (wep->entindex() != last_wep || last_crit_tick - current_late_user_cmd->command_number < 0 || (last_crit_tick - current_late_user_cmd->command_number) * g_GlobalVars->interval_per_tick > 30)
            last_crit_tick = nextCritTick();

        // Used by multiple things
        bool can_crit = canWeaponWithdraw(wep);

        if (bucket != last_bucket || wep->entindex() != last_wep)
        {
            // Recalculate shots until crit
            if (!can_crit)
                shots_until_crit = shotsUntilCrit(wep);
        }
        // Get Crit multiplier info
        std::pair<float, float> crit_mult_info = critMultInfo(wep);

        // Display for when crithack is active
        if (isEnabled() && last_crit_tick != -1)
            AddCenterString("Forcing Crits!", colors::red);

        // Weapon can't randomly crit
        if (!re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr))
        {
            AddCenterString("Weapon cannot randomly crit.", colors::red);
            return;
        }

        // Observed crit chance is not low enough, display how much damage is needed until we can crit again
        if (crit_mult_info.first > crit_mult_info.second)
            AddCenterString("Damage Until crit: " + std::to_string(damageUntilToCrit(wep)), colors::orange);
        else if (!can_crit)
            AddCenterString("Shots until crit: " + std::to_string(shots_until_crit), colors::orange);

        // Mark bucket as ready/not ready
        auto color = colors::red;
        if (can_crit)
            color = colors::green;
        AddCenterString("Crit Bucket: " + std::to_string(bucket), color);

        // Time until crit (for old mode)
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

// Reset everything
void LevelShutdown()
{
    last_crit_tick = -1;
    cached_damage  = 0;
    crit_damage    = 0;
}

// Listener for damage
class CritEventListener : public IGameEventListener
{
public:
    void FireGameEvent(KeyValues *event) override
    {
        // Reset cached Damage, round reset
        if (!strcmp(event->GetName(), "teamplay_round_start"))
        {
            crit_damage   = 0;
            cached_damage = g_pPlayerResource->GetDamage(g_pLocalPlayer->entity_idx);
        }
        // Something took damage
        else if (!strcmp(event->GetName(), "player_hurt"))
        {
            // That something was hurt by us
            if (g_IEngine->GetPlayerForUserID(event->GetInt("attacker")) == g_pLocalPlayer->entity_idx)
            {
                // Don't count self damage
                int victim = g_IEngine->GetPlayerForUserID(event->GetInt("userid"));
                if (victim != g_pLocalPlayer->entity_idx)
                {
                    // This is only ranged damage and crit boost does not count towards it either
                    if (shot_weapon_mode != weapon_melee && !re::CTFPlayerShared::IsCritBoosted(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E))))
                    {
                        // General damage counter
                        int damage = event->GetInt("damageamount");
                        if (damage > player_status_list[victim].health)
                            damage = player_status_list[victim].health;

                        // Crit damage counter
                        if (event->GetBool("crit"))
                            crit_damage += damage;

                        // Mini crit case
                        if (event->GetBool("minicrit"))
                        {
                            if (!player_status_list[victim].was_jarated && !player_status_list[victim].was_markedfordeath)
                                crit_damage += damage;
                        }
                        cached_damage += damage;
                    }
                }
            }
        }
    }
};

static CritEventListener listener{};

static InitRoutine init([]() {
    EC::Register(EC::CreateMoveLate, CreateMove, "crit_cm");
    EC::Register(EC::Draw, Draw, "crit_draw");
    EC::Register(EC::LevelShutdown, LevelShutdown, "crit_lvlshutdown");
    g_IGameEventManager->AddListener(&listener, false);
});
} // namespace criticals
