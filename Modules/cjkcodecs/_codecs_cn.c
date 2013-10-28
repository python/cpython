/*
 * _codecs_cn.c: Codecs collection for Mainland Chinese encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#include "cjkcodecs.h"
#include "mappings_cn.h"

/**
 * hz is predefined as 100 on AIX. So we undefine it to avoid
 * conflict against hz codec's.
 */
#ifdef _AIX
#undef hz
#endif

/* GBK and GB2312 map differently in few codepoints that are listed below:
 *
 *              gb2312                          gbk
 * A1A4         U+30FB KATAKANA MIDDLE DOT      U+00B7 MIDDLE DOT
 * A1AA         U+2015 HORIZONTAL BAR           U+2014 EM DASH
 * A844         undefined                       U+2015 HORIZONTAL BAR
 */

#define GBK_DECODE(dc1, dc2, writer)                                \
    if ((dc1) == 0xa1 && (dc2) == 0xaa) {                           \
        OUTCHAR(0x2014);                                            \
    }                                                               \
    else if ((dc1) == 0xa8 && (dc2) == 0x44) {                      \
        OUTCHAR(0x2015);                                            \
    }                                                               \
    else if ((dc1) == 0xa1 && (dc2) == 0xa4) {                      \
        OUTCHAR(0x00b7);                                            \
    }                                                               \
    else if (TRYMAP_DEC(gb2312, decoded, dc1 ^ 0x80, dc2 ^ 0x80)) { \
        OUTCHAR(decoded);                                           \
    }                                                               \
    else if (TRYMAP_DEC(gbkext, decoded, dc1, dc2)) {               \
        OUTCHAR(decoded);                                           \
    }

#define GBK_ENCODE(code, assi)                                         \
    if ((code) == 0x2014) {                                            \
        (assi) = 0xa1aa;                                               \
    } else if ((code) == 0x2015) {                                     \
        (assi) = 0xa844;                                               \
    } else if ((code) == 0x00b7) {                                     \
        (assi) = 0xa1a4;                                               \
    } else if ((code) != 0x30fb && TRYMAP_ENC(gbcommon, assi, code)) { \
        ;                                                              \
    }

/*
 * GB2312 codec
 */

ENCODER(gb2312)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;

        if (c < 0x80) {
            WRITEBYTE1((unsigned char)c);
            NEXT(1, 1);
            continue;
        }

        if (c > 0xFFFF)
            return 1;

        REQUIRE_OUTBUF(2);
        if (TRYMAP_ENC(gbcommon, code, c))
            ;
        else
            return 1;

        if (code & 0x8000) /* MSB set: GBK */
            return 1;

        OUTBYTE1((code >> 8) | 0x80);
        OUTBYTE2((code & 0xFF) | 0x80);
        NEXT(1, 2);
    }

    return 0;
}

DECODER(gb2312)
{
    while (inleft > 0) {
        unsigned char c = **inbuf;
        Py_UCS4 decoded;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2);
        if (TRYMAP_DEC(gb2312, decoded, c ^ 0x80, INBYTE2 ^ 0x80)) {
            OUTCHAR(decoded);
            NEXT_IN(2);
        }
        else
            return 1;
    }

    return 0;
}


/*
 * GBK codec
 */

ENCODER(gbk)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;

        if (c < 0x80) {
            WRITEBYTE1((unsigned char)c);
            NEXT(1, 1);
            continue;
        }

        if (c > 0xFFFF)
            return 1;

        REQUIRE_OUTBUF(2);

        GBK_ENCODE(c, code)
        else
            return 1;

        OUTBYTE1((code >> 8) | 0x80);
        if (code & 0x8000)
            OUTBYTE2((code & 0xFF)); /* MSB set: GBK */
        else
            OUTBYTE2((code & 0xFF) | 0x80); /* MSB unset: GB2312 */
        NEXT(1, 2);
    }

    return 0;
}

DECODER(gbk)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_UCS4 decoded;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2);

        GBK_DECODE(c, INBYTE2, writer)
        else
            return 1;

        NEXT_IN(2);
    }

    return 0;
}


/*
 * GB18030 codec
 */

ENCODER(gb18030)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;

        if (c < 0x80) {
            WRITEBYTE1(c);
            NEXT(1, 1);
            continue;
        }

        if (c >= 0x10000) {
            Py_UCS4 tc = c - 0x10000;
            assert (c <= 0x10FFFF);

            REQUIRE_OUTBUF(4);

            OUTBYTE4((unsigned char)(tc % 10) + 0x30);
            tc /= 10;
            OUTBYTE3((unsigned char)(tc % 126) + 0x81);
            tc /= 126;
            OUTBYTE2((unsigned char)(tc % 10) + 0x30);
            tc /= 10;
            OUTBYTE1((unsigned char)(tc + 0x90));

            NEXT(1, 4);
            continue;
        }

        REQUIRE_OUTBUF(2);

        GBK_ENCODE(c, code)
        else if (TRYMAP_ENC(gb18030ext, code, c))
            ;
        else {
            const struct _gb18030_to_unibmp_ranges *utrrange;

            REQUIRE_OUTBUF(4);

            for (utrrange = gb18030_to_unibmp_ranges;
                 utrrange->first != 0;
                 utrrange++)
                if (utrrange->first <= c &&
                    c <= utrrange->last) {
                    Py_UCS4 tc;

                    tc = c - utrrange->first +
                         utrrange->base;

                    OUTBYTE4((unsigned char)(tc % 10) + 0x30);
                    tc /= 10;
                    OUTBYTE3((unsigned char)(tc % 126) + 0x81);
                    tc /= 126;
                    OUTBYTE2((unsigned char)(tc % 10) + 0x30);
                    tc /= 10;
                    OUTBYTE1((unsigned char)tc + 0x81);

                    NEXT(1, 4);
                    break;
                }

            if (utrrange->first == 0)
                return 1;
            continue;
        }

        OUTBYTE1((code >> 8) | 0x80);
        if (code & 0x8000)
            OUTBYTE2((code & 0xFF)); /* MSB set: GBK or GB18030ext */
        else
            OUTBYTE2((code & 0xFF) | 0x80); /* MSB unset: GB2312 */

        NEXT(1, 2);
    }

    return 0;
}

DECODER(gb18030)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1, c2;
        Py_UCS4 decoded;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2);

        c2 = INBYTE2;
        if (c2 >= 0x30 && c2 <= 0x39) { /* 4 bytes seq */
            const struct _gb18030_to_unibmp_ranges *utr;
            unsigned char c3, c4;
            Py_UCS4 lseq;

            REQUIRE_INBUF(4);
            c3 = INBYTE3;
            c4 = INBYTE4;
            if (c < 0x81 || c3 < 0x81 || c4 < 0x30 || c4 > 0x39)
                return 1;
            c -= 0x81;  c2 -= 0x30;
            c3 -= 0x81; c4 -= 0x30;

            if (c < 4) { /* U+0080 - U+FFFF */
                lseq = ((Py_UCS4)c * 10 + c2) * 1260 +
                    (Py_UCS4)c3 * 10 + c4;
                if (lseq < 39420) {
                    for (utr = gb18030_to_unibmp_ranges;
                         lseq >= (utr + 1)->base;
                         utr++) ;
                    OUTCHAR(utr->first - utr->base + lseq);
                    NEXT_IN(4);
                    continue;
                }
            }
            else if (c >= 15) { /* U+10000 - U+10FFFF */
                lseq = 0x10000 + (((Py_UCS4)c-15) * 10 + c2)
                    * 1260 + (Py_UCS4)c3 * 10 + c4;
                if (lseq <= 0x10FFFF) {
                    OUTCHAR(lseq);
                    NEXT_IN(4);
                    continue;
                }
            }
            return 1;
        }

        GBK_DECODE(c, c2, writer)
        else if (TRYMAP_DEC(gb18030ext, decoded, c, c2))
            OUTCHAR(decoded);
        else
            return 1;

        NEXT_IN(2);
    }

    return 0;
}


/*
 * HZ codec
 */

ENCODER_INIT(hz)
{
    state->i = 0;
    return 0;
}

ENCODER_RESET(hz)
{
    if (state->i != 0) {
        WRITEBYTE2('~', '}');
        state->i = 0;
        NEXT_OUT(2);
    }
    return 0;
}

ENCODER(hz)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;

        if (c < 0x80) {
            if (state->i == 0) {
                WRITEBYTE1((unsigned char)c);
                NEXT(1, 1);
            }
            else {
                WRITEBYTE3('~', '}', (unsigned char)c);
                NEXT(1, 3);
                state->i = 0;
            }
            continue;
        }

        if (c > 0xFFFF)
            return 1;

        if (TRYMAP_ENC(gbcommon, code, c))
            ;
        else
            return 1;

        if (code & 0x8000) /* MSB set: GBK */
            return 1;

        if (state->i == 0) {
            WRITEBYTE4('~', '{', code >> 8, code & 0xff);
            NEXT(1, 4);
            state->i = 1;
        }
        else {
            WRITEBYTE2(code >> 8, code & 0xff);
            NEXT(1, 2);
        }
    }

    return 0;
}

DECODER_INIT(hz)
{
    state->i = 0;
    return 0;
}

DECODER_RESET(hz)
{
    state->i = 0;
    return 0;
}

DECODER(hz)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_UCS4 decoded;

        if (c == '~') {
            unsigned char c2 = INBYTE2;

            REQUIRE_INBUF(2);
            if (c2 == '~') {
                OUTCHAR('~');
                NEXT_IN(2);
                continue;
            }
            else if (c2 == '{' && state->i == 0)
                state->i = 1; /* set GB */
            else if (c2 == '}' && state->i == 1)
                state->i = 0; /* set ASCII */
            else if (c2 == '\n')
                ; /* line-continuation */
            else
                return 1;
            NEXT_IN(2);
            continue;
        }

        if (c & 0x80)
            return 1;

        if (state->i == 0) { /* ASCII mode */
            OUTCHAR(c);
            NEXT_IN(1);
        }
        else { /* GB mode */
            REQUIRE_INBUF(2);
            if (TRYMAP_DEC(gb2312, decoded, c, INBYTE2)) {
                OUTCHAR(decoded);
                NEXT_IN(2);
            }
            else
                return 1;
        }
    }

    return 0;
}


BEGIN_MAPPINGS_LIST
  MAPPING_DECONLY(gb2312)
  MAPPING_DECONLY(gbkext)
  MAPPING_ENCONLY(gbcommon)
  MAPPING_ENCDEC(gb18030ext)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS(gb2312)
  CODEC_STATELESS(gbk)
  CODEC_STATELESS(gb18030)
  CODEC_STATEFUL(hz)
END_CODECS_LIST

I_AM_A_MODULE_FOR(cn)
