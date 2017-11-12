/*
 * drawex.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "drawing.h"
#include "colors.hpp"
#include "drawex.hpp"
#include "catpclient.h"

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

int pipe_fd;

namespace api
{

bool ready_state = false;

void intialize()
{
    pipe_fd = open(drawex_pipe_name, O_WRONLY);
    ready_state = true;
}

void draw_rect(float x, float y, float w, float h, const float* rgba)
{
    cat_send_render_packet_rect(pipe_fd, x, y, w, h, rgba);
}

void draw_rect_outlined(float x, float y, float w, float h, const float* rgba, float thickness)
{
    cat_send_render_packet_rect_outline(pipe_fd, x, y, w, h, rgba, thickness);
}

void draw_line(float x, float y, float dx, float dy, const float* rgba, float thickness)
{
    cat_send_render_packet_line(pipe_fd, x, y, dx, dy, rgba, thickness);
}

void draw_rect_textured(float x, float y, float w, float h, const float* rgba, float u, float v, float s, float t)
{
    assert(0);
}

void draw_circle(float x, float y, float radius, const float *rgba, float thickness, int steps)
{
    cat_send_render_packet_circle(pipe_fd, x, y, radius, rgba, thickness, steps);
}

void draw_begin()
{
    cat_send_render_packet_begin(pipe_fd, draw::wts.Base());
}

void draw_end()
{
    cat_send_render_packet_end(pipe_fd);
}

}}


