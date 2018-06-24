/*
 * PlayerList.hpp
 *
 *  Created on: Apr 11, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "../CBaseContainer.h"
#include "menu/ncc/Menu.hpp"
namespace menu::ncc
{

constexpr int size_table[] = { 32, 32, 80, 160, 80, 80, 32, 32, 32 };
int size_table_width();

class PlayerList : public CBaseContainer
{
public:
    PlayerList();

    virtual void Draw(int x, int y) override;
    virtual void Update() override;
    virtual void OnKeyPress(ButtonCode_t key, bool repeat) override;
    virtual void MoveChildren() override;
    virtual bool IsVisible() override;
    inline virtual void SortByZIndex() override{};
};
}
