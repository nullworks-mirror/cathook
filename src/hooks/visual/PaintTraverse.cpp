/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(PaintTraverse, void, vgui::IPanel *this_,
                     unsigned int panel, bool force, bool allow_force)
{
    return original::PaintTraverse(this_, panel, force, allow_force);
}
}