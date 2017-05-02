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

extern const std::vector<std::string> builtin_default;
extern const std::vector<std::string> builtin_lennyfaces;
extern const std::vector<std::string> builtin_blanks;
extern const std::vector<std::string> builtin_nonecore;
extern const std::vector<std::string> builtin_lmaobox;
extern const std::vector<std::string> builtin_lithium;

extern CatVar enabled;
extern CatVar filename;
extern CatCommand reload;

void CreateMove();
void Reset();
void Reload();

}}}

#endif /* HACKS_SPAM_H_ */
