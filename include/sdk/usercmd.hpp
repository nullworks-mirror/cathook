/*
 * usercmd.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include <stdint.h>

#include <mathlib/vector.h>

class CUserCmd
{
public:
    virtual ~CUserCmd(){};
    int command_number;
    int tick_count;
    Vector viewangles;
    float forwardmove;
    float sidemove;
    float upmove;
    int buttons;
    uint8_t impulse;
    int weaponselect;
    int weaponsubtype;
    int random_seed;
    short mousedx;
    short mousedy;
    bool hasbeenpredicted;
};
