///////////////////////////////////////////////////////////////////////////////
//
/// \file       test_filter_flags.c
/// \brief      Tests Filter Flags coders
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "tests.h"


static uint8_t buffer[4096];
static lzma_filter known_flags;
static lzma_filter decoded_flags;
static lzma_stream strm = LZMA_STREAM_INIT;


static bool
encode(uint32_t known_size)
{
	memcrap(buffer, sizeof(buffer));

	uint32_t tmp;
	if (lzma_filter_flags_size(&tmp, &known_flags) != LZMA_OK)
		return true;

	if (tmp != known_size)
		return true;

	size_t out_pos = 0;
	if (lzma_filter_flags_encode(&known_flags,
			buffer, &out_pos, known_size) != LZMA_OK)
		return true;

	if (out_pos != known_size)
		return true;

	return false;
}


static bool
decode_ret(uint32_t known_size, lzma_ret expected_ret)
{
	memcrap(&decoded_flags, sizeof(decoded_flags));

	size_t pos = 0;
	if (lzma_filter_flags_decode(&decoded_flags, NULL,
				buffer, &pos, known_size) != expected_ret
			|| pos != known_size)
		return true;

	return false;
}


static bool
decode(uint32_t known_size)
{
	if (decode_ret(known_size, LZMA_OK))
		return true;

	if (known_flags.id != decoded_flags.id)
		return true;

	return false;
}


#if defined(HAVE_ENCODER_X86) && defined(HAVE_DECODER_X86)
static void
test_bcj(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_X86;
	known_flags.options = NULL;

	expect(!encode(2));
	expect(!decode(2));
	expect(decoded_flags.options == NULL);

	// Test 2
	lzma_options_bcj options;
	options.start_offset = 0;
	known_flags.options = &options;
	expect(!encode(2));
	expect(!decode(2));
	expect(decoded_flags.options == NULL);

	// Test 3
	options.start_offset = 123456;
	known_flags.options = &options;
	expect(!encode(6));
	expect(!decode(6));
	expect(decoded_flags.options != NULL);

	lzma_options_bcj *decoded = decoded_flags.options;
	expect(decoded->start_offset == options.start_offset);

	free(decoded);
}
#endif


#if defined(HAVE_ENCODER_DELTA) && defined(HAVE_DECODER_DELTA)
static void
test_delta(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_DELTA;
	known_flags.options = NULL;
	expect(encode(99));

	// Test 2
	lzma_options_delta options = {
		.type = LZMA_DELTA_TYPE_BYTE,
		.dist = 0
	};
	known_flags.options = &options;
	expect(encode(99));

	// Test 3
	options.dist = LZMA_DELTA_DIST_MIN;
	expect(!encode(3));
	expect(!decode(3));
	expect(((lzma_options_delta *)(decoded_flags.options))->dist
			== options.dist);

	free(decoded_flags.options);

	// Test 4
	options.dist = LZMA_DELTA_DIST_MAX;
	expect(!encode(3));
	expect(!decode(3));
	expect(((lzma_options_delta *)(decoded_flags.options))->dist
			== options.dist);

	free(decoded_flags.options);

	// Test 5
	options.dist = LZMA_DELTA_DIST_MAX + 1;
	expect(encode(99));
}
#endif

/*
#ifdef HAVE_FILTER_LZMA
static void
validate_lzma(void)
{
	const lzma_options_lzma *known = known_flags.options;
	const lzma_options_lzma *decoded = decoded_flags.options;

	expect(known->dictionary_size <= decoded->dictionary_size);

	if (known->dictionary_size == 1)
		expect(decoded->dictionary_size == 1);
	else
		expect(known->dictionary_size + known->dictionary_size / 2
				> decoded->dictionary_size);

	expect(known->literal_context_bits == decoded->literal_context_bits);
	expect(known->literal_pos_bits == decoded->literal_pos_bits);
	expect(known->pos_bits == decoded->pos_bits);
}


static void
test_lzma(void)
{
	// Test 1
	known_flags.id = LZMA_FILTER_LZMA1;
	known_flags.options = NULL;
	expect(encode(99));

	// Test 2
	lzma_options_lzma options = {
		.dictionary_size = 0,
		.literal_context_bits = 0,
		.literal_pos_bits = 0,
		.pos_bits = 0,
		.preset_dictionary = NULL,
		.preset_dictionary_size = 0,
		.mode = LZMA_MODE_INVALID,
		.fast_bytes = 0,
		.match_finder = LZMA_MF_INVALID,
		.match_finder_cycles = 0,
	};

	// Test 3 (empty dictionary not allowed)
	known_flags.options = &options;
	expect(encode(99));

	// Test 4 (brute-force test some valid dictionary sizes)
	options.dictionary_size = LZMA_DICTIONARY_SIZE_MIN;
	while (options.dictionary_size != LZMA_DICTIONARY_SIZE_MAX) {
		if (++options.dictionary_size == 5000)
			options.dictionary_size = LZMA_DICTIONARY_SIZE_MAX - 5;

		expect(!encode(4));
		expect(!decode(4));
		validate_lzma();

		free(decoded_flags.options);
	}

	// Test 5 (too big dictionary size)
	options.dictionary_size = LZMA_DICTIONARY_SIZE_MAX + 1;
	expect(encode(99));

	// Test 6 (brute-force test lc/lp/pb)
	options.dictionary_size = LZMA_DICTIONARY_SIZE_MIN;
	for (uint32_t lc = LZMA_LITERAL_CONTEXT_BITS_MIN;
			lc <= LZMA_LITERAL_CONTEXT_BITS_MAX; ++lc) {
		for (uint32_t lp = LZMA_LITERAL_POS_BITS_MIN;
				lp <= LZMA_LITERAL_POS_BITS_MAX; ++lp) {
			for (uint32_t pb = LZMA_POS_BITS_MIN;
					pb <= LZMA_POS_BITS_MAX; ++pb) {
				if (lc + lp > LZMA_LITERAL_BITS_MAX)
					continue;

				options.literal_context_bits = lc;
				options.literal_pos_bits = lp;
				options.pos_bits = pb;

				expect(!encode(4));
				expect(!decode(4));
				validate_lzma();

				free(decoded_flags.options);
			}
		}
	}
}
#endif
*/

int
main(void)
{
#if defined(HAVE_ENCODER_X86) && defined(HAVE_DECODER_X86)
	test_bcj();
#endif
#if defined(HAVE_ENCODER_DELTA) && defined(HAVE_DECODER_DELTA)
	test_delta();
#endif
// #ifdef HAVE_FILTER_LZMA
// 	test_lzma();
// #endif

	lzma_end(&strm);

	return 0;
}
