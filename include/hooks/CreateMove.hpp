/*
 * CreateMove.h
 *
 *  Created on: Jan 8, 2017
 *      Author: nullifiedcat
 */

#ifndef CREATEMOVE_H_
#define CREATEMOVE_H_

class CUserCmd;

extern bool *bSendPackets;
bool CreateMove_hook(void *, float, CUserCmd *);

#endif /* CREATEMOVE_H_ */
