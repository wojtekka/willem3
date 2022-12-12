/*
 * Copyright (c) 2022 Wojtek Kaniewski <wojtekka@toxygen.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the author nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "pp.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/io.h>
#include <linux/ppdev.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define CONTROL_MASK 0x0b
#define STATUS_MASK 0x80

int pp_rstatus(const pp_t *p)
{
	unsigned char val;

	switch (p->type)
    {
		case PP_DIRECT:
			return inb(p->port + 1) ^ STATUS_MASK;

		case PP_PARPORT:
			if (ioctl(p->fd, PPRSTATUS, &val) == -1)
				return -1;

			return val ^ STATUS_MASK;

        default:
            return -1;
	}
}

int pp_wcontrol(const pp_t *p, unsigned char val)
{
	val ^= CONTROL_MASK;

	switch (p->type)
    {
		case PP_DIRECT:
			outb(val, p->port + 2);
			return 0;

		case PP_PARPORT:
			return ioctl(p->fd, PPWCONTROL, &val);

        default:
            return -1;
	}
}

int pp_rcontrol(const pp_t *p)
{
	unsigned char val;

	switch (p->type)
    {
		case PP_DIRECT:
			return inb(p->port + 2) ^ CONTROL_MASK;

		case PP_PARPORT:
			if (ioctl(p->fd, PPRCONTROL, &val) == -1)
				return -1;

			return val ^ CONTROL_MASK;

        default:
        	return -1;
	}
}

int pp_wdata(const pp_t *p, unsigned char val)
{
	switch (p->type)
    {
		case PP_DIRECT:
			outb(val, p->port);
			return 0;

		case PP_PARPORT:
			return ioctl(p->fd, PPWDATA, &val);

        default:
        	return -1;
	}
}

int pp_rdata(const pp_t *p)
{
	unsigned char val;

	switch (p->type)
    {
		case PP_DIRECT:
			return inb(p->port);

		case PP_PARPORT:
			if (ioctl(p->fd, PPRDATA, &val) == -1)
				return -1;

			return val;

        default:
        	return -1;
	}
}

int pp_open(pp_t *p, const char *port)
{
    if (port == NULL)
    {
        return -1;
    }

	if (strncmp(port, "/dev", 4) == 0)
    {
		int fd = open(port, O_RDWR);

		if (fd == -1)
        {
			return -1;
        }

		p->type = PP_PARPORT;
		p->fd = fd;

		if (ioctl(fd, PPCLAIM, 0) == -1)
        {
			int errno_save = errno;
			close(fd); 
			errno = errno_save;
			return -1;
		}

		return 0;
	}

	if (strncmp(port, "0x", 2) == 0)
    {
		p->type = PP_DIRECT;
		p->port = strtoul(port, NULL, 0);

	    return ioperm(p->port, 3, 1);
	}

	return -1;
}

int pp_close(pp_t *p)
{
	switch (p->type)
    {
		case PP_PARPORT:
			if (ioctl(p->fd, PPRELEASE, 0) == -1)
            {
				close(p->fd);
				return -1;
			}

            p->type = PP_NONE;
			return close(p->fd);

		case PP_DIRECT:
            p->type = PP_NONE;
            return ioperm(p->port, 3, 0);

        default:
	        p->type = PP_NONE;
	        return -1;
	}
}

