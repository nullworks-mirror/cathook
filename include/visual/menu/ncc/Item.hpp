/*
 * Item.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "../CBaseWidget.h"

namespace menu::ncc
{

class Item : public CBaseWidget
{
public:
    constexpr static int psize_x = 220;
    constexpr static int psize_y = 15;

    static int size_x;
    static int size_y;

    Item(std::string name = "ncc_menu_item");

    virtual void Draw(int x, int y) override;
    virtual void HandleCustomEvent(KeyValues *event) override;
};
}
