/*
 * Tooltip.hpp
 *
 *  Created on: Mar 27, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "menu/CTextLabel.h"

#include "common.hpp"

namespace menu::ncc
{

class Tooltip : public CTextLabel
{
public:
    Tooltip();

    virtual void Draw(int x, int y) override;
    virtual void HandleCustomEvent(KeyValues *event) override;
    inline virtual PositionMode GetPositionMode() override
    {
        return PositionMode::FLOATING;
    }
};
}
