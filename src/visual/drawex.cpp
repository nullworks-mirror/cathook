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
#include "overlay.h"
}

namespace drawex
{

namespace api
{

bool ready_state = false;
bool init = false;

void initialize()
{
    int status = xoverlay_init();
    if (status < 0)
    {
        logging::Info("ERROR: could not initialize Xoverlay");
    }
}

void draw_rect(float x, float y, float w, float h, const float* rgba)
{
    PROF_SECTION(DRAWEX_draw_rect);
    xoverlay_draw_rect(x, y, w, h, *reinterpret_cast<xoverlay_rgba_t *>(rgba));
}

void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness)
{
    PROF_SECTION(DRAWEX_draw_rect_outline);
    xoverlay_draw_rect_outline(x, y, w, h, *reinterpret_cast<xoverlay_rgba_t *>(rgba), thickness);
}

void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness)
{
    PROF_SECTION(DRAWEX_draw_line);
    xoverlay_draw_line(x, y, dx, dy, *reinterpret_cast<xoverlay_rgba_t *>(rgba), thickness);
}

void draw_rect_textured(float x, float y, float w, float h, const float* rgba, texture_handle_t texture, float u, float v, float s, float t)
{
    PROF_SECTION(DRAWEX_draw_rect_textured);
    xoverlay_draw_rect_textured(x, y, w, h, *reinterpret_cast<xoverlay_rgba_t *>(rgba), texture.handle, u, v, s, t);
}

void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps)
{
    PROF_SECTION(DRAWEX_draw_circle);
    xoverlay_draw_circle(x, y, radius, *reinterpret_cast<xoverlay_rgba_t *>(rgba), thickness, steps);
}

void draw_string(float x, float y, const char *string, font_handle_t font, const float *rgba)
{
    PROF_SECTION(DRAWEX_draw_string);
    xoverlay_draw_string(x, y, string, font.handle, *reinterpret_cast<xoverlay_rgba_t *>(rgba), nullptr, nullptr);
}

void draw_string_with_outline(float x, float y, const char *string, font_handle_t font, const float *rgba, const float *rgba_outline, float thickness)
{
    PROF_SECTION(DRAWEX_draw_string_with_outline);
    xoverlay_draw_string_with_outline(x, y, string, font.handle, *reinterpret_cast<xoverlay_rgba_t *>(rgba), *reinterpret_cast<xoverlay_rgba_t *>(rgba_outline), thickness, 1, nullptr, nullptr);
}

void draw_begin()
{
    PROF_SECTION(DRAWEX_draw_begin);
    xoverlay_draw_begin();
}

void draw_end()
{
    PROF_SECTION(DRAWEX_draw_end);
    xoverlay_draw_end();
}

}}


