/*
 * catsmclient.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include "xshm.h"
#include "xpcmutex.h"
#include "shmstream.h"

cat_shm_render_context_t
cat_shm_connect(const char *name);

void
cat_shm_render_begin(cat_shm_render_context_t *ctx, const float *world_to_screen);

void
cat_shm_render_end(cat_shm_render_context_t *ctx);

void
cat_shm_render_rect(cat_shm_render_context_t *ctx, float x, float y, float w, float h, const float *rgba);

void
cat_shm_render_rect_outline(cat_shm_render_context_t *ctx, float x, float y, float w, float h, const float *rgba, float thickness);

void
cat_shm_render_line(cat_shm_render_context_t *ctx, float x, float y, float dx, float dy, const float *rgba, float thickness);

void
cat_shm_render_string(cat_shm_render_context_t *ctx, float x, float y, const char *string, const float *rgba);

void
cat_shm_render_circle(cat_shm_render_context_t *ctx, float x, float y, float radius, const float *rgba, float thickness, int steps);
