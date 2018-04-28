/*
 * AutoSticky.h
 *
 *  Created on: Dec 2, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

namespace hacks
{
namespace tf
{
namespace autosticky
{

extern CatVar enabled;
extern CatVar buildings;
extern CatVar distance;

bool ShouldDetonate(CachedEntity *bomb);
void CreateMove();
}
}
}
