/*
 * _euc_kr.c: the EUC-KR codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _euc_kr.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(cp949)
DECMAP(ksx1001)

ENCODER(euc_kr)
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
        TRYMAP_ENC(cp949, code, c);
        else return 1;

        if (code & 0x8000) /* MSB set: CP949 */
            return 1;

        OUT1((code >> 8) | 0x80)
        OUT2((code & 0xFF) | 0x80)
        NEXT(1, 2)
    }

    return 0;
}

DECODER(euc_kr)
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

        TRYMAP_DEC(ksx1001, **outbuf, c ^ 0x80, IN2 ^ 0x80) {
            NEXT(2, 1)
        } else return 2;

    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(euc_kr)
    MAPOPEN(ko_KR)
        IMPORTMAP_DEC(ksx1001)
        IMPORTMAP_ENC(cp949)
    MAPCLOSE()
END_CODEC_REGISTRY(euc_kr)
