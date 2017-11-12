/*
 * catpclient.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "catpclient.h"
#include "catpackets.h"
#include "pipepacket.h"

void
cat_send_render_packet_begin(int fd, const float *world_to_screen)
{
    struct catp_draw_begin_t packet;
    memcpy(packet.world_to_screen, world_to_screen, sizeof(float) * 16);
    pipe_packet_send(fd, CATP_DRAW_BEGIN, sizeof(packet), &packet);
}

void
cat_send_render_packet_end(int fd)
{
    pipe_packet_send(fd, CATP_DRAW_END, 0, 0);
}

void
cat_send_render_packet_rect(int fd, float x, float y, float w, float h, const float *rgba)
{
    struct catp_draw_rect_t packet;
    packet.x = x;
    packet.y = y;
    packet.h = h;
    packet.w = w;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    pipe_packet_send(fd, CATP_DRAW_RECT, sizeof(packet), &packet);
}

void
cat_send_render_packet_rect_outline(int fd, float x, float y, float w, float h, const float *rgba, float thickness)
{
    struct catp_draw_rect_outline_t packet;
    packet.x = x;
    packet.y = y;
    packet.h = h;
    packet.w = w;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.thickness = thickness;
    pipe_packet_send(fd, CATP_DRAW_RECT_OUTLINE, sizeof(packet), &packet);
}

void
cat_send_render_packet_line(int fd, float x, float y, float dx, float dy, const float *rgba, float thickness)
{
    struct catp_draw_line_t packet;
    packet.x = x;
    packet.y = y;
    packet.dx = dx;
    packet.dy = dy;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.thickness = thickness;
    pipe_packet_send(fd, CATP_DRAW_LINE, sizeof(packet), &packet);
}

void
cat_send_render_packet_string(int fd, float x, float y, const char *string, const float *rgba)
{
    struct catp_draw_string_t packet;
    packet.x = x;
    packet.y = y;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.length = strlen(string);
    pipe_packet_send(fd, CATP_DRAW_STRING, sizeof(packet), &packet);
    pipe_packet_write_manual(fd, packet.length, string);
}

void
cat_send_render_packet_circle(int fd, float x, float y, float radius, const float *rgba, float thickness, int steps)
{
    struct catp_draw_circle_t packet;
    packet.x = x;
    packet.y = y;
    packet.radius = radius;
    packet.thickness = thickness;
    packet.steps = steps;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    pipe_packet_send(fd, CATP_DRAW_CIRCLE, sizeof(packet), &packet);
}
