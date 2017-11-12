/*
 * xpcmutex.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "xpcmutex.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

xpcmutex_t
xpcmutex_init(const char *name)
{
    xpcmutex_t mutex;
    strncpy(mutex.name, name, sizeof(mutex.name) - 1);
    char filename[128];
    snprintf(filename, 127, "/tmp/.xpcmutex.%s", mutex.name);
    int mask = umask(0);
    remove(filename);
    mkfifo(filename, 0666);
    umask(mask);
    mutex.fd = open(filename, O_RDWR);
    xpcmutex_unlock(mutex);
    return mutex;
}

xpcmutex_t
xpcmutex_connect(const char *name)
{
    xpcmutex_t mutex;
    strncpy(mutex.name, name, sizeof(mutex.name) - 1);
    char filename[128];
    snprintf(filename, 127, "/tmp/.xpcmutex.%s", mutex.name);
    mutex.fd = open(filename, O_RDWR);
    return mutex;
}

void
xpcmutex_close(xpcmutex_t mutex)
{
    close(mutex.fd);
}

void
xpcmutex_destroy(xpcmutex_t mutex)
{
    char filename[128];
    snprintf(filename, 127, "/tmp/.xpcmutex.%s", mutex.name);
    remove(filename);
}

void
xpcmutex_lock(xpcmutex_t mutex)
{
    char buf[1];
    while (1 != read(mutex.fd, buf, 1))
    {
        usleep(10);
    }
}

void
xpcmutex_unlock(xpcmutex_t mutex)
{
    while (1 != write(mutex.fd, "1", 1))
    {
        usleep(10);
    }
}
