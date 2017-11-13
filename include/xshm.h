/*
 * xshm.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

#include <stdint.h>

typedef struct xshm_s
{
    void *data;
    char name[64];
    uint32_t size;
} xshm_t;

xshm_t
xshm_init(const char *name, uint32_t size);

xshm_t
xshm_connect(const char *name, uint32_t size);

void
xshm_destroy(xshm_t xshm);

void
xshm_close(xshm_t xshm);

