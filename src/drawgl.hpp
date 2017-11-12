/*
 * drawgl.hpp
 *
 *  Created on: May 21, 2017
 *      Author: nullifiedcat
 */

#ifndef DRAWGL_HPP_
#define DRAWGL_HPP_

#include "drawing.h"

#define draw_api drawgl

extern "C" {
#include "freetype-gl/vec234.h"
#include "freetype-gl/vertex-buffer.h"
}

namespace drawgl {

struct vertex_v2c4_t {
	ftgl::vec2 xy;
	ftgl::vec4 rgba;
};

struct vertex_v2t2c4_t {
	ftgl::vec2 xy;
	ftgl::vec2 st;
	ftgl::vec4 rgba;
};

extern ftgl::vertex_buffer_t* buffer_lines;
extern ftgl::vertex_buffer_t* buffer_triangles_plain;
extern ftgl::vertex_buffer_t* buffer_triangles_textured;

extern const float white[4];

void initialize();

void draw_rect(float x, float y, float w, float h, const float* rgba);
void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness);
void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness);
void draw_rect_textured(float x, float y, float w, float h, const float* rgba, float u, float v, float s, float t);
void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps);
void draw_string(float x, float y, const char *string, const float *rgba);

void draw_begin();
void draw_end();

void render();

extern bool ready_state;

}

#endif /* DRAWGL_HPP_ */
