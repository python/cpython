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

/* Returns 1 for Unicode characters having the category 'Zl' or type
   'B', 0 otherwise. */

int _PyUnicode_IsLinebreak(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & LINEBREAK_MASK) != 0;
}

/* Returns the titlecase Unicode characters corresponding to ch or just
   ch if no titlecase mapping is known. */

Py_UNICODE _PyUnicode_ToTitlecase(register Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);
    int delta;

    if (ctype->title)
        delta = ctype->title;
    else
	delta = ctype->upper;

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
    case 0x3007:
	return (double) 0;
    case 0x09F4:
    case 0x215F:
    case 0x2160:
    case 0x2170:
    case 0x3021:
    case 0x3280:
	return (double) 1;
    case 0x00BD:
	return (double) 1 / 2;
    case 0x2153:
	return (double) 1 / 3;
    case 0x00BC:
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
    case 0x277F:
    case 0x2789:
    case 0x2793:
    case 0x3038:
    case 0x3289:
	return (double) 10;
    case 0x0BF1:
    case 0x137B:
    case 0x216D:
    case 0x217D:
	return (double) 100;
    case 0x0BF2:
    case 0x216F:
    case 0x217F:
    case 0x2180:
	return (double) 1000;
    case 0x137C:
    case 0x2182:
	return (double) 10000;
    case 0x216A:
    case 0x217A:
    case 0x246A:
    case 0x247E:
    case 0x2492:
	return (double) 11;
    case 0x216B:
    case 0x217B:
    case 0x246B:
    case 0x247F:
    case 0x2493:
	return (double) 12;
    case 0x246C:
    case 0x2480:
    case 0x2494:
	return (double) 13;
    case 0x246D:
    case 0x2481:
    case 0x2495:
	return (double) 14;
    case 0x246E:
    case 0x2482:
    case 0x2496:
	return (double) 15;
    case 0x09F9:
    case 0x246F:
    case 0x2483:
    case 0x2497:
	return (double) 16;
    case 0x16EE:
    case 0x2470:
    case 0x2484:
    case 0x2498:
	return (double) 17;
    case 0x16EF:
    case 0x2471:
    case 0x2485:
    case 0x2499:
	return (double) 18;
    case 0x16F0:
    case 0x2472:
    case 0x2486:
    case 0x249A:
	return (double) 19;
    case 0x09F5:
    case 0x2161:
    case 0x2171:
    case 0x3022:
    case 0x3281:
	return (double) 2;
    case 0x2154:
	return (double) 2 / 3;
    case 0x2156:
	return (double) 2 / 5;
    case 0x1373:
    case 0x2473:
    case 0x2487:
    case 0x249B:
    case 0x3039:
	return (double) 20;
    case 0x09F6:
    case 0x2162:
    case 0x2172:
    case 0x3023:
    case 0x3282:
	return (double) 3;
    case 0x00BE:
	return (double) 3 / 4;
    case 0x2157:
	return (double) 3 / 5;
    case 0x215C:
	return (double) 3 / 8;
    case 0x1374:
    case 0x303A:
	return (double) 30;
    case 0x09F7:
    case 0x2163:
    case 0x2173:
    case 0x3024:
    case 0x3283:
	return (double) 4;
    case 0x2158:
	return (double) 4 / 5;
    case 0x1375:
	return (double) 40;
    case 0x2164:
    case 0x2174:
    case 0x3025:
    case 0x3284:
	return (double) 5;
    case 0x215A:
	return (double) 5 / 6;
    case 0x215D:
	return (double) 5 / 8;
    case 0x1376:
    case 0x216C:
    case 0x217C:
	return (double) 50;
    case 0x216E:
    case 0x217E:
	return (double) 500;
    case 0x2181:
	return (double) 5000;
    case 0x2165:
    case 0x2175:
    case 0x3026:
    case 0x3285:
	return (double) 6;
    case 0x1377:
	return (double) 60;
    case 0x2166:
    case 0x2176:
    case 0x3027:
    case 0x3286:
	return (double) 7;
    case 0x215E:
	return (double) 7 / 8;
    case 0x1378:
	return (double) 70;
    case 0x2167:
    case 0x2177:
    case 0x3028:
    case 0x3287:
	return (double) 8;
    case 0x1379:
	return (double) 80;
    case 0x2168:
    case 0x2178:
    case 0x3029:
    case 0x3288:
	return (double) 9;
    case 0x137A:
	return (double) 90;
    default:
	return (double) _PyUnicode_ToDigit(ch);
    }
}

int _PyUnicode_IsNumeric(Py_UNICODE ch)
{
    if (_PyUnicode_ToNumeric(ch) < 0.0)
	return 0;
    return 1;
}

#ifndef WANT_WCTYPE_FUNCTIONS

/* Returns 1 for Unicode characters having the bidirectional type
   'WS', 'B' or 'S' or the category 'Zs', 0 otherwise. */

int _PyUnicode_IsWhitespace(Py_UNICODE ch)
{
    const _PyUnicode_TypeRecord *ctype = gettyperecord(ch);

    return (ctype->flags & SPACE_MASK) != 0;
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
