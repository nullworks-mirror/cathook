/*
 * ItemVariable.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "menu/ncc/Item.hpp"

class CatVar;

namespace menu
{
namespace ncc
{

class ItemVariable : public Item
{
public:
    ItemVariable(CatVar &variable);

    void Change(float amount);
    void PutChar(char ch);

    virtual void Update() override;
    virtual bool ConsumesKey(ButtonCode_t key) override;
    virtual void OnMousePress() override;
    virtual void OnFocusLose() override;
    virtual void OnKeyPress(ButtonCode_t key, bool repeat) override;
    virtual void Draw(int x, int y) override;

public:
    CatVar &catvar;
    bool capturing{ false };
};
}
}
