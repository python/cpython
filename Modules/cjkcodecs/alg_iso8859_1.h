/*
 * alg_iso8859_1.c: Encoder/Decoder macro for ISO8859-1
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: alg_iso8859_1.h,v 1.3 2003/12/31 05:46:55 perky Exp $
 */

#define ISO8859_1_ENCODE(c, assi)                               \
    if ((c) <= 0xff) (assi) = (c);

#define ISO8859_1_DECODE(c, assi)                               \
    if (1/*(c) <= 0xff*/) (assi) = (c);
