/*
 * drawex.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "common.hpp"
#include "hack.hpp"

extern "C"
{
#include "catsmclient.h"
}

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

const char *drawex_pipe_name = "/tmp/cathook-rendering-pipe";

namespace drawex
{

cat_shm_render_context_t ctx;
std::thread rendering_thread;

void rendering_routine()
{
    xpcmutex_t server_throttle_mutex = xpcmutex_connect("rendering-throttle");
    while (true)
    {
        xpcmutex_lock(server_throttle_mutex);
        PROF_SECTION(DRAWEX_rendering_routine);
        if (hack::initialized && api::ready_state)
        {
            BeginCheatVisuals();
            DrawCheatVisuals();

#if ENABLE_GUI
            g_pGUI->Update();
#endif

            EndCheatVisuals();
        }
        xpcmutex_unlock(server_throttle_mutex);
        usleep(1000000 / 45);
        //std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

CatCommand restart_render("debug_xoverlay_restart", "restart", []() {
    api::ready_state = false;
    xshm_close(ctx.shm);
    xpcmutex_close(ctx.mutex);
    api::initialize();
});

namespace api
{

bool ready_state = false;

void initialize()
{
    rendering_thread = std::thread(rendering_routine);
    ctx = cat_shm_connect("cathook-rendering");
    ready_state = true;
}

void draw_rect(float x, float y, float w, float h, const float* rgba)
{
    PROF_SECTION(DRAWEX_draw_rect);
    cat_shm_render_rect(&ctx, x, y, w, h, rgba);
}

void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness)
{
    PROF_SECTION(DRAWEX_draw_rect_outline);
    cat_shm_render_rect_outline(&ctx, x, y, w, h, rgba, thickness);
}

void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness)
{
    PROF_SECTION(DRAWEX_draw_line);
    cat_shm_render_line(&ctx, x, y, dx, dy, rgba, thickness);
}

void draw_rect_textured(float x, float y, float w, float h, const float* rgba, float u, float v, float s, float t)
{
    assert(0);
}

void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps)
{
    PROF_SECTION(DRAWEX_draw_circle);
    cat_shm_render_circle(&ctx, x, y, radius, rgba, thickness, steps);
}

void draw_string(float x, float y, const char *string, const float *rgba)
{
    PROF_SECTION(DRAWEX_draw_string);
    cat_shm_render_string(&ctx, x, y, string, rgba);
}

void draw_begin()
{
    PROF_SECTION(DRAWEX_draw_begin);
    cat_shm_render_begin(&ctx, draw::wts.Base());
}

void draw_end()
{
    PROF_SECTION(DRAWEX_draw_end);
    cat_shm_render_end(&ctx);
}

}}


