/*
 * CreateMove.h
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#pragma once

class CUserCmd;

extern bool *bSendPackets;
bool CreateMove_hook(void *, float, CUserCmd *);
