/*
 * C_BaseCombatWeapon.hpp
 *
 *  Created on: Nov 23, 2017
 *      Author: nullifiedcat
 */

#pragma once

class C_BaseCombatWeapon : public IClientEntity
{
public:
    inline int GetSlot()
    {
        typedef int(*fn_t)(C_BaseCombatWeapon *);
        return vfunc<fn_t>(this, 395, 0)(this);
    }
};


