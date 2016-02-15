/*
 * Copyright (c) 2016 Kenji Aoyama.
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
 * play a 'S98' file on NEC PC-9801-86 sound board on OpenBSD/luna88k
 */

#include <err.h>
#include <stdio.h>

/*
 * S98 file specification can be found at:
 *  http://www.vesta.dti.ne.jp/~tsato/soft_s98v3.html
 */

#define S98_DEFAULT_TIMER_INFO	10
#define S98_DEFAULT_TIMER_INFO2	1000

#define S98_BUFLEN	32	/* XXX */

struct s98_device_info_t {
	u_int32_t	type;
#define	S98_DEVICE_NONE	0
#define	S98_DEVICE_YM2149	1	/* PSG  */
#define S98_DEVICE_YM2203	2	/* OPN  */
#define	S98_DEVICE_YM2612	3	/* OPN2 */
#define	S98_DEVICE_YM2608	4	/* OPNA */
#define	S98_DEVICE_YM2151	5	/* OPM  */
#define	S98_DEVICE_YM2413	6	/* OPLL */
#define	S98_DEVICE_YM3526	7	/* OPL  */
#define	S98_DEVICE_YM3812	8	/* OPL2 */
#define	S98_DEVICE_YMF262	9	/* OPL3 */
#define	S98_DEVICE_AY_3_8910	15	/* PSG  */
#define	S98_DEVICE_SN76489	16	/* DCSG */
	u_int32_t	clock;
	u_int32_t	pan;
	u_int32_t	dummy;	/* reserved */
};

struct s98_info_t {
	u_int32_t	version;
	u_int32_t	timer_sec;
	u_int32_t	timer_usec;
	u_int32_t	offset_tag;
	u_int32_t	offset_dump;
	u_int32_t	offset_loop;
	u_int32_t	device_count;

	/* only one device is supported on luna88k */
	struct s98_device_info_t	device_info;
} s98_info;

void
s98_wait_sync(u_int32_t sync)
{
	u_int32_t i;

	for (i = 0; i < sync; i++)
		opna_busy_wait(s98_info.timer_usec);
}

int
s98_read_4bytes(u_int32_t *dst, FILE *fp)
{
	u_int8_t buf[S98_BUFLEN];

	if (fread(buf, 1, 4, fp) != 4)
		errx(1, "data read (4 bytes) failed");

	*dst = buf[3] << 24 | buf[2] << 16 | buf[1] << 8 | buf[0];
	return 0;	/* success */
}

u_int32_t
s98_get_vvalue(FILE *fp)
{
	u_int32_t vvalue = 0;
	int count = 0;
	u_int8_t byte;

	do {
		if (fread(&byte, 1, 1, fp) != 1)
			errx(1, "data read failed");
		vvalue |= (byte & 0x7f) << (7 * count++);
	} while (byte & 0x80);

	vvalue += 2;

	return vvalue;
}
 
void
s98_print_tag(FILE *fp)
{
	u_int8_t byte;

	if (fseek(fp, s98_info.offset_tag, SEEK_SET) != 0)
		errx(1, "seek failed");

	while ((fread(&byte, 1, 1, fp) == 1) && (byte != 0x00)) {
		switch (byte) {
		case 0x0a:
			putchar('\n');
			break;
		default:
			putchar(byte);
			break;
		}
	}
}

int
s98_parse_header(FILE *fp)
{
	u_int8_t buf[S98_BUFLEN];
	u_int32_t compressing, timer_info, timer_info2;

	/*
	 * check magic and version
	 */
	if (fread(buf, 1, 4, fp) != 4)
		errx(1, "data read failed");

	if ((buf[0] != 'S') || (buf[1] != '9') || (buf[2] != '8'))
		errx(1, "bad magic '%c%c%c'", buf[0], buf[1], buf[2]);

	if ((buf[3] != '1') && (buf[3] != '3'))
		errx(1, "bad version '%c', should be '1' or '3'", buf[3]);

	s98_info.version = buf[3] - '0';

	/*
	 * timer
	 */
	if ((s98_read_4bytes(&timer_info, fp) != 0)
	    || (s98_read_4bytes(&timer_info2, fp) != 0))
		errx(1, "data read failed");

	if (timer_info  == 0)
		timer_info  = S98_DEFAULT_TIMER_INFO;
	if (timer_info2 == 0)
		timer_info2 = S98_DEFAULT_TIMER_INFO2;

	s98_info.timer_sec  = timer_info / timer_info2;
	s98_info.timer_usec = (timer_info * 1000000 / timer_info2) % 1000000;

	/*
	 * commpression method (should be 0)
	 */
	if (s98_read_4bytes(&compressing, fp) != 0)
		errx(1, "data read failed");

	if (compressing != 0)
		errx(1, "compressing %d is not supported");

	/*
	 * offsets
	 */
	if ((s98_read_4bytes(&s98_info.offset_tag, fp) != 0)
	    || (s98_read_4bytes(&s98_info.offset_dump, fp) != 0)
	    || (s98_read_4bytes(&s98_info.offset_loop, fp) != 0))
		errx(1, "data read failed");

	/*
	 * device count
	 */
	if (s98_read_4bytes(&s98_info.device_count, fp) != 0)
		errx(1, "data read failed");

	if (s98_read_4bytes(&s98_info.device_info.type, fp) != 0)
		errx(1, "data read failed");

	if (s98_info.device_count == 0) {
		s98_info.device_count = 1;
		s98_info.device_info.type = S98_DEVICE_YM2608;
	}

	if (s98_info.device_count != 1)
		errx(1, "device count is %d, should be 1", s98_info.device_count);

	if (s98_info.device_info.type != S98_DEVICE_YM2608)
		errx(1, "device_type should be YM2608");

	/*
	 * dump info
	 */
	printf("header info:\n");
	printf(" version: %d\n", s98_info.version);
	printf(" timer_sec: %d\n", s98_info.timer_sec);
	printf(" timer_usec: %d\n", s98_info.timer_usec);
	printf(" offset_tag: 0x%08x\n", s98_info.offset_tag);
	printf(" offset_dump: 0x%08x\n", s98_info.offset_dump);
	printf(" offset_loop: 0x%08x\n", s98_info.offset_loop);
	printf(" device_count: %d\n", s98_info.device_count);

	return 0;
}

int
s98_play(FILE *fp)
{
	u_int8_t buf[S98_BUFLEN];
	u_int32_t nsync;
	int s98_finish = 0;

	s98_parse_header(fp);

	s98_print_tag(fp);

	if (fseek(fp, s98_info.offset_dump, SEEK_SET) != 0)
		errx(1, "seek failed");

	while (s98_finish == 0) {
		if (fread(buf, 1, 1, fp) != 1)
			errx(1, "data read failed");

		switch (buf[0]) {
		case 0x00:
		case 0x01:
			if (fread(&buf[1], 1, 2, fp) != 2)
				errx(1, "data read failed");
			if (buf[1] == 0x27)
				buf[2] &= 0xf3; /* disable timer on luna88k */
			if (buf[0] == 0)
				opna_write(buf[1], buf[2]);
			else
				opna_write_ex(buf[1], buf[2]);
			break;	
		case 0xff:
			s98_wait_sync(1);
			break;
		case 0xfe:
			nsync = s98_get_vvalue(fp);
			s98_wait_sync(nsync);
			break;
		case 0xfd:
			s98_finish = 1;
			break;
		default:
			errx(1, "strange S98 command 0x%02x", buf[0]);
			break;
		}
	}
	return 0;
}
