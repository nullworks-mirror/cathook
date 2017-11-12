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

void intialize();

void draw_rect(float x, float y, float w, float h, const float* rgba = colors::white);
void draw_rect_outlined(float x, float y, float w, float h, const float* rgba = colors::white, float thickness);
void draw_line(float x, float y, float dx, float dy, const float* rgba = colors::white, float thickness);
void draw_rect_textured(float x, float y, float w, float h, const float* rgba = colors::white, float u, float v, float s, float t);

void draw_begin();
void draw_end();

}

}
