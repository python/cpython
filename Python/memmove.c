
/* A perhaps slow but I hope correct implementation of memmove */

extern char *memcpy(char *, char *, int);

char *
memmove(char *dst, char *src, int n)
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
