/*
 * _gbk.c: the GBK codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _gbk.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"
#include "tweak_gbk.h"

ENCMAP(gbcommon)
DECMAP(gb2312)
DECMAP(gbkext)

ENCODER(gbk)
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

        GBK_PREENCODE(c, code)
        else TRYMAP_ENC(gbcommon, code, c);
        else return 1;

        OUT1((code >> 8) | 0x80)
        if (code & 0x8000)
            OUT2((code & 0xFF)) /* MSB set: GBK */
        else
            OUT2((code & 0xFF) | 0x80) /* MSB unset: GB2312 */
        NEXT(1, 2)
    }

    return 0;
}

DECODER(gbk)
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

        GBK_PREDECODE(c, IN2, **outbuf)
        else TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, IN2 ^ 0x80);
        else TRYMAP_DEC(gbkext, **outbuf, c, IN2);
        else return 2;

        NEXT(2, 1)
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(gbk)
    MAPOPEN(zh_CN)
        IMPORTMAP_DEC(gb2312)
        IMPORTMAP_DEC(gbkext)
        IMPORTMAP_ENC(gbcommon)
    MAPCLOSE()
END_CODEC_REGISTRY(gbk)
