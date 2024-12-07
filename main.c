/*
 * Copyright (c) 2022-2024 Wojtek Kaniewski <wojtekka@toxygen.net>
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
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <sched.h>
#include <limits.h>
#include <signal.h>

#define DEFAULT_PORT "/dev/parport0"

struct chip_config
{
    uint16_t id;                        /* Chip id (manufacturer and device) */
    uint32_t size;                      /* Size in bytes */
    uint32_t sector_size;               /* Sector size or 0 if byte-programmed */
    const char *name;                   /* Chip name */
    unsigned int max_write_usec;        /* Maximum sector/byte write time in microseconds */
    unsigned int max_chip_erase_sec;    /* Maximum chip erase time in seconds */
};

#define K(x) ((x) * 1024)
struct chip_config chip_config[] =
{
    { 0xda0b, K(256), 0, "W49F002A", 50, 1 },

    { 0x01a4, K(512), 0, "Am29F040", 7, 64 },

    { 0x1f5d, K(32), 128, "AT29C512", 10000, 20 },
    { 0x1fd5, K(128), 128, "AT29C010", 10000, 20 },
    { 0x1fda, K(256), 256, "AT29C020", 10000, 20 },
    { 0x1f5b, K(512), 512, "AT29C040", 10000, 20 },
    { 0x1fa4, K(512), 256, "AT29C040A", 10000, 20 },
    { 0x1f3b, K(512), 512, "AT29LV040", 10000, 20 },
    { 0x1fc4, K(512), 256, "AT29LV040A", 10000, 20 }
};
#undef K

pp_t pp;

void set_vcc(bool value)
{
    if (value)
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) | PARPORT_CONTROL_INIT);
    }
    else
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) & ~PARPORT_CONTROL_INIT);
    }
}

void set_vpp(bool value)
{
    if (value)
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) | PARPORT_CONTROL_STROBE);
    }
    else
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) & ~PARPORT_CONTROL_STROBE);
    }
}

void set_s4(bool value)
{
    if (value)
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) | PARPORT_CONTROL_SELECT);
    }
    else
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) & ~PARPORT_CONTROL_SELECT);
    }
}

void set_s6(bool value)
{
    if (value)
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) & ~PARPORT_CONTROL_AUTOFD);
    }
    else
    {
        pp_wcontrol(&pp, pp_rcontrol(&pp) | PARPORT_CONTROL_AUTOFD);
    }
}

void write_address(uint32_t value)
{
    pp_wdata(&pp, 0);
    set_s6(false);
    int i;
    const int addr_size = 24;
    for (i = 0; i < addr_size; ++i)
    {
        value <<= 1;
        uint8_t data = (value & (1 << addr_size)) ? 2 : 0;    // D
        pp_wdata(&pp, data);
        pp_wdata(&pp, data | 1);    // CLK
        pp_wdata(&pp, data);
    }
}

void write_data_w_delay(uint32_t addr, uint8_t value, unsigned int usec)
{
    write_address(addr);
    set_s6(true);

    usleep(2);
    pp_wdata(&pp, value);
    usleep(2);
    set_s4(false);
    if (usec)
    {
        usleep(usec);
    }
    set_s4(true);
}

void write_data(uint32_t addr, uint8_t value)
{
    write_data_w_delay(addr, value, 0);
}

uint8_t read_data(uint32_t addr)
{
    write_address(addr);
    set_s6(false);

    pp_wdata(&pp, 2 | 4);   // P/S=1, CLK=0
    pp_wdata(&pp, 2);       // P/S=1, CLK=1
    pp_wdata(&pp, 4);       // P/S=0, CLK=0
    int i;
    uint8_t value = 0;
    for (i = 0; i < 8; ++i)
    {
        value <<= 1;
        if (!(pp_rstatus(&pp) & PARPORT_STATUS_ACK))
        {
            value |= 1;
        }
        pp_wdata(&pp, 0);   // P/S=0, CLK=1
        pp_wdata(&pp, 4);   // P/S=0, CLK=0
    }
    pp_wdata(&pp, 0);

    return value;
}

uint16_t get_id(void)
{
    write_data(0x5555, 0xaa);
    write_data(0x2aaa, 0x55);
    write_data(0x5555, 0x90);
    usleep(10000);

    uint16_t id = read_data(0) << 8;
    id |= read_data(1);

    write_data(0x5555, 0xaa);
    write_data(0x2aaa, 0x55);
    write_data(0x5555, 0xf0);
    usleep(10000);

    return id;
}

void flash_write(uint32_t addr, const uint8_t *data, size_t len)
{
    write_data(0x5555, 0xaa);
    write_data(0x2aaa, 0x55);
    write_data(0x5555, 0xa0);
    
    while (len > 0)
    {
        write_data(addr, *data);
        addr++;
        data++;
        len--;
    }
}

void flash_erase(void)
{
    write_data(0x5555, 0xaa);
    write_data(0x2aaa, 0x55);
    write_data(0x5555, 0x80);

    write_data(0x5555, 0xaa);
    write_data(0x2aaa, 0x55);
    write_data(0x5555, 0x10);
}

int set_realtime(void)
{
	struct sched_param p;

	memset(&p, 0, sizeof(p));
	p.sched_priority = 1;

	return sched_setscheduler(0, SCHED_RR, &p);
}

void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [OPTIONS]\n"
                    "\n"
                    "  -p, --port=PORT       select either parport (e.g. /dev/parport0) or physical\n"
                    "                        port (e.g. 0x378). Default is " DEFAULT_PORT ".\n"
                    "  -e, --erase           erase chip\n"
                    "  -b, --blank-check     black check\n"
                    "  -r, --read=FILENAME   read chip to the specified file\n"
                    "  -w, --write=FILENAME  write chip from the specified file\n"
                    "  -v, --verify          verify after writing\n"
                    "  -o, --offset=BYTES    start reading or writing at specified offset\n"
                    "  -s, --size=BYTES      override chip size when reading\n"
                    "  -h, --help            print this message\n"
                    "\n"
                    "File format is binary. Multiple operations may be selected and will be executed\n"
                    "in the following order: erase, blank check, read or write, verify.\n"
                    "\n", argv0);
}

bool terminate = false;

void handle_signal(int)
{
    terminate = true;
}

int main(int argc, char **argv)
{
    const char *port = DEFAULT_PORT;
    bool do_erase = false;
    bool do_blank_check = false;
    const char *do_read = NULL;
    const char *do_write = NULL;
    bool do_verify = false;
    uint32_t size = 0;
    uint32_t offset = 0;
    int do_test = -1;

    while (true)
    {
        static const struct option long_options[] = 
        {
            { "test-off",       no_argument,        0, 0 }, // 0
            { "test-vcc-on",    no_argument,        0, 0 }, // 1
            { "test-vcc-off",   no_argument,        0, 0 }, // 2
            { "test-vpp-on",    no_argument,        0, 0 }, // 3
            { "test-vpp-off",   no_argument,        0, 0 }, // 4
            { "test-s4-high",   no_argument,        0, 0 }, // 5
            { "test-s4-low",    no_argument,        0, 0 }, // 6
            { "test-s6-high",   no_argument,        0, 0 }, // 7
            { "test-s6-low",    no_argument,        0, 0 }, // 8
            { "test-d-high",    no_argument,        0, 0 }, // 9
            { "test-d-low",     no_argument,        0, 0 }, // 10
            { "test-clk-high",  no_argument,        0, 0 }, // 11
            { "test-clk-low",   no_argument,        0, 0 }, // 12
            { "port",           required_argument,  0, 'p' },
            { "erase",          no_argument,        0, 'e' },
            { "blank-check",    no_argument,        0, 'b' },
            { "read",           required_argument,  0, 'r' },
            { "write",          required_argument,  0, 'w' },
            { "verify",         no_argument,        0, 'v' },
            { "offset",         required_argument,  0, 'o' },
            { "size",           required_argument,  0, 's' },
            { "help",           no_argument,        0, 'h' },
            { 0,                0,                  0, 0 }
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "p:ebr:w:vs:o:h", long_options, &option_index);

        if (c == -1)
        {
            break;
        }

        switch (c)
        {
        case 0:
            do_test = option_index;
            break;

        case 'p':
            port = optarg;
            break;

        case 'e':
            do_erase = true;
            break;

        case 'r':
            if (do_write)
            {
                fprintf(stderr, "Conflicting options\n");
                exit(1);
            }
            do_read = optarg;
            break;

        case 'w':
            if (do_read)
            {
                fprintf(stderr, "Conflicting options\n");
                exit(1);
            }
            do_write = optarg;
            break;

        case 'o':
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(optarg, &endptr, 0);
            if (tmp > UINT32_MAX || (tmp == ULONG_MAX && errno == ERANGE) || (tmp == 0 && errno == EINVAL) || (*endptr != 0))
            {
                fprintf(stderr, "Invalid offset '%s'\n", optarg);
                exit(1);
            }
            offset = (uint32_t) tmp;
            break;
        }

        case 's':
        {
            char *endptr = NULL;
            unsigned long tmp = strtoul(optarg, &endptr, 0);
            if (tmp > UINT32_MAX || (tmp == ULONG_MAX && errno == ERANGE) || (tmp == 0 && errno == EINVAL) || (*endptr != 0))
            {
                fprintf(stderr, "Invalid size '%s'\n", optarg);
                exit(1);
            }
            size = (uint32_t) tmp;
            break;
        }

        case 'v':
            do_verify = true;
            break;

        case 'b':
            if (do_write)
            {
                fprintf(stderr, "Conflicting options\n");
                exit(1);
            }
            do_blank_check = true;
            break;

        case 'h':
            usage(argv[0]);
            exit(0);

        default:
            exit(1);
        }
    }

    if (do_verify && !do_write)
    {
        fprintf(stderr, "Conflicting options\n");
        exit(1);
    }

    set_realtime();

    if (pp_open(&pp, port) == -1)
    {
        printf("pp_open failed\n");
        perror(port);
        exit(1);
    }

    signal(SIGTERM, handle_signal);
    signal(SIGINT, handle_signal);

    if (do_test != -1)
    {
        switch (do_test)
        {
        case 1: // vcc on
            pp_wdata(&pp, 0);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT | PARPORT_CONTROL_INIT);
            break;
        case 3: // vpp on
            pp_wdata(&pp, 0);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT | PARPORT_CONTROL_STROBE);
            break;
        case 6: // s4 low
            pp_wdata(&pp, 0);
            pp_wcontrol(&pp, 0);
            break;
        case 8: // s6 low
            pp_wdata(&pp, 0);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT | PARPORT_CONTROL_AUTOFD);
            break;
        case 9: // d high
            pp_wdata(&pp, 2);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT);
            break;
        case 11:    // clk high
            pp_wdata(&pp, 1);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT);
            break;
        default:
            pp_wdata(&pp, 0);
            pp_wcontrol(&pp, PARPORT_CONTROL_SELECT);
            break;
        }
        exit(0);
    }

    pp_wdata(&pp, 0);
    set_s4(true);
    set_s6(true);
    set_vpp(false);
    set_vcc(false);

    set_vcc(true);

    usleep(100000);

    uint16_t chip_id = get_id();

    struct chip_config *cc = NULL;
    for (int i = 0; i < sizeof(chip_config) / sizeof(chip_config[0]); ++i)
    {
        if (chip_id == chip_config[i].id)
        {
            cc = &chip_config[i];
            break;
        }
    }

    if (cc == NULL) 
    {
        fprintf(stderr, "Chip id 0x%04x not supported\n", chip_id);
        goto failure;
    }

    printf("Chip id 0x%04x (%s)\n", chip_id, cc->name);

    if (size == 0)
    {
        size = cc->size;
    }

    if (do_erase)
    {
        flash_erase();

        printf("Erasing...");
        fflush(stdout);

        /* Wait while the bit is toggling */
        while (!terminate)
        {
            uint8_t data1 = read_data(0);
            uint8_t data2 = read_data(0);

            if (data1 == 0xff && data2 == 0xff)
            {
                break;
            }

            usleep(100000);
        }

        if (!terminate)
        {
            printf("\nErase complete\n");
        }
    }

    if (!terminate && do_blank_check)
    {
        uint32_t addr;

        for (addr = 0; !terminate && addr < size; ++addr)
        {
            if (addr % 1024 == 0)
            {
                printf("\rBlank check %u kB...", addr / 1024);
                fflush(stdout);
            }

            uint8_t byte = read_data(offset + addr);

            if (byte != 0xff)
            {
                printf("\n");
                fprintf(stderr, "Black check failed at 0x%08x: 0x%02x\n", offset + addr, byte);
                goto failure;
            }
        }

        if (!terminate)
        {
            printf("\nBlank check complete\n");
        }
    }

    if (!terminate && do_read != NULL)
    {
        int fd = open(do_read, O_CREAT | O_TRUNC | O_WRONLY, 0644);

        if (fd == -1)
        {
            perror(do_read);
            goto failure;
        }

        uint8_t *buf = malloc(size);

        if (buf == NULL)
        {
            perror("malloc");
            close(fd);
            goto failure;
        }

        uint32_t addr;

        for (addr = 0; !terminate && addr < size; addr++)
        {
            if (addr % 1024 == 0)
            {
                printf("\rReading %u kB...", addr / 1024);
                fflush(stdout);
            }
            buf[addr] = read_data(addr);
        }
        
        if (!terminate)
        {
            printf("\nRead complete\n");

            if (write(fd, buf, size) != size)
            {
                perror("write");
                free(buf);
                close(fd);
                goto failure;
            }
        }

        free(buf);
        close(fd);
    }

    if (!terminate && flash && do_write != NULL)
    {
        int fd = open(do_write, O_RDONLY);

        if (fd == -1)
        {
            perror(do_write);
            goto failure;
        }

        const size_t buffer_len = (cc->sector_size > 0) ? cc->sector_size : 1024;
        uint8_t buf[buffer_len];
        uint32_t addr = 0;
        ssize_t read_len;

        /* Store size for verification */
        size = 0;

        while (!terminate && (read_len = read(fd, buf, buffer_len)) > 0)
        {
            if (addr % 1024 == 0)
            {
                printf("\rWriting %u kB...", addr / 1024);
                fflush(stdout);
            }

            if (cc->sector_size > 0)
            {
                bool empty = true;
                for (size_t i = 0; empty && i < read_len; ++i)
                {
                    if (buf[i] != 0xff)
                    {
                        empty = false;
                    }
                }

                if (!empty)
                {
                    flash_write(offset + addr, buf, read_len);
                    usleep(cc->max_write_usec);
                }
                addr += cc->sector_size;
                size += read_len;
            }
            else
            {
                for (size_t i = 0; i < read_len; ++i)
                {
                    if (buf[i] != 0xff)
                    {
                        flash_write(offset + addr, &buf[i], 1);
                        usleep(cc->max_write_usec);
                    }
                    addr++;
                }
                size += read_len;
            }
        }

        if (!terminate)
        {
            printf("\nWrite complete\n");
        }

        close(fd);
    }

    if (!terminate && do_verify && do_write != NULL)
    {
        int fd = open(do_write, O_RDONLY);

        if (fd == -1)
        {
            perror(do_write);
            goto failure;
        }

        uint8_t *buf = malloc(size);

        if (buf == NULL)
        {
            perror("malloc");
            close(fd);
            goto failure;
        }

        if (read(fd, buf, size) != size)
        {
            perror(do_read);
            close(fd);
            free(buf);
            goto failure;
        }

        close(fd);

        uint32_t addr;

        for (addr = 0; !terminate && addr < size; addr++)
        {
            if (addr % 1024 == 0)
            {
                printf("\rVerifying %u kB...", addr / 1024);
                fflush(stdout);
            }

            uint8_t byte = read_data(offset + addr);

            if (byte != buf[addr])
            {
                printf("\n");
                fprintf(stderr, "Verification failed at 0x%08x: expected 0x%02x, actual 0x%02x\n", offset + addr, buf[addr], byte);
                free(buf);
                goto failure;
            }
        }

        if (!terminate)
        {
            printf("\nVerify complete\n");
        }

        free(buf);
    }

    printf("Turning off\n");
    set_vcc(false);
    usleep(100000);
    pp_close(&pp);
    return 0;

failure:
    set_vcc(false);
    pp_close(&pp);
    return 1;
}
