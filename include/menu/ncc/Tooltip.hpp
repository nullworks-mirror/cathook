/*
 * Tooltip.hpp
 *
 *  Created on: Mar 27, 2017
 *      Author: nullifiedcat
 */

#ifndef TOOLTIP_HPP_
#define TOOLTIP_HPP_
#if TEXTMODE_VAC != 1
#include "menu/CTextLabel.h"

#include "common.hpp"

namespace menu
{
namespace ncc
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
}
#endif
#endif /* TOOLTIP_HPP_ */
