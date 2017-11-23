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

                    if (g_pGameRules->critmode == 5 ||
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
    inline static float& crit_bucket_(IClientEntity *self)
    {
        return *(float *)(unsigned(self) + 2616u);
    }
};


