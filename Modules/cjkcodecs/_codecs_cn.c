/*
 * _codecs_cn.c: Codecs collection for Mainland Chinese encodings
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: _codecs_cn.c,v 1.8 2004/07/07 14:59:26 perky Exp $
 */

#include "cjkcodecs.h"
#include "mappings_cn.h"

#define GBK_PREDECODE(dc1, dc2, assi) \
	if ((dc1) == 0xa1 && (dc2) == 0xaa) (assi) = 0x2014; \
	else if ((dc1) == 0xa8 && (dc2) == 0x44) (assi) = 0x2015; \
	else if ((dc1) == 0xa1 && (dc2) == 0xa4) (assi) = 0x00b7;
#define GBK_PREENCODE(code, assi) \
	if ((code) == 0x2014) (assi) = 0xa1aa; \
	else if ((code) == 0x2015) (assi) = 0xa844; \
	else if ((code) == 0x00b7) (assi) = 0xa1a4;

/*
 * GB2312 codec
 */

ENCODER(gb2312)
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
		TRYMAP_ENC(gbcommon, code, c);
		else return 1;

		if (code & 0x8000) /* MSB set: GBK */
			return 1;

		OUT1((code >> 8) | 0x80)
		OUT2((code & 0xFF) | 0x80)
		NEXT(1, 2)
	}

	return 0;
}

DECODER(gb2312)
{
	while (inleft > 0) {
		unsigned char c = **inbuf;

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)
		TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, IN2 ^ 0x80) {
			NEXT(2, 1)
		}
		else return 2;
	}

	return 0;
}


/*
 * GBK codec
 */

ENCODER(gbk)
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

		GBK_PREENCODE(c, code)
		else TRYMAP_ENC(gbcommon, code, c);
		else return 1;

		OUT1((code >> 8) | 0x80)
		if (code & 0x8000)
			OUT2((code & 0xFF)) /* MSB set: GBK */
		else
			OUT2((code & 0xFF) | 0x80) /* MSB unset: GB2312 */
		NEXT(1, 2)
	}

	return 0;
}

DECODER(gbk)
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

		GBK_PREDECODE(c, IN2, **outbuf)
		else TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, IN2 ^ 0x80);
		else TRYMAP_DEC(gbkext, **outbuf, c, IN2);
		else return 2;

		NEXT(2, 1)
	}

	return 0;
}


/*
 * GB18030 codec
 */

ENCODER(gb18030)
{
	while (inleft > 0) {
		ucs4_t c = IN1;
		DBCHAR code;

		if (c < 0x80) {
			WRITE1(c)
			NEXT(1, 1)
			continue;
		}

		DECODE_SURROGATE(c)
		if (c > 0x10FFFF)
#if Py_UNICODE_SIZE == 2
			return 2; /* surrogates pair */
#else
			return 1;
#endif
		else if (c >= 0x10000) {
			ucs4_t tc = c - 0x10000;

			REQUIRE_OUTBUF(4)

			OUT4((unsigned char)(tc % 10) + 0x30)
			tc /= 10;
			OUT3((unsigned char)(tc % 126) + 0x81)
			tc /= 126;
			OUT2((unsigned char)(tc % 10) + 0x30)
			tc /= 10;
			OUT1((unsigned char)(tc + 0x90))

#if Py_UNICODE_SIZE == 2
			NEXT(2, 4) /* surrogates pair */
#else
			NEXT(1, 4)
#endif
			continue;
		}

		REQUIRE_OUTBUF(2)

		GBK_PREENCODE(c, code)
		else TRYMAP_ENC(gbcommon, code, c);
		else TRYMAP_ENC(gb18030ext, code, c);
		else {
			const struct _gb18030_to_unibmp_ranges *utrrange;

			REQUIRE_OUTBUF(4)

			for (utrrange = gb18030_to_unibmp_ranges;
			     utrrange->first != 0;
			     utrrange++)
				if (utrrange->first <= c &&
				    c <= utrrange->last) {
					Py_UNICODE tc;

					tc = c - utrrange->first +
					     utrrange->base;

					OUT4((unsigned char)(tc % 10) + 0x30)
					tc /= 10;
					OUT3((unsigned char)(tc % 126) + 0x81)
					tc /= 126;
					OUT2((unsigned char)(tc % 10) + 0x30)
					tc /= 10;
					OUT1((unsigned char)tc + 0x81)

					NEXT(1, 4)
					break;
				}

			if (utrrange->first == 0) {
				PyErr_SetString(PyExc_RuntimeError,
						"unicode mapping invalid");
				return 1;
			}
			continue;
		}

		OUT1((code >> 8) | 0x80)
		if (code & 0x8000)
			OUT2((code & 0xFF)) /* MSB set: GBK or GB18030ext */
		else
			OUT2((code & 0xFF) | 0x80) /* MSB unset: GB2312 */

		NEXT(1, 2)
	}

	return 0;
}

DECODER(gb18030)
{
	while (inleft > 0) {
		unsigned char c = IN1, c2;

		REQUIRE_OUTBUF(1)

		if (c < 0x80) {
			OUT1(c)
			NEXT(1, 1)
			continue;
		}

		REQUIRE_INBUF(2)

		c2 = IN2;
		if (c2 >= 0x30 && c2 <= 0x39) { /* 4 bytes seq */
			const struct _gb18030_to_unibmp_ranges *utr;
			unsigned char c3, c4;
			ucs4_t lseq;

			REQUIRE_INBUF(4)
			c3 = IN3;
			c4 = IN4;
			if (c < 0x81 || c3 < 0x81 || c4 < 0x30 || c4 > 0x39)
				return 4;
			c -= 0x81;  c2 -= 0x30;
			c3 -= 0x81; c4 -= 0x30;

			if (c < 4) { /* U+0080 - U+FFFF */
				lseq = ((ucs4_t)c * 10 + c2) * 1260 +
					(ucs4_t)c3 * 10 + c4;
				if (lseq < 39420) {
					for (utr = gb18030_to_unibmp_ranges;
					     lseq >= (utr + 1)->base;
					     utr++) ;
					OUT1(utr->first - utr->base + lseq)
					NEXT(4, 1)
					continue;
				}
			}
			else if (c >= 15) { /* U+10000 - U+10FFFF */
				lseq = 0x10000 + (((ucs4_t)c-15) * 10 + c2)
					* 1260 + (ucs4_t)c3 * 10 + c4;
				if (lseq <= 0x10FFFF) {
					WRITEUCS4(lseq);
					NEXT_IN(4)
					continue;
				}
			}
			return 4;
		}

		GBK_PREDECODE(c, c2, **outbuf)
		else TRYMAP_DEC(gb2312, **outbuf, c ^ 0x80, c2 ^ 0x80);
		else TRYMAP_DEC(gbkext, **outbuf, c, c2);
		else TRYMAP_DEC(gb18030ext, **outbuf, c, c2);
		else return 2;

		NEXT(2, 1)
	}

	return 0;
}


/*
 * HZ codec
 */

ENCODER_INIT(hz)
{
	state->i = 0;
	return 0;
}

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
		Py_UNICODE c = IN1;
		DBCHAR code;

		if (c < 0x80) {
			if (state->i == 0) {
				WRITE1((unsigned char)c)
				NEXT(1, 1)
			}
			else {
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
		}
		else {
			WRITE2(code >> 8, code & 0xff)
			NEXT(1, 2)
		}
	}

	return 0;
}

DECODER_INIT(hz)
{
	state->i = 0;
	return 0;
}

DECODER_RESET(hz)
{
	state->i = 0;
	return 0;
}

DECODER(hz)
{
	while (inleft > 0) {
		unsigned char c = IN1;

		if (c == '~') {
			unsigned char c2 = IN2;

			REQUIRE_INBUF(2)
			if (c2 == '~') {
				WRITE1('~')
				NEXT(2, 1)
				continue;
			}
			else if (c2 == '{' && state->i == 0)
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
		}
		else { /* GB mode */
			REQUIRE_INBUF(2)
			REQUIRE_OUTBUF(1)
			TRYMAP_DEC(gb2312, **outbuf, c, IN2) {
				NEXT(2, 1)
			}
			else
				return 2;
		}
	}

	return 0;
}


BEGIN_MAPPINGS_LIST
  MAPPING_DECONLY(gb2312)
  MAPPING_DECONLY(gbkext)
  MAPPING_ENCONLY(gbcommon)
  MAPPING_ENCDEC(gb18030ext)
END_MAPPINGS_LIST

BEGIN_CODECS_LIST
  CODEC_STATELESS(gb2312)
  CODEC_STATELESS(gbk)
  CODEC_STATELESS(gb18030)
  CODEC_STATEFUL(hz)
END_CODECS_LIST

I_AM_A_MODULE_FOR(cn)
