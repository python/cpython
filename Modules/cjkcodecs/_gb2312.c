/*
 * _gb2312.c: the GB2312 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _gb2312.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(gbcommon)
DECMAP(gb2312)

ENCODER(gb2312)
{
    while (inleft > 0) {
        Py_UNICODE  c = IN1;
        DBCHAR      code;

        if (c < 0x80) {
            WRITE1(c)
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        RESERVE_OUTBUF(2)
        TRYMAP_ENC(gbcommon, code, c);
        else return 1;

        if (code & 0x8000) /* MSB set: GBK */
            return 1;

        OUT1((code >> 8) | 0x80)
        OUT2((code & 0xFF) | 0x80)
        NEXT(1, 2)
    }

    return 0;
}

DECODER(gb2312)
{
    while (inleft > 0) {
        unsigned char    c = **inbuf;

        RESERVE_OUTBUF(1)

        if (c < 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }

        RESERVE_INBUF(2)
        TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, IN2 ^ 0x80) {
            NEXT(2, 1)
        } else return 2;
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(gb2312)
    MAPOPEN(zh_CN)
        IMPORTMAP_DEC(gb2312)
        IMPORTMAP_ENC(gbcommon)
    MAPCLOSE()
END_CODEC_REGISTRY(gb2312)
