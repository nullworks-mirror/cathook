/*
 * C_TFWeaponBase.hpp
 *
 *  Created on: Nov 23, 2017
 *      Author: nullifiedcat
 */

#pragma once

class C_TFWeaponBase : public C_BaseCombatWeapon
{
public:
    inline static IClientEntity *GetOwnerViaInterface(IClientEntity *self)
    {
        typedef IClientEntity *(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(451, offsets::undefined, 451), 0)(self);
    }
    inline static bool UsesPrimaryAmmo(IClientEntity *self)
    {
        typedef bool(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(447, offsets::undefined, 447), 0)(self);
    }
    inline static bool HasPrimaryAmmo(IClientEntity *self)
    {
        typedef bool(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(317, offsets::undefined, 317), 0)(self);
    }
    inline static bool AreRandomCritsEnabled(IClientEntity *self)
    {
        typedef bool(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(466, offsets::undefined, 466), 0)(self);
    }
    inline static bool CalcIsAttackCriticalHelper(IClientEntity *self)
    {
        typedef bool(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(460, offsets::undefined, 460), 0)(self);
    }
    inline static bool CalcIsAttackCriticalHelperNoCrits(IClientEntity *self)
    {
        typedef bool(*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(461, offsets::undefined, 461), 0)(self);
    }
    inline static bool CanFireCriticalShot(IClientEntity *self, bool unknown1, IClientEntity *unknown2)
    {    
        typedef bool(*fn_t)(IClientEntity *, bool, IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(489, offsets::undefined, 489), 0)(self, unknown1, unknown2);
    }
    inline static void AddToCritBucket(IClientEntity *self, float value)
    {
        constexpr float max_bucket_capacity = 1000.0f;
        bucket = fminf(bucket_() + value, max_bucket_capacity);
    }
    inline static bool IsAllowedToWithdrawFromCritBucket(IClientEntity *self, float value)
    {
        uint16_t weapon_info_handle = weapon_info_handle_(self);
        void *weapon_info = nullptr; // GetFileWeaponInfoFromHandle(weapon_info_handle);
        /*
        if (!weapon_info->unk_1736)
        {
            
        }
        */
    }
    inline static bool CalcIsAttackCriticalHelper_re(IClientEntity *self)
    {
        IClientEntity *owner = GetOwnerViaInterface(self);
        
        if (owner == nullptr)
            return false;
        
        if (!C_BaseEntity::IsPlayer(owner))
            return false;
        
        CTFPlayerShared *shared = C_BasePlayer::shared_(owner);
        float critmult = CTFPlayerShared::GetCritMult(shared);
        if (!CanFireCriticalShot(self, 0, nullptr))
            return false;
        
        if (CTFPlayerShared::IsCritBoosted(shared))
            return true;
            
        
        float multiplier = 0.5f;
        int seed = C_BaseEntity::m_nPredictionRandomSeed() ^ (owner->entindex() | self->entindex());
        RandomSeed(seed);
        
        bool result = true;
        if (multiplier * 10000.0f <= RandomInt(0, 9999))
        {
            result = false;
            multiplier = 0.0f;
        }
        
        return false;
    }
    inline static int CalcIsAttackCritical(IClientEntity *self)
    {
        IClientEntity *owner = GetOwnerViaInterface(self);
        if (owner)
        {
            if (IsPlayer(owner))
            {
                // Always run calculations
                // Never write anything into entity, at least from here.

                // if (g_GlobalVars->framecount != *(int *)(self + 2872))
                {
                    // *(int *)(self + 2872) = g_GlobalVars->framecount;
                    // *(char *)(self + 2839) = 0;

                    if (g_pGameRules->roundmode == 5 &&
                        g_pGameRules->winning_team == NET_INT(owner, netvar.iTeamNum))
                    {
                        // *(char *)(self + 2838) = 1;
                        return 1;
                    }
                    else
                    {
                        if (AreRandomCritsEnabled(self))
                        {
                            return CalcIsAttackCriticalHelper(self);
                        }
                        else
                        {
                            return CalcIsAttackCriticalHelperNoCrits(self);
                        }
                    }
                }
            }
        }

        return 0;
    }
    inline static uint16_t& weapon_info_handle_(IClientEntity *self)
    {
        return *(uint16_t *)(unsigned(self) + 2750u);
    }
    inline static float& crit_bucket_(IClientEntity *self)
    {
        return *(float *)(unsigned(self) + 2616u);
    }
};


