/*
 * drawex.hpp
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <thread>

extern "C"
{
#include "overlay.h"
}

#define draw_api drawex::api

namespace drawex
{

namespace api
{

struct font_handle_t
{
    xoverlay_font_handle_t handle;
};

struct texture_handle_t
{
    xoverlay_texture_handle_t handle;
};

font_handle_t    create_font(const char *path, float size);
texture_handle_t create_texture(const char *path);

void destroy_font(font_handle_t font);
void destroy_texture(texture_handle_t texture);

bool ready();

void initialize();

void draw_rect(float x, float y, float w, float h, const float* rgba);
void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness);
void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness);
void draw_rect_textured(float x, float y, float w, float h, const float* rgba, texture_handle_t texture, float u, float v, float s, float t);
void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps);
void draw_string(float x, float y, const char *string, font_handle_t font, const float *rgba);
void draw_string_with_outline(float x, float y, const char *string, font_handle_t font, const float *rgba, float thickness);

void draw_begin();
void draw_end();

}

}
