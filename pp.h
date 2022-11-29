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

#ifndef PP_H
#define PP_H

#include <linux/parport.h>

typedef enum
{
    PP_NONE = 0,
    PP_PARPORT,
    PP_DIRECT
} pp_type_t;

#define PP_PARPORT PP_PARPORT
#define PP_DIRECT PP_DIRECT

typedef struct
{
    pp_type_t type;

    /* PP_PARPORT */
    int fd;

    /* PP_DIRECT */
    int port;
} pp_t;

int pp_open(pp_t *p, const char *path);
int pp_close(pp_t *p);

int pp_rstatus(const pp_t *p);
int pp_wcontrol(const pp_t *p, unsigned char val);
int pp_rcontrol(const pp_t *p);
int pp_wdata(const pp_t *p, unsigned char val);
int pp_rdata(const pp_t *p);

int pp_realtime(void);

#endif /* PP_H */

