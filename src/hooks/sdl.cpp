/*
 * sdl.cpp
 *
 *  Created on: May 19, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include "hooks/HookedMethods.hpp"
#include "hack.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_events.h>
#include <GL/glew.h>
#include <GL/gl.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

SDL_Window *sdl_current_window{ nullptr };
SDL_GL_SwapWindow_t *SDL_GL_SwapWindow_loc{ nullptr };
SDL_GL_SwapWindow_t SDL_GL_SwapWindow_o{ nullptr };

SDL_PollEvent_t *SDL_PollEvent_loc{ nullptr };
SDL_PollEvent_t SDL_PollEvent_o{ nullptr };

typedef uint32_t (*SDL_GetWindowFlags_t)(SDL_Window *window);

SDL_GetWindowFlags_t *SDL_GetWindowFlags_loc{ nullptr };
SDL_GetWindowFlags_t SDL_GetWindowFlags_o{ nullptr };

uint32_t SDL_GetWindowFlags_hook(SDL_Window *window)
{
    uint32_t flags = SDL_GetWindowFlags_o(window);
    flags &= ~(SDL_WINDOW_MINIMIZED | SDL_WINDOW_HIDDEN);
    flags |= SDL_WINDOW_SHOWN;
    return flags;
}

void SDL_GL_SwapWindow_hook(SDL_Window *window)
{

}

int SDL_PollEvent_hook(SDL_Event *event)
{
    int ret = SDL_PollEvent_o(event);
    if (ret)
    {
        logging::Info("event %x %x", event->type, event->common.type);
        if (event->type == SDL_WINDOWEVENT)
        {
            switch (event->window.event)
            {
            case SDL_WINDOWEVENT_HIDDEN:
                logging::Info("Window hidden");
                return 0;
            case SDL_WINDOWEVENT_MINIMIZED:
                logging::Info("Window minimized");
                return 0;
            }
        }
    }
    return ret;
}

void DoSDLHooking()
{
    SDL_GL_SwapWindow_loc = reinterpret_cast<SDL_GL_SwapWindow_t *>(
        sharedobj::libsdl().Pointer(0xFD648));

    SDL_GL_SwapWindow_o    = *SDL_GL_SwapWindow_loc;
    *SDL_GL_SwapWindow_loc = SDL_GL_SwapWindow_hook;

    //        SDL_GetWindowFlags_loc =
    //        reinterpret_cast<SDL_GetWindowFlags_t*>(sharedobj::libsdl().Pointer(0xFD588));

    //        SDL_GetWindowFlags_o = *SDL_GetWindowFlags_loc;
    //        *SDL_GetWindowFlags_loc = SDL_GetWindowFlags_hook;

    //        SDL_PollEvent_loc =
    //        reinterpret_cast<SDL_PollEvent_t*>(sharedobj::libsdl().Pointer(0xFCF64));

    //        SDL_PollEvent_o = *SDL_PollEvent_loc;
    //        *SDL_PollEvent_loc = SDL_PollEvent_hook;
}

void DoSDLUnhooking()
{
    *SDL_GL_SwapWindow_loc = SDL_GL_SwapWindow_o;
    //        *SDL_GetWindowFlags_loc = SDL_GetWindowFlags_o;
    //        *SDL_PollEvent_loc = SDL_PollEvent_o;
}
