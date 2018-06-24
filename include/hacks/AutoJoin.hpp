/*
 * AutoJoin.hpp
 *
 *  Created on: Jul 28, 2017
 *      Author: nullifiedcat
 */

#pragma once
#include "common.hpp"
namespace hacks::shared::autojoin
{

extern CatVar auto_queue;
extern Timer queuetime;

void Update();
void UpdateSearch();
}
