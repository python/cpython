///////////////////////////////////////////////////////////////////////////////
//
/// \file       test_bcj_exact_size.c
/// \brief      Tests BCJ decoding when the output size is known
///
/// These tests fail with XZ Utils 5.0.3 and earlier.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"


/// Something to be compressed
static const uint8_t in[16] = "0123456789ABCDEF";

/// in[] after compression
static uint8_t compressed[1024];
static size_t compressed_size = 0;

/// Output buffer for decompressing compressed[]
static uint8_t out[sizeof(in)];


static void
compress(void)
{
	// Compress with PowerPC BCJ and LZMA2. PowerPC BCJ is used because
	// it has fixed 4-byte alignment which makes triggering the potential
	// bug easy.
	lzma_options_lzma opt_lzma2;
	succeed(lzma_lzma_preset(&opt_lzma2, 0));

	lzma_filter filters[3] = {
		{ .id = LZMA_FILTER_POWERPC, .options = NULL },
		{ .id = LZMA_FILTER_LZMA2, .options = &opt_lzma2 },
		{ .id = LZMA_VLI_UNKNOWN, .options = NULL },
	};

	expect(lzma_stream_buffer_encode(filters, LZMA_CHECK_CRC32, NULL,
			in, sizeof(in),
			compressed, &compressed_size, sizeof(compressed))
			== LZMA_OK);
}


static void
decompress(void)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	expect(lzma_stream_decoder(&strm, 10 << 20, 0) == LZMA_OK);

	strm.next_in = compressed;
	strm.next_out = out;

	while (true) {
		if (strm.total_in < compressed_size)
			strm.avail_in = 1;

		const lzma_ret ret = lzma_code(&strm, LZMA_RUN);
		if (ret == LZMA_STREAM_END) {
			expect(strm.total_in == compressed_size);
			expect(strm.total_out == sizeof(in));
			lzma_end(&strm);
			return;
		}

		expect(ret == LZMA_OK);

		if (strm.total_out < sizeof(in))
			strm.avail_out = 1;
	}
}


static void
decompress_empty(void)
{
	// An empty file with one Block using PowerPC BCJ and LZMA2.
	static const uint8_t empty_bcj_lzma2[] = {
		0xFD, 0x37, 0x7A, 0x58, 0x5A, 0x00, 0x00, 0x01,
		0x69, 0x22, 0xDE, 0x36, 0x02, 0x01, 0x05, 0x00,
		0x21, 0x01, 0x00, 0x00, 0x7F, 0xE0, 0xF1, 0xC8,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x11, 0x00, 0x3B, 0x96, 0x5F, 0x73,
		0x90, 0x42, 0x99, 0x0D, 0x01, 0x00, 0x00, 0x00,
		0x00, 0x01, 0x59, 0x5A
	};

	// Decompress without giving any output space.
	uint64_t memlimit = 1 << 20;
	size_t in_pos = 0;
	size_t out_pos = 0;
	expect(lzma_stream_buffer_decode(&memlimit, 0, NULL,
			empty_bcj_lzma2, &in_pos, sizeof(empty_bcj_lzma2),
			out, &out_pos, 0) == LZMA_OK);
	expect(in_pos == sizeof(empty_bcj_lzma2));
	expect(out_pos == 0);
}


extern int
main(void)
{
	compress();
	decompress();
	decompress_empty();
	return 0;
}
