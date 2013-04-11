/*
 * _codecs_kr.c: Codecs collection for Korean encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#include "cjkcodecs.h"
#include "mappings_kr.h"

/*
 * EUC-KR codec
 */

#define EUCKR_JAMO_FIRSTBYTE    0xA4
#define EUCKR_JAMO_FILLER       0xD4

static const unsigned char u2cgk_choseong[19] = {
    0xa1, 0xa2, 0xa4, 0xa7, 0xa8, 0xa9, 0xb1, 0xb2,
    0xb3, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb,
    0xbc, 0xbd, 0xbe
};
static const unsigned char u2cgk_jungseong[21] = {
    0xbf, 0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6,
    0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce,
    0xcf, 0xd0, 0xd1, 0xd2, 0xd3
};
static const unsigned char u2cgk_jongseong[28] = {
    0xd4, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 0xb0,
    0xb1, 0xb2, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xba,
    0xbb, 0xbc, 0xbd, 0xbe
};

ENCODER(euc_kr)
{
    while (inleft > 0) {
        Py_UCS4 c = IN1;
        DBCHAR code;

        if (c < 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        REQUIRE_OUTBUF(2)
        TRYMAP_ENC(cp949, code, c);
        else return 1;

        if ((code & 0x8000) == 0) {
            /* KS X 1001 coded character */
            OUT1((code >> 8) | 0x80)
            OUT2((code & 0xFF) | 0x80)
            NEXT(1, 2)
        }
        else {          /* Mapping is found in CP949 extension,
                 * but we encode it in KS X 1001:1998 Annex 3,
                 * make-up sequence for EUC-KR. */

            REQUIRE_OUTBUF(8)

            /* syllable composition precedence */
            OUT1(EUCKR_JAMO_FIRSTBYTE)
            OUT2(EUCKR_JAMO_FILLER)

            /* All codepoints in CP949 extension are in unicode
             * Hangul Syllable area. */
            assert(0xac00 <= c && c <= 0xd7a3);
            c -= 0xac00;

            OUT3(EUCKR_JAMO_FIRSTBYTE)
            OUT4(u2cgk_choseong[c / 588])
            NEXT_OUT(4)

            OUT1(EUCKR_JAMO_FIRSTBYTE)
            OUT2(u2cgk_jungseong[(c / 28) % 21])
            OUT3(EUCKR_JAMO_FIRSTBYTE)
            OUT4(u2cgk_jongseong[c % 28])
            NEXT(1, 4)
        }
    }

    return 0;
}

#define NONE    127

static const unsigned char cgk2u_choseong[] = { /* [A1, BE] */
       0,    1, NONE,    2, NONE, NONE,    3,    4,
       5, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
       6,    7,    8, NONE,    9,   10,   11,   12,
      13,   14,   15,   16,   17,   18
};
static const unsigned char cgk2u_jongseong[] = { /* [A1, BE] */
       1,    2,    3,    4,    5,    6,    7, NONE,
       8,    9,   10,   11,   12,   13,   14,   15,
      16,   17, NONE,   18,   19,   20,   21,   22,
    NONE,   23,   24,   25,   26,   27
};

DECODER(euc_kr)
{
    while (inleft > 0) {
        unsigned char c = IN1;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2)

        if (c == EUCKR_JAMO_FIRSTBYTE &&
            IN2 == EUCKR_JAMO_FILLER) {
            /* KS X 1001:1998 Annex 3 make-up sequence */
            DBCHAR cho, jung, jong;

            REQUIRE_INBUF(8)
            if ((*inbuf)[2] != EUCKR_JAMO_FIRSTBYTE ||
                (*inbuf)[4] != EUCKR_JAMO_FIRSTBYTE ||
                (*inbuf)[6] != EUCKR_JAMO_FIRSTBYTE)
                return 1;

            c = (*inbuf)[3];
            if (0xa1 <= c && c <= 0xbe)
                cho = cgk2u_choseong[c - 0xa1];
            else
                cho = NONE;

            c = (*inbuf)[5];
            jung = (0xbf <= c && c <= 0xd3) ? c - 0xbf : NONE;

            c = (*inbuf)[7];
            if (c == EUCKR_JAMO_FILLER)
                jong = 0;
            else if (0xa1 <= c && c <= 0xbe)
                jong = cgk2u_jongseong[c - 0xa1];
            else
                jong = NONE;

            if (cho == NONE || jung == NONE || jong == NONE)
                return 1;

            OUTCHAR(0xac00 + cho*588 + jung*28 + jong);
            NEXT_IN(8);
        }
        else TRYMAP_DEC(ksx1001, writer, c ^ 0x80, IN2 ^ 0x80) {
            NEXT_IN(2);
        }
        else
            return 1;
    }

    return 0;
}
#undef NONE


/*
 * CP949 codec
 */

ENCODER(cp949)
{
    while (inleft > 0) {
        Py_UCS4 c = IN1;
        DBCHAR code;

        if (c < 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        REQUIRE_OUTBUF(2)
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
        unsigned char c = IN1;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2)
        TRYMAP_DEC(ksx1001, writer, c ^ 0x80, IN2 ^ 0x80);
        else TRYMAP_DEC(cp949ext, writer, c, IN2);
        else return 1;

        NEXT_IN(2);
    }

    return 0;
}


/*
 * JOHAB codec
 */

static const unsigned char u2johabidx_choseong[32] = {
                0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11, 0x12, 0x13, 0x14,
};
static const unsigned char u2johabidx_jungseong[32] = {
                      0x03, 0x04, 0x05, 0x06, 0x07,
                0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                0x1a, 0x1b, 0x1c, 0x1d,
};
static const unsigned char u2johabidx_jongseong[32] = {
          0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
    0x10, 0x11,       0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
};
static const DBCHAR u2johabjamo[] = {
            0x8841, 0x8c41, 0x8444, 0x9041, 0x8446, 0x8447, 0x9441,
    0x9841, 0x9c41, 0x844a, 0x844b, 0x844c, 0x844d, 0x844e, 0x844f,
    0x8450, 0xa041, 0xa441, 0xa841, 0x8454, 0xac41, 0xb041, 0xb441,
    0xb841, 0xbc41, 0xc041, 0xc441, 0xc841, 0xcc41, 0xd041, 0x8461,
    0x8481, 0x84a1, 0x84c1, 0x84e1, 0x8541, 0x8561, 0x8581, 0x85a1,
    0x85c1, 0x85e1, 0x8641, 0x8661, 0x8681, 0x86a1, 0x86c1, 0x86e1,
    0x8741, 0x8761, 0x8781, 0x87a1,
};

ENCODER(johab)
{
    while (inleft > 0) {
        Py_UCS4 c = IN1;
        DBCHAR code;

        if (c < 0x80) {
            WRITE1((unsigned char)c)
            NEXT(1, 1)
            continue;
        }
        UCS4INVALID(c)

        REQUIRE_OUTBUF(2)

        if (c >= 0xac00 && c <= 0xd7a3) {
            c -= 0xac00;
            code = 0x8000 |
                (u2johabidx_choseong[c / 588] << 10) |
                (u2johabidx_jungseong[(c / 28) % 21] << 5) |
                u2johabidx_jongseong[c % 28];
        }
        else if (c >= 0x3131 && c <= 0x3163)
            code = u2johabjamo[c - 0x3131];
        else TRYMAP_ENC(cp949, code, c) {
            unsigned char c1, c2, t2;
            unsigned short t1;

            assert((code & 0x8000) == 0);
            c1 = code >> 8;
            c2 = code & 0xff;
            if (((c1 >= 0x21 && c1 <= 0x2c) ||
                (c1 >= 0x4a && c1 <= 0x7d)) &&
                (c2 >= 0x21 && c2 <= 0x7e)) {
                t1 = (c1 < 0x4a ? (c1 - 0x21 + 0x1b2) :
                          (c1 - 0x21 + 0x197));
                t2 = ((t1 & 1) ? 0x5e : 0) + (c2 - 0x21);
                OUT1(t1 >> 1)
                OUT2(t2 < 0x4e ? t2 + 0x31 : t2 + 0x43)
                NEXT(1, 2)
                continue;
            }
            else
                return 1;
        }
        else
            return 1;

        OUT1(code >> 8)
        OUT2(code & 0xff)
        NEXT(1, 2)
    }

    return 0;
}

#define FILL 0xfd
#define NONE 0xff

static const unsigned char johabidx_choseong[32] = {
    NONE, FILL, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d,
    0x0e, 0x0f, 0x10, 0x11, 0x12, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
};
static const unsigned char johabidx_jungseong[32] = {
    NONE, NONE, FILL, 0x00, 0x01, 0x02, 0x03, 0x04,
    NONE, NONE, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
    NONE, NONE, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10,
    NONE, NONE, 0x11, 0x12, 0x13, 0x14, NONE, NONE,
};
static const unsigned char johabidx_jongseong[32] = {
    NONE, FILL, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
    0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e,
    0x0f, 0x10, NONE, 0x11, 0x12, 0x13, 0x14, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, NONE, NONE,
};

static const unsigned char johabjamo_choseong[32] = {
    NONE, FILL, 0x31, 0x32, 0x34, 0x37, 0x38, 0x39,
    0x41, 0x42, 0x43, 0x45, 0x46, 0x47, 0x48, 0x49,
    0x4a, 0x4b, 0x4c, 0x4d, 0x4e, NONE, NONE, NONE,
    NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE,
};
static const unsigned char johabjamo_jungseong[32] = {
    NONE, NONE, FILL, 0x4f, 0x50, 0x51, 0x52, 0x53,
    NONE, NONE, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
    NONE, NONE, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    NONE, NONE, 0x60, 0x61, 0x62, 0x63, NONE, NONE,
};
static const unsigned char johabjamo_jongseong[32] = {
    NONE, FILL, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
    0x37, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
    0x40, 0x41, NONE, 0x42, 0x44, 0x45, 0x46, 0x47,
    0x48, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, NONE, NONE,
};

DECODER(johab)
{
    while (inleft > 0) {
        unsigned char    c = IN1, c2;

        if (c < 0x80) {
            OUTCHAR(c);
            NEXT_IN(1);
            continue;
        }

        REQUIRE_INBUF(2)
        c2 = IN2;

        if (c < 0xd8) {
            /* johab hangul */
            unsigned char c_cho, c_jung, c_jong;
            unsigned char i_cho, i_jung, i_jong;

            c_cho = (c >> 2) & 0x1f;
            c_jung = ((c << 3) | c2 >> 5) & 0x1f;
            c_jong = c2 & 0x1f;

            i_cho = johabidx_choseong[c_cho];
            i_jung = johabidx_jungseong[c_jung];
            i_jong = johabidx_jongseong[c_jong];

            if (i_cho == NONE || i_jung == NONE || i_jong == NONE)
                return 1;

            /* we don't use U+1100 hangul jamo yet. */
            if (i_cho == FILL) {
                if (i_jung == FILL) {
                    if (i_jong == FILL)
                        OUTCHAR(0x3000);
                    else
                        OUTCHAR(0x3100 |
                            johabjamo_jongseong[c_jong]);
                }
                else {
                    if (i_jong == FILL)
                        OUTCHAR(0x3100 |
                            johabjamo_jungseong[c_jung]);
                    else
                        return 1;
                }
            } else {
                if (i_jung == FILL) {
                    if (i_jong == FILL)
                        OUTCHAR(0x3100 |
                            johabjamo_choseong[c_cho]);
                    else
                        return 1;
                }
                else
                    OUTCHAR(0xac00 +
                        i_cho * 588 +
                        i_jung * 28 +
                        (i_jong == FILL ? 0 : i_jong));
            }
            NEXT_IN(2);
        } else {
            /* KS X 1001 except hangul jamos and syllables */
            if (c == 0xdf || c > 0xf9 ||
                c2 < 0x31 || (c2 >= 0x80 && c2 < 0x91) ||
                (c2 & 0x7f) == 0x7f ||
                (c == 0xda && (c2 >= 0xa1 && c2 <= 0xd3)))
                return 1;
            else {
                unsigned char t1, t2;

                t1 = (c < 0xe0 ? 2 * (c - 0xd9) :
                         2 * c - 0x197);
                t2 = (c2 < 0x91 ? c2 - 0x31 : c2 - 0x43);
                t1 = t1 + (t2 < 0x5e ? 0 : 1) + 0x21;
                t2 = (t2 < 0x5e ? t2 : t2 - 0x5e) + 0x21;

                TRYMAP_DEC(ksx1001, writer, t1, t2);
                else return 1;
                NEXT_IN(2);
            }
        }
    }

    return 0;
}
#undef NONE
#undef FILL


BEGIN_MAPPINGS_LIST
  MAPPING_DECONLY(ksx1001)
  MAPPING_ENCONLY(cp949)
  MAPPING_DECONLY(cp949ext)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS(euc_kr)
  CODEC_STATELESS(cp949)
  CODEC_STATELESS(johab)
END_CODECS_LIST

I_AM_A_MODULE_FOR(kr)
