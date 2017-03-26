/*
 * Menu.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef MENU_HPP_
#define MENU_HPP_

#include "../../drawing.h"

#include "List.hpp"

namespace menu { namespace ncc {

extern unsigned long font_title; // Verdana Bold 10px
extern unsigned long font_item;  // Verdana 	 10px

constexpr int color_fg = 		colors::Create(15, 150, 150, 255);
constexpr int color_bg = 		colors::Transparent(color_fg, 0.07f);
constexpr int color_bg_hover =  colors::Transparent(color_fg, 0.30f);

void Init();
List& MainList();

}}

#endif /* MENU_HPP_ */
