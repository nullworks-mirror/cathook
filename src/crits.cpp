#include "common.hpp"
#include "crits.hpp"
#include "netadr.h"

std::unordered_map<int, int> command_number_mod{};

namespace criticals
{
settings::Boolean enabled{ "crit.enabled", "false" };
settings::Boolean melee{ "crit.melee", "false" };
static settings::Button crit_key{ "crit.key", "<null>" };
static settings::Boolean force_no_crit{ "crit.anti-crit", "true" };
settings::Boolean old_mode{ "crit.old-mode", "false" };

static settings::Boolean draw{ "crit.draw-info", "false" };
static settings::Boolean draw_meter{ "crit.draw-meter", "false" };
// Draw control
static settings::Int draw_string_x{ "crit.draw-info.x", "8" };
static settings::Int draw_string_y{ "crit.draw-info.y", "800" };
static settings::Int size{ "crit.bar-size", "100" };
static settings::Int bar_x{ "crit.bar-x", "50" };
static settings::Int bar_y{ "crit.bar-y", "500" };

// How much is added to bucket per shot?
static float added_per_shot = 0.0f;
// Needed to calculate observed crit chance properly
static int cached_damage    = 0;
static int crit_damage      = 0;
static int shot_weapon_mode = 0;

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
        flMultiply = RemapValClamped(((float) call_count / (float) crit_checks), 0.1f, 1.f, 1.f, 3.f);
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
bool isEnabled()
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
// Should we Prevent crits?
bool preventCrits()
{
    // Can't randomly crit
    if (!randomCritEnabled())
        return false;
    if (g_pLocalPlayer->weapon_mode != weapon_melee && (force_no_crit && crit_key && !crit_key.isKeyDown()))
        return true;
    return false;
}

// We cycle between the crit cmds so we want to store where we are currently at
std::vector<int> crit_cmds;

// We need to store a bunch of data for when we kill someone with a crit
struct player_status
{
    int health{};
    bool was_jarated{};
    bool was_markedfordeath{};
};
int current_index = 0;
static std::array<player_status, 33> player_status_list{};

// Main function that forces a crit
void force_crit()
{
    // Crithack should not run
    if (!isEnabled() || preventCrits())
        return;

    // New mode stuff (well when not using melee nor using pipe launcher)
    if (!old_mode && g_pLocalPlayer->weapon_mode != weapon_melee && LOCAL_W->m_iClassID() != CL_CLASS(CTFPipebombLauncher))
    {
        // Force to not crit
        if (crit_key && !crit_key.isKeyDown())
        {
            // Ignore if we are crit boosted
            if (!re::CTFPlayerShared::IsCritBoosted(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E))))
            {
                // Try only 5 times, give up after
                int tries = 0;
                while (nextCritTick() == current_late_user_cmd->command_number && tries < 5)
                {
                    current_late_user_cmd->command_number++;
                    current_late_user_cmd->random_seed = MD5_PseudoRandom(current_late_user_cmd->command_number) & 0x7FFFFFFF;
                    tries++;
                }
            }
        }
        // We have valid crit command numbers
        else if (crit_cmds.size())
        {
            if (current_index >= crit_cmds.size())
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
                current_late_user_cmd->buttons &= ~IN_ATTACK;
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

static std::vector<float> crit_mult_storage;
static int last_crits;

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

    // Needs to be updated in melee too
    int current_crits = CE_INT(LOCAL_E, netvar.m_iCrits);
    // Melee doesn't Need fixing either, still needs crits though
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
    {
        if (!CanShoot() || g_pLocalPlayer->weapon_melee_damage_tick)
        {
            shot_weapon_mode = weapon_melee;
            last_crits       = current_crits;
        }
        return;
    }

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
        bool bucket_decreased = false;
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

            // We want to prevent any increase in bucket
            if (bucket > last_bucket)
                bucket = last_bucket;

            // First crit does not count, not sure why
            else
            {
                if (current_crits != 0)
                    bucket = last_bucket;
                else
                    bucket_decreased = true;
            }
        }

        // We shot, add to bucket
        if (last_fire_time < fire_time && !bucket_decreased)
        {
            bucket += added_per_shot;

            // Update the weapon we shot with, important for projectile crits later
            shot_weapon_mode = g_pLocalPlayer->weapon_mode;
            // Don't overshoot
            if (bucket > getBucketCap())
                bucket = getBucketCap();
        }
        // We shot a crit, remove from bucket, also try to avoid the crit boosted ones, + don't take crits from other weapons
        if (g_pLocalPlayer->weapon_mode == shot_weapon_mode && last_crits < current_crits && !re::CTFPlayerShared::IsCritBoosted(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E))))
        {
            if (!crit_mult_storage.size())
                crit_mult_storage.push_back(getWithdrawMult(weapon));
            bucket -= added_per_shot * crit_mult_storage[0];
            // Do not overshoot
            if (bucket < 0.0f)
                bucket = 0.0f;
        }
        // Store last two crit mults
        if (std::find(crit_mult_storage.begin(), crit_mult_storage.end(), getWithdrawMult(weapon)) == crit_mult_storage.end())
        {
            crit_mult_storage.push_back(getWithdrawMult(weapon));
            if (crit_mult_storage.size() > 2)
                crit_mult_storage.erase(crit_mult_storage.begin());
        }
    }
    else
        crit_mult_storage.clear();

    last_fire_time = fire_time;
    last_crits     = current_crits;
    last_tick      = tickcount;
    last_weapon    = weapon->entindex();
    last_bucket    = bucket;

    info.crit_bucket = bucket;
    info.restore_data(weapon);
}

// Beggars
static bool should_crit_beggars = false;
static bool attacked_last_tick  = false;
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

    if (!enabled && !melee && !draw && !draw_meter)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

    // Fix our bucket
    fixBucket(RAW_ENT(LOCAL_W));

    // Update magic crit commands
    updateCmds();
    if (!enabled && !melee)
        return;
    if (!force_ticks && (!(melee && g_pLocalPlayer->weapon_mode == weapon_melee) && !force_no_crit && crit_key && !crit_key.isKeyDown()))
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

    // Beggars check
    if (CE_INT(LOCAL_W, netvar.iItemDefinitionIndex) == 730)
    {
        // Check if we released the barrage by releasing m1, also lock bool so people don't just release m1 and tap it again
        if (!should_crit_beggars)
            should_crit_beggars = !(current_late_user_cmd->buttons & IN_ATTACK) && attacked_last_tick;
        // Update
        attacked_last_tick = current_user_cmd->buttons & IN_ATTACK;
        if (!CE_INT(LOCAL_W, netvar.m_iClip1))
        {
            // Reset
            should_crit_beggars = false;
        }
    }
    // Should we force crits?
    if (force_ticks || should_crit_beggars || (CanShoot() && current_late_user_cmd->buttons & IN_ATTACK))
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

#if ENABLE_VISUALS

// Need our own Text drawing
static std::array<std::string, 32> crit_strings;
static size_t crit_strings_count{ 0 };
static std::array<rgba_t, 32> crit_strings_colors{ colors::empty };

static std::string bar_string = "";

void AddCritString(const std::string &string, const rgba_t &color)
{
    crit_strings[crit_strings_count]        = string;
    crit_strings_colors[crit_strings_count] = color;
    ++crit_strings_count;
}

void DrawCritStrings()
{
    // Positions
    float x = *bar_x + *size * 2.0f;
    float y = *bar_y + *size / 5.0f;

    if (bar_string != "")
    {
        float sx, sy;
        fonts::menu->stringSize(bar_string, &sx, &sy);
        // Center and draw below
        draw::String((x - sx) / 2, (y + sy), colors::red, bar_string.c_str(), *fonts::center_screen);
        y += fonts::center_screen->size + 1;
    }

    x = *draw_string_x;
    y = *draw_string_y;
    for (size_t i = 0; i < crit_strings_count; ++i)
    {
        float sx, sy;
        fonts::menu->stringSize(crit_strings[i], &sx, &sy);
        draw::String(x, y, crit_strings_colors[i], crit_strings[i].c_str(), *fonts::center_screen);
        y += fonts::center_screen->size + 1;
    }
    crit_strings_count = 0;
    bar_string         = "";
}

static Timer update_shots{};

void Draw()
{
    if (!draw && !draw_meter)
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

        if (bucket != last_bucket || wep->entindex() != last_wep || update_shots.test_and_set(500))
        {
            // Recalculate shots until crit
            if (!can_crit)
                shots_until_crit = shotsUntilCrit(wep);
        }
        // Get Crit multiplier info
        std::pair<float, float> crit_mult_info = critMultInfo(wep);

        // Draw Text
        if (draw)
        {
            // Display for when crithack is active
            if (isEnabled() && last_crit_tick != -1)
                AddCritString("Forcing Crits!", colors::red);

            // Weapon can't randomly crit
            if (!re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr))
            {
                AddCritString("Weapon cannot randomly crit.", colors::red);
                DrawCritStrings();
                return;
            }

            // Observed crit chance is not low enough, display how much damage is needed until we can crit again
            if (crit_mult_info.first > crit_mult_info.second)
                AddCritString("Damage Until crit: " + std::to_string(damageUntilToCrit(wep)), colors::orange);
            else if (!can_crit)
                AddCritString("Shots until crit: " + std::to_string(shots_until_crit), colors::orange);

            // Mark bucket as ready/not ready
            auto color = colors::red;
            if (can_crit)
                color = colors::green;
            AddCritString("Crit Bucket: " + std::to_string(bucket), color);

            // Time until crit (for old mode)
            if (old_mode && can_crit && last_crit_tick != -1)
            {
                // Ticks / Ticks per second
                float time = (last_crit_tick - current_late_user_cmd->command_number) * g_GlobalVars->interval_per_tick;
                AddCritString("Crit in " + std::to_string(time) + "s", colors::orange);
            }
        }

        // Draw Bar
        if (draw_meter)
        {
            // Can crit?
            if (re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr))
            {
                rgba_t bucket_color = colors::FromRGBA8(0x53, 0xbc, 0x31, 255);
                // Forcing crits, change crit bucket color to a nice azure blue
                if (isEnabled())
                    bucket_color = colors::FromRGBA8(0x34, 0xeb, 0xae, 255);
                // Color everything red
                if (crit_mult_info.first > crit_mult_info.second || !can_crit)
                    bucket_color = colors::red;

                // Get the percentage the bucket will take up
                float bucket_percentage = bucket / getBucketCap();

                // Get the percentage of the bucket after a crit
                float bucket_percentage_post_crit = bucket;

                // First run the add and subtract calculations
                bucket_percentage_post_crit += added_per_shot;
                if (bucket_percentage_post_crit > getBucketCap())
                    bucket_percentage_post_crit = getBucketCap();
                bucket_percentage_post_crit -= added_per_shot * getWithdrawMult(wep);
                if (bucket_percentage_post_crit < 0.0f)
                    bucket_percentage_post_crit = 0.0f;

                // Now convert to percentage
                bucket_percentage_post_crit /= getBucketCap();

                // Colors for all the different cases
                static rgba_t reduction_color_base = colors::FromRGBA8(0xe4, 0x63, 0x35, 255);
                rgba_t reduction_color             = colors::Fade(reduction_color_base, colors::white, g_GlobalVars->curtime, 2.0f);

                // Time to draw

                // Draw background
                static rgba_t background_color = colors::FromRGBA8(96, 96, 96, 150);
                float bar_bg_x_size            = *size * 2.0f;
                float bar_bg_y_size            = *size / 5.0f;
                draw::Rectangle(*bar_x - 5.0f, *bar_y - 5.0f, bar_bg_x_size + 10.0f, bar_bg_y_size + 10.0f, background_color);

                // Need special draw logic here
                if ((crit_mult_info.first > crit_mult_info.second || !can_crit) && !(g_pLocalPlayer->weapon_mode == weapon_melee))
                {
                    draw::Rectangle(*bar_x, *bar_y, bar_bg_x_size * bucket_percentage, bar_bg_y_size, bucket_color);

                    if (crit_mult_info.first > crit_mult_info.second)
                        bar_string = std::to_string(damageUntilToCrit(wep)) + " Damage until Crit!";
                    else
                    {
                        if (shots_until_crit != 1)
                            bar_string = std::to_string(shots_until_crit) + " Shots until Crit!";
                        else
                            bar_string = std::to_string(shots_until_crit) + " Shot until Crit!";
                    }
                }
                else
                {

                    // Calculate how big the green part needs to be
                    float bucket_draw_percentage = bucket_percentage_post_crit;

                    // Are we subtracting more than we have in the buffer?
                    float x_offset_bucket = bucket_draw_percentage * bar_bg_x_size;
                    if (x_offset_bucket > 0.0f)
                        draw::Rectangle(*bar_x, *bar_y, x_offset_bucket, bar_bg_y_size, bucket_color);
                    else
                        x_offset_bucket = 0.0f;

                    // Calculate how big the Reduction part needs to be
                    float reduction_draw_percentage = bucket_percentage - bucket_percentage_post_crit;
                    if (bucket_draw_percentage < 0.0f)
                        reduction_draw_percentage += bucket_draw_percentage;
                    draw::Rectangle(*bar_x + x_offset_bucket, *bar_y, bar_bg_x_size * reduction_draw_percentage, bar_bg_y_size, reduction_color);
                }
            }
        }

        // Update
        last_bucket = bucket;
        last_wep    = wep->entindex();
        DrawCritStrings();
    }
}
#endif

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
                    // This is only ranged damage
                    if (shot_weapon_mode != weapon_melee)
                    {
                        // General damage counter
                        int damage = event->GetInt("damageamount");
                        if (damage > player_status_list[victim].health)
                            damage = player_status_list[victim].health;

                        // Crit handling
                        if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !re::CTFPlayerShared::IsCritBoosted(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E))))
                        {
                            // Crit damage counter
                            if (event->GetBool("crit"))
                                crit_damage += damage;

                            // Mini crit case
                            if (event->GetBool("minicrit"))
                            {
                                if (!player_status_list[victim].was_jarated && !player_status_list[victim].was_markedfordeath)
                                    crit_damage += damage;
                            }
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
#if ENABLE_VISUALS
    EC::Register(EC::Draw, Draw, "crit_draw");
#endif
    EC::Register(EC::LevelShutdown, LevelShutdown, "crit_lvlshutdown");
    g_IGameEventManager->AddListener(&listener, false);
    EC::Register(
        EC::Shutdown, []() { g_IGameEventManager->RemoveListener(&listener); }, "crit_shutdown");
});
} // namespace criticals
