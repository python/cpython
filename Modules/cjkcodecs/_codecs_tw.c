/*
 * _codecs_tw.c: Codecs collection for Taiwan's encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _codecs_tw.c,v 1.10 2004/07/07 14:59:26 perky Exp $
 */

#include "cjkcodecs.h"
#include "mappings_tw.h"

/*
 * BIG5 codec
 */

ENCODER(big5)
{
	while (inleft > 0) {
		Py_UNICODE c = **inbuf;
		DBCHAR code;

		if (c < 0x80) {
			REQUIRE_OUTBUF(1)
			**outbuf = (unsigned char)c;
			NEXT(1, 1)
			continue;
		}
		UCS4INVALID(c)

		REQUIRE_OUTBUF(2)

		TRYMAP_ENC(big5, code, c);
		else return 1;

		OUT1(code >> 8)
		OUT2(code & 0xFF)
		NEXT(1, 2)
	}

	return 0;
}

DECODER(big5)
{
	while (inleft > 0) {
		unsigned char c = IN1;

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)
		TRYMAP_DEC(big5, **outbuf, c, IN2) {
			NEXT(2, 1)
		}
		else return 2;
	}

	return 0;
}


/*
 * CP950 codec
 */

ENCODER(cp950)
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

		REQUIRE_OUTBUF(2)
		TRYMAP_ENC(cp950ext, code, c);
		else TRYMAP_ENC(big5, code, c);
		else return 1;

		OUT1(code >> 8)
		OUT2(code & 0xFF)
		NEXT(1, 2)
	}

	return 0;
}

DECODER(cp950)
{
	while (inleft > 0) {
		unsigned char c = IN1;

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)

		TRYMAP_DEC(cp950ext, **outbuf, c, IN2);
		else TRYMAP_DEC(big5, **outbuf, c, IN2);
		else return 2;

		NEXT(2, 1)
	}

	return 0;
}



BEGIN_MAPPINGS_LIST
  MAPPING_ENCDEC(big5)
  MAPPING_ENCDEC(cp950ext)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS(big5)
  CODEC_STATELESS(cp950)
END_CODECS_LIST

I_AM_A_MODULE_FOR(tw)
