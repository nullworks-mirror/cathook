/*
 * Menu.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "visual/drawing.hpp"
#include "visual/colors.hpp"

#include "../guicommon.h"

#include "menu/ncc/List.hpp"
#include "menu/ncc/Item.hpp"
#include "menu/ncc/ItemSublist.hpp"
#include "menu/ncc/ItemTitle.hpp"
#include "menu/ncc/ItemVariable.hpp"
#include "menu/ncc/List.hpp"
#include "menu/ncc/PlayerList.hpp"
#include "menu/ncc/PlayerListEntry.hpp"
#include "menu/ncc/Root.hpp"
#include "menu/ncc/Tooltip.hpp"

namespace menu
{
namespace ncc
{

class List;
class Tooltip;

extern CatVar scale;

constexpr int psize_font_item  = 12;
constexpr int psize_font_title = 14;

extern unsigned long font_title; // Verdana Bold 10px?
extern unsigned long font_item;  // Verdana 	 10px?

extern Tooltip *tooltip;
extern Root *root;

void RefreshFonts();
void ShowTooltip(const std::string &text);

void Init();
List &MainList();
}
}
