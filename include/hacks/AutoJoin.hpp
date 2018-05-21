/*
 * AutoJoin.hpp
 *
 *  Created on: Jul 28, 2017
 *      Author: nullifiedcat
 */

#pragma once
#include "common.hpp"
namespace hacks
{
namespace shared
{
namespace autojoin
{

extern CatVar auto_queue;
extern Timer queuetime;

void Update();
void UpdateSearch();
}
}
}
