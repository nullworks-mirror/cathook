/*
 * HTrigger.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#ifndef HTRIGGER_H_
#define HTRIGGER_H_

#include "IHack.h"

class CatVar;

namespace hacks { namespace shared { namespace triggerbot {

extern CatVar enabled;
extern CatVar respect_cloak; // TODO move to Targeting
extern CatVar zoomed_only;
extern CatVar hitbox;
extern CatVar bodyshot;
extern CatVar finishing_hit;
extern CatVar max_range;
extern CatVar buildings;
extern CatVar ignore_vaccinator;
extern CatVar ambassador;
extern CatVar accuracy;

void CreateMove();
void Draw();

}}}

#endif /* HTRIGGER_H_ */
