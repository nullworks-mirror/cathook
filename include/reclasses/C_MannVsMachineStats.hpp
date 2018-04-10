/*
 * C_MannVsMachineStats.hpp
 *
 *  Created on: Apr 10, 2018
 *      Author: bencat07
 */

#pragma once
#include "reclasses.hpp"

namespace re
{

class C_MannVsMachineStats
{
public:
    C_MannVsMachineStats() = delete;
    static C_MannVsMachineStats *C_MannVsMachineStats();

public:
    int AddLocalPlayerUpgrade(int id, unsigned &a3);
};
}
