/*
   Unicode character type helpers.

   Written by Marc-Andre Lemburg (mal@lemburg.com).
   Modified for Python 2.0 by Fredrik Lundh (fredrik@pythonware.com)

   Copyright (c) Corporation for National Research Initiatives.

*/

#include "Python.h"
#include "unicodeobject.h"

#define ALPHA_MASK 0x01
#define DECIMAL_MASK 0x02
#define DIGIT_MASK 0x04
#define LOWER_MASK 0x08
#define LINEBREAK_MASK 0x10
#define SPACE_MASK 0x20
#define TITLE_MASK 0x40
#define UPPER_MASK 0x80
#define NODELTA_MASK 0x100

typedef struct {
    const Py_UNICODE upper;
    const Py_UNICODE lower;
    const Py_UNICODE title;
    const unsigned char decimal;
    const unsigned char digit;
    const unsigned short flags;
} _PyUnicode_TypeRecord;

#include "unicodetype_db.h"

static const _PyUnicode_TypeRecord *
gettyperecord(Py_UNICODE code)
{
    int index;

#ifdef Py_UNICODE_WIDE
    if (code >= 0x110000)
        index = 0;
    else
#endif
    {
        index = index1[(code>>SHIFT)];
        index = index2[(index<<SHIFT)+(code&((1<<SHIFT)-1))];
    }

    return &_PyUnicode_TypeRecords[index];
}

/* Returns 1 for Unicode characters having the category 'Zl', 'Zp' or
   type 'B', 0 otherwise. */

int _PyUnicode_IsLinebreak(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x000A: /* LINE FEED */
    case 0x000D: /* CARRIAGE RETURN */
    case 0x001C: /* FILE SEPARATOR */
    case 0x001D: /* GROUP SEPARATOR */
    case 0x001E: /* RECORD SEPARATOR */
    case 0x0085: /* NEXT LINE */
    case 0x2028: /* LINE SEPARATOR */
    case 0x2029: /* PARAGRAPH SEPARATOR */
	return 1;
    default:
	return 0;
    }
}

/* Returns the titlecase Unicode characters corresponding to ch or just
   ch if no titlecase mapping is known. */

Py_UNICODE _PyUnicode_ToTitlecase(register Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);
    int delta = ctype->title;

    if (ctype->flags & NODELTA_MASK)
	return delta;

    if (delta >= 32768)
	    delta -= 65536;

    return ch + delta;
}

/* Returns 1 for Unicode characters having the category 'Lt', 0
   otherwise. */

int _PyUnicode_IsTitlecase(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & TITLE_MASK) != 0;
}

/* Returns the integer decimal (0-9) for Unicode characters having
   this property, -1 otherwise. */

int _PyUnicode_ToDecimalDigit(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & DECIMAL_MASK) ? ctype->decimal : -1;
}

int _PyUnicode_IsDecimalDigit(Py_UNICODE ch)
{
    if (_PyUnicode_ToDecimalDigit(ch) < 0)
	return 0;
    return 1;
}

/* Returns the integer digit (0-9) for Unicode characters having
   this property, -1 otherwise. */

int _PyUnicode_ToDigit(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & DIGIT_MASK) ? ctype->digit : -1;
}

int _PyUnicode_IsDigit(Py_UNICODE ch)
{
    if (_PyUnicode_ToDigit(ch) < 0)
	return 0;
    return 1;
}

/* Returns the numeric value as double for Unicode characters having
   this property, -1.0 otherwise. */

/* TODO: replace with unicodetype_db.h table */

double _PyUnicode_ToNumeric(Py_UNICODE ch)
{
    switch (ch) {
    case 0x0F33:
        return (double) -1 / 2;
    case 0x17F0:
    case 0x3007:
#ifdef Py_UNICODE_WIDE
    case 0x1018A:
#endif
	return (double) 0;
    case 0x09F4:
    case 0x17F1:
    case 0x215F:
    case 0x2160:
    case 0x2170:
    case 0x3021:
    case 0x3192:
    case 0x3220:
    case 0x3280:
#ifdef Py_UNICODE_WIDE
    case 0x10107:
    case 0x10142:
    case 0x10158:
    case 0x10159:
    case 0x1015A:
    case 0x10320:
    case 0x103D1:
#endif
	return (double) 1;
    case 0x00BD:
    case 0x0F2A:
    case 0x2CFD:
#ifdef Py_UNICODE_WIDE
    case 0x10141:
    case 0x10175:
    case 0x10176:
#endif
	return (double) 1 / 2;
    case 0x2153:
	return (double) 1 / 3;
    case 0x00BC:
#ifdef Py_UNICODE_WIDE
    case 0x10140:
#endif
	return (double) 1 / 4;
    case 0x2155:
	return (double) 1 / 5;
    case 0x2159:
	return (double) 1 / 6;
    case 0x215B:
	return (double) 1 / 8;
    case 0x0BF0:
    case 0x1372:
    case 0x2169:
    case 0x2179:
    case 0x2469:
    case 0x247D:
    case 0x2491:
    case 0x24FE:
    case 0x277F:
    case 0x2789:
    case 0x2793:
    case 0x3038:
    case 0x3229:
    case 0x3289:
#ifdef Py_UNICODE_WIDE
    case 0x10110:
    case 0x10149:
    case 0x10150:
    case 0x10157:
    case 0x10160:
    case 0x10161:
    case 0x10162:
    case 0x10163:
    case 0x10164:
    case 0x10322:
    case 0x103D3:
    case 0x10A44:
#endif
	return (double) 10;
    case 0x0BF1:
    case 0x137B:
    case 0x216D:
    case 0x217D:
#ifdef Py_UNICODE_WIDE
    case 0x10119:
    case 0x1014B:
    case 0x10152:
    case 0x1016A:
    case 0x103D5:
    case 0x10A46:
#endif
	return (double) 100;
    case 0x0BF2:
    case 0x216F:
    case 0x217F:
    case 0x2180:
#ifdef Py_UNICODE_WIDE
    case 0x10122:
    case 0x1014D:
    case 0x10154:
    case 0x10171:
    case 0x10A47:
#endif
	return (double) 1000;
    case 0x137C:
    case 0x2182:
#ifdef Py_UNICODE_WIDE
    case 0x1012B:
    case 0x10155:
#endif
	return (double) 10000;
    case 0x216A:
    case 0x217A:
    case 0x246A:
    case 0x247E:
    case 0x2492:
    case 0x24EB:
	return (double) 11;
    case 0x0F2F:
        return (double) 11 / 2;
    case 0x216B:
    case 0x217B:
    case 0x246B:
    case 0x247F:
    case 0x2493:
    case 0x24EC:
	return (double) 12;
    case 0x246C:
    case 0x2480:
    case 0x2494:
    case 0x24ED:
	return (double) 13;
    case 0x0F30:
        return (double) 13 / 2;
    case 0x246D:
    case 0x2481:
    case 0x2495:
    case 0x24EE:
	return (double) 14;
    case 0x246E:
    case 0x2482:
    case 0x2496:
    case 0x24EF:
	return (double) 15;
    case 0x0F31:
        return (double) 15 / 2;
    case 0x09F9:
    case 0x246F:
    case 0x2483:
    case 0x2497:
    case 0x24F0:
	return (double) 16;
    case 0x16EE:
    case 0x2470:
    case 0x2484:
    case 0x2498:
    case 0x24F1:
	return (double) 17;
    case 0x0F32:
        return (double) 17 / 2;
    case 0x16EF:
    case 0x2471:
    case 0x2485:
    case 0x2499:
    case 0x24F2:
	return (double) 18;
    case 0x16F0:
    case 0x2472:
    case 0x2486:
    case 0x249A:
    case 0x24F3:
	return (double) 19;
    case 0x09F5:
    case 0x17F2:
    case 0x2161:
    case 0x2171:
    case 0x3022:
    case 0x3193:
    case 0x3221:
    case 0x3281:
#ifdef Py_UNICODE_WIDE
    case 0x10108:
    case 0x1015B:
    case 0x1015C:
    case 0x1015D:
    case 0x1015E:
    case 0x103D2:
#endif
	return (double) 2;
    case 0x2154:
#ifdef Py_UNICODE_WIDE
    case 0x10177:
#endif
	return (double) 2 / 3;
    case 0x2156:
        return (double) 2 / 5;
    case 0x1373:
    case 0x2473:
    case 0x2487:
    case 0x249B:
    case 0x24F4:
    case 0x3039:
#ifdef Py_UNICODE_WIDE
    case 0x10111:
    case 0x103D4:
    case 0x10A45:
#endif
        return (double) 20;
#ifdef Py_UNICODE_WIDE
    case 0x1011A:
        return (double) 200;
    case 0x10123:
        return (double) 2000;
    case 0x1012C:
        return (double) 20000;
#endif
    case 0x3251:
        return (double) 21;
    case 0x3252:
        return (double) 22;
    case 0x3253:
        return (double) 23;
    case 0x3254:
        return (double) 24;
    case 0x3255:
        return (double) 25;
    case 0x3256:
        return (double) 26;
    case 0x3257:
        return (double) 27;
    case 0x3258:
        return (double) 28;
    case 0x3259:
        return (double) 29;
    case 0x09F6:
    case 0x17F3:
    case 0x2162:
    case 0x2172:
    case 0x3023:
    case 0x3194:
    case 0x3222:
    case 0x3282:
#ifdef Py_UNICODE_WIDE
    case 0x10109:
#endif
	return (double) 3;
    case 0x0F2B:
        return (double) 3 / 2;
    case 0x00BE:
#ifdef Py_UNICODE_WIDE
    case 0x10178:
#endif
	return (double) 3 / 4;
    case 0x2157:
	return (double) 3 / 5;
    case 0x215C:
	return (double) 3 / 8;
    case 0x1374:
    case 0x303A:
    case 0x325A:
#ifdef Py_UNICODE_WIDE
    case 0x10112:
    case 0x10165:
#endif
	return (double) 30;
#ifdef Py_UNICODE_WIDE
    case 0x1011B:
    case 0x1016B:
        return (double) 300;
    case 0x10124:
        return (double) 3000;
    case 0x1012D:
        return (double) 30000;
#endif
    case 0x325B:
        return (double) 31;
    case 0x325C:
        return (double) 32;
    case 0x325D:
        return (double) 33;
    case 0x325E:
        return (double) 34;
    case 0x325F:
        return (double) 35;
    case 0x32B1:
        return (double) 36;
    case 0x32B2:
        return (double) 37;
    case 0x32B3:
        return (double) 38;
    case 0x32B4:
        return (double) 39;
    case 0x09F7:
    case 0x17F4:
    case 0x2163:
    case 0x2173:
    case 0x3024:
    case 0x3195:
    case 0x3223:
    case 0x3283:
#ifdef Py_UNICODE_WIDE
    case 0x1010A:
#endif
	return (double) 4;
    case 0x2158:
	return (double) 4 / 5;
    case 0x1375:
    case 0x32B5:
#ifdef Py_UNICODE_WIDE
    case 0x10113:
#endif
        return (double) 40;
#ifdef Py_UNICODE_WIDE
    case 0x1011C:
        return (double) 400;
    case 0x10125:
        return (double) 4000;
    case 0x1012E:
        return (double) 40000;
#endif
    case 0x32B6:
        return (double) 41;
    case 0x32B7:
        return (double) 42;
    case 0x32B8:
        return (double) 43;
    case 0x32B9:
        return (double) 44;
    case 0x32BA:
        return (double) 45;
    case 0x32BB:
        return (double) 46;
    case 0x32BC:
        return (double) 47;
    case 0x32BD:
        return (double) 48;
    case 0x32BE:
        return (double) 49;
    case 0x17F5:
    case 0x2164:
    case 0x2174:
    case 0x3025:
    case 0x3224:
    case 0x3284:
#ifdef Py_UNICODE_WIDE
    case 0x1010B:
    case 0x10143:
    case 0x10148:
    case 0x1014F:
    case 0x1015F:
    case 0x10173:
    case 0x10321:
#endif
	return (double) 5;
    case 0x0F2C:
        return (double) 5 / 2;
    case 0x215A:
	return (double) 5 / 6;
    case 0x215D:
	return (double) 5 / 8;
    case 0x1376:
    case 0x216C:
    case 0x217C:
    case 0x32BF:
#ifdef Py_UNICODE_WIDE
    case 0x10114:
    case 0x10144:
    case 0x1014A:
    case 0x10151:
    case 0x10166:
    case 0x10167:
    case 0x10168:
    case 0x10169:
    case 0x10174:
    case 0x10323:
#endif
	return (double) 50;
    case 0x216E:
    case 0x217E:
#ifdef Py_UNICODE_WIDE
    case 0x1011D:
    case 0x10145:
    case 0x1014C:
    case 0x10153:
    case 0x1016C:
    case 0x1016D:
    case 0x1016E:
    case 0x1016F:
    case 0x10170:
#endif
	return (double) 500;
    case 0x2181:
#ifdef Py_UNICODE_WIDE
    case 0x10126:
    case 0x10146:
    case 0x1014E:
    case 0x10172:
#endif
	return (double) 5000;
#ifdef Py_UNICODE_WIDE
    case 0x1012F:
    case 0x10147:
    case 0x10156:
        return (double) 50000;
#endif
    case 0x17F6:
    case 0x2165:
    case 0x2175:
    case 0x3026:
    case 0x3225:
    case 0x3285:
#ifdef Py_UNICODE_WIDE
    case 0x1010C:
#endif
	return (double) 6;
    case 0x1377:
#ifdef Py_UNICODE_WIDE
    case 0x10115:
#endif
	return (double) 60;
#ifdef Py_UNICODE_WIDE
    case 0x1011E:
        return (double) 600;
    case 0x10127:
        return (double) 6000;
    case 0x10130:
        return (double) 60000;
#endif
    case 0x17F7:
    case 0x2166:
    case 0x2176:
    case 0x3027:
    case 0x3226:
    case 0x3286:
#ifdef Py_UNICODE_WIDE
    case 0x1010D:
#endif
	return (double) 7;
    case 0x0F2D:
        return (double) 7 / 2;
    case 0x215E:
	return (double) 7 / 8;
    case 0x1378:
#ifdef Py_UNICODE_WIDE
    case 0x10116:
#endif
	return (double) 70;
#ifdef Py_UNICODE_WIDE
    case 0x1011F:
        return (double) 700;
    case 0x10128:
        return (double) 7000;
    case 0x10131:
        return (double) 70000;
#endif
    case 0x17F8:
    case 0x2167:
    case 0x2177:
    case 0x3028:
    case 0x3227:
    case 0x3287:
#ifdef Py_UNICODE_WIDE
    case 0x1010E:
#endif
	return (double) 8;
    case 0x1379:
#ifdef Py_UNICODE_WIDE
    case 0x10117:
#endif
	return (double) 80;
#ifdef Py_UNICODE_WIDE
    case 0x10120:
        return (double) 800;
    case 0x10129:
        return (double) 8000;
    case 0x10132:
        return (double) 80000;
#endif
    case 0x17F9:
    case 0x2168:
    case 0x2178:
    case 0x3029:
    case 0x3228:
    case 0x3288:
#ifdef Py_UNICODE_WIDE
    case 0x1010F:
#endif
	return (double) 9;
    case 0x0F2E:
        return (double) 9 / 2;
    case 0x137A:
#ifdef Py_UNICODE_WIDE
    case 0x10118:
#endif
	return (double) 90;
#ifdef Py_UNICODE_WIDE
    case 0x10121:
    case 0x1034A:
        return (double) 900;
    case 0x1012A:
        return (double) 9000;
    case 0x10133:
        return (double) 90000;
#endif
    default:
	return (double) _PyUnicode_ToDigit(ch);
    }
}

int _PyUnicode_IsNumeric(Py_UNICODE ch)
{
    return _PyUnicode_ToNumeric(ch) != -1.0;
}

#ifndef WANT_WCTYPE_FUNCTIONS

/* Returns 1 for Unicode characters having the bidirectional type
   'WS', 'B' or 'S' or the category 'Zs', 0 otherwise. */

int _PyUnicode_IsWhitespace(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0009: /* HORIZONTAL TABULATION */
    case 0x000A: /* LINE FEED */
    case 0x000B: /* VERTICAL TABULATION */
    case 0x000C: /* FORM FEED */
    case 0x000D: /* CARRIAGE RETURN */
    case 0x001C: /* FILE SEPARATOR */
    case 0x001D: /* GROUP SEPARATOR */
    case 0x001E: /* RECORD SEPARATOR */
    case 0x001F: /* UNIT SEPARATOR */
    case 0x0020: /* SPACE */
    case 0x0085: /* NEXT LINE */
    case 0x00A0: /* NO-BREAK SPACE */
    case 0x1680: /* OGHAM SPACE MARK */
    case 0x2000: /* EN QUAD */
    case 0x2001: /* EM QUAD */
    case 0x2002: /* EN SPACE */
    case 0x2003: /* EM SPACE */
    case 0x2004: /* THREE-PER-EM SPACE */
    case 0x2005: /* FOUR-PER-EM SPACE */
    case 0x2006: /* SIX-PER-EM SPACE */
    case 0x2007: /* FIGURE SPACE */
    case 0x2008: /* PUNCTUATION SPACE */
    case 0x2009: /* THIN SPACE */
    case 0x200A: /* HAIR SPACE */
    case 0x200B: /* ZERO WIDTH SPACE */
    case 0x2028: /* LINE SEPARATOR */
    case 0x2029: /* PARAGRAPH SEPARATOR */
    case 0x202F: /* NARROW NO-BREAK SPACE */
    case 0x205F: /* MEDIUM MATHEMATICAL SPACE */
    case 0x3000: /* IDEOGRAPHIC SPACE */
	return 1;
    default:
	return 0;
    }
}

/* Returns 1 for Unicode characters having the category 'Ll', 0
   otherwise. */

int _PyUnicode_IsLowercase(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & LOWER_MASK) != 0;
}

/* Returns 1 for Unicode characters having the category 'Lu', 0
   otherwise. */

int _PyUnicode_IsUppercase(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & UPPER_MASK) != 0;
}

/* Returns the uppercase Unicode characters corresponding to ch or just
   ch if no uppercase mapping is known. */

Py_UNICODE _PyUnicode_ToUppercase(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);
    int delta = ctype->upper;
    if (ctype->flags & NODELTA_MASK)
	return delta;
    if (delta >= 32768)
	    delta -= 65536;
    return ch + delta;
}

/* Returns the lowercase Unicode characters corresponding to ch or just
   ch if no lowercase mapping is known. */

Py_UNICODE _PyUnicode_ToLowercase(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);
    int delta = ctype->lower;
    if (ctype->flags & NODELTA_MASK)
	return delta;
    if (delta >= 32768)
	    delta -= 65536;
    return ch + delta;
}

/* Returns 1 for Unicode characters having the category 'Ll', 'Lu', 'Lt',
   'Lo' or 'Lm',  0 otherwise. */

int _PyUnicode_IsAlpha(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & ALPHA_MASK) != 0;
}

#else

/* Export the interfaces using the wchar_t type for portability
   reasons:  */

int _PyUnicode_IsWhitespace(Py_UNICODE ch)
{
    return iswspace(ch);
}

int _PyUnicode_IsLowercase(Py_UNICODE ch)
{
    return iswlower(ch);
}

int _PyUnicode_IsUppercase(Py_UNICODE ch)
{
    return iswupper(ch);
}

Py_UNICODE _PyUnicode_ToLowercase(Py_UNICODE ch)
{
    return towlower(ch);
}

Py_UNICODE _PyUnicode_ToUppercase(Py_UNICODE ch)
{
    return towupper(ch);
}

int _PyUnicode_IsAlpha(Py_UNICODE ch)
{
    return iswalpha(ch);
}

#endif
