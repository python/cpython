///////////////////////////////////////////////////////////////////////////////
//
/// \file       full_flush.c
/// \brief      Encode files using LZMA_FULL_FLUSH
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"
#include "lzma.h"
#include <stdio.h>

#define CHUNK 64


static lzma_stream strm = LZMA_STREAM_INIT;
static FILE *file_in;


static void
encode(size_t size, lzma_action action)
{
	uint8_t in[CHUNK];
	uint8_t out[CHUNK];
	lzma_ret ret;

	do {
		if (strm.avail_in == 0 && size > 0) {
			const size_t amount = my_min(size, CHUNK);
			strm.avail_in = fread(in, 1, amount, file_in);
			strm.next_in = in;
			size -= amount; // Intentionally not using avail_in.
		}

		strm.next_out = out;
		strm.avail_out = CHUNK;

		ret = lzma_code(&strm, size == 0 ? action : LZMA_RUN);

		if (ret != LZMA_OK && ret != LZMA_STREAM_END) {
			fprintf(stderr, "%s:%u: %s: ret == %d\n",
					__FILE__, __LINE__, __func__, ret);
			exit(1);
		}

		fwrite(out, 1, CHUNK - strm.avail_out, stdout);

	} while (size > 0 || strm.avail_out == 0);

	if ((action == LZMA_RUN && ret != LZMA_OK)
			|| (action != LZMA_RUN && ret != LZMA_STREAM_END)) {
		fprintf(stderr, "%s:%u: %s: ret == %d\n",
				__FILE__, __LINE__, __func__, ret);
		exit(1);
	}
}


int
main(int argc, char **argv)
{
	file_in = argc > 1 ? fopen(argv[1], "rb") : stdin;


	// Config
	lzma_options_lzma opt_lzma;
	if (lzma_lzma_preset(&opt_lzma, 1)) {
		fprintf(stderr, "preset failed\n");
		exit(1);
	}
	lzma_filter filters[LZMA_FILTERS_MAX + 1];
	filters[0].id = LZMA_FILTER_LZMA2;
	filters[0].options = &opt_lzma;
	filters[1].id = LZMA_VLI_UNKNOWN;

	// Init
	if (lzma_stream_encoder(&strm, filters, LZMA_CHECK_CRC32) != LZMA_OK) {
		fprintf(stderr, "init failed\n");
		exit(1);
	}

// 	if (lzma_easy_encoder(&strm, 1)) {
// 		fprintf(stderr, "init failed\n");
// 		exit(1);
// 	}

	// Encoding
	encode(0, LZMA_FULL_FLUSH);
	encode(6, LZMA_FULL_FLUSH);
	encode(0, LZMA_FULL_FLUSH);
	encode(7, LZMA_FULL_FLUSH);
	encode(0, LZMA_FULL_FLUSH);
	encode(0, LZMA_FINISH);

	// Clean up
	lzma_end(&strm);

	return 0;
}
