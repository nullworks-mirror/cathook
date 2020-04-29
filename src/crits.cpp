#include "common.hpp"
#include "crits.hpp"
#include "Backtrack.hpp"
#include "netadr.h"

std::unordered_map<int, int> command_number_mod{};

namespace criticals
{
settings::Boolean enabled{ "crit.enabled", "false" };
settings::Boolean melee{ "crit.melee", "false" };
static settings::Button crit_key{ "crit.key", "<null>" };
static settings::Boolean force_no_crit{ "crit.anti-crit", "true" };
settings::Boolean old_mode{ "crit.old-mode", "false" };

static settings::Boolean draw{ "crit.info", "false" };
static settings::Boolean draw_meter{ "crit.draw-meter", "false" };
// Draw control
static settings::Int draw_string_x{ "crit.draw-info.x", "8" };
static settings::Int draw_string_y{ "crit.draw-info.y", "800" };
static settings::Int size{ "crit.bar-size", "100" };
static settings::Int bar_x{ "crit.bar-x", "50" };
static settings::Int bar_y{ "crit.bar-y", "500" };

// Debug rvar
static settings::Boolean debug_desync{ "crit.desync-debug", "false" };

// How much is added to bucket per shot?
static float added_per_shot = 0.0f;
// Needed to calculate observed crit chance properly
static int cached_damage   = 0;
static int crit_damage     = 0;
static int melee_damage    = 0;
static int round_damage    = 0;
static bool is_out_of_sync = false;

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
    float chance = 0.02f;
    if (g_pLocalPlayer->weapon_mode == weapon_melee)
        chance = 0.15f;
    float flMultCritChance = AttribHookFloat_fn(crit_mult * chance, "mult_crit_chance", wep, 0, 1);

    return flMultCritChance;
}

// Server gives us garbage so let's just calc our own
static float getObservedCritChance()
{
    if (!(cached_damage - round_damage))
        return 0.0f;
    // Same is used by server
    float normalized_damage = (float) crit_damage / 3.0f;
    return normalized_damage / (normalized_damage + (float) ((cached_damage - round_damage) - crit_damage));
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
    return damage - (cached_damage - round_damage);
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
    if (g_pLocalPlayer->weapon_mode == weapon_melee || (force_no_crit && crit_key && !crit_key.isKeyDown()))
        return true;
    return false;
}

// We cycle between the crit cmds so we want to store where we are currently at
std::vector<int> crit_cmds;

// We need to store a bunch of data for when we kill someone with a crit
struct player_status
{
    int health{};
};
int current_index = 0;
static std::array<player_status, 33> player_status_list{};

// Function for preventing a crit
bool prevent_crit()
{
    int tries = 0;
    while (nextCritTick() == current_late_user_cmd->command_number && tries < 5)
    {
        current_late_user_cmd->command_number++;
        current_late_user_cmd->random_seed = MD5_PseudoRandom(current_late_user_cmd->command_number);
        tries++;
    }
    // Failed
    if (nextCritTick() == current_late_user_cmd->command_number)
        return false;
    // Suceeded
    return true;
}

// Main function that forces a crit
void force_crit()
{
    // Crithack should not run
    if (!isEnabled() && !preventCrits())
        return;

    // New mode stuff (well when not using melee nor using pipe launcher)
    if (!old_mode && g_pLocalPlayer->weapon_mode != weapon_melee && LOCAL_W->m_iClassID() != CL_CLASS(CTFPipebombLauncher))
    {
        // Force to not crit
        if (crit_key && !crit_key.isKeyDown())
        {
            // Prevent Crit
            prevent_crit();
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
                // Code for handling when to not crit with melee weapons
                else if (force_no_crit)
                {
                    if (hacks::shared::backtrack::isBacktrackEnabled)
                    {
                        int target = hacks::shared::backtrack::iBestTarget;
                        // Valid backtrack target
                        if (target > 1)
                        {
                            // Closest tick for melee
                            int besttick = hacks::shared::backtrack::BestTick;
                            // Out of range, don't crit
                            if (hacks::shared::backtrack::headPositions[target][besttick].entorigin.DistTo(LOCAL_E->m_vecOrigin()) >= re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W)) + 150.0f)
                            {
                                prevent_crit();
                                return;
                            }
                        }
                        else
                        {
                            prevent_crit();
                            return;
                        }
                    }
                    // Normal check, get closest entity and check distance
                    else
                    {
                        auto ent = getClosestEntity(LOCAL_E->m_vecOrigin());
                        if (!ent || ent->m_flDistance() >= re::C_TFWeaponBaseMelee::GetSwingRange(RAW_ENT(LOCAL_W)) + 150.0f)
                        {
                            prevent_crit();
                            return;
                        }
                    }
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
        if (g_pLocalPlayer->weapon_mode == weapon_melee)
            xor_dat = (weapon->entindex() << 16 | LOCAL_E->m_IDX << 8);

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

                        // (old - new) / damage_multiplier = damage
                        added_per_shot = (getBucketCap() - new_info.crit_bucket) / (g_pLocalPlayer->weapon_mode == weapon_melee ? 1.5f : 3.0f);
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
    info.observed_crit_chance = getObservedCritChance();
    info.restore_data(weapon);
}

static std::vector<float> crit_mult_storage;
static float last_bucket_fix                      = -1;
static float previous_server_observed_crit_chance = 0.0f;
// Fix bucket on non-local servers
void fixBucket(IClientEntity *weapon, CUserCmd *cmd)
{
    INetChannel *ch = (INetChannel *) g_IEngine->GetNetChannelInfo();
    if (!ch)
        return;
    auto addr = ch->GetRemoteAddress();
    // Local server needs no fixing
    if (addr.type == NA_LOOPBACK)
        return;

    static int last_weapon;
    // This tracks only when bucket is updated
    static int last_update_command;
    // This always tracks
    static int last_call_command;

    fixObservedCritchance(weapon);
    weapon_info info(weapon);

    float bucket = info.crit_bucket;

    // Changed bucket more than once this tick
    if (weapon->entindex() == last_weapon && bucket != last_bucket_fix && last_update_command == cmd->command_number)
        bucket = last_bucket_fix;

    last_weapon = weapon->entindex();
    // Bucket changed, update
    if (last_bucket_fix != bucket)
        last_update_command = cmd->command_number;
    last_call_command = cmd->command_number;
    last_bucket_fix   = bucket;

    info.crit_bucket = bucket;
    info.restore_data(weapon);
}

// Beggars
static bool should_crit_beggars = false;
static bool attacked_last_tick  = false;

// Damage this round
static Timer round_damage_update_timer{};
void CreateMove()
{
    // Need to wait a bit due to it being glitchy at the start of the round
    if (!round_damage && round_damage_update_timer.check(500) && !round_damage_update_timer.check(1000))
        round_damage = g_pPlayerResource->GetDamage(g_pLocalPlayer->entity_idx);

    // Base on melee damage and server networked one rather than anything else
    cached_damage = g_pPlayerResource->GetDamage(g_pLocalPlayer->entity_idx) - melee_damage;

    // We need to update player states regardless, else we can't sync the observed crit chance
    for (int i = 0; i <= g_IEngine->GetMaxClients(); i++)
    {
        CachedEntity *ent = ENTITY(i);
        if (CE_VALID(ent) && ent->m_bAlivePlayer() && g_pPlayerResource->GetHealth(ent))
        {
            auto &status = player_status_list[i];
            // Only sync if new health is bigger, We do the rest in player_hurt
            if (status.health < g_pPlayerResource->GetHealth(ent))
                status.health = g_pPlayerResource->GetHealth(ent);
        }
    }

    if (!enabled && !melee && !draw && !draw_meter)
        return;
    if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W))
        return;

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
    if (!re::C_TFWeaponBase::AreRandomCritsEnabled(weapon) || !added_per_shot)
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
    // Should we run crit logic?
    if (force_no_crit || force_ticks || should_crit_beggars || (CanShoot() && current_late_user_cmd->buttons & IN_ATTACK))
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
        // fixObservedCritchance(wep);

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
            if (!re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr) || !added_per_shot)
            {
                AddCritString("Weapon cannot randomly crit.", colors::red);
                DrawCritStrings();
                return;
            }

            // We are out of sync. RIP
            if (is_out_of_sync)
                AddCritString("Out of sync.", colors::red);
            // Observed crit chance is not low enough, display how much damage is needed until we can crit again
            else if (crit_mult_info.first > crit_mult_info.second && g_pLocalPlayer->weapon_mode != weapon_melee)
                AddCritString("Damage Until crit: " + std::to_string(damageUntilToCrit(wep)), colors::orange);
            else if (!can_crit)
                AddCritString("Shots until crit: " + std::to_string(shots_until_crit), colors::orange);

            // Mark bucket as ready/not ready
            auto color = colors::red;
            if (can_crit && (crit_mult_info.first <= crit_mult_info.second || g_pLocalPlayer->weapon_mode != weapon_melee))
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
            if (re::C_TFWeaponBase::CanFireCriticalShot(RAW_ENT(LOCAL_W), false, nullptr) && added_per_shot)
            {
                rgba_t bucket_color = colors::FromRGBA8(0x53, 0xbc, 0x31, 255);
                // Forcing crits, change crit bucket color to a nice azure blue
                if (isEnabled())
                    bucket_color = colors::FromRGBA8(0x34, 0xeb, 0xae, 255);
                // Color everything red
                if ((crit_mult_info.first > crit_mult_info.second && g_pLocalPlayer->weapon_mode != weapon_melee) || !can_crit)
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
                if (is_out_of_sync || (crit_mult_info.first > crit_mult_info.second && g_pLocalPlayer->weapon_mode != weapon_melee) || !can_crit)
                {
                    draw::Rectangle(*bar_x, *bar_y, bar_bg_x_size * bucket_percentage, bar_bg_y_size, bucket_color);

                    if (is_out_of_sync)
                        bar_string = "Out of sync.";
                    else if (crit_mult_info.first > crit_mult_info.second && g_pLocalPlayer->weapon_mode != weapon_melee)
                        bar_string = std::to_string(damageUntilToCrit(wep)) + " Damage until Crit!";
                    else
                    {
                        if (shots_until_crit != 1)
                            bar_string = std::to_string(shots_until_crit) + " Shots until Crit!";
                        else
                            bar_string = std::to_string(shots_until_crit) + " Shot until Crit!";
                    }
                }
                // Still run when out of sync
                if (!((crit_mult_info.first > crit_mult_info.second && g_pLocalPlayer->weapon_mode != weapon_melee) || !can_crit))
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
            cached_damage = 0;
            // Need to handle later due to buggy behaviour at round start
            round_damage = 0;
            round_damage_update_timer.update();
        }
        // Something took damage
        else if (!strcmp(event->GetName(), "player_hurt"))
        {
            int victim        = g_IEngine->GetPlayerForUserID(event->GetInt("userid"));
            CachedEntity *ent = ENTITY(g_IEngine->GetPlayerForUserID(victim));
            int health        = event->GetInt("health");

            // Or Basically, Actual damage dealt
            int health_difference = 0;
            if (CE_VALID(ent))
            {
                auto &status      = player_status_list[victim];
                health_difference = status.health - health;
                status.health     = health;
            }

            // That something was hurt by us
            if (g_IEngine->GetPlayerForUserID(event->GetInt("attacker")) == g_pLocalPlayer->entity_idx)
            {
                // Don't count self damage

                if (victim != g_pLocalPlayer->entity_idx)
                {
                    // The weapon we damaged with
                    int weaponid   = event->GetInt("weaponid");
                    int weapon_idx = getWeaponByID(LOCAL_E, weaponid);

                    bool isMelee = false;
                    if (IDX_GOOD(weapon_idx))
                    {
                        int slot = re::C_BaseCombatWeapon::GetSlot(g_IEntityList->GetClientEntity(weapon_idx));
                        if (slot == 2)
                            isMelee = true;
                    }

                    // Iterate all the weapons of the local palyer for weaponid

                    // General damage counter
                    int damage = event->GetInt("damageamount");
                    if (damage > health_difference && !health)
                        damage = health_difference;

                    // Not a melee weapon
                    if (!isMelee)
                    {
                        // Crit handling
                        if (CE_BAD(LOCAL_E) || CE_BAD(LOCAL_W) || !re::CTFPlayerShared::IsCritBoosted(re::CTFPlayerShared::GetPlayerShared(RAW_ENT(LOCAL_E))))
                        {
                            // Crit damage counter
                            if (event->GetBool("crit"))
                                crit_damage += damage;
                        }
                    }
                    else
                    {
                        // Melee damage
                        melee_damage += damage;
                    }
                }
            }
        }
    }
};

static CritEventListener listener{};

void observedcritchance_nethook(const CRecvProxyData *data, void *pWeapon, void *out)
{
    // Do default action by default
    auto fl_observed_crit_chance = reinterpret_cast<float *>(out);
    *fl_observed_crit_chance     = data->m_Value.m_Float;
    if (CE_BAD(LOCAL_W) || !enabled)
        return;
    if (pWeapon != LOCAL_W->InternalEntity())
        return;

    float sent_chance = data->m_Value.m_Float;
    if (sent_chance)
    {
        // Before fix
        float old_observed_chance;
        if (debug_desync)
            old_observed_chance = getObservedCritChance();
        // Sync our chance, Player ressource is guranteed to be working, melee damage not, but it's fairly reliable
        int ranged_damage = g_pPlayerResource->GetDamage(g_pLocalPlayer->entity_idx) - melee_damage;

        // We need to do some checks for our sync process
        if (ranged_damage != 0 && 2.0f * sent_chance + 1 != 0.0f)
        {
            cached_damage = ranged_damage - round_damage;
            // Powered by math
            crit_damage = (3.0f * ranged_damage * sent_chance) / (2.0f * sent_chance + 1);
        }
        // We Were out of sync with the server
        if (debug_desync && sent_chance > old_observed_chance && fabsf(sent_chance - old_observed_chance) > 0.01f)
        {
            logging::Info("Out of sync! Observed crit chance is %f, but client expected: %f, fixed to %f", data->m_Value.m_Float, old_observed_chance, getObservedCritChance());
        }
    }
}

static ProxyFnHook observed_crit_chance_hook{};

// Reset everything
void LevelShutdown()
{
    last_crit_tick  = -1;
    cached_damage   = 0;
    crit_damage     = 0;
    melee_damage    = 0;
    last_bucket_fix = -1;
    round_damage    = 0;
    is_out_of_sync  = false;
}

// Prints basically everything you need to know about crits
static CatCommand debug_print_crit_info("debug_print_crit_info", "Print a bunch of useful crit info", []() {
    if (CE_BAD(LOCAL_E))
        return;

    logging::Info("Player specific information:");
    logging::Info("Ranged Damage this round: %d", cached_damage - round_damage);
    logging::Info("Melee Damage this round: %d", melee_damage);
    logging::Info("Crit Damage this round: %d", crit_damage);
    logging::Info("Observed crit chance: %f", getObservedCritChance());
    if (CE_GOOD(LOCAL_W))
    {
        IClientEntity *wep = RAW_ENT(LOCAL_W);
        weapon_info info(wep);
        logging::Info("Weapon specific information:");
        logging::Info("Crit bucket: %f", info.crit_bucket);
        logging::Info("Needed Crit chance: %f", critMultInfo(wep).second);
        logging::Info("Added per shot: %f", added_per_shot);
        logging::Info("Subtracted per crit: %f", added_per_shot * getWithdrawMult(wep));
        logging::Info("Damage Until crit: %d", damageUntilToCrit(wep));
        logging::Info("Shots until crit: %d", shotsUntilCrit(wep));
    }
});

static InitRoutine init([]() {
    EC::Register(EC::CreateMoveLate, CreateMove, "crit_cm");
#if ENABLE_VISUALS
    EC::Register(EC::Draw, Draw, "crit_draw");
#endif
    EC::Register(EC::LevelShutdown, LevelShutdown, "crit_lvlshutdown");
    g_IGameEventManager->AddListener(&listener, false);
    HookNetvar({ "DT_TFWeaponBase", "LocalActiveTFWeaponData", "m_flObservedCritChance" }, observed_crit_chance_hook, observedcritchance_nethook);
    EC::Register(
        EC::Shutdown,
        []() {
            g_IGameEventManager->RemoveListener(&listener);
            observed_crit_chance_hook.restore();
        },
        "crit_shutdown");
    // Attached in game, out of sync
    if (g_IEngine->IsInGame())
        is_out_of_sync = true;
});
} // namespace criticals
