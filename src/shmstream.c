/*
 * shmstream.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "shmstream.h"

#include <unistd.h>
#include <stdio.h>

int
cat_shm_packet_send(cat_shm_render_context_t *ctx, uint16_t type, uint32_t length, void *data)
{
    cat_shm_packet_header_t packet;
    packet.magic = CAT_SHM_PACKET_MAGIC;
    packet.type = type;
    packet.length = length;

    if (cat_shm_packet_write_manual(ctx, sizeof(packet), &packet) < 0)
        return -1;
    if (length != 0)
    {
        if (cat_shm_packet_write_manual(ctx, length, data) < 0)
            return -1;
    }
    ((struct cat_shm_header_s *)ctx->shm.data)->lastpacket++;
    return 0;
}

int
cat_shm_packet_write_manual(cat_shm_render_context_t *ctx, uint32_t length, void *data)
{
    uint32_t total = 0;
    uint32_t size;
    int status;


    while (total < length)
    {
        size = length - total;
        uint32_t size_avail = ctx->shm.size - ctx->curpos - sizeof(struct cat_shm_header_s);
        if (size_avail < size)
        {
            size = size_avail;
        }
        memcpy(ctx->shm.data + sizeof(struct cat_shm_header_s) +  ctx->curpos, data + total, size);
        ctx->curpos += size;
        if (ctx->curpos == ctx->shm.size - sizeof(struct cat_shm_header_s))
        {
            ctx->curpos = 0;
        }
        total += size;
    }
    return 0;
}

int
cat_shm_packet_read(cat_shm_render_context_t *ctx, cat_shm_packet_header_t *header, void *out, uint32_t out_length)
{
    int status;
    int total = 0;
    int size = 0;

    while (((struct cat_shm_header_s *)ctx->shm.data)->lastpacket == ctx->lastpacket) usleep(1);

    xpcmutex_lock(ctx->mutex);

    if (cat_shm_packet_read_manual(ctx, sizeof(cat_shm_packet_header_t), header) < 0)
    {
        fprintf(stderr, "cat_shm read packet error\n");
        xpcmutex_unlock(ctx->mutex);
        return -1;
    }
    if (header->magic != CAT_SHM_PACKET_MAGIC)
    {
        fprintf(stderr, "cat_shm read corrupted packet: %hx %hx %x\n", header->magic, header->type, header->length);
        xpcmutex_unlock(ctx->mutex);
        return -1;
    }
    if (header->length != 0)
    {
        if (out_length < header->length)
        {
            printf("not enough memory %x %x %hx %hx\n", out_length, header->length, header->type, header->magic);
            return -1;
        }
        //printf("reading payload %u %hu\n", header->length, header->type);
        cat_shm_packet_read_manual(ctx, header->length, out);
        /*total = 0;
        while (total < header->length)
        {
            size = header->length - total;
            if (size > out_length)
            {
                printf("total %u length %u max %u\n", total, header->length, out_length);
                return -1;
            }
            uint32_t size_avail = ctx->shm.size - ctx->curpos -  sizeof(struct cat_shm_header_s);
            if (size_avail < size)
            {
                size = size_avail;
            }
            printf("reading %x from %x\n", size, ctx->curpos + sizeof(struct cat_shm_header_s));
            memcpy(out + total, ctx->shm.data + sizeof(struct cat_shm_header_s) +  ctx->curpos, size);
            ctx->curpos += size;
            if (ctx->curpos + sizeof(struct cat_shm_header_s) == ctx->shm.size)
            {
                ctx->curpos = 0;
            }
            total += size;
        }*/
    }
    ctx->lastpacket++;// ((struct cat_shm_header_s *)ctx->shm.data)->lastpacket;
    xpcmutex_unlock(ctx->mutex);

    return 0;
}

int
cat_shm_packet_read_manual(cat_shm_render_context_t *ctx, uint32_t length, void *data)
{
    uint32_t total = 0;
    uint32_t size;

    while (total < length)
    {
        size = length - total;
        uint32_t size_avail = ctx->shm.size - ctx->curpos - sizeof(struct cat_shm_header_s);
        if (size_avail < size)
        {
            //printf("overflow\n");
            size = size_avail;
        }
        //printf("s %x t %x l %x p %x\n", size, total, length, ctx->curpos);

        memcpy(data + total, ctx->shm.data + sizeof(struct cat_shm_header_s) + ctx->curpos, size);
        ctx->curpos += size;
        if (ctx->curpos == ctx->shm.size - sizeof(struct cat_shm_header_s))
        {
            //printf("OVERFLOW %x\n", ctx->curpos);
            ctx->curpos = 0;
        }
        total += size;
    }
    return 0;
}

