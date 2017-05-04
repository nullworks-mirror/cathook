/*
 * FollowBot.h
 *
 *  Created on: Mar 19, 2017
 *      Author: nullifiedcat
 */

#ifdef IPC_ENABLED

#ifndef HACKS_FOLLOWBOT_H_
#define HACKS_FOLLOWBOT_H_

class CatCommand;
class CatVar;
class CachedEntity;

#include "../ipc.h"

namespace hacks { namespace shared { namespace followbot {

extern CatCommand move_to_crosshair;
extern CatCommand follow;
extern CatCommand follow_entity;
extern CatVar bot;

extern unsigned follow_steamid;
extern int following_idx;

bool IsBot(CachedEntity* entity);
void DoWalking();
void PrintDebug();
void AddMessageHandlers(ipc::peer_t* peer);
void AfterCreateMove();

}}}

#endif /* HACKS_FOLLOWBOT_H_ */

#endif
