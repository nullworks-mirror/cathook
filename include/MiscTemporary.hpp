/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#pragma once

#include "common.hpp"

// This is a temporary file to put code that needs moving/refactoring in.

extern bool *bSendPackets;
extern CatVar no_zoom;
extern CatVar clean_screenshots;
extern CatVar disable_visuals;
extern CatVar disconnect_reason;
#if ENABLE_VISUALS
extern int spectator_target;
#endif

extern SDL_Window *sdl_current_window;

void DoSDLHooking();
void DoSDLUnhooking();