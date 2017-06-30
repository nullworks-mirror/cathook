/*
 * HTrigger.h
 *
 *  Created on: Oct 5, 2016
 *      Author: nullifiedcat
 */

#ifndef HTRIGGER_H_
#define HTRIGGER_H_

#include "../common.h"

class CatVar;

namespace hacks { namespace shared { namespace triggerbot {

void CreateMove();
bool ShouldShoot();
bool IsTargetStateGood(CachedEntity* entity);
CachedEntity* FindEntInSight(float range);
bool HeadPreferable(CachedEntity* target);
bool UpdateAimkey();
float EffectiveTargetingRange();
void Draw();

}}}

#endif /* HTRIGGER_H_ */
