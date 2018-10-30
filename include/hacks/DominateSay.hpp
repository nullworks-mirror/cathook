/*
 * DominateSay.h
 *
 *  Created on: Oct 30, 2017
 */

#pragma once

#include "common.hpp"

namespace hacks::shared::dominatesay
{

void init();
void shutdown();
void reload();

extern const std::vector<std::string> builtin_default;
extern const std::vector<std::string> jp_anime;
} // namespace hacks::shared::dominatesay
