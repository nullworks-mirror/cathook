/*
 * ItemTitle.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#ifndef ITEMTITLE_HPP_
#define ITEMTITLE_HPP_
#if TEXTMODE_VAC != 1
#include "menu/ncc/Item.hpp"

namespace menu
{
namespace ncc
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
}
#endif
#endif /* ITEMTITLE_HPP_ */
