/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(DispatchUserMessage, bool, void *this_, int type,
                     bf_read &buffer)
{
    return original::DispatchUserMessage(this_, type, buffer);
}
}