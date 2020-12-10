///////////////////////////////////////////////////////////////////////////////
//
/// \file       known_sizes.c
/// \brief      Encodes .lzma Stream with sizes known in Block Header
///
/// The input file is encoded in RAM, and the known Compressed Size
/// and/or Uncompressed Size values are stored in the Block Header.
/// As of writing there's no such Stream encoder in liblzma.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"
#include "lzma.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <stdio.h>


// Support file sizes up to 1 MiB. We use this for output space too, so files
// close to 1 MiB had better compress at least a little or we have a buffer
// overflow.
#define BUFFER_SIZE (1U << 20)


int
main(void)
{
	// Allocate the buffers.
	uint8_t *in = malloc(BUFFER_SIZE);
	uint8_t *out = malloc(BUFFER_SIZE);
	if (in == NULL || out == NULL)
		return 1;

	// Fill the input buffer.
	const size_t in_size = fread(in, 1, BUFFER_SIZE, stdin);

	// Filter setup
	lzma_options_lzma opt_lzma;
	if (lzma_lzma_preset(&opt_lzma, 1))
		return 1;

	lzma_filter filters[] = {
		{
			.id = LZMA_FILTER_LZMA2,
			.options = &opt_lzma
		},
		{
			.id = LZMA_VLI_UNKNOWN
		}
	};

	lzma_block block = {
		.check = LZMA_CHECK_CRC32,
		.compressed_size = BUFFER_SIZE, // Worst case reserve
		.uncompressed_size = in_size,
		.filters = filters,
	};

	lzma_stream strm = LZMA_STREAM_INIT;
	if (lzma_block_encoder(&strm, &block) != LZMA_OK)
		return 1;

	// Reserve space for Stream Header and Block Header. We need to
	// calculate the size of the Block Header first.
	if (lzma_block_header_size(&block) != LZMA_OK)
		return 1;

	size_t out_size = LZMA_STREAM_HEADER_SIZE + block.header_size;

	strm.next_in = in;
	strm.avail_in = in_size;
	strm.next_out = out + out_size;
	strm.avail_out = BUFFER_SIZE - out_size;

	if (lzma_code(&strm, LZMA_FINISH) != LZMA_STREAM_END)
		return 1;

	out_size += strm.total_out;

	if (lzma_block_header_encode(&block, out + LZMA_STREAM_HEADER_SIZE)
			!= LZMA_OK)
		return 1;

	lzma_index *idx = lzma_index_init(NULL);
	if (idx == NULL)
		return 1;

	if (lzma_index_append(idx, NULL, block.header_size + strm.total_out,
			strm.total_in) != LZMA_OK)
		return 1;

	if (lzma_index_encoder(&strm, idx) != LZMA_OK)
		return 1;

	if (lzma_code(&strm, LZMA_RUN) != LZMA_STREAM_END)
		return 1;

	out_size += strm.total_out;

	lzma_end(&strm);

	lzma_index_end(idx, NULL);

	// Encode the Stream Header and Stream Footer. backwards_size is
	// needed only for the Stream Footer.
	lzma_stream_flags sf = {
		.backward_size = strm.total_out,
		.check = block.check,
	};

	if (lzma_stream_header_encode(&sf, out) != LZMA_OK)
		return 1;

	if (lzma_stream_footer_encode(&sf, out + out_size) != LZMA_OK)
		return 1;

	out_size += LZMA_STREAM_HEADER_SIZE;

	// Write out the file.
	fwrite(out, 1, out_size, stdout);

	return 0;
}
