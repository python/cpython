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
    while (inleft > 0) {
        Py_UNICODE c = IN1;
        DBCHAR code;
        unsigned char c1, c2;

        if (c <= 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }
        else if (c >= 0xff61 && c <= 0xff9f) {
            WRITE1(c - 0xfec0)
            NEXT(1, 1)
            continue;
        }
        else if (c >= 0xf8f0 && c <= 0xf8f3) {
            /* Windows compatibility */
            REQUIRE_OUTBUF(1)
            if (c == 0xf8f0)
                OUT1(0xa0)
            else
                OUT1(c - 0xfef1 + 0xfd)
            NEXT(1, 1)
            continue;
        }

        UCS4INVALID(c)
        REQUIRE_OUTBUF(2)

        TRYMAP_ENC(cp932ext, code, c) {
            OUT1(code >> 8)
            OUT2(code & 0xff)
        }
        else TRYMAP_ENC(jisxcommon, code, c) {
            if (code & 0x8000) /* MSB set: JIS X 0212 */
                return 1;

            /* JIS X 0208 */
            c1 = code >> 8;
            c2 = code & 0xff;
            c2 = (((c1 - 0x21) & 1) ? 0x5e : 0) + (c2 - 0x21);
            c1 = (c1 - 0x21) >> 1;
            OUT1(c1 < 0x1f ? c1 + 0x81 : c1 + 0xc1)
            OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
        }
        else if (c >= 0xe000 && c < 0xe758) {
            /* User-defined area */
            c1 = (Py_UNICODE)(c - 0xe000) / 188;
            c2 = (Py_UNICODE)(c - 0xe000) % 188;
            OUT1(c1 + 0xf0)
            OUT2(c2 < 0x3f ? c2 + 0x40 : c2 + 0x41)
        }
        else
            return 1;

        NEXT(1, 2)
    }

    return 0;
}

DECODER(cp932)
{
    while (inleft > 0) {
        unsigned char c = IN1, c2;

        REQUIRE_OUTBUF(1)
        if (c <= 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }
        else if (c >= 0xa0 && c <= 0xdf) {
            if (c == 0xa0)
                OUT1(0xf8f0) /* half-width katakana */
            else
                OUT1(0xfec0 + c)
            NEXT(1, 1)
            continue;
        }
        else if (c >= 0xfd/* && c <= 0xff*/) {
            /* Windows compatibility */
            OUT1(0xf8f1 - 0xfd + c)
            NEXT(1, 1)
            continue;
        }

        REQUIRE_INBUF(2)
        c2 = IN2;

        TRYMAP_DEC(cp932ext, **outbuf, c, c2);
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)){
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 2;

            c = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c = (2 * c + (c2 < 0x5e ? 0 : 1) + 0x21);
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

            TRYMAP_DEC(jisx0208, **outbuf, c, c2);
            else return 2;
        }
        else if (c >= 0xf0 && c <= 0xf9) {
            if ((c2 >= 0x40 && c2 <= 0x7e) ||
                (c2 >= 0x80 && c2 <= 0xfc))
                OUT1(0xe000 + 188 * (c - 0xf0) +
                     (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41))
            else
                return 2;
        }
        else
            return 2;

        NEXT(2, 1)
    }

    return 0;
}


/*
 * EUC-JIS-2004 codec
 */

ENCODER(euc_jis_2004)
{
    while (inleft > 0) {
        ucs4_t c = IN1;
        DBCHAR code;
        Py_ssize_t insize;

        if (c < 0x80) {
            WRITE1(c)
            NEXT(1, 1)
            continue;
        }

        DECODE_SURROGATE(c)
        insize = GET_INSIZE(c);

        if (c <= 0xFFFF) {
            EMULATE_JISX0213_2000_ENCODE_BMP(code, c)
            else TRYMAP_ENC(jisx0213_bmp, code, c) {
                if (code == MULTIC) {
                    if (inleft < 2) {
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
                        code = find_pairencmap(
                            (ucs2_t)c, (*inbuf)[1],
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
            else TRYMAP_ENC(jisxcommon, code, c);
            else if (c >= 0xff61 && c <= 0xff9f) {
                /* JIS X 0201 half-width katakana */
                WRITE2(0x8e, c - 0xfec0)
                NEXT(1, 2)
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
            else TRYMAP_ENC(jisx0213_emp, code, c & 0xffff);
            else return insize;
        }
        else
            return insize;

        if (code & 0x8000) {
            /* Codeset 2 */
            WRITE3(0x8f, code >> 8, (code & 0xFF) | 0x80)
            NEXT(insize, 3)
        } else {
            /* Codeset 1 */
            WRITE2((code >> 8) | 0x80, (code & 0xFF) | 0x80)
            NEXT(insize, 2)
        }
    }

    return 0;
}

DECODER(euc_jis_2004)
{
    while (inleft > 0) {
        unsigned char c = IN1;
        ucs4_t code;

        REQUIRE_OUTBUF(1)

        if (c < 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }

        if (c == 0x8e) {
            /* JIS X 0201 half-width katakana */
            unsigned char c2;

            REQUIRE_INBUF(2)
            c2 = IN2;
            if (c2 >= 0xa1 && c2 <= 0xdf) {
                OUT1(0xfec0 + c2)
                NEXT(2, 1)
            }
            else
                return 2;
        }
        else if (c == 0x8f) {
            unsigned char c2, c3;

            REQUIRE_INBUF(3)
            c2 = IN2 ^ 0x80;
            c3 = IN3 ^ 0x80;

            /* JIS X 0213 Plane 2 or JIS X 0212 (see NOTES) */
            EMULATE_JISX0213_2000_DECODE_PLANE2(**outbuf, c2, c3)
            else TRYMAP_DEC(jisx0213_2_bmp, **outbuf, c2, c3) ;
            else TRYMAP_DEC(jisx0213_2_emp, code, c2, c3) {
                WRITEUCS4(EMPBASE | code)
                NEXT_IN(3)
                continue;
            }
            else TRYMAP_DEC(jisx0212, **outbuf, c2, c3) ;
            else return 3;
            NEXT(3, 1)
        }
        else {
            unsigned char c2;

            REQUIRE_INBUF(2)
            c ^= 0x80;
            c2 = IN2 ^ 0x80;

            /* JIS X 0213 Plane 1 */
            EMULATE_JISX0213_2000_DECODE_PLANE1(**outbuf, c, c2)
            else if (c == 0x21 && c2 == 0x40) **outbuf = 0xff3c;
            else if (c == 0x22 && c2 == 0x32) **outbuf = 0xff5e;
            else TRYMAP_DEC(jisx0208, **outbuf, c, c2);
            else TRYMAP_DEC(jisx0213_1_bmp, **outbuf, c, c2);
            else TRYMAP_DEC(jisx0213_1_emp, code, c, c2) {
                WRITEUCS4(EMPBASE | code)
                NEXT_IN(2)
                continue;
            }
            else TRYMAP_DEC(jisx0213_pair, code, c, c2) {
                WRITE2(code >> 16, code & 0xffff)
                NEXT(2, 2)
                continue;
            }
            else return 2;
            NEXT(2, 1)
        }
    }

    return 0;
}


/*
 * EUC-JP codec
 */

ENCODER(euc_jp)
{
    while (inleft > 0) {
        Py_UNICODE c = IN1;
        DBCHAR code;

        if (c < 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }

        UCS4INVALID(c)

        TRYMAP_ENC(jisxcommon, code, c);
        else if (c >= 0xff61 && c <= 0xff9f) {
            /* JIS X 0201 half-width katakana */
            WRITE2(0x8e, c - 0xfec0)
            NEXT(1, 2)
            continue;
        }
#ifndef STRICT_BUILD
        else if (c == 0xff3c) /* FULL-WIDTH REVERSE SOLIDUS */
            code = 0x2140;
        else if (c == 0xa5) { /* YEN SIGN */
            WRITE1(0x5c);
            NEXT(1, 1)
            continue;
        } else if (c == 0x203e) { /* OVERLINE */
            WRITE1(0x7e);
            NEXT(1, 1)
            continue;
        }
#endif
        else
            return 1;

        if (code & 0x8000) {
            /* JIS X 0212 */
            WRITE3(0x8f, code >> 8, (code & 0xFF) | 0x80)
            NEXT(1, 3)
        } else {
            /* JIS X 0208 */
            WRITE2((code >> 8) | 0x80, (code & 0xFF) | 0x80)
            NEXT(1, 2)
        }
    }

    return 0;
}

DECODER(euc_jp)
{
    while (inleft > 0) {
        unsigned char c = IN1;

        REQUIRE_OUTBUF(1)

            if (c < 0x80) {
                OUT1(c)
                NEXT(1, 1)
                continue;
            }

        if (c == 0x8e) {
            /* JIS X 0201 half-width katakana */
            unsigned char c2;

            REQUIRE_INBUF(2)
            c2 = IN2;
            if (c2 >= 0xa1 && c2 <= 0xdf) {
                OUT1(0xfec0 + c2)
                NEXT(2, 1)
            }
            else
                return 2;
        }
        else if (c == 0x8f) {
            unsigned char c2, c3;

            REQUIRE_INBUF(3)
            c2 = IN2;
            c3 = IN3;
            /* JIS X 0212 */
            TRYMAP_DEC(jisx0212, **outbuf, c2 ^ 0x80, c3 ^ 0x80) {
                NEXT(3, 1)
            }
            else
                return 3;
        }
        else {
            unsigned char c2;

            REQUIRE_INBUF(2)
            c2 = IN2;
            /* JIS X 0208 */
#ifndef STRICT_BUILD
            if (c == 0xa1 && c2 == 0xc0)
                /* FULL-WIDTH REVERSE SOLIDUS */
                **outbuf = 0xff3c;
            else
#endif
                TRYMAP_DEC(jisx0208, **outbuf,
                           c ^ 0x80, c2 ^ 0x80) ;
            else return 2;
            NEXT(2, 1)
        }
    }

    return 0;
}


/*
 * SHIFT_JIS codec
 */

ENCODER(shift_jis)
{
    while (inleft > 0) {
        Py_UNICODE c = IN1;
        DBCHAR code;
        unsigned char c1, c2;

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
            REQUIRE_OUTBUF(1)

            OUT1((unsigned char)code)
            NEXT(1, 1)
            continue;
        }

        REQUIRE_OUTBUF(2)

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
        unsigned char c = IN1;

        REQUIRE_OUTBUF(1)

#ifdef STRICT_BUILD
        JISX0201_R_DECODE(c, **outbuf)
#else
        if (c < 0x80) **outbuf = c;
#endif
        else JISX0201_K_DECODE(c, **outbuf)
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xea)){
            unsigned char c1, c2;

            REQUIRE_INBUF(2)
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
            }
            else
                return 2;
        }
        else
            return 2;

        NEXT(1, 1) /* JIS X 0201 */
    }

    return 0;
}


/*
 * SHIFT_JIS-2004 codec
 */

ENCODER(shift_jis_2004)
{
    while (inleft > 0) {
        ucs4_t c = IN1;
        DBCHAR code = NOCHAR;
        int c1, c2;
        Py_ssize_t insize;

        JISX0201_ENCODE(c, code)
        else DECODE_SURROGATE(c)

        if (code < 0x80 || (code >= 0xa1 && code <= 0xdf)) {
            WRITE1((unsigned char)code)
            NEXT(1, 1)
            continue;
        }

        REQUIRE_OUTBUF(2)
        insize = GET_INSIZE(c);

        if (code == NOCHAR) {
            if (c <= 0xffff) {
                EMULATE_JISX0213_2000_ENCODE_BMP(code, c)
                else TRYMAP_ENC(jisx0213_bmp, code, c) {
                    if (code == MULTIC) {
                        if (inleft < 2) {
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
                            code = find_pairencmap(
                                (ucs2_t)c, IN2,
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
                else TRYMAP_ENC(jisxcommon, code, c) {
                    /* abandon JIS X 0212 codes */
                    if (code & 0x8000)
                        return 1;
                }
                else return 1;
            }
            else if (c >> 16 == EMPBASE >> 16) {
                EMULATE_JISX0213_2000_ENCODE_EMP(code, c)
                else TRYMAP_ENC(jisx0213_emp, code, c&0xffff);
                else return insize;
            }
            else
                return insize;
        }

        c1 = code >> 8;
        c2 = (code & 0xff) - 0x21;

        if (c1 & 0x80) { /* Plane 2 */
            if (c1 >= 0xee) c1 -= 0x87;
            else if (c1 >= 0xac || c1 == 0xa8) c1 -= 0x49;
            else c1 -= 0x43;
        }
        else /* Plane 1 */
            c1 -= 0x21;

        if (c1 & 1) c2 += 0x5e;
        c1 >>= 1;
        OUT1(c1 + (c1 < 0x1f ? 0x81 : 0xc1))
        OUT2(c2 + (c2 < 0x3f ? 0x40 : 0x41))

        NEXT(insize, 2)
    }

    return 0;
}

DECODER(shift_jis_2004)
{
    while (inleft > 0) {
        unsigned char c = IN1;

        REQUIRE_OUTBUF(1)
        JISX0201_DECODE(c, **outbuf)
        else if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)){
            unsigned char c1, c2;
            ucs4_t code;

            REQUIRE_INBUF(2)
            c2 = IN2;
            if (c2 < 0x40 || (c2 > 0x7e && c2 < 0x80) || c2 > 0xfc)
                return 2;

            c1 = (c < 0xe0 ? c - 0x81 : c - 0xc1);
            c2 = (c2 < 0x80 ? c2 - 0x40 : c2 - 0x41);
            c1 = (2 * c1 + (c2 < 0x5e ? 0 : 1));
            c2 = (c2 < 0x5e ? c2 : c2 - 0x5e) + 0x21;

            if (c1 < 0x5e) { /* Plane 1 */
                c1 += 0x21;
                EMULATE_JISX0213_2000_DECODE_PLANE1(**outbuf,
                                c1, c2)
                else TRYMAP_DEC(jisx0208, **outbuf, c1, c2) {
                    NEXT_OUT(1)
                }
                else TRYMAP_DEC(jisx0213_1_bmp, **outbuf,
                                c1, c2) {
                    NEXT_OUT(1)
                }
                else TRYMAP_DEC(jisx0213_1_emp, code, c1, c2) {
                    WRITEUCS4(EMPBASE | code)
                }
                else TRYMAP_DEC(jisx0213_pair, code, c1, c2) {
                    WRITE2(code >> 16, code & 0xffff)
                    NEXT_OUT(2)
                }
                else
                    return 2;
                NEXT_IN(2)
            }
            else { /* Plane 2 */
                if (c1 >= 0x67) c1 += 0x07;
                else if (c1 >= 0x63 || c1 == 0x5f) c1 -= 0x37;
                else c1 -= 0x3d;

                EMULATE_JISX0213_2000_DECODE_PLANE2(**outbuf,
                                c1, c2)
                else TRYMAP_DEC(jisx0213_2_bmp, **outbuf,
                                c1, c2) ;
                else TRYMAP_DEC(jisx0213_2_emp, code, c1, c2) {
                    WRITEUCS4(EMPBASE | code)
                    NEXT_IN(2)
                    continue;
                }
                else
                    return 2;
                NEXT(2, 1)
            }
            continue;
        }
        else
            return 2;

        NEXT(1, 1) /* JIS X 0201 */
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
