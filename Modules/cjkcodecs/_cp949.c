/*
 * _cp949.c: the CP949 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _cp949.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(cp949)
DECMAP(ksx1001)
DECMAP(cp949ext)

ENCODER(cp949)
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
        TRYMAP_ENC(cp949, code, c);
        else return 1;

        OUT1((code >> 8) | 0x80)
        if (code & 0x8000)
            OUT2(code & 0xFF) /* MSB set: CP949 */
        else
            OUT2((code & 0xFF) | 0x80) /* MSB unset: ks x 1001 */
        NEXT(1, 2)
    }

    return 0;
}

DECODER(cp949)
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
        TRYMAP_DEC(ksx1001, **outbuf, c ^ 0x80, IN2 ^ 0x80);
        else TRYMAP_DEC(cp949ext, **outbuf, c, IN2);
        else return 2;

        NEXT(2, 1)
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(cp949)
    MAPOPEN(ko_KR)
        IMPORTMAP_DEC(ksx1001)
        IMPORTMAP_DEC(cp949ext)
        IMPORTMAP_ENC(cp949)
    MAPCLOSE()
END_CODEC_REGISTRY(cp949)
