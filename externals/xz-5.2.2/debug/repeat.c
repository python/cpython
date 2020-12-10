///////////////////////////////////////////////////////////////////////////////
//
/// \file       repeat.c
/// \brief      Repeats given string given times
///
/// This program can be useful when debugging run-length encoder in
/// the Subblock filter, especially the condition when repeat count
/// doesn't fit into 28-bit integer.
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#include "sysdefs.h"
#include <stdio.h>


int
main(int argc, char **argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s COUNT STRING\n", argv[0]);
		exit(1);
	}

	unsigned long long count = strtoull(argv[1], NULL, 10);
	const size_t size = strlen(argv[2]);

	while (count-- != 0)
		fwrite(argv[2], 1, size, stdout);

	return !!(ferror(stdout) || fclose(stdout));
}
