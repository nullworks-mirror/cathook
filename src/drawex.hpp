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
#include "catsmclient.h"
}

#define draw_api drawex::api

namespace drawex
{

extern cat_shm_render_context_t ctx;
extern std::thread rendering_thread;

void rendering_routine();

namespace api
{

extern bool ready_state;

void initialize();

void draw_rect(float x, float y, float w, float h, const float* rgba);
void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness);
void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness);
void draw_rect_textured(float x, float y, float w, float h, const float* rgba, float u, float v, float s, float t);
void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps);
void draw_string(float x, float y, const char *string, const float *rgba);

void draw_begin();
void draw_end();

}

}
