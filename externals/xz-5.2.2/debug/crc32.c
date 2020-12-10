///////////////////////////////////////////////////////////////////////////////
//
/// \file       crc32.c
/// \brief      Primitive CRC32 calculation tool
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


int
main(void)
{
	uint32_t crc = 0;

	do {
		uint8_t buf[BUFSIZ];
		const size_t size = fread(buf, 1, sizeof(buf), stdin);
		crc = lzma_crc32(buf, size, crc);
	} while (!ferror(stdin) && !feof(stdin));

	//printf("%08" PRIX32 "\n", crc);

	// I want it little endian so it's easy to work with hex editor.
	printf("%02" PRIX32 " ", crc & 0xFF);
	printf("%02" PRIX32 " ", (crc >> 8) & 0xFF);
	printf("%02" PRIX32 " ", (crc >> 16) & 0xFF);
	printf("%02" PRIX32 " ", crc >> 24);
	printf("\n");

	return 0;
}
