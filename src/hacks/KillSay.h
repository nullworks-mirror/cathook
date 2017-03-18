/*
 * KillSay.h
 *
 *  Created on: Jan 19, 2017
 *      Author: nullifiedcat
 */

#ifndef HACKS_KILLSAY_H_
#define HACKS_KILLSAY_H_

#include "IHack.h"

#include "../fixsdk.h"
#include <igameevents.h>


class CatCommand;

class KillSayEventListener : public IGameEventListener2 {
	virtual void FireGameEvent(IGameEvent* event);
};

namespace hacks { namespace shared { namespace killsay {

extern CatVar enabled;
extern CatVar filename;
extern CatCommand reload;

void Init();
void Shutdown();
void Reload();
std::string ComposeKillSay(IGameEvent* event);

}}}

#endif /* HACKS_KILLSAY_H_ */
