/*
 * _codecs_kr.c: Codecs collection for Korean encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _codecs_kr.c,v 1.8 2004/07/07 14:59:26 perky Exp $
 */

#include "cjkcodecs.h"
#include "mappings_kr.h"

/*
 * EUC-KR codec
 */

ENCODER(euc_kr)
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
		TRYMAP_ENC(cp949, code, c);
		else return 1;

		if (code & 0x8000) /* MSB set: CP949 */
			return 1;

		OUT1((code >> 8) | 0x80)
		OUT2((code & 0xFF) | 0x80)
		NEXT(1, 2)
	}

	return 0;
}

DECODER(euc_kr)
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

		TRYMAP_DEC(ksx1001, **outbuf, c ^ 0x80, IN2 ^ 0x80) {
			NEXT(2, 1)
		} else return 2;
	}

	return 0;
}


/*
 * CP949 codec
 */

ENCODER(cp949)
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

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)
		TRYMAP_DEC(ksx1001, **outbuf, c ^ 0x80, IN2 ^ 0x80);
		else TRYMAP_DEC(cp949ext, **outbuf, c, IN2);
		else return 2;

		NEXT(2, 1)
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
		Py_UNICODE c = IN1;
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

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
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
				return 2;

			/* we don't use U+1100 hangul jamo yet. */
			if (i_cho == FILL) {
				if (i_jung == FILL) {
					if (i_jong == FILL)
						OUT1(0x3000)
					else
						OUT1(0x3100 |
						  johabjamo_jongseong[c_jong])
				}
				else {
					if (i_jong == FILL)
						OUT1(0x3100 |
						  johabjamo_jungseong[c_jung])
					else
						return 2;
				}
			} else {
				if (i_jung == FILL) {
					if (i_jong == FILL)
						OUT1(0x3100 |
						  johabjamo_choseong[c_cho])
					else
						return 2;
				}
				else
					OUT1(0xac00 +
					     i_cho * 588 +
					     i_jung * 28 +
					     (i_jong == FILL ? 0 : i_jong))
			}
			NEXT(2, 1)
		} else {
			/* KS X 1001 except hangul jamos and syllables */
			if (c == 0xdf || c > 0xf9 ||
			    c2 < 0x31 || (c2 >= 0x80 && c2 < 0x91) ||
			    (c2 & 0x7f) == 0x7f ||
			    (c == 0xda && (c2 >= 0xa1 && c2 <= 0xd3)))
				return 2;
			else {
				unsigned char t1, t2;

				t1 = (c < 0xe0 ? 2 * (c - 0xd9) :
						 2 * c - 0x197);
				t2 = (c2 < 0x91 ? c2 - 0x31 : c2 - 0x43);
				t1 = t1 + (t2 < 0x5e ? 0 : 1) + 0x21;
				t2 = (t2 < 0x5e ? t2 : t2 - 0x5e) + 0x21;

				TRYMAP_DEC(ksx1001, **outbuf, t1, t2);
				else return 2;
				NEXT(2, 1)
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
