/*
 * pipepacket.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <stdint.h>

#define PIPE_PACKET_MAGIC (unsigned short)0xCA75
#define PIPE_PACKET_MAGIC_DATA_END (unsigned short)0x2710

struct pipe_packet_header_t
{
    uint16_t magic;
    uint16_t type;
    uint32_t length;
};

/*
 *      Sends packet to pipe
 *      Returns 0 on success, -1 on failure
 */
int
pipe_packet_send(int fd, uint16_t type, uint32_t length, void *data);

/*
 *      DANGEROUS!
 *      Writes bytes of data to pipe.
 *      Returns 0 on success, -1 on failure
 */
int
pipe_packet_write_manual(int fd, uint32_t length, void *data);

/*
 *      Reads a packet from the pipe.
 *      Returns pointer to memory with packet data
 *              or NULL if read failed
 *              or valid pointer to undefined data if packet has 0 length
 *      Writes packet header data into *header
 */
void*
pipe_packet_read(int fd, struct pipe_packet_header_t *header);

/*
 *      DANGEROUS!
 *      Reads bytes of data from pipe.
 *      Returns 0 on success, -1 on failure
 *      Writes data into *data
 */
int
pipe_packet_read_manual(int fd, uint32_t length, void *data);
