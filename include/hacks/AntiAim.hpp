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
extern CatVar yaw;
extern CatVar pitch;
extern CatVar yaw_mode;
extern CatVar pitch_mode;
extern CatVar roll;
extern CatVar no_clamping;
extern CatVar spin;
extern CatVar lisp;
extern CatVar communicate;

extern int safe_space;

void SetSafeSpace(int safespace);
bool ShouldAA(CUserCmd *cmd);
void ProcessUserCmd(CUserCmd *cmd);
}