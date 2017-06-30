 
/*
 * HTrigger.cpp
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#include "../beforecheaders.h"
#include <memory> // std::unique_ptr
#include "../aftercheaders.h"

#include "Trigger.h"

#include "../common.h"
#include "../sdk.h"

namespace hacks { namespace shared { namespace triggerbot {

CatVar enabled(CV_SWITCH, "trigger_enabled", "0", "Enable", "Master Triggerbot switch");
	
static CatVar trigger_key(CV_KEY, "trigger_key", "0", "Triggerbot key", "Aimkey. Look at Aimkey Mode too!");
static CatEnum trigger_key_modes_enum({ "DISABLED", "AIMKEY", "REVERSE", "TOGGLE" });
static CatVar trigger_key_mode(trigger_key_modes_enum, "trigger_key_mode", "1", "Triggerbot key mode", "DISABLED: aimbot is always active\nAIMKEY: aimbot is active when key is down\nREVERSE: aimbot is disabled when key is down\nTOGGLE: pressing key toggles aimbot");
	
CatEnum hitbox_mode_enum({ "AUTO-HEAD", "AUTO-CLOSEST", "Head only" });
CatVar hitbox_mode(hitbox_mode_enum, "trigger_hitboxmode", "0", "Hitbox Mode", "Defines hitbox selection mode");

static CatVar ignore_vaccinator(CV_SWITCH, "trigger_ignore_vaccinator", "1", "Ignore Vaccinator", "Hitscan weapons won't fire if enemy is vaccinated against bullets");
static CatVar ignore_hoovy(CV_SWITCH, "trigger_ignore_hoovy", "1", "Ignore Hoovies", "Aimbot won't attack hoovies");
static CatVar ignore_cloak(CV_SWITCH, "trigger_ignore_cloak", "1", "Ignore cloaked", "Don't aim at invisible enemies");
static CatVar buildings_sentry(CV_SWITCH, "trigger_buildings_sentry", "1", "Trigger Sentry", "Should aimbot aim at sentryguns?");
static CatVar buildings_other(CV_SWITCH, "trigger_buildings_other", "1", "Trigger Other building", "Should aimbot aim at other buildings");
static CatVar stickybot(CV_SWITCH, "trigger_stickys", "0", "Trigger Sticky", "Should aimbot aim at stickys");
static CatVar teammates(CV_SWITCH, "trigger_teammates", "0", "Trigger teammates", "Aim at your own team. Useful for HL2DM");
	
static CatVar wait_for_charge(CV_SWITCH, "trigger_charge", "0", "Wait for sniper rifle charge", "Aimbot waits until it has enough charge to kill");
static CatVar zoomed_only(CV_SWITCH, "trigger_zoomed", "1", "Zoomed only", "Don't autoshoot with unzoomed rifles");
static CatVar max_range(CV_INT, "trigger_maxrange", "0", "Max distance",
		"Max range for aimbot\n"
		"900-1100 range is efficient for scout/widowmaker engineer", 4096.0f);
	

// The main "loop" of the aimbot. 
void CreateMove() {	
	
	// Check if aimbot is enabled
	if (!enabled) return;
	
	// Check if player can aim
	if (!ShouldShoot()) return;
	
	// Get and ent in front of the player
	CachedEntity* ent = FindEntInSight(EffectiveTargetingRange());
	
	// Determine whether the triggerbot should shoot, then act accordingly
	if (!IsTargetStateGood(ent)) g_pUserCmd->buttons |= IN_ATTACK;
		
	return;
}
	
// The first check to see if the player should shoot in the first place
bool ShouldShoot() {
	
	// Check for +use
	if (g_pUserCmd->buttons & IN_USE) return false;
	
	// Check if using action slot item 
	if (g_pLocalPlayer->using_action_slot_item) return false;
	
	// Check the aimkey to determine if it should shoot
	if (!UpdateAimkey()) return false;
	
	IF_GAME (IsTF2()) {
		// Check if Carrying A building
		if (CE_BYTE(g_pLocalPlayer->entity, netvar.m_bCarryingObject)) return false;
		// Check if deadringer out
		if (CE_BYTE(g_pLocalPlayer->entity, netvar.m_bFeignDeathReady)) return false;
		// If zoomed only is on, check if zoomed
		if (zoomed_only && g_pLocalPlayer->holding_sniper_rifle) {
			if (!g_pLocalPlayer->bZoomed && !(g_pUserCmd->buttons & IN_ATTACK)) return false;
		}
		// Check if player is taunting
		if (HasCondition<TFCond_Taunting>(g_pLocalPlayer->entity)) return false;
		// Check if player is cloaked
		if (IsPlayerInvisible(g_pLocalPlayer->entity)) return false;
	}
	
	IF_GAME (IsTF2()) {
		switch (GetWeaponMode()) {
		case weapon_hitscan:
			break;
		case weapon_melee:
			break;
		// Check if player is using a projectile based weapon
		case weapon_projectile:
			return false;
			break;
		// Check if player doesnt have a weapon usable by aimbot
		default:
			return false;
		};
	}
	IF_GAME (IsTF()) {
		// Check if player is zooming
		if (g_pLocalPlayer->bZoomed) {
			if (!(g_pUserCmd->buttons & (IN_ATTACK | IN_ATTACK2))) {
				if (!CanHeadshot()) return false;
			}
		}
	}
	IF_GAME (IsTF()) {
		// Check if crithack allows attacking
		if (!AllowAttacking())
			return false;
	}
	return true;
}
	
// A second check to determine whether a target is good enough to be aimed at
bool IsTargetStateGood(CachedEntity* entity) {
	
	// Check for Players
	if (entity->m_Type == ENTITY_PLAYER) {
		// Check if target is The local player
		if (entity == LOCAL_E) return false;
		// Dont aim at dead player
		if (!entity->m_bAlivePlayer) return false;
		// Dont aim at teammates
		if (!entity->m_bEnemy && !teammates) return false;

		IF_GAME (IsTF()) {
			// If settings allow waiting for charge, and current charge cant kill target, dont aim
			if (wait_for_charge && g_pLocalPlayer->holding_sniper_rifle) {
				float bdmg = CE_FLOAT(g_pLocalPlayer->weapon(), netvar.flChargedDamage);
				if (g_GlobalVars->curtime - g_pLocalPlayer->flZoomBegin <= 1.0f) bdmg = 50.0f;
				if ((bdmg * 3) < (HasDarwins(entity) ? (entity->m_iHealth * 1.15) : entity->m_iHealth)) {
					return false;
				}
			}
			// If settings allow, ignore taunting players
			if (ignore_taunting && HasCondition<TFCond_Taunting>(entity)) return false;
			// Dont target invulnerable players, ex: uber, bonk
			if (IsPlayerInvulnerable(entity)) return false;
			// If settings allow, dont target cloaked players
			if (ignore_cloak && IsPlayerInvisible(entity)) return false;
			// If settings allow, dont target vaccinated players
			if (g_pLocalPlayer->weapon_mode == weaponmode::weapon_hitscan || LOCAL_W->m_iClassID == CL_CLASS(CTFCompoundBow))
				if (ignore_vaccinator && HasCondition<TFCond_UberBulletResist>(entity)) return false;
		}
		// Dont target players marked as friendly
		if (playerlist::IsFriendly(playerlist::AccessData(entity).state)) return false;
		IF_GAME (IsTF()) {
			// If settings allow, ignore hoovys
			if (ignore_hoovy && IsHoovy(entity)) {
				return false;
			}
		}

		// Head hitbox detection
		if (HeadPreferable(entity)) {
			// Get fov values for head and spine3, which is right below the head
			Vector head_vec;
			Vector spine3_vec;
			GetHitbox(entity, hitbox_t::head, head_vec);
			GetHitbox(entity, hitbox_t::spine_3, spine3_vec);
			float fov_head = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, head_vec);
			float fov_body = GetFov(g_pLocalPlayer->v_OrigViewangles, g_pLocalPlayer->v_Eye, spine3_vec);
			
			// Compare them and if the crosshair isnt closer to the head, then return false
			if (fov_head >= fov_body) return false;
		}	
		
		// Target passed the tests so return true
		return true;
		
	// Check for buildings
	} else if (entity->m_Type == ENTITY_BUILDING) {
		// Check if building aimbot is enabled
		if ( !(buildings_other || buildings_sentry) ) return false;
		// Check if enemy building
		if (!entity->m_bEnemy) return false;
		
		// If needed, Check if building type is allowed
		if ( !(buildings_other && buildings_sentry) ) {
			// Check if target is a sentrygun
			if ( entity->m_iClassID == CL_CLASS(CObjectSentrygun) ) {
				// If sentrys are not allowed, dont target
				if (!buildings_sentry) return false;
			} else {
				// If target is not a sentry, check if other buildings are allowed
				if (!buildings_other) return false;
			}			
		}
		
		// Target passed the tests so return true
		return true;
	
	// Check for stickybombs
	} else if (entity->m_iClassID == CL_CLASS(CTFGrenadePipebombProjectile)) {
		// Check if sticky aimbot is enabled
		if (!stickybot) return false;
		
		// Check if thrower is a teammate
		if (!entity->m_bEnemy) return false;
		
		// Check if target is a pipe bomb
		if (CE_INT(entity, netvar.iPipeType) != 1) return false;
		
		// Target passed the tests so return true
		return true;
		
	} else {
		// If target is not player, building or sticky, return false
		return false;
	}
	// An impossible error so just return false
	return false;
}
		
// A function to return a potential entity in front of the player
CachedEntity* FindEntInSight(float range) {
	// We dont want to hit ourself so we set an ignore
    trace_t trace;
    trace::filter_default.SetSelf(RAW_ENT(g_pLocalPlayer->entity));
	
	// Use math to get a vector in front of the player
	float sp, sy, cp, cy;
	QAngle angle;
	Vector forward;
	g_IEngine->GetViewAngles(angle);
	sy = sinf(DEG2RAD(angle[1]));
	cy = cosf(DEG2RAD(angle[1]));
	sp = sinf(DEG2RAD(angle[0]));
	cp = cosf(DEG2RAD(angle[0]));
	forward.x = cp * cy;
	forward.y = cp * sy;
	forward.z = -sp;
	forward = forward * range + g_pLocalPlayer->v_Eye;
	
	// Setup the trace starting with the origin of the local players eyes attemting to hit the end vector we determined
    Ray_t ray;
	ray.Init(g_pLocalPlayer->v_Eye, forward);
	{
		PROF_SECTION(IEVV_TraceRay);
		g_ITrace->TraceRay(ray, MASK_SHOT_HULL, &trace::filter_default, &trace);
	}
	// Return an ent if that is what we hit
    if (trace.m_pEnt) {
        return ENTITY(((IClientEntity*)trace.m_pEnt)->entindex());
	}
	
	// Since we didnt hit and entity, the vis check failed so return 0
    return 0;
	
}
	
// A function to find whether the head should be used for a target
bool HeadPreferable(CachedEntity* target) {

	// Switch based on the priority type we need
	switch ((int)hitbox_mode) {
	case 0: { // AUTO-HEAD priority
		// Var to keep if we can bodyshot
		bool headonly = false;
		// Save the local players current weapon to a var
		int ci = g_pLocalPlayer->weapon()->m_iClassID;
		IF_GAME (IsTF()) {
			// If user is using a sniper rifle, Set headonly to whether we can headshot or not, 
			if (g_pLocalPlayer->holding_sniper_rifle) {
				headonly = CanHeadshot();
			// If player is using an ambassador, set headonly to true
			} else if (IsAmbassador(g_pLocalPlayer->weapon())) {
				headonly = true;
			}
			// Bodyshot handling
			if (g_pLocalPlayer->holding_sniper_rifle) {
				// Some keeper vars
				float cdmg, bdmg;
				// Grab netvar for current charge damage
				cdmg = CE_FLOAT(LOCAL_W, netvar.flChargedDamage);
				// Set our baseline bodyshot damage
				bdmg = 50;
				// Darwins damage correction
				if (HasDarwins(target)) {
					// Darwins protects against 15% of damage
					bdmg = (bdmg * .85) - 1;
					cdmg = (cdmg * .85) - 1;
				}
				// Vaccinator damage correction
				if (HasCondition<TFCond_UberBulletResist>(target)) {
					// Vac charge protects against 75% of damage
					bdmg = (bdmg * .25) - 1;
					cdmg = (cdmg * .25) - 1;
				} else if (HasCondition<TFCond_SmallBulletResist>(target)) {
					// Passive bullet resist protects against 10% of damage
					bdmg = (bdmg * .90) - 1;
					cdmg = (cdmg * .90) - 1;
				}
				// Invis damage correction
				if (IsPlayerInvisible(target)) {
					// Invis spies get protection from 10% of damage
					bdmg = (bdmg * .80) - 1;
					cdmg = (cdmg * .80) - 1;
				}
				// If can headshot and if bodyshot kill from charge damage, or if crit boosted and they have 150 health, or if player isnt zoomed, or if the enemy has less than 40, due to darwins, and only if they have less than 150 health will it try to bodyshot
				if (CanHeadshot() && (cdmg >= target->m_iHealth || IsPlayerCritBoosted(g_pLocalPlayer->entity) || !g_pLocalPlayer->bZoomed || target->m_iHealth <= bdmg)  && target->m_iHealth <= 150) {
					// We dont need to hit the head as a bodyshot will kill
					headonly = false;
				}
			}
		// In counter-strike source, headshots are what we want
		} else IF_GAME (IsCSS()) {
			headonly = true;
		}
		// Return our var of if we need to headshot
		return headonly;
			
	} break;	
	case 1: { // AUTO-CLOSEST priority
		// We dont need the head so just use anything
		return false;
	} break;
	case 2: { // Head only
		// User wants the head only
		return true;
	} break;
	}
	// We dont know what the user wants so just use anything
	return false;
}
	
// A function that determins whether aimkey allows aiming
bool UpdateAimkey() {
	static bool trigger_key_flip = false;
	static bool pressed_last_tick = false;
	bool allow_trigger_key = true;
	// Check if aimkey is used
	if (trigger_key && trigger_key_mode) {
		// Grab whether the aimkey is depressed
		bool key_down = g_IInputSystem->IsButtonDown((ButtonCode_t)(int)trigger_key);
		// Switch based on the user set aimkey mode
		switch ((int)trigger_key_mode) {
		// Only while key is depressed, enable
		case 1:
			if (!key_down) {
				allow_trigger_key = false;
			}
			break;
		// Only while key is not depressed, enable
		case 2:
			if (key_down) {
				allow_trigger_key = false;
			}
			break;
		// Aimkey acts like a toggle switch
		case 3:
			if (!pressed_last_tick && key_down) trigger_key_flip = !trigger_key_flip;
			if (!trigger_key_flip) {
				allow_trigger_key = false;
			}
		}
		pressed_last_tick = key_down;
	}
	// Return whether the aimkey allows aiming
	return allow_trigger_key;
}
	
// Func to find value of how far to target ents
float EffectiveTargetingRange() {
	// Melees use a close range, TODO add dynamic range for demoknight swords
	if (GetWeaponMode() == weapon_melee) {
		return 100.0f;
	// Pyros only have so much untill their flames hit
	} else if ( g_pLocalPlayer->weapon()->m_iClassID == CL_CLASS(CTFFlameThrower) ) {
		return 185.0f;
	}
	// If user has set a max range, then use their setting, 
	if (max_range) {
		return (float)max_range;
	// else use a pre-set range
	} else {
		return 8012.0f;
	}
}

void Draw() {

}

}}}


