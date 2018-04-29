/*
  Created by Jenny White on 29.04.18.
  Copyright (c) 2018 nullworks. All rights reserved.
*/

#include <SDL_syswm.h>
#include <MiscTemporary.hpp>
#include <visual/SDLHooks.hpp>
#include "HookedMethods.hpp"

namespace hooked_methods
{

DEFINE_HOOKED_METHOD(SDL_GL_SwapWindow, void, SDL_Window *window)
{
    static SDL_SysWMinfo wminfo;
    wminfo.version.major = 2;
    wminfo.version.minor = 0;
    typedef SDL_bool (*SDL_GetWindowWMInfo_t)(SDL_Window * window,
                                              SDL_SysWMinfo * info);
    static SDL_GetWindowWMInfo_t GetWindowWMInfo =
            *reinterpret_cast<SDL_GetWindowWMInfo_t *>(
                    sharedobj::libsdl().Pointer(0xFD4D8));
    static bool init_wminfo{ false };
    if (!init_wminfo)
    {
        GetWindowWMInfo(window, &wminfo);
        init_wminfo = true;
    }
    if (!sdl_hooks::window)
        sdl_hooks::window = window;

    static bool init{ false };

    static SDL_GLContext tf2_sdl = SDL_GL_GetCurrentContext();

    if (cathook && !disable_visuals)
    {
        PROF_SECTION(SWAPWINDOW_cathook);
        if (not init)
        {
            draw_api::initialize();
            init = true;
        }
        render_cheat_visuals();
    }
    {
        PROF_SECTION(SWAPWINDOW_tf2);
        SDL_GL_MakeCurrent(window, tf2_sdl);
        original::SDL_GL_SwapWindow(window);
        // glXMakeContextCurrent(wminfo.info.x11.display,
        // wminfo.info.x11.window,
        //                      wminfo.info.x11.window, tf2);
        // glXSwapBuffers(wminfo.info.x11.display, wminfo.info.x11.window);
    }
}
}