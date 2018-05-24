/*
 * AntiCheat.hpp
 *
 *  Created on: Jun 5, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

#include "ac/aimbot.hpp"
#include "ac/antiaim.hpp"
#include "ac/bhop.hpp"

namespace hacks::shared::anticheat
{

void Accuse(int eid, const std::string &hack, const std::string &details);
static CatVar autorage(CV_SWITCH, "ac_autorage", "0", "Auto Rage");
void Init();
void CreateMove();

void ResetPlayer(int index);
void ResetEverything();
}
