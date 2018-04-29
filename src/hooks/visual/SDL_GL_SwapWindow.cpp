/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(SDL_GL_SwapWindow, void, SDL_Window *window)
{
    return original::SDL_GL_SwapWindow(window);
}

}