/*
 * AntiAim.h
 *
 *  Created on: Oct 26, 2016
 *      Author: nullifiedcat
 */

#pragma once

class CatVar;
class CUserCmd;

namespace hacks::shared::antiaim
{

// TODO paste AA from AimTux

extern CatVar enabled;
extern CatVar communicate;
extern CatVar yaw_mode;

extern int safe_space;

void SetSafeSpace(int safespace);
bool ShouldAA(CUserCmd *cmd);
void ProcessUserCmd(CUserCmd *cmd);
}
