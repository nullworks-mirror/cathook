/*
 * pipepacket.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "pipepacket.h"

#include <unistd.h>
#include <stdio.h>

int
pipe_packet_send(int fd, uint16_t type, uint32_t length, void *data)
{
    int status;
    unsigned total = 0;
    unsigned size = 0;

    struct pipe_packet_header_t header;
    header.magic = PIPE_PACKET_MAGIC;
    header.type = type;
    header.length = length;

    if (pipe_packet_write_manual(fd, sizeof(struct pipe_packet_header_t), &header) < 0)
        return -1;
    if (length != 0)
    {
        if (pipe_packet_write_manual(fd, length, data) < 0)
            return -1;
        uint16_t magic = PIPE_PACKET_MAGIC_DATA_END;
        write(fd, &magic, sizeof(magic));
    }
    return 0;
}

int
pipe_packet_write_manual(int fd, uint32_t length, void *data)
{
    uint32_t total = 0;
    uint32_t size;
    int status;

    while (total < length)
    {
        size = length - total;
        status = write(fd, data + total, size);
        if (status < 0)
        {
            perror("pipe_packet write error");
            return -1;
        }
        total += status;
    }

    return 0;
}

void*
pipe_packet_read(int fd, struct pipe_packet_header_t *header)
{
    int capacity = 80;
    uint8_t *data = malloc(capacity);
    int status;
    int total = 0;
    int size = 0;

    while (total < sizeof(struct pipe_packet_header_t))
    {
        status = read(fd, (void *)(header + total), sizeof(struct pipe_packet_header_t) - total);
        if (status < 0)
        {
            perror("pipe_packet read error");
            return NULL;
        }
        total += status;
    }
    if (header->magic != PIPE_PACKET_MAGIC)
    {
        fprintf(stderr, "pipe_packet read corrupted packet\n");
        return NULL;
    }
    if (header->length != 0)
    {
        total = 0;
        while (total < header->length)
        {
            if (capacity - total < 60)
            {
                capacity *= 2 + 14;
                data = realloc(data, capacity);
            }
            size = header->length - total;
            if (size > capacity)
                size = capacity;
            status = read(fd, data, size);
            if (status < 0)
            {
                perror("pipe_packet read error");
                return NULL;
            }
            total += status;
        }
        uint16_t magic;
        read(fd, &magic, sizeof(magic));
        if (magic != PIPE_PACKET_MAGIC_DATA_END)
        {
            fprintf(stderr, "pipe_packet read corrupted packet\n");
            return NULL;
        }
    }
    return (void *)data;
}

int
pipe_packet_read_manual(int fd, uint32_t length, void *data)
{
    uint32_t total = 0;
    uint32_t size;
    int status;

    while (total < length)
    {
        size = length - total;
        status = read(fd, data + total, size);
        if (status < 0)
        {
            perror("pipe_packet read error");
            return -1;
        }
        total += status;
    }

    return 0;
}

