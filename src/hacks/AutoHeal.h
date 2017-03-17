/*
 * AutoHeal.h
 *
 *  Created on: Dec 3, 2016
 *      Author: nullifiedcat
 */

#ifndef HACKS_AUTOHEAL_H_
#define HACKS_AUTOHEAL_H_

#include "IHack.h"

namespace hacks { namespace tf { namespace autoheal {

extern CatVar enabled;
extern CatVar silent;
// TODO extern CatVar target_only;
void CreateMove();

int BestTarget();
int HealingPriority(int idx);
bool CanHeal(int idx);

}}}

#endif /* HACKS_AUTOHEAL_H_ */
