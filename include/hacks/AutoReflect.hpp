/*
 * AutoReflect.h
 *
 *  Created on: Nov 18, 2016
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

namespace hacks
{
namespace tf
{
namespace autoreflect
{
void Draw();
void CreateMove();
bool ShouldReflect(CachedEntity *ent);
bool IsEntStickyBomb(CachedEntity *ent);
}
}
}
