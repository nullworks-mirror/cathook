/*
 * ItemTitle.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "menu/ncc/Item.hpp"

namespace menu::ncc
{

class ItemTitle : public Item
{
public:
    ItemTitle(std::string title);

    virtual void Draw(int x, int y) override;

public:
    const std::string title;
};
}
