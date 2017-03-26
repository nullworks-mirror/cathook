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

void Init();
List& MainList();

}}

#endif /* MENU_HPP_ */
