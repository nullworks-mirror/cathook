/*
 * catpclient.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

void
cat_send_render_packet_begin(int fd, const float *world_to_screen);

void
cat_send_render_packet_end(int fd);

void
cat_send_render_packet_rect(int fd, float x, float y, float w, float h, const float *rgba);

void
cat_send_render_packet_rect_outline(int fd, float x, float y, float w, float h, const float *rgba, float thickness);

void
cat_send_render_packet_line(int fd, float x, float y, float dx, float dy, const float *rgba, float thickness);

void
cat_send_render_packet_string(int fd, float x, float y, const char *string, const float *rgba);

void
cat_send_render_packet_circle(int fd, float x, float y, float radius, const float *rgba, float thickness, int steps);
