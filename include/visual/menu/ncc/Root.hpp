/*
 * Root.hpp
 *
 *  Created on: Mar 26, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "../CBaseWindow.h"

namespace menu::ncc
{

class Root : public CBaseWindow
{
public:
    Root();
    void Setup();
    virtual void Update() override;
    virtual void OnKeyPress(ButtonCode_t key, bool repeat) override;
    virtual void Draw(int x, int y) override;
    inline virtual void MoveChildren() override{};
};
}
