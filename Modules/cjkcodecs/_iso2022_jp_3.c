/*
 * _iso2022_jp_3.c: the ISO-2022-JP-3 codec (JIS X 0213)
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _iso2022_jp_3.c,v 1.7 2003/12/31 05:46:55 perky Exp $
 */

#define USING_BINARY_PAIR_SEARCH
#define ISO2022_DESIGNATIONS    \
        CHARSET_ASCII, CHARSET_JISX0208, CHARSET_JISX0213_1, CHARSET_JISX0213_2
#define ISO2022_NO_SHIFT
#define ISO2022_USE_JISX0208EXT

#include "codeccommon.h"
#include "iso2022common.h"
#include "map_jisx0213_pairs.h"

ENCMAP(jisxcommon)
DECMAP(jisx0208)
DECMAP(jisx0212)
ENCMAP(jisx0213_bmp)
DECMAP(jisx0213_1_bmp)
DECMAP(jisx0213_2_bmp)
ENCMAP(jisx0213_emp)
DECMAP(jisx0213_1_emp)
DECMAP(jisx0213_2_emp)

#define EMPBASE     0x20000

#define HAVE_ENCODER_INIT
ENCODER_INIT(iso2022_jp_3)
{
    STATE_CLEARFLAGS(state)
    STATE_SETG0(state, CHARSET_ASCII)
    STATE_SETG1(state, CHARSET_ASCII)
    return 0;
}

#define HAVE_ENCODER_RESET
ENCODER_RESET(iso2022_jp_3)
{
    if (STATE_GETG0(state) != CHARSET_ASCII) {
        WRITE3(ESC, '(', 'B')
        STATE_SETG0(state, CHARSET_ASCII)
        NEXT_OUT(3)
    }
    return 0;
}

ENCODER(iso2022_jp_3)
{
  while (inleft > 0) {
    unsigned char charset;
    ucs4_t   c = IN1;
    DBCHAR   code;
    size_t   insize;

    if (c < 0x80) {
        switch (STATE_GETG0(state)) {
        case CHARSET_ASCII:
            WRITE1(c)
            NEXT(1, 1)
            break;
        default:
            WRITE4(ESC, '(', 'B', c)
            STATE_SETG0(state, CHARSET_ASCII)
            NEXT(1, 4)
            break;
        }
        if (c == '\n')
            STATE_CLEARFLAG(state, F_SHIFTED)
        continue;
    }

    DECODE_SURROGATE(c)
    insize = GET_INSIZE(c);

    if (c <= 0xffff) {
        TRYMAP_ENC(jisx0213_bmp, code, c) {
            if (code == MULTIC) {
                if (inleft < 2) {
                    if (flags & MBENC_FLUSH) {
                        code = find_pairencmap(c, 0, jisx0213_pairencmap,
                                            JISX0213_ENCPAIRS);
                        if (code == DBCINV)
                            return 1;
                    } else
                        return MBERR_TOOFEW;
                } else {
                    code = find_pairencmap(c, IN2,
                                jisx0213_pairencmap, JISX0213_ENCPAIRS);
                    if (code == DBCINV) {
                        code = find_pairencmap(c, 0, jisx0213_pairencmap,
                                            JISX0213_ENCPAIRS);
                        if (code == DBCINV)
                            return 1;
                    } else
                        insize = 2;
                }
            }
        } else TRYMAP_ENC(jisxcommon, code, c) {
            if (code & 0x8000)
                return 1; /* avoid JIS X 0212 codes */
        } else if (c == 0xff3c) /* F/W REVERSE SOLIDUS */
            code = 0x2140;
        else
            return 1;
    } else if (c >> 16 == EMPBASE >> 16) {
        TRYMAP_ENC(jisx0213_emp, code, c & 0xffff);
        else return insize;
    } else
        return insize;

    charset = STATE_GETG0(state);
    if (code & 0x8000) { /* MSB set: Plane 2 */
        if (charset != CHARSET_JISX0213_2) {
            WRITE4(ESC, '$', '(', 'P')
            STATE_SETG0(state, CHARSET_JISX0213_2)
            NEXT_OUT(4)
        }
        WRITE2((code >> 8) & 0x7f, code & 0x7f)
    } else { /* MSB unset: Plane 1 */
        if (charset != CHARSET_JISX0213_1) {
            WRITE4(ESC, '$', '(', 'O')
            STATE_SETG0(state, CHARSET_JISX0213_1)
            NEXT_OUT(4)
        }
        WRITE2(code >> 8, code & 0xff)
    }
    NEXT(insize, 2)
  }

  return 0;
}

#define HAVE_DECODER_INIT
DECODER_INIT(iso2022_jp_3)
{
    STATE_CLEARFLAGS(state)
    STATE_SETG0(state, CHARSET_ASCII)
    STATE_SETG1(state, CHARSET_ASCII)
    return 0;
}

#define HAVE_DECODER_RESET
DECODER_RESET(iso2022_jp_3)
{
    STATE_CLEARFLAG(state, F_SHIFTED)
    return 0;
}

DECODER(iso2022_jp_3)
{
  ISO2022_LOOP_BEGIN
    unsigned char    charset, c2;
    ucs4_t           code;

    ISO2022_GETCHARSET(charset, c)

    if (charset & CHARSET_DOUBLEBYTE) {
        RESERVE_INBUF(2)
        RESERVE_OUTBUF(1)
        c2 = IN2;
        if (charset == CHARSET_JISX0213_1) {
            if (c == 0x21 && c2 == 0x40) **outbuf = 0xff3c;
            else TRYMAP_DEC(jisx0208, **outbuf, c, c2);
            else TRYMAP_DEC(jisx0213_1_bmp, **outbuf, c, c2);
            else TRYMAP_DEC(jisx0213_1_emp, code, c, c2) {
                PUTUCS4(EMPBASE | code)
                NEXT_IN(2)
                continue;
            } else TRYMAP_DEC(jisx0213_pair, code, c, c2) {
                WRITE2(code >> 16, code & 0xffff)
                NEXT(2, 2)
                continue;
            } else return 2;
        } else if (charset == CHARSET_JISX0213_2) {
            TRYMAP_DEC(jisx0213_2_bmp, **outbuf, c, c2);
            else TRYMAP_DEC(jisx0213_2_emp, code, c, c2) {
                PUTUCS4(EMPBASE | code)
                NEXT_IN(2)
                continue;
            } else return 2;
        } else
            return MBERR_INTERNAL;
        NEXT(2, 1)
    } else if (charset == CHARSET_ASCII) {
        RESERVE_OUTBUF(1)
        OUT1(c)
        NEXT(1, 1)
    } else
        return MBERR_INTERNAL;
  ISO2022_LOOP_END

  return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(iso2022_jp_3)
    MAPOPEN(ja_JP)
        IMPORTMAP_DEC(jisx0208)
        IMPORTMAP_DEC(jisx0212)
        IMPORTMAP_ENC(jisxcommon)
        IMPORTMAP_ENC(jisx0213_bmp)
        IMPORTMAP_DEC(jisx0213_1_bmp)
        IMPORTMAP_DEC(jisx0213_2_bmp)
        IMPORTMAP_ENC(jisx0213_emp)
        IMPORTMAP_DEC(jisx0213_1_emp)
        IMPORTMAP_DEC(jisx0213_2_emp)
    MAPCLOSE()
END_CODEC_REGISTRY(iso2022_jp_3)
