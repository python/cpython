/*
 * _shift_jis.c: the SHIFT-JIS codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _shift_jis.c,v 1.4 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"
#include "alg_jisx0201.h"

ENCMAP(jisxcommon)
DECMAP(jisx0208)

ENCODER(shift_jis)
{
    while (inleft > 0) {
        Py_UNICODE       c = IN1;
        DBCHAR           code;
        unsigned char    c1, c2;

#ifdef STRICT_BUILD
        JISX0201_R_ENCODE(c, code)
#else
        if (c < 0x80) code = c;
        else if (c == 0x00a5) code = 0x5c; /* YEN SIGN */
        else if (c == 0x203e) code = 0x7e; /* OVERLINE */
#endif
        else JISX0201_K_ENCODE(c, code)
        else UCS4INVALID(c)
        else code = NOCHAR;

        if (code < 0x80 || (code >= 0xa1 && code <= 0xdf)) {
            RESERVE_OUTBUF(1)

            OUT1((unsigned char)code)
            NEXT(1, 1)
            continue;
        }

        RESERVE_OUTBUF(2)

        if (code == NOCHAR) {
            TRYMAP_ENC(jisxcommon, code, c);
#ifndef STRICT_BUILD
            else if (c == 0xff3c)
                code = 0x2140; /* FULL-WIDTH REVERSE SOLIDUS */
#endif
            else
                return 1;

            if (code & 0x8000) /* MSB set: JIS X 0212 */
                return 1;
        }

        c1 = code >> 8;
        c2 = code & 0xff;
        c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
        c1 = (c1 - 0x21) >> 1;
        OUT1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1)
        OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
        NEXT(1, 2)
    }

    return 0;
}

DECODER(shift_jis)
{
    while (inleft > 0) {
        unsigned char    c = IN1;

        RESERVE_OUTBUF(1)

#ifdef STRICT_BUILD
        JISX0201_R_DECODE(c, **outbuf)
#else
        if (c < 0x80) **outbuf = c;
#endif
        else JISX0201_K_DECODE(c, **outbuf)
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)) {
            unsigned char    c1, c2;

            RESERVE_INBUF(2)
            c2 = IN2;
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 2;

            c1 = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c1 = (2 * c1 + (c2 < 0x5e ? 0 : 1) + 0x21);
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

#ifndef STRICT_BUILD
            if (c1 == 0x21 && c2 == 0x40) {
                /* FULL-WIDTH REVERSE SOLIDUS */
                OUT1(0xff3c)
                NEXT(2, 1)
                continue;
            }
#endif
            TRYMAP_DEC(jisx0208, **outbuf, c1, c2) {
                NEXT(2, 1)
                continue;
            } else
                return 2;
        } else
            return 2;

        NEXT(1, 1) /* JIS X 0201 */
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(shift_jis)
    MAPOPEN(ja_JP)
        IMPORTMAP_DEC(jisx0208)
        IMPORTMAP_ENC(jisxcommon)
    MAPCLOSE()
END_CODEC_REGISTRY(shift_jis)
