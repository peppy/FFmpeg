/*
 * Buffered file io for ffmpeg system
 * Copyright (c) 2001 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/avstring.h"
#include "avformat.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdlib.h>
#include "os_support.h"


/* standard file protocol */

static int file_open(URLContext *h, const char *filename, int flags)
{
    int access;
    int fd;

    av_strstart(filename, "file:", &filename);

    if (flags & URL_RDWR) {
        access = O_CREAT | O_TRUNC | O_RDWR;
    } else if (flags & URL_WRONLY) {
        access = O_CREAT | O_TRUNC | O_WRONLY;
    } else {
        access = O_RDONLY;
    }
#ifdef O_BINARY
    access |= O_BINARY;
#endif
    fd = open(filename, access, 0666);
    if (fd < 0)
        return AVERROR(ENOENT);
    h->priv_data = (void *)(size_t)fd;
    return 0;
}

static int file_read(URLContext *h, unsigned char *buf, int size)
{
    int fd = (size_t)h->priv_data;
    return read(fd, buf, size);
}

static int file_write(URLContext *h, unsigned char *buf, int size)
{
    int fd = (size_t)h->priv_data;
    return write(fd, buf, size);
}

/* XXX: use llseek */
static offset_t file_seek(URLContext *h, offset_t pos, int whence)
{
    int fd = (size_t)h->priv_data;
    return lseek(fd, pos, whence);
}

static int file_close(URLContext *h)
{
    int fd = (size_t)h->priv_data;
    return close(fd);
}

URLProtocol file_protocol = {
    "file",
    file_open,
    file_read,
    file_write,
    file_seek,
    file_close,
};

/* standard memory protocol */

static int memory_open(URLContext *h, const char *memoryname, int flags)
{
    MemoryStreamDefinition *md = NULL;
	char *split[2];

	md = av_mallocz(sizeof(MemoryStreamDefinition));

	av_strstart(memoryname, "memory:", &memoryname);

    split[0] = strtok(memoryname, "|"); // Splits spaces between words in str
    split[1] = strtok(NULL, "|");

	md->curr = atoi(split[0]);
	md->start = md->curr;
	md->size = atoi(split[1]);
	
    h->priv_data = md;

	return 0;
}

static int memory_read(URLContext *h, unsigned char *buf, int size)
{
    MemoryStreamDefinition *md = h->priv_data;
	
	int readable = md->size - (md->curr - md->start);

	if (readable <= 0)
		return 0;

	if (size < readable)
		readable = size;

	for (int i = 0; i < readable; i++)
		*(buf++) = *(md->curr++);

    return readable;
}

static int memory_write(URLContext *h, unsigned char *buf, int size)
{
    return -1;
}

/* XXX: use llseek */
static offset_t memory_seek(URLContext *h, offset_t pos, int whence)
{
    MemoryStreamDefinition *md = h->priv_data;
	md->curr = md->start + pos;
	return (int)md->curr;
}

static int memory_close(URLContext *h)
{
    av_free(h->priv_data);
	return 0;
}

URLProtocol memory_protocol = {
    "memory",
    memory_open,
    memory_read,
    memory_write,
    memory_seek,
    memory_close,
};

/* pipe protocol */

static int pipe_open(URLContext *h, const char *filename, int flags)
{
    int fd;
    char *final;
    av_strstart(filename, "pipe:", &filename);

    fd = strtol(filename, &final, 10);
    if((filename == final) || *final ) {/* No digits found, or something like 10ab */
        if (flags & URL_WRONLY) {
            fd = 1;
        } else {
            fd = 0;
        }
    }
#ifdef O_BINARY
    setmode(fd, O_BINARY);
#endif
    h->priv_data = (void *)(size_t)fd;
    h->is_streamed = 1;
    return 0;
}

URLProtocol pipe_protocol = {
    "pipe",
    pipe_open,
    file_read,
    file_write,
};
