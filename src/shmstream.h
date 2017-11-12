/*
 * shmstream.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <stdint.h>
#include "xshm.h"
#include "xpcmutex.h"

#define CAT_SHM_PACKET_MAGIC (unsigned short)0xCA75
#define CAT_SHM_SIZE 1024 * 1024 * 8

typedef struct cat_shm_render_context_s
{
    xshm_t      shm;
    xpcmutex_t  mutex;
    uint32_t    curpos;
    uint32_t    lastpacket;
} cat_shm_render_context_t;

typedef struct cat_shm_packet_header_s
{
    uint16_t magic;
    uint16_t type;
    uint32_t length;
} cat_shm_packet_header_t;

struct cat_shm_header_s
{
    uint32_t lastpacket;
};

int
cat_shm_packet_send(cat_shm_render_context_t *ctx, uint16_t type, uint32_t length, void *data);

int
cat_shm_packet_write_manual(cat_shm_render_context_t *ctx, uint32_t length, void *data);

int
cat_shm_packet_read(cat_shm_render_context_t *ctx, cat_shm_packet_header_t *header, void *out, uint32_t out_length);

int
cat_shm_packet_read_manual(cat_shm_render_context_t *ctx, uint32_t length, void *data);
