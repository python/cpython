/*
 * alg_iso8859_7.c: Encoder/Decoder macro for ISO8859-7
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: alg_iso8859_7.h,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

/*
 * 0x2888fbc9 and 0xbffffd77 are magic number that indicates availability
 * of mapping for each differences. (0 and 0x2d0)
 */

#define ISO8859_7_ENCODE(c, assi)                               \
    if ((c) <= 0xa0) (assi) = (c);                              \
    else if ((c) < 0xc0 && (0x288f3bc9L & (1L << ((c)-0xa0))))  \
        (assi) = (c);                                           \
    else if ((c) >= 0x0384 && (c) <= 0x03ce && ((c) >= 0x03a4 ||\
             (0xbffffd77L & (1L << ((c)-0x0384)))))             \
        (assi) = (c) - 0x02d0;                                  \
    else if ((c)>>1 == 0x2018>>1) (assi) = (c) - 0x1f77;        \
    else if ((c) == 0x2015) (assi) = 0xaf;

#define ISO8859_7_DECODE(c, assi)                               \
    if ((c) < 0xa0) (assi) = (c);                               \
    else if ((c) < 0xc0 && (0x288f3bc9L & (1L << ((c)-0xa0))))  \
        (assi) = (c);                                           \
    else if ((c) >= 0xb4 && (c) <= 0xfe && ((c) >= 0xd4 ||      \
             (0xbffffd77L & (1L << ((c)-0xb4)))))               \
        (assi) = 0x02d0 + (c);                                  \
    else if ((c) == 0xa1) (assi) = 0x2018;                      \
    else if ((c) == 0xa2) (assi) = 0x2019;                      \
    else if ((c) == 0xaf) (assi) = 0x2015;
