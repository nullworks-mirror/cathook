/*
 * Airstuck.h
 *
 *  Created on: Nov 26, 2016
 *      Author: nullifiedcat
 */

#ifndef HACKS_AIRSTUCK_H_
#define HACKS_AIRSTUCK_H_

#include "IHack.h"

namespace hacks { namespace shared { namespace airstuck {

extern CatVar stuck;

void SendNOP();
void CreateMove();
bool IsStuck();
void Reset();

}}}

#endif /* HACKS_AIRSTUCK_H_ */
