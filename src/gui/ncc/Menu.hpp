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

class List;
class Tooltip;

extern unsigned long font_title; // Verdana Bold 10px?
extern unsigned long font_item;  // Verdana 	 10px?
extern Tooltip* tooltip;

void ShowTooltip(const std::string& text);

void Init();
List& MainList();

}}

#endif /* MENU_HPP_ */
