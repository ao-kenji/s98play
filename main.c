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
 * NEC PC-9801-86 sound board (FM synth part) on OpenBSD/luna88k
 */

#include <stdio.h>
#include <stdlib.h>	/* getprogname(3) */
#include <unistd.h>	/* getopt(3) */

#include "opna.h"

/* prototype */
int s98play(FILE *);

void	usage(void);

/* global */
int debug = 0;		/* debug */
int opna_fd;
FILE *sound_fp = NULL;

int
main(int argc, char **argv)
{
	/*
	 * parse options
	 */
	int ch;
	extern char *optarg;
	extern int optind, opterr;

	while ((ch = getopt(argc, argv, "d")) != -1) {
		switch (ch) {
		case 'd':	/* debug flag */
			debug = 1;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
		return 1;
	}

	opna_fd = opna_open();
	if (opna_fd == -1) goto exit1;

	sound_fp = fopen(argv[0], "rb");
	if (sound_fp == NULL) goto exit2;

	s98_play(sound_fp);

	fclose(sound_fp);

	opna_close();

	return 0;	/* success */

exit2:
	opna_close();
exit1:
	return 1;	/* error */
}

void
usage(void)
{
	printf("Usage: %s [options] s98file\n", getprogname());
	printf("\t-d	: debug flag\n");
	exit(1);
}
