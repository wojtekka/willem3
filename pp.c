/*
 * (C) Copyright 2022 Wojtek Kaniewski <wojtekka@toxygen.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *    
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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

