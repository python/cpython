/*
 * _cp950.c: the CP950 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _cp950.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(big5)
ENCMAP(cp950ext)
DECMAP(big5)
DECMAP(cp950ext)

ENCODER(cp950)
{
    while (inleft > 0) {
        Py_UNICODE  c = IN1;
        DBCHAR      code;

        if (c < 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        RESERVE_OUTBUF(2)
        TRYMAP_ENC(cp950ext, code, c);
        else TRYMAP_ENC(big5, code, c);
        else return 1;

        OUT1(code >> 8)
        OUT2(code & 0xFF)
        NEXT(1, 2)
    }

    return 0;
}

DECODER(cp950)
{
    while (inleft > 0) {
        unsigned char    c = IN1;

        RESERVE_OUTBUF(1)

        if (c < 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }

        RESERVE_INBUF(2)

        TRYMAP_DEC(cp950ext, **outbuf, c, IN2);
        else TRYMAP_DEC(big5, **outbuf, c, IN2);
        else return 2;

        NEXT(2, 1)
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(cp950)
    MAPOPEN(zh_TW)
        IMPORTMAP_ENCDEC(big5)
        IMPORTMAP_ENCDEC(cp950ext)
    MAPCLOSE()
END_CODEC_REGISTRY(cp950)
