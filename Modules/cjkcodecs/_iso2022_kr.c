/*
 * _iso2022_kr.c: the ISO-2022-KR codec (RFC1557)
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _iso2022_kr.c,v 1.3 2003/12/31 05:46:55 perky Exp $
 */

#define ISO2022_DESIGNATIONS \
        CHARSET_ASCII, CHARSET_KSX1001

#include "codeccommon.h"
#include "iso2022common.h"

ENCMAP(cp949)
DECMAP(ksx1001)

#define HAVE_ENCODER_INIT
ENCODER_INIT(iso2022_kr)
{
    STATE_CLEARFLAGS(state)
    STATE_SETG0(state, CHARSET_ASCII)
    STATE_SETG1(state, CHARSET_ASCII)
    return 0;
}

#define HAVE_ENCODER_RESET
ENCODER_RESET(iso2022_kr)
{
    if (STATE_GETFLAG(state, F_SHIFTED)) {
        RESERVE_OUTBUF(1)
        OUT1(SI)
        NEXT_OUT(1)
        STATE_CLEARFLAG(state, F_SHIFTED)
    }
    return 0;
}

ENCODER(iso2022_kr)
{
    while (inleft > 0) {
        Py_UNICODE  c = **inbuf;
        DBCHAR      code;

        if (c < 0x80) {
            if (STATE_GETFLAG(state, F_SHIFTED)) {
                WRITE2(SI, c)
                STATE_CLEARFLAG(state, F_SHIFTED)
                NEXT(1, 2)
            } else {
                WRITE1(c)
                NEXT(1, 1)
            }
            if (c == '\n')
                STATE_CLEARFLAG(state, F_SHIFTED)
        } else UCS4INVALID(c)
        else {
            if (STATE_GETG1(state) != CHARSET_KSX1001) {
                WRITE4(ESC, '$', ')', 'C')
                STATE_SETG1(state, CHARSET_KSX1001)
                NEXT_OUT(4)
            }

            if (!STATE_GETFLAG(state, F_SHIFTED)) {
                WRITE1(SO)
                STATE_SETFLAG(state, F_SHIFTED)
                NEXT_OUT(1)
            }

            TRYMAP_ENC(cp949, code, c) {
                if (code & 0x8000) /* MSB set: CP949 */
                    return 1;
                WRITE2(code >> 8, code & 0xff)
                NEXT(1, 2)
            } else
                return 1;
        }
    }

    return 0;
}

#define HAVE_DECODER_INIT
DECODER_INIT(iso2022_kr)
{
    STATE_CLEARFLAGS(state)
    STATE_SETG0(state, CHARSET_ASCII)
    STATE_SETG1(state, CHARSET_ASCII)
    return 0;
}

#define HAVE_DECODER_RESET
DECODER_RESET(iso2022_kr)
{
    STATE_CLEARFLAG(state, F_SHIFTED)
    return 0;
}

DECODER(iso2022_kr)
{
  ISO2022_LOOP_BEGIN
    unsigned char    charset, c2;

    ISO2022_GETCHARSET(charset, c)

    if (charset & CHARSET_DOUBLEBYTE) {
        /* all double byte character sets are in KS X 1001 here */
        RESERVE_INBUF(2)
        RESERVE_OUTBUF(1)
        c2 = IN2;
        if (c2 >= 0x80)
            return 1;
        TRYMAP_DEC(ksx1001, **outbuf, c, c2) {
            NEXT(2, 1)
        } else
            return 2;
    } else {
        RESERVE_OUTBUF(1)
        OUT1(c);
        NEXT(1, 1)
    }
  ISO2022_LOOP_END
  return 0;
}

#include "codecentry.h"
BEGIN_CODEC_REGISTRY(iso2022_kr)
    MAPOPEN(ko_KR)
        IMPORTMAP_DEC(ksx1001)
        IMPORTMAP_ENC(cp949)
    MAPCLOSE()
END_CODEC_REGISTRY(iso2022_kr)
