/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(SendNetMsg, bool, INetChannel *this_, INetMessage &message,
                     bool force_reliable, bool voice)
{
    return original::SendNetMsg(this_, message, force_reliable, voice);
}
}