/*
 * _big5.c: the Big5 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _big5.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(big5)
DECMAP(big5)

ENCODER(big5)
{
    while (inleft > 0) {
        Py_UNICODE  c = **inbuf;
        DBCHAR      code;

        if (c < 0x80) {
            RESERVE_OUTBUF(1)
            **outbuf = (unsigned char)c;
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        RESERVE_OUTBUF(2)

        TRYMAP_ENC(big5, code, c);
        else return 1;

        (*outbuf)[0] = code >> 8;
        (*outbuf)[1] = code & 0xFF;
        NEXT(1, 2)
    }

    return 0;
}

DECODER(big5)
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
        TRYMAP_DEC(big5, **outbuf, c, IN2) {
            NEXT(2, 1)
        } else return 2;
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(big5)
    MAPOPEN(zh_TW)
        IMPORTMAP_ENCDEC(big5)
    MAPCLOSE()
END_CODEC_REGISTRY(big5)
