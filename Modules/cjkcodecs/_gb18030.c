/*
 * _gb18030.c: the GB18030 codec
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _gb18030.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"
#include "tweak_gbk.h"
#include "map_gb18030uni.h"

ENCMAP(gbcommon)
ENCMAP(gb18030ext)
DECMAP(gb2312)
DECMAP(gbkext)
DECMAP(gb18030ext)

ENCODER(gb18030)
{
    while (inleft > 0) {
        ucs4_t  c = IN1;
        DBCHAR  code;

        if (c < 0x80) {
            WRITE1(c)
            NEXT(1, 1)
            continue;
        }

        DECODE_SURROGATE(c)
        if (c > 0x10FFFF)
#if Py_UNICODE_SIZE == 2
            return 2; /* surrogates pair */
#else
            return 1;
#endif
        else if (c >= 0x10000) {
            ucs4_t   tc = c - 0x10000;

            RESERVE_OUTBUF(4)

            OUT4((unsigned char)(tc % 10) + 0x30)
            tc /= 10;
            OUT3((unsigned char)(tc % 126) + 0x81)
            tc /= 126;
            OUT2((unsigned char)(tc % 10) + 0x30)
            tc /= 10;
            OUT1((unsigned char)(tc + 0x90))

#if Py_UNICODE_SIZE == 2
            NEXT(2, 4) /* surrogates pair */
#else
            NEXT(1, 4)
#endif
            continue;
        }

        RESERVE_OUTBUF(2)

        GBK_PREENCODE(c, code)
        else TRYMAP_ENC(gbcommon, code, c);
        else TRYMAP_ENC(gb18030ext, code, c);
        else {
            const struct _gb18030_to_unibmp_ranges  *utrrange;

            RESERVE_OUTBUF(4)

            for (utrrange = gb18030_to_unibmp_ranges;
                 utrrange->first != 0;
                 utrrange++)
                if (utrrange->first <= c && c <= utrrange->last) {
                    Py_UNICODE   tc;

                    tc = c - utrrange->first + utrrange->base;

                    OUT4((unsigned char)(tc % 10) + 0x30)
                    tc /= 10;
                    OUT3((unsigned char)(tc % 126) + 0x81)
                    tc /= 126;
                    OUT2((unsigned char)(tc % 10) + 0x30)
                    tc /= 10;
                    OUT1((unsigned char)tc + 0x81)

                    NEXT(1, 4)
                    break;
                }

            if (utrrange->first == 0) {
                PyErr_SetString(PyExc_RuntimeError,
                                "unicode mapping invalid");
                return 1;
            }
            continue;
        }

        OUT1((code >> 8) | 0x80)
        if (code & 0x8000)
            OUT2((code & 0xFF)) /* MSB set: GBK or GB18030ext */
        else
            OUT2((code & 0xFF) | 0x80) /* MSB unset: GB2312 */

        NEXT(1, 2)
    }

    return 0;
}

DECODER(gb18030)
{
    while (inleft > 0) {
        unsigned char    c = IN1, c2;

        RESERVE_OUTBUF(1)

        if (c < 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }

        RESERVE_INBUF(2)

        c2 = IN2;
        if (c2 >= 0x30 && c2 <= 0x39) { /* 4 bytes seq */
            const struct _gb18030_to_unibmp_ranges *utr;
            unsigned char    c3, c4;
            ucs4_t           lseq;

            RESERVE_INBUF(4)
            c3 = IN3;
            c4 = IN4;
            if (c < 0x81 || c3 < 0x81 || c4 < 0x30 || c4 > 0x39)
                return 4;
            c -= 0x81;  c2 -= 0x30;
            c3 -= 0x81; c4 -= 0x30;

            if (c < 4) { /* U+0080 - U+FFFF */
                lseq = ((ucs4_t)c * 10 + c2) * 1260 +
                        (ucs4_t)c3 * 10 + c4;
                if (lseq < 39420) {
                    for (utr = gb18030_to_unibmp_ranges;
                         lseq >= (utr + 1)->base;
                         utr++) ;
                    OUT1(utr->first - utr->base + lseq)
                    NEXT(4, 1)
                    continue;
                }
            }
            else if (c >= 15) { /* U+10000 - U+10FFFF */
                lseq = 0x10000 + (((ucs4_t)c-15) * 10 + c2) * 1260 +
                        (ucs4_t)c3 * 10 + c4;
                if (lseq <= 0x10FFFF) {
                    PUTUCS4(lseq);
                    NEXT_IN(4)
                    continue;
                }
            }
            return 4;
        }

        GBK_PREDECODE(c, c2, **outbuf)
        else TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, c2 ^ 0x80);
        else TRYMAP_DEC(gbkext, **outbuf, c, c2);
        else TRYMAP_DEC(gb18030ext, **outbuf, c, c2);
        else return 2;

        NEXT(2, 1)
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(gb18030)
    MAPOPEN(zh_CN)
        IMPORTMAP_DEC(gb2312)
        IMPORTMAP_DEC(gbkext)
        IMPORTMAP_ENC(gbcommon)
        IMPORTMAP_ENCDEC(gb18030ext)
    MAPCLOSE()
END_CODEC_REGISTRY(gb18030)
