/*
 * sdl.cpp
 *
 *  Created on: May 19, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"

#include "hooks/hookedmethods.hpp"
#include "hack.hpp"

#include <link.h>

#include <SDL.h>
#include <imgui.h>
#include <imgui_impl_sdl.h>
#include "../ftrender.hpp"

SDL_PollEvent_t* SDL_PollEvent_loc { nullptr };
SDL_GL_SwapWindow_t* SDL_GL_SwapWindow_loc { nullptr };
SDL_PollEvent_t SDL_PollEvent_o { nullptr };
SDL_GL_SwapWindow_t SDL_GL_SwapWindow_o { nullptr };

int SDL_PollEvent_hook(SDL_Event* event) {
    int retval = SDL_PollEvent_o(event);
    if (event && (event->key.keysym.sym & ~SDLK_SCANCODE_MASK) < 512) {
        ImGui_ImplSdl_ProcessEvent(event);
    }
    return retval;
}

#include <GL/gl.h>
#include "../atlas.hpp"
#include "../drawgl.hpp"
#include "../gui/im/Im.hpp"

void SDL_GL_SwapWindow_hook(SDL_Window* window) {
    static SDL_GLContext ctx_tf2 = SDL_GL_GetCurrentContext();
    static SDL_GLContext ctx_imgui = nullptr;
    static SDL_GLContext ctx_cathook = nullptr;

    static bool init { false };

    if (!disable_visuals) {
        PROF_SECTION(DRAW_cheat);
        if (not init) {
#if RENDERING_ENGINE_OPENGL
            ctx_imgui = SDL_GL_CreateContext(window);
            ImGui_ImplSdl_Init(window);
            ctx_cathook = SDL_GL_CreateContext(window);
#elif RENDERING_ENGINE_XOVERLAY

#elif
#error "Unsupported rendering engine"
#endif
            FTGL_Init();
            textures::Init();
            draw_api::initialize();
        }

        if (!cathook) {
            SDL_GL_MakeCurrent(window, ctx_tf2);
            SDL_GL_SwapWindow_o(window);
            return;
        }

        SDL_GL_MakeCurrent(window, ctx_cathook);
        {
            std::lock_guard<std::mutex> draw_lock(drawing_mutex);
            draw_api::render();
        }
        SDL_GL_MakeCurrent(window, ctx_imgui);
        {
            PROF_SECTION(DRAW_imgui);
            ImGui_ImplSdl_NewFrame(window);
            menu::im::Render();
            ImGui::Render();
        }
    }
    {
        PROF_SECTION(DRAW_valve);
        SDL_GL_MakeCurrent(window, ctx_tf2);
        SDL_GL_SwapWindow_o(window);
    }
}

void DoSDLHooking() {
	SDL_GL_SwapWindow_loc = reinterpret_cast<SDL_GL_SwapWindow_t*>(sharedobj::libsdl().Pointer(0xFD648));
	SDL_PollEvent_loc = reinterpret_cast<SDL_PollEvent_t*>(sharedobj::libsdl().Pointer(0xFCF64));

	SDL_GL_SwapWindow_o = *SDL_GL_SwapWindow_loc;
	SDL_PollEvent_o = *SDL_PollEvent_loc;

	*SDL_GL_SwapWindow_loc = SDL_GL_SwapWindow_hook;
	*SDL_PollEvent_loc = SDL_PollEvent_hook;
}

void DoSDLUnhooking() {
	*SDL_GL_SwapWindow_loc = SDL_GL_SwapWindow_o;
	*SDL_PollEvent_loc = SDL_PollEvent_o;
}

