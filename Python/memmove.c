/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* A perhaps slow but I hope correct implementation of memmove */

extern char *memcpy();

char *
memmove(dst, src, n)
	char *dst;
	char *src;
	int n;
{
	char *realdst = dst;
	if (n <= 0)
		return dst;
	if (src >= dst+n || dst >= src+n)
		return memcpy(dst, src, n);
	if (src > dst) {
		while (--n >= 0)
			*dst++ = *src++;
	}
	else if (src < dst) {
		src += n;
		dst += n;
		while (--n >= 0)
			*--dst = *--src;
	}
	return realdst;
}
