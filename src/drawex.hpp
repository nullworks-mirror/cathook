/*
 * drawex.hpp
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#define draw_api drawex::api

namespace drawex
{

extern int pipe_fd;

namespace api
{

extern bool ready_state;

void intialize();

void draw_rect(float x, float y, float w, float h, const float* rgba);
void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness);
void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness);
void draw_rect_textured(float x, float y, float w, float h, const float* rgba, float u, float v, float s, float t);
void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps);

void draw_begin();
void draw_end();

}

}
