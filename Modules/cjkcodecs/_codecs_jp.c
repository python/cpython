/*
 * _codecs_jp.c: Codecs collection for Japanese encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#define USING_BINARY_PAIR_SEARCH
#define EMPBASE 0x20000

#include "cjkcodecs.h"
#include "mappings_jp.h"
#include "mappings_jisx0213_pair.h"
#include "alg_jisx0201.h"
#include "emu_jisx0213_2000.h"

/*
 * CP932 codec
 */

ENCODER(cp932)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;
        unsigned char c1, c2;

        if (c <= 0x80) {
            WRITEBYTE1((unsigned char)c);
            NEXT(1, 1);
            continue;
        }
        else if (c >= 0xff61 && c <= 0xff9f) {
            WRITEBYTE1(c - 0xfec0);
            NEXT(1, 1);
            continue;
        }
        else if (c >= 0xf8f0 && c <= 0xf8f3) {
            /* Windows compatibility */
            REQUIRE_OUTBUF(1);
            if (c == 0xf8f0)
                OUTBYTE1(0xa0);
            else
                OUTBYTE1(c - 0xf8f1 + 0xfd);
            NEXT(1, 1);
            continue;
        }

        if (c > 0xFFFF)
            return 1;
        REQUIRE_OUTBUF(2);

        if (TRYMAP_ENC(cp932ext, code, c)) {
            OUTBYTE1(code >> 8);
            OUTBYTE2(code & 0xff);
        }
        else if (TRYMAP_ENC(jisxcommon, code, c)) {
            if (code & 0x8000) /* MSB set: JIS X 0212 */
                return 1;

            /* JIS X 0208 */
            c1 = code >> 8;
            c2 = code & 0xff;
            c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
            c1 = (c1 - 0x21) >> 1;
            OUTBYTE1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1);
            OUTBYTE2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41);
        }
        else if (c >= 0xe000 && c < 0xe758) {
            /* User-defined area */
            c1 = (Py_UCS4)(c - 0xe000) / 188;
            c2 = (Py_UCS4)(c - 0xe000) % 188;
            OUTBYTE1(c1 + 0xf0);
            OUTBYTE2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41);
        }
        else
            return 1;

        NEXT(1, 2);
    }

    return 0;
}

DECODER(cp932)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1, c2;
        Py_UCS4 decoded;

        if (c <= 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }
        else if (c >= 0xa0 && c <= 0xdf) {
            if (c == 0xa0)
                OUTCHAR(0xf8f0); /* half-width katakana */
            else
                OUTCHAR(0xfec0 + c);
            NEXT_IN(1);
            continue;
        }
        else if (c >= 0xfd/* && c <= 0xff*/) {
            /* Windows compatibility */
            OUTCHAR(0xf8f1 - 0xfd + c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2);
        c2 = INBYTE2;

        if (TRYMAP_DEC(cp932ext, decoded, c, c2))
            OUTCHAR(decoded);
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)){
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 1;

            c = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c = (2 * c + (c2 < 0x5e ? 0 : 1) + 0x21);
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

            if (TRYMAP_DEC(jisx0208, decoded, c, c2))
                OUTCHAR(decoded);
            else
                return 1;
        }
        else if (c >= 0xf0 && c <= 0xf9) {
            if ((c2 >= 0x40 && c2 <= 0x7e) ||
                (c2 >= 0x80 && c2 <= 0xfc))
                OUTCHAR(0xe000 + 188 * (c - 0xf0) +
                    (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41));
            else
                return 1;
        }
        else
            return 1;

        NEXT_IN(2);
    }

    return 0;
}


/*
 * EUC-JIS-2004 codec
 */

ENCODER(euc_jis_2004)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;
        Py_ssize_t insize;

        if (c < 0x80) {
            WRITEBYTE1(c);
            NEXT(1, 1);
            continue;
        }

        insize = 1;

        if (c <= 0xFFFF) {
            EMULATE_JISX0213_2000_ENCODE_BMP(code, c)
            else if (TRYMAP_ENC(jisx0213_bmp, code, c)) {
                if (code == MULTIC) {
                    if (inlen - *inpos < 2) {
                        if (flags & MBENC_FLUSH) {
                            code = find_pairencmap(
                                (ucs2_t)c, 0,
                              jisx0213_pair_encmap,
                                JISX0213_ENCPAIRS);
                            if (code == DBCINV)
                                return 1;
                        }
                        else
                            return MBERR_TOOFEW;
                    }
                    else {
                        Py_UCS4 c2 = INCHAR2;
                        code = find_pairencmap(
                            (ucs2_t)c, c2,
                            jisx0213_pair_encmap,
                            JISX0213_ENCPAIRS);
                        if (code == DBCINV) {
                            code = find_pairencmap(
                                (ucs2_t)c, 0,
                              jisx0213_pair_encmap,
                                JISX0213_ENCPAIRS);
                            if (code == DBCINV)
                                return 1;
                        } else
                            insize = 2;
                    }
                }
            }
            else if (TRYMAP_ENC(jisxcommon, code, c))
                ;
            else if (c >= 0xff61 && c <= 0xff9f) {
                /* JIS X 0201 half-width katakana */
                WRITEBYTE2(0x8e, c - 0xfec0);
                NEXT(1, 2);
                continue;
            }
            else if (c == 0xff3c)
                /* F/W REVERSE SOLIDUS (see NOTES) */
                code = 0x2140;
            else if (c == 0xff5e)
                /* F/W TILDE (see NOTES) */
                code = 0x2232;
            else
                return 1;
        }
        else if (c >> 16 == EMPBASE >> 16) {
            EMULATE_JISX0213_2000_ENCODE_EMP(code, c)
            else if (TRYMAP_ENC(jisx0213_emp, code, c & 0xffff))
                ;
            else
                return insize;
        }
        else
            return insize;

        if (code & 0x8000) {
            /* Codeset 2 */
            WRITEBYTE3(0x8f, code >> 8, (code & 0xFF) | 0x80);
            NEXT(insize, 3);
        } else {
            /* Codeset 1 */
            WRITEBYTE2((code >> 8) | 0x80, (code & 0xFF) | 0x80);
            NEXT(insize, 2);
        }
    }

    return 0;
}

DECODER(euc_jis_2004)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_UCS4 code, decoded;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        if (c == 0x8e) {
            /* JIS X 0201 half-width katakana */
            unsigned char c2;

            REQUIRE_INBUF(2);
            c2 = INBYTE2;
            if (c2 >= 0xa1 && c2 <= 0xdf) {
                OUTCHAR(0xfec0 + c2);
                NEXT_IN(2);
            }
            else
                return 1;
        }
        else if (c == 0x8f) {
            unsigned char c2, c3;

            REQUIRE_INBUF(3);
            c2 = INBYTE2 ^ 0x80;
            c3 = INBYTE3 ^ 0x80;

            /* JIS X 0213 Plane 2 or JIS X 0212 (see NOTES) */
            EMULATE_JISX0213_2000_DECODE_PLANE2(writer, c2, c3)
            else if (TRYMAP_DEC(jisx0213_2_bmp, decoded, c2, c3))
                OUTCHAR(decoded);
            else if (TRYMAP_DEC(jisx0213_2_emp, code, c2, c3)) {
                OUTCHAR(EMPBASE | code);
                NEXT_IN(3);
                continue;
            }
            else if (TRYMAP_DEC(jisx0212, decoded, c2, c3))
                OUTCHAR(decoded);
            else
                return 1;
            NEXT_IN(3);
        }
        else {
            unsigned char c2;

            REQUIRE_INBUF(2);
            c ^= 0x80;
            c2 = INBYTE2 ^ 0x80;

            /* JIS X 0213 Plane 1 */
            EMULATE_JISX0213_2000_DECODE_PLANE1(writer, c, c2)
            else if (c == 0x21 && c2 == 0x40)
                OUTCHAR(0xff3c);
            else if (c == 0x22 && c2 == 0x32)
                OUTCHAR(0xff5e);
            else if (TRYMAP_DEC(jisx0208, decoded, c, c2))
                OUTCHAR(decoded);
            else if (TRYMAP_DEC(jisx0213_1_bmp, decoded, c, c2))
                OUTCHAR(decoded);
            else if (TRYMAP_DEC(jisx0213_1_emp, code, c, c2)) {
                OUTCHAR(EMPBASE | code);
                NEXT_IN(2);
                continue;
            }
            else if (TRYMAP_DEC(jisx0213_pair, code, c, c2)) {
                OUTCHAR2(code >> 16, code & 0xffff);
                NEXT_IN(2);
                continue;
            }
            else
                return 1;
            NEXT_IN(2);
        }
    }

    return 0;
}


/*
 * EUC-JP codec
 */

ENCODER(euc_jp)
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

        if (TRYMAP_ENC(jisxcommon, code, c))
            ;
        else if (c >= 0xff61 && c <= 0xff9f) {
            /* JIS X 0201 half-width katakana */
            WRITEBYTE2(0x8e, c - 0xfec0);
            NEXT(1, 2);
            continue;
        }
#ifndef STRICT_BUILD
        else if (c == 0xff3c) /* FULL-WIDTH REVERSE SOLIDUS */
            code = 0x2140;
        else if (c == 0xa5) { /* YEN SIGN */
            WRITEBYTE1(0x5c);
            NEXT(1, 1);
            continue;
        } else if (c == 0x203e) { /* OVERLINE */
            WRITEBYTE1(0x7e);
            NEXT(1, 1);
            continue;
        }
#endif
        else
            return 1;

        if (code & 0x8000) {
            /* JIS X 0212 */
            WRITEBYTE3(0x8f, code >> 8, (code & 0xFF) | 0x80);
            NEXT(1, 3);
        } else {
            /* JIS X 0208 */
            WRITEBYTE2((code >> 8) | 0x80, (code & 0xFF) | 0x80);
            NEXT(1, 2);
        }
    }

    return 0;
}

DECODER(euc_jp)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_UCS4 decoded;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        if (c == 0x8e) {
            /* JIS X 0201 half-width katakana */
            unsigned char c2;

            REQUIRE_INBUF(2);
            c2 = INBYTE2;
            if (c2 >= 0xa1 && c2 <= 0xdf) {
                OUTCHAR(0xfec0 + c2);
                NEXT_IN(2);
            }
            else
                return 1;
        }
        else if (c == 0x8f) {
            unsigned char c2, c3;

            REQUIRE_INBUF(3);
            c2 = INBYTE2;
            c3 = INBYTE3;
            /* JIS X 0212 */
            if (TRYMAP_DEC(jisx0212, decoded, c2 ^ 0x80, c3 ^ 0x80)) {
                OUTCHAR(decoded);
                NEXT_IN(3);
            }
            else
                return 1;
        }
        else {
            unsigned char c2;

            REQUIRE_INBUF(2);
            c2 = INBYTE2;
            /* JIS X 0208 */
#ifndef STRICT_BUILD
            if (c == 0xa1 && c2 == 0xc0)
                /* FULL-WIDTH REVERSE SOLIDUS */
                OUTCHAR(0xff3c);
            else
#endif
            if (TRYMAP_DEC(jisx0208, decoded, c ^ 0x80, c2 ^ 0x80))
                OUTCHAR(decoded);
            else
                return 1;
            NEXT_IN(2);
        }
    }

    return 0;
}


/*
 * SHIFT_JIS codec
 */

ENCODER(shift_jis)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code;
        unsigned char c1, c2;

#ifdef STRICT_BUILD
        JISX0201_R_ENCODE(c, code)
#else
        if (c < 0x80)
            code = c;
        else if (c == 0x00a5)
            code = 0x5c; /* YEN SIGN */
        else if (c == 0x203e)
            code = 0x7e; /* OVERLINE */
#endif
        else JISX0201_K_ENCODE(c, code)
        else if (c > 0xFFFF)
            return 1;
        else
            code = NOCHAR;

        if (code < 0x80 || (code >= 0xa1 && code <= 0xdf)) {
            REQUIRE_OUTBUF(1);

            OUTBYTE1((unsigned char)code);
            NEXT(1, 1);
            continue;
        }

        REQUIRE_OUTBUF(2);

        if (code == NOCHAR) {
            if (TRYMAP_ENC(jisxcommon, code, c))
                ;
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
        OUTBYTE1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1);
        OUTBYTE2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41);
        NEXT(1, 2);
    }

    return 0;
}

DECODER(shift_jis)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_UCS4 decoded;

#ifdef STRICT_BUILD
        JISX0201_R_DECODE(c, writer)
#else
        if (c < 0x80)
            OUTCHAR(c);
#endif
        else JISX0201_K_DECODE(c, writer)
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)){
            unsigned char c1, c2;

            REQUIRE_INBUF(2);
            c2 = INBYTE2;
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 1;

            c1 = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c1 = (2 * c1 + (c2 < 0x5e ? 0 : 1) + 0x21);
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

#ifndef STRICT_BUILD
            if (c1 == 0x21 && c2 == 0x40) {
                /* FULL-WIDTH REVERSE SOLIDUS */
                OUTCHAR(0xff3c);
                NEXT_IN(2);
                continue;
            }
#endif
            if (TRYMAP_DEC(jisx0208, decoded, c1, c2)) {
                OUTCHAR(decoded);
                NEXT_IN(2);
                continue;
            }
            else
                return 1;
        }
        else
            return 1;

        NEXT_IN(1); /* JIS X 0201 */
    }

    return 0;
}


/*
 * SHIFT_JIS-2004 codec
 */

ENCODER(shift_jis_2004)
{
    while (*inpos < inlen) {
        Py_UCS4 c = INCHAR1;
        DBCHAR code = NOCHAR;
        int c1, c2;
        Py_ssize_t insize;

        JISX0201_ENCODE(c, code)

        if (code < 0x80 || (code >= 0xa1 && code <= 0xdf)) {
            WRITEBYTE1((unsigned char)code);
            NEXT(1, 1);
            continue;
        }

        REQUIRE_OUTBUF(2);
        insize = 1;

        if (code == NOCHAR) {
            if (c <= 0xffff) {
                EMULATE_JISX0213_2000_ENCODE_BMP(code, c)
                else if (TRYMAP_ENC(jisx0213_bmp, code, c)) {
                    if (code == MULTIC) {
                        if (inlen - *inpos < 2) {
                            if (flags & MBENC_FLUSH) {
                            code = find_pairencmap
                                ((ucs2_t)c, 0,
                              jisx0213_pair_encmap,
                                JISX0213_ENCPAIRS);
                            if (code == DBCINV)
                                return 1;
                            }
                            else
                                return MBERR_TOOFEW;
                        }
                        else {
                            Py_UCS4 ch2 = INCHAR2;
                            code = find_pairencmap(
                                (ucs2_t)c, ch2,
                              jisx0213_pair_encmap,
                                JISX0213_ENCPAIRS);
                            if (code == DBCINV) {
                            code = find_pairencmap(
                                (ucs2_t)c, 0,
                              jisx0213_pair_encmap,
                                JISX0213_ENCPAIRS);
                            if (code == DBCINV)
                                return 1;
                            }
                            else
                                insize = 2;
                        }
                    }
                }
                else if (TRYMAP_ENC(jisxcommon, code, c)) {
                    /* abandon JIS X 0212 codes */
                    if (code & 0x8000)
                        return 1;
                }
                else
                    return 1;
            }
            else if (c >> 16 == EMPBASE >> 16) {
                EMULATE_JISX0213_2000_ENCODE_EMP(code, c)
                else if (TRYMAP_ENC(jisx0213_emp, code, c&0xffff))
                    ;
                else
                    return insize;
            }
            else
                return insize;
        }

        c1 = code >> 8;
        c2 = (code & 0xff) - 0x21;

        if (c1 & 0x80) {
            /* Plane 2 */
            if (c1 >= 0xee)
                c1 -= 0x87;
            else if (c1 >= 0xac || c1 == 0xa8)
                c1 -= 0x49;
            else
                c1 -= 0x43;
        }
        else {
            /* Plane 1 */
            c1 -= 0x21;
        }

        if (c1 & 1)
            c2 += 0x5e;
        c1 >>= 1;
        OUTBYTE1(c1 + (c1 < 0x1f ? 0x81 : 0xc1));
        OUTBYTE2(c2 + (c2 < 0x3f ? 0x40 : 0x41));

        NEXT(insize, 2);
    }

    return 0;
}

DECODER(shift_jis_2004)
{
    while (inleft > 0) {
        unsigned char c = INBYTE1;

        JISX0201_DECODE(c, writer)
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)){
            unsigned char c1, c2;
            Py_UCS4 code, decoded;

            REQUIRE_INBUF(2);
            c2 = INBYTE2;
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 1;

            c1 = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c1 = (2 * c1 + (c2 < 0x5e ? 0 : 1));
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

            if (c1 < 0x5e) { /* Plane 1 */
                c1 += 0x21;
                EMULATE_JISX0213_2000_DECODE_PLANE1(writer,
                                c1, c2)
                else if (TRYMAP_DEC(jisx0208, decoded, c1, c2))
                    OUTCHAR(decoded);
                else if (TRYMAP_DEC(jisx0213_1_bmp, decoded, c1, c2))
                    OUTCHAR(decoded);
                else if (TRYMAP_DEC(jisx0213_1_emp, code, c1, c2))
                    OUTCHAR(EMPBASE | code);
                else if (TRYMAP_DEC(jisx0213_pair, code, c1, c2))
                    OUTCHAR2(code >> 16, code & 0xffff);
                else
                    return 1;
                NEXT_IN(2);
            }
            else { /* Plane 2 */
                if (c1 >= 0x67)
                    c1 += 0x07;
                else if (c1 >= 0x63 || c1 == 0x5f)
                    c1 -= 0x37;
                else
                    c1 -= 0x3d;

                EMULATE_JISX0213_2000_DECODE_PLANE2(writer,
                                c1, c2)
                else if (TRYMAP_DEC(jisx0213_2_bmp, decoded, c1, c2))
                    OUTCHAR(decoded);
                else if (TRYMAP_DEC(jisx0213_2_emp, code, c1, c2)) {
                    OUTCHAR(EMPBASE | code);
                    NEXT_IN(2);
                    continue;
                }
                else
                    return 1;
                NEXT_IN(2);
            }
            continue;
        }
        else
            return 1;

        NEXT_IN(1); /* JIS X 0201 */
    }

    return 0;
}


BEGIN_MAPPINGS_LIST
  MAPPING_DECONLY(jisx0208)
  MAPPING_DECONLY(jisx0212)
  MAPPING_ENCONLY(jisxcommon)
  MAPPING_DECONLY(jisx0213_1_bmp)
  MAPPING_DECONLY(jisx0213_2_bmp)
  MAPPING_ENCONLY(jisx0213_bmp)
  MAPPING_DECONLY(jisx0213_1_emp)
  MAPPING_DECONLY(jisx0213_2_emp)
  MAPPING_ENCONLY(jisx0213_emp)
  MAPPING_ENCDEC(jisx0213_pair)
  MAPPING_ENCDEC(cp932ext)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS(shift_jis)
  CODEC_STATELESS(cp932)
  CODEC_STATELESS(euc_jp)
  CODEC_STATELESS(shift_jis_2004)
  CODEC_STATELESS(euc_jis_2004)
  { "euc_jisx0213", (void *)2000, NULL, _STATELESS_METHODS(euc_jis_2004) },
  { "shift_jisx0213", (void *)2000, NULL, _STATELESS_METHODS(shift_jis_2004) },
END_CODECS_LIST

I_AM_A_MODULE_FOR(jp)
