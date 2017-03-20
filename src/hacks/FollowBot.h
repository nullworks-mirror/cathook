/*
 * FollowBot.h
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#ifndef HACKS_FOLLOWBOT_H_
#define HACKS_FOLLOWBOT_H_

class CatCommand;
class CatVar;

namespace hacks { namespace shared { namespace followbot {

extern CatCommand move_to_crosshair;
extern CatCommand follow;
extern CatCommand follow_entity;
extern CatVar bot;

extern unsigned follow_steamid;
extern int following_idx;

void DoWalking();

}}}

#endif /* HACKS_FOLLOWBOT_H_ */
