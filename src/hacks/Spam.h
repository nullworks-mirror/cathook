/*
 * Spam.h
 *
 *  Created on: Jan 21, 2017
 *      Author: nullifiedcat
 */

#ifndef HACKS_SPAM_H_
#define HACKS_SPAM_H_

#include "IHack.h"

#include "../textfile.h"

class CatCommand;

namespace hacks { namespace shared { namespace spam {

extern CatVar enabled;
extern CatVar filename;
extern CatVar newlines;
extern CatCommand reload;

void CreateMove();
void Reset();
void Reload();

}}}

#endif /* HACKS_SPAM_H_ */
