/*
 * xpcmutex.h
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#pragma once

/*
 * Cross-Process C shared mutex
 */

typedef struct xpcmutex_s
{
    char name[64];
    int  fd;
} xpcmutex_t;

xpcmutex_t
xpcmutex_init(const char *name);

xpcmutex_t
xpcmutex_connect(const char *name);

void
xpcmutex_close(xpcmutex_t mutex);

void
xpcmutex_destroy(xpcmutex_t mutex);

void
xpcmutex_lock(xpcmutex_t mutex);

void
xpcmutex_unlock(xpcmutex_t mutex);
