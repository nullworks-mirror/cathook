/*
 * catsmclient.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "catsmclient.h"
#include "catpackets.h"
#include "shmstream.h"

cat_shm_render_context_t
cat_shm_connect(const char *name)
{
    cat_shm_render_context_t ctx;
    ctx.shm = xshm_connect(name, CAT_SHM_SIZE);
    ctx.mutex = xpcmutex_connect(name);
    ctx.curpos = 0;
    ctx.lastpacket = 0;
    return ctx;
}

void
cat_shm_render_begin(cat_shm_render_context_t *ctx, const float *world_to_screen)
{
    xpcmutex_lock(ctx->mutex);
    struct catp_draw_begin_t packet;
    memcpy(packet.world_to_screen, world_to_screen, sizeof(float) * 16);
    cat_shm_packet_send(ctx, CATP_DRAW_BEGIN, sizeof(packet), &packet);
}

void
cat_shm_render_end(cat_shm_render_context_t *ctx)
{
    cat_shm_packet_send(ctx, CATP_DRAW_END, 0, 0);
    xpcmutex_unlock(ctx->mutex);
}

void
cat_shm_render_rect(cat_shm_render_context_t *ctx, float x, float y, float w, float h, const float *rgba)
{
    struct catp_draw_rect_t packet;
    packet.x = x;
    packet.y = y;
    packet.h = h;
    packet.w = w;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    cat_shm_packet_send(ctx, CATP_DRAW_RECT, sizeof(packet), &packet);
}

void
cat_shm_render_rect_outline(cat_shm_render_context_t *ctx, float x, float y, float w, float h, const float *rgba, float thickness)
{
    struct catp_draw_rect_outline_t packet;
    packet.x = x;
    packet.y = y;
    packet.h = h;
    packet.w = w;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.thickness = thickness;
    cat_shm_packet_send(ctx, CATP_DRAW_RECT_OUTLINE, sizeof(packet), &packet);
}

void
cat_shm_render_line(cat_shm_render_context_t *ctx, float x, float y, float dx, float dy, const float *rgba, float thickness)
{
    struct catp_draw_line_t packet;
    packet.x = x;
    packet.y = y;
    packet.dx = dx;
    packet.dy = dy;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.thickness = thickness;
    cat_shm_packet_send(ctx, CATP_DRAW_LINE, sizeof(packet), &packet);
}

void
cat_shm_render_string(cat_shm_render_context_t *ctx, float x, float y, const char *string, const float *rgba)
{
    struct catp_draw_string_t packet;
    packet.x = x;
    packet.y = y;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    packet.length = strlen(string);
    cat_shm_packet_send(ctx, CATP_DRAW_STRING, sizeof(packet), &packet);
    cat_shm_packet_write_manual(ctx, packet.length, string);
}

void
cat_shm_render_circle(cat_shm_render_context_t *ctx, float x, float y, float radius, const float *rgba, float thickness, int steps)
{
    struct catp_draw_circle_t packet;
    packet.x = x;
    packet.y = y;
    packet.radius = radius;
    packet.thickness = thickness;
    packet.steps = steps;
    memcpy(packet.rgba, rgba, sizeof(float) * 4);
    cat_shm_packet_send(ctx, CATP_DRAW_CIRCLE, sizeof(packet), &packet);
}



