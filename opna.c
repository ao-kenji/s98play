/*
 * Copyright (c) 2015 Kenji Aoyama.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * handle PC-9801-86 FM synth part via pcexio(4).
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>	/* close(2) */
#include <sys/mman.h>	/* mmap(2) */
#include <machine/pcex.h>

#include "opna.h"

/* global */
int	pcexio_fd;
u_int8_t *pcexio_base;
u_int8_t *opna_bi_reg;	/* YM2608 Basic Index register */
u_int8_t *opna_bd_reg;	/* YM2608 Basic Data register */
u_int8_t *opna_ei_reg;	/* YM2608 Extended Index register */
u_int8_t *opna_ed_reg;	/* YM2608 Extended Data register */

/*
 * Open and initialize
 */
int
opna_open(void)
{
	u_int8_t *sound_id, data;

	pcexio_fd = open("/dev/pcexio", O_RDWR, 0600);
	if (pcexio_fd == -1) {
		perror("open");
		goto exit1;
	}

	pcexio_base = (u_int8_t *)mmap(NULL, 0x10000, PROT_READ | PROT_WRITE,
		MAP_SHARED, pcexio_fd, 0x0);

	if (pcexio_base == MAP_FAILED) {
		perror("mmap");
		goto exit2;
	}

	/* if we find WSN-A[24]F, set it up */
	data = *(pcexio_base + 0x51e3);
	if (data == 0xc2)
		*(pcexio_base + 0x57e3) = 0xf0;	/* sound ID = 0x40 */

	/* check the existence of PC-9801-86 board */
	sound_id = pcexio_base + 0xa460;
	data = *sound_id;
	if ((data & 0xf0) != 0x40) {
		perror("can not found PC-9801-86 board");
		goto exit2;
	}

	/* enable YM2608 */
	*sound_id = ((data & 0xfc) | 0x01);

	opna_bi_reg = pcexio_base + 0x188;	
	opna_bd_reg = pcexio_base + 0x18a;	
	opna_ei_reg = pcexio_base + 0x18c;	
	opna_ed_reg = pcexio_base + 0x18e;	

	/* enable YM2608(OPNA) mode (default is YM2203(OPN) mode) */
	data = opna_read(0x29);
	data |= 0x80;
	opna_write(0x29, data);

	return pcexio_fd;

exit2:
	munmap((void *)pcexio_base, 0x10000);
exit1:
	close(pcexio_fd);
	return -1;
}

/*
 * Close
 */
void
opna_close(void)
{
	u_int8_t data;

	/* key off all channel slots */
	opna_write(0x28, 0x00);		/* ch 1 */
	opna_write(0x28, 0x01);		/* ch 2 */
	opna_write(0x28, 0x02);		/* ch 3 */
	opna_write(0x28, 0x04);		/* ch 4 */
	opna_write(0x28, 0x05);		/* ch 5 */
	opna_write(0x28, 0x06);		/* ch 6 */

	/* output off */
	data = opna_read(0xb4);
	data &= 0x3f;
	opna_write(0xb4, data);

	data = opna_read(0xb5);
	data &= 0x3f;
	opna_write(0xb5, data);

	data = opna_read(0xb6);
	data &= 0x3f;
	opna_write(0xb6, data);

	data = opna_read_ex(0xb4);
	data &= 0x3f;
	opna_write_ex(0xb4, data);

	data = opna_read_ex(0xb5);
	data &= 0x3f;
	opna_write_ex(0xb5, data);

	data = opna_read_ex(0xb6);
	data &= 0x3f;
	opna_write_ex(0xb6, data);

	munmap((void *)pcexio_base, 0x10000);
	close(pcexio_fd);
}

/*
 * OPNA needs wait cycles
 *  after address write: 17 cycles @ 8MHz = 2.125 us 
 *  after data write   : 83 cycles @ 8MHz = 10.375 us
 * OPL3 wait cycles
 *  after address write: 1.9 us 
 *  after data write   : 83 cycles @ 8MHz = 10.375 us
 */

/*
 * Register read/write
 */
u_int8_t
opna_read(u_int8_t index)
{
	u_int8_t ret;

	*opna_bi_reg = index;
	ret = *opna_bd_reg;

	return ret;
}

u_int8_t
opna_read_ex(u_int8_t index)
{
	u_int8_t ret;

	*opna_ei_reg = index;
	ret = *opna_ed_reg;

	return ret;
}

void
opna_write(u_int8_t index, u_int8_t data)
{
	*opna_bi_reg = index;
	*opna_bd_reg = data;
}

void
opna_write_ex(u_int8_t index, u_int8_t data)
{
	*opna_ei_reg = index;
	*opna_ed_reg = data;
}
