/*
 * _hz.c: the HZ codec (RFC1843)
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _hz.c,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#include "codeccommon.h"

ENCMAP(gbcommon)
DECMAP(gb2312)

#define HAVE_ENCODER_INIT
ENCODER_INIT(hz)
{
    state->i = 0;
    return 0;
}

#define HAVE_ENCODER_RESET
ENCODER_RESET(hz)
{
    if (state->i != 0) {
        WRITE2('~', '}')
        state->i = 0;
        NEXT_OUT(2)
    }
    return 0;
}

ENCODER(hz)
{
    while (inleft > 0) {
        Py_UNICODE  c = IN1;
        DBCHAR      code;

        if (c < 0x80) {
            if (state->i == 0) {
                WRITE1((unsigned char)c)
                NEXT(1, 1)
            } else {
                WRITE3('~', '}', (unsigned char)c)
                NEXT(1, 3)
                state->i = 0;
            }
            continue;
        }

        UCS4INVALID(c)

        TRYMAP_ENC(gbcommon, code, c);
        else return 1;

        if (code & 0x8000) /* MSB set: GBK */
            return 1;

        if (state->i == 0) {
            WRITE4('~', '{', code >> 8, code & 0xff)
            NEXT(1, 4)
            state->i = 1;
        } else {
            WRITE2(code >> 8, code & 0xff)
            NEXT(1, 2)
        }
    }

    return 0;
}

#define HAVE_DECODER_INIT
DECODER_INIT(hz)
{
    state->i = 0;
    return 0;
}

#define HAVE_DECODER_RESET
DECODER_RESET(hz)
{
    state->i = 0;
    return 0;
}

DECODER(hz)
{
    while (inleft > 0) {
        unsigned char    c = IN1;

        if (c == '~') {
            unsigned char    c2 = IN2;

            RESERVE_INBUF(2)
            if (c2 == '~') {
                WRITE1('~')
                NEXT(2, 1)
                continue;
            } else if (c2 == '{' && state->i == 0)
                state->i = 1; /* set GB */
            else if (c2 == '}' && state->i == 1)
                state->i = 0; /* set ASCII */
            else if (c2 == '\n')
                ; /* line-continuation */
            else
                return 2;
            NEXT(2, 0);
            continue;
        }

        if (c & 0x80)
            return 1;

        if (state->i == 0) { /* ASCII mode */
            WRITE1(c)
            NEXT(1, 1)
        } else { /* GB mode */
            RESERVE_INBUF(2)
            RESERVE_OUTBUF(1)
            TRYMAP_DEC(gb2312, **outbuf, c, IN2) {
                NEXT(2, 1)
            } else
                return 2;
        }
    }

    return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(hz)
    MAPOPEN(zh_CN)
        IMPORTMAP_DEC(gb2312)
        IMPORTMAP_ENC(gbcommon)
    MAPCLOSE()
END_CODEC_REGISTRY(hz)
