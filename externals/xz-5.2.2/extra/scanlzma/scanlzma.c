/*
    scanlzma, scan for lzma compressed data in stdin and echo it to stdout.
    Copyright (C) 2006 Timo Lindfors

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
*/

/* Usage example:

   $ wget http://www.wifi-shop.cz/Files/produkty/wa2204/wa2204av1.4.1.zip
   $ unzip wa2204av1.4.1.zip
   $ gcc scanlzma.c -o scanlzma -Wall
   $ ./scanlzma 0 < WA2204-FW1.4.1/linux-1.4.bin | lzma -c -d | strings | grep -i "copyright"
   UpdateDD version 2.5, Copyright (C) 2005 Philipp Benner.
   Copyright (C) 2005 Philipp Benner.
   Copyright (C) 2005 Philipp Benner.
   mawk 1.3%s%s %s, Copyright (C) Michael D. Brennan
   # Copyright (C) 1998, 1999, 2001  Henry Spencer.
   ...

*/


/* LZMA compressed file format */
/* --------------------------- */
/* Offset Size Description */
/*   0     1   Special LZMA properties for compressed data */
/*   1     4   Dictionary size (little endian) */
/*   5     8   Uncompressed size (little endian). -1 means unknown size */
/*  13         Compressed data */

#define BUFSIZE 4096

int find_lzma_header(unsigned char *buf) {
	return (buf[0] < 0xE1
		&& buf[0] == 0x5d
		&& buf[4] < 0x20
		&& (memcmp (buf + 10 , "\x00\x00\x00", 3) == 0
		    || (memcmp (buf + 5, "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF", 8) == 0)));
}

int main(int argc, char *argv[]) {
	char buf[BUFSIZE];
	int ret, i, numlzma, blocks=0;

	if (argc != 2) {
		printf("usage: %s numlzma < infile | lzma -c -d > outfile\n"
		       "where numlzma is index of lzma file to extract, starting from zero.\n",
		       argv[0]);
		exit(1);
	}
	numlzma = atoi(argv[1]);

	for (;;) {
		/* Read data. */
		ret = fread(buf, BUFSIZE, 1, stdin);
		if (ret != 1)
			break;

		/* Scan for signature. */
		for (i = 0; i<BUFSIZE-23; i++) {
			if (find_lzma_header(buf+i) && numlzma-- <= 0) {
				fwrite(buf+i, (BUFSIZE-i), 1, stdout);
				for (;;) {
					int ch;
					ch = getchar();
					if (ch == EOF)
						exit(0);
					putchar(ch);
				}
			}
		}
		blocks++;
	}
	return 1;
}
