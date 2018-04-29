/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(FrameStageNotify, void, void *this_,
                     ClientFrameStage_t stage)
{
    return original::FrameStageNotify(this_, stage);
}
}