/*
 * xshm.c
 *
 *  Created on: Nov 12, 2017
 *      Author: nullifiedcat
 */

#include "xshm.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

xshm_t
xshm_init(const char *name, uint32_t size)
{
    xshm_t xshm;
    strncpy(xshm.name, name, sizeof(xshm.name) - 1);
    char filename[128];
    snprintf(filename, 127, "xshm_%s", xshm.name);
    xshm.size = size;
    shm_unlink(xshm.name);
    int omask = umask(0);
    int flags = O_RDWR | O_CREAT;
    int fd = shm_open(xshm.name, flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0)
    {
        perror("xshm opening error\n");
        return xshm;
    }
    if (ftruncate(fd, size) != 0)
    {
        perror("xshm opening error\n");
        return xshm;
    }
    umask(omask);
    xshm.data = mmap(0, size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    if (xshm.data == (void *) -1)
    {
        perror("xshm mapping error\n");
        return xshm;
    }
    close(fd);
    return xshm;
}

xshm_t
xshm_connect(const char *name, uint32_t size)
{
    xshm_t xshm;
    strncpy(xshm.name, name, sizeof(xshm.name) - 1);
    char filename[128];
    snprintf(filename, 127, "xshm_%s", xshm.name);
    xshm.size = size;
    int omask = umask(0);
    int flags = O_RDWR;
    int fd = shm_open(xshm.name, flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if (fd < 0)
    {
        perror("xshm opening error\n");
        return xshm;
    }
    if (ftruncate(fd, size) != 0)
    {
        perror("xshm opening error\n");
        return xshm;
    }
    umask(omask);
    xshm.data = mmap(0, size, PROT_WRITE | PROT_READ | PROT_EXEC, MAP_SHARED, fd, 0);
    if (xshm.data == (void *) -1)
    {
        perror("xshm mapping error\n");
        return xshm;
    }
    close(fd);
    return xshm;
}

void
xshm_destroy(xshm_t xshm)
{
    munmap(xshm.data, xshm.size);
    shm_unlink(xshm.name);
}

void
xshm_close(xshm_t xshm)
{
    munmap(xshm.data, xshm.size);
}


