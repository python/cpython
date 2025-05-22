/*
 * Copyright (c) 2008-2020 Stefan Krah. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <mpdecimal.h>


int
main(int argc, char **argv)
{
	mpd_context_t ctx;
	mpd_t *a, *b;
	mpd_t *q, *r;
	char *qs, *rs;
	char status_str[MPD_MAX_FLAG_STRING];
	clock_t start_clock, end_clock;

	if (argc != 3) {
		fprintf(stderr, "divmod: usage: ./divmod x y\n");
		exit(1);
	}

	mpd_init(&ctx, 38);
	ctx.traps = 0;

	q = mpd_new(&ctx);
	r = mpd_new(&ctx);
	a = mpd_new(&ctx);
	b = mpd_new(&ctx);
	mpd_set_string(a, argv[1], &ctx);
	mpd_set_string(b, argv[2], &ctx);

	start_clock = clock();
	mpd_divmod(q, r, a, b, &ctx);
	end_clock = clock();
	fprintf(stderr, "time: %f\n\n",
	           (double)(end_clock-start_clock)/(double)CLOCKS_PER_SEC);

	qs = mpd_to_sci(q, 1);
	rs = mpd_to_sci(r, 1);

	mpd_snprint_flags(status_str, MPD_MAX_FLAG_STRING, ctx.status);
	printf("%s  %s  %s\n", qs, rs, status_str);

	mpd_del(q);
	mpd_del(r);
	mpd_del(a);
	mpd_del(b);
	mpd_free(qs);
	mpd_free(rs);

	return 0;
}


