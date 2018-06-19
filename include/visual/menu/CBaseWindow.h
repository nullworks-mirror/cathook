/*
 * CBaseWindow.h
 *
 *  Created on: Jan 25, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "visual/menu/CBaseContainer.h"

class CBaseWindow : public CBaseContainer
{
public:
    inline CBaseWindow(std::string name = "unnamed", IWidget *parent = nullptr)
        : CBaseContainer(name, parent)
    {
    }
    inline virtual ~CBaseWindow(){};

    virtual void OnFocusGain() override;
    virtual void OnFocusLose() override;
    virtual void Draw(int x, int y) override;
    virtual void MoveChildren() override;
};
