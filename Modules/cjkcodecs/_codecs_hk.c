/*
 * _codecs_hk.c: Codecs collection for encodings from Hong Kong
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#define USING_IMPORTED_MAPS

#include "cjkcodecs.h"
#include "mappings_hk.h"

/*
 * BIG5HKSCS codec
 */

static const encode_map *big5_encmap = NULL;
static const decode_map *big5_decmap = NULL;

CODEC_INIT(big5hkscs)
{
    static int initialized = 0;

    if (!initialized && IMPORT_MAP(tw, big5, &big5_encmap, &big5_decmap))
        return -1;
    initialized = 1;
    return 0;
}

/*
 * There are four possible pair unicode -> big5hkscs maps as in HKSCS 2004:
 *  U+00CA U+0304 -> 8862  (U+00CA alone is mapped to 8866)
 *  U+00CA U+030C -> 8864
 *  U+00EA U+0304 -> 88a3  (U+00EA alone is mapped to 88a7)
 *  U+00EA U+030C -> 88a5
 * These are handled by not mapping tables but a hand-written code.
 */
static const DBCHAR big5hkscs_pairenc_table[4] = {0x8862, 0x8864, 0x88a3, 0x88a5};

ENCODER(big5hkscs)
{
    while (inleft > 0) {
        ucs4_t c = **inbuf;
        DBCHAR code;
        Py_ssize_t insize;

        if (c < 0x80) {
            REQUIRE_OUTBUF(1)
            **outbuf = (unsigned char)c;
            NEXT(1, 1)
            continue;
        }

        DECODE_SURROGATE(c)
        insize = GET_INSIZE(c);

        REQUIRE_OUTBUF(2)

        if (c < 0x10000) {
            TRYMAP_ENC(big5hkscs_bmp, code, c) {
                if (code == MULTIC) {
                    if (inleft >= 2 &&
                        ((c & 0xffdf) == 0x00ca) &&
                        (((*inbuf)[1] & 0xfff7) == 0x0304)) {
                        code = big5hkscs_pairenc_table[
                            ((c >> 4) |
                             ((*inbuf)[1] >> 3)) & 3];
                        insize = 2;
                    }
                    else if (inleft < 2 &&
                             !(flags & MBENC_FLUSH))
                        return MBERR_TOOFEW;
                    else {
                        if (c == 0xca)
                            code = 0x8866;
                        else /* c == 0xea */
                            code = 0x88a7;
                    }
                }
            }
            else TRYMAP_ENC(big5, code, c);
            else return 1;
        }
        else if (c < 0x20000)
            return insize;
        else if (c < 0x30000) {
            TRYMAP_ENC(big5hkscs_nonbmp, code, c & 0xffff);
            else return insize;
        }
        else
            return insize;

        OUT1(code >> 8)
        OUT2(code & 0xFF)
        NEXT(insize, 2)
    }

    return 0;
}

#define BH2S(c1, c2) (((c1) - 0x87) * (0xfe - 0x40 + 1) + ((c2) - 0x40))

DECODER(big5hkscs)
{
    while (inleft > 0) {
        unsigned char c = IN1;
        ucs4_t decoded;

        REQUIRE_OUTBUF(1)

        if (c < 0x80) {
            OUT1(c)
            NEXT(1, 1)
            continue;
        }

        REQUIRE_INBUF(2)

        if (0xc6 > c || c > 0xc8 || (c < 0xc7 && IN2 < 0xa1)) {
            TRYMAP_DEC(big5, **outbuf, c, IN2) {
                NEXT(2, 1)
                continue;
            }
        }

        TRYMAP_DEC(big5hkscs, decoded, c, IN2)
        {
            int s = BH2S(c, IN2);
            const unsigned char *hintbase;

            assert(0x87 <= c && c <= 0xfe);
            assert(0x40 <= IN2 && IN2 <= 0xfe);

            if (BH2S(0x87, 0x40) <= s && s <= BH2S(0xa0, 0xfe)) {
                    hintbase = big5hkscs_phint_0;
                    s -= BH2S(0x87, 0x40);
            }
            else if (BH2S(0xc6,0xa1) <= s && s <= BH2S(0xc8,0xfe)){
                    hintbase = big5hkscs_phint_12130;
                    s -= BH2S(0xc6, 0xa1);
            }
            else if (BH2S(0xf9,0xd6) <= s && s <= BH2S(0xfe,0xfe)){
                    hintbase = big5hkscs_phint_21924;
                    s -= BH2S(0xf9, 0xd6);
            }
            else
                    return MBERR_INTERNAL;

            if (hintbase[s >> 3] & (1 << (s & 7))) {
                    WRITEUCS4(decoded | 0x20000)
                    NEXT_IN(2)
            }
            else {
                    OUT1(decoded)
                    NEXT(2, 1)
            }
            continue;
        }

        switch ((c << 8) | IN2) {
        case 0x8862: WRITE2(0x00ca, 0x0304); break;
        case 0x8864: WRITE2(0x00ca, 0x030c); break;
        case 0x88a3: WRITE2(0x00ea, 0x0304); break;
        case 0x88a5: WRITE2(0x00ea, 0x030c); break;
        default: return 2;
        }

        NEXT(2, 2) /* all decoded codepoints are pairs, above. */
    }

    return 0;
}


BEGIN_MAPPINGS_LIST
  MAPPING_DECONLY(big5hkscs)
  MAPPING_ENCONLY(big5hkscs_bmp)
  MAPPING_ENCONLY(big5hkscs_nonbmp)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS_WINIT(big5hkscs)
END_CODECS_LIST

I_AM_A_MODULE_FOR(hk)
