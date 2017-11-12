/*
 * drawex.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "drawex.hpp"
#include "catpclient.h"

#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

const char *drawex_pipe_name = "/tmp/cathook-rendering-pipe";

namespace drawex
{

int pipe_fd;

namespace api
{

void intialize()
{
    pipe_fd = open(drawex_pipe_name, O_WRONLY);
}

void draw_rect(float x, float y, float w, float h, const float* rgba = colors::white)
{
    cat_send_render_packet_rect(pipe_fd, x, y, w, h, rgba);
}

void draw_rect_outlined(float x, float y, float w, float h, const float* rgba = colors::white, float thickness)
{
    cat_send_render_packet_rect_outline(pipe_fd, x, y, w, h, rgba, thickness);
}

void draw_line(float x, float y, float dx, float dy, const float* rgba = colors::white, float thickness)
{
    cat_send_render_packet_line(pipe_fd, x, y, dx, dy, rgba, thickness);
}

void draw_rect_textured(float x, float y, float w, float h, const float* rgba = colors::white, float u, float v, float s, float t)
{
    static_assert(0, "draw_rect_textured is not implemented");
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


