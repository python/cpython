/*
 * _codecs_hk.c: Codecs collection for encodings from Hong Kong
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _codecs_hk.c,v 1.4 2004/07/18 04:44:27 perky Exp $
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

ENCODER(big5hkscs)
{
	while (inleft > 0) {
		ucs4_t c = **inbuf;
		DBCHAR code;
		int insize;

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
			TRYMAP_ENC(big5hkscs_bmp, code, c);
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

#define BH2S(c1, c2) (((c1) - 0x88) * (0xfe - 0x40 + 1) + ((c2) - 0x40))

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

		if (0xc6 <= c && c <= 0xc8 && (c >= 0xc7 || IN2 >= 0xa1))
			goto hkscsdec;

		TRYMAP_DEC(big5, **outbuf, c, IN2) {
			NEXT(2, 1)
		}
		else
hkscsdec:	TRYMAP_DEC(big5hkscs, decoded, c, IN2) {
			int s = BH2S(c, IN2);
			const unsigned char *hintbase;

			assert(0x88 <= c && c <= 0xfe);
			assert(0x40 <= IN2 && IN2 <= 0xfe);

			if (BH2S(0x88, 0x40) <= s && s <= BH2S(0xa0, 0xfe)) {
				hintbase = big5hkscs_phint_0;
				s -= BH2S(0x88, 0x40);
			}
			else if (BH2S(0xc6,0xa1) <= s && s <= BH2S(0xc8,0xfe)){
				hintbase = big5hkscs_phint_11939;
				s -= BH2S(0xc6, 0xa1);
			}
			else if (BH2S(0xf9,0xd6) <= s && s <= BH2S(0xfe,0xfe)){
				hintbase = big5hkscs_phint_21733;
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
		}
		else return 2;
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
