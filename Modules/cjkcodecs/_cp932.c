/*
 * _cp932.c: the CP932 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _cp932.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(jisxcommon)
ENCMAP(cp932ext)
DECMAP(jisx0208)
DECMAP(cp932ext)

ENCODER(cp932)
{
    while (inleft > 0) {
        Py_UNICODE       c = IN1;
        DBCHAR           code;
        unsigned char    c1, c2;

        if (c <= 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        } else if (c >= 0xff61 && c <= 0xff9f) {
            WRITE1(c - 0xfec0)
            NEXT(1, 1)
            continue;
        } else if (c >= 0xf8f0 && c <= 0xf8f3) {
            /* Windows compatability */
            RESERVE_OUTBUF(1)
            if (c == 0xf8f0)
                OUT1(0xa0)
            else
                OUT1(c - 0xfef1 + 0xfd)
            NEXT(1, 1)
            continue;
        }

        UCS4INVALID(c)
        RESERVE_OUTBUF(2)

        TRYMAP_ENC(cp932ext, code, c) {
            OUT1(code >> 8)
            OUT2(code & 0xff)
        } else TRYMAP_ENC(jisxcommon, code, c) {
            if (code & 0x8000) /* MSB set: JIS X 0212 */
                return 1;

            /* JIS X 0208 */
            c1 = code >> 8;
            c2 = code & 0xff;
            c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
            c1 = (c1 - 0x21) >> 1;
            OUT1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1)
            OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
        } else if (c >= 0xe000 && c < 0xe758) {
            /* User-defined area */
            c1 = (Py_UNICODE)(c - 0xe000) / 188;
            c2 = (Py_UNICODE)(c - 0xe000) % 188;
            OUT1(c1 + 0xf0)
            OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
        } else
            return 1;

        NEXT(1, 2)
    }

    return 0;
}

DECODER(cp932)
{
    while (inleft > 0) {
        unsigned char    c = IN1, c2;

        RESERVE_OUTBUF(1)
        if (c <= 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        } else if (c >= 0xa0 && c <= 0xdf) {
            if (c == 0xa0)
                OUT1(0xf8f0) /* half-width katakana */
            else
                OUT1(0xfec0 + c)
            NEXT(1, 1)
            continue;
        } else if (c >= 0xfd/* && c <= 0xff*/) {
            /* Windows compatibility */
            OUT1(0xf8f1 - 0xfd + c)
            NEXT(1, 1)
            continue;
        }

        RESERVE_INBUF(2)
        c2 = IN2;

        TRYMAP_DEC(cp932ext, **outbuf, c, c2);
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)) {
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 2;

            c = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c = (2 * c + (c2 < 0x5e ? 0 : 1) + 0x21);
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

            TRYMAP_DEC(jisx0208, **outbuf, c, c2);
            else return 2;
        } else if (c >= 0xf0 && c <= 0xf9) {
            if ((c2 >= 0x40 && c2 <= 0x7e) || (c2 >= 0x80 && c2 <= 0xfc))
                OUT1(0xe000 + 188 * (c - 0xf0) +
                       (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41))
            else
                return 2;
        } else
            return 2;

        NEXT(2, 1)
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(cp932)
    MAPOPEN(ja_JP)
        IMPORTMAP_DEC(jisx0208)
        IMPORTMAP_ENCDEC(cp932ext)
        IMPORTMAP_ENC(jisxcommon)
    MAPCLOSE()
END_CODEC_REGISTRY(cp932)
