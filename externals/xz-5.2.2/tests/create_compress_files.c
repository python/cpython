///////////////////////////////////////////////////////////////////////////////
//
/// \file       create_compress_files.c
/// \brief      Creates bunch of test files to be compressed
///
/// Using a test file generator program saves space in the source code
/// package considerably.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"
#include <stdio.h>


// Avoid re-creating the test files every time the tests are run.
#define create_test(name) \
do { \
	if (!file_exists("compress_generated_" #name)) { \
		FILE *file = file_create("compress_generated_" #name); \
		write_ ## name(file); \
		file_finish(file, "compress_generated_" #name); \
	} \
} while (0)


static bool
file_exists(const char *filename)
{
	// Trying to be somewhat portable by avoiding stat().
	FILE *file = fopen(filename, "rb");
	bool ret;

	if (file != NULL) {
		fclose(file);
		ret = true;
	} else {
		ret = false;
	}

	return ret;
}


static FILE *
file_create(const char *filename)
{
	FILE *file = fopen(filename, "wb");

	if (file == NULL) {
		perror(filename);
		exit(1);
	}

	return file;
}


static void
file_finish(FILE *file, const char *filename)
{
	const bool ferror_fail = ferror(file);
	const bool fclose_fail = fclose(file);

	if (ferror_fail || fclose_fail) {
		perror(filename);
		exit(1);
	}
}


// File that repeats "abc\n" a few thousand times. This is targeted
// especially at Subblock filter's run-length encoder.
static void
write_abc(FILE *file)
{
	for (size_t i = 0; i < 12345; ++i)
		if (fwrite("abc\n", 4, 1, file) != 1)
			exit(1);
}


// File that doesn't compress. We always use the same random seed to
// generate identical files on all systems.
static void
write_random(FILE *file)
{
	uint32_t n = 5;

	for (size_t i = 0; i < 123456; ++i) {
		n = 101771 * n + 71777;

		putc((uint8_t)(n), file);
		putc((uint8_t)(n >> 8), file);
		putc((uint8_t)(n >> 16), file);
		putc((uint8_t)(n >> 24), file);
	}
}


// Text file
static void
write_text(FILE *file)
{
	static const char *lorem[] = {
		"Lorem", "ipsum", "dolor", "sit", "amet,", "consectetur",
		"adipisicing", "elit,", "sed", "do", "eiusmod", "tempor",
		"incididunt", "ut", "labore", "et", "dolore", "magna",
		"aliqua.", "Ut", "enim", "ad", "minim", "veniam,", "quis",
		"nostrud", "exercitation", "ullamco", "laboris", "nisi",
		"ut", "aliquip", "ex", "ea", "commodo", "consequat.",
		"Duis", "aute", "irure", "dolor", "in", "reprehenderit",
		"in", "voluptate", "velit", "esse", "cillum", "dolore",
		"eu", "fugiat", "nulla", "pariatur.", "Excepteur", "sint",
		"occaecat", "cupidatat", "non", "proident,", "sunt", "in",
		"culpa", "qui", "officia", "deserunt", "mollit", "anim",
		"id", "est", "laborum."
	};

	// Let the first paragraph be the original text.
	for (size_t w = 0; w < ARRAY_SIZE(lorem); ++w) {
		fprintf(file, "%s ", lorem[w]);

		if (w % 7 == 6)
			fprintf(file, "\n");
	}

	// The rest shall be (hopefully) meaningless combinations of
	// the same words.
	uint32_t n = 29;

	for (size_t p = 0; p < 500; ++p) {
		fprintf(file, "\n\n");

		for (size_t w = 0; w < ARRAY_SIZE(lorem); ++w) {
			n = 101771 * n + 71777;

			fprintf(file, "%s ", lorem[n % ARRAY_SIZE(lorem)]);

			if (w % 7 == 6)
				fprintf(file, "\n");
		}
	}
}


int
main(void)
{
	create_test(abc);
	create_test(random);
	create_test(text);
	return 0;
}
