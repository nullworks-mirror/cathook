/*
 * drawgl.hpp
 *
 *  Created on: May 21, 2017
 *      Author: nullifiedcat
 */

#ifndef DRAWGL_HPP_
#define DRAWGL_HPP_

#include "drawing.h"

namespace drawgl {

extern const float white[4];

void Initialize();

void FilledRect(float x, float y, float w, float h, const float* rgba = white);
void Line(float x, float y, float dx, float dy, const float* rgba = white);
void Rect(float x, float y, float w, float h, const float* rgba = white);
void TexturedRect(float x, float y, float w, float h, float u, float v, float u2, float v2, const float* rgba = white);

void Refresh();
void Render();

void PreRender();
void PostRender();

extern bool ready_state;

}

#endif /* DRAWGL_HPP_ */
