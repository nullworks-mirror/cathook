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

const int color_fg = 		colors::Create(255, 105, 180, 255);//colors::Create(15, 150, 150, 255);
const int color_bg = 		colors::Transparent(color_fg, 0.07f);
const int color_bg_hover =  colors::Transparent(color_fg, 0.32f);

void Init();
List& MainList();

}}

#endif /* MENU_HPP_ */
