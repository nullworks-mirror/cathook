/*
 * Spam.h
 *
 *  Created on: Jan 21, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "common.hpp"

class CatCommand;

namespace hacks
{
namespace shared
{
namespace spam
{

extern const std::vector<std::string> builtin_default;
extern const std::vector<std::string> builtin_lennyfaces;
extern const std::vector<std::string> builtin_blanks;
extern const std::vector<std::string> builtin_nonecore;
extern const std::vector<std::string> builtin_lmaobox;
extern const std::vector<std::string> builtin_lithium;

extern CatVar spam_source;

void Init();
void CreateMove();
void Reload();
}
}
}
