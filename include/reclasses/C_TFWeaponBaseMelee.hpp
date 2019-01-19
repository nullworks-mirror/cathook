/*
 * C_TFWeaponBaseMelee.hpp
 *
 *  Created on: Nov 23, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "reclasses.hpp"

namespace re
{

class C_TFWeaponBaseMelee : public C_TFWeaponBase
{
public:
    inline static bool DoSwingTrace(IClientEntity *self, trace_t *trace)
    {
        typedef bool (*fn_t)(IClientEntity *, trace_t *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(523, offsets::undefined, 523), 0)(self, trace);
    }
    inline static int GetSwingRange(IClientEntity *self)
    {
        if (g_pLocalPlayer->holding_sapper)
            return 115;
        typedef int (*fn_t)(IClientEntity *);
        return vfunc<fn_t>(self, offsets::PlatformOffset(521, offsets::undefined, 521), 0)(self);
    }
};
} // namespace re
