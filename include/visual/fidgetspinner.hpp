/*
 * fidgetspinner.hpp
 *
 *  Created on: Jul 4, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <visual/atlas.hpp>
#include "common.hpp"

class CatVar;

extern std::array<textures::sprite, 4> spinner_states;
extern CatVar v9mode;
void InitSpinner();
void DrawSpinner();
