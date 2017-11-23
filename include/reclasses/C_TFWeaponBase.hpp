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
};


