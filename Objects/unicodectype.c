/*
   Unicode character type helpers.

   The data contained in the function's switch tables was extracted
   from the Unicode 3.0 data file.

Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

*/

#include "Python.h"

#include "unicodeobject.h"

#if defined(macintosh) || defined(MS_WIN64)
/*XXX This was required to avoid a compiler error for an early Win64
 * cross-compiler that was used for the port to Win64. When the platform is
 * released the MS_WIN64 inclusion here should no longer be necessary.
 */
/* This probably needs to be defined for some other compilers too. It breaks the
** 5000-label switch statement up into switches with around 1000 cases each.
*/
#define BREAK_SWITCH_UP	return 1; } switch (ch) {
#else
#define BREAK_SWITCH_UP /* nothing */
#endif


/* Returns 1 for Unicode characters having the category 'Zl' or type
   'B', 0 otherwise. */

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

Py_UNICODE _PyUnicode_ToTitlecase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x01C4: /* LATIN CAPITAL LETTER DZ WITH CARON */
        return (Py_UNICODE)0x01C5;
    case 0x01C6: /* LATIN SMALL LETTER DZ WITH CARON */
        return (Py_UNICODE)0x01C5;
    case 0x01C7: /* LATIN CAPITAL LETTER LJ */
        return (Py_UNICODE)0x01C8;
    case 0x01C9: /* LATIN SMALL LETTER LJ */
        return (Py_UNICODE)0x01C8;
    case 0x01CA: /* LATIN CAPITAL LETTER NJ */
        return (Py_UNICODE)0x01CB;
    case 0x01CC: /* LATIN SMALL LETTER NJ */
        return (Py_UNICODE)0x01CB;
    case 0x01F1: /* LATIN CAPITAL LETTER DZ */
        return (Py_UNICODE)0x01F2;
    case 0x01F3: /* LATIN SMALL LETTER DZ */
        return (Py_UNICODE)0x01F2;
    default:
	return Py_UNICODE_TOUPPER(ch);
    }
}

/* Returns 1 for Unicode characters having the category 'Lt', 0
   otherwise. */

int _PyUnicode_IsTitlecase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x01C5: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON */
    case 0x01C8: /* LATIN CAPITAL LETTER L WITH SMALL LETTER J */
    case 0x01CB: /* LATIN CAPITAL LETTER N WITH SMALL LETTER J */
    case 0x01F2: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z */
    case 0x1F88: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PROSGEGRAMMENI */
    case 0x1F89: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PROSGEGRAMMENI */
    case 0x1F8A: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
    case 0x1F8B: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
    case 0x1F8C: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
    case 0x1F8D: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
    case 0x1F8E: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1F8F: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1F98: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PROSGEGRAMMENI */
    case 0x1F99: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PROSGEGRAMMENI */
    case 0x1F9A: /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
    case 0x1F9B: /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
    case 0x1F9C: /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
    case 0x1F9D: /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
    case 0x1F9E: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1F9F: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1FA8: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PROSGEGRAMMENI */
    case 0x1FA9: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PROSGEGRAMMENI */
    case 0x1FAA: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
    case 0x1FAB: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
    case 0x1FAC: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
    case 0x1FAD: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
    case 0x1FAE: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1FAF: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
    case 0x1FBC: /* GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI */
    case 0x1FCC: /* GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI */
    case 0x1FFC: /* GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI */
	return 1;
    default:
	return 0;
    }
}

/* Returns the integer decimal (0-9) for Unicode characters having
   this property, -1 otherwise. */

int _PyUnicode_ToDecimalDigit(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0030:
    case 0x0660:
    case 0x06F0:
    case 0x0966:
    case 0x09E6:
    case 0x0A66:
    case 0x0AE6:
    case 0x0B66:
    case 0x0C66:
    case 0x0CE6:
    case 0x0D66:
    case 0x0E50:
    case 0x0ED0:
    case 0x0F20:
    case 0x1040:
    case 0x17E0:
    case 0x1810:
    case 0x2070:
    case 0x2080:
    case 0xFF10:
	return 0;
    case 0x0031:
    case 0x00B9:
    case 0x0661:
    case 0x06F1:
    case 0x0967:
    case 0x09E7:
    case 0x0A67:
    case 0x0AE7:
    case 0x0B67:
    case 0x0BE7:
    case 0x0C67:
    case 0x0CE7:
    case 0x0D67:
    case 0x0E51:
    case 0x0ED1:
    case 0x0F21:
    case 0x1041:
    case 0x1369:
    case 0x17E1:
    case 0x1811:
    case 0x2081:
    case 0xFF11:
	return 1;
    case 0x0032:
    case 0x00B2:
    case 0x0662:
    case 0x06F2:
    case 0x0968:
    case 0x09E8:
    case 0x0A68:
    case 0x0AE8:
    case 0x0B68:
    case 0x0BE8:
    case 0x0C68:
    case 0x0CE8:
    case 0x0D68:
    case 0x0E52:
    case 0x0ED2:
    case 0x0F22:
    case 0x1042:
    case 0x136A:
    case 0x17E2:
    case 0x1812:
    case 0x2082:
    case 0xFF12:
	return 2;
    case 0x0033:
    case 0x00B3:
    case 0x0663:
    case 0x06F3:
    case 0x0969:
    case 0x09E9:
    case 0x0A69:
    case 0x0AE9:
    case 0x0B69:
    case 0x0BE9:
    case 0x0C69:
    case 0x0CE9:
    case 0x0D69:
    case 0x0E53:
    case 0x0ED3:
    case 0x0F23:
    case 0x1043:
    case 0x136B:
    case 0x17E3:
    case 0x1813:
    case 0x2083:
    case 0xFF13:
	return 3;
    case 0x0034:
    case 0x0664:
    case 0x06F4:
    case 0x096A:
    case 0x09EA:
    case 0x0A6A:
    case 0x0AEA:
    case 0x0B6A:
    case 0x0BEA:
    case 0x0C6A:
    case 0x0CEA:
    case 0x0D6A:
    case 0x0E54:
    case 0x0ED4:
    case 0x0F24:
    case 0x1044:
    case 0x136C:
    case 0x17E4:
    case 0x1814:
    case 0x2074:
    case 0x2084:
    case 0xFF14:
	return 4;
    case 0x0035:
    case 0x0665:
    case 0x06F5:
    case 0x096B:
    case 0x09EB:
    case 0x0A6B:
    case 0x0AEB:
    case 0x0B6B:
    case 0x0BEB:
    case 0x0C6B:
    case 0x0CEB:
    case 0x0D6B:
    case 0x0E55:
    case 0x0ED5:
    case 0x0F25:
    case 0x1045:
    case 0x136D:
    case 0x17E5:
    case 0x1815:
    case 0x2075:
    case 0x2085:
    case 0xFF15:
	return 5;
    case 0x0036:
    case 0x0666:
    case 0x06F6:
    case 0x096C:
    case 0x09EC:
    case 0x0A6C:
    case 0x0AEC:
    case 0x0B6C:
    case 0x0BEC:
    case 0x0C6C:
    case 0x0CEC:
    case 0x0D6C:
    case 0x0E56:
    case 0x0ED6:
    case 0x0F26:
    case 0x1046:
    case 0x136E:
    case 0x17E6:
    case 0x1816:
    case 0x2076:
    case 0x2086:
    case 0xFF16:
	return 6;
    case 0x0037:
    case 0x0667:
    case 0x06F7:
    case 0x096D:
    case 0x09ED:
    case 0x0A6D:
    case 0x0AED:
    case 0x0B6D:
    case 0x0BED:
    case 0x0C6D:
    case 0x0CED:
    case 0x0D6D:
    case 0x0E57:
    case 0x0ED7:
    case 0x0F27:
    case 0x1047:
    case 0x136F:
    case 0x17E7:
    case 0x1817:
    case 0x2077:
    case 0x2087:
    case 0xFF17:
	return 7;
    case 0x0038:
    case 0x0668:
    case 0x06F8:
    case 0x096E:
    case 0x09EE:
    case 0x0A6E:
    case 0x0AEE:
    case 0x0B6E:
    case 0x0BEE:
    case 0x0C6E:
    case 0x0CEE:
    case 0x0D6E:
    case 0x0E58:
    case 0x0ED8:
    case 0x0F28:
    case 0x1048:
    case 0x1370:
    case 0x17E8:
    case 0x1818:
    case 0x2078:
    case 0x2088:
    case 0xFF18:
	return 8;
    case 0x0039:
    case 0x0669:
    case 0x06F9:
    case 0x096F:
    case 0x09EF:
    case 0x0A6F:
    case 0x0AEF:
    case 0x0B6F:
    case 0x0BEF:
    case 0x0C6F:
    case 0x0CEF:
    case 0x0D6F:
    case 0x0E59:
    case 0x0ED9:
    case 0x0F29:
    case 0x1049:
    case 0x1371:
    case 0x17E9:
    case 0x1819:
    case 0x2079:
    case 0x2089:
    case 0xFF19:
	return 9;
    default:
	return -1;
    }
}

int _PyUnicode_IsDecimalDigit(register const Py_UNICODE ch)
{
    if (_PyUnicode_ToDecimalDigit(ch) < 0)
	return 0;
    return 1;
}

/* Returns the integer digit (0-9) for Unicode characters having
   this property, -1 otherwise. */

int _PyUnicode_ToDigit(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x24EA:
	return 0;
    case 0x2460:
    case 0x2474:
    case 0x2488:
    case 0x2776:
    case 0x2780:
    case 0x278A:
	return 1;
    case 0x2461:
    case 0x2475:
    case 0x2489:
    case 0x2777:
    case 0x2781:
    case 0x278B:
	return 2;
    case 0x2462:
    case 0x2476:
    case 0x248A:
    case 0x2778:
    case 0x2782:
    case 0x278C:
	return 3;
    case 0x2463:
    case 0x2477:
    case 0x248B:
    case 0x2779:
    case 0x2783:
    case 0x278D:
	return 4;
    case 0x2464:
    case 0x2478:
    case 0x248C:
    case 0x277A:
    case 0x2784:
    case 0x278E:
	return 5;
    case 0x2465:
    case 0x2479:
    case 0x248D:
    case 0x277B:
    case 0x2785:
    case 0x278F:
	return 6;
    case 0x2466:
    case 0x247A:
    case 0x248E:
    case 0x277C:
    case 0x2786:
    case 0x2790:
	return 7;
    case 0x2467:
    case 0x247B:
    case 0x248F:
    case 0x277D:
    case 0x2787:
    case 0x2791:
	return 8;
    case 0x2468:
    case 0x247C:
    case 0x2490:
    case 0x277E:
    case 0x2788:
    case 0x2792:
	return 9;
    default:
	return _PyUnicode_ToDecimalDigit(ch);
    }
}

int _PyUnicode_IsDigit(register const Py_UNICODE ch)
{
    if (_PyUnicode_ToDigit(ch) < 0)
	return 0;
    return 1;
}

/* Returns the numeric value as double for Unicode characters having
   this property, -1.0 otherwise. */

double _PyUnicode_ToNumeric(register const Py_UNICODE ch)
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

int _PyUnicode_IsNumeric(register const Py_UNICODE ch)
{
    if (_PyUnicode_ToNumeric(ch) < 0.0)
	return 0;
    return 1;
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
    case 0x3000: /* IDEOGRAPHIC SPACE */
	return 1;
    default:
	return 0;
    }
}

/* Returns 1 for Unicode characters having the category 'Ll', 0
   otherwise. */

int _PyUnicode_IsLowercase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0061: /* LATIN SMALL LETTER A */
    case 0x0062: /* LATIN SMALL LETTER B */
    case 0x0063: /* LATIN SMALL LETTER C */
    case 0x0064: /* LATIN SMALL LETTER D */
    case 0x0065: /* LATIN SMALL LETTER E */
    case 0x0066: /* LATIN SMALL LETTER F */
    case 0x0067: /* LATIN SMALL LETTER G */
    case 0x0068: /* LATIN SMALL LETTER H */
    case 0x0069: /* LATIN SMALL LETTER I */
    case 0x006A: /* LATIN SMALL LETTER J */
    case 0x006B: /* LATIN SMALL LETTER K */
    case 0x006C: /* LATIN SMALL LETTER L */
    case 0x006D: /* LATIN SMALL LETTER M */
    case 0x006E: /* LATIN SMALL LETTER N */
    case 0x006F: /* LATIN SMALL LETTER O */
    case 0x0070: /* LATIN SMALL LETTER P */
    case 0x0071: /* LATIN SMALL LETTER Q */
    case 0x0072: /* LATIN SMALL LETTER R */
    case 0x0073: /* LATIN SMALL LETTER S */
    case 0x0074: /* LATIN SMALL LETTER T */
    case 0x0075: /* LATIN SMALL LETTER U */
    case 0x0076: /* LATIN SMALL LETTER V */
    case 0x0077: /* LATIN SMALL LETTER W */
    case 0x0078: /* LATIN SMALL LETTER X */
    case 0x0079: /* LATIN SMALL LETTER Y */
    case 0x007A: /* LATIN SMALL LETTER Z */
    case 0x00AA: /* FEMININE ORDINAL INDICATOR */
    case 0x00B5: /* MICRO SIGN */
    case 0x00BA: /* MASCULINE ORDINAL INDICATOR */
    case 0x00DF: /* LATIN SMALL LETTER SHARP S */
    case 0x00E0: /* LATIN SMALL LETTER A WITH GRAVE */
    case 0x00E1: /* LATIN SMALL LETTER A WITH ACUTE */
    case 0x00E2: /* LATIN SMALL LETTER A WITH CIRCUMFLEX */
    case 0x00E3: /* LATIN SMALL LETTER A WITH TILDE */
    case 0x00E4: /* LATIN SMALL LETTER A WITH DIAERESIS */
    case 0x00E5: /* LATIN SMALL LETTER A WITH RING ABOVE */
    case 0x00E6: /* LATIN SMALL LETTER AE */
    case 0x00E7: /* LATIN SMALL LETTER C WITH CEDILLA */
    case 0x00E8: /* LATIN SMALL LETTER E WITH GRAVE */
    case 0x00E9: /* LATIN SMALL LETTER E WITH ACUTE */
    case 0x00EA: /* LATIN SMALL LETTER E WITH CIRCUMFLEX */
    case 0x00EB: /* LATIN SMALL LETTER E WITH DIAERESIS */
    case 0x00EC: /* LATIN SMALL LETTER I WITH GRAVE */
    case 0x00ED: /* LATIN SMALL LETTER I WITH ACUTE */
    case 0x00EE: /* LATIN SMALL LETTER I WITH CIRCUMFLEX */
    case 0x00EF: /* LATIN SMALL LETTER I WITH DIAERESIS */
    case 0x00F0: /* LATIN SMALL LETTER ETH */
    case 0x00F1: /* LATIN SMALL LETTER N WITH TILDE */
    case 0x00F2: /* LATIN SMALL LETTER O WITH GRAVE */
    case 0x00F3: /* LATIN SMALL LETTER O WITH ACUTE */
    case 0x00F4: /* LATIN SMALL LETTER O WITH CIRCUMFLEX */
    case 0x00F5: /* LATIN SMALL LETTER O WITH TILDE */
    case 0x00F6: /* LATIN SMALL LETTER O WITH DIAERESIS */
    case 0x00F8: /* LATIN SMALL LETTER O WITH STROKE */
    case 0x00F9: /* LATIN SMALL LETTER U WITH GRAVE */
    case 0x00FA: /* LATIN SMALL LETTER U WITH ACUTE */
    case 0x00FB: /* LATIN SMALL LETTER U WITH CIRCUMFLEX */
    case 0x00FC: /* LATIN SMALL LETTER U WITH DIAERESIS */
    case 0x00FD: /* LATIN SMALL LETTER Y WITH ACUTE */
    case 0x00FE: /* LATIN SMALL LETTER THORN */
    case 0x00FF: /* LATIN SMALL LETTER Y WITH DIAERESIS */
    case 0x0101: /* LATIN SMALL LETTER A WITH MACRON */
    case 0x0103: /* LATIN SMALL LETTER A WITH BREVE */
    case 0x0105: /* LATIN SMALL LETTER A WITH OGONEK */
    case 0x0107: /* LATIN SMALL LETTER C WITH ACUTE */
    case 0x0109: /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
    case 0x010B: /* LATIN SMALL LETTER C WITH DOT ABOVE */
    case 0x010D: /* LATIN SMALL LETTER C WITH CARON */
    case 0x010F: /* LATIN SMALL LETTER D WITH CARON */
    case 0x0111: /* LATIN SMALL LETTER D WITH STROKE */
    case 0x0113: /* LATIN SMALL LETTER E WITH MACRON */
    case 0x0115: /* LATIN SMALL LETTER E WITH BREVE */
    case 0x0117: /* LATIN SMALL LETTER E WITH DOT ABOVE */
    case 0x0119: /* LATIN SMALL LETTER E WITH OGONEK */
    case 0x011B: /* LATIN SMALL LETTER E WITH CARON */
    case 0x011D: /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
    case 0x011F: /* LATIN SMALL LETTER G WITH BREVE */
    case 0x0121: /* LATIN SMALL LETTER G WITH DOT ABOVE */
    case 0x0123: /* LATIN SMALL LETTER G WITH CEDILLA */
    case 0x0125: /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
    case 0x0127: /* LATIN SMALL LETTER H WITH STROKE */
    case 0x0129: /* LATIN SMALL LETTER I WITH TILDE */
    case 0x012B: /* LATIN SMALL LETTER I WITH MACRON */
    case 0x012D: /* LATIN SMALL LETTER I WITH BREVE */
    case 0x012F: /* LATIN SMALL LETTER I WITH OGONEK */
    case 0x0131: /* LATIN SMALL LETTER DOTLESS I */
    case 0x0133: /* LATIN SMALL LIGATURE IJ */
    case 0x0135: /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
    case 0x0137: /* LATIN SMALL LETTER K WITH CEDILLA */
    case 0x0138: /* LATIN SMALL LETTER KRA */
    case 0x013A: /* LATIN SMALL LETTER L WITH ACUTE */
    case 0x013C: /* LATIN SMALL LETTER L WITH CEDILLA */
    case 0x013E: /* LATIN SMALL LETTER L WITH CARON */
    case 0x0140: /* LATIN SMALL LETTER L WITH MIDDLE DOT */
    case 0x0142: /* LATIN SMALL LETTER L WITH STROKE */
    case 0x0144: /* LATIN SMALL LETTER N WITH ACUTE */
    case 0x0146: /* LATIN SMALL LETTER N WITH CEDILLA */
    case 0x0148: /* LATIN SMALL LETTER N WITH CARON */
    case 0x0149: /* LATIN SMALL LETTER N PRECEDED BY APOSTROPHE */
    case 0x014B: /* LATIN SMALL LETTER ENG */
    case 0x014D: /* LATIN SMALL LETTER O WITH MACRON */
    case 0x014F: /* LATIN SMALL LETTER O WITH BREVE */
    case 0x0151: /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    case 0x0153: /* LATIN SMALL LIGATURE OE */
    case 0x0155: /* LATIN SMALL LETTER R WITH ACUTE */
    case 0x0157: /* LATIN SMALL LETTER R WITH CEDILLA */
    case 0x0159: /* LATIN SMALL LETTER R WITH CARON */
    case 0x015B: /* LATIN SMALL LETTER S WITH ACUTE */
    case 0x015D: /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
    case 0x015F: /* LATIN SMALL LETTER S WITH CEDILLA */
    case 0x0161: /* LATIN SMALL LETTER S WITH CARON */
    case 0x0163: /* LATIN SMALL LETTER T WITH CEDILLA */
    case 0x0165: /* LATIN SMALL LETTER T WITH CARON */
    case 0x0167: /* LATIN SMALL LETTER T WITH STROKE */
    case 0x0169: /* LATIN SMALL LETTER U WITH TILDE */
    case 0x016B: /* LATIN SMALL LETTER U WITH MACRON */
    case 0x016D: /* LATIN SMALL LETTER U WITH BREVE */
    case 0x016F: /* LATIN SMALL LETTER U WITH RING ABOVE */
    case 0x0171: /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    case 0x0173: /* LATIN SMALL LETTER U WITH OGONEK */
    case 0x0175: /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
    case 0x0177: /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
    case 0x017A: /* LATIN SMALL LETTER Z WITH ACUTE */
    case 0x017C: /* LATIN SMALL LETTER Z WITH DOT ABOVE */
    case 0x017E: /* LATIN SMALL LETTER Z WITH CARON */
    case 0x017F: /* LATIN SMALL LETTER LONG S */
    case 0x0180: /* LATIN SMALL LETTER B WITH STROKE */
    case 0x0183: /* LATIN SMALL LETTER B WITH TOPBAR */
    case 0x0185: /* LATIN SMALL LETTER TONE SIX */
    case 0x0188: /* LATIN SMALL LETTER C WITH HOOK */
    case 0x018C: /* LATIN SMALL LETTER D WITH TOPBAR */
    case 0x018D: /* LATIN SMALL LETTER TURNED DELTA */
    case 0x0192: /* LATIN SMALL LETTER F WITH HOOK */
    case 0x0195: /* LATIN SMALL LETTER HV */
    case 0x0199: /* LATIN SMALL LETTER K WITH HOOK */
    case 0x019A: /* LATIN SMALL LETTER L WITH BAR */
    case 0x019B: /* LATIN SMALL LETTER LAMBDA WITH STROKE */
    case 0x019E: /* LATIN SMALL LETTER N WITH LONG RIGHT LEG */
    case 0x01A1: /* LATIN SMALL LETTER O WITH HORN */
    case 0x01A3: /* LATIN SMALL LETTER OI */
    case 0x01A5: /* LATIN SMALL LETTER P WITH HOOK */
    case 0x01A8: /* LATIN SMALL LETTER TONE TWO */
    case 0x01AA: /* LATIN LETTER REVERSED ESH LOOP */
    case 0x01AB: /* LATIN SMALL LETTER T WITH PALATAL HOOK */
    case 0x01AD: /* LATIN SMALL LETTER T WITH HOOK */
    case 0x01B0: /* LATIN SMALL LETTER U WITH HORN */
    case 0x01B4: /* LATIN SMALL LETTER Y WITH HOOK */
    case 0x01B6: /* LATIN SMALL LETTER Z WITH STROKE */
    case 0x01B9: /* LATIN SMALL LETTER EZH REVERSED */
    case 0x01BA: /* LATIN SMALL LETTER EZH WITH TAIL */
    case 0x01BD: /* LATIN SMALL LETTER TONE FIVE */
    case 0x01BE: /* LATIN LETTER INVERTED GLOTTAL STOP WITH STROKE */
    case 0x01BF: /* LATIN LETTER WYNN */
    case 0x01C6: /* LATIN SMALL LETTER DZ WITH CARON */
    case 0x01C9: /* LATIN SMALL LETTER LJ */
    case 0x01CC: /* LATIN SMALL LETTER NJ */
    case 0x01CE: /* LATIN SMALL LETTER A WITH CARON */
    case 0x01D0: /* LATIN SMALL LETTER I WITH CARON */
    case 0x01D2: /* LATIN SMALL LETTER O WITH CARON */
    case 0x01D4: /* LATIN SMALL LETTER U WITH CARON */
    case 0x01D6: /* LATIN SMALL LETTER U WITH DIAERESIS AND MACRON */
    case 0x01D8: /* LATIN SMALL LETTER U WITH DIAERESIS AND ACUTE */
    case 0x01DA: /* LATIN SMALL LETTER U WITH DIAERESIS AND CARON */
    case 0x01DC: /* LATIN SMALL LETTER U WITH DIAERESIS AND GRAVE */
    case 0x01DD: /* LATIN SMALL LETTER TURNED E */
    case 0x01DF: /* LATIN SMALL LETTER A WITH DIAERESIS AND MACRON */
    case 0x01E1: /* LATIN SMALL LETTER A WITH DOT ABOVE AND MACRON */
    case 0x01E3: /* LATIN SMALL LETTER AE WITH MACRON */
    case 0x01E5: /* LATIN SMALL LETTER G WITH STROKE */
    case 0x01E7: /* LATIN SMALL LETTER G WITH CARON */
    case 0x01E9: /* LATIN SMALL LETTER K WITH CARON */
    case 0x01EB: /* LATIN SMALL LETTER O WITH OGONEK */
    case 0x01ED: /* LATIN SMALL LETTER O WITH OGONEK AND MACRON */
    case 0x01EF: /* LATIN SMALL LETTER EZH WITH CARON */
    case 0x01F0: /* LATIN SMALL LETTER J WITH CARON */
    case 0x01F3: /* LATIN SMALL LETTER DZ */
    case 0x01F5: /* LATIN SMALL LETTER G WITH ACUTE */
    case 0x01F9: /* LATIN SMALL LETTER N WITH GRAVE */
    case 0x01FB: /* LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE */
    case 0x01FD: /* LATIN SMALL LETTER AE WITH ACUTE */
    case 0x01FF: /* LATIN SMALL LETTER O WITH STROKE AND ACUTE */
    case 0x0201: /* LATIN SMALL LETTER A WITH DOUBLE GRAVE */
    case 0x0203: /* LATIN SMALL LETTER A WITH INVERTED BREVE */
    case 0x0205: /* LATIN SMALL LETTER E WITH DOUBLE GRAVE */
    case 0x0207: /* LATIN SMALL LETTER E WITH INVERTED BREVE */
    case 0x0209: /* LATIN SMALL LETTER I WITH DOUBLE GRAVE */
    case 0x020B: /* LATIN SMALL LETTER I WITH INVERTED BREVE */
    case 0x020D: /* LATIN SMALL LETTER O WITH DOUBLE GRAVE */
    case 0x020F: /* LATIN SMALL LETTER O WITH INVERTED BREVE */
    case 0x0211: /* LATIN SMALL LETTER R WITH DOUBLE GRAVE */
    case 0x0213: /* LATIN SMALL LETTER R WITH INVERTED BREVE */
    case 0x0215: /* LATIN SMALL LETTER U WITH DOUBLE GRAVE */
    case 0x0217: /* LATIN SMALL LETTER U WITH INVERTED BREVE */
    case 0x0219: /* LATIN SMALL LETTER S WITH COMMA BELOW */
    case 0x021B: /* LATIN SMALL LETTER T WITH COMMA BELOW */
    case 0x021D: /* LATIN SMALL LETTER YOGH */
    case 0x021F: /* LATIN SMALL LETTER H WITH CARON */
    case 0x0223: /* LATIN SMALL LETTER OU */
    case 0x0225: /* LATIN SMALL LETTER Z WITH HOOK */
    case 0x0227: /* LATIN SMALL LETTER A WITH DOT ABOVE */
    case 0x0229: /* LATIN SMALL LETTER E WITH CEDILLA */
    case 0x022B: /* LATIN SMALL LETTER O WITH DIAERESIS AND MACRON */
    case 0x022D: /* LATIN SMALL LETTER O WITH TILDE AND MACRON */
    case 0x022F: /* LATIN SMALL LETTER O WITH DOT ABOVE */
    case 0x0231: /* LATIN SMALL LETTER O WITH DOT ABOVE AND MACRON */
    case 0x0233: /* LATIN SMALL LETTER Y WITH MACRON */
    case 0x0250: /* LATIN SMALL LETTER TURNED A */
    case 0x0251: /* LATIN SMALL LETTER ALPHA */
    case 0x0252: /* LATIN SMALL LETTER TURNED ALPHA */
    case 0x0253: /* LATIN SMALL LETTER B WITH HOOK */
    case 0x0254: /* LATIN SMALL LETTER OPEN O */
    case 0x0255: /* LATIN SMALL LETTER C WITH CURL */
    case 0x0256: /* LATIN SMALL LETTER D WITH TAIL */
    case 0x0257: /* LATIN SMALL LETTER D WITH HOOK */
    case 0x0258: /* LATIN SMALL LETTER REVERSED E */
    case 0x0259: /* LATIN SMALL LETTER SCHWA */
    case 0x025A: /* LATIN SMALL LETTER SCHWA WITH HOOK */
    case 0x025B: /* LATIN SMALL LETTER OPEN E */
    case 0x025C: /* LATIN SMALL LETTER REVERSED OPEN E */
    case 0x025D: /* LATIN SMALL LETTER REVERSED OPEN E WITH HOOK */
    case 0x025E: /* LATIN SMALL LETTER CLOSED REVERSED OPEN E */
    case 0x025F: /* LATIN SMALL LETTER DOTLESS J WITH STROKE */
    case 0x0260: /* LATIN SMALL LETTER G WITH HOOK */
    case 0x0261: /* LATIN SMALL LETTER SCRIPT G */
    case 0x0262: /* LATIN LETTER SMALL CAPITAL G */
    case 0x0263: /* LATIN SMALL LETTER GAMMA */
    case 0x0264: /* LATIN SMALL LETTER RAMS HORN */
    case 0x0265: /* LATIN SMALL LETTER TURNED H */
    case 0x0266: /* LATIN SMALL LETTER H WITH HOOK */
    case 0x0267: /* LATIN SMALL LETTER HENG WITH HOOK */
    case 0x0268: /* LATIN SMALL LETTER I WITH STROKE */
    case 0x0269: /* LATIN SMALL LETTER IOTA */
    case 0x026A: /* LATIN LETTER SMALL CAPITAL I */
    case 0x026B: /* LATIN SMALL LETTER L WITH MIDDLE TILDE */
    case 0x026C: /* LATIN SMALL LETTER L WITH BELT */
    case 0x026D: /* LATIN SMALL LETTER L WITH RETROFLEX HOOK */
    case 0x026E: /* LATIN SMALL LETTER LEZH */
    case 0x026F: /* LATIN SMALL LETTER TURNED M */
    case 0x0270: /* LATIN SMALL LETTER TURNED M WITH LONG LEG */
    case 0x0271: /* LATIN SMALL LETTER M WITH HOOK */
    case 0x0272: /* LATIN SMALL LETTER N WITH LEFT HOOK */
    case 0x0273: /* LATIN SMALL LETTER N WITH RETROFLEX HOOK */
    case 0x0274: /* LATIN LETTER SMALL CAPITAL N */
    case 0x0275: /* LATIN SMALL LETTER BARRED O */
    case 0x0276: /* LATIN LETTER SMALL CAPITAL OE */
    case 0x0277: /* LATIN SMALL LETTER CLOSED OMEGA */
    case 0x0278: /* LATIN SMALL LETTER PHI */
    case 0x0279: /* LATIN SMALL LETTER TURNED R */
    case 0x027A: /* LATIN SMALL LETTER TURNED R WITH LONG LEG */
    case 0x027B: /* LATIN SMALL LETTER TURNED R WITH HOOK */
    case 0x027C: /* LATIN SMALL LETTER R WITH LONG LEG */
    case 0x027D: /* LATIN SMALL LETTER R WITH TAIL */
    case 0x027E: /* LATIN SMALL LETTER R WITH FISHHOOK */
    case 0x027F: /* LATIN SMALL LETTER REVERSED R WITH FISHHOOK */
    case 0x0280: /* LATIN LETTER SMALL CAPITAL R */
    case 0x0281: /* LATIN LETTER SMALL CAPITAL INVERTED R */
    case 0x0282: /* LATIN SMALL LETTER S WITH HOOK */
    case 0x0283: /* LATIN SMALL LETTER ESH */
    case 0x0284: /* LATIN SMALL LETTER DOTLESS J WITH STROKE AND HOOK */
    case 0x0285: /* LATIN SMALL LETTER SQUAT REVERSED ESH */
    case 0x0286: /* LATIN SMALL LETTER ESH WITH CURL */
    case 0x0287: /* LATIN SMALL LETTER TURNED T */
    case 0x0288: /* LATIN SMALL LETTER T WITH RETROFLEX HOOK */
    case 0x0289: /* LATIN SMALL LETTER U BAR */
    case 0x028A: /* LATIN SMALL LETTER UPSILON */
    case 0x028B: /* LATIN SMALL LETTER V WITH HOOK */
    case 0x028C: /* LATIN SMALL LETTER TURNED V */
    case 0x028D: /* LATIN SMALL LETTER TURNED W */
    case 0x028E: /* LATIN SMALL LETTER TURNED Y */
    case 0x028F: /* LATIN LETTER SMALL CAPITAL Y */
    case 0x0290: /* LATIN SMALL LETTER Z WITH RETROFLEX HOOK */
    case 0x0291: /* LATIN SMALL LETTER Z WITH CURL */
    case 0x0292: /* LATIN SMALL LETTER EZH */
    case 0x0293: /* LATIN SMALL LETTER EZH WITH CURL */
    case 0x0294: /* LATIN LETTER GLOTTAL STOP */
    case 0x0295: /* LATIN LETTER PHARYNGEAL VOICED FRICATIVE */
    case 0x0296: /* LATIN LETTER INVERTED GLOTTAL STOP */
    case 0x0297: /* LATIN LETTER STRETCHED C */
    case 0x0298: /* LATIN LETTER BILABIAL CLICK */
    case 0x0299: /* LATIN LETTER SMALL CAPITAL B */
    case 0x029A: /* LATIN SMALL LETTER CLOSED OPEN E */
    case 0x029B: /* LATIN LETTER SMALL CAPITAL G WITH HOOK */
    case 0x029C: /* LATIN LETTER SMALL CAPITAL H */
    case 0x029D: /* LATIN SMALL LETTER J WITH CROSSED-TAIL */
    case 0x029E: /* LATIN SMALL LETTER TURNED K */
    case 0x029F: /* LATIN LETTER SMALL CAPITAL L */
    case 0x02A0: /* LATIN SMALL LETTER Q WITH HOOK */
    case 0x02A1: /* LATIN LETTER GLOTTAL STOP WITH STROKE */
    case 0x02A2: /* LATIN LETTER REVERSED GLOTTAL STOP WITH STROKE */
    case 0x02A3: /* LATIN SMALL LETTER DZ DIGRAPH */
    case 0x02A4: /* LATIN SMALL LETTER DEZH DIGRAPH */
    case 0x02A5: /* LATIN SMALL LETTER DZ DIGRAPH WITH CURL */
    case 0x02A6: /* LATIN SMALL LETTER TS DIGRAPH */
    case 0x02A7: /* LATIN SMALL LETTER TESH DIGRAPH */
    case 0x02A8: /* LATIN SMALL LETTER TC DIGRAPH WITH CURL */
    case 0x02A9: /* LATIN SMALL LETTER FENG DIGRAPH */
    case 0x02AA: /* LATIN SMALL LETTER LS DIGRAPH */
    case 0x02AB: /* LATIN SMALL LETTER LZ DIGRAPH */
    case 0x02AC: /* LATIN LETTER BILABIAL PERCUSSIVE */
    case 0x02AD: /* LATIN LETTER BIDENTAL PERCUSSIVE */
    case 0x0390: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    case 0x03AC: /* GREEK SMALL LETTER ALPHA WITH TONOS */
    case 0x03AD: /* GREEK SMALL LETTER EPSILON WITH TONOS */
    case 0x03AE: /* GREEK SMALL LETTER ETA WITH TONOS */
    case 0x03AF: /* GREEK SMALL LETTER IOTA WITH TONOS */
    case 0x03B0: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    case 0x03B1: /* GREEK SMALL LETTER ALPHA */
    case 0x03B2: /* GREEK SMALL LETTER BETA */
    case 0x03B3: /* GREEK SMALL LETTER GAMMA */
    case 0x03B4: /* GREEK SMALL LETTER DELTA */
    case 0x03B5: /* GREEK SMALL LETTER EPSILON */
    case 0x03B6: /* GREEK SMALL LETTER ZETA */
    case 0x03B7: /* GREEK SMALL LETTER ETA */
    case 0x03B8: /* GREEK SMALL LETTER THETA */
    case 0x03B9: /* GREEK SMALL LETTER IOTA */
    case 0x03BA: /* GREEK SMALL LETTER KAPPA */
    case 0x03BB: /* GREEK SMALL LETTER LAMDA */
    case 0x03BC: /* GREEK SMALL LETTER MU */
    case 0x03BD: /* GREEK SMALL LETTER NU */
    case 0x03BE: /* GREEK SMALL LETTER XI */
    case 0x03BF: /* GREEK SMALL LETTER OMICRON */
    case 0x03C0: /* GREEK SMALL LETTER PI */
    case 0x03C1: /* GREEK SMALL LETTER RHO */
    case 0x03C2: /* GREEK SMALL LETTER FINAL SIGMA */
    case 0x03C3: /* GREEK SMALL LETTER SIGMA */
    case 0x03C4: /* GREEK SMALL LETTER TAU */
    case 0x03C5: /* GREEK SMALL LETTER UPSILON */
    case 0x03C6: /* GREEK SMALL LETTER PHI */
    case 0x03C7: /* GREEK SMALL LETTER CHI */
    case 0x03C8: /* GREEK SMALL LETTER PSI */
    case 0x03C9: /* GREEK SMALL LETTER OMEGA */
    case 0x03CA: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    case 0x03CB: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    case 0x03CC: /* GREEK SMALL LETTER OMICRON WITH TONOS */
    case 0x03CD: /* GREEK SMALL LETTER UPSILON WITH TONOS */
    case 0x03CE: /* GREEK SMALL LETTER OMEGA WITH TONOS */
    case 0x03D0: /* GREEK BETA SYMBOL */
    case 0x03D1: /* GREEK THETA SYMBOL */
    case 0x03D5: /* GREEK PHI SYMBOL */
    case 0x03D6: /* GREEK PI SYMBOL */
    case 0x03D7: /* GREEK KAI SYMBOL */
    case 0x03DB: /* GREEK SMALL LETTER STIGMA */
    case 0x03DD: /* GREEK SMALL LETTER DIGAMMA */
    case 0x03DF: /* GREEK SMALL LETTER KOPPA */
    case 0x03E1: /* GREEK SMALL LETTER SAMPI */
    case 0x03E3: /* COPTIC SMALL LETTER SHEI */
    case 0x03E5: /* COPTIC SMALL LETTER FEI */
    case 0x03E7: /* COPTIC SMALL LETTER KHEI */
    case 0x03E9: /* COPTIC SMALL LETTER HORI */
    case 0x03EB: /* COPTIC SMALL LETTER GANGIA */
    case 0x03ED: /* COPTIC SMALL LETTER SHIMA */
    case 0x03EF: /* COPTIC SMALL LETTER DEI */
    case 0x03F0: /* GREEK KAPPA SYMBOL */
    case 0x03F1: /* GREEK RHO SYMBOL */
    case 0x03F2: /* GREEK LUNATE SIGMA SYMBOL */
    case 0x03F3: /* GREEK LETTER YOT */
    case 0x0430: /* CYRILLIC SMALL LETTER A */
    case 0x0431: /* CYRILLIC SMALL LETTER BE */
    case 0x0432: /* CYRILLIC SMALL LETTER VE */
    case 0x0433: /* CYRILLIC SMALL LETTER GHE */
    case 0x0434: /* CYRILLIC SMALL LETTER DE */
    case 0x0435: /* CYRILLIC SMALL LETTER IE */
    case 0x0436: /* CYRILLIC SMALL LETTER ZHE */
    case 0x0437: /* CYRILLIC SMALL LETTER ZE */
    case 0x0438: /* CYRILLIC SMALL LETTER I */
    case 0x0439: /* CYRILLIC SMALL LETTER SHORT I */
    case 0x043A: /* CYRILLIC SMALL LETTER KA */
    case 0x043B: /* CYRILLIC SMALL LETTER EL */
    case 0x043C: /* CYRILLIC SMALL LETTER EM */
    case 0x043D: /* CYRILLIC SMALL LETTER EN */
    case 0x043E: /* CYRILLIC SMALL LETTER O */
    case 0x043F: /* CYRILLIC SMALL LETTER PE */
    case 0x0440: /* CYRILLIC SMALL LETTER ER */
    case 0x0441: /* CYRILLIC SMALL LETTER ES */
    case 0x0442: /* CYRILLIC SMALL LETTER TE */
    case 0x0443: /* CYRILLIC SMALL LETTER U */
    case 0x0444: /* CYRILLIC SMALL LETTER EF */
    case 0x0445: /* CYRILLIC SMALL LETTER HA */
    case 0x0446: /* CYRILLIC SMALL LETTER TSE */
    case 0x0447: /* CYRILLIC SMALL LETTER CHE */
    case 0x0448: /* CYRILLIC SMALL LETTER SHA */
    case 0x0449: /* CYRILLIC SMALL LETTER SHCHA */
    case 0x044A: /* CYRILLIC SMALL LETTER HARD SIGN */
    case 0x044B: /* CYRILLIC SMALL LETTER YERU */
    case 0x044C: /* CYRILLIC SMALL LETTER SOFT SIGN */
    case 0x044D: /* CYRILLIC SMALL LETTER E */
    case 0x044E: /* CYRILLIC SMALL LETTER YU */
    case 0x044F: /* CYRILLIC SMALL LETTER YA */
    case 0x0450: /* CYRILLIC SMALL LETTER IE WITH GRAVE */
    case 0x0451: /* CYRILLIC SMALL LETTER IO */
    case 0x0452: /* CYRILLIC SMALL LETTER DJE */
    case 0x0453: /* CYRILLIC SMALL LETTER GJE */
    case 0x0454: /* CYRILLIC SMALL LETTER UKRAINIAN IE */
    case 0x0455: /* CYRILLIC SMALL LETTER DZE */
    case 0x0456: /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    case 0x0457: /* CYRILLIC SMALL LETTER YI */
    case 0x0458: /* CYRILLIC SMALL LETTER JE */
    case 0x0459: /* CYRILLIC SMALL LETTER LJE */
    case 0x045A: /* CYRILLIC SMALL LETTER NJE */
    case 0x045B: /* CYRILLIC SMALL LETTER TSHE */
    case 0x045C: /* CYRILLIC SMALL LETTER KJE */
    case 0x045D: /* CYRILLIC SMALL LETTER I WITH GRAVE */
    case 0x045E: /* CYRILLIC SMALL LETTER SHORT U */
    case 0x045F: /* CYRILLIC SMALL LETTER DZHE */
    case 0x0461: /* CYRILLIC SMALL LETTER OMEGA */
    case 0x0463: /* CYRILLIC SMALL LETTER YAT */
    case 0x0465: /* CYRILLIC SMALL LETTER IOTIFIED E */
    case 0x0467: /* CYRILLIC SMALL LETTER LITTLE YUS */
    case 0x0469: /* CYRILLIC SMALL LETTER IOTIFIED LITTLE YUS */
    case 0x046B: /* CYRILLIC SMALL LETTER BIG YUS */
    case 0x046D: /* CYRILLIC SMALL LETTER IOTIFIED BIG YUS */
    case 0x046F: /* CYRILLIC SMALL LETTER KSI */
    case 0x0471: /* CYRILLIC SMALL LETTER PSI */
    case 0x0473: /* CYRILLIC SMALL LETTER FITA */
    case 0x0475: /* CYRILLIC SMALL LETTER IZHITSA */
    case 0x0477: /* CYRILLIC SMALL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
    case 0x0479: /* CYRILLIC SMALL LETTER UK */
    case 0x047B: /* CYRILLIC SMALL LETTER ROUND OMEGA */
    case 0x047D: /* CYRILLIC SMALL LETTER OMEGA WITH TITLO */
    case 0x047F: /* CYRILLIC SMALL LETTER OT */
    case 0x0481: /* CYRILLIC SMALL LETTER KOPPA */
    case 0x048D: /* CYRILLIC SMALL LETTER SEMISOFT SIGN */
    case 0x048F: /* CYRILLIC SMALL LETTER ER WITH TICK */
    case 0x0491: /* CYRILLIC SMALL LETTER GHE WITH UPTURN */
    case 0x0493: /* CYRILLIC SMALL LETTER GHE WITH STROKE */
    case 0x0495: /* CYRILLIC SMALL LETTER GHE WITH MIDDLE HOOK */
    case 0x0497: /* CYRILLIC SMALL LETTER ZHE WITH DESCENDER */
    case 0x0499: /* CYRILLIC SMALL LETTER ZE WITH DESCENDER */
    case 0x049B: /* CYRILLIC SMALL LETTER KA WITH DESCENDER */
    case 0x049D: /* CYRILLIC SMALL LETTER KA WITH VERTICAL STROKE */
    case 0x049F: /* CYRILLIC SMALL LETTER KA WITH STROKE */
    case 0x04A1: /* CYRILLIC SMALL LETTER BASHKIR KA */
    case 0x04A3: /* CYRILLIC SMALL LETTER EN WITH DESCENDER */
    case 0x04A5: /* CYRILLIC SMALL LIGATURE EN GHE */
    case 0x04A7: /* CYRILLIC SMALL LETTER PE WITH MIDDLE HOOK */
    case 0x04A9: /* CYRILLIC SMALL LETTER ABKHASIAN HA */
    case 0x04AB: /* CYRILLIC SMALL LETTER ES WITH DESCENDER */
    case 0x04AD: /* CYRILLIC SMALL LETTER TE WITH DESCENDER */
    case 0x04AF: /* CYRILLIC SMALL LETTER STRAIGHT U */
    case 0x04B1: /* CYRILLIC SMALL LETTER STRAIGHT U WITH STROKE */
    case 0x04B3: /* CYRILLIC SMALL LETTER HA WITH DESCENDER */
    case 0x04B5: /* CYRILLIC SMALL LIGATURE TE TSE */
    case 0x04B7: /* CYRILLIC SMALL LETTER CHE WITH DESCENDER */
    case 0x04B9: /* CYRILLIC SMALL LETTER CHE WITH VERTICAL STROKE */
    case 0x04BB: /* CYRILLIC SMALL LETTER SHHA */
    case 0x04BD: /* CYRILLIC SMALL LETTER ABKHASIAN CHE */
    case 0x04BF: /* CYRILLIC SMALL LETTER ABKHASIAN CHE WITH DESCENDER */
    case 0x04C2: /* CYRILLIC SMALL LETTER ZHE WITH BREVE */
    case 0x04C4: /* CYRILLIC SMALL LETTER KA WITH HOOK */
    case 0x04C8: /* CYRILLIC SMALL LETTER EN WITH HOOK */
    case 0x04CC: /* CYRILLIC SMALL LETTER KHAKASSIAN CHE */
    case 0x04D1: /* CYRILLIC SMALL LETTER A WITH BREVE */
    case 0x04D3: /* CYRILLIC SMALL LETTER A WITH DIAERESIS */
    case 0x04D5: /* CYRILLIC SMALL LIGATURE A IE */
    case 0x04D7: /* CYRILLIC SMALL LETTER IE WITH BREVE */
    case 0x04D9: /* CYRILLIC SMALL LETTER SCHWA */
    case 0x04DB: /* CYRILLIC SMALL LETTER SCHWA WITH DIAERESIS */
    case 0x04DD: /* CYRILLIC SMALL LETTER ZHE WITH DIAERESIS */
    case 0x04DF: /* CYRILLIC SMALL LETTER ZE WITH DIAERESIS */
    case 0x04E1: /* CYRILLIC SMALL LETTER ABKHASIAN DZE */
    case 0x04E3: /* CYRILLIC SMALL LETTER I WITH MACRON */
    case 0x04E5: /* CYRILLIC SMALL LETTER I WITH DIAERESIS */
    case 0x04E7: /* CYRILLIC SMALL LETTER O WITH DIAERESIS */
    case 0x04E9: /* CYRILLIC SMALL LETTER BARRED O */
    case 0x04EB: /* CYRILLIC SMALL LETTER BARRED O WITH DIAERESIS */
    case 0x04ED: /* CYRILLIC SMALL LETTER E WITH DIAERESIS */
    case 0x04EF: /* CYRILLIC SMALL LETTER U WITH MACRON */
    case 0x04F1: /* CYRILLIC SMALL LETTER U WITH DIAERESIS */
    case 0x04F3: /* CYRILLIC SMALL LETTER U WITH DOUBLE ACUTE */
    case 0x04F5: /* CYRILLIC SMALL LETTER CHE WITH DIAERESIS */
    case 0x04F9: /* CYRILLIC SMALL LETTER YERU WITH DIAERESIS */
    case 0x0561: /* ARMENIAN SMALL LETTER AYB */
    case 0x0562: /* ARMENIAN SMALL LETTER BEN */
    case 0x0563: /* ARMENIAN SMALL LETTER GIM */
    case 0x0564: /* ARMENIAN SMALL LETTER DA */
    case 0x0565: /* ARMENIAN SMALL LETTER ECH */
    case 0x0566: /* ARMENIAN SMALL LETTER ZA */
    case 0x0567: /* ARMENIAN SMALL LETTER EH */
    case 0x0568: /* ARMENIAN SMALL LETTER ET */
    case 0x0569: /* ARMENIAN SMALL LETTER TO */
    case 0x056A: /* ARMENIAN SMALL LETTER ZHE */
    case 0x056B: /* ARMENIAN SMALL LETTER INI */
    case 0x056C: /* ARMENIAN SMALL LETTER LIWN */
    case 0x056D: /* ARMENIAN SMALL LETTER XEH */
    case 0x056E: /* ARMENIAN SMALL LETTER CA */
    case 0x056F: /* ARMENIAN SMALL LETTER KEN */
    case 0x0570: /* ARMENIAN SMALL LETTER HO */
    case 0x0571: /* ARMENIAN SMALL LETTER JA */
    case 0x0572: /* ARMENIAN SMALL LETTER GHAD */
    case 0x0573: /* ARMENIAN SMALL LETTER CHEH */
    case 0x0574: /* ARMENIAN SMALL LETTER MEN */
    case 0x0575: /* ARMENIAN SMALL LETTER YI */
    case 0x0576: /* ARMENIAN SMALL LETTER NOW */
    case 0x0577: /* ARMENIAN SMALL LETTER SHA */
    case 0x0578: /* ARMENIAN SMALL LETTER VO */
    case 0x0579: /* ARMENIAN SMALL LETTER CHA */
    case 0x057A: /* ARMENIAN SMALL LETTER PEH */
    case 0x057B: /* ARMENIAN SMALL LETTER JHEH */
    case 0x057C: /* ARMENIAN SMALL LETTER RA */
    case 0x057D: /* ARMENIAN SMALL LETTER SEH */
    case 0x057E: /* ARMENIAN SMALL LETTER VEW */
    case 0x057F: /* ARMENIAN SMALL LETTER TIWN */
    case 0x0580: /* ARMENIAN SMALL LETTER REH */
    case 0x0581: /* ARMENIAN SMALL LETTER CO */
    case 0x0582: /* ARMENIAN SMALL LETTER YIWN */
    case 0x0583: /* ARMENIAN SMALL LETTER PIWR */
    case 0x0584: /* ARMENIAN SMALL LETTER KEH */
    case 0x0585: /* ARMENIAN SMALL LETTER OH */
    case 0x0586: /* ARMENIAN SMALL LETTER FEH */
    case 0x0587: /* ARMENIAN SMALL LIGATURE ECH YIWN */
    case 0x1E01: /* LATIN SMALL LETTER A WITH RING BELOW */
    case 0x1E03: /* LATIN SMALL LETTER B WITH DOT ABOVE */
    case 0x1E05: /* LATIN SMALL LETTER B WITH DOT BELOW */
    case 0x1E07: /* LATIN SMALL LETTER B WITH LINE BELOW */
    case 0x1E09: /* LATIN SMALL LETTER C WITH CEDILLA AND ACUTE */
    case 0x1E0B: /* LATIN SMALL LETTER D WITH DOT ABOVE */
    case 0x1E0D: /* LATIN SMALL LETTER D WITH DOT BELOW */
    case 0x1E0F: /* LATIN SMALL LETTER D WITH LINE BELOW */
    case 0x1E11: /* LATIN SMALL LETTER D WITH CEDILLA */
    case 0x1E13: /* LATIN SMALL LETTER D WITH CIRCUMFLEX BELOW */
    case 0x1E15: /* LATIN SMALL LETTER E WITH MACRON AND GRAVE */
    case 0x1E17: /* LATIN SMALL LETTER E WITH MACRON AND ACUTE */
    case 0x1E19: /* LATIN SMALL LETTER E WITH CIRCUMFLEX BELOW */
    case 0x1E1B: /* LATIN SMALL LETTER E WITH TILDE BELOW */
    case 0x1E1D: /* LATIN SMALL LETTER E WITH CEDILLA AND BREVE */
    case 0x1E1F: /* LATIN SMALL LETTER F WITH DOT ABOVE */
    case 0x1E21: /* LATIN SMALL LETTER G WITH MACRON */
    case 0x1E23: /* LATIN SMALL LETTER H WITH DOT ABOVE */
    case 0x1E25: /* LATIN SMALL LETTER H WITH DOT BELOW */
    case 0x1E27: /* LATIN SMALL LETTER H WITH DIAERESIS */
    case 0x1E29: /* LATIN SMALL LETTER H WITH CEDILLA */
    case 0x1E2B: /* LATIN SMALL LETTER H WITH BREVE BELOW */
    case 0x1E2D: /* LATIN SMALL LETTER I WITH TILDE BELOW */
    case 0x1E2F: /* LATIN SMALL LETTER I WITH DIAERESIS AND ACUTE */
    case 0x1E31: /* LATIN SMALL LETTER K WITH ACUTE */
    case 0x1E33: /* LATIN SMALL LETTER K WITH DOT BELOW */
    case 0x1E35: /* LATIN SMALL LETTER K WITH LINE BELOW */
    case 0x1E37: /* LATIN SMALL LETTER L WITH DOT BELOW */
    case 0x1E39: /* LATIN SMALL LETTER L WITH DOT BELOW AND MACRON */
    case 0x1E3B: /* LATIN SMALL LETTER L WITH LINE BELOW */
    case 0x1E3D: /* LATIN SMALL LETTER L WITH CIRCUMFLEX BELOW */
    case 0x1E3F: /* LATIN SMALL LETTER M WITH ACUTE */
    case 0x1E41: /* LATIN SMALL LETTER M WITH DOT ABOVE */
    case 0x1E43: /* LATIN SMALL LETTER M WITH DOT BELOW */
    case 0x1E45: /* LATIN SMALL LETTER N WITH DOT ABOVE */
    case 0x1E47: /* LATIN SMALL LETTER N WITH DOT BELOW */
    case 0x1E49: /* LATIN SMALL LETTER N WITH LINE BELOW */
    case 0x1E4B: /* LATIN SMALL LETTER N WITH CIRCUMFLEX BELOW */
    case 0x1E4D: /* LATIN SMALL LETTER O WITH TILDE AND ACUTE */
    case 0x1E4F: /* LATIN SMALL LETTER O WITH TILDE AND DIAERESIS */
    case 0x1E51: /* LATIN SMALL LETTER O WITH MACRON AND GRAVE */
    case 0x1E53: /* LATIN SMALL LETTER O WITH MACRON AND ACUTE */
    case 0x1E55: /* LATIN SMALL LETTER P WITH ACUTE */
    case 0x1E57: /* LATIN SMALL LETTER P WITH DOT ABOVE */
    case 0x1E59: /* LATIN SMALL LETTER R WITH DOT ABOVE */
    case 0x1E5B: /* LATIN SMALL LETTER R WITH DOT BELOW */
    case 0x1E5D: /* LATIN SMALL LETTER R WITH DOT BELOW AND MACRON */
    case 0x1E5F: /* LATIN SMALL LETTER R WITH LINE BELOW */
    case 0x1E61: /* LATIN SMALL LETTER S WITH DOT ABOVE */
    case 0x1E63: /* LATIN SMALL LETTER S WITH DOT BELOW */
    case 0x1E65: /* LATIN SMALL LETTER S WITH ACUTE AND DOT ABOVE */
    case 0x1E67: /* LATIN SMALL LETTER S WITH CARON AND DOT ABOVE */
    case 0x1E69: /* LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE */
    case 0x1E6B: /* LATIN SMALL LETTER T WITH DOT ABOVE */
    case 0x1E6D: /* LATIN SMALL LETTER T WITH DOT BELOW */
    case 0x1E6F: /* LATIN SMALL LETTER T WITH LINE BELOW */
    case 0x1E71: /* LATIN SMALL LETTER T WITH CIRCUMFLEX BELOW */
    case 0x1E73: /* LATIN SMALL LETTER U WITH DIAERESIS BELOW */
    case 0x1E75: /* LATIN SMALL LETTER U WITH TILDE BELOW */
    case 0x1E77: /* LATIN SMALL LETTER U WITH CIRCUMFLEX BELOW */
    case 0x1E79: /* LATIN SMALL LETTER U WITH TILDE AND ACUTE */
    case 0x1E7B: /* LATIN SMALL LETTER U WITH MACRON AND DIAERESIS */
    case 0x1E7D: /* LATIN SMALL LETTER V WITH TILDE */
    case 0x1E7F: /* LATIN SMALL LETTER V WITH DOT BELOW */
    case 0x1E81: /* LATIN SMALL LETTER W WITH GRAVE */
    case 0x1E83: /* LATIN SMALL LETTER W WITH ACUTE */
    case 0x1E85: /* LATIN SMALL LETTER W WITH DIAERESIS */
    case 0x1E87: /* LATIN SMALL LETTER W WITH DOT ABOVE */
    case 0x1E89: /* LATIN SMALL LETTER W WITH DOT BELOW */
    case 0x1E8B: /* LATIN SMALL LETTER X WITH DOT ABOVE */
    case 0x1E8D: /* LATIN SMALL LETTER X WITH DIAERESIS */
    case 0x1E8F: /* LATIN SMALL LETTER Y WITH DOT ABOVE */
    case 0x1E91: /* LATIN SMALL LETTER Z WITH CIRCUMFLEX */
    case 0x1E93: /* LATIN SMALL LETTER Z WITH DOT BELOW */
    case 0x1E95: /* LATIN SMALL LETTER Z WITH LINE BELOW */
    case 0x1E96: /* LATIN SMALL LETTER H WITH LINE BELOW */
    case 0x1E97: /* LATIN SMALL LETTER T WITH DIAERESIS */
    case 0x1E98: /* LATIN SMALL LETTER W WITH RING ABOVE */
    case 0x1E99: /* LATIN SMALL LETTER Y WITH RING ABOVE */
    case 0x1E9A: /* LATIN SMALL LETTER A WITH RIGHT HALF RING */
    case 0x1E9B: /* LATIN SMALL LETTER LONG S WITH DOT ABOVE */
    case 0x1EA1: /* LATIN SMALL LETTER A WITH DOT BELOW */
    case 0x1EA3: /* LATIN SMALL LETTER A WITH HOOK ABOVE */
    case 0x1EA5: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE */
    case 0x1EA7: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND GRAVE */
    case 0x1EA9: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1EAB: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND TILDE */
    case 0x1EAD: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EAF: /* LATIN SMALL LETTER A WITH BREVE AND ACUTE */
    case 0x1EB1: /* LATIN SMALL LETTER A WITH BREVE AND GRAVE */
    case 0x1EB3: /* LATIN SMALL LETTER A WITH BREVE AND HOOK ABOVE */
    case 0x1EB5: /* LATIN SMALL LETTER A WITH BREVE AND TILDE */
    case 0x1EB7: /* LATIN SMALL LETTER A WITH BREVE AND DOT BELOW */
    case 0x1EB9: /* LATIN SMALL LETTER E WITH DOT BELOW */
    case 0x1EBB: /* LATIN SMALL LETTER E WITH HOOK ABOVE */
    case 0x1EBD: /* LATIN SMALL LETTER E WITH TILDE */
    case 0x1EBF: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE */
    case 0x1EC1: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND GRAVE */
    case 0x1EC3: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1EC5: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND TILDE */
    case 0x1EC7: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EC9: /* LATIN SMALL LETTER I WITH HOOK ABOVE */
    case 0x1ECB: /* LATIN SMALL LETTER I WITH DOT BELOW */
    case 0x1ECD: /* LATIN SMALL LETTER O WITH DOT BELOW */
    case 0x1ECF: /* LATIN SMALL LETTER O WITH HOOK ABOVE */
    case 0x1ED1: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND ACUTE */
    case 0x1ED3: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND GRAVE */
    case 0x1ED5: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1ED7: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND TILDE */
    case 0x1ED9: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EDB: /* LATIN SMALL LETTER O WITH HORN AND ACUTE */
    case 0x1EDD: /* LATIN SMALL LETTER O WITH HORN AND GRAVE */
    case 0x1EDF: /* LATIN SMALL LETTER O WITH HORN AND HOOK ABOVE */
    case 0x1EE1: /* LATIN SMALL LETTER O WITH HORN AND TILDE */
    case 0x1EE3: /* LATIN SMALL LETTER O WITH HORN AND DOT BELOW */
    case 0x1EE5: /* LATIN SMALL LETTER U WITH DOT BELOW */
    case 0x1EE7: /* LATIN SMALL LETTER U WITH HOOK ABOVE */
    case 0x1EE9: /* LATIN SMALL LETTER U WITH HORN AND ACUTE */
    case 0x1EEB: /* LATIN SMALL LETTER U WITH HORN AND GRAVE */
    case 0x1EED: /* LATIN SMALL LETTER U WITH HORN AND HOOK ABOVE */
    case 0x1EEF: /* LATIN SMALL LETTER U WITH HORN AND TILDE */
    case 0x1EF1: /* LATIN SMALL LETTER U WITH HORN AND DOT BELOW */
    case 0x1EF3: /* LATIN SMALL LETTER Y WITH GRAVE */
    case 0x1EF5: /* LATIN SMALL LETTER Y WITH DOT BELOW */
    case 0x1EF7: /* LATIN SMALL LETTER Y WITH HOOK ABOVE */
    case 0x1EF9: /* LATIN SMALL LETTER Y WITH TILDE */
    case 0x1F00: /* GREEK SMALL LETTER ALPHA WITH PSILI */
    case 0x1F01: /* GREEK SMALL LETTER ALPHA WITH DASIA */
    case 0x1F02: /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA */
    case 0x1F03: /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA */
    case 0x1F04: /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA */
    case 0x1F05: /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA */
    case 0x1F06: /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI */
    case 0x1F07: /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI */
    case 0x1F10: /* GREEK SMALL LETTER EPSILON WITH PSILI */
    case 0x1F11: /* GREEK SMALL LETTER EPSILON WITH DASIA */
    case 0x1F12: /* GREEK SMALL LETTER EPSILON WITH PSILI AND VARIA */
    case 0x1F13: /* GREEK SMALL LETTER EPSILON WITH DASIA AND VARIA */
    case 0x1F14: /* GREEK SMALL LETTER EPSILON WITH PSILI AND OXIA */
    case 0x1F15: /* GREEK SMALL LETTER EPSILON WITH DASIA AND OXIA */
    case 0x1F20: /* GREEK SMALL LETTER ETA WITH PSILI */
    case 0x1F21: /* GREEK SMALL LETTER ETA WITH DASIA */
    case 0x1F22: /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA */
    case 0x1F23: /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA */
    case 0x1F24: /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA */
    case 0x1F25: /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA */
    case 0x1F26: /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI */
    case 0x1F27: /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI */
    case 0x1F30: /* GREEK SMALL LETTER IOTA WITH PSILI */
    case 0x1F31: /* GREEK SMALL LETTER IOTA WITH DASIA */
    case 0x1F32: /* GREEK SMALL LETTER IOTA WITH PSILI AND VARIA */
    case 0x1F33: /* GREEK SMALL LETTER IOTA WITH DASIA AND VARIA */
    case 0x1F34: /* GREEK SMALL LETTER IOTA WITH PSILI AND OXIA */
    case 0x1F35: /* GREEK SMALL LETTER IOTA WITH DASIA AND OXIA */
    case 0x1F36: /* GREEK SMALL LETTER IOTA WITH PSILI AND PERISPOMENI */
    case 0x1F37: /* GREEK SMALL LETTER IOTA WITH DASIA AND PERISPOMENI */
    case 0x1F40: /* GREEK SMALL LETTER OMICRON WITH PSILI */
    case 0x1F41: /* GREEK SMALL LETTER OMICRON WITH DASIA */
    case 0x1F42: /* GREEK SMALL LETTER OMICRON WITH PSILI AND VARIA */
    case 0x1F43: /* GREEK SMALL LETTER OMICRON WITH DASIA AND VARIA */
    case 0x1F44: /* GREEK SMALL LETTER OMICRON WITH PSILI AND OXIA */
    case 0x1F45: /* GREEK SMALL LETTER OMICRON WITH DASIA AND OXIA */
    case 0x1F50: /* GREEK SMALL LETTER UPSILON WITH PSILI */
    case 0x1F51: /* GREEK SMALL LETTER UPSILON WITH DASIA */
    case 0x1F52: /* GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA */
    case 0x1F53: /* GREEK SMALL LETTER UPSILON WITH DASIA AND VARIA */
    case 0x1F54: /* GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA */
    case 0x1F55: /* GREEK SMALL LETTER UPSILON WITH DASIA AND OXIA */
    case 0x1F56: /* GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI */
    case 0x1F57: /* GREEK SMALL LETTER UPSILON WITH DASIA AND PERISPOMENI */
    case 0x1F60: /* GREEK SMALL LETTER OMEGA WITH PSILI */
    case 0x1F61: /* GREEK SMALL LETTER OMEGA WITH DASIA */
    case 0x1F62: /* GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA */
    case 0x1F63: /* GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA */
    case 0x1F64: /* GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA */
    case 0x1F65: /* GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA */
    case 0x1F66: /* GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI */
    case 0x1F67: /* GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI */
    case 0x1F70: /* GREEK SMALL LETTER ALPHA WITH VARIA */
    case 0x1F71: /* GREEK SMALL LETTER ALPHA WITH OXIA */
    case 0x1F72: /* GREEK SMALL LETTER EPSILON WITH VARIA */
    case 0x1F73: /* GREEK SMALL LETTER EPSILON WITH OXIA */
    case 0x1F74: /* GREEK SMALL LETTER ETA WITH VARIA */
    case 0x1F75: /* GREEK SMALL LETTER ETA WITH OXIA */
    case 0x1F76: /* GREEK SMALL LETTER IOTA WITH VARIA */
    case 0x1F77: /* GREEK SMALL LETTER IOTA WITH OXIA */
    case 0x1F78: /* GREEK SMALL LETTER OMICRON WITH VARIA */
    case 0x1F79: /* GREEK SMALL LETTER OMICRON WITH OXIA */
    case 0x1F7A: /* GREEK SMALL LETTER UPSILON WITH VARIA */
    case 0x1F7B: /* GREEK SMALL LETTER UPSILON WITH OXIA */
    case 0x1F7C: /* GREEK SMALL LETTER OMEGA WITH VARIA */
    case 0x1F7D: /* GREEK SMALL LETTER OMEGA WITH OXIA */
    case 0x1F80: /* GREEK SMALL LETTER ALPHA WITH PSILI AND YPOGEGRAMMENI */
    case 0x1F81: /* GREEK SMALL LETTER ALPHA WITH DASIA AND YPOGEGRAMMENI */
    case 0x1F82: /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
    case 0x1F83: /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
    case 0x1F84: /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
    case 0x1F85: /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
    case 0x1F86: /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1F87: /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1F90: /* GREEK SMALL LETTER ETA WITH PSILI AND YPOGEGRAMMENI */
    case 0x1F91: /* GREEK SMALL LETTER ETA WITH DASIA AND YPOGEGRAMMENI */
    case 0x1F92: /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
    case 0x1F93: /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
    case 0x1F94: /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
    case 0x1F95: /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
    case 0x1F96: /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1F97: /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1FA0: /* GREEK SMALL LETTER OMEGA WITH PSILI AND YPOGEGRAMMENI */
    case 0x1FA1: /* GREEK SMALL LETTER OMEGA WITH DASIA AND YPOGEGRAMMENI */
    case 0x1FA2: /* GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
    case 0x1FA3: /* GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
    case 0x1FA4: /* GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
    case 0x1FA5: /* GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
    case 0x1FA6: /* GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1FA7: /* GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1FB0: /* GREEK SMALL LETTER ALPHA WITH VRACHY */
    case 0x1FB1: /* GREEK SMALL LETTER ALPHA WITH MACRON */
    case 0x1FB2: /* GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI */
    case 0x1FB3: /* GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI */
    case 0x1FB4: /* GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI */
    case 0x1FB6: /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI */
    case 0x1FB7: /* GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1FBE: /* GREEK PROSGEGRAMMENI */
    case 0x1FC2: /* GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI */
    case 0x1FC3: /* GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI */
    case 0x1FC4: /* GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI */
    case 0x1FC6: /* GREEK SMALL LETTER ETA WITH PERISPOMENI */
    case 0x1FC7: /* GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI */
    case 0x1FD0: /* GREEK SMALL LETTER IOTA WITH VRACHY */
    case 0x1FD1: /* GREEK SMALL LETTER IOTA WITH MACRON */
    case 0x1FD2: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA */
    case 0x1FD3: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA */
    case 0x1FD6: /* GREEK SMALL LETTER IOTA WITH PERISPOMENI */
    case 0x1FD7: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI */
    case 0x1FE0: /* GREEK SMALL LETTER UPSILON WITH VRACHY */
    case 0x1FE1: /* GREEK SMALL LETTER UPSILON WITH MACRON */
    case 0x1FE2: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA */
    case 0x1FE3: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA */
    case 0x1FE4: /* GREEK SMALL LETTER RHO WITH PSILI */
    case 0x1FE5: /* GREEK SMALL LETTER RHO WITH DASIA */
    case 0x1FE6: /* GREEK SMALL LETTER UPSILON WITH PERISPOMENI */
    case 0x1FE7: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI */
    case 0x1FF2: /* GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI */
    case 0x1FF3: /* GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI */
    case 0x1FF4: /* GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI */
    case 0x1FF6: /* GREEK SMALL LETTER OMEGA WITH PERISPOMENI */
    case 0x1FF7: /* GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND YPOGEGRAMMENI */
    case 0x207F: /* SUPERSCRIPT LATIN SMALL LETTER N */
    case 0x210A: /* SCRIPT SMALL G */
    case 0x210E: /* PLANCK CONSTANT */
    case 0x210F: /* PLANCK CONSTANT OVER TWO PI */
    case 0x2113: /* SCRIPT SMALL L */
    case 0x212F: /* SCRIPT SMALL E */
    case 0x2134: /* SCRIPT SMALL O */
    case 0x2139: /* INFORMATION SOURCE */
    case 0xFB00: /* LATIN SMALL LIGATURE FF */
    case 0xFB01: /* LATIN SMALL LIGATURE FI */
    case 0xFB02: /* LATIN SMALL LIGATURE FL */
    case 0xFB03: /* LATIN SMALL LIGATURE FFI */
    case 0xFB04: /* LATIN SMALL LIGATURE FFL */
    case 0xFB05: /* LATIN SMALL LIGATURE LONG S T */
    case 0xFB06: /* LATIN SMALL LIGATURE ST */
    case 0xFB13: /* ARMENIAN SMALL LIGATURE MEN NOW */
    case 0xFB14: /* ARMENIAN SMALL LIGATURE MEN ECH */
    case 0xFB15: /* ARMENIAN SMALL LIGATURE MEN INI */
    case 0xFB16: /* ARMENIAN SMALL LIGATURE VEW NOW */
    case 0xFB17: /* ARMENIAN SMALL LIGATURE MEN XEH */
    case 0xFF41: /* FULLWIDTH LATIN SMALL LETTER A */
    case 0xFF42: /* FULLWIDTH LATIN SMALL LETTER B */
    case 0xFF43: /* FULLWIDTH LATIN SMALL LETTER C */
    case 0xFF44: /* FULLWIDTH LATIN SMALL LETTER D */
    case 0xFF45: /* FULLWIDTH LATIN SMALL LETTER E */
    case 0xFF46: /* FULLWIDTH LATIN SMALL LETTER F */
    case 0xFF47: /* FULLWIDTH LATIN SMALL LETTER G */
    case 0xFF48: /* FULLWIDTH LATIN SMALL LETTER H */
    case 0xFF49: /* FULLWIDTH LATIN SMALL LETTER I */
    case 0xFF4A: /* FULLWIDTH LATIN SMALL LETTER J */
    case 0xFF4B: /* FULLWIDTH LATIN SMALL LETTER K */
    case 0xFF4C: /* FULLWIDTH LATIN SMALL LETTER L */
    case 0xFF4D: /* FULLWIDTH LATIN SMALL LETTER M */
    case 0xFF4E: /* FULLWIDTH LATIN SMALL LETTER N */
    case 0xFF4F: /* FULLWIDTH LATIN SMALL LETTER O */
    case 0xFF50: /* FULLWIDTH LATIN SMALL LETTER P */
    case 0xFF51: /* FULLWIDTH LATIN SMALL LETTER Q */
    case 0xFF52: /* FULLWIDTH LATIN SMALL LETTER R */
    case 0xFF53: /* FULLWIDTH LATIN SMALL LETTER S */
    case 0xFF54: /* FULLWIDTH LATIN SMALL LETTER T */
    case 0xFF55: /* FULLWIDTH LATIN SMALL LETTER U */
    case 0xFF56: /* FULLWIDTH LATIN SMALL LETTER V */
    case 0xFF57: /* FULLWIDTH LATIN SMALL LETTER W */
    case 0xFF58: /* FULLWIDTH LATIN SMALL LETTER X */
    case 0xFF59: /* FULLWIDTH LATIN SMALL LETTER Y */
    case 0xFF5A: /* FULLWIDTH LATIN SMALL LETTER Z */
	return 1;
    default:
	return 0;
    }
}

/* Returns 1 for Unicode characters having the category 'Lu', 0
   otherwise. */

int _PyUnicode_IsUppercase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0041: /* LATIN CAPITAL LETTER A */
    case 0x0042: /* LATIN CAPITAL LETTER B */
    case 0x0043: /* LATIN CAPITAL LETTER C */
    case 0x0044: /* LATIN CAPITAL LETTER D */
    case 0x0045: /* LATIN CAPITAL LETTER E */
    case 0x0046: /* LATIN CAPITAL LETTER F */
    case 0x0047: /* LATIN CAPITAL LETTER G */
    case 0x0048: /* LATIN CAPITAL LETTER H */
    case 0x0049: /* LATIN CAPITAL LETTER I */
    case 0x004A: /* LATIN CAPITAL LETTER J */
    case 0x004B: /* LATIN CAPITAL LETTER K */
    case 0x004C: /* LATIN CAPITAL LETTER L */
    case 0x004D: /* LATIN CAPITAL LETTER M */
    case 0x004E: /* LATIN CAPITAL LETTER N */
    case 0x004F: /* LATIN CAPITAL LETTER O */
    case 0x0050: /* LATIN CAPITAL LETTER P */
    case 0x0051: /* LATIN CAPITAL LETTER Q */
    case 0x0052: /* LATIN CAPITAL LETTER R */
    case 0x0053: /* LATIN CAPITAL LETTER S */
    case 0x0054: /* LATIN CAPITAL LETTER T */
    case 0x0055: /* LATIN CAPITAL LETTER U */
    case 0x0056: /* LATIN CAPITAL LETTER V */
    case 0x0057: /* LATIN CAPITAL LETTER W */
    case 0x0058: /* LATIN CAPITAL LETTER X */
    case 0x0059: /* LATIN CAPITAL LETTER Y */
    case 0x005A: /* LATIN CAPITAL LETTER Z */
    case 0x00C0: /* LATIN CAPITAL LETTER A WITH GRAVE */
    case 0x00C1: /* LATIN CAPITAL LETTER A WITH ACUTE */
    case 0x00C2: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
    case 0x00C3: /* LATIN CAPITAL LETTER A WITH TILDE */
    case 0x00C4: /* LATIN CAPITAL LETTER A WITH DIAERESIS */
    case 0x00C5: /* LATIN CAPITAL LETTER A WITH RING ABOVE */
    case 0x00C6: /* LATIN CAPITAL LETTER AE */
    case 0x00C7: /* LATIN CAPITAL LETTER C WITH CEDILLA */
    case 0x00C8: /* LATIN CAPITAL LETTER E WITH GRAVE */
    case 0x00C9: /* LATIN CAPITAL LETTER E WITH ACUTE */
    case 0x00CA: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
    case 0x00CB: /* LATIN CAPITAL LETTER E WITH DIAERESIS */
    case 0x00CC: /* LATIN CAPITAL LETTER I WITH GRAVE */
    case 0x00CD: /* LATIN CAPITAL LETTER I WITH ACUTE */
    case 0x00CE: /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
    case 0x00CF: /* LATIN CAPITAL LETTER I WITH DIAERESIS */
    case 0x00D0: /* LATIN CAPITAL LETTER ETH */
    case 0x00D1: /* LATIN CAPITAL LETTER N WITH TILDE */
    case 0x00D2: /* LATIN CAPITAL LETTER O WITH GRAVE */
    case 0x00D3: /* LATIN CAPITAL LETTER O WITH ACUTE */
    case 0x00D4: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
    case 0x00D5: /* LATIN CAPITAL LETTER O WITH TILDE */
    case 0x00D6: /* LATIN CAPITAL LETTER O WITH DIAERESIS */
    case 0x00D8: /* LATIN CAPITAL LETTER O WITH STROKE */
    case 0x00D9: /* LATIN CAPITAL LETTER U WITH GRAVE */
    case 0x00DA: /* LATIN CAPITAL LETTER U WITH ACUTE */
    case 0x00DB: /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
    case 0x00DC: /* LATIN CAPITAL LETTER U WITH DIAERESIS */
    case 0x00DD: /* LATIN CAPITAL LETTER Y WITH ACUTE */
    case 0x00DE: /* LATIN CAPITAL LETTER THORN */
    case 0x0100: /* LATIN CAPITAL LETTER A WITH MACRON */
    case 0x0102: /* LATIN CAPITAL LETTER A WITH BREVE */
    case 0x0104: /* LATIN CAPITAL LETTER A WITH OGONEK */
    case 0x0106: /* LATIN CAPITAL LETTER C WITH ACUTE */
    case 0x0108: /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    case 0x010A: /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
    case 0x010C: /* LATIN CAPITAL LETTER C WITH CARON */
    case 0x010E: /* LATIN CAPITAL LETTER D WITH CARON */
    case 0x0110: /* LATIN CAPITAL LETTER D WITH STROKE */
    case 0x0112: /* LATIN CAPITAL LETTER E WITH MACRON */
    case 0x0114: /* LATIN CAPITAL LETTER E WITH BREVE */
    case 0x0116: /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
    case 0x0118: /* LATIN CAPITAL LETTER E WITH OGONEK */
    case 0x011A: /* LATIN CAPITAL LETTER E WITH CARON */
    case 0x011C: /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    case 0x011E: /* LATIN CAPITAL LETTER G WITH BREVE */
    case 0x0120: /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
    case 0x0122: /* LATIN CAPITAL LETTER G WITH CEDILLA */
    case 0x0124: /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    case 0x0126: /* LATIN CAPITAL LETTER H WITH STROKE */
    case 0x0128: /* LATIN CAPITAL LETTER I WITH TILDE */
    case 0x012A: /* LATIN CAPITAL LETTER I WITH MACRON */
    case 0x012C: /* LATIN CAPITAL LETTER I WITH BREVE */
    case 0x012E: /* LATIN CAPITAL LETTER I WITH OGONEK */
    case 0x0130: /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
    case 0x0132: /* LATIN CAPITAL LIGATURE IJ */
    case 0x0134: /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    case 0x0136: /* LATIN CAPITAL LETTER K WITH CEDILLA */
    case 0x0139: /* LATIN CAPITAL LETTER L WITH ACUTE */
    case 0x013B: /* LATIN CAPITAL LETTER L WITH CEDILLA */
    case 0x013D: /* LATIN CAPITAL LETTER L WITH CARON */
    case 0x013F: /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
    case 0x0141: /* LATIN CAPITAL LETTER L WITH STROKE */
    case 0x0143: /* LATIN CAPITAL LETTER N WITH ACUTE */
    case 0x0145: /* LATIN CAPITAL LETTER N WITH CEDILLA */
    case 0x0147: /* LATIN CAPITAL LETTER N WITH CARON */
    case 0x014A: /* LATIN CAPITAL LETTER ENG */
    case 0x014C: /* LATIN CAPITAL LETTER O WITH MACRON */
    case 0x014E: /* LATIN CAPITAL LETTER O WITH BREVE */
    case 0x0150: /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    case 0x0152: /* LATIN CAPITAL LIGATURE OE */
    case 0x0154: /* LATIN CAPITAL LETTER R WITH ACUTE */
    case 0x0156: /* LATIN CAPITAL LETTER R WITH CEDILLA */
    case 0x0158: /* LATIN CAPITAL LETTER R WITH CARON */
    case 0x015A: /* LATIN CAPITAL LETTER S WITH ACUTE */
    case 0x015C: /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    case 0x015E: /* LATIN CAPITAL LETTER S WITH CEDILLA */
    case 0x0160: /* LATIN CAPITAL LETTER S WITH CARON */
    case 0x0162: /* LATIN CAPITAL LETTER T WITH CEDILLA */
    case 0x0164: /* LATIN CAPITAL LETTER T WITH CARON */
    case 0x0166: /* LATIN CAPITAL LETTER T WITH STROKE */
    case 0x0168: /* LATIN CAPITAL LETTER U WITH TILDE */
    case 0x016A: /* LATIN CAPITAL LETTER U WITH MACRON */
    case 0x016C: /* LATIN CAPITAL LETTER U WITH BREVE */
    case 0x016E: /* LATIN CAPITAL LETTER U WITH RING ABOVE */
    case 0x0170: /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    case 0x0172: /* LATIN CAPITAL LETTER U WITH OGONEK */
    case 0x0174: /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
    case 0x0176: /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
    case 0x0178: /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
    case 0x0179: /* LATIN CAPITAL LETTER Z WITH ACUTE */
    case 0x017B: /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    case 0x017D: /* LATIN CAPITAL LETTER Z WITH CARON */
    case 0x0181: /* LATIN CAPITAL LETTER B WITH HOOK */
    case 0x0182: /* LATIN CAPITAL LETTER B WITH TOPBAR */
    case 0x0184: /* LATIN CAPITAL LETTER TONE SIX */
    case 0x0186: /* LATIN CAPITAL LETTER OPEN O */
    case 0x0187: /* LATIN CAPITAL LETTER C WITH HOOK */
    case 0x0189: /* LATIN CAPITAL LETTER AFRICAN D */
    case 0x018A: /* LATIN CAPITAL LETTER D WITH HOOK */
    case 0x018B: /* LATIN CAPITAL LETTER D WITH TOPBAR */
    case 0x018E: /* LATIN CAPITAL LETTER REVERSED E */
    case 0x018F: /* LATIN CAPITAL LETTER SCHWA */
    case 0x0190: /* LATIN CAPITAL LETTER OPEN E */
    case 0x0191: /* LATIN CAPITAL LETTER F WITH HOOK */
    case 0x0193: /* LATIN CAPITAL LETTER G WITH HOOK */
    case 0x0194: /* LATIN CAPITAL LETTER GAMMA */
    case 0x0196: /* LATIN CAPITAL LETTER IOTA */
    case 0x0197: /* LATIN CAPITAL LETTER I WITH STROKE */
    case 0x0198: /* LATIN CAPITAL LETTER K WITH HOOK */
    case 0x019C: /* LATIN CAPITAL LETTER TURNED M */
    case 0x019D: /* LATIN CAPITAL LETTER N WITH LEFT HOOK */
    case 0x019F: /* LATIN CAPITAL LETTER O WITH MIDDLE TILDE */
    case 0x01A0: /* LATIN CAPITAL LETTER O WITH HORN */
    case 0x01A2: /* LATIN CAPITAL LETTER OI */
    case 0x01A4: /* LATIN CAPITAL LETTER P WITH HOOK */
    case 0x01A6: /* LATIN LETTER YR */
    case 0x01A7: /* LATIN CAPITAL LETTER TONE TWO */
    case 0x01A9: /* LATIN CAPITAL LETTER ESH */
    case 0x01AC: /* LATIN CAPITAL LETTER T WITH HOOK */
    case 0x01AE: /* LATIN CAPITAL LETTER T WITH RETROFLEX HOOK */
    case 0x01AF: /* LATIN CAPITAL LETTER U WITH HORN */
    case 0x01B1: /* LATIN CAPITAL LETTER UPSILON */
    case 0x01B2: /* LATIN CAPITAL LETTER V WITH HOOK */
    case 0x01B3: /* LATIN CAPITAL LETTER Y WITH HOOK */
    case 0x01B5: /* LATIN CAPITAL LETTER Z WITH STROKE */
    case 0x01B7: /* LATIN CAPITAL LETTER EZH */
    case 0x01B8: /* LATIN CAPITAL LETTER EZH REVERSED */
    case 0x01BC: /* LATIN CAPITAL LETTER TONE FIVE */
    case 0x01C4: /* LATIN CAPITAL LETTER DZ WITH CARON */
    case 0x01C7: /* LATIN CAPITAL LETTER LJ */
    case 0x01CA: /* LATIN CAPITAL LETTER NJ */
    case 0x01CD: /* LATIN CAPITAL LETTER A WITH CARON */
    case 0x01CF: /* LATIN CAPITAL LETTER I WITH CARON */
    case 0x01D1: /* LATIN CAPITAL LETTER O WITH CARON */
    case 0x01D3: /* LATIN CAPITAL LETTER U WITH CARON */
    case 0x01D5: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON */
    case 0x01D7: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE */
    case 0x01D9: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND CARON */
    case 0x01DB: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND GRAVE */
    case 0x01DE: /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
    case 0x01E0: /* LATIN CAPITAL LETTER A WITH DOT ABOVE AND MACRON */
    case 0x01E2: /* LATIN CAPITAL LETTER AE WITH MACRON */
    case 0x01E4: /* LATIN CAPITAL LETTER G WITH STROKE */
    case 0x01E6: /* LATIN CAPITAL LETTER G WITH CARON */
    case 0x01E8: /* LATIN CAPITAL LETTER K WITH CARON */
    case 0x01EA: /* LATIN CAPITAL LETTER O WITH OGONEK */
    case 0x01EC: /* LATIN CAPITAL LETTER O WITH OGONEK AND MACRON */
    case 0x01EE: /* LATIN CAPITAL LETTER EZH WITH CARON */
    case 0x01F1: /* LATIN CAPITAL LETTER DZ */
    case 0x01F4: /* LATIN CAPITAL LETTER G WITH ACUTE */
    case 0x01F6: /* LATIN CAPITAL LETTER HWAIR */
    case 0x01F7: /* LATIN CAPITAL LETTER WYNN */
    case 0x01F8: /* LATIN CAPITAL LETTER N WITH GRAVE */
    case 0x01FA: /* LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE */
    case 0x01FC: /* LATIN CAPITAL LETTER AE WITH ACUTE */
    case 0x01FE: /* LATIN CAPITAL LETTER O WITH STROKE AND ACUTE */
    case 0x0200: /* LATIN CAPITAL LETTER A WITH DOUBLE GRAVE */
    case 0x0202: /* LATIN CAPITAL LETTER A WITH INVERTED BREVE */
    case 0x0204: /* LATIN CAPITAL LETTER E WITH DOUBLE GRAVE */
    case 0x0206: /* LATIN CAPITAL LETTER E WITH INVERTED BREVE */
    case 0x0208: /* LATIN CAPITAL LETTER I WITH DOUBLE GRAVE */
    case 0x020A: /* LATIN CAPITAL LETTER I WITH INVERTED BREVE */
    case 0x020C: /* LATIN CAPITAL LETTER O WITH DOUBLE GRAVE */
    case 0x020E: /* LATIN CAPITAL LETTER O WITH INVERTED BREVE */
    case 0x0210: /* LATIN CAPITAL LETTER R WITH DOUBLE GRAVE */
    case 0x0212: /* LATIN CAPITAL LETTER R WITH INVERTED BREVE */
    case 0x0214: /* LATIN CAPITAL LETTER U WITH DOUBLE GRAVE */
    case 0x0216: /* LATIN CAPITAL LETTER U WITH INVERTED BREVE */
    case 0x0218: /* LATIN CAPITAL LETTER S WITH COMMA BELOW */
    case 0x021A: /* LATIN CAPITAL LETTER T WITH COMMA BELOW */
    case 0x021C: /* LATIN CAPITAL LETTER YOGH */
    case 0x021E: /* LATIN CAPITAL LETTER H WITH CARON */
    case 0x0222: /* LATIN CAPITAL LETTER OU */
    case 0x0224: /* LATIN CAPITAL LETTER Z WITH HOOK */
    case 0x0226: /* LATIN CAPITAL LETTER A WITH DOT ABOVE */
    case 0x0228: /* LATIN CAPITAL LETTER E WITH CEDILLA */
    case 0x022A: /* LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON */
    case 0x022C: /* LATIN CAPITAL LETTER O WITH TILDE AND MACRON */
    case 0x022E: /* LATIN CAPITAL LETTER O WITH DOT ABOVE */
    case 0x0230: /* LATIN CAPITAL LETTER O WITH DOT ABOVE AND MACRON */
    case 0x0232: /* LATIN CAPITAL LETTER Y WITH MACRON */
    case 0x0386: /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
    case 0x0388: /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
    case 0x0389: /* GREEK CAPITAL LETTER ETA WITH TONOS */
    case 0x038A: /* GREEK CAPITAL LETTER IOTA WITH TONOS */
    case 0x038C: /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
    case 0x038E: /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
    case 0x038F: /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
    case 0x0391: /* GREEK CAPITAL LETTER ALPHA */
    case 0x0392: /* GREEK CAPITAL LETTER BETA */
    case 0x0393: /* GREEK CAPITAL LETTER GAMMA */
    case 0x0394: /* GREEK CAPITAL LETTER DELTA */
    case 0x0395: /* GREEK CAPITAL LETTER EPSILON */
    case 0x0396: /* GREEK CAPITAL LETTER ZETA */
    case 0x0397: /* GREEK CAPITAL LETTER ETA */
    case 0x0398: /* GREEK CAPITAL LETTER THETA */
    case 0x0399: /* GREEK CAPITAL LETTER IOTA */
    case 0x039A: /* GREEK CAPITAL LETTER KAPPA */
    case 0x039B: /* GREEK CAPITAL LETTER LAMDA */
    case 0x039C: /* GREEK CAPITAL LETTER MU */
    case 0x039D: /* GREEK CAPITAL LETTER NU */
    case 0x039E: /* GREEK CAPITAL LETTER XI */
    case 0x039F: /* GREEK CAPITAL LETTER OMICRON */
    case 0x03A0: /* GREEK CAPITAL LETTER PI */
    case 0x03A1: /* GREEK CAPITAL LETTER RHO */
    case 0x03A3: /* GREEK CAPITAL LETTER SIGMA */
    case 0x03A4: /* GREEK CAPITAL LETTER TAU */
    case 0x03A5: /* GREEK CAPITAL LETTER UPSILON */
    case 0x03A6: /* GREEK CAPITAL LETTER PHI */
    case 0x03A7: /* GREEK CAPITAL LETTER CHI */
    case 0x03A8: /* GREEK CAPITAL LETTER PSI */
    case 0x03A9: /* GREEK CAPITAL LETTER OMEGA */
    case 0x03AA: /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    case 0x03AB: /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    case 0x03D2: /* GREEK UPSILON WITH HOOK SYMBOL */
    case 0x03D3: /* GREEK UPSILON WITH ACUTE AND HOOK SYMBOL */
    case 0x03D4: /* GREEK UPSILON WITH DIAERESIS AND HOOK SYMBOL */
    case 0x03DA: /* GREEK LETTER STIGMA */
    case 0x03DC: /* GREEK LETTER DIGAMMA */
    case 0x03DE: /* GREEK LETTER KOPPA */
    case 0x03E0: /* GREEK LETTER SAMPI */
    case 0x03E2: /* COPTIC CAPITAL LETTER SHEI */
    case 0x03E4: /* COPTIC CAPITAL LETTER FEI */
    case 0x03E6: /* COPTIC CAPITAL LETTER KHEI */
    case 0x03E8: /* COPTIC CAPITAL LETTER HORI */
    case 0x03EA: /* COPTIC CAPITAL LETTER GANGIA */
    case 0x03EC: /* COPTIC CAPITAL LETTER SHIMA */
    case 0x03EE: /* COPTIC CAPITAL LETTER DEI */
    case 0x0400: /* CYRILLIC CAPITAL LETTER IE WITH GRAVE */
    case 0x0401: /* CYRILLIC CAPITAL LETTER IO */
    case 0x0402: /* CYRILLIC CAPITAL LETTER DJE */
    case 0x0403: /* CYRILLIC CAPITAL LETTER GJE */
    case 0x0404: /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    case 0x0405: /* CYRILLIC CAPITAL LETTER DZE */
    case 0x0406: /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    case 0x0407: /* CYRILLIC CAPITAL LETTER YI */
    case 0x0408: /* CYRILLIC CAPITAL LETTER JE */
    case 0x0409: /* CYRILLIC CAPITAL LETTER LJE */
    case 0x040A: /* CYRILLIC CAPITAL LETTER NJE */
    case 0x040B: /* CYRILLIC CAPITAL LETTER TSHE */
    case 0x040C: /* CYRILLIC CAPITAL LETTER KJE */
    case 0x040D: /* CYRILLIC CAPITAL LETTER I WITH GRAVE */
    case 0x040E: /* CYRILLIC CAPITAL LETTER SHORT U */
    case 0x040F: /* CYRILLIC CAPITAL LETTER DZHE */
    case 0x0410: /* CYRILLIC CAPITAL LETTER A */
    case 0x0411: /* CYRILLIC CAPITAL LETTER BE */
    case 0x0412: /* CYRILLIC CAPITAL LETTER VE */
    case 0x0413: /* CYRILLIC CAPITAL LETTER GHE */
    case 0x0414: /* CYRILLIC CAPITAL LETTER DE */
    case 0x0415: /* CYRILLIC CAPITAL LETTER IE */
    case 0x0416: /* CYRILLIC CAPITAL LETTER ZHE */
    case 0x0417: /* CYRILLIC CAPITAL LETTER ZE */
    case 0x0418: /* CYRILLIC CAPITAL LETTER I */
    case 0x0419: /* CYRILLIC CAPITAL LETTER SHORT I */
    case 0x041A: /* CYRILLIC CAPITAL LETTER KA */
    case 0x041B: /* CYRILLIC CAPITAL LETTER EL */
    case 0x041C: /* CYRILLIC CAPITAL LETTER EM */
    case 0x041D: /* CYRILLIC CAPITAL LETTER EN */
    case 0x041E: /* CYRILLIC CAPITAL LETTER O */
    case 0x041F: /* CYRILLIC CAPITAL LETTER PE */
    case 0x0420: /* CYRILLIC CAPITAL LETTER ER */
    case 0x0421: /* CYRILLIC CAPITAL LETTER ES */
    case 0x0422: /* CYRILLIC CAPITAL LETTER TE */
    case 0x0423: /* CYRILLIC CAPITAL LETTER U */
    case 0x0424: /* CYRILLIC CAPITAL LETTER EF */
    case 0x0425: /* CYRILLIC CAPITAL LETTER HA */
    case 0x0426: /* CYRILLIC CAPITAL LETTER TSE */
    case 0x0427: /* CYRILLIC CAPITAL LETTER CHE */
    case 0x0428: /* CYRILLIC CAPITAL LETTER SHA */
    case 0x0429: /* CYRILLIC CAPITAL LETTER SHCHA */
    case 0x042A: /* CYRILLIC CAPITAL LETTER HARD SIGN */
    case 0x042B: /* CYRILLIC CAPITAL LETTER YERU */
    case 0x042C: /* CYRILLIC CAPITAL LETTER SOFT SIGN */
    case 0x042D: /* CYRILLIC CAPITAL LETTER E */
    case 0x042E: /* CYRILLIC CAPITAL LETTER YU */
    case 0x042F: /* CYRILLIC CAPITAL LETTER YA */
    case 0x0460: /* CYRILLIC CAPITAL LETTER OMEGA */
    case 0x0462: /* CYRILLIC CAPITAL LETTER YAT */
    case 0x0464: /* CYRILLIC CAPITAL LETTER IOTIFIED E */
    case 0x0466: /* CYRILLIC CAPITAL LETTER LITTLE YUS */
    case 0x0468: /* CYRILLIC CAPITAL LETTER IOTIFIED LITTLE YUS */
    case 0x046A: /* CYRILLIC CAPITAL LETTER BIG YUS */
    case 0x046C: /* CYRILLIC CAPITAL LETTER IOTIFIED BIG YUS */
    case 0x046E: /* CYRILLIC CAPITAL LETTER KSI */
    case 0x0470: /* CYRILLIC CAPITAL LETTER PSI */
    case 0x0472: /* CYRILLIC CAPITAL LETTER FITA */
    case 0x0474: /* CYRILLIC CAPITAL LETTER IZHITSA */
    case 0x0476: /* CYRILLIC CAPITAL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
    case 0x0478: /* CYRILLIC CAPITAL LETTER UK */
    case 0x047A: /* CYRILLIC CAPITAL LETTER ROUND OMEGA */
    case 0x047C: /* CYRILLIC CAPITAL LETTER OMEGA WITH TITLO */
    case 0x047E: /* CYRILLIC CAPITAL LETTER OT */
    case 0x0480: /* CYRILLIC CAPITAL LETTER KOPPA */
    case 0x048C: /* CYRILLIC CAPITAL LETTER SEMISOFT SIGN */
    case 0x048E: /* CYRILLIC CAPITAL LETTER ER WITH TICK */
    case 0x0490: /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
    case 0x0492: /* CYRILLIC CAPITAL LETTER GHE WITH STROKE */
    case 0x0494: /* CYRILLIC CAPITAL LETTER GHE WITH MIDDLE HOOK */
    case 0x0496: /* CYRILLIC CAPITAL LETTER ZHE WITH DESCENDER */
    case 0x0498: /* CYRILLIC CAPITAL LETTER ZE WITH DESCENDER */
    case 0x049A: /* CYRILLIC CAPITAL LETTER KA WITH DESCENDER */
    case 0x049C: /* CYRILLIC CAPITAL LETTER KA WITH VERTICAL STROKE */
    case 0x049E: /* CYRILLIC CAPITAL LETTER KA WITH STROKE */
    case 0x04A0: /* CYRILLIC CAPITAL LETTER BASHKIR KA */
    case 0x04A2: /* CYRILLIC CAPITAL LETTER EN WITH DESCENDER */
    case 0x04A4: /* CYRILLIC CAPITAL LIGATURE EN GHE */
    case 0x04A6: /* CYRILLIC CAPITAL LETTER PE WITH MIDDLE HOOK */
    case 0x04A8: /* CYRILLIC CAPITAL LETTER ABKHASIAN HA */
    case 0x04AA: /* CYRILLIC CAPITAL LETTER ES WITH DESCENDER */
    case 0x04AC: /* CYRILLIC CAPITAL LETTER TE WITH DESCENDER */
    case 0x04AE: /* CYRILLIC CAPITAL LETTER STRAIGHT U */
    case 0x04B0: /* CYRILLIC CAPITAL LETTER STRAIGHT U WITH STROKE */
    case 0x04B2: /* CYRILLIC CAPITAL LETTER HA WITH DESCENDER */
    case 0x04B4: /* CYRILLIC CAPITAL LIGATURE TE TSE */
    case 0x04B6: /* CYRILLIC CAPITAL LETTER CHE WITH DESCENDER */
    case 0x04B8: /* CYRILLIC CAPITAL LETTER CHE WITH VERTICAL STROKE */
    case 0x04BA: /* CYRILLIC CAPITAL LETTER SHHA */
    case 0x04BC: /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE */
    case 0x04BE: /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE WITH DESCENDER */
    case 0x04C0: /* CYRILLIC LETTER PALOCHKA */
    case 0x04C1: /* CYRILLIC CAPITAL LETTER ZHE WITH BREVE */
    case 0x04C3: /* CYRILLIC CAPITAL LETTER KA WITH HOOK */
    case 0x04C7: /* CYRILLIC CAPITAL LETTER EN WITH HOOK */
    case 0x04CB: /* CYRILLIC CAPITAL LETTER KHAKASSIAN CHE */
    case 0x04D0: /* CYRILLIC CAPITAL LETTER A WITH BREVE */
    case 0x04D2: /* CYRILLIC CAPITAL LETTER A WITH DIAERESIS */
    case 0x04D4: /* CYRILLIC CAPITAL LIGATURE A IE */
    case 0x04D6: /* CYRILLIC CAPITAL LETTER IE WITH BREVE */
    case 0x04D8: /* CYRILLIC CAPITAL LETTER SCHWA */
    case 0x04DA: /* CYRILLIC CAPITAL LETTER SCHWA WITH DIAERESIS */
    case 0x04DC: /* CYRILLIC CAPITAL LETTER ZHE WITH DIAERESIS */
    case 0x04DE: /* CYRILLIC CAPITAL LETTER ZE WITH DIAERESIS */
    case 0x04E0: /* CYRILLIC CAPITAL LETTER ABKHASIAN DZE */
    case 0x04E2: /* CYRILLIC CAPITAL LETTER I WITH MACRON */
    case 0x04E4: /* CYRILLIC CAPITAL LETTER I WITH DIAERESIS */
    case 0x04E6: /* CYRILLIC CAPITAL LETTER O WITH DIAERESIS */
    case 0x04E8: /* CYRILLIC CAPITAL LETTER BARRED O */
    case 0x04EA: /* CYRILLIC CAPITAL LETTER BARRED O WITH DIAERESIS */
    case 0x04EC: /* CYRILLIC CAPITAL LETTER E WITH DIAERESIS */
    case 0x04EE: /* CYRILLIC CAPITAL LETTER U WITH MACRON */
    case 0x04F0: /* CYRILLIC CAPITAL LETTER U WITH DIAERESIS */
    case 0x04F2: /* CYRILLIC CAPITAL LETTER U WITH DOUBLE ACUTE */
    case 0x04F4: /* CYRILLIC CAPITAL LETTER CHE WITH DIAERESIS */
    case 0x04F8: /* CYRILLIC CAPITAL LETTER YERU WITH DIAERESIS */
    case 0x0531: /* ARMENIAN CAPITAL LETTER AYB */
    case 0x0532: /* ARMENIAN CAPITAL LETTER BEN */
    case 0x0533: /* ARMENIAN CAPITAL LETTER GIM */
    case 0x0534: /* ARMENIAN CAPITAL LETTER DA */
    case 0x0535: /* ARMENIAN CAPITAL LETTER ECH */
    case 0x0536: /* ARMENIAN CAPITAL LETTER ZA */
    case 0x0537: /* ARMENIAN CAPITAL LETTER EH */
    case 0x0538: /* ARMENIAN CAPITAL LETTER ET */
    case 0x0539: /* ARMENIAN CAPITAL LETTER TO */
    case 0x053A: /* ARMENIAN CAPITAL LETTER ZHE */
    case 0x053B: /* ARMENIAN CAPITAL LETTER INI */
    case 0x053C: /* ARMENIAN CAPITAL LETTER LIWN */
    case 0x053D: /* ARMENIAN CAPITAL LETTER XEH */
    case 0x053E: /* ARMENIAN CAPITAL LETTER CA */
    case 0x053F: /* ARMENIAN CAPITAL LETTER KEN */
    case 0x0540: /* ARMENIAN CAPITAL LETTER HO */
    case 0x0541: /* ARMENIAN CAPITAL LETTER JA */
    case 0x0542: /* ARMENIAN CAPITAL LETTER GHAD */
    case 0x0543: /* ARMENIAN CAPITAL LETTER CHEH */
    case 0x0544: /* ARMENIAN CAPITAL LETTER MEN */
    case 0x0545: /* ARMENIAN CAPITAL LETTER YI */
    case 0x0546: /* ARMENIAN CAPITAL LETTER NOW */
    case 0x0547: /* ARMENIAN CAPITAL LETTER SHA */
    case 0x0548: /* ARMENIAN CAPITAL LETTER VO */
    case 0x0549: /* ARMENIAN CAPITAL LETTER CHA */
    case 0x054A: /* ARMENIAN CAPITAL LETTER PEH */
    case 0x054B: /* ARMENIAN CAPITAL LETTER JHEH */
    case 0x054C: /* ARMENIAN CAPITAL LETTER RA */
    case 0x054D: /* ARMENIAN CAPITAL LETTER SEH */
    case 0x054E: /* ARMENIAN CAPITAL LETTER VEW */
    case 0x054F: /* ARMENIAN CAPITAL LETTER TIWN */
    case 0x0550: /* ARMENIAN CAPITAL LETTER REH */
    case 0x0551: /* ARMENIAN CAPITAL LETTER CO */
    case 0x0552: /* ARMENIAN CAPITAL LETTER YIWN */
    case 0x0553: /* ARMENIAN CAPITAL LETTER PIWR */
    case 0x0554: /* ARMENIAN CAPITAL LETTER KEH */
    case 0x0555: /* ARMENIAN CAPITAL LETTER OH */
    case 0x0556: /* ARMENIAN CAPITAL LETTER FEH */
    case 0x10A0: /* GEORGIAN CAPITAL LETTER AN */
    case 0x10A1: /* GEORGIAN CAPITAL LETTER BAN */
    case 0x10A2: /* GEORGIAN CAPITAL LETTER GAN */
    case 0x10A3: /* GEORGIAN CAPITAL LETTER DON */
    case 0x10A4: /* GEORGIAN CAPITAL LETTER EN */
    case 0x10A5: /* GEORGIAN CAPITAL LETTER VIN */
    case 0x10A6: /* GEORGIAN CAPITAL LETTER ZEN */
    case 0x10A7: /* GEORGIAN CAPITAL LETTER TAN */
    case 0x10A8: /* GEORGIAN CAPITAL LETTER IN */
    case 0x10A9: /* GEORGIAN CAPITAL LETTER KAN */
    case 0x10AA: /* GEORGIAN CAPITAL LETTER LAS */
    case 0x10AB: /* GEORGIAN CAPITAL LETTER MAN */
    case 0x10AC: /* GEORGIAN CAPITAL LETTER NAR */
    case 0x10AD: /* GEORGIAN CAPITAL LETTER ON */
    case 0x10AE: /* GEORGIAN CAPITAL LETTER PAR */
    case 0x10AF: /* GEORGIAN CAPITAL LETTER ZHAR */
    case 0x10B0: /* GEORGIAN CAPITAL LETTER RAE */
    case 0x10B1: /* GEORGIAN CAPITAL LETTER SAN */
    case 0x10B2: /* GEORGIAN CAPITAL LETTER TAR */
    case 0x10B3: /* GEORGIAN CAPITAL LETTER UN */
    case 0x10B4: /* GEORGIAN CAPITAL LETTER PHAR */
    case 0x10B5: /* GEORGIAN CAPITAL LETTER KHAR */
    case 0x10B6: /* GEORGIAN CAPITAL LETTER GHAN */
    case 0x10B7: /* GEORGIAN CAPITAL LETTER QAR */
    case 0x10B8: /* GEORGIAN CAPITAL LETTER SHIN */
    case 0x10B9: /* GEORGIAN CAPITAL LETTER CHIN */
    case 0x10BA: /* GEORGIAN CAPITAL LETTER CAN */
    case 0x10BB: /* GEORGIAN CAPITAL LETTER JIL */
    case 0x10BC: /* GEORGIAN CAPITAL LETTER CIL */
    case 0x10BD: /* GEORGIAN CAPITAL LETTER CHAR */
    case 0x10BE: /* GEORGIAN CAPITAL LETTER XAN */
    case 0x10BF: /* GEORGIAN CAPITAL LETTER JHAN */
    case 0x10C0: /* GEORGIAN CAPITAL LETTER HAE */
    case 0x10C1: /* GEORGIAN CAPITAL LETTER HE */
    case 0x10C2: /* GEORGIAN CAPITAL LETTER HIE */
    case 0x10C3: /* GEORGIAN CAPITAL LETTER WE */
    case 0x10C4: /* GEORGIAN CAPITAL LETTER HAR */
    case 0x10C5: /* GEORGIAN CAPITAL LETTER HOE */
    case 0x1E00: /* LATIN CAPITAL LETTER A WITH RING BELOW */
    case 0x1E02: /* LATIN CAPITAL LETTER B WITH DOT ABOVE */
    case 0x1E04: /* LATIN CAPITAL LETTER B WITH DOT BELOW */
    case 0x1E06: /* LATIN CAPITAL LETTER B WITH LINE BELOW */
    case 0x1E08: /* LATIN CAPITAL LETTER C WITH CEDILLA AND ACUTE */
    case 0x1E0A: /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
    case 0x1E0C: /* LATIN CAPITAL LETTER D WITH DOT BELOW */
    case 0x1E0E: /* LATIN CAPITAL LETTER D WITH LINE BELOW */
    case 0x1E10: /* LATIN CAPITAL LETTER D WITH CEDILLA */
    case 0x1E12: /* LATIN CAPITAL LETTER D WITH CIRCUMFLEX BELOW */
    case 0x1E14: /* LATIN CAPITAL LETTER E WITH MACRON AND GRAVE */
    case 0x1E16: /* LATIN CAPITAL LETTER E WITH MACRON AND ACUTE */
    case 0x1E18: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX BELOW */
    case 0x1E1A: /* LATIN CAPITAL LETTER E WITH TILDE BELOW */
    case 0x1E1C: /* LATIN CAPITAL LETTER E WITH CEDILLA AND BREVE */
    case 0x1E1E: /* LATIN CAPITAL LETTER F WITH DOT ABOVE */
    case 0x1E20: /* LATIN CAPITAL LETTER G WITH MACRON */
    case 0x1E22: /* LATIN CAPITAL LETTER H WITH DOT ABOVE */
    case 0x1E24: /* LATIN CAPITAL LETTER H WITH DOT BELOW */
    case 0x1E26: /* LATIN CAPITAL LETTER H WITH DIAERESIS */
    case 0x1E28: /* LATIN CAPITAL LETTER H WITH CEDILLA */
    case 0x1E2A: /* LATIN CAPITAL LETTER H WITH BREVE BELOW */
    case 0x1E2C: /* LATIN CAPITAL LETTER I WITH TILDE BELOW */
    case 0x1E2E: /* LATIN CAPITAL LETTER I WITH DIAERESIS AND ACUTE */
    case 0x1E30: /* LATIN CAPITAL LETTER K WITH ACUTE */
    case 0x1E32: /* LATIN CAPITAL LETTER K WITH DOT BELOW */
    case 0x1E34: /* LATIN CAPITAL LETTER K WITH LINE BELOW */
    case 0x1E36: /* LATIN CAPITAL LETTER L WITH DOT BELOW */
    case 0x1E38: /* LATIN CAPITAL LETTER L WITH DOT BELOW AND MACRON */
    case 0x1E3A: /* LATIN CAPITAL LETTER L WITH LINE BELOW */
    case 0x1E3C: /* LATIN CAPITAL LETTER L WITH CIRCUMFLEX BELOW */
    case 0x1E3E: /* LATIN CAPITAL LETTER M WITH ACUTE */
    case 0x1E40: /* LATIN CAPITAL LETTER M WITH DOT ABOVE */
    case 0x1E42: /* LATIN CAPITAL LETTER M WITH DOT BELOW */
    case 0x1E44: /* LATIN CAPITAL LETTER N WITH DOT ABOVE */
    case 0x1E46: /* LATIN CAPITAL LETTER N WITH DOT BELOW */
    case 0x1E48: /* LATIN CAPITAL LETTER N WITH LINE BELOW */
    case 0x1E4A: /* LATIN CAPITAL LETTER N WITH CIRCUMFLEX BELOW */
    case 0x1E4C: /* LATIN CAPITAL LETTER O WITH TILDE AND ACUTE */
    case 0x1E4E: /* LATIN CAPITAL LETTER O WITH TILDE AND DIAERESIS */
    case 0x1E50: /* LATIN CAPITAL LETTER O WITH MACRON AND GRAVE */
    case 0x1E52: /* LATIN CAPITAL LETTER O WITH MACRON AND ACUTE */
    case 0x1E54: /* LATIN CAPITAL LETTER P WITH ACUTE */
    case 0x1E56: /* LATIN CAPITAL LETTER P WITH DOT ABOVE */
    case 0x1E58: /* LATIN CAPITAL LETTER R WITH DOT ABOVE */
    case 0x1E5A: /* LATIN CAPITAL LETTER R WITH DOT BELOW */
    case 0x1E5C: /* LATIN CAPITAL LETTER R WITH DOT BELOW AND MACRON */
    case 0x1E5E: /* LATIN CAPITAL LETTER R WITH LINE BELOW */
    case 0x1E60: /* LATIN CAPITAL LETTER S WITH DOT ABOVE */
    case 0x1E62: /* LATIN CAPITAL LETTER S WITH DOT BELOW */
    case 0x1E64: /* LATIN CAPITAL LETTER S WITH ACUTE AND DOT ABOVE */
    case 0x1E66: /* LATIN CAPITAL LETTER S WITH CARON AND DOT ABOVE */
    case 0x1E68: /* LATIN CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE */
    case 0x1E6A: /* LATIN CAPITAL LETTER T WITH DOT ABOVE */
    case 0x1E6C: /* LATIN CAPITAL LETTER T WITH DOT BELOW */
    case 0x1E6E: /* LATIN CAPITAL LETTER T WITH LINE BELOW */
    case 0x1E70: /* LATIN CAPITAL LETTER T WITH CIRCUMFLEX BELOW */
    case 0x1E72: /* LATIN CAPITAL LETTER U WITH DIAERESIS BELOW */
    case 0x1E74: /* LATIN CAPITAL LETTER U WITH TILDE BELOW */
    case 0x1E76: /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX BELOW */
    case 0x1E78: /* LATIN CAPITAL LETTER U WITH TILDE AND ACUTE */
    case 0x1E7A: /* LATIN CAPITAL LETTER U WITH MACRON AND DIAERESIS */
    case 0x1E7C: /* LATIN CAPITAL LETTER V WITH TILDE */
    case 0x1E7E: /* LATIN CAPITAL LETTER V WITH DOT BELOW */
    case 0x1E80: /* LATIN CAPITAL LETTER W WITH GRAVE */
    case 0x1E82: /* LATIN CAPITAL LETTER W WITH ACUTE */
    case 0x1E84: /* LATIN CAPITAL LETTER W WITH DIAERESIS */
    case 0x1E86: /* LATIN CAPITAL LETTER W WITH DOT ABOVE */
    case 0x1E88: /* LATIN CAPITAL LETTER W WITH DOT BELOW */
    case 0x1E8A: /* LATIN CAPITAL LETTER X WITH DOT ABOVE */
    case 0x1E8C: /* LATIN CAPITAL LETTER X WITH DIAERESIS */
    case 0x1E8E: /* LATIN CAPITAL LETTER Y WITH DOT ABOVE */
    case 0x1E90: /* LATIN CAPITAL LETTER Z WITH CIRCUMFLEX */
    case 0x1E92: /* LATIN CAPITAL LETTER Z WITH DOT BELOW */
    case 0x1E94: /* LATIN CAPITAL LETTER Z WITH LINE BELOW */
    case 0x1EA0: /* LATIN CAPITAL LETTER A WITH DOT BELOW */
    case 0x1EA2: /* LATIN CAPITAL LETTER A WITH HOOK ABOVE */
    case 0x1EA4: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE */
    case 0x1EA6: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE */
    case 0x1EA8: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1EAA: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE */
    case 0x1EAC: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EAE: /* LATIN CAPITAL LETTER A WITH BREVE AND ACUTE */
    case 0x1EB0: /* LATIN CAPITAL LETTER A WITH BREVE AND GRAVE */
    case 0x1EB2: /* LATIN CAPITAL LETTER A WITH BREVE AND HOOK ABOVE */
    case 0x1EB4: /* LATIN CAPITAL LETTER A WITH BREVE AND TILDE */
    case 0x1EB6: /* LATIN CAPITAL LETTER A WITH BREVE AND DOT BELOW */
    case 0x1EB8: /* LATIN CAPITAL LETTER E WITH DOT BELOW */
    case 0x1EBA: /* LATIN CAPITAL LETTER E WITH HOOK ABOVE */
    case 0x1EBC: /* LATIN CAPITAL LETTER E WITH TILDE */
    case 0x1EBE: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE */
    case 0x1EC0: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE */
    case 0x1EC2: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1EC4: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE */
    case 0x1EC6: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EC8: /* LATIN CAPITAL LETTER I WITH HOOK ABOVE */
    case 0x1ECA: /* LATIN CAPITAL LETTER I WITH DOT BELOW */
    case 0x1ECC: /* LATIN CAPITAL LETTER O WITH DOT BELOW */
    case 0x1ECE: /* LATIN CAPITAL LETTER O WITH HOOK ABOVE */
    case 0x1ED0: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE */
    case 0x1ED2: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE */
    case 0x1ED4: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE */
    case 0x1ED6: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE */
    case 0x1ED8: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW */
    case 0x1EDA: /* LATIN CAPITAL LETTER O WITH HORN AND ACUTE */
    case 0x1EDC: /* LATIN CAPITAL LETTER O WITH HORN AND GRAVE */
    case 0x1EDE: /* LATIN CAPITAL LETTER O WITH HORN AND HOOK ABOVE */
    case 0x1EE0: /* LATIN CAPITAL LETTER O WITH HORN AND TILDE */
    case 0x1EE2: /* LATIN CAPITAL LETTER O WITH HORN AND DOT BELOW */
    case 0x1EE4: /* LATIN CAPITAL LETTER U WITH DOT BELOW */
    case 0x1EE6: /* LATIN CAPITAL LETTER U WITH HOOK ABOVE */
    case 0x1EE8: /* LATIN CAPITAL LETTER U WITH HORN AND ACUTE */
    case 0x1EEA: /* LATIN CAPITAL LETTER U WITH HORN AND GRAVE */
    case 0x1EEC: /* LATIN CAPITAL LETTER U WITH HORN AND HOOK ABOVE */
    case 0x1EEE: /* LATIN CAPITAL LETTER U WITH HORN AND TILDE */
    case 0x1EF0: /* LATIN CAPITAL LETTER U WITH HORN AND DOT BELOW */
    case 0x1EF2: /* LATIN CAPITAL LETTER Y WITH GRAVE */
    case 0x1EF4: /* LATIN CAPITAL LETTER Y WITH DOT BELOW */
    case 0x1EF6: /* LATIN CAPITAL LETTER Y WITH HOOK ABOVE */
    case 0x1EF8: /* LATIN CAPITAL LETTER Y WITH TILDE */
    case 0x1F08: /* GREEK CAPITAL LETTER ALPHA WITH PSILI */
    case 0x1F09: /* GREEK CAPITAL LETTER ALPHA WITH DASIA */
    case 0x1F0A: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA */
    case 0x1F0B: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA */
    case 0x1F0C: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA */
    case 0x1F0D: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA */
    case 0x1F0E: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI */
    case 0x1F0F: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI */
    case 0x1F18: /* GREEK CAPITAL LETTER EPSILON WITH PSILI */
    case 0x1F19: /* GREEK CAPITAL LETTER EPSILON WITH DASIA */
    case 0x1F1A: /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND VARIA */
    case 0x1F1B: /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND VARIA */
    case 0x1F1C: /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND OXIA */
    case 0x1F1D: /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND OXIA */
    case 0x1F28: /* GREEK CAPITAL LETTER ETA WITH PSILI */
    case 0x1F29: /* GREEK CAPITAL LETTER ETA WITH DASIA */
    case 0x1F2A: /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA */
    case 0x1F2B: /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA */
    case 0x1F2C: /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA */
    case 0x1F2D: /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA */
    case 0x1F2E: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI */
    case 0x1F2F: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI */
    case 0x1F38: /* GREEK CAPITAL LETTER IOTA WITH PSILI */
    case 0x1F39: /* GREEK CAPITAL LETTER IOTA WITH DASIA */
    case 0x1F3A: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND VARIA */
    case 0x1F3B: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND VARIA */
    case 0x1F3C: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND OXIA */
    case 0x1F3D: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND OXIA */
    case 0x1F3E: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND PERISPOMENI */
    case 0x1F3F: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND PERISPOMENI */
    case 0x1F48: /* GREEK CAPITAL LETTER OMICRON WITH PSILI */
    case 0x1F49: /* GREEK CAPITAL LETTER OMICRON WITH DASIA */
    case 0x1F4A: /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND VARIA */
    case 0x1F4B: /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND VARIA */
    case 0x1F4C: /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND OXIA */
    case 0x1F4D: /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND OXIA */
    case 0x1F59: /* GREEK CAPITAL LETTER UPSILON WITH DASIA */
    case 0x1F5B: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND VARIA */
    case 0x1F5D: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND OXIA */
    case 0x1F5F: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND PERISPOMENI */
    case 0x1F68: /* GREEK CAPITAL LETTER OMEGA WITH PSILI */
    case 0x1F69: /* GREEK CAPITAL LETTER OMEGA WITH DASIA */
    case 0x1F6A: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA */
    case 0x1F6B: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA */
    case 0x1F6C: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA */
    case 0x1F6D: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA */
    case 0x1F6E: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI */
    case 0x1F6F: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI */
    case 0x1FB8: /* GREEK CAPITAL LETTER ALPHA WITH VRACHY */
    case 0x1FB9: /* GREEK CAPITAL LETTER ALPHA WITH MACRON */
    case 0x1FBA: /* GREEK CAPITAL LETTER ALPHA WITH VARIA */
    case 0x1FBB: /* GREEK CAPITAL LETTER ALPHA WITH OXIA */
    case 0x1FC8: /* GREEK CAPITAL LETTER EPSILON WITH VARIA */
    case 0x1FC9: /* GREEK CAPITAL LETTER EPSILON WITH OXIA */
    case 0x1FCA: /* GREEK CAPITAL LETTER ETA WITH VARIA */
    case 0x1FCB: /* GREEK CAPITAL LETTER ETA WITH OXIA */
    case 0x1FD8: /* GREEK CAPITAL LETTER IOTA WITH VRACHY */
    case 0x1FD9: /* GREEK CAPITAL LETTER IOTA WITH MACRON */
    case 0x1FDA: /* GREEK CAPITAL LETTER IOTA WITH VARIA */
    case 0x1FDB: /* GREEK CAPITAL LETTER IOTA WITH OXIA */
    case 0x1FE8: /* GREEK CAPITAL LETTER UPSILON WITH VRACHY */
    case 0x1FE9: /* GREEK CAPITAL LETTER UPSILON WITH MACRON */
    case 0x1FEA: /* GREEK CAPITAL LETTER UPSILON WITH VARIA */
    case 0x1FEB: /* GREEK CAPITAL LETTER UPSILON WITH OXIA */
    case 0x1FEC: /* GREEK CAPITAL LETTER RHO WITH DASIA */
    case 0x1FF8: /* GREEK CAPITAL LETTER OMICRON WITH VARIA */
    case 0x1FF9: /* GREEK CAPITAL LETTER OMICRON WITH OXIA */
    case 0x1FFA: /* GREEK CAPITAL LETTER OMEGA WITH VARIA */
    case 0x1FFB: /* GREEK CAPITAL LETTER OMEGA WITH OXIA */
    case 0x2102: /* DOUBLE-STRUCK CAPITAL C */
    case 0x2107: /* EULER CONSTANT */
    case 0x210B: /* SCRIPT CAPITAL H */
    case 0x210C: /* BLACK-LETTER CAPITAL H */
    case 0x210D: /* DOUBLE-STRUCK CAPITAL H */
    case 0x2110: /* SCRIPT CAPITAL I */
    case 0x2111: /* BLACK-LETTER CAPITAL I */
    case 0x2112: /* SCRIPT CAPITAL L */
    case 0x2115: /* DOUBLE-STRUCK CAPITAL N */
    case 0x2119: /* DOUBLE-STRUCK CAPITAL P */
    case 0x211A: /* DOUBLE-STRUCK CAPITAL Q */
    case 0x211B: /* SCRIPT CAPITAL R */
    case 0x211C: /* BLACK-LETTER CAPITAL R */
    case 0x211D: /* DOUBLE-STRUCK CAPITAL R */
    case 0x2124: /* DOUBLE-STRUCK CAPITAL Z */
    case 0x2126: /* OHM SIGN */
    case 0x2128: /* BLACK-LETTER CAPITAL Z */
    case 0x212A: /* KELVIN SIGN */
    case 0x212B: /* ANGSTROM SIGN */
    case 0x212C: /* SCRIPT CAPITAL B */
    case 0x212D: /* BLACK-LETTER CAPITAL C */
    case 0x2130: /* SCRIPT CAPITAL E */
    case 0x2131: /* SCRIPT CAPITAL F */
    case 0x2133: /* SCRIPT CAPITAL M */
    case 0xFF21: /* FULLWIDTH LATIN CAPITAL LETTER A */
    case 0xFF22: /* FULLWIDTH LATIN CAPITAL LETTER B */
    case 0xFF23: /* FULLWIDTH LATIN CAPITAL LETTER C */
    case 0xFF24: /* FULLWIDTH LATIN CAPITAL LETTER D */
    case 0xFF25: /* FULLWIDTH LATIN CAPITAL LETTER E */
    case 0xFF26: /* FULLWIDTH LATIN CAPITAL LETTER F */
    case 0xFF27: /* FULLWIDTH LATIN CAPITAL LETTER G */
    case 0xFF28: /* FULLWIDTH LATIN CAPITAL LETTER H */
    case 0xFF29: /* FULLWIDTH LATIN CAPITAL LETTER I */
    case 0xFF2A: /* FULLWIDTH LATIN CAPITAL LETTER J */
    case 0xFF2B: /* FULLWIDTH LATIN CAPITAL LETTER K */
    case 0xFF2C: /* FULLWIDTH LATIN CAPITAL LETTER L */
    case 0xFF2D: /* FULLWIDTH LATIN CAPITAL LETTER M */
    case 0xFF2E: /* FULLWIDTH LATIN CAPITAL LETTER N */
    case 0xFF2F: /* FULLWIDTH LATIN CAPITAL LETTER O */
    case 0xFF30: /* FULLWIDTH LATIN CAPITAL LETTER P */
    case 0xFF31: /* FULLWIDTH LATIN CAPITAL LETTER Q */
    case 0xFF32: /* FULLWIDTH LATIN CAPITAL LETTER R */
    case 0xFF33: /* FULLWIDTH LATIN CAPITAL LETTER S */
    case 0xFF34: /* FULLWIDTH LATIN CAPITAL LETTER T */
    case 0xFF35: /* FULLWIDTH LATIN CAPITAL LETTER U */
    case 0xFF36: /* FULLWIDTH LATIN CAPITAL LETTER V */
    case 0xFF37: /* FULLWIDTH LATIN CAPITAL LETTER W */
    case 0xFF38: /* FULLWIDTH LATIN CAPITAL LETTER X */
    case 0xFF39: /* FULLWIDTH LATIN CAPITAL LETTER Y */
    case 0xFF3A: /* FULLWIDTH LATIN CAPITAL LETTER Z */
	return 1;
    default:
	return 0;
    }
}

/* Returns the uppercase Unicode characters corresponding to ch or just
   ch if no uppercase mapping is known. */

Py_UNICODE _PyUnicode_ToUppercase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0061: /* LATIN SMALL LETTER A */
	return (Py_UNICODE)0x0041;
    case 0x0062: /* LATIN SMALL LETTER B */
	return (Py_UNICODE)0x0042;
    case 0x0063: /* LATIN SMALL LETTER C */
	return (Py_UNICODE)0x0043;
    case 0x0064: /* LATIN SMALL LETTER D */
	return (Py_UNICODE)0x0044;
    case 0x0065: /* LATIN SMALL LETTER E */
	return (Py_UNICODE)0x0045;
    case 0x0066: /* LATIN SMALL LETTER F */
	return (Py_UNICODE)0x0046;
    case 0x0067: /* LATIN SMALL LETTER G */
	return (Py_UNICODE)0x0047;
    case 0x0068: /* LATIN SMALL LETTER H */
	return (Py_UNICODE)0x0048;
    case 0x0069: /* LATIN SMALL LETTER I */
	return (Py_UNICODE)0x0049;
    case 0x006A: /* LATIN SMALL LETTER J */
	return (Py_UNICODE)0x004A;
    case 0x006B: /* LATIN SMALL LETTER K */
	return (Py_UNICODE)0x004B;
    case 0x006C: /* LATIN SMALL LETTER L */
	return (Py_UNICODE)0x004C;
    case 0x006D: /* LATIN SMALL LETTER M */
	return (Py_UNICODE)0x004D;
    case 0x006E: /* LATIN SMALL LETTER N */
	return (Py_UNICODE)0x004E;
    case 0x006F: /* LATIN SMALL LETTER O */
	return (Py_UNICODE)0x004F;
    case 0x0070: /* LATIN SMALL LETTER P */
	return (Py_UNICODE)0x0050;
    case 0x0071: /* LATIN SMALL LETTER Q */
	return (Py_UNICODE)0x0051;
    case 0x0072: /* LATIN SMALL LETTER R */
	return (Py_UNICODE)0x0052;
    case 0x0073: /* LATIN SMALL LETTER S */
	return (Py_UNICODE)0x0053;
    case 0x0074: /* LATIN SMALL LETTER T */
	return (Py_UNICODE)0x0054;
    case 0x0075: /* LATIN SMALL LETTER U */
	return (Py_UNICODE)0x0055;
    case 0x0076: /* LATIN SMALL LETTER V */
	return (Py_UNICODE)0x0056;
    case 0x0077: /* LATIN SMALL LETTER W */
	return (Py_UNICODE)0x0057;
    case 0x0078: /* LATIN SMALL LETTER X */
	return (Py_UNICODE)0x0058;
    case 0x0079: /* LATIN SMALL LETTER Y */
	return (Py_UNICODE)0x0059;
    case 0x007A: /* LATIN SMALL LETTER Z */
	return (Py_UNICODE)0x005A;
    case 0x00B5: /* MICRO SIGN */
	return (Py_UNICODE)0x039C;
    case 0x00E0: /* LATIN SMALL LETTER A WITH GRAVE */
	return (Py_UNICODE)0x00C0;
    case 0x00E1: /* LATIN SMALL LETTER A WITH ACUTE */
	return (Py_UNICODE)0x00C1;
    case 0x00E2: /* LATIN SMALL LETTER A WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00C2;
    case 0x00E3: /* LATIN SMALL LETTER A WITH TILDE */
	return (Py_UNICODE)0x00C3;
    case 0x00E4: /* LATIN SMALL LETTER A WITH DIAERESIS */
	return (Py_UNICODE)0x00C4;
    case 0x00E5: /* LATIN SMALL LETTER A WITH RING ABOVE */
	return (Py_UNICODE)0x00C5;
    case 0x00E6: /* LATIN SMALL LETTER AE */
	return (Py_UNICODE)0x00C6;
    case 0x00E7: /* LATIN SMALL LETTER C WITH CEDILLA */
	return (Py_UNICODE)0x00C7;
    case 0x00E8: /* LATIN SMALL LETTER E WITH GRAVE */
	return (Py_UNICODE)0x00C8;
    case 0x00E9: /* LATIN SMALL LETTER E WITH ACUTE */
	return (Py_UNICODE)0x00C9;
    case 0x00EA: /* LATIN SMALL LETTER E WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00CA;
    case 0x00EB: /* LATIN SMALL LETTER E WITH DIAERESIS */
	return (Py_UNICODE)0x00CB;
    case 0x00EC: /* LATIN SMALL LETTER I WITH GRAVE */
	return (Py_UNICODE)0x00CC;
    case 0x00ED: /* LATIN SMALL LETTER I WITH ACUTE */
	return (Py_UNICODE)0x00CD;
    case 0x00EE: /* LATIN SMALL LETTER I WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00CE;
    case 0x00EF: /* LATIN SMALL LETTER I WITH DIAERESIS */
	return (Py_UNICODE)0x00CF;
    case 0x00F0: /* LATIN SMALL LETTER ETH */
	return (Py_UNICODE)0x00D0;
    case 0x00F1: /* LATIN SMALL LETTER N WITH TILDE */
	return (Py_UNICODE)0x00D1;
    case 0x00F2: /* LATIN SMALL LETTER O WITH GRAVE */
	return (Py_UNICODE)0x00D2;
    case 0x00F3: /* LATIN SMALL LETTER O WITH ACUTE */
	return (Py_UNICODE)0x00D3;
    case 0x00F4: /* LATIN SMALL LETTER O WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00D4;
    case 0x00F5: /* LATIN SMALL LETTER O WITH TILDE */
	return (Py_UNICODE)0x00D5;
    case 0x00F6: /* LATIN SMALL LETTER O WITH DIAERESIS */
	return (Py_UNICODE)0x00D6;
    case 0x00F8: /* LATIN SMALL LETTER O WITH STROKE */
	return (Py_UNICODE)0x00D8;
    case 0x00F9: /* LATIN SMALL LETTER U WITH GRAVE */
	return (Py_UNICODE)0x00D9;
    case 0x00FA: /* LATIN SMALL LETTER U WITH ACUTE */
	return (Py_UNICODE)0x00DA;
    case 0x00FB: /* LATIN SMALL LETTER U WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00DB;
    case 0x00FC: /* LATIN SMALL LETTER U WITH DIAERESIS */
	return (Py_UNICODE)0x00DC;
    case 0x00FD: /* LATIN SMALL LETTER Y WITH ACUTE */
	return (Py_UNICODE)0x00DD;
    case 0x00FE: /* LATIN SMALL LETTER THORN */
	return (Py_UNICODE)0x00DE;
    case 0x00FF: /* LATIN SMALL LETTER Y WITH DIAERESIS */
	return (Py_UNICODE)0x0178;
    case 0x0101: /* LATIN SMALL LETTER A WITH MACRON */
	return (Py_UNICODE)0x0100;
    case 0x0103: /* LATIN SMALL LETTER A WITH BREVE */
	return (Py_UNICODE)0x0102;
    case 0x0105: /* LATIN SMALL LETTER A WITH OGONEK */
	return (Py_UNICODE)0x0104;
    case 0x0107: /* LATIN SMALL LETTER C WITH ACUTE */
	return (Py_UNICODE)0x0106;
    case 0x0109: /* LATIN SMALL LETTER C WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0108;
    case 0x010B: /* LATIN SMALL LETTER C WITH DOT ABOVE */
	return (Py_UNICODE)0x010A;
    case 0x010D: /* LATIN SMALL LETTER C WITH CARON */
	return (Py_UNICODE)0x010C;
    case 0x010F: /* LATIN SMALL LETTER D WITH CARON */
	return (Py_UNICODE)0x010E;
    case 0x0111: /* LATIN SMALL LETTER D WITH STROKE */
	return (Py_UNICODE)0x0110;
    case 0x0113: /* LATIN SMALL LETTER E WITH MACRON */
	return (Py_UNICODE)0x0112;
    case 0x0115: /* LATIN SMALL LETTER E WITH BREVE */
	return (Py_UNICODE)0x0114;
    case 0x0117: /* LATIN SMALL LETTER E WITH DOT ABOVE */
	return (Py_UNICODE)0x0116;
    case 0x0119: /* LATIN SMALL LETTER E WITH OGONEK */
	return (Py_UNICODE)0x0118;
    case 0x011B: /* LATIN SMALL LETTER E WITH CARON */
	return (Py_UNICODE)0x011A;
    case 0x011D: /* LATIN SMALL LETTER G WITH CIRCUMFLEX */
	return (Py_UNICODE)0x011C;
    case 0x011F: /* LATIN SMALL LETTER G WITH BREVE */
	return (Py_UNICODE)0x011E;
    case 0x0121: /* LATIN SMALL LETTER G WITH DOT ABOVE */
	return (Py_UNICODE)0x0120;
    case 0x0123: /* LATIN SMALL LETTER G WITH CEDILLA */
	return (Py_UNICODE)0x0122;
    case 0x0125: /* LATIN SMALL LETTER H WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0124;
    case 0x0127: /* LATIN SMALL LETTER H WITH STROKE */
	return (Py_UNICODE)0x0126;
    case 0x0129: /* LATIN SMALL LETTER I WITH TILDE */
	return (Py_UNICODE)0x0128;
    case 0x012B: /* LATIN SMALL LETTER I WITH MACRON */
	return (Py_UNICODE)0x012A;
    case 0x012D: /* LATIN SMALL LETTER I WITH BREVE */
	return (Py_UNICODE)0x012C;
    case 0x012F: /* LATIN SMALL LETTER I WITH OGONEK */
	return (Py_UNICODE)0x012E;
    case 0x0131: /* LATIN SMALL LETTER DOTLESS I */
	return (Py_UNICODE)0x0049;
    case 0x0133: /* LATIN SMALL LIGATURE IJ */
	return (Py_UNICODE)0x0132;
    case 0x0135: /* LATIN SMALL LETTER J WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0134;
    case 0x0137: /* LATIN SMALL LETTER K WITH CEDILLA */
	return (Py_UNICODE)0x0136;
    case 0x013A: /* LATIN SMALL LETTER L WITH ACUTE */
	return (Py_UNICODE)0x0139;
    case 0x013C: /* LATIN SMALL LETTER L WITH CEDILLA */
	return (Py_UNICODE)0x013B;
    case 0x013E: /* LATIN SMALL LETTER L WITH CARON */
	return (Py_UNICODE)0x013D;
    case 0x0140: /* LATIN SMALL LETTER L WITH MIDDLE DOT */
	return (Py_UNICODE)0x013F;
    case 0x0142: /* LATIN SMALL LETTER L WITH STROKE */
	return (Py_UNICODE)0x0141;
    case 0x0144: /* LATIN SMALL LETTER N WITH ACUTE */
	return (Py_UNICODE)0x0143;
    case 0x0146: /* LATIN SMALL LETTER N WITH CEDILLA */
	return (Py_UNICODE)0x0145;
    case 0x0148: /* LATIN SMALL LETTER N WITH CARON */
	return (Py_UNICODE)0x0147;
    case 0x014B: /* LATIN SMALL LETTER ENG */
	return (Py_UNICODE)0x014A;
    case 0x014D: /* LATIN SMALL LETTER O WITH MACRON */
	return (Py_UNICODE)0x014C;
    case 0x014F: /* LATIN SMALL LETTER O WITH BREVE */
	return (Py_UNICODE)0x014E;
    case 0x0151: /* LATIN SMALL LETTER O WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x0150;
    case 0x0153: /* LATIN SMALL LIGATURE OE */
	return (Py_UNICODE)0x0152;
    case 0x0155: /* LATIN SMALL LETTER R WITH ACUTE */
	return (Py_UNICODE)0x0154;
    case 0x0157: /* LATIN SMALL LETTER R WITH CEDILLA */
	return (Py_UNICODE)0x0156;
    case 0x0159: /* LATIN SMALL LETTER R WITH CARON */
	return (Py_UNICODE)0x0158;
    case 0x015B: /* LATIN SMALL LETTER S WITH ACUTE */
	return (Py_UNICODE)0x015A;
    case 0x015D: /* LATIN SMALL LETTER S WITH CIRCUMFLEX */
	return (Py_UNICODE)0x015C;
    case 0x015F: /* LATIN SMALL LETTER S WITH CEDILLA */
	return (Py_UNICODE)0x015E;
    case 0x0161: /* LATIN SMALL LETTER S WITH CARON */
	return (Py_UNICODE)0x0160;
    case 0x0163: /* LATIN SMALL LETTER T WITH CEDILLA */
	return (Py_UNICODE)0x0162;
    case 0x0165: /* LATIN SMALL LETTER T WITH CARON */
	return (Py_UNICODE)0x0164;
    case 0x0167: /* LATIN SMALL LETTER T WITH STROKE */
	return (Py_UNICODE)0x0166;
    case 0x0169: /* LATIN SMALL LETTER U WITH TILDE */
	return (Py_UNICODE)0x0168;
    case 0x016B: /* LATIN SMALL LETTER U WITH MACRON */
	return (Py_UNICODE)0x016A;
    case 0x016D: /* LATIN SMALL LETTER U WITH BREVE */
	return (Py_UNICODE)0x016C;
    case 0x016F: /* LATIN SMALL LETTER U WITH RING ABOVE */
	return (Py_UNICODE)0x016E;
    case 0x0171: /* LATIN SMALL LETTER U WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x0170;
    case 0x0173: /* LATIN SMALL LETTER U WITH OGONEK */
	return (Py_UNICODE)0x0172;
    case 0x0175: /* LATIN SMALL LETTER W WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0174;
    case 0x0177: /* LATIN SMALL LETTER Y WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0176;
    case 0x017A: /* LATIN SMALL LETTER Z WITH ACUTE */
	return (Py_UNICODE)0x0179;
    case 0x017C: /* LATIN SMALL LETTER Z WITH DOT ABOVE */
	return (Py_UNICODE)0x017B;
    case 0x017E: /* LATIN SMALL LETTER Z WITH CARON */
	return (Py_UNICODE)0x017D;
    case 0x017F: /* LATIN SMALL LETTER LONG S */
	return (Py_UNICODE)0x0053;
    case 0x0183: /* LATIN SMALL LETTER B WITH TOPBAR */
	return (Py_UNICODE)0x0182;
    case 0x0185: /* LATIN SMALL LETTER TONE SIX */
	return (Py_UNICODE)0x0184;
    case 0x0188: /* LATIN SMALL LETTER C WITH HOOK */
	return (Py_UNICODE)0x0187;
    case 0x018C: /* LATIN SMALL LETTER D WITH TOPBAR */
	return (Py_UNICODE)0x018B;
    case 0x0192: /* LATIN SMALL LETTER F WITH HOOK */
	return (Py_UNICODE)0x0191;
    case 0x0195: /* LATIN SMALL LETTER HV */
	return (Py_UNICODE)0x01F6;
    case 0x0199: /* LATIN SMALL LETTER K WITH HOOK */
	return (Py_UNICODE)0x0198;
    case 0x01A1: /* LATIN SMALL LETTER O WITH HORN */
	return (Py_UNICODE)0x01A0;
    case 0x01A3: /* LATIN SMALL LETTER OI */
	return (Py_UNICODE)0x01A2;
    case 0x01A5: /* LATIN SMALL LETTER P WITH HOOK */
	return (Py_UNICODE)0x01A4;
    case 0x01A8: /* LATIN SMALL LETTER TONE TWO */
	return (Py_UNICODE)0x01A7;
    case 0x01AD: /* LATIN SMALL LETTER T WITH HOOK */
	return (Py_UNICODE)0x01AC;
    case 0x01B0: /* LATIN SMALL LETTER U WITH HORN */
	return (Py_UNICODE)0x01AF;
    case 0x01B4: /* LATIN SMALL LETTER Y WITH HOOK */
	return (Py_UNICODE)0x01B3;
    case 0x01B6: /* LATIN SMALL LETTER Z WITH STROKE */
	return (Py_UNICODE)0x01B5;
    case 0x01B9: /* LATIN SMALL LETTER EZH REVERSED */
	return (Py_UNICODE)0x01B8;
    case 0x01BD: /* LATIN SMALL LETTER TONE FIVE */
	return (Py_UNICODE)0x01BC;
    case 0x01BF: /* LATIN LETTER WYNN */
	return (Py_UNICODE)0x01F7;
    case 0x01C5: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON */
	return (Py_UNICODE)0x01C4;
    case 0x01C6: /* LATIN SMALL LETTER DZ WITH CARON */
	return (Py_UNICODE)0x01C4;
    case 0x01C8: /* LATIN CAPITAL LETTER L WITH SMALL LETTER J */
	return (Py_UNICODE)0x01C7;
    case 0x01C9: /* LATIN SMALL LETTER LJ */
	return (Py_UNICODE)0x01C7;
    case 0x01CB: /* LATIN CAPITAL LETTER N WITH SMALL LETTER J */
	return (Py_UNICODE)0x01CA;
    case 0x01CC: /* LATIN SMALL LETTER NJ */
	return (Py_UNICODE)0x01CA;
    case 0x01CE: /* LATIN SMALL LETTER A WITH CARON */
	return (Py_UNICODE)0x01CD;
    case 0x01D0: /* LATIN SMALL LETTER I WITH CARON */
	return (Py_UNICODE)0x01CF;
    case 0x01D2: /* LATIN SMALL LETTER O WITH CARON */
	return (Py_UNICODE)0x01D1;
    case 0x01D4: /* LATIN SMALL LETTER U WITH CARON */
	return (Py_UNICODE)0x01D3;
    case 0x01D6: /* LATIN SMALL LETTER U WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x01D5;
    case 0x01D8: /* LATIN SMALL LETTER U WITH DIAERESIS AND ACUTE */
	return (Py_UNICODE)0x01D7;
    case 0x01DA: /* LATIN SMALL LETTER U WITH DIAERESIS AND CARON */
	return (Py_UNICODE)0x01D9;
    case 0x01DC: /* LATIN SMALL LETTER U WITH DIAERESIS AND GRAVE */
	return (Py_UNICODE)0x01DB;
    case 0x01DD: /* LATIN SMALL LETTER TURNED E */
	return (Py_UNICODE)0x018E;
    case 0x01DF: /* LATIN SMALL LETTER A WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x01DE;
    case 0x01E1: /* LATIN SMALL LETTER A WITH DOT ABOVE AND MACRON */
	return (Py_UNICODE)0x01E0;
    case 0x01E3: /* LATIN SMALL LETTER AE WITH MACRON */
	return (Py_UNICODE)0x01E2;
    case 0x01E5: /* LATIN SMALL LETTER G WITH STROKE */
	return (Py_UNICODE)0x01E4;
    case 0x01E7: /* LATIN SMALL LETTER G WITH CARON */
	return (Py_UNICODE)0x01E6;
    case 0x01E9: /* LATIN SMALL LETTER K WITH CARON */
	return (Py_UNICODE)0x01E8;
    case 0x01EB: /* LATIN SMALL LETTER O WITH OGONEK */
	return (Py_UNICODE)0x01EA;
    case 0x01ED: /* LATIN SMALL LETTER O WITH OGONEK AND MACRON */
	return (Py_UNICODE)0x01EC;
    case 0x01EF: /* LATIN SMALL LETTER EZH WITH CARON */
	return (Py_UNICODE)0x01EE;
    case 0x01F2: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z */
	return (Py_UNICODE)0x01F1;
    case 0x01F3: /* LATIN SMALL LETTER DZ */
	return (Py_UNICODE)0x01F1;
    case 0x01F5: /* LATIN SMALL LETTER G WITH ACUTE */
	return (Py_UNICODE)0x01F4;
    case 0x01F9: /* LATIN SMALL LETTER N WITH GRAVE */
	return (Py_UNICODE)0x01F8;
    case 0x01FB: /* LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE */
	return (Py_UNICODE)0x01FA;
    case 0x01FD: /* LATIN SMALL LETTER AE WITH ACUTE */
	return (Py_UNICODE)0x01FC;
    case 0x01FF: /* LATIN SMALL LETTER O WITH STROKE AND ACUTE */
	return (Py_UNICODE)0x01FE;
    case 0x0201: /* LATIN SMALL LETTER A WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0200;
    case 0x0203: /* LATIN SMALL LETTER A WITH INVERTED BREVE */
	return (Py_UNICODE)0x0202;
    case 0x0205: /* LATIN SMALL LETTER E WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0204;
    case 0x0207: /* LATIN SMALL LETTER E WITH INVERTED BREVE */
	return (Py_UNICODE)0x0206;
    case 0x0209: /* LATIN SMALL LETTER I WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0208;
    case 0x020B: /* LATIN SMALL LETTER I WITH INVERTED BREVE */
	return (Py_UNICODE)0x020A;
    case 0x020D: /* LATIN SMALL LETTER O WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x020C;
    case 0x020F: /* LATIN SMALL LETTER O WITH INVERTED BREVE */
	return (Py_UNICODE)0x020E;
    case 0x0211: /* LATIN SMALL LETTER R WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0210;
    case 0x0213: /* LATIN SMALL LETTER R WITH INVERTED BREVE */
	return (Py_UNICODE)0x0212;
    case 0x0215: /* LATIN SMALL LETTER U WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0214;
    case 0x0217: /* LATIN SMALL LETTER U WITH INVERTED BREVE */
	return (Py_UNICODE)0x0216;
    case 0x0219: /* LATIN SMALL LETTER S WITH COMMA BELOW */
	return (Py_UNICODE)0x0218;
    case 0x021B: /* LATIN SMALL LETTER T WITH COMMA BELOW */
	return (Py_UNICODE)0x021A;
    case 0x021D: /* LATIN SMALL LETTER YOGH */
	return (Py_UNICODE)0x021C;
    case 0x021F: /* LATIN SMALL LETTER H WITH CARON */
	return (Py_UNICODE)0x021E;
    case 0x0223: /* LATIN SMALL LETTER OU */
	return (Py_UNICODE)0x0222;
    case 0x0225: /* LATIN SMALL LETTER Z WITH HOOK */
	return (Py_UNICODE)0x0224;
    case 0x0227: /* LATIN SMALL LETTER A WITH DOT ABOVE */
	return (Py_UNICODE)0x0226;
    case 0x0229: /* LATIN SMALL LETTER E WITH CEDILLA */
	return (Py_UNICODE)0x0228;
    case 0x022B: /* LATIN SMALL LETTER O WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x022A;
    case 0x022D: /* LATIN SMALL LETTER O WITH TILDE AND MACRON */
	return (Py_UNICODE)0x022C;
    case 0x022F: /* LATIN SMALL LETTER O WITH DOT ABOVE */
	return (Py_UNICODE)0x022E;
    case 0x0231: /* LATIN SMALL LETTER O WITH DOT ABOVE AND MACRON */
	return (Py_UNICODE)0x0230;
    case 0x0233: /* LATIN SMALL LETTER Y WITH MACRON */
	return (Py_UNICODE)0x0232;
    case 0x0253: /* LATIN SMALL LETTER B WITH HOOK */
	return (Py_UNICODE)0x0181;
    case 0x0254: /* LATIN SMALL LETTER OPEN O */
	return (Py_UNICODE)0x0186;
    case 0x0256: /* LATIN SMALL LETTER D WITH TAIL */
	return (Py_UNICODE)0x0189;
    case 0x0257: /* LATIN SMALL LETTER D WITH HOOK */
	return (Py_UNICODE)0x018A;
    case 0x0259: /* LATIN SMALL LETTER SCHWA */
	return (Py_UNICODE)0x018F;
    case 0x025B: /* LATIN SMALL LETTER OPEN E */
	return (Py_UNICODE)0x0190;
    case 0x0260: /* LATIN SMALL LETTER G WITH HOOK */
	return (Py_UNICODE)0x0193;
    case 0x0263: /* LATIN SMALL LETTER GAMMA */
	return (Py_UNICODE)0x0194;
    case 0x0268: /* LATIN SMALL LETTER I WITH STROKE */
	return (Py_UNICODE)0x0197;
    case 0x0269: /* LATIN SMALL LETTER IOTA */
	return (Py_UNICODE)0x0196;
    case 0x026F: /* LATIN SMALL LETTER TURNED M */
	return (Py_UNICODE)0x019C;
    case 0x0272: /* LATIN SMALL LETTER N WITH LEFT HOOK */
	return (Py_UNICODE)0x019D;
    case 0x0275: /* LATIN SMALL LETTER BARRED O */
	return (Py_UNICODE)0x019F;
    case 0x0280: /* LATIN LETTER SMALL CAPITAL R */
	return (Py_UNICODE)0x01A6;
    case 0x0283: /* LATIN SMALL LETTER ESH */
	return (Py_UNICODE)0x01A9;
    case 0x0288: /* LATIN SMALL LETTER T WITH RETROFLEX HOOK */
	return (Py_UNICODE)0x01AE;
    case 0x028A: /* LATIN SMALL LETTER UPSILON */
	return (Py_UNICODE)0x01B1;
    case 0x028B: /* LATIN SMALL LETTER V WITH HOOK */
	return (Py_UNICODE)0x01B2;
    case 0x0292: /* LATIN SMALL LETTER EZH */
	return (Py_UNICODE)0x01B7;
    case 0x0345: /* COMBINING GREEK YPOGEGRAMMENI */
	return (Py_UNICODE)0x0399;
    case 0x03AC: /* GREEK SMALL LETTER ALPHA WITH TONOS */
	return (Py_UNICODE)0x0386;
    case 0x03AD: /* GREEK SMALL LETTER EPSILON WITH TONOS */
	return (Py_UNICODE)0x0388;
    case 0x03AE: /* GREEK SMALL LETTER ETA WITH TONOS */
	return (Py_UNICODE)0x0389;
    case 0x03AF: /* GREEK SMALL LETTER IOTA WITH TONOS */
	return (Py_UNICODE)0x038A;
    case 0x03B1: /* GREEK SMALL LETTER ALPHA */
	return (Py_UNICODE)0x0391;
    case 0x03B2: /* GREEK SMALL LETTER BETA */
	return (Py_UNICODE)0x0392;
    case 0x03B3: /* GREEK SMALL LETTER GAMMA */
	return (Py_UNICODE)0x0393;
    case 0x03B4: /* GREEK SMALL LETTER DELTA */
	return (Py_UNICODE)0x0394;
    case 0x03B5: /* GREEK SMALL LETTER EPSILON */
	return (Py_UNICODE)0x0395;
    case 0x03B6: /* GREEK SMALL LETTER ZETA */
	return (Py_UNICODE)0x0396;
    case 0x03B7: /* GREEK SMALL LETTER ETA */
	return (Py_UNICODE)0x0397;
    case 0x03B8: /* GREEK SMALL LETTER THETA */
	return (Py_UNICODE)0x0398;
    case 0x03B9: /* GREEK SMALL LETTER IOTA */
	return (Py_UNICODE)0x0399;
    case 0x03BA: /* GREEK SMALL LETTER KAPPA */
	return (Py_UNICODE)0x039A;
    case 0x03BB: /* GREEK SMALL LETTER LAMDA */
	return (Py_UNICODE)0x039B;
    case 0x03BC: /* GREEK SMALL LETTER MU */
	return (Py_UNICODE)0x039C;
    case 0x03BD: /* GREEK SMALL LETTER NU */
	return (Py_UNICODE)0x039D;
    case 0x03BE: /* GREEK SMALL LETTER XI */
	return (Py_UNICODE)0x039E;
    case 0x03BF: /* GREEK SMALL LETTER OMICRON */
	return (Py_UNICODE)0x039F;
    case 0x03C0: /* GREEK SMALL LETTER PI */
	return (Py_UNICODE)0x03A0;
    case 0x03C1: /* GREEK SMALL LETTER RHO */
	return (Py_UNICODE)0x03A1;
    case 0x03C2: /* GREEK SMALL LETTER FINAL SIGMA */
	return (Py_UNICODE)0x03A3;
    case 0x03C3: /* GREEK SMALL LETTER SIGMA */
	return (Py_UNICODE)0x03A3;
    case 0x03C4: /* GREEK SMALL LETTER TAU */
	return (Py_UNICODE)0x03A4;
    case 0x03C5: /* GREEK SMALL LETTER UPSILON */
	return (Py_UNICODE)0x03A5;
    case 0x03C6: /* GREEK SMALL LETTER PHI */
	return (Py_UNICODE)0x03A6;
    case 0x03C7: /* GREEK SMALL LETTER CHI */
	return (Py_UNICODE)0x03A7;
    case 0x03C8: /* GREEK SMALL LETTER PSI */
	return (Py_UNICODE)0x03A8;
    case 0x03C9: /* GREEK SMALL LETTER OMEGA */
	return (Py_UNICODE)0x03A9;
    case 0x03CA: /* GREEK SMALL LETTER IOTA WITH DIALYTIKA */
	return (Py_UNICODE)0x03AA;
    case 0x03CB: /* GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
	return (Py_UNICODE)0x03AB;
    case 0x03CC: /* GREEK SMALL LETTER OMICRON WITH TONOS */
	return (Py_UNICODE)0x038C;
    case 0x03CD: /* GREEK SMALL LETTER UPSILON WITH TONOS */
	return (Py_UNICODE)0x038E;
    case 0x03CE: /* GREEK SMALL LETTER OMEGA WITH TONOS */
	return (Py_UNICODE)0x038F;
    case 0x03D0: /* GREEK BETA SYMBOL */
	return (Py_UNICODE)0x0392;
    case 0x03D1: /* GREEK THETA SYMBOL */
	return (Py_UNICODE)0x0398;
    case 0x03D5: /* GREEK PHI SYMBOL */
	return (Py_UNICODE)0x03A6;
    case 0x03D6: /* GREEK PI SYMBOL */
	return (Py_UNICODE)0x03A0;
    case 0x03DB: /* GREEK SMALL LETTER STIGMA */
	return (Py_UNICODE)0x03DA;
    case 0x03DD: /* GREEK SMALL LETTER DIGAMMA */
	return (Py_UNICODE)0x03DC;
    case 0x03DF: /* GREEK SMALL LETTER KOPPA */
	return (Py_UNICODE)0x03DE;
    case 0x03E1: /* GREEK SMALL LETTER SAMPI */
	return (Py_UNICODE)0x03E0;
    case 0x03E3: /* COPTIC SMALL LETTER SHEI */
	return (Py_UNICODE)0x03E2;
    case 0x03E5: /* COPTIC SMALL LETTER FEI */
	return (Py_UNICODE)0x03E4;
    case 0x03E7: /* COPTIC SMALL LETTER KHEI */
	return (Py_UNICODE)0x03E6;
    case 0x03E9: /* COPTIC SMALL LETTER HORI */
	return (Py_UNICODE)0x03E8;
    case 0x03EB: /* COPTIC SMALL LETTER GANGIA */
	return (Py_UNICODE)0x03EA;
    case 0x03ED: /* COPTIC SMALL LETTER SHIMA */
	return (Py_UNICODE)0x03EC;
    case 0x03EF: /* COPTIC SMALL LETTER DEI */
	return (Py_UNICODE)0x03EE;
    case 0x03F0: /* GREEK KAPPA SYMBOL */
	return (Py_UNICODE)0x039A;
    case 0x03F1: /* GREEK RHO SYMBOL */
	return (Py_UNICODE)0x03A1;
    case 0x03F2: /* GREEK LUNATE SIGMA SYMBOL */
	return (Py_UNICODE)0x03A3;
    case 0x0430: /* CYRILLIC SMALL LETTER A */
	return (Py_UNICODE)0x0410;
    case 0x0431: /* CYRILLIC SMALL LETTER BE */
	return (Py_UNICODE)0x0411;
    case 0x0432: /* CYRILLIC SMALL LETTER VE */
	return (Py_UNICODE)0x0412;
    case 0x0433: /* CYRILLIC SMALL LETTER GHE */
	return (Py_UNICODE)0x0413;
    case 0x0434: /* CYRILLIC SMALL LETTER DE */
	return (Py_UNICODE)0x0414;
    case 0x0435: /* CYRILLIC SMALL LETTER IE */
	return (Py_UNICODE)0x0415;
    case 0x0436: /* CYRILLIC SMALL LETTER ZHE */
	return (Py_UNICODE)0x0416;
    case 0x0437: /* CYRILLIC SMALL LETTER ZE */
	return (Py_UNICODE)0x0417;
    case 0x0438: /* CYRILLIC SMALL LETTER I */
	return (Py_UNICODE)0x0418;
    case 0x0439: /* CYRILLIC SMALL LETTER SHORT I */
	return (Py_UNICODE)0x0419;
    case 0x043A: /* CYRILLIC SMALL LETTER KA */
	return (Py_UNICODE)0x041A;
    case 0x043B: /* CYRILLIC SMALL LETTER EL */
	return (Py_UNICODE)0x041B;
    case 0x043C: /* CYRILLIC SMALL LETTER EM */
	return (Py_UNICODE)0x041C;
    case 0x043D: /* CYRILLIC SMALL LETTER EN */
	return (Py_UNICODE)0x041D;
    case 0x043E: /* CYRILLIC SMALL LETTER O */
	return (Py_UNICODE)0x041E;
    case 0x043F: /* CYRILLIC SMALL LETTER PE */
	return (Py_UNICODE)0x041F;
    case 0x0440: /* CYRILLIC SMALL LETTER ER */
	return (Py_UNICODE)0x0420;
    case 0x0441: /* CYRILLIC SMALL LETTER ES */
	return (Py_UNICODE)0x0421;
    case 0x0442: /* CYRILLIC SMALL LETTER TE */
	return (Py_UNICODE)0x0422;
    case 0x0443: /* CYRILLIC SMALL LETTER U */
	return (Py_UNICODE)0x0423;
    case 0x0444: /* CYRILLIC SMALL LETTER EF */
	return (Py_UNICODE)0x0424;
    case 0x0445: /* CYRILLIC SMALL LETTER HA */
	return (Py_UNICODE)0x0425;
    case 0x0446: /* CYRILLIC SMALL LETTER TSE */
	return (Py_UNICODE)0x0426;
    case 0x0447: /* CYRILLIC SMALL LETTER CHE */
	return (Py_UNICODE)0x0427;
    case 0x0448: /* CYRILLIC SMALL LETTER SHA */
	return (Py_UNICODE)0x0428;
    case 0x0449: /* CYRILLIC SMALL LETTER SHCHA */
	return (Py_UNICODE)0x0429;
    case 0x044A: /* CYRILLIC SMALL LETTER HARD SIGN */
	return (Py_UNICODE)0x042A;
    case 0x044B: /* CYRILLIC SMALL LETTER YERU */
	return (Py_UNICODE)0x042B;
    case 0x044C: /* CYRILLIC SMALL LETTER SOFT SIGN */
	return (Py_UNICODE)0x042C;
    case 0x044D: /* CYRILLIC SMALL LETTER E */
	return (Py_UNICODE)0x042D;
    case 0x044E: /* CYRILLIC SMALL LETTER YU */
	return (Py_UNICODE)0x042E;
    case 0x044F: /* CYRILLIC SMALL LETTER YA */
	return (Py_UNICODE)0x042F;
    case 0x0450: /* CYRILLIC SMALL LETTER IE WITH GRAVE */
	return (Py_UNICODE)0x0400;
    case 0x0451: /* CYRILLIC SMALL LETTER IO */
	return (Py_UNICODE)0x0401;
    case 0x0452: /* CYRILLIC SMALL LETTER DJE */
	return (Py_UNICODE)0x0402;
    case 0x0453: /* CYRILLIC SMALL LETTER GJE */
	return (Py_UNICODE)0x0403;
    case 0x0454: /* CYRILLIC SMALL LETTER UKRAINIAN IE */
	return (Py_UNICODE)0x0404;
    case 0x0455: /* CYRILLIC SMALL LETTER DZE */
	return (Py_UNICODE)0x0405;
    case 0x0456: /* CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
	return (Py_UNICODE)0x0406;
    case 0x0457: /* CYRILLIC SMALL LETTER YI */
	return (Py_UNICODE)0x0407;
    case 0x0458: /* CYRILLIC SMALL LETTER JE */
	return (Py_UNICODE)0x0408;
    case 0x0459: /* CYRILLIC SMALL LETTER LJE */
	return (Py_UNICODE)0x0409;
    case 0x045A: /* CYRILLIC SMALL LETTER NJE */
	return (Py_UNICODE)0x040A;
    case 0x045B: /* CYRILLIC SMALL LETTER TSHE */
	return (Py_UNICODE)0x040B;
    case 0x045C: /* CYRILLIC SMALL LETTER KJE */
	return (Py_UNICODE)0x040C;
    case 0x045D: /* CYRILLIC SMALL LETTER I WITH GRAVE */
	return (Py_UNICODE)0x040D;
    case 0x045E: /* CYRILLIC SMALL LETTER SHORT U */
	return (Py_UNICODE)0x040E;
    case 0x045F: /* CYRILLIC SMALL LETTER DZHE */
	return (Py_UNICODE)0x040F;
    case 0x0461: /* CYRILLIC SMALL LETTER OMEGA */
	return (Py_UNICODE)0x0460;
    case 0x0463: /* CYRILLIC SMALL LETTER YAT */
	return (Py_UNICODE)0x0462;
    case 0x0465: /* CYRILLIC SMALL LETTER IOTIFIED E */
	return (Py_UNICODE)0x0464;
    case 0x0467: /* CYRILLIC SMALL LETTER LITTLE YUS */
	return (Py_UNICODE)0x0466;
    case 0x0469: /* CYRILLIC SMALL LETTER IOTIFIED LITTLE YUS */
	return (Py_UNICODE)0x0468;
    case 0x046B: /* CYRILLIC SMALL LETTER BIG YUS */
	return (Py_UNICODE)0x046A;
    case 0x046D: /* CYRILLIC SMALL LETTER IOTIFIED BIG YUS */
	return (Py_UNICODE)0x046C;
    case 0x046F: /* CYRILLIC SMALL LETTER KSI */
	return (Py_UNICODE)0x046E;
    case 0x0471: /* CYRILLIC SMALL LETTER PSI */
	return (Py_UNICODE)0x0470;
    case 0x0473: /* CYRILLIC SMALL LETTER FITA */
	return (Py_UNICODE)0x0472;
    case 0x0475: /* CYRILLIC SMALL LETTER IZHITSA */
	return (Py_UNICODE)0x0474;
    case 0x0477: /* CYRILLIC SMALL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
	return (Py_UNICODE)0x0476;
    case 0x0479: /* CYRILLIC SMALL LETTER UK */
	return (Py_UNICODE)0x0478;
    case 0x047B: /* CYRILLIC SMALL LETTER ROUND OMEGA */
	return (Py_UNICODE)0x047A;
    case 0x047D: /* CYRILLIC SMALL LETTER OMEGA WITH TITLO */
	return (Py_UNICODE)0x047C;
    case 0x047F: /* CYRILLIC SMALL LETTER OT */
	return (Py_UNICODE)0x047E;
    case 0x0481: /* CYRILLIC SMALL LETTER KOPPA */
	return (Py_UNICODE)0x0480;
    case 0x048D: /* CYRILLIC SMALL LETTER SEMISOFT SIGN */
	return (Py_UNICODE)0x048C;
    case 0x048F: /* CYRILLIC SMALL LETTER ER WITH TICK */
	return (Py_UNICODE)0x048E;
    case 0x0491: /* CYRILLIC SMALL LETTER GHE WITH UPTURN */
	return (Py_UNICODE)0x0490;
    case 0x0493: /* CYRILLIC SMALL LETTER GHE WITH STROKE */
	return (Py_UNICODE)0x0492;
    case 0x0495: /* CYRILLIC SMALL LETTER GHE WITH MIDDLE HOOK */
	return (Py_UNICODE)0x0494;
    case 0x0497: /* CYRILLIC SMALL LETTER ZHE WITH DESCENDER */
	return (Py_UNICODE)0x0496;
    case 0x0499: /* CYRILLIC SMALL LETTER ZE WITH DESCENDER */
	return (Py_UNICODE)0x0498;
    case 0x049B: /* CYRILLIC SMALL LETTER KA WITH DESCENDER */
	return (Py_UNICODE)0x049A;
    case 0x049D: /* CYRILLIC SMALL LETTER KA WITH VERTICAL STROKE */
	return (Py_UNICODE)0x049C;
    case 0x049F: /* CYRILLIC SMALL LETTER KA WITH STROKE */
	return (Py_UNICODE)0x049E;
    case 0x04A1: /* CYRILLIC SMALL LETTER BASHKIR KA */
	return (Py_UNICODE)0x04A0;
    case 0x04A3: /* CYRILLIC SMALL LETTER EN WITH DESCENDER */
	return (Py_UNICODE)0x04A2;
    case 0x04A5: /* CYRILLIC SMALL LIGATURE EN GHE */
	return (Py_UNICODE)0x04A4;
    case 0x04A7: /* CYRILLIC SMALL LETTER PE WITH MIDDLE HOOK */
	return (Py_UNICODE)0x04A6;
    case 0x04A9: /* CYRILLIC SMALL LETTER ABKHASIAN HA */
	return (Py_UNICODE)0x04A8;
    case 0x04AB: /* CYRILLIC SMALL LETTER ES WITH DESCENDER */
	return (Py_UNICODE)0x04AA;
    case 0x04AD: /* CYRILLIC SMALL LETTER TE WITH DESCENDER */
	return (Py_UNICODE)0x04AC;
    case 0x04AF: /* CYRILLIC SMALL LETTER STRAIGHT U */
	return (Py_UNICODE)0x04AE;
    case 0x04B1: /* CYRILLIC SMALL LETTER STRAIGHT U WITH STROKE */
	return (Py_UNICODE)0x04B0;
    case 0x04B3: /* CYRILLIC SMALL LETTER HA WITH DESCENDER */
	return (Py_UNICODE)0x04B2;
    case 0x04B5: /* CYRILLIC SMALL LIGATURE TE TSE */
	return (Py_UNICODE)0x04B4;
    case 0x04B7: /* CYRILLIC SMALL LETTER CHE WITH DESCENDER */
	return (Py_UNICODE)0x04B6;
    case 0x04B9: /* CYRILLIC SMALL LETTER CHE WITH VERTICAL STROKE */
	return (Py_UNICODE)0x04B8;
    case 0x04BB: /* CYRILLIC SMALL LETTER SHHA */
	return (Py_UNICODE)0x04BA;
    case 0x04BD: /* CYRILLIC SMALL LETTER ABKHASIAN CHE */
	return (Py_UNICODE)0x04BC;
    case 0x04BF: /* CYRILLIC SMALL LETTER ABKHASIAN CHE WITH DESCENDER */
	return (Py_UNICODE)0x04BE;
    case 0x04C2: /* CYRILLIC SMALL LETTER ZHE WITH BREVE */
	return (Py_UNICODE)0x04C1;
    case 0x04C4: /* CYRILLIC SMALL LETTER KA WITH HOOK */
	return (Py_UNICODE)0x04C3;
    case 0x04C8: /* CYRILLIC SMALL LETTER EN WITH HOOK */
	return (Py_UNICODE)0x04C7;
    case 0x04CC: /* CYRILLIC SMALL LETTER KHAKASSIAN CHE */
	return (Py_UNICODE)0x04CB;
    case 0x04D1: /* CYRILLIC SMALL LETTER A WITH BREVE */
	return (Py_UNICODE)0x04D0;
    case 0x04D3: /* CYRILLIC SMALL LETTER A WITH DIAERESIS */
	return (Py_UNICODE)0x04D2;
    case 0x04D5: /* CYRILLIC SMALL LIGATURE A IE */
	return (Py_UNICODE)0x04D4;
    case 0x04D7: /* CYRILLIC SMALL LETTER IE WITH BREVE */
	return (Py_UNICODE)0x04D6;
    case 0x04D9: /* CYRILLIC SMALL LETTER SCHWA */
	return (Py_UNICODE)0x04D8;
    case 0x04DB: /* CYRILLIC SMALL LETTER SCHWA WITH DIAERESIS */
	return (Py_UNICODE)0x04DA;
    case 0x04DD: /* CYRILLIC SMALL LETTER ZHE WITH DIAERESIS */
	return (Py_UNICODE)0x04DC;
    case 0x04DF: /* CYRILLIC SMALL LETTER ZE WITH DIAERESIS */
	return (Py_UNICODE)0x04DE;
    case 0x04E1: /* CYRILLIC SMALL LETTER ABKHASIAN DZE */
	return (Py_UNICODE)0x04E0;
    case 0x04E3: /* CYRILLIC SMALL LETTER I WITH MACRON */
	return (Py_UNICODE)0x04E2;
    case 0x04E5: /* CYRILLIC SMALL LETTER I WITH DIAERESIS */
	return (Py_UNICODE)0x04E4;
    case 0x04E7: /* CYRILLIC SMALL LETTER O WITH DIAERESIS */
	return (Py_UNICODE)0x04E6;
    case 0x04E9: /* CYRILLIC SMALL LETTER BARRED O */
	return (Py_UNICODE)0x04E8;
    case 0x04EB: /* CYRILLIC SMALL LETTER BARRED O WITH DIAERESIS */
	return (Py_UNICODE)0x04EA;
    case 0x04ED: /* CYRILLIC SMALL LETTER E WITH DIAERESIS */
	return (Py_UNICODE)0x04EC;
    case 0x04EF: /* CYRILLIC SMALL LETTER U WITH MACRON */
	return (Py_UNICODE)0x04EE;
    case 0x04F1: /* CYRILLIC SMALL LETTER U WITH DIAERESIS */
	return (Py_UNICODE)0x04F0;
    case 0x04F3: /* CYRILLIC SMALL LETTER U WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x04F2;
    case 0x04F5: /* CYRILLIC SMALL LETTER CHE WITH DIAERESIS */
	return (Py_UNICODE)0x04F4;
    case 0x04F9: /* CYRILLIC SMALL LETTER YERU WITH DIAERESIS */
	return (Py_UNICODE)0x04F8;
    case 0x0561: /* ARMENIAN SMALL LETTER AYB */
	return (Py_UNICODE)0x0531;
    case 0x0562: /* ARMENIAN SMALL LETTER BEN */
	return (Py_UNICODE)0x0532;
    case 0x0563: /* ARMENIAN SMALL LETTER GIM */
	return (Py_UNICODE)0x0533;
    case 0x0564: /* ARMENIAN SMALL LETTER DA */
	return (Py_UNICODE)0x0534;
    case 0x0565: /* ARMENIAN SMALL LETTER ECH */
	return (Py_UNICODE)0x0535;
    case 0x0566: /* ARMENIAN SMALL LETTER ZA */
	return (Py_UNICODE)0x0536;
    case 0x0567: /* ARMENIAN SMALL LETTER EH */
	return (Py_UNICODE)0x0537;
    case 0x0568: /* ARMENIAN SMALL LETTER ET */
	return (Py_UNICODE)0x0538;
    case 0x0569: /* ARMENIAN SMALL LETTER TO */
	return (Py_UNICODE)0x0539;
    case 0x056A: /* ARMENIAN SMALL LETTER ZHE */
	return (Py_UNICODE)0x053A;
    case 0x056B: /* ARMENIAN SMALL LETTER INI */
	return (Py_UNICODE)0x053B;
    case 0x056C: /* ARMENIAN SMALL LETTER LIWN */
	return (Py_UNICODE)0x053C;
    case 0x056D: /* ARMENIAN SMALL LETTER XEH */
	return (Py_UNICODE)0x053D;
    case 0x056E: /* ARMENIAN SMALL LETTER CA */
	return (Py_UNICODE)0x053E;
    case 0x056F: /* ARMENIAN SMALL LETTER KEN */
	return (Py_UNICODE)0x053F;
    case 0x0570: /* ARMENIAN SMALL LETTER HO */
	return (Py_UNICODE)0x0540;
    case 0x0571: /* ARMENIAN SMALL LETTER JA */
	return (Py_UNICODE)0x0541;
    case 0x0572: /* ARMENIAN SMALL LETTER GHAD */
	return (Py_UNICODE)0x0542;
    case 0x0573: /* ARMENIAN SMALL LETTER CHEH */
	return (Py_UNICODE)0x0543;
    case 0x0574: /* ARMENIAN SMALL LETTER MEN */
	return (Py_UNICODE)0x0544;
    case 0x0575: /* ARMENIAN SMALL LETTER YI */
	return (Py_UNICODE)0x0545;
    case 0x0576: /* ARMENIAN SMALL LETTER NOW */
	return (Py_UNICODE)0x0546;
    case 0x0577: /* ARMENIAN SMALL LETTER SHA */
	return (Py_UNICODE)0x0547;
    case 0x0578: /* ARMENIAN SMALL LETTER VO */
	return (Py_UNICODE)0x0548;
    case 0x0579: /* ARMENIAN SMALL LETTER CHA */
	return (Py_UNICODE)0x0549;
    case 0x057A: /* ARMENIAN SMALL LETTER PEH */
	return (Py_UNICODE)0x054A;
    case 0x057B: /* ARMENIAN SMALL LETTER JHEH */
	return (Py_UNICODE)0x054B;
    case 0x057C: /* ARMENIAN SMALL LETTER RA */
	return (Py_UNICODE)0x054C;
    case 0x057D: /* ARMENIAN SMALL LETTER SEH */
	return (Py_UNICODE)0x054D;
    case 0x057E: /* ARMENIAN SMALL LETTER VEW */
	return (Py_UNICODE)0x054E;
    case 0x057F: /* ARMENIAN SMALL LETTER TIWN */
	return (Py_UNICODE)0x054F;
    case 0x0580: /* ARMENIAN SMALL LETTER REH */
	return (Py_UNICODE)0x0550;
    case 0x0581: /* ARMENIAN SMALL LETTER CO */
	return (Py_UNICODE)0x0551;
    case 0x0582: /* ARMENIAN SMALL LETTER YIWN */
	return (Py_UNICODE)0x0552;
    case 0x0583: /* ARMENIAN SMALL LETTER PIWR */
	return (Py_UNICODE)0x0553;
    case 0x0584: /* ARMENIAN SMALL LETTER KEH */
	return (Py_UNICODE)0x0554;
    case 0x0585: /* ARMENIAN SMALL LETTER OH */
	return (Py_UNICODE)0x0555;
    case 0x0586: /* ARMENIAN SMALL LETTER FEH */
	return (Py_UNICODE)0x0556;
    case 0x1E01: /* LATIN SMALL LETTER A WITH RING BELOW */
	return (Py_UNICODE)0x1E00;
    case 0x1E03: /* LATIN SMALL LETTER B WITH DOT ABOVE */
	return (Py_UNICODE)0x1E02;
    case 0x1E05: /* LATIN SMALL LETTER B WITH DOT BELOW */
	return (Py_UNICODE)0x1E04;
    case 0x1E07: /* LATIN SMALL LETTER B WITH LINE BELOW */
	return (Py_UNICODE)0x1E06;
    case 0x1E09: /* LATIN SMALL LETTER C WITH CEDILLA AND ACUTE */
	return (Py_UNICODE)0x1E08;
    case 0x1E0B: /* LATIN SMALL LETTER D WITH DOT ABOVE */
	return (Py_UNICODE)0x1E0A;
    case 0x1E0D: /* LATIN SMALL LETTER D WITH DOT BELOW */
	return (Py_UNICODE)0x1E0C;
    case 0x1E0F: /* LATIN SMALL LETTER D WITH LINE BELOW */
	return (Py_UNICODE)0x1E0E;
    case 0x1E11: /* LATIN SMALL LETTER D WITH CEDILLA */
	return (Py_UNICODE)0x1E10;
    case 0x1E13: /* LATIN SMALL LETTER D WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E12;
    case 0x1E15: /* LATIN SMALL LETTER E WITH MACRON AND GRAVE */
	return (Py_UNICODE)0x1E14;
    case 0x1E17: /* LATIN SMALL LETTER E WITH MACRON AND ACUTE */
	return (Py_UNICODE)0x1E16;
    case 0x1E19: /* LATIN SMALL LETTER E WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E18;
    case 0x1E1B: /* LATIN SMALL LETTER E WITH TILDE BELOW */
	return (Py_UNICODE)0x1E1A;
    case 0x1E1D: /* LATIN SMALL LETTER E WITH CEDILLA AND BREVE */
	return (Py_UNICODE)0x1E1C;
    case 0x1E1F: /* LATIN SMALL LETTER F WITH DOT ABOVE */
	return (Py_UNICODE)0x1E1E;
    case 0x1E21: /* LATIN SMALL LETTER G WITH MACRON */
	return (Py_UNICODE)0x1E20;
    case 0x1E23: /* LATIN SMALL LETTER H WITH DOT ABOVE */
	return (Py_UNICODE)0x1E22;
    case 0x1E25: /* LATIN SMALL LETTER H WITH DOT BELOW */
	return (Py_UNICODE)0x1E24;
    case 0x1E27: /* LATIN SMALL LETTER H WITH DIAERESIS */
	return (Py_UNICODE)0x1E26;
    case 0x1E29: /* LATIN SMALL LETTER H WITH CEDILLA */
	return (Py_UNICODE)0x1E28;
    case 0x1E2B: /* LATIN SMALL LETTER H WITH BREVE BELOW */
	return (Py_UNICODE)0x1E2A;
    case 0x1E2D: /* LATIN SMALL LETTER I WITH TILDE BELOW */
	return (Py_UNICODE)0x1E2C;
    case 0x1E2F: /* LATIN SMALL LETTER I WITH DIAERESIS AND ACUTE */
	return (Py_UNICODE)0x1E2E;
    case 0x1E31: /* LATIN SMALL LETTER K WITH ACUTE */
	return (Py_UNICODE)0x1E30;
    case 0x1E33: /* LATIN SMALL LETTER K WITH DOT BELOW */
	return (Py_UNICODE)0x1E32;
    case 0x1E35: /* LATIN SMALL LETTER K WITH LINE BELOW */
	return (Py_UNICODE)0x1E34;
    case 0x1E37: /* LATIN SMALL LETTER L WITH DOT BELOW */
	return (Py_UNICODE)0x1E36;
    case 0x1E39: /* LATIN SMALL LETTER L WITH DOT BELOW AND MACRON */
	return (Py_UNICODE)0x1E38;
    case 0x1E3B: /* LATIN SMALL LETTER L WITH LINE BELOW */
	return (Py_UNICODE)0x1E3A;
    case 0x1E3D: /* LATIN SMALL LETTER L WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E3C;
    case 0x1E3F: /* LATIN SMALL LETTER M WITH ACUTE */
	return (Py_UNICODE)0x1E3E;
    case 0x1E41: /* LATIN SMALL LETTER M WITH DOT ABOVE */
	return (Py_UNICODE)0x1E40;
    case 0x1E43: /* LATIN SMALL LETTER M WITH DOT BELOW */
	return (Py_UNICODE)0x1E42;
    case 0x1E45: /* LATIN SMALL LETTER N WITH DOT ABOVE */
	return (Py_UNICODE)0x1E44;
    case 0x1E47: /* LATIN SMALL LETTER N WITH DOT BELOW */
	return (Py_UNICODE)0x1E46;
    case 0x1E49: /* LATIN SMALL LETTER N WITH LINE BELOW */
	return (Py_UNICODE)0x1E48;
    case 0x1E4B: /* LATIN SMALL LETTER N WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E4A;
    case 0x1E4D: /* LATIN SMALL LETTER O WITH TILDE AND ACUTE */
	return (Py_UNICODE)0x1E4C;
    case 0x1E4F: /* LATIN SMALL LETTER O WITH TILDE AND DIAERESIS */
	return (Py_UNICODE)0x1E4E;
    case 0x1E51: /* LATIN SMALL LETTER O WITH MACRON AND GRAVE */
	return (Py_UNICODE)0x1E50;
    case 0x1E53: /* LATIN SMALL LETTER O WITH MACRON AND ACUTE */
	return (Py_UNICODE)0x1E52;
    case 0x1E55: /* LATIN SMALL LETTER P WITH ACUTE */
	return (Py_UNICODE)0x1E54;
    case 0x1E57: /* LATIN SMALL LETTER P WITH DOT ABOVE */
	return (Py_UNICODE)0x1E56;
    case 0x1E59: /* LATIN SMALL LETTER R WITH DOT ABOVE */
	return (Py_UNICODE)0x1E58;
    case 0x1E5B: /* LATIN SMALL LETTER R WITH DOT BELOW */
	return (Py_UNICODE)0x1E5A;
    case 0x1E5D: /* LATIN SMALL LETTER R WITH DOT BELOW AND MACRON */
	return (Py_UNICODE)0x1E5C;
    case 0x1E5F: /* LATIN SMALL LETTER R WITH LINE BELOW */
	return (Py_UNICODE)0x1E5E;
    case 0x1E61: /* LATIN SMALL LETTER S WITH DOT ABOVE */
	return (Py_UNICODE)0x1E60;
    case 0x1E63: /* LATIN SMALL LETTER S WITH DOT BELOW */
	return (Py_UNICODE)0x1E62;
    case 0x1E65: /* LATIN SMALL LETTER S WITH ACUTE AND DOT ABOVE */
	return (Py_UNICODE)0x1E64;
    case 0x1E67: /* LATIN SMALL LETTER S WITH CARON AND DOT ABOVE */
	return (Py_UNICODE)0x1E66;
    case 0x1E69: /* LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE */
	return (Py_UNICODE)0x1E68;
    case 0x1E6B: /* LATIN SMALL LETTER T WITH DOT ABOVE */
	return (Py_UNICODE)0x1E6A;
    case 0x1E6D: /* LATIN SMALL LETTER T WITH DOT BELOW */
	return (Py_UNICODE)0x1E6C;
    case 0x1E6F: /* LATIN SMALL LETTER T WITH LINE BELOW */
	return (Py_UNICODE)0x1E6E;
    case 0x1E71: /* LATIN SMALL LETTER T WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E70;
    case 0x1E73: /* LATIN SMALL LETTER U WITH DIAERESIS BELOW */
	return (Py_UNICODE)0x1E72;
    case 0x1E75: /* LATIN SMALL LETTER U WITH TILDE BELOW */
	return (Py_UNICODE)0x1E74;
    case 0x1E77: /* LATIN SMALL LETTER U WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E76;
    case 0x1E79: /* LATIN SMALL LETTER U WITH TILDE AND ACUTE */
	return (Py_UNICODE)0x1E78;
    case 0x1E7B: /* LATIN SMALL LETTER U WITH MACRON AND DIAERESIS */
	return (Py_UNICODE)0x1E7A;
    case 0x1E7D: /* LATIN SMALL LETTER V WITH TILDE */
	return (Py_UNICODE)0x1E7C;
    case 0x1E7F: /* LATIN SMALL LETTER V WITH DOT BELOW */
	return (Py_UNICODE)0x1E7E;
    case 0x1E81: /* LATIN SMALL LETTER W WITH GRAVE */
	return (Py_UNICODE)0x1E80;
    case 0x1E83: /* LATIN SMALL LETTER W WITH ACUTE */
	return (Py_UNICODE)0x1E82;
    case 0x1E85: /* LATIN SMALL LETTER W WITH DIAERESIS */
	return (Py_UNICODE)0x1E84;
    case 0x1E87: /* LATIN SMALL LETTER W WITH DOT ABOVE */
	return (Py_UNICODE)0x1E86;
    case 0x1E89: /* LATIN SMALL LETTER W WITH DOT BELOW */
	return (Py_UNICODE)0x1E88;
    case 0x1E8B: /* LATIN SMALL LETTER X WITH DOT ABOVE */
	return (Py_UNICODE)0x1E8A;
    case 0x1E8D: /* LATIN SMALL LETTER X WITH DIAERESIS */
	return (Py_UNICODE)0x1E8C;
    case 0x1E8F: /* LATIN SMALL LETTER Y WITH DOT ABOVE */
	return (Py_UNICODE)0x1E8E;
    case 0x1E91: /* LATIN SMALL LETTER Z WITH CIRCUMFLEX */
	return (Py_UNICODE)0x1E90;
    case 0x1E93: /* LATIN SMALL LETTER Z WITH DOT BELOW */
	return (Py_UNICODE)0x1E92;
    case 0x1E95: /* LATIN SMALL LETTER Z WITH LINE BELOW */
	return (Py_UNICODE)0x1E94;
    case 0x1E9B: /* LATIN SMALL LETTER LONG S WITH DOT ABOVE */
	return (Py_UNICODE)0x1E60;
    case 0x1EA1: /* LATIN SMALL LETTER A WITH DOT BELOW */
	return (Py_UNICODE)0x1EA0;
    case 0x1EA3: /* LATIN SMALL LETTER A WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EA2;
    case 0x1EA5: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1EA4;
    case 0x1EA7: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1EA6;
    case 0x1EA9: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1EA8;
    case 0x1EAB: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1EAA;
    case 0x1EAD: /* LATIN SMALL LETTER A WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1EAC;
    case 0x1EAF: /* LATIN SMALL LETTER A WITH BREVE AND ACUTE */
	return (Py_UNICODE)0x1EAE;
    case 0x1EB1: /* LATIN SMALL LETTER A WITH BREVE AND GRAVE */
	return (Py_UNICODE)0x1EB0;
    case 0x1EB3: /* LATIN SMALL LETTER A WITH BREVE AND HOOK ABOVE */
	return (Py_UNICODE)0x1EB2;
    case 0x1EB5: /* LATIN SMALL LETTER A WITH BREVE AND TILDE */
	return (Py_UNICODE)0x1EB4;
    case 0x1EB7: /* LATIN SMALL LETTER A WITH BREVE AND DOT BELOW */
	return (Py_UNICODE)0x1EB6;
    case 0x1EB9: /* LATIN SMALL LETTER E WITH DOT BELOW */
	return (Py_UNICODE)0x1EB8;
    case 0x1EBB: /* LATIN SMALL LETTER E WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EBA;
    case 0x1EBD: /* LATIN SMALL LETTER E WITH TILDE */
	return (Py_UNICODE)0x1EBC;
    case 0x1EBF: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1EBE;
    case 0x1EC1: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1EC0;
    case 0x1EC3: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1EC2;
    case 0x1EC5: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1EC4;
    case 0x1EC7: /* LATIN SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1EC6;
    case 0x1EC9: /* LATIN SMALL LETTER I WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EC8;
    case 0x1ECB: /* LATIN SMALL LETTER I WITH DOT BELOW */
	return (Py_UNICODE)0x1ECA;
    case 0x1ECD: /* LATIN SMALL LETTER O WITH DOT BELOW */
	return (Py_UNICODE)0x1ECC;
    case 0x1ECF: /* LATIN SMALL LETTER O WITH HOOK ABOVE */
	return (Py_UNICODE)0x1ECE;
    case 0x1ED1: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1ED0;
    case 0x1ED3: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1ED2;
    case 0x1ED5: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1ED4;
    case 0x1ED7: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1ED6;
    case 0x1ED9: /* LATIN SMALL LETTER O WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1ED8;
    case 0x1EDB: /* LATIN SMALL LETTER O WITH HORN AND ACUTE */
	return (Py_UNICODE)0x1EDA;
    case 0x1EDD: /* LATIN SMALL LETTER O WITH HORN AND GRAVE */
	return (Py_UNICODE)0x1EDC;
    case 0x1EDF: /* LATIN SMALL LETTER O WITH HORN AND HOOK ABOVE */
	return (Py_UNICODE)0x1EDE;
    case 0x1EE1: /* LATIN SMALL LETTER O WITH HORN AND TILDE */
	return (Py_UNICODE)0x1EE0;
    case 0x1EE3: /* LATIN SMALL LETTER O WITH HORN AND DOT BELOW */
	return (Py_UNICODE)0x1EE2;
    case 0x1EE5: /* LATIN SMALL LETTER U WITH DOT BELOW */
	return (Py_UNICODE)0x1EE4;
    case 0x1EE7: /* LATIN SMALL LETTER U WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EE6;
    case 0x1EE9: /* LATIN SMALL LETTER U WITH HORN AND ACUTE */
	return (Py_UNICODE)0x1EE8;
    case 0x1EEB: /* LATIN SMALL LETTER U WITH HORN AND GRAVE */
	return (Py_UNICODE)0x1EEA;
    case 0x1EED: /* LATIN SMALL LETTER U WITH HORN AND HOOK ABOVE */
	return (Py_UNICODE)0x1EEC;
    case 0x1EEF: /* LATIN SMALL LETTER U WITH HORN AND TILDE */
	return (Py_UNICODE)0x1EEE;
    case 0x1EF1: /* LATIN SMALL LETTER U WITH HORN AND DOT BELOW */
	return (Py_UNICODE)0x1EF0;
    case 0x1EF3: /* LATIN SMALL LETTER Y WITH GRAVE */
	return (Py_UNICODE)0x1EF2;
    case 0x1EF5: /* LATIN SMALL LETTER Y WITH DOT BELOW */
	return (Py_UNICODE)0x1EF4;
    case 0x1EF7: /* LATIN SMALL LETTER Y WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EF6;
    case 0x1EF9: /* LATIN SMALL LETTER Y WITH TILDE */
	return (Py_UNICODE)0x1EF8;
    case 0x1F00: /* GREEK SMALL LETTER ALPHA WITH PSILI */
	return (Py_UNICODE)0x1F08;
    case 0x1F01: /* GREEK SMALL LETTER ALPHA WITH DASIA */
	return (Py_UNICODE)0x1F09;
    case 0x1F02: /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F0A;
    case 0x1F03: /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F0B;
    case 0x1F04: /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F0C;
    case 0x1F05: /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F0D;
    case 0x1F06: /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F0E;
    case 0x1F07: /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F0F;
    case 0x1F10: /* GREEK SMALL LETTER EPSILON WITH PSILI */
	return (Py_UNICODE)0x1F18;
    case 0x1F11: /* GREEK SMALL LETTER EPSILON WITH DASIA */
	return (Py_UNICODE)0x1F19;
    case 0x1F12: /* GREEK SMALL LETTER EPSILON WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F1A;
    case 0x1F13: /* GREEK SMALL LETTER EPSILON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F1B;
    case 0x1F14: /* GREEK SMALL LETTER EPSILON WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F1C;
    case 0x1F15: /* GREEK SMALL LETTER EPSILON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F1D;
    case 0x1F20: /* GREEK SMALL LETTER ETA WITH PSILI */
	return (Py_UNICODE)0x1F28;
    case 0x1F21: /* GREEK SMALL LETTER ETA WITH DASIA */
	return (Py_UNICODE)0x1F29;
    case 0x1F22: /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F2A;
    case 0x1F23: /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F2B;
    case 0x1F24: /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F2C;
    case 0x1F25: /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F2D;
    case 0x1F26: /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F2E;
    case 0x1F27: /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F2F;
    case 0x1F30: /* GREEK SMALL LETTER IOTA WITH PSILI */
	return (Py_UNICODE)0x1F38;
    case 0x1F31: /* GREEK SMALL LETTER IOTA WITH DASIA */
	return (Py_UNICODE)0x1F39;
    case 0x1F32: /* GREEK SMALL LETTER IOTA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F3A;
    case 0x1F33: /* GREEK SMALL LETTER IOTA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F3B;
    case 0x1F34: /* GREEK SMALL LETTER IOTA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F3C;
    case 0x1F35: /* GREEK SMALL LETTER IOTA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F3D;
    case 0x1F36: /* GREEK SMALL LETTER IOTA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F3E;
    case 0x1F37: /* GREEK SMALL LETTER IOTA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F3F;
    case 0x1F40: /* GREEK SMALL LETTER OMICRON WITH PSILI */
	return (Py_UNICODE)0x1F48;
    case 0x1F41: /* GREEK SMALL LETTER OMICRON WITH DASIA */
	return (Py_UNICODE)0x1F49;
    case 0x1F42: /* GREEK SMALL LETTER OMICRON WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F4A;
    case 0x1F43: /* GREEK SMALL LETTER OMICRON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F4B;
    case 0x1F44: /* GREEK SMALL LETTER OMICRON WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F4C;
    case 0x1F45: /* GREEK SMALL LETTER OMICRON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F4D;
    case 0x1F51: /* GREEK SMALL LETTER UPSILON WITH DASIA */
	return (Py_UNICODE)0x1F59;
    case 0x1F53: /* GREEK SMALL LETTER UPSILON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F5B;
    case 0x1F55: /* GREEK SMALL LETTER UPSILON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F5D;
    case 0x1F57: /* GREEK SMALL LETTER UPSILON WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F5F;
    case 0x1F60: /* GREEK SMALL LETTER OMEGA WITH PSILI */
	return (Py_UNICODE)0x1F68;
    case 0x1F61: /* GREEK SMALL LETTER OMEGA WITH DASIA */
	return (Py_UNICODE)0x1F69;
    case 0x1F62: /* GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F6A;
    case 0x1F63: /* GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F6B;
    case 0x1F64: /* GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F6C;
    case 0x1F65: /* GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F6D;
    case 0x1F66: /* GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F6E;
    case 0x1F67: /* GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F6F;
    case 0x1F70: /* GREEK SMALL LETTER ALPHA WITH VARIA */
	return (Py_UNICODE)0x1FBA;
    case 0x1F71: /* GREEK SMALL LETTER ALPHA WITH OXIA */
	return (Py_UNICODE)0x1FBB;
    case 0x1F72: /* GREEK SMALL LETTER EPSILON WITH VARIA */
	return (Py_UNICODE)0x1FC8;
    case 0x1F73: /* GREEK SMALL LETTER EPSILON WITH OXIA */
	return (Py_UNICODE)0x1FC9;
    case 0x1F74: /* GREEK SMALL LETTER ETA WITH VARIA */
	return (Py_UNICODE)0x1FCA;
    case 0x1F75: /* GREEK SMALL LETTER ETA WITH OXIA */
	return (Py_UNICODE)0x1FCB;
    case 0x1F76: /* GREEK SMALL LETTER IOTA WITH VARIA */
	return (Py_UNICODE)0x1FDA;
    case 0x1F77: /* GREEK SMALL LETTER IOTA WITH OXIA */
	return (Py_UNICODE)0x1FDB;
    case 0x1F78: /* GREEK SMALL LETTER OMICRON WITH VARIA */
	return (Py_UNICODE)0x1FF8;
    case 0x1F79: /* GREEK SMALL LETTER OMICRON WITH OXIA */
	return (Py_UNICODE)0x1FF9;
    case 0x1F7A: /* GREEK SMALL LETTER UPSILON WITH VARIA */
	return (Py_UNICODE)0x1FEA;
    case 0x1F7B: /* GREEK SMALL LETTER UPSILON WITH OXIA */
	return (Py_UNICODE)0x1FEB;
    case 0x1F7C: /* GREEK SMALL LETTER OMEGA WITH VARIA */
	return (Py_UNICODE)0x1FFA;
    case 0x1F7D: /* GREEK SMALL LETTER OMEGA WITH OXIA */
	return (Py_UNICODE)0x1FFB;
    case 0x1F80: /* GREEK SMALL LETTER ALPHA WITH PSILI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F88;
    case 0x1F81: /* GREEK SMALL LETTER ALPHA WITH DASIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F89;
    case 0x1F82: /* GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8A;
    case 0x1F83: /* GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8B;
    case 0x1F84: /* GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8C;
    case 0x1F85: /* GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8D;
    case 0x1F86: /* GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8E;
    case 0x1F87: /* GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F8F;
    case 0x1F90: /* GREEK SMALL LETTER ETA WITH PSILI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F98;
    case 0x1F91: /* GREEK SMALL LETTER ETA WITH DASIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F99;
    case 0x1F92: /* GREEK SMALL LETTER ETA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9A;
    case 0x1F93: /* GREEK SMALL LETTER ETA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9B;
    case 0x1F94: /* GREEK SMALL LETTER ETA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9C;
    case 0x1F95: /* GREEK SMALL LETTER ETA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9D;
    case 0x1F96: /* GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9E;
    case 0x1F97: /* GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1F9F;
    case 0x1FA0: /* GREEK SMALL LETTER OMEGA WITH PSILI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FA8;
    case 0x1FA1: /* GREEK SMALL LETTER OMEGA WITH DASIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FA9;
    case 0x1FA2: /* GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAA;
    case 0x1FA3: /* GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAB;
    case 0x1FA4: /* GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAC;
    case 0x1FA5: /* GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAD;
    case 0x1FA6: /* GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAE;
    case 0x1FA7: /* GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI AND YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FAF;
    case 0x1FB0: /* GREEK SMALL LETTER ALPHA WITH VRACHY */
	return (Py_UNICODE)0x1FB8;
    case 0x1FB1: /* GREEK SMALL LETTER ALPHA WITH MACRON */
	return (Py_UNICODE)0x1FB9;
    case 0x1FB3: /* GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FBC;
    case 0x1FBE: /* GREEK PROSGEGRAMMENI */
	return (Py_UNICODE)0x0399;
    case 0x1FC3: /* GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FCC;
    case 0x1FD0: /* GREEK SMALL LETTER IOTA WITH VRACHY */
	return (Py_UNICODE)0x1FD8;
    case 0x1FD1: /* GREEK SMALL LETTER IOTA WITH MACRON */
	return (Py_UNICODE)0x1FD9;
    case 0x1FE0: /* GREEK SMALL LETTER UPSILON WITH VRACHY */
	return (Py_UNICODE)0x1FE8;
    case 0x1FE1: /* GREEK SMALL LETTER UPSILON WITH MACRON */
	return (Py_UNICODE)0x1FE9;
    case 0x1FE5: /* GREEK SMALL LETTER RHO WITH DASIA */
	return (Py_UNICODE)0x1FEC;
    case 0x1FF3: /* GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI */
	return (Py_UNICODE)0x1FFC;
    case 0x2170: /* SMALL ROMAN NUMERAL ONE */
	return (Py_UNICODE)0x2160;
    case 0x2171: /* SMALL ROMAN NUMERAL TWO */
	return (Py_UNICODE)0x2161;
    case 0x2172: /* SMALL ROMAN NUMERAL THREE */
	return (Py_UNICODE)0x2162;
    case 0x2173: /* SMALL ROMAN NUMERAL FOUR */
	return (Py_UNICODE)0x2163;
    case 0x2174: /* SMALL ROMAN NUMERAL FIVE */
	return (Py_UNICODE)0x2164;
    case 0x2175: /* SMALL ROMAN NUMERAL SIX */
	return (Py_UNICODE)0x2165;
    case 0x2176: /* SMALL ROMAN NUMERAL SEVEN */
	return (Py_UNICODE)0x2166;
    case 0x2177: /* SMALL ROMAN NUMERAL EIGHT */
	return (Py_UNICODE)0x2167;
    case 0x2178: /* SMALL ROMAN NUMERAL NINE */
	return (Py_UNICODE)0x2168;
    case 0x2179: /* SMALL ROMAN NUMERAL TEN */
	return (Py_UNICODE)0x2169;
    case 0x217A: /* SMALL ROMAN NUMERAL ELEVEN */
	return (Py_UNICODE)0x216A;
    case 0x217B: /* SMALL ROMAN NUMERAL TWELVE */
	return (Py_UNICODE)0x216B;
    case 0x217C: /* SMALL ROMAN NUMERAL FIFTY */
	return (Py_UNICODE)0x216C;
    case 0x217D: /* SMALL ROMAN NUMERAL ONE HUNDRED */
	return (Py_UNICODE)0x216D;
    case 0x217E: /* SMALL ROMAN NUMERAL FIVE HUNDRED */
	return (Py_UNICODE)0x216E;
    case 0x217F: /* SMALL ROMAN NUMERAL ONE THOUSAND */
	return (Py_UNICODE)0x216F;
    case 0x24D0: /* CIRCLED LATIN SMALL LETTER A */
	return (Py_UNICODE)0x24B6;
    case 0x24D1: /* CIRCLED LATIN SMALL LETTER B */
	return (Py_UNICODE)0x24B7;
    case 0x24D2: /* CIRCLED LATIN SMALL LETTER C */
	return (Py_UNICODE)0x24B8;
    case 0x24D3: /* CIRCLED LATIN SMALL LETTER D */
	return (Py_UNICODE)0x24B9;
    case 0x24D4: /* CIRCLED LATIN SMALL LETTER E */
	return (Py_UNICODE)0x24BA;
    case 0x24D5: /* CIRCLED LATIN SMALL LETTER F */
	return (Py_UNICODE)0x24BB;
    case 0x24D6: /* CIRCLED LATIN SMALL LETTER G */
	return (Py_UNICODE)0x24BC;
    case 0x24D7: /* CIRCLED LATIN SMALL LETTER H */
	return (Py_UNICODE)0x24BD;
    case 0x24D8: /* CIRCLED LATIN SMALL LETTER I */
	return (Py_UNICODE)0x24BE;
    case 0x24D9: /* CIRCLED LATIN SMALL LETTER J */
	return (Py_UNICODE)0x24BF;
    case 0x24DA: /* CIRCLED LATIN SMALL LETTER K */
	return (Py_UNICODE)0x24C0;
    case 0x24DB: /* CIRCLED LATIN SMALL LETTER L */
	return (Py_UNICODE)0x24C1;
    case 0x24DC: /* CIRCLED LATIN SMALL LETTER M */
	return (Py_UNICODE)0x24C2;
    case 0x24DD: /* CIRCLED LATIN SMALL LETTER N */
	return (Py_UNICODE)0x24C3;
    case 0x24DE: /* CIRCLED LATIN SMALL LETTER O */
	return (Py_UNICODE)0x24C4;
    case 0x24DF: /* CIRCLED LATIN SMALL LETTER P */
	return (Py_UNICODE)0x24C5;
    case 0x24E0: /* CIRCLED LATIN SMALL LETTER Q */
	return (Py_UNICODE)0x24C6;
    case 0x24E1: /* CIRCLED LATIN SMALL LETTER R */
	return (Py_UNICODE)0x24C7;
    case 0x24E2: /* CIRCLED LATIN SMALL LETTER S */
	return (Py_UNICODE)0x24C8;
    case 0x24E3: /* CIRCLED LATIN SMALL LETTER T */
	return (Py_UNICODE)0x24C9;
    case 0x24E4: /* CIRCLED LATIN SMALL LETTER U */
	return (Py_UNICODE)0x24CA;
    case 0x24E5: /* CIRCLED LATIN SMALL LETTER V */
	return (Py_UNICODE)0x24CB;
    case 0x24E6: /* CIRCLED LATIN SMALL LETTER W */
	return (Py_UNICODE)0x24CC;
    case 0x24E7: /* CIRCLED LATIN SMALL LETTER X */
	return (Py_UNICODE)0x24CD;
    case 0x24E8: /* CIRCLED LATIN SMALL LETTER Y */
	return (Py_UNICODE)0x24CE;
    case 0x24E9: /* CIRCLED LATIN SMALL LETTER Z */
	return (Py_UNICODE)0x24CF;
    case 0xFF41: /* FULLWIDTH LATIN SMALL LETTER A */
	return (Py_UNICODE)0xFF21;
    case 0xFF42: /* FULLWIDTH LATIN SMALL LETTER B */
	return (Py_UNICODE)0xFF22;
    case 0xFF43: /* FULLWIDTH LATIN SMALL LETTER C */
	return (Py_UNICODE)0xFF23;
    case 0xFF44: /* FULLWIDTH LATIN SMALL LETTER D */
	return (Py_UNICODE)0xFF24;
    case 0xFF45: /* FULLWIDTH LATIN SMALL LETTER E */
	return (Py_UNICODE)0xFF25;
    case 0xFF46: /* FULLWIDTH LATIN SMALL LETTER F */
	return (Py_UNICODE)0xFF26;
    case 0xFF47: /* FULLWIDTH LATIN SMALL LETTER G */
	return (Py_UNICODE)0xFF27;
    case 0xFF48: /* FULLWIDTH LATIN SMALL LETTER H */
	return (Py_UNICODE)0xFF28;
    case 0xFF49: /* FULLWIDTH LATIN SMALL LETTER I */
	return (Py_UNICODE)0xFF29;
    case 0xFF4A: /* FULLWIDTH LATIN SMALL LETTER J */
	return (Py_UNICODE)0xFF2A;
    case 0xFF4B: /* FULLWIDTH LATIN SMALL LETTER K */
	return (Py_UNICODE)0xFF2B;
    case 0xFF4C: /* FULLWIDTH LATIN SMALL LETTER L */
	return (Py_UNICODE)0xFF2C;
    case 0xFF4D: /* FULLWIDTH LATIN SMALL LETTER M */
	return (Py_UNICODE)0xFF2D;
    case 0xFF4E: /* FULLWIDTH LATIN SMALL LETTER N */
	return (Py_UNICODE)0xFF2E;
    case 0xFF4F: /* FULLWIDTH LATIN SMALL LETTER O */
	return (Py_UNICODE)0xFF2F;
    case 0xFF50: /* FULLWIDTH LATIN SMALL LETTER P */
	return (Py_UNICODE)0xFF30;
    case 0xFF51: /* FULLWIDTH LATIN SMALL LETTER Q */
	return (Py_UNICODE)0xFF31;
    case 0xFF52: /* FULLWIDTH LATIN SMALL LETTER R */
	return (Py_UNICODE)0xFF32;
    case 0xFF53: /* FULLWIDTH LATIN SMALL LETTER S */
	return (Py_UNICODE)0xFF33;
    case 0xFF54: /* FULLWIDTH LATIN SMALL LETTER T */
	return (Py_UNICODE)0xFF34;
    case 0xFF55: /* FULLWIDTH LATIN SMALL LETTER U */
	return (Py_UNICODE)0xFF35;
    case 0xFF56: /* FULLWIDTH LATIN SMALL LETTER V */
	return (Py_UNICODE)0xFF36;
    case 0xFF57: /* FULLWIDTH LATIN SMALL LETTER W */
	return (Py_UNICODE)0xFF37;
    case 0xFF58: /* FULLWIDTH LATIN SMALL LETTER X */
	return (Py_UNICODE)0xFF38;
    case 0xFF59: /* FULLWIDTH LATIN SMALL LETTER Y */
	return (Py_UNICODE)0xFF39;
    case 0xFF5A: /* FULLWIDTH LATIN SMALL LETTER Z */
	return (Py_UNICODE)0xFF3A;
    default:
	return ch;
    }
}

/* Returns the lowercase Unicode characters corresponding to ch or just
   ch if no lowercase mapping is known. */

Py_UNICODE _PyUnicode_ToLowercase(register const Py_UNICODE ch)
{
    switch (ch) {
    case 0x0041: /* LATIN CAPITAL LETTER A */
	return (Py_UNICODE)0x0061;
    case 0x0042: /* LATIN CAPITAL LETTER B */
	return (Py_UNICODE)0x0062;
    case 0x0043: /* LATIN CAPITAL LETTER C */
	return (Py_UNICODE)0x0063;
    case 0x0044: /* LATIN CAPITAL LETTER D */
	return (Py_UNICODE)0x0064;
    case 0x0045: /* LATIN CAPITAL LETTER E */
	return (Py_UNICODE)0x0065;
    case 0x0046: /* LATIN CAPITAL LETTER F */
	return (Py_UNICODE)0x0066;
    case 0x0047: /* LATIN CAPITAL LETTER G */
	return (Py_UNICODE)0x0067;
    case 0x0048: /* LATIN CAPITAL LETTER H */
	return (Py_UNICODE)0x0068;
    case 0x0049: /* LATIN CAPITAL LETTER I */
	return (Py_UNICODE)0x0069;
    case 0x004A: /* LATIN CAPITAL LETTER J */
	return (Py_UNICODE)0x006A;
    case 0x004B: /* LATIN CAPITAL LETTER K */
	return (Py_UNICODE)0x006B;
    case 0x004C: /* LATIN CAPITAL LETTER L */
	return (Py_UNICODE)0x006C;
    case 0x004D: /* LATIN CAPITAL LETTER M */
	return (Py_UNICODE)0x006D;
    case 0x004E: /* LATIN CAPITAL LETTER N */
	return (Py_UNICODE)0x006E;
    case 0x004F: /* LATIN CAPITAL LETTER O */
	return (Py_UNICODE)0x006F;
    case 0x0050: /* LATIN CAPITAL LETTER P */
	return (Py_UNICODE)0x0070;
    case 0x0051: /* LATIN CAPITAL LETTER Q */
	return (Py_UNICODE)0x0071;
    case 0x0052: /* LATIN CAPITAL LETTER R */
	return (Py_UNICODE)0x0072;
    case 0x0053: /* LATIN CAPITAL LETTER S */
	return (Py_UNICODE)0x0073;
    case 0x0054: /* LATIN CAPITAL LETTER T */
	return (Py_UNICODE)0x0074;
    case 0x0055: /* LATIN CAPITAL LETTER U */
	return (Py_UNICODE)0x0075;
    case 0x0056: /* LATIN CAPITAL LETTER V */
	return (Py_UNICODE)0x0076;
    case 0x0057: /* LATIN CAPITAL LETTER W */
	return (Py_UNICODE)0x0077;
    case 0x0058: /* LATIN CAPITAL LETTER X */
	return (Py_UNICODE)0x0078;
    case 0x0059: /* LATIN CAPITAL LETTER Y */
	return (Py_UNICODE)0x0079;
    case 0x005A: /* LATIN CAPITAL LETTER Z */
	return (Py_UNICODE)0x007A;
    case 0x00C0: /* LATIN CAPITAL LETTER A WITH GRAVE */
	return (Py_UNICODE)0x00E0;
    case 0x00C1: /* LATIN CAPITAL LETTER A WITH ACUTE */
	return (Py_UNICODE)0x00E1;
    case 0x00C2: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00E2;
    case 0x00C3: /* LATIN CAPITAL LETTER A WITH TILDE */
	return (Py_UNICODE)0x00E3;
    case 0x00C4: /* LATIN CAPITAL LETTER A WITH DIAERESIS */
	return (Py_UNICODE)0x00E4;
    case 0x00C5: /* LATIN CAPITAL LETTER A WITH RING ABOVE */
	return (Py_UNICODE)0x00E5;
    case 0x00C6: /* LATIN CAPITAL LETTER AE */
	return (Py_UNICODE)0x00E6;
    case 0x00C7: /* LATIN CAPITAL LETTER C WITH CEDILLA */
	return (Py_UNICODE)0x00E7;
    case 0x00C8: /* LATIN CAPITAL LETTER E WITH GRAVE */
	return (Py_UNICODE)0x00E8;
    case 0x00C9: /* LATIN CAPITAL LETTER E WITH ACUTE */
	return (Py_UNICODE)0x00E9;
    case 0x00CA: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00EA;
    case 0x00CB: /* LATIN CAPITAL LETTER E WITH DIAERESIS */
	return (Py_UNICODE)0x00EB;
    case 0x00CC: /* LATIN CAPITAL LETTER I WITH GRAVE */
	return (Py_UNICODE)0x00EC;
    case 0x00CD: /* LATIN CAPITAL LETTER I WITH ACUTE */
	return (Py_UNICODE)0x00ED;
    case 0x00CE: /* LATIN CAPITAL LETTER I WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00EE;
    case 0x00CF: /* LATIN CAPITAL LETTER I WITH DIAERESIS */
	return (Py_UNICODE)0x00EF;
    case 0x00D0: /* LATIN CAPITAL LETTER ETH */
	return (Py_UNICODE)0x00F0;
    case 0x00D1: /* LATIN CAPITAL LETTER N WITH TILDE */
	return (Py_UNICODE)0x00F1;
    case 0x00D2: /* LATIN CAPITAL LETTER O WITH GRAVE */
	return (Py_UNICODE)0x00F2;
    case 0x00D3: /* LATIN CAPITAL LETTER O WITH ACUTE */
	return (Py_UNICODE)0x00F3;
    case 0x00D4: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00F4;
    case 0x00D5: /* LATIN CAPITAL LETTER O WITH TILDE */
	return (Py_UNICODE)0x00F5;
    case 0x00D6: /* LATIN CAPITAL LETTER O WITH DIAERESIS */
	return (Py_UNICODE)0x00F6;
    case 0x00D8: /* LATIN CAPITAL LETTER O WITH STROKE */
	return (Py_UNICODE)0x00F8;
    case 0x00D9: /* LATIN CAPITAL LETTER U WITH GRAVE */
	return (Py_UNICODE)0x00F9;
    case 0x00DA: /* LATIN CAPITAL LETTER U WITH ACUTE */
	return (Py_UNICODE)0x00FA;
    case 0x00DB: /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX */
	return (Py_UNICODE)0x00FB;
    case 0x00DC: /* LATIN CAPITAL LETTER U WITH DIAERESIS */
	return (Py_UNICODE)0x00FC;
    case 0x00DD: /* LATIN CAPITAL LETTER Y WITH ACUTE */
	return (Py_UNICODE)0x00FD;
    case 0x00DE: /* LATIN CAPITAL LETTER THORN */
	return (Py_UNICODE)0x00FE;
    case 0x0100: /* LATIN CAPITAL LETTER A WITH MACRON */
	return (Py_UNICODE)0x0101;
    case 0x0102: /* LATIN CAPITAL LETTER A WITH BREVE */
	return (Py_UNICODE)0x0103;
    case 0x0104: /* LATIN CAPITAL LETTER A WITH OGONEK */
	return (Py_UNICODE)0x0105;
    case 0x0106: /* LATIN CAPITAL LETTER C WITH ACUTE */
	return (Py_UNICODE)0x0107;
    case 0x0108: /* LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0109;
    case 0x010A: /* LATIN CAPITAL LETTER C WITH DOT ABOVE */
	return (Py_UNICODE)0x010B;
    case 0x010C: /* LATIN CAPITAL LETTER C WITH CARON */
	return (Py_UNICODE)0x010D;
    case 0x010E: /* LATIN CAPITAL LETTER D WITH CARON */
	return (Py_UNICODE)0x010F;
    case 0x0110: /* LATIN CAPITAL LETTER D WITH STROKE */
	return (Py_UNICODE)0x0111;
    case 0x0112: /* LATIN CAPITAL LETTER E WITH MACRON */
	return (Py_UNICODE)0x0113;
    case 0x0114: /* LATIN CAPITAL LETTER E WITH BREVE */
	return (Py_UNICODE)0x0115;
    case 0x0116: /* LATIN CAPITAL LETTER E WITH DOT ABOVE */
	return (Py_UNICODE)0x0117;
    case 0x0118: /* LATIN CAPITAL LETTER E WITH OGONEK */
	return (Py_UNICODE)0x0119;
    case 0x011A: /* LATIN CAPITAL LETTER E WITH CARON */
	return (Py_UNICODE)0x011B;
    case 0x011C: /* LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
	return (Py_UNICODE)0x011D;
    case 0x011E: /* LATIN CAPITAL LETTER G WITH BREVE */
	return (Py_UNICODE)0x011F;
    case 0x0120: /* LATIN CAPITAL LETTER G WITH DOT ABOVE */
	return (Py_UNICODE)0x0121;
    case 0x0122: /* LATIN CAPITAL LETTER G WITH CEDILLA */
	return (Py_UNICODE)0x0123;
    case 0x0124: /* LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0125;
    case 0x0126: /* LATIN CAPITAL LETTER H WITH STROKE */
	return (Py_UNICODE)0x0127;
    case 0x0128: /* LATIN CAPITAL LETTER I WITH TILDE */
	return (Py_UNICODE)0x0129;
    case 0x012A: /* LATIN CAPITAL LETTER I WITH MACRON */
	return (Py_UNICODE)0x012B;
    case 0x012C: /* LATIN CAPITAL LETTER I WITH BREVE */
	return (Py_UNICODE)0x012D;
    case 0x012E: /* LATIN CAPITAL LETTER I WITH OGONEK */
	return (Py_UNICODE)0x012F;
    case 0x0130: /* LATIN CAPITAL LETTER I WITH DOT ABOVE */
	return (Py_UNICODE)0x0069;
    case 0x0132: /* LATIN CAPITAL LIGATURE IJ */
	return (Py_UNICODE)0x0133;
    case 0x0134: /* LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0135;
    case 0x0136: /* LATIN CAPITAL LETTER K WITH CEDILLA */
	return (Py_UNICODE)0x0137;
    case 0x0139: /* LATIN CAPITAL LETTER L WITH ACUTE */
	return (Py_UNICODE)0x013A;
    case 0x013B: /* LATIN CAPITAL LETTER L WITH CEDILLA */
	return (Py_UNICODE)0x013C;
    case 0x013D: /* LATIN CAPITAL LETTER L WITH CARON */
	return (Py_UNICODE)0x013E;
    case 0x013F: /* LATIN CAPITAL LETTER L WITH MIDDLE DOT */
	return (Py_UNICODE)0x0140;
    case 0x0141: /* LATIN CAPITAL LETTER L WITH STROKE */
	return (Py_UNICODE)0x0142;
    case 0x0143: /* LATIN CAPITAL LETTER N WITH ACUTE */
	return (Py_UNICODE)0x0144;
    case 0x0145: /* LATIN CAPITAL LETTER N WITH CEDILLA */
	return (Py_UNICODE)0x0146;
    case 0x0147: /* LATIN CAPITAL LETTER N WITH CARON */
	return (Py_UNICODE)0x0148;
    case 0x014A: /* LATIN CAPITAL LETTER ENG */
	return (Py_UNICODE)0x014B;
    case 0x014C: /* LATIN CAPITAL LETTER O WITH MACRON */
	return (Py_UNICODE)0x014D;
    case 0x014E: /* LATIN CAPITAL LETTER O WITH BREVE */
	return (Py_UNICODE)0x014F;
    case 0x0150: /* LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x0151;
    case 0x0152: /* LATIN CAPITAL LIGATURE OE */
	return (Py_UNICODE)0x0153;
    case 0x0154: /* LATIN CAPITAL LETTER R WITH ACUTE */
	return (Py_UNICODE)0x0155;
    case 0x0156: /* LATIN CAPITAL LETTER R WITH CEDILLA */
	return (Py_UNICODE)0x0157;
    case 0x0158: /* LATIN CAPITAL LETTER R WITH CARON */
	return (Py_UNICODE)0x0159;
    case 0x015A: /* LATIN CAPITAL LETTER S WITH ACUTE */
	return (Py_UNICODE)0x015B;
    case 0x015C: /* LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
	return (Py_UNICODE)0x015D;
    case 0x015E: /* LATIN CAPITAL LETTER S WITH CEDILLA */
	return (Py_UNICODE)0x015F;
    case 0x0160: /* LATIN CAPITAL LETTER S WITH CARON */
	return (Py_UNICODE)0x0161;
    case 0x0162: /* LATIN CAPITAL LETTER T WITH CEDILLA */
	return (Py_UNICODE)0x0163;
    case 0x0164: /* LATIN CAPITAL LETTER T WITH CARON */
	return (Py_UNICODE)0x0165;
    case 0x0166: /* LATIN CAPITAL LETTER T WITH STROKE */
	return (Py_UNICODE)0x0167;
    case 0x0168: /* LATIN CAPITAL LETTER U WITH TILDE */
	return (Py_UNICODE)0x0169;
    case 0x016A: /* LATIN CAPITAL LETTER U WITH MACRON */
	return (Py_UNICODE)0x016B;
    case 0x016C: /* LATIN CAPITAL LETTER U WITH BREVE */
	return (Py_UNICODE)0x016D;
    case 0x016E: /* LATIN CAPITAL LETTER U WITH RING ABOVE */
	return (Py_UNICODE)0x016F;
    case 0x0170: /* LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x0171;
    case 0x0172: /* LATIN CAPITAL LETTER U WITH OGONEK */
	return (Py_UNICODE)0x0173;
    case 0x0174: /* LATIN CAPITAL LETTER W WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0175;
    case 0x0176: /* LATIN CAPITAL LETTER Y WITH CIRCUMFLEX */
	return (Py_UNICODE)0x0177;
    case 0x0178: /* LATIN CAPITAL LETTER Y WITH DIAERESIS */
	return (Py_UNICODE)0x00FF;
    case 0x0179: /* LATIN CAPITAL LETTER Z WITH ACUTE */
	return (Py_UNICODE)0x017A;
    case 0x017B: /* LATIN CAPITAL LETTER Z WITH DOT ABOVE */
	return (Py_UNICODE)0x017C;
    case 0x017D: /* LATIN CAPITAL LETTER Z WITH CARON */
	return (Py_UNICODE)0x017E;
    case 0x0181: /* LATIN CAPITAL LETTER B WITH HOOK */
	return (Py_UNICODE)0x0253;
    case 0x0182: /* LATIN CAPITAL LETTER B WITH TOPBAR */
	return (Py_UNICODE)0x0183;
    case 0x0184: /* LATIN CAPITAL LETTER TONE SIX */
	return (Py_UNICODE)0x0185;
    case 0x0186: /* LATIN CAPITAL LETTER OPEN O */
	return (Py_UNICODE)0x0254;
    case 0x0187: /* LATIN CAPITAL LETTER C WITH HOOK */
	return (Py_UNICODE)0x0188;
    case 0x0189: /* LATIN CAPITAL LETTER AFRICAN D */
	return (Py_UNICODE)0x0256;
    case 0x018A: /* LATIN CAPITAL LETTER D WITH HOOK */
	return (Py_UNICODE)0x0257;
    case 0x018B: /* LATIN CAPITAL LETTER D WITH TOPBAR */
	return (Py_UNICODE)0x018C;
    case 0x018E: /* LATIN CAPITAL LETTER REVERSED E */
	return (Py_UNICODE)0x01DD;
    case 0x018F: /* LATIN CAPITAL LETTER SCHWA */
	return (Py_UNICODE)0x0259;
    case 0x0190: /* LATIN CAPITAL LETTER OPEN E */
	return (Py_UNICODE)0x025B;
    case 0x0191: /* LATIN CAPITAL LETTER F WITH HOOK */
	return (Py_UNICODE)0x0192;
    case 0x0193: /* LATIN CAPITAL LETTER G WITH HOOK */
	return (Py_UNICODE)0x0260;
    case 0x0194: /* LATIN CAPITAL LETTER GAMMA */
	return (Py_UNICODE)0x0263;
    case 0x0196: /* LATIN CAPITAL LETTER IOTA */
	return (Py_UNICODE)0x0269;
    case 0x0197: /* LATIN CAPITAL LETTER I WITH STROKE */
	return (Py_UNICODE)0x0268;
    case 0x0198: /* LATIN CAPITAL LETTER K WITH HOOK */
	return (Py_UNICODE)0x0199;
    case 0x019C: /* LATIN CAPITAL LETTER TURNED M */
	return (Py_UNICODE)0x026F;
    case 0x019D: /* LATIN CAPITAL LETTER N WITH LEFT HOOK */
	return (Py_UNICODE)0x0272;
    case 0x019F: /* LATIN CAPITAL LETTER O WITH MIDDLE TILDE */
	return (Py_UNICODE)0x0275;
    case 0x01A0: /* LATIN CAPITAL LETTER O WITH HORN */
	return (Py_UNICODE)0x01A1;
    case 0x01A2: /* LATIN CAPITAL LETTER OI */
	return (Py_UNICODE)0x01A3;
    case 0x01A4: /* LATIN CAPITAL LETTER P WITH HOOK */
	return (Py_UNICODE)0x01A5;
    case 0x01A6: /* LATIN LETTER YR */
	return (Py_UNICODE)0x0280;
    case 0x01A7: /* LATIN CAPITAL LETTER TONE TWO */
	return (Py_UNICODE)0x01A8;
    case 0x01A9: /* LATIN CAPITAL LETTER ESH */
	return (Py_UNICODE)0x0283;
    case 0x01AC: /* LATIN CAPITAL LETTER T WITH HOOK */
	return (Py_UNICODE)0x01AD;
    case 0x01AE: /* LATIN CAPITAL LETTER T WITH RETROFLEX HOOK */
	return (Py_UNICODE)0x0288;
    case 0x01AF: /* LATIN CAPITAL LETTER U WITH HORN */
	return (Py_UNICODE)0x01B0;
    case 0x01B1: /* LATIN CAPITAL LETTER UPSILON */
	return (Py_UNICODE)0x028A;
    case 0x01B2: /* LATIN CAPITAL LETTER V WITH HOOK */
	return (Py_UNICODE)0x028B;
    case 0x01B3: /* LATIN CAPITAL LETTER Y WITH HOOK */
	return (Py_UNICODE)0x01B4;
    case 0x01B5: /* LATIN CAPITAL LETTER Z WITH STROKE */
	return (Py_UNICODE)0x01B6;
    case 0x01B7: /* LATIN CAPITAL LETTER EZH */
	return (Py_UNICODE)0x0292;
    case 0x01B8: /* LATIN CAPITAL LETTER EZH REVERSED */
	return (Py_UNICODE)0x01B9;
    case 0x01BC: /* LATIN CAPITAL LETTER TONE FIVE */
	return (Py_UNICODE)0x01BD;
    case 0x01C4: /* LATIN CAPITAL LETTER DZ WITH CARON */
	return (Py_UNICODE)0x01C6;
    case 0x01C5: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON */
	return (Py_UNICODE)0x01C6;
    case 0x01C7: /* LATIN CAPITAL LETTER LJ */
	return (Py_UNICODE)0x01C9;
    case 0x01C8: /* LATIN CAPITAL LETTER L WITH SMALL LETTER J */
	return (Py_UNICODE)0x01C9;
    case 0x01CA: /* LATIN CAPITAL LETTER NJ */
	return (Py_UNICODE)0x01CC;
    case 0x01CB: /* LATIN CAPITAL LETTER N WITH SMALL LETTER J */
	return (Py_UNICODE)0x01CC;
    case 0x01CD: /* LATIN CAPITAL LETTER A WITH CARON */
	return (Py_UNICODE)0x01CE;
    case 0x01CF: /* LATIN CAPITAL LETTER I WITH CARON */
	return (Py_UNICODE)0x01D0;
    case 0x01D1: /* LATIN CAPITAL LETTER O WITH CARON */
	return (Py_UNICODE)0x01D2;
    case 0x01D3: /* LATIN CAPITAL LETTER U WITH CARON */
	return (Py_UNICODE)0x01D4;
    case 0x01D5: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x01D6;
    case 0x01D7: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE */
	return (Py_UNICODE)0x01D8;
    case 0x01D9: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND CARON */
	return (Py_UNICODE)0x01DA;
    case 0x01DB: /* LATIN CAPITAL LETTER U WITH DIAERESIS AND GRAVE */
	return (Py_UNICODE)0x01DC;
    case 0x01DE: /* LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x01DF;
    case 0x01E0: /* LATIN CAPITAL LETTER A WITH DOT ABOVE AND MACRON */
	return (Py_UNICODE)0x01E1;
    case 0x01E2: /* LATIN CAPITAL LETTER AE WITH MACRON */
	return (Py_UNICODE)0x01E3;
    case 0x01E4: /* LATIN CAPITAL LETTER G WITH STROKE */
	return (Py_UNICODE)0x01E5;
    case 0x01E6: /* LATIN CAPITAL LETTER G WITH CARON */
	return (Py_UNICODE)0x01E7;
    case 0x01E8: /* LATIN CAPITAL LETTER K WITH CARON */
	return (Py_UNICODE)0x01E9;
    case 0x01EA: /* LATIN CAPITAL LETTER O WITH OGONEK */
	return (Py_UNICODE)0x01EB;
    case 0x01EC: /* LATIN CAPITAL LETTER O WITH OGONEK AND MACRON */
	return (Py_UNICODE)0x01ED;
    case 0x01EE: /* LATIN CAPITAL LETTER EZH WITH CARON */
	return (Py_UNICODE)0x01EF;
    case 0x01F1: /* LATIN CAPITAL LETTER DZ */
	return (Py_UNICODE)0x01F3;
    case 0x01F2: /* LATIN CAPITAL LETTER D WITH SMALL LETTER Z */
	return (Py_UNICODE)0x01F3;
    case 0x01F4: /* LATIN CAPITAL LETTER G WITH ACUTE */
	return (Py_UNICODE)0x01F5;
    case 0x01F6: /* LATIN CAPITAL LETTER HWAIR */
	return (Py_UNICODE)0x0195;
    case 0x01F7: /* LATIN CAPITAL LETTER WYNN */
	return (Py_UNICODE)0x01BF;
    case 0x01F8: /* LATIN CAPITAL LETTER N WITH GRAVE */
	return (Py_UNICODE)0x01F9;
    case 0x01FA: /* LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE */
	return (Py_UNICODE)0x01FB;
    case 0x01FC: /* LATIN CAPITAL LETTER AE WITH ACUTE */
	return (Py_UNICODE)0x01FD;
    case 0x01FE: /* LATIN CAPITAL LETTER O WITH STROKE AND ACUTE */
	return (Py_UNICODE)0x01FF;
    case 0x0200: /* LATIN CAPITAL LETTER A WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0201;
    case 0x0202: /* LATIN CAPITAL LETTER A WITH INVERTED BREVE */
	return (Py_UNICODE)0x0203;
    case 0x0204: /* LATIN CAPITAL LETTER E WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0205;
    case 0x0206: /* LATIN CAPITAL LETTER E WITH INVERTED BREVE */
	return (Py_UNICODE)0x0207;
    case 0x0208: /* LATIN CAPITAL LETTER I WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0209;
    case 0x020A: /* LATIN CAPITAL LETTER I WITH INVERTED BREVE */
	return (Py_UNICODE)0x020B;
    case 0x020C: /* LATIN CAPITAL LETTER O WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x020D;
    case 0x020E: /* LATIN CAPITAL LETTER O WITH INVERTED BREVE */
	return (Py_UNICODE)0x020F;
    case 0x0210: /* LATIN CAPITAL LETTER R WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0211;
    case 0x0212: /* LATIN CAPITAL LETTER R WITH INVERTED BREVE */
	return (Py_UNICODE)0x0213;
    case 0x0214: /* LATIN CAPITAL LETTER U WITH DOUBLE GRAVE */
	return (Py_UNICODE)0x0215;
    case 0x0216: /* LATIN CAPITAL LETTER U WITH INVERTED BREVE */
	return (Py_UNICODE)0x0217;
    case 0x0218: /* LATIN CAPITAL LETTER S WITH COMMA BELOW */
	return (Py_UNICODE)0x0219;
    case 0x021A: /* LATIN CAPITAL LETTER T WITH COMMA BELOW */
	return (Py_UNICODE)0x021B;
    case 0x021C: /* LATIN CAPITAL LETTER YOGH */
	return (Py_UNICODE)0x021D;
    case 0x021E: /* LATIN CAPITAL LETTER H WITH CARON */
	return (Py_UNICODE)0x021F;
    case 0x0222: /* LATIN CAPITAL LETTER OU */
	return (Py_UNICODE)0x0223;
    case 0x0224: /* LATIN CAPITAL LETTER Z WITH HOOK */
	return (Py_UNICODE)0x0225;
    case 0x0226: /* LATIN CAPITAL LETTER A WITH DOT ABOVE */
	return (Py_UNICODE)0x0227;
    case 0x0228: /* LATIN CAPITAL LETTER E WITH CEDILLA */
	return (Py_UNICODE)0x0229;
    case 0x022A: /* LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON */
	return (Py_UNICODE)0x022B;
    case 0x022C: /* LATIN CAPITAL LETTER O WITH TILDE AND MACRON */
	return (Py_UNICODE)0x022D;
    case 0x022E: /* LATIN CAPITAL LETTER O WITH DOT ABOVE */
	return (Py_UNICODE)0x022F;
    case 0x0230: /* LATIN CAPITAL LETTER O WITH DOT ABOVE AND MACRON */
	return (Py_UNICODE)0x0231;
    case 0x0232: /* LATIN CAPITAL LETTER Y WITH MACRON */
	return (Py_UNICODE)0x0233;
    case 0x0386: /* GREEK CAPITAL LETTER ALPHA WITH TONOS */
	return (Py_UNICODE)0x03AC;
    case 0x0388: /* GREEK CAPITAL LETTER EPSILON WITH TONOS */
	return (Py_UNICODE)0x03AD;
    case 0x0389: /* GREEK CAPITAL LETTER ETA WITH TONOS */
	return (Py_UNICODE)0x03AE;
    case 0x038A: /* GREEK CAPITAL LETTER IOTA WITH TONOS */
	return (Py_UNICODE)0x03AF;
    case 0x038C: /* GREEK CAPITAL LETTER OMICRON WITH TONOS */
	return (Py_UNICODE)0x03CC;
    case 0x038E: /* GREEK CAPITAL LETTER UPSILON WITH TONOS */
	return (Py_UNICODE)0x03CD;
    case 0x038F: /* GREEK CAPITAL LETTER OMEGA WITH TONOS */
	return (Py_UNICODE)0x03CE;
    case 0x0391: /* GREEK CAPITAL LETTER ALPHA */
	return (Py_UNICODE)0x03B1;
    case 0x0392: /* GREEK CAPITAL LETTER BETA */
	return (Py_UNICODE)0x03B2;
    case 0x0393: /* GREEK CAPITAL LETTER GAMMA */
	return (Py_UNICODE)0x03B3;
    case 0x0394: /* GREEK CAPITAL LETTER DELTA */
	return (Py_UNICODE)0x03B4;
    case 0x0395: /* GREEK CAPITAL LETTER EPSILON */
	return (Py_UNICODE)0x03B5;
    case 0x0396: /* GREEK CAPITAL LETTER ZETA */
	return (Py_UNICODE)0x03B6;
    case 0x0397: /* GREEK CAPITAL LETTER ETA */
	return (Py_UNICODE)0x03B7;
    case 0x0398: /* GREEK CAPITAL LETTER THETA */
	return (Py_UNICODE)0x03B8;
    case 0x0399: /* GREEK CAPITAL LETTER IOTA */
	return (Py_UNICODE)0x03B9;
    case 0x039A: /* GREEK CAPITAL LETTER KAPPA */
	return (Py_UNICODE)0x03BA;
    case 0x039B: /* GREEK CAPITAL LETTER LAMDA */
	return (Py_UNICODE)0x03BB;
    case 0x039C: /* GREEK CAPITAL LETTER MU */
	return (Py_UNICODE)0x03BC;
    case 0x039D: /* GREEK CAPITAL LETTER NU */
	return (Py_UNICODE)0x03BD;
    case 0x039E: /* GREEK CAPITAL LETTER XI */
	return (Py_UNICODE)0x03BE;
    case 0x039F: /* GREEK CAPITAL LETTER OMICRON */
	return (Py_UNICODE)0x03BF;
    case 0x03A0: /* GREEK CAPITAL LETTER PI */
	return (Py_UNICODE)0x03C0;
    case 0x03A1: /* GREEK CAPITAL LETTER RHO */
	return (Py_UNICODE)0x03C1;
    case 0x03A3: /* GREEK CAPITAL LETTER SIGMA */
	return (Py_UNICODE)0x03C3;
    case 0x03A4: /* GREEK CAPITAL LETTER TAU */
	return (Py_UNICODE)0x03C4;
    case 0x03A5: /* GREEK CAPITAL LETTER UPSILON */
	return (Py_UNICODE)0x03C5;
    case 0x03A6: /* GREEK CAPITAL LETTER PHI */
	return (Py_UNICODE)0x03C6;
    case 0x03A7: /* GREEK CAPITAL LETTER CHI */
	return (Py_UNICODE)0x03C7;
    case 0x03A8: /* GREEK CAPITAL LETTER PSI */
	return (Py_UNICODE)0x03C8;
    case 0x03A9: /* GREEK CAPITAL LETTER OMEGA */
	return (Py_UNICODE)0x03C9;
    case 0x03AA: /* GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
	return (Py_UNICODE)0x03CA;
    case 0x03AB: /* GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
	return (Py_UNICODE)0x03CB;
    case 0x03DA: /* GREEK LETTER STIGMA */
	return (Py_UNICODE)0x03DB;
    case 0x03DC: /* GREEK LETTER DIGAMMA */
	return (Py_UNICODE)0x03DD;
    case 0x03DE: /* GREEK LETTER KOPPA */
	return (Py_UNICODE)0x03DF;
    case 0x03E0: /* GREEK LETTER SAMPI */
	return (Py_UNICODE)0x03E1;
    case 0x03E2: /* COPTIC CAPITAL LETTER SHEI */
	return (Py_UNICODE)0x03E3;
    case 0x03E4: /* COPTIC CAPITAL LETTER FEI */
	return (Py_UNICODE)0x03E5;
    case 0x03E6: /* COPTIC CAPITAL LETTER KHEI */
	return (Py_UNICODE)0x03E7;
    case 0x03E8: /* COPTIC CAPITAL LETTER HORI */
	return (Py_UNICODE)0x03E9;
    case 0x03EA: /* COPTIC CAPITAL LETTER GANGIA */
	return (Py_UNICODE)0x03EB;
    case 0x03EC: /* COPTIC CAPITAL LETTER SHIMA */
	return (Py_UNICODE)0x03ED;
    case 0x03EE: /* COPTIC CAPITAL LETTER DEI */
	return (Py_UNICODE)0x03EF;
    case 0x0400: /* CYRILLIC CAPITAL LETTER IE WITH GRAVE */
	return (Py_UNICODE)0x0450;
    case 0x0401: /* CYRILLIC CAPITAL LETTER IO */
	return (Py_UNICODE)0x0451;
    case 0x0402: /* CYRILLIC CAPITAL LETTER DJE */
	return (Py_UNICODE)0x0452;
    case 0x0403: /* CYRILLIC CAPITAL LETTER GJE */
	return (Py_UNICODE)0x0453;
    case 0x0404: /* CYRILLIC CAPITAL LETTER UKRAINIAN IE */
	return (Py_UNICODE)0x0454;
    case 0x0405: /* CYRILLIC CAPITAL LETTER DZE */
	return (Py_UNICODE)0x0455;
    case 0x0406: /* CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
	return (Py_UNICODE)0x0456;
    case 0x0407: /* CYRILLIC CAPITAL LETTER YI */
	return (Py_UNICODE)0x0457;
    case 0x0408: /* CYRILLIC CAPITAL LETTER JE */
	return (Py_UNICODE)0x0458;
    case 0x0409: /* CYRILLIC CAPITAL LETTER LJE */
	return (Py_UNICODE)0x0459;
    case 0x040A: /* CYRILLIC CAPITAL LETTER NJE */
	return (Py_UNICODE)0x045A;
    case 0x040B: /* CYRILLIC CAPITAL LETTER TSHE */
	return (Py_UNICODE)0x045B;
    case 0x040C: /* CYRILLIC CAPITAL LETTER KJE */
	return (Py_UNICODE)0x045C;
    case 0x040D: /* CYRILLIC CAPITAL LETTER I WITH GRAVE */
	return (Py_UNICODE)0x045D;
    case 0x040E: /* CYRILLIC CAPITAL LETTER SHORT U */
	return (Py_UNICODE)0x045E;
    case 0x040F: /* CYRILLIC CAPITAL LETTER DZHE */
	return (Py_UNICODE)0x045F;
    case 0x0410: /* CYRILLIC CAPITAL LETTER A */
	return (Py_UNICODE)0x0430;
    case 0x0411: /* CYRILLIC CAPITAL LETTER BE */
	return (Py_UNICODE)0x0431;
    case 0x0412: /* CYRILLIC CAPITAL LETTER VE */
	return (Py_UNICODE)0x0432;
    case 0x0413: /* CYRILLIC CAPITAL LETTER GHE */
	return (Py_UNICODE)0x0433;
    case 0x0414: /* CYRILLIC CAPITAL LETTER DE */
	return (Py_UNICODE)0x0434;
    case 0x0415: /* CYRILLIC CAPITAL LETTER IE */
	return (Py_UNICODE)0x0435;
    case 0x0416: /* CYRILLIC CAPITAL LETTER ZHE */
	return (Py_UNICODE)0x0436;
    case 0x0417: /* CYRILLIC CAPITAL LETTER ZE */
	return (Py_UNICODE)0x0437;
    case 0x0418: /* CYRILLIC CAPITAL LETTER I */
	return (Py_UNICODE)0x0438;
    case 0x0419: /* CYRILLIC CAPITAL LETTER SHORT I */
	return (Py_UNICODE)0x0439;
    case 0x041A: /* CYRILLIC CAPITAL LETTER KA */
	return (Py_UNICODE)0x043A;
    case 0x041B: /* CYRILLIC CAPITAL LETTER EL */
	return (Py_UNICODE)0x043B;
    case 0x041C: /* CYRILLIC CAPITAL LETTER EM */
	return (Py_UNICODE)0x043C;
    case 0x041D: /* CYRILLIC CAPITAL LETTER EN */
	return (Py_UNICODE)0x043D;
    case 0x041E: /* CYRILLIC CAPITAL LETTER O */
	return (Py_UNICODE)0x043E;
    case 0x041F: /* CYRILLIC CAPITAL LETTER PE */
	return (Py_UNICODE)0x043F;
    case 0x0420: /* CYRILLIC CAPITAL LETTER ER */
	return (Py_UNICODE)0x0440;
    case 0x0421: /* CYRILLIC CAPITAL LETTER ES */
	return (Py_UNICODE)0x0441;
    case 0x0422: /* CYRILLIC CAPITAL LETTER TE */
	return (Py_UNICODE)0x0442;
    case 0x0423: /* CYRILLIC CAPITAL LETTER U */
	return (Py_UNICODE)0x0443;
    case 0x0424: /* CYRILLIC CAPITAL LETTER EF */
	return (Py_UNICODE)0x0444;
    case 0x0425: /* CYRILLIC CAPITAL LETTER HA */
	return (Py_UNICODE)0x0445;
    case 0x0426: /* CYRILLIC CAPITAL LETTER TSE */
	return (Py_UNICODE)0x0446;
    case 0x0427: /* CYRILLIC CAPITAL LETTER CHE */
	return (Py_UNICODE)0x0447;
    case 0x0428: /* CYRILLIC CAPITAL LETTER SHA */
	return (Py_UNICODE)0x0448;
    case 0x0429: /* CYRILLIC CAPITAL LETTER SHCHA */
	return (Py_UNICODE)0x0449;
    case 0x042A: /* CYRILLIC CAPITAL LETTER HARD SIGN */
	return (Py_UNICODE)0x044A;
    case 0x042B: /* CYRILLIC CAPITAL LETTER YERU */
	return (Py_UNICODE)0x044B;
    case 0x042C: /* CYRILLIC CAPITAL LETTER SOFT SIGN */
	return (Py_UNICODE)0x044C;
    case 0x042D: /* CYRILLIC CAPITAL LETTER E */
	return (Py_UNICODE)0x044D;
    case 0x042E: /* CYRILLIC CAPITAL LETTER YU */
	return (Py_UNICODE)0x044E;
    case 0x042F: /* CYRILLIC CAPITAL LETTER YA */
	return (Py_UNICODE)0x044F;
    case 0x0460: /* CYRILLIC CAPITAL LETTER OMEGA */
	return (Py_UNICODE)0x0461;
    case 0x0462: /* CYRILLIC CAPITAL LETTER YAT */
	return (Py_UNICODE)0x0463;
    case 0x0464: /* CYRILLIC CAPITAL LETTER IOTIFIED E */
	return (Py_UNICODE)0x0465;
    case 0x0466: /* CYRILLIC CAPITAL LETTER LITTLE YUS */
	return (Py_UNICODE)0x0467;
    case 0x0468: /* CYRILLIC CAPITAL LETTER IOTIFIED LITTLE YUS */
	return (Py_UNICODE)0x0469;
    case 0x046A: /* CYRILLIC CAPITAL LETTER BIG YUS */
	return (Py_UNICODE)0x046B;
    case 0x046C: /* CYRILLIC CAPITAL LETTER IOTIFIED BIG YUS */
	return (Py_UNICODE)0x046D;
    case 0x046E: /* CYRILLIC CAPITAL LETTER KSI */
	return (Py_UNICODE)0x046F;
    case 0x0470: /* CYRILLIC CAPITAL LETTER PSI */
	return (Py_UNICODE)0x0471;
    case 0x0472: /* CYRILLIC CAPITAL LETTER FITA */
	return (Py_UNICODE)0x0473;
    case 0x0474: /* CYRILLIC CAPITAL LETTER IZHITSA */
	return (Py_UNICODE)0x0475;
    case 0x0476: /* CYRILLIC CAPITAL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT */
	return (Py_UNICODE)0x0477;
    case 0x0478: /* CYRILLIC CAPITAL LETTER UK */
	return (Py_UNICODE)0x0479;
    case 0x047A: /* CYRILLIC CAPITAL LETTER ROUND OMEGA */
	return (Py_UNICODE)0x047B;
    case 0x047C: /* CYRILLIC CAPITAL LETTER OMEGA WITH TITLO */
	return (Py_UNICODE)0x047D;
    case 0x047E: /* CYRILLIC CAPITAL LETTER OT */
	return (Py_UNICODE)0x047F;
    case 0x0480: /* CYRILLIC CAPITAL LETTER KOPPA */
	return (Py_UNICODE)0x0481;
    case 0x048C: /* CYRILLIC CAPITAL LETTER SEMISOFT SIGN */
	return (Py_UNICODE)0x048D;
    case 0x048E: /* CYRILLIC CAPITAL LETTER ER WITH TICK */
	return (Py_UNICODE)0x048F;
    case 0x0490: /* CYRILLIC CAPITAL LETTER GHE WITH UPTURN */
	return (Py_UNICODE)0x0491;
    case 0x0492: /* CYRILLIC CAPITAL LETTER GHE WITH STROKE */
	return (Py_UNICODE)0x0493;
    case 0x0494: /* CYRILLIC CAPITAL LETTER GHE WITH MIDDLE HOOK */
	return (Py_UNICODE)0x0495;
    case 0x0496: /* CYRILLIC CAPITAL LETTER ZHE WITH DESCENDER */
	return (Py_UNICODE)0x0497;
    case 0x0498: /* CYRILLIC CAPITAL LETTER ZE WITH DESCENDER */
	return (Py_UNICODE)0x0499;
    case 0x049A: /* CYRILLIC CAPITAL LETTER KA WITH DESCENDER */
	return (Py_UNICODE)0x049B;
    case 0x049C: /* CYRILLIC CAPITAL LETTER KA WITH VERTICAL STROKE */
	return (Py_UNICODE)0x049D;
    case 0x049E: /* CYRILLIC CAPITAL LETTER KA WITH STROKE */
	return (Py_UNICODE)0x049F;
    case 0x04A0: /* CYRILLIC CAPITAL LETTER BASHKIR KA */
	return (Py_UNICODE)0x04A1;
    case 0x04A2: /* CYRILLIC CAPITAL LETTER EN WITH DESCENDER */
	return (Py_UNICODE)0x04A3;
    case 0x04A4: /* CYRILLIC CAPITAL LIGATURE EN GHE */
	return (Py_UNICODE)0x04A5;
    case 0x04A6: /* CYRILLIC CAPITAL LETTER PE WITH MIDDLE HOOK */
	return (Py_UNICODE)0x04A7;
    case 0x04A8: /* CYRILLIC CAPITAL LETTER ABKHASIAN HA */
	return (Py_UNICODE)0x04A9;
    case 0x04AA: /* CYRILLIC CAPITAL LETTER ES WITH DESCENDER */
	return (Py_UNICODE)0x04AB;
    case 0x04AC: /* CYRILLIC CAPITAL LETTER TE WITH DESCENDER */
	return (Py_UNICODE)0x04AD;
    case 0x04AE: /* CYRILLIC CAPITAL LETTER STRAIGHT U */
	return (Py_UNICODE)0x04AF;
    case 0x04B0: /* CYRILLIC CAPITAL LETTER STRAIGHT U WITH STROKE */
	return (Py_UNICODE)0x04B1;
    case 0x04B2: /* CYRILLIC CAPITAL LETTER HA WITH DESCENDER */
	return (Py_UNICODE)0x04B3;
    case 0x04B4: /* CYRILLIC CAPITAL LIGATURE TE TSE */
	return (Py_UNICODE)0x04B5;
    case 0x04B6: /* CYRILLIC CAPITAL LETTER CHE WITH DESCENDER */
	return (Py_UNICODE)0x04B7;
    case 0x04B8: /* CYRILLIC CAPITAL LETTER CHE WITH VERTICAL STROKE */
	return (Py_UNICODE)0x04B9;
    case 0x04BA: /* CYRILLIC CAPITAL LETTER SHHA */
	return (Py_UNICODE)0x04BB;
    case 0x04BC: /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE */
	return (Py_UNICODE)0x04BD;
    case 0x04BE: /* CYRILLIC CAPITAL LETTER ABKHASIAN CHE WITH DESCENDER */
	return (Py_UNICODE)0x04BF;
    case 0x04C1: /* CYRILLIC CAPITAL LETTER ZHE WITH BREVE */
	return (Py_UNICODE)0x04C2;
    case 0x04C3: /* CYRILLIC CAPITAL LETTER KA WITH HOOK */
	return (Py_UNICODE)0x04C4;
    case 0x04C7: /* CYRILLIC CAPITAL LETTER EN WITH HOOK */
	return (Py_UNICODE)0x04C8;
    case 0x04CB: /* CYRILLIC CAPITAL LETTER KHAKASSIAN CHE */
	return (Py_UNICODE)0x04CC;
    case 0x04D0: /* CYRILLIC CAPITAL LETTER A WITH BREVE */
	return (Py_UNICODE)0x04D1;
    case 0x04D2: /* CYRILLIC CAPITAL LETTER A WITH DIAERESIS */
	return (Py_UNICODE)0x04D3;
    case 0x04D4: /* CYRILLIC CAPITAL LIGATURE A IE */
	return (Py_UNICODE)0x04D5;
    case 0x04D6: /* CYRILLIC CAPITAL LETTER IE WITH BREVE */
	return (Py_UNICODE)0x04D7;
    case 0x04D8: /* CYRILLIC CAPITAL LETTER SCHWA */
	return (Py_UNICODE)0x04D9;
    case 0x04DA: /* CYRILLIC CAPITAL LETTER SCHWA WITH DIAERESIS */
	return (Py_UNICODE)0x04DB;
    case 0x04DC: /* CYRILLIC CAPITAL LETTER ZHE WITH DIAERESIS */
	return (Py_UNICODE)0x04DD;
    case 0x04DE: /* CYRILLIC CAPITAL LETTER ZE WITH DIAERESIS */
	return (Py_UNICODE)0x04DF;
    case 0x04E0: /* CYRILLIC CAPITAL LETTER ABKHASIAN DZE */
	return (Py_UNICODE)0x04E1;
    case 0x04E2: /* CYRILLIC CAPITAL LETTER I WITH MACRON */
	return (Py_UNICODE)0x04E3;
    case 0x04E4: /* CYRILLIC CAPITAL LETTER I WITH DIAERESIS */
	return (Py_UNICODE)0x04E5;
    case 0x04E6: /* CYRILLIC CAPITAL LETTER O WITH DIAERESIS */
	return (Py_UNICODE)0x04E7;
    case 0x04E8: /* CYRILLIC CAPITAL LETTER BARRED O */
	return (Py_UNICODE)0x04E9;
    case 0x04EA: /* CYRILLIC CAPITAL LETTER BARRED O WITH DIAERESIS */
	return (Py_UNICODE)0x04EB;
    case 0x04EC: /* CYRILLIC CAPITAL LETTER E WITH DIAERESIS */
	return (Py_UNICODE)0x04ED;
    case 0x04EE: /* CYRILLIC CAPITAL LETTER U WITH MACRON */
	return (Py_UNICODE)0x04EF;
    case 0x04F0: /* CYRILLIC CAPITAL LETTER U WITH DIAERESIS */
	return (Py_UNICODE)0x04F1;
    case 0x04F2: /* CYRILLIC CAPITAL LETTER U WITH DOUBLE ACUTE */
	return (Py_UNICODE)0x04F3;
    case 0x04F4: /* CYRILLIC CAPITAL LETTER CHE WITH DIAERESIS */
	return (Py_UNICODE)0x04F5;
    case 0x04F8: /* CYRILLIC CAPITAL LETTER YERU WITH DIAERESIS */
	return (Py_UNICODE)0x04F9;
    case 0x0531: /* ARMENIAN CAPITAL LETTER AYB */
	return (Py_UNICODE)0x0561;
    case 0x0532: /* ARMENIAN CAPITAL LETTER BEN */
	return (Py_UNICODE)0x0562;
    case 0x0533: /* ARMENIAN CAPITAL LETTER GIM */
	return (Py_UNICODE)0x0563;
    case 0x0534: /* ARMENIAN CAPITAL LETTER DA */
	return (Py_UNICODE)0x0564;
    case 0x0535: /* ARMENIAN CAPITAL LETTER ECH */
	return (Py_UNICODE)0x0565;
    case 0x0536: /* ARMENIAN CAPITAL LETTER ZA */
	return (Py_UNICODE)0x0566;
    case 0x0537: /* ARMENIAN CAPITAL LETTER EH */
	return (Py_UNICODE)0x0567;
    case 0x0538: /* ARMENIAN CAPITAL LETTER ET */
	return (Py_UNICODE)0x0568;
    case 0x0539: /* ARMENIAN CAPITAL LETTER TO */
	return (Py_UNICODE)0x0569;
    case 0x053A: /* ARMENIAN CAPITAL LETTER ZHE */
	return (Py_UNICODE)0x056A;
    case 0x053B: /* ARMENIAN CAPITAL LETTER INI */
	return (Py_UNICODE)0x056B;
    case 0x053C: /* ARMENIAN CAPITAL LETTER LIWN */
	return (Py_UNICODE)0x056C;
    case 0x053D: /* ARMENIAN CAPITAL LETTER XEH */
	return (Py_UNICODE)0x056D;
    case 0x053E: /* ARMENIAN CAPITAL LETTER CA */
	return (Py_UNICODE)0x056E;
    case 0x053F: /* ARMENIAN CAPITAL LETTER KEN */
	return (Py_UNICODE)0x056F;
    case 0x0540: /* ARMENIAN CAPITAL LETTER HO */
	return (Py_UNICODE)0x0570;
    case 0x0541: /* ARMENIAN CAPITAL LETTER JA */
	return (Py_UNICODE)0x0571;
    case 0x0542: /* ARMENIAN CAPITAL LETTER GHAD */
	return (Py_UNICODE)0x0572;
    case 0x0543: /* ARMENIAN CAPITAL LETTER CHEH */
	return (Py_UNICODE)0x0573;
    case 0x0544: /* ARMENIAN CAPITAL LETTER MEN */
	return (Py_UNICODE)0x0574;
    case 0x0545: /* ARMENIAN CAPITAL LETTER YI */
	return (Py_UNICODE)0x0575;
    case 0x0546: /* ARMENIAN CAPITAL LETTER NOW */
	return (Py_UNICODE)0x0576;
    case 0x0547: /* ARMENIAN CAPITAL LETTER SHA */
	return (Py_UNICODE)0x0577;
    case 0x0548: /* ARMENIAN CAPITAL LETTER VO */
	return (Py_UNICODE)0x0578;
    case 0x0549: /* ARMENIAN CAPITAL LETTER CHA */
	return (Py_UNICODE)0x0579;
    case 0x054A: /* ARMENIAN CAPITAL LETTER PEH */
	return (Py_UNICODE)0x057A;
    case 0x054B: /* ARMENIAN CAPITAL LETTER JHEH */
	return (Py_UNICODE)0x057B;
    case 0x054C: /* ARMENIAN CAPITAL LETTER RA */
	return (Py_UNICODE)0x057C;
    case 0x054D: /* ARMENIAN CAPITAL LETTER SEH */
	return (Py_UNICODE)0x057D;
    case 0x054E: /* ARMENIAN CAPITAL LETTER VEW */
	return (Py_UNICODE)0x057E;
    case 0x054F: /* ARMENIAN CAPITAL LETTER TIWN */
	return (Py_UNICODE)0x057F;
    case 0x0550: /* ARMENIAN CAPITAL LETTER REH */
	return (Py_UNICODE)0x0580;
    case 0x0551: /* ARMENIAN CAPITAL LETTER CO */
	return (Py_UNICODE)0x0581;
    case 0x0552: /* ARMENIAN CAPITAL LETTER YIWN */
	return (Py_UNICODE)0x0582;
    case 0x0553: /* ARMENIAN CAPITAL LETTER PIWR */
	return (Py_UNICODE)0x0583;
    case 0x0554: /* ARMENIAN CAPITAL LETTER KEH */
	return (Py_UNICODE)0x0584;
    case 0x0555: /* ARMENIAN CAPITAL LETTER OH */
	return (Py_UNICODE)0x0585;
    case 0x0556: /* ARMENIAN CAPITAL LETTER FEH */
	return (Py_UNICODE)0x0586;
    case 0x1E00: /* LATIN CAPITAL LETTER A WITH RING BELOW */
	return (Py_UNICODE)0x1E01;
    case 0x1E02: /* LATIN CAPITAL LETTER B WITH DOT ABOVE */
	return (Py_UNICODE)0x1E03;
    case 0x1E04: /* LATIN CAPITAL LETTER B WITH DOT BELOW */
	return (Py_UNICODE)0x1E05;
    case 0x1E06: /* LATIN CAPITAL LETTER B WITH LINE BELOW */
	return (Py_UNICODE)0x1E07;
    case 0x1E08: /* LATIN CAPITAL LETTER C WITH CEDILLA AND ACUTE */
	return (Py_UNICODE)0x1E09;
    case 0x1E0A: /* LATIN CAPITAL LETTER D WITH DOT ABOVE */
	return (Py_UNICODE)0x1E0B;
    case 0x1E0C: /* LATIN CAPITAL LETTER D WITH DOT BELOW */
	return (Py_UNICODE)0x1E0D;
    case 0x1E0E: /* LATIN CAPITAL LETTER D WITH LINE BELOW */
	return (Py_UNICODE)0x1E0F;
    case 0x1E10: /* LATIN CAPITAL LETTER D WITH CEDILLA */
	return (Py_UNICODE)0x1E11;
    case 0x1E12: /* LATIN CAPITAL LETTER D WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E13;
    case 0x1E14: /* LATIN CAPITAL LETTER E WITH MACRON AND GRAVE */
	return (Py_UNICODE)0x1E15;
    case 0x1E16: /* LATIN CAPITAL LETTER E WITH MACRON AND ACUTE */
	return (Py_UNICODE)0x1E17;
    case 0x1E18: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E19;
    case 0x1E1A: /* LATIN CAPITAL LETTER E WITH TILDE BELOW */
	return (Py_UNICODE)0x1E1B;
    case 0x1E1C: /* LATIN CAPITAL LETTER E WITH CEDILLA AND BREVE */
	return (Py_UNICODE)0x1E1D;
    case 0x1E1E: /* LATIN CAPITAL LETTER F WITH DOT ABOVE */
	return (Py_UNICODE)0x1E1F;
    case 0x1E20: /* LATIN CAPITAL LETTER G WITH MACRON */
	return (Py_UNICODE)0x1E21;
    case 0x1E22: /* LATIN CAPITAL LETTER H WITH DOT ABOVE */
	return (Py_UNICODE)0x1E23;
    case 0x1E24: /* LATIN CAPITAL LETTER H WITH DOT BELOW */
	return (Py_UNICODE)0x1E25;
    case 0x1E26: /* LATIN CAPITAL LETTER H WITH DIAERESIS */
	return (Py_UNICODE)0x1E27;
    case 0x1E28: /* LATIN CAPITAL LETTER H WITH CEDILLA */
	return (Py_UNICODE)0x1E29;
    case 0x1E2A: /* LATIN CAPITAL LETTER H WITH BREVE BELOW */
	return (Py_UNICODE)0x1E2B;
    case 0x1E2C: /* LATIN CAPITAL LETTER I WITH TILDE BELOW */
	return (Py_UNICODE)0x1E2D;
    case 0x1E2E: /* LATIN CAPITAL LETTER I WITH DIAERESIS AND ACUTE */
	return (Py_UNICODE)0x1E2F;
    case 0x1E30: /* LATIN CAPITAL LETTER K WITH ACUTE */
	return (Py_UNICODE)0x1E31;
    case 0x1E32: /* LATIN CAPITAL LETTER K WITH DOT BELOW */
	return (Py_UNICODE)0x1E33;
    case 0x1E34: /* LATIN CAPITAL LETTER K WITH LINE BELOW */
	return (Py_UNICODE)0x1E35;
    case 0x1E36: /* LATIN CAPITAL LETTER L WITH DOT BELOW */
	return (Py_UNICODE)0x1E37;
    case 0x1E38: /* LATIN CAPITAL LETTER L WITH DOT BELOW AND MACRON */
	return (Py_UNICODE)0x1E39;
    case 0x1E3A: /* LATIN CAPITAL LETTER L WITH LINE BELOW */
	return (Py_UNICODE)0x1E3B;
    case 0x1E3C: /* LATIN CAPITAL LETTER L WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E3D;
    case 0x1E3E: /* LATIN CAPITAL LETTER M WITH ACUTE */
	return (Py_UNICODE)0x1E3F;
    case 0x1E40: /* LATIN CAPITAL LETTER M WITH DOT ABOVE */
	return (Py_UNICODE)0x1E41;
    case 0x1E42: /* LATIN CAPITAL LETTER M WITH DOT BELOW */
	return (Py_UNICODE)0x1E43;
    case 0x1E44: /* LATIN CAPITAL LETTER N WITH DOT ABOVE */
	return (Py_UNICODE)0x1E45;
    case 0x1E46: /* LATIN CAPITAL LETTER N WITH DOT BELOW */
	return (Py_UNICODE)0x1E47;
    case 0x1E48: /* LATIN CAPITAL LETTER N WITH LINE BELOW */
	return (Py_UNICODE)0x1E49;
    case 0x1E4A: /* LATIN CAPITAL LETTER N WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E4B;
    case 0x1E4C: /* LATIN CAPITAL LETTER O WITH TILDE AND ACUTE */
	return (Py_UNICODE)0x1E4D;
    case 0x1E4E: /* LATIN CAPITAL LETTER O WITH TILDE AND DIAERESIS */
	return (Py_UNICODE)0x1E4F;
    case 0x1E50: /* LATIN CAPITAL LETTER O WITH MACRON AND GRAVE */
	return (Py_UNICODE)0x1E51;
    case 0x1E52: /* LATIN CAPITAL LETTER O WITH MACRON AND ACUTE */
	return (Py_UNICODE)0x1E53;
    case 0x1E54: /* LATIN CAPITAL LETTER P WITH ACUTE */
	return (Py_UNICODE)0x1E55;
    case 0x1E56: /* LATIN CAPITAL LETTER P WITH DOT ABOVE */
	return (Py_UNICODE)0x1E57;
    case 0x1E58: /* LATIN CAPITAL LETTER R WITH DOT ABOVE */
	return (Py_UNICODE)0x1E59;
    case 0x1E5A: /* LATIN CAPITAL LETTER R WITH DOT BELOW */
	return (Py_UNICODE)0x1E5B;
    case 0x1E5C: /* LATIN CAPITAL LETTER R WITH DOT BELOW AND MACRON */
	return (Py_UNICODE)0x1E5D;
    case 0x1E5E: /* LATIN CAPITAL LETTER R WITH LINE BELOW */
	return (Py_UNICODE)0x1E5F;
    case 0x1E60: /* LATIN CAPITAL LETTER S WITH DOT ABOVE */
	return (Py_UNICODE)0x1E61;
    case 0x1E62: /* LATIN CAPITAL LETTER S WITH DOT BELOW */
	return (Py_UNICODE)0x1E63;
    case 0x1E64: /* LATIN CAPITAL LETTER S WITH ACUTE AND DOT ABOVE */
	return (Py_UNICODE)0x1E65;
    case 0x1E66: /* LATIN CAPITAL LETTER S WITH CARON AND DOT ABOVE */
	return (Py_UNICODE)0x1E67;
    case 0x1E68: /* LATIN CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE */
	return (Py_UNICODE)0x1E69;
    case 0x1E6A: /* LATIN CAPITAL LETTER T WITH DOT ABOVE */
	return (Py_UNICODE)0x1E6B;
    case 0x1E6C: /* LATIN CAPITAL LETTER T WITH DOT BELOW */
	return (Py_UNICODE)0x1E6D;
    case 0x1E6E: /* LATIN CAPITAL LETTER T WITH LINE BELOW */
	return (Py_UNICODE)0x1E6F;
    case 0x1E70: /* LATIN CAPITAL LETTER T WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E71;
    case 0x1E72: /* LATIN CAPITAL LETTER U WITH DIAERESIS BELOW */
	return (Py_UNICODE)0x1E73;
    case 0x1E74: /* LATIN CAPITAL LETTER U WITH TILDE BELOW */
	return (Py_UNICODE)0x1E75;
    case 0x1E76: /* LATIN CAPITAL LETTER U WITH CIRCUMFLEX BELOW */
	return (Py_UNICODE)0x1E77;
    case 0x1E78: /* LATIN CAPITAL LETTER U WITH TILDE AND ACUTE */
	return (Py_UNICODE)0x1E79;
    case 0x1E7A: /* LATIN CAPITAL LETTER U WITH MACRON AND DIAERESIS */
	return (Py_UNICODE)0x1E7B;
    case 0x1E7C: /* LATIN CAPITAL LETTER V WITH TILDE */
	return (Py_UNICODE)0x1E7D;
    case 0x1E7E: /* LATIN CAPITAL LETTER V WITH DOT BELOW */
	return (Py_UNICODE)0x1E7F;
    case 0x1E80: /* LATIN CAPITAL LETTER W WITH GRAVE */
	return (Py_UNICODE)0x1E81;
    case 0x1E82: /* LATIN CAPITAL LETTER W WITH ACUTE */
	return (Py_UNICODE)0x1E83;
    case 0x1E84: /* LATIN CAPITAL LETTER W WITH DIAERESIS */
	return (Py_UNICODE)0x1E85;
    case 0x1E86: /* LATIN CAPITAL LETTER W WITH DOT ABOVE */
	return (Py_UNICODE)0x1E87;
    case 0x1E88: /* LATIN CAPITAL LETTER W WITH DOT BELOW */
	return (Py_UNICODE)0x1E89;
    case 0x1E8A: /* LATIN CAPITAL LETTER X WITH DOT ABOVE */
	return (Py_UNICODE)0x1E8B;
    case 0x1E8C: /* LATIN CAPITAL LETTER X WITH DIAERESIS */
	return (Py_UNICODE)0x1E8D;
    case 0x1E8E: /* LATIN CAPITAL LETTER Y WITH DOT ABOVE */
	return (Py_UNICODE)0x1E8F;
    case 0x1E90: /* LATIN CAPITAL LETTER Z WITH CIRCUMFLEX */
	return (Py_UNICODE)0x1E91;
    case 0x1E92: /* LATIN CAPITAL LETTER Z WITH DOT BELOW */
	return (Py_UNICODE)0x1E93;
    case 0x1E94: /* LATIN CAPITAL LETTER Z WITH LINE BELOW */
	return (Py_UNICODE)0x1E95;
    case 0x1EA0: /* LATIN CAPITAL LETTER A WITH DOT BELOW */
	return (Py_UNICODE)0x1EA1;
    case 0x1EA2: /* LATIN CAPITAL LETTER A WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EA3;
    case 0x1EA4: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1EA5;
    case 0x1EA6: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1EA7;
    case 0x1EA8: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1EA9;
    case 0x1EAA: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1EAB;
    case 0x1EAC: /* LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1EAD;
    case 0x1EAE: /* LATIN CAPITAL LETTER A WITH BREVE AND ACUTE */
	return (Py_UNICODE)0x1EAF;
    case 0x1EB0: /* LATIN CAPITAL LETTER A WITH BREVE AND GRAVE */
	return (Py_UNICODE)0x1EB1;
    case 0x1EB2: /* LATIN CAPITAL LETTER A WITH BREVE AND HOOK ABOVE */
	return (Py_UNICODE)0x1EB3;
    case 0x1EB4: /* LATIN CAPITAL LETTER A WITH BREVE AND TILDE */
	return (Py_UNICODE)0x1EB5;
    case 0x1EB6: /* LATIN CAPITAL LETTER A WITH BREVE AND DOT BELOW */
	return (Py_UNICODE)0x1EB7;
    case 0x1EB8: /* LATIN CAPITAL LETTER E WITH DOT BELOW */
	return (Py_UNICODE)0x1EB9;
    case 0x1EBA: /* LATIN CAPITAL LETTER E WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EBB;
    case 0x1EBC: /* LATIN CAPITAL LETTER E WITH TILDE */
	return (Py_UNICODE)0x1EBD;
    case 0x1EBE: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1EBF;
    case 0x1EC0: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1EC1;
    case 0x1EC2: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1EC3;
    case 0x1EC4: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1EC5;
    case 0x1EC6: /* LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1EC7;
    case 0x1EC8: /* LATIN CAPITAL LETTER I WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EC9;
    case 0x1ECA: /* LATIN CAPITAL LETTER I WITH DOT BELOW */
	return (Py_UNICODE)0x1ECB;
    case 0x1ECC: /* LATIN CAPITAL LETTER O WITH DOT BELOW */
	return (Py_UNICODE)0x1ECD;
    case 0x1ECE: /* LATIN CAPITAL LETTER O WITH HOOK ABOVE */
	return (Py_UNICODE)0x1ECF;
    case 0x1ED0: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE */
	return (Py_UNICODE)0x1ED1;
    case 0x1ED2: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE */
	return (Py_UNICODE)0x1ED3;
    case 0x1ED4: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE */
	return (Py_UNICODE)0x1ED5;
    case 0x1ED6: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE */
	return (Py_UNICODE)0x1ED7;
    case 0x1ED8: /* LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW */
	return (Py_UNICODE)0x1ED9;
    case 0x1EDA: /* LATIN CAPITAL LETTER O WITH HORN AND ACUTE */
	return (Py_UNICODE)0x1EDB;
    case 0x1EDC: /* LATIN CAPITAL LETTER O WITH HORN AND GRAVE */
	return (Py_UNICODE)0x1EDD;
    case 0x1EDE: /* LATIN CAPITAL LETTER O WITH HORN AND HOOK ABOVE */
	return (Py_UNICODE)0x1EDF;
    case 0x1EE0: /* LATIN CAPITAL LETTER O WITH HORN AND TILDE */
	return (Py_UNICODE)0x1EE1;
    case 0x1EE2: /* LATIN CAPITAL LETTER O WITH HORN AND DOT BELOW */
	return (Py_UNICODE)0x1EE3;
    case 0x1EE4: /* LATIN CAPITAL LETTER U WITH DOT BELOW */
	return (Py_UNICODE)0x1EE5;
    case 0x1EE6: /* LATIN CAPITAL LETTER U WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EE7;
    case 0x1EE8: /* LATIN CAPITAL LETTER U WITH HORN AND ACUTE */
	return (Py_UNICODE)0x1EE9;
    case 0x1EEA: /* LATIN CAPITAL LETTER U WITH HORN AND GRAVE */
	return (Py_UNICODE)0x1EEB;
    case 0x1EEC: /* LATIN CAPITAL LETTER U WITH HORN AND HOOK ABOVE */
	return (Py_UNICODE)0x1EED;
    case 0x1EEE: /* LATIN CAPITAL LETTER U WITH HORN AND TILDE */
	return (Py_UNICODE)0x1EEF;
    case 0x1EF0: /* LATIN CAPITAL LETTER U WITH HORN AND DOT BELOW */
	return (Py_UNICODE)0x1EF1;
    case 0x1EF2: /* LATIN CAPITAL LETTER Y WITH GRAVE */
	return (Py_UNICODE)0x1EF3;
    case 0x1EF4: /* LATIN CAPITAL LETTER Y WITH DOT BELOW */
	return (Py_UNICODE)0x1EF5;
    case 0x1EF6: /* LATIN CAPITAL LETTER Y WITH HOOK ABOVE */
	return (Py_UNICODE)0x1EF7;
    case 0x1EF8: /* LATIN CAPITAL LETTER Y WITH TILDE */
	return (Py_UNICODE)0x1EF9;
    case 0x1F08: /* GREEK CAPITAL LETTER ALPHA WITH PSILI */
	return (Py_UNICODE)0x1F00;
    case 0x1F09: /* GREEK CAPITAL LETTER ALPHA WITH DASIA */
	return (Py_UNICODE)0x1F01;
    case 0x1F0A: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F02;
    case 0x1F0B: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F03;
    case 0x1F0C: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F04;
    case 0x1F0D: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F05;
    case 0x1F0E: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F06;
    case 0x1F0F: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F07;
    case 0x1F18: /* GREEK CAPITAL LETTER EPSILON WITH PSILI */
	return (Py_UNICODE)0x1F10;
    case 0x1F19: /* GREEK CAPITAL LETTER EPSILON WITH DASIA */
	return (Py_UNICODE)0x1F11;
    case 0x1F1A: /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F12;
    case 0x1F1B: /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F13;
    case 0x1F1C: /* GREEK CAPITAL LETTER EPSILON WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F14;
    case 0x1F1D: /* GREEK CAPITAL LETTER EPSILON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F15;
    case 0x1F28: /* GREEK CAPITAL LETTER ETA WITH PSILI */
	return (Py_UNICODE)0x1F20;
    case 0x1F29: /* GREEK CAPITAL LETTER ETA WITH DASIA */
	return (Py_UNICODE)0x1F21;
    case 0x1F2A: /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F22;
    case 0x1F2B: /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F23;
    case 0x1F2C: /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F24;
    case 0x1F2D: /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F25;
    case 0x1F2E: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F26;
    case 0x1F2F: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F27;
    case 0x1F38: /* GREEK CAPITAL LETTER IOTA WITH PSILI */
	return (Py_UNICODE)0x1F30;
    case 0x1F39: /* GREEK CAPITAL LETTER IOTA WITH DASIA */
	return (Py_UNICODE)0x1F31;
    case 0x1F3A: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F32;
    case 0x1F3B: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F33;
    case 0x1F3C: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F34;
    case 0x1F3D: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F35;
    case 0x1F3E: /* GREEK CAPITAL LETTER IOTA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F36;
    case 0x1F3F: /* GREEK CAPITAL LETTER IOTA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F37;
    case 0x1F48: /* GREEK CAPITAL LETTER OMICRON WITH PSILI */
	return (Py_UNICODE)0x1F40;
    case 0x1F49: /* GREEK CAPITAL LETTER OMICRON WITH DASIA */
	return (Py_UNICODE)0x1F41;
    case 0x1F4A: /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F42;
    case 0x1F4B: /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F43;
    case 0x1F4C: /* GREEK CAPITAL LETTER OMICRON WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F44;
    case 0x1F4D: /* GREEK CAPITAL LETTER OMICRON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F45;
    case 0x1F59: /* GREEK CAPITAL LETTER UPSILON WITH DASIA */
	return (Py_UNICODE)0x1F51;
    case 0x1F5B: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F53;
    case 0x1F5D: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F55;
    case 0x1F5F: /* GREEK CAPITAL LETTER UPSILON WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F57;
    case 0x1F68: /* GREEK CAPITAL LETTER OMEGA WITH PSILI */
	return (Py_UNICODE)0x1F60;
    case 0x1F69: /* GREEK CAPITAL LETTER OMEGA WITH DASIA */
	return (Py_UNICODE)0x1F61;
    case 0x1F6A: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA */
	return (Py_UNICODE)0x1F62;
    case 0x1F6B: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA */
	return (Py_UNICODE)0x1F63;
    case 0x1F6C: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA */
	return (Py_UNICODE)0x1F64;
    case 0x1F6D: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA */
	return (Py_UNICODE)0x1F65;
    case 0x1F6E: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI */
	return (Py_UNICODE)0x1F66;
    case 0x1F6F: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI */
	return (Py_UNICODE)0x1F67;
    case 0x1F88: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F80;
    case 0x1F89: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F81;
    case 0x1F8A: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F82;
    case 0x1F8B: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F83;
    case 0x1F8C: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F84;
    case 0x1F8D: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F85;
    case 0x1F8E: /* GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F86;
    case 0x1F8F: /* GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F87;
    case 0x1F98: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F90;
    case 0x1F99: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F91;
    case 0x1F9A: /* GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F92;
    case 0x1F9B: /* GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F93;
    case 0x1F9C: /* GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F94;
    case 0x1F9D: /* GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F95;
    case 0x1F9E: /* GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F96;
    case 0x1F9F: /* GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1F97;
    case 0x1FA8: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA0;
    case 0x1FA9: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA1;
    case 0x1FAA: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA2;
    case 0x1FAB: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA3;
    case 0x1FAC: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA4;
    case 0x1FAD: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA5;
    case 0x1FAE: /* GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA6;
    case 0x1FAF: /* GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI AND PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FA7;
    case 0x1FB8: /* GREEK CAPITAL LETTER ALPHA WITH VRACHY */
	return (Py_UNICODE)0x1FB0;
    case 0x1FB9: /* GREEK CAPITAL LETTER ALPHA WITH MACRON */
	return (Py_UNICODE)0x1FB1;
    case 0x1FBA: /* GREEK CAPITAL LETTER ALPHA WITH VARIA */
	return (Py_UNICODE)0x1F70;
    case 0x1FBB: /* GREEK CAPITAL LETTER ALPHA WITH OXIA */
	return (Py_UNICODE)0x1F71;
    case 0x1FBC: /* GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FB3;
    case 0x1FC8: /* GREEK CAPITAL LETTER EPSILON WITH VARIA */
	return (Py_UNICODE)0x1F72;
    case 0x1FC9: /* GREEK CAPITAL LETTER EPSILON WITH OXIA */
	return (Py_UNICODE)0x1F73;
    case 0x1FCA: /* GREEK CAPITAL LETTER ETA WITH VARIA */
	return (Py_UNICODE)0x1F74;
    case 0x1FCB: /* GREEK CAPITAL LETTER ETA WITH OXIA */
	return (Py_UNICODE)0x1F75;
    case 0x1FCC: /* GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FC3;
    case 0x1FD8: /* GREEK CAPITAL LETTER IOTA WITH VRACHY */
	return (Py_UNICODE)0x1FD0;
    case 0x1FD9: /* GREEK CAPITAL LETTER IOTA WITH MACRON */
	return (Py_UNICODE)0x1FD1;
    case 0x1FDA: /* GREEK CAPITAL LETTER IOTA WITH VARIA */
	return (Py_UNICODE)0x1F76;
    case 0x1FDB: /* GREEK CAPITAL LETTER IOTA WITH OXIA */
	return (Py_UNICODE)0x1F77;
    case 0x1FE8: /* GREEK CAPITAL LETTER UPSILON WITH VRACHY */
	return (Py_UNICODE)0x1FE0;
    case 0x1FE9: /* GREEK CAPITAL LETTER UPSILON WITH MACRON */
	return (Py_UNICODE)0x1FE1;
    case 0x1FEA: /* GREEK CAPITAL LETTER UPSILON WITH VARIA */
	return (Py_UNICODE)0x1F7A;
    case 0x1FEB: /* GREEK CAPITAL LETTER UPSILON WITH OXIA */
	return (Py_UNICODE)0x1F7B;
    case 0x1FEC: /* GREEK CAPITAL LETTER RHO WITH DASIA */
	return (Py_UNICODE)0x1FE5;
    case 0x1FF8: /* GREEK CAPITAL LETTER OMICRON WITH VARIA */
	return (Py_UNICODE)0x1F78;
    case 0x1FF9: /* GREEK CAPITAL LETTER OMICRON WITH OXIA */
	return (Py_UNICODE)0x1F79;
    case 0x1FFA: /* GREEK CAPITAL LETTER OMEGA WITH VARIA */
	return (Py_UNICODE)0x1F7C;
    case 0x1FFB: /* GREEK CAPITAL LETTER OMEGA WITH OXIA */
	return (Py_UNICODE)0x1F7D;
    case 0x1FFC: /* GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI */
	return (Py_UNICODE)0x1FF3;
    case 0x2126: /* OHM SIGN */
	return (Py_UNICODE)0x03C9;
    case 0x212A: /* KELVIN SIGN */
	return (Py_UNICODE)0x006B;
    case 0x212B: /* ANGSTROM SIGN */
	return (Py_UNICODE)0x00E5;
    case 0x2160: /* ROMAN NUMERAL ONE */
	return (Py_UNICODE)0x2170;
    case 0x2161: /* ROMAN NUMERAL TWO */
	return (Py_UNICODE)0x2171;
    case 0x2162: /* ROMAN NUMERAL THREE */
	return (Py_UNICODE)0x2172;
    case 0x2163: /* ROMAN NUMERAL FOUR */
	return (Py_UNICODE)0x2173;
    case 0x2164: /* ROMAN NUMERAL FIVE */
	return (Py_UNICODE)0x2174;
    case 0x2165: /* ROMAN NUMERAL SIX */
	return (Py_UNICODE)0x2175;
    case 0x2166: /* ROMAN NUMERAL SEVEN */
	return (Py_UNICODE)0x2176;
    case 0x2167: /* ROMAN NUMERAL EIGHT */
	return (Py_UNICODE)0x2177;
    case 0x2168: /* ROMAN NUMERAL NINE */
	return (Py_UNICODE)0x2178;
    case 0x2169: /* ROMAN NUMERAL TEN */
	return (Py_UNICODE)0x2179;
    case 0x216A: /* ROMAN NUMERAL ELEVEN */
	return (Py_UNICODE)0x217A;
    case 0x216B: /* ROMAN NUMERAL TWELVE */
	return (Py_UNICODE)0x217B;
    case 0x216C: /* ROMAN NUMERAL FIFTY */
	return (Py_UNICODE)0x217C;
    case 0x216D: /* ROMAN NUMERAL ONE HUNDRED */
	return (Py_UNICODE)0x217D;
    case 0x216E: /* ROMAN NUMERAL FIVE HUNDRED */
	return (Py_UNICODE)0x217E;
    case 0x216F: /* ROMAN NUMERAL ONE THOUSAND */
	return (Py_UNICODE)0x217F;
    case 0x24B6: /* CIRCLED LATIN CAPITAL LETTER A */
	return (Py_UNICODE)0x24D0;
    case 0x24B7: /* CIRCLED LATIN CAPITAL LETTER B */
	return (Py_UNICODE)0x24D1;
    case 0x24B8: /* CIRCLED LATIN CAPITAL LETTER C */
	return (Py_UNICODE)0x24D2;
    case 0x24B9: /* CIRCLED LATIN CAPITAL LETTER D */
	return (Py_UNICODE)0x24D3;
    case 0x24BA: /* CIRCLED LATIN CAPITAL LETTER E */
	return (Py_UNICODE)0x24D4;
    case 0x24BB: /* CIRCLED LATIN CAPITAL LETTER F */
	return (Py_UNICODE)0x24D5;
    case 0x24BC: /* CIRCLED LATIN CAPITAL LETTER G */
	return (Py_UNICODE)0x24D6;
    case 0x24BD: /* CIRCLED LATIN CAPITAL LETTER H */
	return (Py_UNICODE)0x24D7;
    case 0x24BE: /* CIRCLED LATIN CAPITAL LETTER I */
	return (Py_UNICODE)0x24D8;
    case 0x24BF: /* CIRCLED LATIN CAPITAL LETTER J */
	return (Py_UNICODE)0x24D9;
    case 0x24C0: /* CIRCLED LATIN CAPITAL LETTER K */
	return (Py_UNICODE)0x24DA;
    case 0x24C1: /* CIRCLED LATIN CAPITAL LETTER L */
	return (Py_UNICODE)0x24DB;
    case 0x24C2: /* CIRCLED LATIN CAPITAL LETTER M */
	return (Py_UNICODE)0x24DC;
    case 0x24C3: /* CIRCLED LATIN CAPITAL LETTER N */
	return (Py_UNICODE)0x24DD;
    case 0x24C4: /* CIRCLED LATIN CAPITAL LETTER O */
	return (Py_UNICODE)0x24DE;
    case 0x24C5: /* CIRCLED LATIN CAPITAL LETTER P */
	return (Py_UNICODE)0x24DF;
    case 0x24C6: /* CIRCLED LATIN CAPITAL LETTER Q */
	return (Py_UNICODE)0x24E0;
    case 0x24C7: /* CIRCLED LATIN CAPITAL LETTER R */
	return (Py_UNICODE)0x24E1;
    case 0x24C8: /* CIRCLED LATIN CAPITAL LETTER S */
	return (Py_UNICODE)0x24E2;
    case 0x24C9: /* CIRCLED LATIN CAPITAL LETTER T */
	return (Py_UNICODE)0x24E3;
    case 0x24CA: /* CIRCLED LATIN CAPITAL LETTER U */
	return (Py_UNICODE)0x24E4;
    case 0x24CB: /* CIRCLED LATIN CAPITAL LETTER V */
	return (Py_UNICODE)0x24E5;
    case 0x24CC: /* CIRCLED LATIN CAPITAL LETTER W */
	return (Py_UNICODE)0x24E6;
    case 0x24CD: /* CIRCLED LATIN CAPITAL LETTER X */
	return (Py_UNICODE)0x24E7;
    case 0x24CE: /* CIRCLED LATIN CAPITAL LETTER Y */
	return (Py_UNICODE)0x24E8;
    case 0x24CF: /* CIRCLED LATIN CAPITAL LETTER Z */
	return (Py_UNICODE)0x24E9;
    case 0xFF21: /* FULLWIDTH LATIN CAPITAL LETTER A */
	return (Py_UNICODE)0xFF41;
    case 0xFF22: /* FULLWIDTH LATIN CAPITAL LETTER B */
	return (Py_UNICODE)0xFF42;
    case 0xFF23: /* FULLWIDTH LATIN CAPITAL LETTER C */
	return (Py_UNICODE)0xFF43;
    case 0xFF24: /* FULLWIDTH LATIN CAPITAL LETTER D */
	return (Py_UNICODE)0xFF44;
    case 0xFF25: /* FULLWIDTH LATIN CAPITAL LETTER E */
	return (Py_UNICODE)0xFF45;
    case 0xFF26: /* FULLWIDTH LATIN CAPITAL LETTER F */
	return (Py_UNICODE)0xFF46;
    case 0xFF27: /* FULLWIDTH LATIN CAPITAL LETTER G */
	return (Py_UNICODE)0xFF47;
    case 0xFF28: /* FULLWIDTH LATIN CAPITAL LETTER H */
	return (Py_UNICODE)0xFF48;
    case 0xFF29: /* FULLWIDTH LATIN CAPITAL LETTER I */
	return (Py_UNICODE)0xFF49;
    case 0xFF2A: /* FULLWIDTH LATIN CAPITAL LETTER J */
	return (Py_UNICODE)0xFF4A;
    case 0xFF2B: /* FULLWIDTH LATIN CAPITAL LETTER K */
	return (Py_UNICODE)0xFF4B;
    case 0xFF2C: /* FULLWIDTH LATIN CAPITAL LETTER L */
	return (Py_UNICODE)0xFF4C;
    case 0xFF2D: /* FULLWIDTH LATIN CAPITAL LETTER M */
	return (Py_UNICODE)0xFF4D;
    case 0xFF2E: /* FULLWIDTH LATIN CAPITAL LETTER N */
	return (Py_UNICODE)0xFF4E;
    case 0xFF2F: /* FULLWIDTH LATIN CAPITAL LETTER O */
	return (Py_UNICODE)0xFF4F;
    case 0xFF30: /* FULLWIDTH LATIN CAPITAL LETTER P */
	return (Py_UNICODE)0xFF50;
    case 0xFF31: /* FULLWIDTH LATIN CAPITAL LETTER Q */
	return (Py_UNICODE)0xFF51;
    case 0xFF32: /* FULLWIDTH LATIN CAPITAL LETTER R */
	return (Py_UNICODE)0xFF52;
    case 0xFF33: /* FULLWIDTH LATIN CAPITAL LETTER S */
	return (Py_UNICODE)0xFF53;
    case 0xFF34: /* FULLWIDTH LATIN CAPITAL LETTER T */
	return (Py_UNICODE)0xFF54;
    case 0xFF35: /* FULLWIDTH LATIN CAPITAL LETTER U */
	return (Py_UNICODE)0xFF55;
    case 0xFF36: /* FULLWIDTH LATIN CAPITAL LETTER V */
	return (Py_UNICODE)0xFF56;
    case 0xFF37: /* FULLWIDTH LATIN CAPITAL LETTER W */
	return (Py_UNICODE)0xFF57;
    case 0xFF38: /* FULLWIDTH LATIN CAPITAL LETTER X */
	return (Py_UNICODE)0xFF58;
    case 0xFF39: /* FULLWIDTH LATIN CAPITAL LETTER Y */
	return (Py_UNICODE)0xFF59;
    case 0xFF3A: /* FULLWIDTH LATIN CAPITAL LETTER Z */
	return (Py_UNICODE)0xFF5A;
    default:
	return ch;
    }
}

/* Returns 1 for Unicode characters having the category 'Ll', 'Lu', 'Lt',
   'Lo' or 'Lm',  0 otherwise. */

int _PyUnicode_IsAlpha(register const Py_UNICODE ch)
{
    if (_PyUnicode_IsLowercase(ch) ||
    	_PyUnicode_IsUppercase(ch) ||
    	_PyUnicode_IsTitlecase(ch))
    	return 1;

    /* Letters with category 'Lo' or 'Lm' */    	
    switch (ch) {
    case 0x01BB: /* LATIN LETTER TWO WITH STROKE */
    case 0x01C0: /* LATIN LETTER DENTAL CLICK */
    case 0x01C1: /* LATIN LETTER LATERAL CLICK */
    case 0x01C2: /* LATIN LETTER ALVEOLAR CLICK */
    case 0x01C3: /* LATIN LETTER RETROFLEX CLICK */
    case 0x02B0: /* MODIFIER LETTER SMALL H */
    case 0x02B1: /* MODIFIER LETTER SMALL H WITH HOOK */
    case 0x02B2: /* MODIFIER LETTER SMALL J */
    case 0x02B3: /* MODIFIER LETTER SMALL R */
    case 0x02B4: /* MODIFIER LETTER SMALL TURNED R */
    case 0x02B5: /* MODIFIER LETTER SMALL TURNED R WITH HOOK */
    case 0x02B6: /* MODIFIER LETTER SMALL CAPITAL INVERTED R */
    case 0x02B7: /* MODIFIER LETTER SMALL W */
    case 0x02B8: /* MODIFIER LETTER SMALL Y */
    case 0x02BB: /* MODIFIER LETTER TURNED COMMA */
    case 0x02BC: /* MODIFIER LETTER APOSTROPHE */
    case 0x02BD: /* MODIFIER LETTER REVERSED COMMA */
    case 0x02BE: /* MODIFIER LETTER RIGHT HALF RING */
    case 0x02BF: /* MODIFIER LETTER LEFT HALF RING */
    case 0x02C0: /* MODIFIER LETTER GLOTTAL STOP */
    case 0x02C1: /* MODIFIER LETTER REVERSED GLOTTAL STOP */
    case 0x02D0: /* MODIFIER LETTER TRIANGULAR COLON */
    case 0x02D1: /* MODIFIER LETTER HALF TRIANGULAR COLON */
    case 0x02E0: /* MODIFIER LETTER SMALL GAMMA */
    case 0x02E1: /* MODIFIER LETTER SMALL L */
    case 0x02E2: /* MODIFIER LETTER SMALL S */
    case 0x02E3: /* MODIFIER LETTER SMALL X */
    case 0x02E4: /* MODIFIER LETTER SMALL REVERSED GLOTTAL STOP */
    case 0x02EE: /* MODIFIER LETTER DOUBLE APOSTROPHE */
    case 0x037A: /* GREEK YPOGEGRAMMENI */
    case 0x0559: /* ARMENIAN MODIFIER LETTER LEFT HALF RING */
    case 0x05D0: /* HEBREW LETTER ALEF */
    case 0x05D1: /* HEBREW LETTER BET */
    case 0x05D2: /* HEBREW LETTER GIMEL */
    case 0x05D3: /* HEBREW LETTER DALET */
    case 0x05D4: /* HEBREW LETTER HE */
    case 0x05D5: /* HEBREW LETTER VAV */
    case 0x05D6: /* HEBREW LETTER ZAYIN */
    case 0x05D7: /* HEBREW LETTER HET */
    case 0x05D8: /* HEBREW LETTER TET */
    case 0x05D9: /* HEBREW LETTER YOD */
    case 0x05DA: /* HEBREW LETTER FINAL KAF */
    case 0x05DB: /* HEBREW LETTER KAF */
    case 0x05DC: /* HEBREW LETTER LAMED */
    case 0x05DD: /* HEBREW LETTER FINAL MEM */
    case 0x05DE: /* HEBREW LETTER MEM */
    case 0x05DF: /* HEBREW LETTER FINAL NUN */
    case 0x05E0: /* HEBREW LETTER NUN */
    case 0x05E1: /* HEBREW LETTER SAMEKH */
    case 0x05E2: /* HEBREW LETTER AYIN */
    case 0x05E3: /* HEBREW LETTER FINAL PE */
    case 0x05E4: /* HEBREW LETTER PE */
    case 0x05E5: /* HEBREW LETTER FINAL TSADI */
    case 0x05E6: /* HEBREW LETTER TSADI */
    case 0x05E7: /* HEBREW LETTER QOF */
    case 0x05E8: /* HEBREW LETTER RESH */
    case 0x05E9: /* HEBREW LETTER SHIN */
    case 0x05EA: /* HEBREW LETTER TAV */
    case 0x05F0: /* HEBREW LIGATURE YIDDISH DOUBLE VAV */
    case 0x05F1: /* HEBREW LIGATURE YIDDISH VAV YOD */
    case 0x05F2: /* HEBREW LIGATURE YIDDISH DOUBLE YOD */
    case 0x0621: /* ARABIC LETTER HAMZA */
    case 0x0622: /* ARABIC LETTER ALEF WITH MADDA ABOVE */
    case 0x0623: /* ARABIC LETTER ALEF WITH HAMZA ABOVE */
    case 0x0624: /* ARABIC LETTER WAW WITH HAMZA ABOVE */
    case 0x0625: /* ARABIC LETTER ALEF WITH HAMZA BELOW */
    case 0x0626: /* ARABIC LETTER YEH WITH HAMZA ABOVE */
    case 0x0627: /* ARABIC LETTER ALEF */
    case 0x0628: /* ARABIC LETTER BEH */
    case 0x0629: /* ARABIC LETTER TEH MARBUTA */
    case 0x062A: /* ARABIC LETTER TEH */
    case 0x062B: /* ARABIC LETTER THEH */
    case 0x062C: /* ARABIC LETTER JEEM */
    case 0x062D: /* ARABIC LETTER HAH */
    case 0x062E: /* ARABIC LETTER KHAH */
    case 0x062F: /* ARABIC LETTER DAL */
    case 0x0630: /* ARABIC LETTER THAL */
    case 0x0631: /* ARABIC LETTER REH */
    case 0x0632: /* ARABIC LETTER ZAIN */
    case 0x0633: /* ARABIC LETTER SEEN */
    case 0x0634: /* ARABIC LETTER SHEEN */
    case 0x0635: /* ARABIC LETTER SAD */
    case 0x0636: /* ARABIC LETTER DAD */
    case 0x0637: /* ARABIC LETTER TAH */
    case 0x0638: /* ARABIC LETTER ZAH */
    case 0x0639: /* ARABIC LETTER AIN */
    case 0x063A: /* ARABIC LETTER GHAIN */
    case 0x0640: /* ARABIC TATWEEL */
    case 0x0641: /* ARABIC LETTER FEH */
    case 0x0642: /* ARABIC LETTER QAF */
    case 0x0643: /* ARABIC LETTER KAF */
    case 0x0644: /* ARABIC LETTER LAM */
    case 0x0645: /* ARABIC LETTER MEEM */
    case 0x0646: /* ARABIC LETTER NOON */
    case 0x0647: /* ARABIC LETTER HEH */
    case 0x0648: /* ARABIC LETTER WAW */
    case 0x0649: /* ARABIC LETTER ALEF MAKSURA */
    case 0x064A: /* ARABIC LETTER YEH */
    case 0x0671: /* ARABIC LETTER ALEF WASLA */
    case 0x0672: /* ARABIC LETTER ALEF WITH WAVY HAMZA ABOVE */
    case 0x0673: /* ARABIC LETTER ALEF WITH WAVY HAMZA BELOW */
    case 0x0674: /* ARABIC LETTER HIGH HAMZA */
    case 0x0675: /* ARABIC LETTER HIGH HAMZA ALEF */
    case 0x0676: /* ARABIC LETTER HIGH HAMZA WAW */
    case 0x0677: /* ARABIC LETTER U WITH HAMZA ABOVE */
    case 0x0678: /* ARABIC LETTER HIGH HAMZA YEH */
    case 0x0679: /* ARABIC LETTER TTEH */
    case 0x067A: /* ARABIC LETTER TTEHEH */
    case 0x067B: /* ARABIC LETTER BEEH */
    case 0x067C: /* ARABIC LETTER TEH WITH RING */
    case 0x067D: /* ARABIC LETTER TEH WITH THREE DOTS ABOVE DOWNWARDS */
    case 0x067E: /* ARABIC LETTER PEH */
    case 0x067F: /* ARABIC LETTER TEHEH */
    case 0x0680: /* ARABIC LETTER BEHEH */
    case 0x0681: /* ARABIC LETTER HAH WITH HAMZA ABOVE */
    case 0x0682: /* ARABIC LETTER HAH WITH TWO DOTS VERTICAL ABOVE */
    case 0x0683: /* ARABIC LETTER NYEH */
    case 0x0684: /* ARABIC LETTER DYEH */
    case 0x0685: /* ARABIC LETTER HAH WITH THREE DOTS ABOVE */
    case 0x0686: /* ARABIC LETTER TCHEH */
    case 0x0687: /* ARABIC LETTER TCHEHEH */
    case 0x0688: /* ARABIC LETTER DDAL */
    case 0x0689: /* ARABIC LETTER DAL WITH RING */
    case 0x068A: /* ARABIC LETTER DAL WITH DOT BELOW */
    case 0x068B: /* ARABIC LETTER DAL WITH DOT BELOW AND SMALL TAH */
    case 0x068C: /* ARABIC LETTER DAHAL */
    case 0x068D: /* ARABIC LETTER DDAHAL */
    case 0x068E: /* ARABIC LETTER DUL */
    case 0x068F: /* ARABIC LETTER DAL WITH THREE DOTS ABOVE DOWNWARDS */
    case 0x0690: /* ARABIC LETTER DAL WITH FOUR DOTS ABOVE */
    case 0x0691: /* ARABIC LETTER RREH */
    case 0x0692: /* ARABIC LETTER REH WITH SMALL V */
    case 0x0693: /* ARABIC LETTER REH WITH RING */
    case 0x0694: /* ARABIC LETTER REH WITH DOT BELOW */
    case 0x0695: /* ARABIC LETTER REH WITH SMALL V BELOW */
    case 0x0696: /* ARABIC LETTER REH WITH DOT BELOW AND DOT ABOVE */
    case 0x0697: /* ARABIC LETTER REH WITH TWO DOTS ABOVE */
    case 0x0698: /* ARABIC LETTER JEH */
    case 0x0699: /* ARABIC LETTER REH WITH FOUR DOTS ABOVE */
    case 0x069A: /* ARABIC LETTER SEEN WITH DOT BELOW AND DOT ABOVE */
    case 0x069B: /* ARABIC LETTER SEEN WITH THREE DOTS BELOW */
    case 0x069C: /* ARABIC LETTER SEEN WITH THREE DOTS BELOW AND THREE DOTS ABOVE */
    case 0x069D: /* ARABIC LETTER SAD WITH TWO DOTS BELOW */
    case 0x069E: /* ARABIC LETTER SAD WITH THREE DOTS ABOVE */
    case 0x069F: /* ARABIC LETTER TAH WITH THREE DOTS ABOVE */
    case 0x06A0: /* ARABIC LETTER AIN WITH THREE DOTS ABOVE */
    case 0x06A1: /* ARABIC LETTER DOTLESS FEH */
    case 0x06A2: /* ARABIC LETTER FEH WITH DOT MOVED BELOW */
    case 0x06A3: /* ARABIC LETTER FEH WITH DOT BELOW */
    case 0x06A4: /* ARABIC LETTER VEH */
    case 0x06A5: /* ARABIC LETTER FEH WITH THREE DOTS BELOW */
    case 0x06A6: /* ARABIC LETTER PEHEH */
    case 0x06A7: /* ARABIC LETTER QAF WITH DOT ABOVE */
    case 0x06A8: /* ARABIC LETTER QAF WITH THREE DOTS ABOVE */
    case 0x06A9: /* ARABIC LETTER KEHEH */
    case 0x06AA: /* ARABIC LETTER SWASH KAF */
    case 0x06AB: /* ARABIC LETTER KAF WITH RING */
    case 0x06AC: /* ARABIC LETTER KAF WITH DOT ABOVE */
    case 0x06AD: /* ARABIC LETTER NG */
    case 0x06AE: /* ARABIC LETTER KAF WITH THREE DOTS BELOW */
    case 0x06AF: /* ARABIC LETTER GAF */
    case 0x06B0: /* ARABIC LETTER GAF WITH RING */
    case 0x06B1: /* ARABIC LETTER NGOEH */
    case 0x06B2: /* ARABIC LETTER GAF WITH TWO DOTS BELOW */
    case 0x06B3: /* ARABIC LETTER GUEH */
    case 0x06B4: /* ARABIC LETTER GAF WITH THREE DOTS ABOVE */
    case 0x06B5: /* ARABIC LETTER LAM WITH SMALL V */
    case 0x06B6: /* ARABIC LETTER LAM WITH DOT ABOVE */
    case 0x06B7: /* ARABIC LETTER LAM WITH THREE DOTS ABOVE */
    case 0x06B8: /* ARABIC LETTER LAM WITH THREE DOTS BELOW */
    case 0x06B9: /* ARABIC LETTER NOON WITH DOT BELOW */
    case 0x06BA: /* ARABIC LETTER NOON GHUNNA */
    case 0x06BB: /* ARABIC LETTER RNOON */
    case 0x06BC: /* ARABIC LETTER NOON WITH RING */
    case 0x06BD: /* ARABIC LETTER NOON WITH THREE DOTS ABOVE */
    case 0x06BE: /* ARABIC LETTER HEH DOACHASHMEE */
    case 0x06BF: /* ARABIC LETTER TCHEH WITH DOT ABOVE */
    case 0x06C0: /* ARABIC LETTER HEH WITH YEH ABOVE */
    case 0x06C1: /* ARABIC LETTER HEH GOAL */
    case 0x06C2: /* ARABIC LETTER HEH GOAL WITH HAMZA ABOVE */
    case 0x06C3: /* ARABIC LETTER TEH MARBUTA GOAL */
    case 0x06C4: /* ARABIC LETTER WAW WITH RING */
    case 0x06C5: /* ARABIC LETTER KIRGHIZ OE */
    case 0x06C6: /* ARABIC LETTER OE */
    case 0x06C7: /* ARABIC LETTER U */
    case 0x06C8: /* ARABIC LETTER YU */
    case 0x06C9: /* ARABIC LETTER KIRGHIZ YU */
    case 0x06CA: /* ARABIC LETTER WAW WITH TWO DOTS ABOVE */
    case 0x06CB: /* ARABIC LETTER VE */
    case 0x06CC: /* ARABIC LETTER FARSI YEH */
    case 0x06CD: /* ARABIC LETTER YEH WITH TAIL */
    case 0x06CE: /* ARABIC LETTER YEH WITH SMALL V */
    case 0x06CF: /* ARABIC LETTER WAW WITH DOT ABOVE */
    case 0x06D0: /* ARABIC LETTER E */
    case 0x06D1: /* ARABIC LETTER YEH WITH THREE DOTS BELOW */
    case 0x06D2: /* ARABIC LETTER YEH BARREE */
    case 0x06D3: /* ARABIC LETTER YEH BARREE WITH HAMZA ABOVE */
    case 0x06D5: /* ARABIC LETTER AE */
    case 0x06E5: /* ARABIC SMALL WAW */
    case 0x06E6: /* ARABIC SMALL YEH */
    case 0x06FA: /* ARABIC LETTER SHEEN WITH DOT BELOW */
    case 0x06FB: /* ARABIC LETTER DAD WITH DOT BELOW */
    case 0x06FC: /* ARABIC LETTER GHAIN WITH DOT BELOW */
    case 0x0710: /* SYRIAC LETTER ALAPH */
    case 0x0712: /* SYRIAC LETTER BETH */
    case 0x0713: /* SYRIAC LETTER GAMAL */
    case 0x0714: /* SYRIAC LETTER GAMAL GARSHUNI */
    case 0x0715: /* SYRIAC LETTER DALATH */
    case 0x0716: /* SYRIAC LETTER DOTLESS DALATH RISH */
    case 0x0717: /* SYRIAC LETTER HE */
    case 0x0718: /* SYRIAC LETTER WAW */
    case 0x0719: /* SYRIAC LETTER ZAIN */
    case 0x071A: /* SYRIAC LETTER HETH */
    case 0x071B: /* SYRIAC LETTER TETH */
    case 0x071C: /* SYRIAC LETTER TETH GARSHUNI */
    case 0x071D: /* SYRIAC LETTER YUDH */
    case 0x071E: /* SYRIAC LETTER YUDH HE */
    case 0x071F: /* SYRIAC LETTER KAPH */
    case 0x0720: /* SYRIAC LETTER LAMADH */
    case 0x0721: /* SYRIAC LETTER MIM */
    case 0x0722: /* SYRIAC LETTER NUN */
    case 0x0723: /* SYRIAC LETTER SEMKATH */
    case 0x0724: /* SYRIAC LETTER FINAL SEMKATH */
    case 0x0725: /* SYRIAC LETTER E */
    case 0x0726: /* SYRIAC LETTER PE */
    case 0x0727: /* SYRIAC LETTER REVERSED PE */
    case 0x0728: /* SYRIAC LETTER SADHE */
    case 0x0729: /* SYRIAC LETTER QAPH */
    case 0x072A: /* SYRIAC LETTER RISH */
    case 0x072B: /* SYRIAC LETTER SHIN */
    case 0x072C: /* SYRIAC LETTER TAW */
    case 0x0780: /* THAANA LETTER HAA */
    case 0x0781: /* THAANA LETTER SHAVIYANI */
    case 0x0782: /* THAANA LETTER NOONU */
    case 0x0783: /* THAANA LETTER RAA */
    case 0x0784: /* THAANA LETTER BAA */
    case 0x0785: /* THAANA LETTER LHAVIYANI */
    case 0x0786: /* THAANA LETTER KAAFU */
    case 0x0787: /* THAANA LETTER ALIFU */
    case 0x0788: /* THAANA LETTER VAAVU */
    case 0x0789: /* THAANA LETTER MEEMU */
    case 0x078A: /* THAANA LETTER FAAFU */
    case 0x078B: /* THAANA LETTER DHAALU */
    case 0x078C: /* THAANA LETTER THAA */
    case 0x078D: /* THAANA LETTER LAAMU */
    case 0x078E: /* THAANA LETTER GAAFU */
    case 0x078F: /* THAANA LETTER GNAVIYANI */
    case 0x0790: /* THAANA LETTER SEENU */
    case 0x0791: /* THAANA LETTER DAVIYANI */
    case 0x0792: /* THAANA LETTER ZAVIYANI */
    case 0x0793: /* THAANA LETTER TAVIYANI */
    case 0x0794: /* THAANA LETTER YAA */
    case 0x0795: /* THAANA LETTER PAVIYANI */
    case 0x0796: /* THAANA LETTER JAVIYANI */
    case 0x0797: /* THAANA LETTER CHAVIYANI */
    case 0x0798: /* THAANA LETTER TTAA */
    case 0x0799: /* THAANA LETTER HHAA */
    case 0x079A: /* THAANA LETTER KHAA */
    case 0x079B: /* THAANA LETTER THAALU */
    case 0x079C: /* THAANA LETTER ZAA */
    case 0x079D: /* THAANA LETTER SHEENU */
    case 0x079E: /* THAANA LETTER SAADHU */
    case 0x079F: /* THAANA LETTER DAADHU */
    case 0x07A0: /* THAANA LETTER TO */
    case 0x07A1: /* THAANA LETTER ZO */
    case 0x07A2: /* THAANA LETTER AINU */
    case 0x07A3: /* THAANA LETTER GHAINU */
    case 0x07A4: /* THAANA LETTER QAAFU */
    case 0x07A5: /* THAANA LETTER WAAVU */
    case 0x0905: /* DEVANAGARI LETTER A */
    case 0x0906: /* DEVANAGARI LETTER AA */
    case 0x0907: /* DEVANAGARI LETTER I */
    case 0x0908: /* DEVANAGARI LETTER II */
    case 0x0909: /* DEVANAGARI LETTER U */
    case 0x090A: /* DEVANAGARI LETTER UU */
    case 0x090B: /* DEVANAGARI LETTER VOCALIC R */
    case 0x090C: /* DEVANAGARI LETTER VOCALIC L */
    case 0x090D: /* DEVANAGARI LETTER CANDRA E */
    case 0x090E: /* DEVANAGARI LETTER SHORT E */
    case 0x090F: /* DEVANAGARI LETTER E */
    case 0x0910: /* DEVANAGARI LETTER AI */
    case 0x0911: /* DEVANAGARI LETTER CANDRA O */
    case 0x0912: /* DEVANAGARI LETTER SHORT O */
    case 0x0913: /* DEVANAGARI LETTER O */
    case 0x0914: /* DEVANAGARI LETTER AU */
    case 0x0915: /* DEVANAGARI LETTER KA */
    case 0x0916: /* DEVANAGARI LETTER KHA */
    case 0x0917: /* DEVANAGARI LETTER GA */
    case 0x0918: /* DEVANAGARI LETTER GHA */
    case 0x0919: /* DEVANAGARI LETTER NGA */
    case 0x091A: /* DEVANAGARI LETTER CA */
    case 0x091B: /* DEVANAGARI LETTER CHA */
    case 0x091C: /* DEVANAGARI LETTER JA */
    case 0x091D: /* DEVANAGARI LETTER JHA */
    case 0x091E: /* DEVANAGARI LETTER NYA */
    case 0x091F: /* DEVANAGARI LETTER TTA */
    case 0x0920: /* DEVANAGARI LETTER TTHA */
    case 0x0921: /* DEVANAGARI LETTER DDA */
    case 0x0922: /* DEVANAGARI LETTER DDHA */
    case 0x0923: /* DEVANAGARI LETTER NNA */
    case 0x0924: /* DEVANAGARI LETTER TA */
    case 0x0925: /* DEVANAGARI LETTER THA */
    case 0x0926: /* DEVANAGARI LETTER DA */
    case 0x0927: /* DEVANAGARI LETTER DHA */
    case 0x0928: /* DEVANAGARI LETTER NA */
    case 0x0929: /* DEVANAGARI LETTER NNNA */
    case 0x092A: /* DEVANAGARI LETTER PA */
    case 0x092B: /* DEVANAGARI LETTER PHA */
    case 0x092C: /* DEVANAGARI LETTER BA */
    case 0x092D: /* DEVANAGARI LETTER BHA */
    case 0x092E: /* DEVANAGARI LETTER MA */
    case 0x092F: /* DEVANAGARI LETTER YA */
    case 0x0930: /* DEVANAGARI LETTER RA */
    case 0x0931: /* DEVANAGARI LETTER RRA */
    case 0x0932: /* DEVANAGARI LETTER LA */
    case 0x0933: /* DEVANAGARI LETTER LLA */
    case 0x0934: /* DEVANAGARI LETTER LLLA */
    case 0x0935: /* DEVANAGARI LETTER VA */
    case 0x0936: /* DEVANAGARI LETTER SHA */
    case 0x0937: /* DEVANAGARI LETTER SSA */
    case 0x0938: /* DEVANAGARI LETTER SA */
    case 0x0939: /* DEVANAGARI LETTER HA */
    case 0x093D: /* DEVANAGARI SIGN AVAGRAHA */
    case 0x0950: /* DEVANAGARI OM */
    case 0x0958: /* DEVANAGARI LETTER QA */
    case 0x0959: /* DEVANAGARI LETTER KHHA */
    case 0x095A: /* DEVANAGARI LETTER GHHA */
    case 0x095B: /* DEVANAGARI LETTER ZA */
    case 0x095C: /* DEVANAGARI LETTER DDDHA */
    case 0x095D: /* DEVANAGARI LETTER RHA */
    case 0x095E: /* DEVANAGARI LETTER FA */
    case 0x095F: /* DEVANAGARI LETTER YYA */
    case 0x0960: /* DEVANAGARI LETTER VOCALIC RR */
    case 0x0961: /* DEVANAGARI LETTER VOCALIC LL */
    case 0x0985: /* BENGALI LETTER A */
    case 0x0986: /* BENGALI LETTER AA */
    case 0x0987: /* BENGALI LETTER I */
    case 0x0988: /* BENGALI LETTER II */
    case 0x0989: /* BENGALI LETTER U */
    case 0x098A: /* BENGALI LETTER UU */
    case 0x098B: /* BENGALI LETTER VOCALIC R */
    case 0x098C: /* BENGALI LETTER VOCALIC L */
    case 0x098F: /* BENGALI LETTER E */
    case 0x0990: /* BENGALI LETTER AI */
    case 0x0993: /* BENGALI LETTER O */
    case 0x0994: /* BENGALI LETTER AU */
    case 0x0995: /* BENGALI LETTER KA */
    case 0x0996: /* BENGALI LETTER KHA */
    case 0x0997: /* BENGALI LETTER GA */
    case 0x0998: /* BENGALI LETTER GHA */
    case 0x0999: /* BENGALI LETTER NGA */
    case 0x099A: /* BENGALI LETTER CA */
    case 0x099B: /* BENGALI LETTER CHA */
    case 0x099C: /* BENGALI LETTER JA */
    case 0x099D: /* BENGALI LETTER JHA */
    case 0x099E: /* BENGALI LETTER NYA */
    case 0x099F: /* BENGALI LETTER TTA */
    case 0x09A0: /* BENGALI LETTER TTHA */
    case 0x09A1: /* BENGALI LETTER DDA */
    case 0x09A2: /* BENGALI LETTER DDHA */
    case 0x09A3: /* BENGALI LETTER NNA */
    case 0x09A4: /* BENGALI LETTER TA */
    case 0x09A5: /* BENGALI LETTER THA */
    case 0x09A6: /* BENGALI LETTER DA */
    case 0x09A7: /* BENGALI LETTER DHA */
    case 0x09A8: /* BENGALI LETTER NA */
    case 0x09AA: /* BENGALI LETTER PA */
    case 0x09AB: /* BENGALI LETTER PHA */
    case 0x09AC: /* BENGALI LETTER BA */
    case 0x09AD: /* BENGALI LETTER BHA */
    case 0x09AE: /* BENGALI LETTER MA */
    case 0x09AF: /* BENGALI LETTER YA */
    case 0x09B0: /* BENGALI LETTER RA */
    case 0x09B2: /* BENGALI LETTER LA */
    case 0x09B6: /* BENGALI LETTER SHA */
    case 0x09B7: /* BENGALI LETTER SSA */
    case 0x09B8: /* BENGALI LETTER SA */
    case 0x09B9: /* BENGALI LETTER HA */
    case 0x09DC: /* BENGALI LETTER RRA */
    case 0x09DD: /* BENGALI LETTER RHA */
    case 0x09DF: /* BENGALI LETTER YYA */
    case 0x09E0: /* BENGALI LETTER VOCALIC RR */
    case 0x09E1: /* BENGALI LETTER VOCALIC LL */
    case 0x09F0: /* BENGALI LETTER RA WITH MIDDLE DIAGONAL */
    case 0x09F1: /* BENGALI LETTER RA WITH LOWER DIAGONAL */
    case 0x0A05: /* GURMUKHI LETTER A */
    case 0x0A06: /* GURMUKHI LETTER AA */
    case 0x0A07: /* GURMUKHI LETTER I */
    case 0x0A08: /* GURMUKHI LETTER II */
    case 0x0A09: /* GURMUKHI LETTER U */
    case 0x0A0A: /* GURMUKHI LETTER UU */
    case 0x0A0F: /* GURMUKHI LETTER EE */
    case 0x0A10: /* GURMUKHI LETTER AI */
    case 0x0A13: /* GURMUKHI LETTER OO */
    case 0x0A14: /* GURMUKHI LETTER AU */
    case 0x0A15: /* GURMUKHI LETTER KA */
    case 0x0A16: /* GURMUKHI LETTER KHA */
    case 0x0A17: /* GURMUKHI LETTER GA */
    case 0x0A18: /* GURMUKHI LETTER GHA */
    case 0x0A19: /* GURMUKHI LETTER NGA */
    case 0x0A1A: /* GURMUKHI LETTER CA */
    case 0x0A1B: /* GURMUKHI LETTER CHA */
    case 0x0A1C: /* GURMUKHI LETTER JA */
    case 0x0A1D: /* GURMUKHI LETTER JHA */
    case 0x0A1E: /* GURMUKHI LETTER NYA */
    case 0x0A1F: /* GURMUKHI LETTER TTA */
    case 0x0A20: /* GURMUKHI LETTER TTHA */
    case 0x0A21: /* GURMUKHI LETTER DDA */
    case 0x0A22: /* GURMUKHI LETTER DDHA */
    case 0x0A23: /* GURMUKHI LETTER NNA */
    case 0x0A24: /* GURMUKHI LETTER TA */
    case 0x0A25: /* GURMUKHI LETTER THA */
    case 0x0A26: /* GURMUKHI LETTER DA */
    case 0x0A27: /* GURMUKHI LETTER DHA */
    case 0x0A28: /* GURMUKHI LETTER NA */
    case 0x0A2A: /* GURMUKHI LETTER PA */
    case 0x0A2B: /* GURMUKHI LETTER PHA */
    case 0x0A2C: /* GURMUKHI LETTER BA */
    case 0x0A2D: /* GURMUKHI LETTER BHA */
    case 0x0A2E: /* GURMUKHI LETTER MA */
    case 0x0A2F: /* GURMUKHI LETTER YA */
    case 0x0A30: /* GURMUKHI LETTER RA */
    case 0x0A32: /* GURMUKHI LETTER LA */
    case 0x0A33: /* GURMUKHI LETTER LLA */
    case 0x0A35: /* GURMUKHI LETTER VA */
    case 0x0A36: /* GURMUKHI LETTER SHA */
    case 0x0A38: /* GURMUKHI LETTER SA */
    case 0x0A39: /* GURMUKHI LETTER HA */
    case 0x0A59: /* GURMUKHI LETTER KHHA */
    case 0x0A5A: /* GURMUKHI LETTER GHHA */
    case 0x0A5B: /* GURMUKHI LETTER ZA */
    case 0x0A5C: /* GURMUKHI LETTER RRA */
    case 0x0A5E: /* GURMUKHI LETTER FA */
    case 0x0A72: /* GURMUKHI IRI */
    case 0x0A73: /* GURMUKHI URA */
    case 0x0A74: /* GURMUKHI EK ONKAR */
    case 0x0A85: /* GUJARATI LETTER A */
    case 0x0A86: /* GUJARATI LETTER AA */
    case 0x0A87: /* GUJARATI LETTER I */
    case 0x0A88: /* GUJARATI LETTER II */
    case 0x0A89: /* GUJARATI LETTER U */
    case 0x0A8A: /* GUJARATI LETTER UU */
    case 0x0A8B: /* GUJARATI LETTER VOCALIC R */
    case 0x0A8D: /* GUJARATI VOWEL CANDRA E */
    case 0x0A8F: /* GUJARATI LETTER E */
    case 0x0A90: /* GUJARATI LETTER AI */
    case 0x0A91: /* GUJARATI VOWEL CANDRA O */
    case 0x0A93: /* GUJARATI LETTER O */
    case 0x0A94: /* GUJARATI LETTER AU */
    case 0x0A95: /* GUJARATI LETTER KA */
    case 0x0A96: /* GUJARATI LETTER KHA */
    case 0x0A97: /* GUJARATI LETTER GA */
    case 0x0A98: /* GUJARATI LETTER GHA */
    case 0x0A99: /* GUJARATI LETTER NGA */
    case 0x0A9A: /* GUJARATI LETTER CA */
    case 0x0A9B: /* GUJARATI LETTER CHA */
    case 0x0A9C: /* GUJARATI LETTER JA */
    case 0x0A9D: /* GUJARATI LETTER JHA */
    case 0x0A9E: /* GUJARATI LETTER NYA */
    case 0x0A9F: /* GUJARATI LETTER TTA */
    case 0x0AA0: /* GUJARATI LETTER TTHA */
    case 0x0AA1: /* GUJARATI LETTER DDA */
    case 0x0AA2: /* GUJARATI LETTER DDHA */
    case 0x0AA3: /* GUJARATI LETTER NNA */
    case 0x0AA4: /* GUJARATI LETTER TA */
    case 0x0AA5: /* GUJARATI LETTER THA */
    case 0x0AA6: /* GUJARATI LETTER DA */
    case 0x0AA7: /* GUJARATI LETTER DHA */
    case 0x0AA8: /* GUJARATI LETTER NA */
    case 0x0AAA: /* GUJARATI LETTER PA */
    case 0x0AAB: /* GUJARATI LETTER PHA */
    case 0x0AAC: /* GUJARATI LETTER BA */
    case 0x0AAD: /* GUJARATI LETTER BHA */
    case 0x0AAE: /* GUJARATI LETTER MA */
    case 0x0AAF: /* GUJARATI LETTER YA */
    case 0x0AB0: /* GUJARATI LETTER RA */
    case 0x0AB2: /* GUJARATI LETTER LA */
    case 0x0AB3: /* GUJARATI LETTER LLA */
    case 0x0AB5: /* GUJARATI LETTER VA */
    case 0x0AB6: /* GUJARATI LETTER SHA */
    case 0x0AB7: /* GUJARATI LETTER SSA */
    case 0x0AB8: /* GUJARATI LETTER SA */
    case 0x0AB9: /* GUJARATI LETTER HA */
    case 0x0ABD: /* GUJARATI SIGN AVAGRAHA */
    case 0x0AD0: /* GUJARATI OM */
    case 0x0AE0: /* GUJARATI LETTER VOCALIC RR */
    case 0x0B05: /* ORIYA LETTER A */
    case 0x0B06: /* ORIYA LETTER AA */
    case 0x0B07: /* ORIYA LETTER I */
    case 0x0B08: /* ORIYA LETTER II */
    case 0x0B09: /* ORIYA LETTER U */
    case 0x0B0A: /* ORIYA LETTER UU */
    case 0x0B0B: /* ORIYA LETTER VOCALIC R */
    case 0x0B0C: /* ORIYA LETTER VOCALIC L */
    case 0x0B0F: /* ORIYA LETTER E */
    case 0x0B10: /* ORIYA LETTER AI */
    case 0x0B13: /* ORIYA LETTER O */
    case 0x0B14: /* ORIYA LETTER AU */
    case 0x0B15: /* ORIYA LETTER KA */
    case 0x0B16: /* ORIYA LETTER KHA */
    case 0x0B17: /* ORIYA LETTER GA */
    case 0x0B18: /* ORIYA LETTER GHA */
    case 0x0B19: /* ORIYA LETTER NGA */
    case 0x0B1A: /* ORIYA LETTER CA */
    case 0x0B1B: /* ORIYA LETTER CHA */
    case 0x0B1C: /* ORIYA LETTER JA */
    case 0x0B1D: /* ORIYA LETTER JHA */
    case 0x0B1E: /* ORIYA LETTER NYA */
    case 0x0B1F: /* ORIYA LETTER TTA */
    case 0x0B20: /* ORIYA LETTER TTHA */
    case 0x0B21: /* ORIYA LETTER DDA */
    case 0x0B22: /* ORIYA LETTER DDHA */
    case 0x0B23: /* ORIYA LETTER NNA */
    case 0x0B24: /* ORIYA LETTER TA */
    case 0x0B25: /* ORIYA LETTER THA */
    case 0x0B26: /* ORIYA LETTER DA */
    case 0x0B27: /* ORIYA LETTER DHA */
    case 0x0B28: /* ORIYA LETTER NA */
    case 0x0B2A: /* ORIYA LETTER PA */
    case 0x0B2B: /* ORIYA LETTER PHA */
    case 0x0B2C: /* ORIYA LETTER BA */
    case 0x0B2D: /* ORIYA LETTER BHA */
    case 0x0B2E: /* ORIYA LETTER MA */
    case 0x0B2F: /* ORIYA LETTER YA */
    case 0x0B30: /* ORIYA LETTER RA */
    case 0x0B32: /* ORIYA LETTER LA */
    case 0x0B33: /* ORIYA LETTER LLA */
    case 0x0B36: /* ORIYA LETTER SHA */
    case 0x0B37: /* ORIYA LETTER SSA */
    case 0x0B38: /* ORIYA LETTER SA */
    case 0x0B39: /* ORIYA LETTER HA */
    case 0x0B3D: /* ORIYA SIGN AVAGRAHA */
    case 0x0B5C: /* ORIYA LETTER RRA */
    case 0x0B5D: /* ORIYA LETTER RHA */
    case 0x0B5F: /* ORIYA LETTER YYA */
    case 0x0B60: /* ORIYA LETTER VOCALIC RR */
    case 0x0B61: /* ORIYA LETTER VOCALIC LL */
    case 0x0B85: /* TAMIL LETTER A */
    case 0x0B86: /* TAMIL LETTER AA */
    case 0x0B87: /* TAMIL LETTER I */
    case 0x0B88: /* TAMIL LETTER II */
    case 0x0B89: /* TAMIL LETTER U */
    case 0x0B8A: /* TAMIL LETTER UU */
    case 0x0B8E: /* TAMIL LETTER E */
    case 0x0B8F: /* TAMIL LETTER EE */
    case 0x0B90: /* TAMIL LETTER AI */
    case 0x0B92: /* TAMIL LETTER O */
    case 0x0B93: /* TAMIL LETTER OO */
    case 0x0B94: /* TAMIL LETTER AU */
    case 0x0B95: /* TAMIL LETTER KA */
    case 0x0B99: /* TAMIL LETTER NGA */
    case 0x0B9A: /* TAMIL LETTER CA */
    case 0x0B9C: /* TAMIL LETTER JA */
    case 0x0B9E: /* TAMIL LETTER NYA */
    case 0x0B9F: /* TAMIL LETTER TTA */
    case 0x0BA3: /* TAMIL LETTER NNA */
    case 0x0BA4: /* TAMIL LETTER TA */
    case 0x0BA8: /* TAMIL LETTER NA */
    case 0x0BA9: /* TAMIL LETTER NNNA */
    case 0x0BAA: /* TAMIL LETTER PA */
    case 0x0BAE: /* TAMIL LETTER MA */
    case 0x0BAF: /* TAMIL LETTER YA */
    case 0x0BB0: /* TAMIL LETTER RA */
    case 0x0BB1: /* TAMIL LETTER RRA */
    case 0x0BB2: /* TAMIL LETTER LA */
    case 0x0BB3: /* TAMIL LETTER LLA */
    case 0x0BB4: /* TAMIL LETTER LLLA */
    case 0x0BB5: /* TAMIL LETTER VA */
    case 0x0BB7: /* TAMIL LETTER SSA */
    case 0x0BB8: /* TAMIL LETTER SA */
    case 0x0BB9: /* TAMIL LETTER HA */
    case 0x0C05: /* TELUGU LETTER A */
    case 0x0C06: /* TELUGU LETTER AA */
    case 0x0C07: /* TELUGU LETTER I */
    case 0x0C08: /* TELUGU LETTER II */
    case 0x0C09: /* TELUGU LETTER U */
    case 0x0C0A: /* TELUGU LETTER UU */
    case 0x0C0B: /* TELUGU LETTER VOCALIC R */
    case 0x0C0C: /* TELUGU LETTER VOCALIC L */
    case 0x0C0E: /* TELUGU LETTER E */
    case 0x0C0F: /* TELUGU LETTER EE */
    case 0x0C10: /* TELUGU LETTER AI */
    case 0x0C12: /* TELUGU LETTER O */
    case 0x0C13: /* TELUGU LETTER OO */
    case 0x0C14: /* TELUGU LETTER AU */
    case 0x0C15: /* TELUGU LETTER KA */
    case 0x0C16: /* TELUGU LETTER KHA */
    case 0x0C17: /* TELUGU LETTER GA */
    case 0x0C18: /* TELUGU LETTER GHA */
    case 0x0C19: /* TELUGU LETTER NGA */
    case 0x0C1A: /* TELUGU LETTER CA */
    case 0x0C1B: /* TELUGU LETTER CHA */
    case 0x0C1C: /* TELUGU LETTER JA */
    case 0x0C1D: /* TELUGU LETTER JHA */
    case 0x0C1E: /* TELUGU LETTER NYA */
    case 0x0C1F: /* TELUGU LETTER TTA */
    case 0x0C20: /* TELUGU LETTER TTHA */
    case 0x0C21: /* TELUGU LETTER DDA */
    case 0x0C22: /* TELUGU LETTER DDHA */
    case 0x0C23: /* TELUGU LETTER NNA */
    case 0x0C24: /* TELUGU LETTER TA */
    case 0x0C25: /* TELUGU LETTER THA */
    case 0x0C26: /* TELUGU LETTER DA */
    case 0x0C27: /* TELUGU LETTER DHA */
    case 0x0C28: /* TELUGU LETTER NA */
    case 0x0C2A: /* TELUGU LETTER PA */
    case 0x0C2B: /* TELUGU LETTER PHA */
    case 0x0C2C: /* TELUGU LETTER BA */
    case 0x0C2D: /* TELUGU LETTER BHA */
    case 0x0C2E: /* TELUGU LETTER MA */
    case 0x0C2F: /* TELUGU LETTER YA */
    case 0x0C30: /* TELUGU LETTER RA */
    case 0x0C31: /* TELUGU LETTER RRA */
    case 0x0C32: /* TELUGU LETTER LA */
    case 0x0C33: /* TELUGU LETTER LLA */
    case 0x0C35: /* TELUGU LETTER VA */
    case 0x0C36: /* TELUGU LETTER SHA */
    case 0x0C37: /* TELUGU LETTER SSA */
    case 0x0C38: /* TELUGU LETTER SA */
    case 0x0C39: /* TELUGU LETTER HA */
    case 0x0C60: /* TELUGU LETTER VOCALIC RR */
    case 0x0C61: /* TELUGU LETTER VOCALIC LL */
    case 0x0C85: /* KANNADA LETTER A */
    case 0x0C86: /* KANNADA LETTER AA */
    case 0x0C87: /* KANNADA LETTER I */
    case 0x0C88: /* KANNADA LETTER II */
    case 0x0C89: /* KANNADA LETTER U */
    case 0x0C8A: /* KANNADA LETTER UU */
    case 0x0C8B: /* KANNADA LETTER VOCALIC R */
    case 0x0C8C: /* KANNADA LETTER VOCALIC L */
    case 0x0C8E: /* KANNADA LETTER E */
    case 0x0C8F: /* KANNADA LETTER EE */
    case 0x0C90: /* KANNADA LETTER AI */
    case 0x0C92: /* KANNADA LETTER O */
    case 0x0C93: /* KANNADA LETTER OO */
    case 0x0C94: /* KANNADA LETTER AU */
    case 0x0C95: /* KANNADA LETTER KA */
    case 0x0C96: /* KANNADA LETTER KHA */
    case 0x0C97: /* KANNADA LETTER GA */
    case 0x0C98: /* KANNADA LETTER GHA */
    case 0x0C99: /* KANNADA LETTER NGA */
    case 0x0C9A: /* KANNADA LETTER CA */
    case 0x0C9B: /* KANNADA LETTER CHA */
    case 0x0C9C: /* KANNADA LETTER JA */
    case 0x0C9D: /* KANNADA LETTER JHA */
    case 0x0C9E: /* KANNADA LETTER NYA */
    case 0x0C9F: /* KANNADA LETTER TTA */
    case 0x0CA0: /* KANNADA LETTER TTHA */
    case 0x0CA1: /* KANNADA LETTER DDA */
    case 0x0CA2: /* KANNADA LETTER DDHA */
    case 0x0CA3: /* KANNADA LETTER NNA */
    case 0x0CA4: /* KANNADA LETTER TA */
    case 0x0CA5: /* KANNADA LETTER THA */
    case 0x0CA6: /* KANNADA LETTER DA */
    case 0x0CA7: /* KANNADA LETTER DHA */
    case 0x0CA8: /* KANNADA LETTER NA */
    case 0x0CAA: /* KANNADA LETTER PA */
    case 0x0CAB: /* KANNADA LETTER PHA */
    case 0x0CAC: /* KANNADA LETTER BA */
    case 0x0CAD: /* KANNADA LETTER BHA */
    case 0x0CAE: /* KANNADA LETTER MA */
    case 0x0CAF: /* KANNADA LETTER YA */
    case 0x0CB0: /* KANNADA LETTER RA */
    case 0x0CB1: /* KANNADA LETTER RRA */
    case 0x0CB2: /* KANNADA LETTER LA */
    case 0x0CB3: /* KANNADA LETTER LLA */
    case 0x0CB5: /* KANNADA LETTER VA */
    case 0x0CB6: /* KANNADA LETTER SHA */
    case 0x0CB7: /* KANNADA LETTER SSA */
    case 0x0CB8: /* KANNADA LETTER SA */
    case 0x0CB9: /* KANNADA LETTER HA */
    case 0x0CDE: /* KANNADA LETTER FA */
    case 0x0CE0: /* KANNADA LETTER VOCALIC RR */
    case 0x0CE1: /* KANNADA LETTER VOCALIC LL */
    case 0x0D05: /* MALAYALAM LETTER A */
    case 0x0D06: /* MALAYALAM LETTER AA */
    case 0x0D07: /* MALAYALAM LETTER I */
    case 0x0D08: /* MALAYALAM LETTER II */
    case 0x0D09: /* MALAYALAM LETTER U */
    case 0x0D0A: /* MALAYALAM LETTER UU */
    case 0x0D0B: /* MALAYALAM LETTER VOCALIC R */
    case 0x0D0C: /* MALAYALAM LETTER VOCALIC L */
    case 0x0D0E: /* MALAYALAM LETTER E */
    case 0x0D0F: /* MALAYALAM LETTER EE */
    case 0x0D10: /* MALAYALAM LETTER AI */
    case 0x0D12: /* MALAYALAM LETTER O */
    case 0x0D13: /* MALAYALAM LETTER OO */
    case 0x0D14: /* MALAYALAM LETTER AU */
    case 0x0D15: /* MALAYALAM LETTER KA */
    case 0x0D16: /* MALAYALAM LETTER KHA */
    case 0x0D17: /* MALAYALAM LETTER GA */
    case 0x0D18: /* MALAYALAM LETTER GHA */
    case 0x0D19: /* MALAYALAM LETTER NGA */
    case 0x0D1A: /* MALAYALAM LETTER CA */
    case 0x0D1B: /* MALAYALAM LETTER CHA */
    case 0x0D1C: /* MALAYALAM LETTER JA */
    case 0x0D1D: /* MALAYALAM LETTER JHA */
    case 0x0D1E: /* MALAYALAM LETTER NYA */
    case 0x0D1F: /* MALAYALAM LETTER TTA */
    case 0x0D20: /* MALAYALAM LETTER TTHA */
    case 0x0D21: /* MALAYALAM LETTER DDA */
    case 0x0D22: /* MALAYALAM LETTER DDHA */
    case 0x0D23: /* MALAYALAM LETTER NNA */
    case 0x0D24: /* MALAYALAM LETTER TA */
    case 0x0D25: /* MALAYALAM LETTER THA */
    case 0x0D26: /* MALAYALAM LETTER DA */
    case 0x0D27: /* MALAYALAM LETTER DHA */
    case 0x0D28: /* MALAYALAM LETTER NA */
    case 0x0D2A: /* MALAYALAM LETTER PA */
    case 0x0D2B: /* MALAYALAM LETTER PHA */
    case 0x0D2C: /* MALAYALAM LETTER BA */
    case 0x0D2D: /* MALAYALAM LETTER BHA */
    case 0x0D2E: /* MALAYALAM LETTER MA */
    case 0x0D2F: /* MALAYALAM LETTER YA */
    case 0x0D30: /* MALAYALAM LETTER RA */
    case 0x0D31: /* MALAYALAM LETTER RRA */
    case 0x0D32: /* MALAYALAM LETTER LA */
    case 0x0D33: /* MALAYALAM LETTER LLA */
    case 0x0D34: /* MALAYALAM LETTER LLLA */
    case 0x0D35: /* MALAYALAM LETTER VA */
    case 0x0D36: /* MALAYALAM LETTER SHA */
    case 0x0D37: /* MALAYALAM LETTER SSA */
    case 0x0D38: /* MALAYALAM LETTER SA */
    case 0x0D39: /* MALAYALAM LETTER HA */
    case 0x0D60: /* MALAYALAM LETTER VOCALIC RR */
    case 0x0D61: /* MALAYALAM LETTER VOCALIC LL */
    case 0x0D85: /* SINHALA LETTER AYANNA */
    case 0x0D86: /* SINHALA LETTER AAYANNA */
    case 0x0D87: /* SINHALA LETTER AEYANNA */
    case 0x0D88: /* SINHALA LETTER AEEYANNA */
    case 0x0D89: /* SINHALA LETTER IYANNA */
    case 0x0D8A: /* SINHALA LETTER IIYANNA */
    case 0x0D8B: /* SINHALA LETTER UYANNA */
    case 0x0D8C: /* SINHALA LETTER UUYANNA */
    case 0x0D8D: /* SINHALA LETTER IRUYANNA */
    case 0x0D8E: /* SINHALA LETTER IRUUYANNA */
    case 0x0D8F: /* SINHALA LETTER ILUYANNA */
    case 0x0D90: /* SINHALA LETTER ILUUYANNA */
    case 0x0D91: /* SINHALA LETTER EYANNA */
    case 0x0D92: /* SINHALA LETTER EEYANNA */
    case 0x0D93: /* SINHALA LETTER AIYANNA */
    case 0x0D94: /* SINHALA LETTER OYANNA */
    case 0x0D95: /* SINHALA LETTER OOYANNA */
    case 0x0D96: /* SINHALA LETTER AUYANNA */
    case 0x0D9A: /* SINHALA LETTER ALPAPRAANA KAYANNA */
    case 0x0D9B: /* SINHALA LETTER MAHAAPRAANA KAYANNA */
    case 0x0D9C: /* SINHALA LETTER ALPAPRAANA GAYANNA */
    case 0x0D9D: /* SINHALA LETTER MAHAAPRAANA GAYANNA */
    case 0x0D9E: /* SINHALA LETTER KANTAJA NAASIKYAYA */
    case 0x0D9F: /* SINHALA LETTER SANYAKA GAYANNA */
    case 0x0DA0: /* SINHALA LETTER ALPAPRAANA CAYANNA */
    case 0x0DA1: /* SINHALA LETTER MAHAAPRAANA CAYANNA */
    case 0x0DA2: /* SINHALA LETTER ALPAPRAANA JAYANNA */
    case 0x0DA3: /* SINHALA LETTER MAHAAPRAANA JAYANNA */
    case 0x0DA4: /* SINHALA LETTER TAALUJA NAASIKYAYA */
    case 0x0DA5: /* SINHALA LETTER TAALUJA SANYOOGA NAAKSIKYAYA */
    case 0x0DA6: /* SINHALA LETTER SANYAKA JAYANNA */
    case 0x0DA7: /* SINHALA LETTER ALPAPRAANA TTAYANNA */
    case 0x0DA8: /* SINHALA LETTER MAHAAPRAANA TTAYANNA */
    case 0x0DA9: /* SINHALA LETTER ALPAPRAANA DDAYANNA */
    case 0x0DAA: /* SINHALA LETTER MAHAAPRAANA DDAYANNA */
    case 0x0DAB: /* SINHALA LETTER MUURDHAJA NAYANNA */
    case 0x0DAC: /* SINHALA LETTER SANYAKA DDAYANNA */
    case 0x0DAD: /* SINHALA LETTER ALPAPRAANA TAYANNA */
    case 0x0DAE: /* SINHALA LETTER MAHAAPRAANA TAYANNA */
    case 0x0DAF: /* SINHALA LETTER ALPAPRAANA DAYANNA */
    case 0x0DB0: /* SINHALA LETTER MAHAAPRAANA DAYANNA */
    case 0x0DB1: /* SINHALA LETTER DANTAJA NAYANNA */
    case 0x0DB3: /* SINHALA LETTER SANYAKA DAYANNA */
    case 0x0DB4: /* SINHALA LETTER ALPAPRAANA PAYANNA */
    case 0x0DB5: /* SINHALA LETTER MAHAAPRAANA PAYANNA */
    case 0x0DB6: /* SINHALA LETTER ALPAPRAANA BAYANNA */
    case 0x0DB7: /* SINHALA LETTER MAHAAPRAANA BAYANNA */
    case 0x0DB8: /* SINHALA LETTER MAYANNA */
    case 0x0DB9: /* SINHALA LETTER AMBA BAYANNA */
    case 0x0DBA: /* SINHALA LETTER YAYANNA */
    case 0x0DBB: /* SINHALA LETTER RAYANNA */
    case 0x0DBD: /* SINHALA LETTER DANTAJA LAYANNA */
    case 0x0DC0: /* SINHALA LETTER VAYANNA */
    case 0x0DC1: /* SINHALA LETTER TAALUJA SAYANNA */
    case 0x0DC2: /* SINHALA LETTER MUURDHAJA SAYANNA */
    case 0x0DC3: /* SINHALA LETTER DANTAJA SAYANNA */
    case 0x0DC4: /* SINHALA LETTER HAYANNA */
    case 0x0DC5: /* SINHALA LETTER MUURDHAJA LAYANNA */
    case 0x0DC6: /* SINHALA LETTER FAYANNA */
    case 0x0E01: /* THAI CHARACTER KO KAI */
    case 0x0E02: /* THAI CHARACTER KHO KHAI */
    case 0x0E03: /* THAI CHARACTER KHO KHUAT */
    case 0x0E04: /* THAI CHARACTER KHO KHWAI */
    case 0x0E05: /* THAI CHARACTER KHO KHON */
    case 0x0E06: /* THAI CHARACTER KHO RAKHANG */
    case 0x0E07: /* THAI CHARACTER NGO NGU */
    case 0x0E08: /* THAI CHARACTER CHO CHAN */
    case 0x0E09: /* THAI CHARACTER CHO CHING */
    case 0x0E0A: /* THAI CHARACTER CHO CHANG */
    case 0x0E0B: /* THAI CHARACTER SO SO */
    case 0x0E0C: /* THAI CHARACTER CHO CHOE */
    case 0x0E0D: /* THAI CHARACTER YO YING */
    case 0x0E0E: /* THAI CHARACTER DO CHADA */
    case 0x0E0F: /* THAI CHARACTER TO PATAK */
    case 0x0E10: /* THAI CHARACTER THO THAN */
    case 0x0E11: /* THAI CHARACTER THO NANGMONTHO */
    case 0x0E12: /* THAI CHARACTER THO PHUTHAO */
    case 0x0E13: /* THAI CHARACTER NO NEN */
    case 0x0E14: /* THAI CHARACTER DO DEK */
    case 0x0E15: /* THAI CHARACTER TO TAO */
    case 0x0E16: /* THAI CHARACTER THO THUNG */
    case 0x0E17: /* THAI CHARACTER THO THAHAN */
    case 0x0E18: /* THAI CHARACTER THO THONG */
    case 0x0E19: /* THAI CHARACTER NO NU */
    case 0x0E1A: /* THAI CHARACTER BO BAIMAI */
    case 0x0E1B: /* THAI CHARACTER PO PLA */
    case 0x0E1C: /* THAI CHARACTER PHO PHUNG */
    case 0x0E1D: /* THAI CHARACTER FO FA */
    case 0x0E1E: /* THAI CHARACTER PHO PHAN */
    case 0x0E1F: /* THAI CHARACTER FO FAN */
    case 0x0E20: /* THAI CHARACTER PHO SAMPHAO */
    case 0x0E21: /* THAI CHARACTER MO MA */
    case 0x0E22: /* THAI CHARACTER YO YAK */
    case 0x0E23: /* THAI CHARACTER RO RUA */
    case 0x0E24: /* THAI CHARACTER RU */
    case 0x0E25: /* THAI CHARACTER LO LING */
    case 0x0E26: /* THAI CHARACTER LU */
    case 0x0E27: /* THAI CHARACTER WO WAEN */
    case 0x0E28: /* THAI CHARACTER SO SALA */
    case 0x0E29: /* THAI CHARACTER SO RUSI */
    case 0x0E2A: /* THAI CHARACTER SO SUA */
    case 0x0E2B: /* THAI CHARACTER HO HIP */
    case 0x0E2C: /* THAI CHARACTER LO CHULA */
    case 0x0E2D: /* THAI CHARACTER O ANG */
    case 0x0E2E: /* THAI CHARACTER HO NOKHUK */
    case 0x0E2F: /* THAI CHARACTER PAIYANNOI */
    case 0x0E30: /* THAI CHARACTER SARA A */
    case 0x0E32: /* THAI CHARACTER SARA AA */
    case 0x0E33: /* THAI CHARACTER SARA AM */
    case 0x0E40: /* THAI CHARACTER SARA E */
    case 0x0E41: /* THAI CHARACTER SARA AE */
    case 0x0E42: /* THAI CHARACTER SARA O */
    case 0x0E43: /* THAI CHARACTER SARA AI MAIMUAN */
    case 0x0E44: /* THAI CHARACTER SARA AI MAIMALAI */
    case 0x0E45: /* THAI CHARACTER LAKKHANGYAO */
    case 0x0E46: /* THAI CHARACTER MAIYAMOK */
    case 0x0E81: /* LAO LETTER KO */
    case 0x0E82: /* LAO LETTER KHO SUNG */
    case 0x0E84: /* LAO LETTER KHO TAM */
    case 0x0E87: /* LAO LETTER NGO */
    case 0x0E88: /* LAO LETTER CO */
    case 0x0E8A: /* LAO LETTER SO TAM */
    case 0x0E8D: /* LAO LETTER NYO */
    case 0x0E94: /* LAO LETTER DO */
    case 0x0E95: /* LAO LETTER TO */
    case 0x0E96: /* LAO LETTER THO SUNG */
    case 0x0E97: /* LAO LETTER THO TAM */
    case 0x0E99: /* LAO LETTER NO */
    case 0x0E9A: /* LAO LETTER BO */
    case 0x0E9B: /* LAO LETTER PO */
    case 0x0E9C: /* LAO LETTER PHO SUNG */
    case 0x0E9D: /* LAO LETTER FO TAM */
    case 0x0E9E: /* LAO LETTER PHO TAM */
    case 0x0E9F: /* LAO LETTER FO SUNG */
    case 0x0EA1: /* LAO LETTER MO */
    case 0x0EA2: /* LAO LETTER YO */
    case 0x0EA3: /* LAO LETTER LO LING */
    case 0x0EA5: /* LAO LETTER LO LOOT */
    case 0x0EA7: /* LAO LETTER WO */
    case 0x0EAA: /* LAO LETTER SO SUNG */
    case 0x0EAB: /* LAO LETTER HO SUNG */
    case 0x0EAD: /* LAO LETTER O */
    case 0x0EAE: /* LAO LETTER HO TAM */
    case 0x0EAF: /* LAO ELLIPSIS */
    case 0x0EB0: /* LAO VOWEL SIGN A */
    case 0x0EB2: /* LAO VOWEL SIGN AA */
    case 0x0EB3: /* LAO VOWEL SIGN AM */
    case 0x0EBD: /* LAO SEMIVOWEL SIGN NYO */
    case 0x0EC0: /* LAO VOWEL SIGN E */
    case 0x0EC1: /* LAO VOWEL SIGN EI */
    case 0x0EC2: /* LAO VOWEL SIGN O */
    case 0x0EC3: /* LAO VOWEL SIGN AY */
    case 0x0EC4: /* LAO VOWEL SIGN AI */
    case 0x0EC6: /* LAO KO LA */
    case 0x0EDC: /* LAO HO NO */
    case 0x0EDD: /* LAO HO MO */
    case 0x0F00: /* TIBETAN SYLLABLE OM */
    case 0x0F40: /* TIBETAN LETTER KA */
    case 0x0F41: /* TIBETAN LETTER KHA */
    case 0x0F42: /* TIBETAN LETTER GA */
    case 0x0F43: /* TIBETAN LETTER GHA */
    case 0x0F44: /* TIBETAN LETTER NGA */
    case 0x0F45: /* TIBETAN LETTER CA */
    case 0x0F46: /* TIBETAN LETTER CHA */
    case 0x0F47: /* TIBETAN LETTER JA */
    case 0x0F49: /* TIBETAN LETTER NYA */
    case 0x0F4A: /* TIBETAN LETTER TTA */
    case 0x0F4B: /* TIBETAN LETTER TTHA */
    case 0x0F4C: /* TIBETAN LETTER DDA */
    case 0x0F4D: /* TIBETAN LETTER DDHA */
    case 0x0F4E: /* TIBETAN LETTER NNA */
    case 0x0F4F: /* TIBETAN LETTER TA */
    case 0x0F50: /* TIBETAN LETTER THA */
    case 0x0F51: /* TIBETAN LETTER DA */
    case 0x0F52: /* TIBETAN LETTER DHA */
    case 0x0F53: /* TIBETAN LETTER NA */
    case 0x0F54: /* TIBETAN LETTER PA */
    case 0x0F55: /* TIBETAN LETTER PHA */
    case 0x0F56: /* TIBETAN LETTER BA */
    case 0x0F57: /* TIBETAN LETTER BHA */
    case 0x0F58: /* TIBETAN LETTER MA */
    case 0x0F59: /* TIBETAN LETTER TSA */
    case 0x0F5A: /* TIBETAN LETTER TSHA */
    case 0x0F5B: /* TIBETAN LETTER DZA */
    case 0x0F5C: /* TIBETAN LETTER DZHA */
    case 0x0F5D: /* TIBETAN LETTER WA */
    case 0x0F5E: /* TIBETAN LETTER ZHA */
    case 0x0F5F: /* TIBETAN LETTER ZA */
    case 0x0F60: /* TIBETAN LETTER -A */
    case 0x0F61: /* TIBETAN LETTER YA */
    case 0x0F62: /* TIBETAN LETTER RA */
    case 0x0F63: /* TIBETAN LETTER LA */
    case 0x0F64: /* TIBETAN LETTER SHA */
    case 0x0F65: /* TIBETAN LETTER SSA */
    case 0x0F66: /* TIBETAN LETTER SA */
    case 0x0F67: /* TIBETAN LETTER HA */
    case 0x0F68: /* TIBETAN LETTER A */
    case 0x0F69: /* TIBETAN LETTER KSSA */
    case 0x0F6A: /* TIBETAN LETTER FIXED-FORM RA */
    case 0x0F88: /* TIBETAN SIGN LCE TSA CAN */
    case 0x0F89: /* TIBETAN SIGN MCHU CAN */
    case 0x0F8A: /* TIBETAN SIGN GRU CAN RGYINGS */
    case 0x0F8B: /* TIBETAN SIGN GRU MED RGYINGS */
    case 0x1000: /* MYANMAR LETTER KA */
    case 0x1001: /* MYANMAR LETTER KHA */
    case 0x1002: /* MYANMAR LETTER GA */
    case 0x1003: /* MYANMAR LETTER GHA */
    case 0x1004: /* MYANMAR LETTER NGA */
    case 0x1005: /* MYANMAR LETTER CA */
    case 0x1006: /* MYANMAR LETTER CHA */
    case 0x1007: /* MYANMAR LETTER JA */
    case 0x1008: /* MYANMAR LETTER JHA */
    case 0x1009: /* MYANMAR LETTER NYA */
    case 0x100A: /* MYANMAR LETTER NNYA */
    case 0x100B: /* MYANMAR LETTER TTA */
    case 0x100C: /* MYANMAR LETTER TTHA */
    case 0x100D: /* MYANMAR LETTER DDA */
    case 0x100E: /* MYANMAR LETTER DDHA */
    case 0x100F: /* MYANMAR LETTER NNA */
    case 0x1010: /* MYANMAR LETTER TA */
    case 0x1011: /* MYANMAR LETTER THA */
    case 0x1012: /* MYANMAR LETTER DA */
    case 0x1013: /* MYANMAR LETTER DHA */
    case 0x1014: /* MYANMAR LETTER NA */
    case 0x1015: /* MYANMAR LETTER PA */
    case 0x1016: /* MYANMAR LETTER PHA */
    case 0x1017: /* MYANMAR LETTER BA */
    case 0x1018: /* MYANMAR LETTER BHA */
    case 0x1019: /* MYANMAR LETTER MA */
    case 0x101A: /* MYANMAR LETTER YA */
    case 0x101B: /* MYANMAR LETTER RA */
    case 0x101C: /* MYANMAR LETTER LA */
    case 0x101D: /* MYANMAR LETTER WA */
    case 0x101E: /* MYANMAR LETTER SA */
    case 0x101F: /* MYANMAR LETTER HA */
    case 0x1020: /* MYANMAR LETTER LLA */
    case 0x1021: /* MYANMAR LETTER A */
    case 0x1023: /* MYANMAR LETTER I */
    case 0x1024: /* MYANMAR LETTER II */
BREAK_SWITCH_UP
    case 0x1025: /* MYANMAR LETTER U */
    case 0x1026: /* MYANMAR LETTER UU */
    case 0x1027: /* MYANMAR LETTER E */
    case 0x1029: /* MYANMAR LETTER O */
    case 0x102A: /* MYANMAR LETTER AU */
    case 0x1050: /* MYANMAR LETTER SHA */
    case 0x1051: /* MYANMAR LETTER SSA */
    case 0x1052: /* MYANMAR LETTER VOCALIC R */
    case 0x1053: /* MYANMAR LETTER VOCALIC RR */
    case 0x1054: /* MYANMAR LETTER VOCALIC L */
    case 0x1055: /* MYANMAR LETTER VOCALIC LL */
    case 0x10D0: /* GEORGIAN LETTER AN */
    case 0x10D1: /* GEORGIAN LETTER BAN */
    case 0x10D2: /* GEORGIAN LETTER GAN */
    case 0x10D3: /* GEORGIAN LETTER DON */
    case 0x10D4: /* GEORGIAN LETTER EN */
    case 0x10D5: /* GEORGIAN LETTER VIN */
    case 0x10D6: /* GEORGIAN LETTER ZEN */
    case 0x10D7: /* GEORGIAN LETTER TAN */
    case 0x10D8: /* GEORGIAN LETTER IN */
    case 0x10D9: /* GEORGIAN LETTER KAN */
    case 0x10DA: /* GEORGIAN LETTER LAS */
    case 0x10DB: /* GEORGIAN LETTER MAN */
    case 0x10DC: /* GEORGIAN LETTER NAR */
    case 0x10DD: /* GEORGIAN LETTER ON */
    case 0x10DE: /* GEORGIAN LETTER PAR */
    case 0x10DF: /* GEORGIAN LETTER ZHAR */
    case 0x10E0: /* GEORGIAN LETTER RAE */
    case 0x10E1: /* GEORGIAN LETTER SAN */
    case 0x10E2: /* GEORGIAN LETTER TAR */
    case 0x10E3: /* GEORGIAN LETTER UN */
    case 0x10E4: /* GEORGIAN LETTER PHAR */
    case 0x10E5: /* GEORGIAN LETTER KHAR */
    case 0x10E6: /* GEORGIAN LETTER GHAN */
    case 0x10E7: /* GEORGIAN LETTER QAR */
    case 0x10E8: /* GEORGIAN LETTER SHIN */
    case 0x10E9: /* GEORGIAN LETTER CHIN */
    case 0x10EA: /* GEORGIAN LETTER CAN */
    case 0x10EB: /* GEORGIAN LETTER JIL */
    case 0x10EC: /* GEORGIAN LETTER CIL */
    case 0x10ED: /* GEORGIAN LETTER CHAR */
    case 0x10EE: /* GEORGIAN LETTER XAN */
    case 0x10EF: /* GEORGIAN LETTER JHAN */
    case 0x10F0: /* GEORGIAN LETTER HAE */
    case 0x10F1: /* GEORGIAN LETTER HE */
    case 0x10F2: /* GEORGIAN LETTER HIE */
    case 0x10F3: /* GEORGIAN LETTER WE */
    case 0x10F4: /* GEORGIAN LETTER HAR */
    case 0x10F5: /* GEORGIAN LETTER HOE */
    case 0x10F6: /* GEORGIAN LETTER FI */
    case 0x1100: /* HANGUL CHOSEONG KIYEOK */
    case 0x1101: /* HANGUL CHOSEONG SSANGKIYEOK */
    case 0x1102: /* HANGUL CHOSEONG NIEUN */
    case 0x1103: /* HANGUL CHOSEONG TIKEUT */
    case 0x1104: /* HANGUL CHOSEONG SSANGTIKEUT */
    case 0x1105: /* HANGUL CHOSEONG RIEUL */
    case 0x1106: /* HANGUL CHOSEONG MIEUM */
    case 0x1107: /* HANGUL CHOSEONG PIEUP */
    case 0x1108: /* HANGUL CHOSEONG SSANGPIEUP */
    case 0x1109: /* HANGUL CHOSEONG SIOS */
    case 0x110A: /* HANGUL CHOSEONG SSANGSIOS */
    case 0x110B: /* HANGUL CHOSEONG IEUNG */
    case 0x110C: /* HANGUL CHOSEONG CIEUC */
    case 0x110D: /* HANGUL CHOSEONG SSANGCIEUC */
    case 0x110E: /* HANGUL CHOSEONG CHIEUCH */
    case 0x110F: /* HANGUL CHOSEONG KHIEUKH */
    case 0x1110: /* HANGUL CHOSEONG THIEUTH */
    case 0x1111: /* HANGUL CHOSEONG PHIEUPH */
    case 0x1112: /* HANGUL CHOSEONG HIEUH */
    case 0x1113: /* HANGUL CHOSEONG NIEUN-KIYEOK */
    case 0x1114: /* HANGUL CHOSEONG SSANGNIEUN */
    case 0x1115: /* HANGUL CHOSEONG NIEUN-TIKEUT */
    case 0x1116: /* HANGUL CHOSEONG NIEUN-PIEUP */
    case 0x1117: /* HANGUL CHOSEONG TIKEUT-KIYEOK */
    case 0x1118: /* HANGUL CHOSEONG RIEUL-NIEUN */
    case 0x1119: /* HANGUL CHOSEONG SSANGRIEUL */
    case 0x111A: /* HANGUL CHOSEONG RIEUL-HIEUH */
    case 0x111B: /* HANGUL CHOSEONG KAPYEOUNRIEUL */
    case 0x111C: /* HANGUL CHOSEONG MIEUM-PIEUP */
    case 0x111D: /* HANGUL CHOSEONG KAPYEOUNMIEUM */
    case 0x111E: /* HANGUL CHOSEONG PIEUP-KIYEOK */
    case 0x111F: /* HANGUL CHOSEONG PIEUP-NIEUN */
    case 0x1120: /* HANGUL CHOSEONG PIEUP-TIKEUT */
    case 0x1121: /* HANGUL CHOSEONG PIEUP-SIOS */
    case 0x1122: /* HANGUL CHOSEONG PIEUP-SIOS-KIYEOK */
    case 0x1123: /* HANGUL CHOSEONG PIEUP-SIOS-TIKEUT */
    case 0x1124: /* HANGUL CHOSEONG PIEUP-SIOS-PIEUP */
    case 0x1125: /* HANGUL CHOSEONG PIEUP-SSANGSIOS */
    case 0x1126: /* HANGUL CHOSEONG PIEUP-SIOS-CIEUC */
    case 0x1127: /* HANGUL CHOSEONG PIEUP-CIEUC */
    case 0x1128: /* HANGUL CHOSEONG PIEUP-CHIEUCH */
    case 0x1129: /* HANGUL CHOSEONG PIEUP-THIEUTH */
    case 0x112A: /* HANGUL CHOSEONG PIEUP-PHIEUPH */
    case 0x112B: /* HANGUL CHOSEONG KAPYEOUNPIEUP */
    case 0x112C: /* HANGUL CHOSEONG KAPYEOUNSSANGPIEUP */
    case 0x112D: /* HANGUL CHOSEONG SIOS-KIYEOK */
    case 0x112E: /* HANGUL CHOSEONG SIOS-NIEUN */
    case 0x112F: /* HANGUL CHOSEONG SIOS-TIKEUT */
    case 0x1130: /* HANGUL CHOSEONG SIOS-RIEUL */
    case 0x1131: /* HANGUL CHOSEONG SIOS-MIEUM */
    case 0x1132: /* HANGUL CHOSEONG SIOS-PIEUP */
    case 0x1133: /* HANGUL CHOSEONG SIOS-PIEUP-KIYEOK */
    case 0x1134: /* HANGUL CHOSEONG SIOS-SSANGSIOS */
    case 0x1135: /* HANGUL CHOSEONG SIOS-IEUNG */
    case 0x1136: /* HANGUL CHOSEONG SIOS-CIEUC */
    case 0x1137: /* HANGUL CHOSEONG SIOS-CHIEUCH */
    case 0x1138: /* HANGUL CHOSEONG SIOS-KHIEUKH */
    case 0x1139: /* HANGUL CHOSEONG SIOS-THIEUTH */
    case 0x113A: /* HANGUL CHOSEONG SIOS-PHIEUPH */
    case 0x113B: /* HANGUL CHOSEONG SIOS-HIEUH */
    case 0x113C: /* HANGUL CHOSEONG CHITUEUMSIOS */
    case 0x113D: /* HANGUL CHOSEONG CHITUEUMSSANGSIOS */
    case 0x113E: /* HANGUL CHOSEONG CEONGCHIEUMSIOS */
    case 0x113F: /* HANGUL CHOSEONG CEONGCHIEUMSSANGSIOS */
    case 0x1140: /* HANGUL CHOSEONG PANSIOS */
    case 0x1141: /* HANGUL CHOSEONG IEUNG-KIYEOK */
    case 0x1142: /* HANGUL CHOSEONG IEUNG-TIKEUT */
    case 0x1143: /* HANGUL CHOSEONG IEUNG-MIEUM */
    case 0x1144: /* HANGUL CHOSEONG IEUNG-PIEUP */
    case 0x1145: /* HANGUL CHOSEONG IEUNG-SIOS */
    case 0x1146: /* HANGUL CHOSEONG IEUNG-PANSIOS */
    case 0x1147: /* HANGUL CHOSEONG SSANGIEUNG */
    case 0x1148: /* HANGUL CHOSEONG IEUNG-CIEUC */
    case 0x1149: /* HANGUL CHOSEONG IEUNG-CHIEUCH */
    case 0x114A: /* HANGUL CHOSEONG IEUNG-THIEUTH */
    case 0x114B: /* HANGUL CHOSEONG IEUNG-PHIEUPH */
    case 0x114C: /* HANGUL CHOSEONG YESIEUNG */
    case 0x114D: /* HANGUL CHOSEONG CIEUC-IEUNG */
    case 0x114E: /* HANGUL CHOSEONG CHITUEUMCIEUC */
    case 0x114F: /* HANGUL CHOSEONG CHITUEUMSSANGCIEUC */
    case 0x1150: /* HANGUL CHOSEONG CEONGCHIEUMCIEUC */
    case 0x1151: /* HANGUL CHOSEONG CEONGCHIEUMSSANGCIEUC */
    case 0x1152: /* HANGUL CHOSEONG CHIEUCH-KHIEUKH */
    case 0x1153: /* HANGUL CHOSEONG CHIEUCH-HIEUH */
    case 0x1154: /* HANGUL CHOSEONG CHITUEUMCHIEUCH */
    case 0x1155: /* HANGUL CHOSEONG CEONGCHIEUMCHIEUCH */
    case 0x1156: /* HANGUL CHOSEONG PHIEUPH-PIEUP */
    case 0x1157: /* HANGUL CHOSEONG KAPYEOUNPHIEUPH */
    case 0x1158: /* HANGUL CHOSEONG SSANGHIEUH */
    case 0x1159: /* HANGUL CHOSEONG YEORINHIEUH */
    case 0x115F: /* HANGUL CHOSEONG FILLER */
    case 0x1160: /* HANGUL JUNGSEONG FILLER */
    case 0x1161: /* HANGUL JUNGSEONG A */
    case 0x1162: /* HANGUL JUNGSEONG AE */
    case 0x1163: /* HANGUL JUNGSEONG YA */
    case 0x1164: /* HANGUL JUNGSEONG YAE */
    case 0x1165: /* HANGUL JUNGSEONG EO */
    case 0x1166: /* HANGUL JUNGSEONG E */
    case 0x1167: /* HANGUL JUNGSEONG YEO */
    case 0x1168: /* HANGUL JUNGSEONG YE */
    case 0x1169: /* HANGUL JUNGSEONG O */
    case 0x116A: /* HANGUL JUNGSEONG WA */
    case 0x116B: /* HANGUL JUNGSEONG WAE */
    case 0x116C: /* HANGUL JUNGSEONG OE */
    case 0x116D: /* HANGUL JUNGSEONG YO */
    case 0x116E: /* HANGUL JUNGSEONG U */
    case 0x116F: /* HANGUL JUNGSEONG WEO */
    case 0x1170: /* HANGUL JUNGSEONG WE */
    case 0x1171: /* HANGUL JUNGSEONG WI */
    case 0x1172: /* HANGUL JUNGSEONG YU */
    case 0x1173: /* HANGUL JUNGSEONG EU */
    case 0x1174: /* HANGUL JUNGSEONG YI */
    case 0x1175: /* HANGUL JUNGSEONG I */
    case 0x1176: /* HANGUL JUNGSEONG A-O */
    case 0x1177: /* HANGUL JUNGSEONG A-U */
    case 0x1178: /* HANGUL JUNGSEONG YA-O */
    case 0x1179: /* HANGUL JUNGSEONG YA-YO */
    case 0x117A: /* HANGUL JUNGSEONG EO-O */
    case 0x117B: /* HANGUL JUNGSEONG EO-U */
    case 0x117C: /* HANGUL JUNGSEONG EO-EU */
    case 0x117D: /* HANGUL JUNGSEONG YEO-O */
    case 0x117E: /* HANGUL JUNGSEONG YEO-U */
    case 0x117F: /* HANGUL JUNGSEONG O-EO */
    case 0x1180: /* HANGUL JUNGSEONG O-E */
    case 0x1181: /* HANGUL JUNGSEONG O-YE */
    case 0x1182: /* HANGUL JUNGSEONG O-O */
    case 0x1183: /* HANGUL JUNGSEONG O-U */
    case 0x1184: /* HANGUL JUNGSEONG YO-YA */
    case 0x1185: /* HANGUL JUNGSEONG YO-YAE */
    case 0x1186: /* HANGUL JUNGSEONG YO-YEO */
    case 0x1187: /* HANGUL JUNGSEONG YO-O */
    case 0x1188: /* HANGUL JUNGSEONG YO-I */
    case 0x1189: /* HANGUL JUNGSEONG U-A */
    case 0x118A: /* HANGUL JUNGSEONG U-AE */
    case 0x118B: /* HANGUL JUNGSEONG U-EO-EU */
    case 0x118C: /* HANGUL JUNGSEONG U-YE */
    case 0x118D: /* HANGUL JUNGSEONG U-U */
    case 0x118E: /* HANGUL JUNGSEONG YU-A */
    case 0x118F: /* HANGUL JUNGSEONG YU-EO */
    case 0x1190: /* HANGUL JUNGSEONG YU-E */
    case 0x1191: /* HANGUL JUNGSEONG YU-YEO */
    case 0x1192: /* HANGUL JUNGSEONG YU-YE */
    case 0x1193: /* HANGUL JUNGSEONG YU-U */
    case 0x1194: /* HANGUL JUNGSEONG YU-I */
    case 0x1195: /* HANGUL JUNGSEONG EU-U */
    case 0x1196: /* HANGUL JUNGSEONG EU-EU */
    case 0x1197: /* HANGUL JUNGSEONG YI-U */
    case 0x1198: /* HANGUL JUNGSEONG I-A */
    case 0x1199: /* HANGUL JUNGSEONG I-YA */
    case 0x119A: /* HANGUL JUNGSEONG I-O */
    case 0x119B: /* HANGUL JUNGSEONG I-U */
    case 0x119C: /* HANGUL JUNGSEONG I-EU */
    case 0x119D: /* HANGUL JUNGSEONG I-ARAEA */
    case 0x119E: /* HANGUL JUNGSEONG ARAEA */
    case 0x119F: /* HANGUL JUNGSEONG ARAEA-EO */
    case 0x11A0: /* HANGUL JUNGSEONG ARAEA-U */
    case 0x11A1: /* HANGUL JUNGSEONG ARAEA-I */
    case 0x11A2: /* HANGUL JUNGSEONG SSANGARAEA */
    case 0x11A8: /* HANGUL JONGSEONG KIYEOK */
    case 0x11A9: /* HANGUL JONGSEONG SSANGKIYEOK */
    case 0x11AA: /* HANGUL JONGSEONG KIYEOK-SIOS */
    case 0x11AB: /* HANGUL JONGSEONG NIEUN */
    case 0x11AC: /* HANGUL JONGSEONG NIEUN-CIEUC */
    case 0x11AD: /* HANGUL JONGSEONG NIEUN-HIEUH */
    case 0x11AE: /* HANGUL JONGSEONG TIKEUT */
    case 0x11AF: /* HANGUL JONGSEONG RIEUL */
    case 0x11B0: /* HANGUL JONGSEONG RIEUL-KIYEOK */
    case 0x11B1: /* HANGUL JONGSEONG RIEUL-MIEUM */
    case 0x11B2: /* HANGUL JONGSEONG RIEUL-PIEUP */
    case 0x11B3: /* HANGUL JONGSEONG RIEUL-SIOS */
    case 0x11B4: /* HANGUL JONGSEONG RIEUL-THIEUTH */
    case 0x11B5: /* HANGUL JONGSEONG RIEUL-PHIEUPH */
    case 0x11B6: /* HANGUL JONGSEONG RIEUL-HIEUH */
    case 0x11B7: /* HANGUL JONGSEONG MIEUM */
    case 0x11B8: /* HANGUL JONGSEONG PIEUP */
    case 0x11B9: /* HANGUL JONGSEONG PIEUP-SIOS */
    case 0x11BA: /* HANGUL JONGSEONG SIOS */
    case 0x11BB: /* HANGUL JONGSEONG SSANGSIOS */
    case 0x11BC: /* HANGUL JONGSEONG IEUNG */
    case 0x11BD: /* HANGUL JONGSEONG CIEUC */
    case 0x11BE: /* HANGUL JONGSEONG CHIEUCH */
    case 0x11BF: /* HANGUL JONGSEONG KHIEUKH */
    case 0x11C0: /* HANGUL JONGSEONG THIEUTH */
    case 0x11C1: /* HANGUL JONGSEONG PHIEUPH */
    case 0x11C2: /* HANGUL JONGSEONG HIEUH */
    case 0x11C3: /* HANGUL JONGSEONG KIYEOK-RIEUL */
    case 0x11C4: /* HANGUL JONGSEONG KIYEOK-SIOS-KIYEOK */
    case 0x11C5: /* HANGUL JONGSEONG NIEUN-KIYEOK */
    case 0x11C6: /* HANGUL JONGSEONG NIEUN-TIKEUT */
    case 0x11C7: /* HANGUL JONGSEONG NIEUN-SIOS */
    case 0x11C8: /* HANGUL JONGSEONG NIEUN-PANSIOS */
    case 0x11C9: /* HANGUL JONGSEONG NIEUN-THIEUTH */
    case 0x11CA: /* HANGUL JONGSEONG TIKEUT-KIYEOK */
    case 0x11CB: /* HANGUL JONGSEONG TIKEUT-RIEUL */
    case 0x11CC: /* HANGUL JONGSEONG RIEUL-KIYEOK-SIOS */
    case 0x11CD: /* HANGUL JONGSEONG RIEUL-NIEUN */
    case 0x11CE: /* HANGUL JONGSEONG RIEUL-TIKEUT */
    case 0x11CF: /* HANGUL JONGSEONG RIEUL-TIKEUT-HIEUH */
    case 0x11D0: /* HANGUL JONGSEONG SSANGRIEUL */
    case 0x11D1: /* HANGUL JONGSEONG RIEUL-MIEUM-KIYEOK */
    case 0x11D2: /* HANGUL JONGSEONG RIEUL-MIEUM-SIOS */
    case 0x11D3: /* HANGUL JONGSEONG RIEUL-PIEUP-SIOS */
    case 0x11D4: /* HANGUL JONGSEONG RIEUL-PIEUP-HIEUH */
    case 0x11D5: /* HANGUL JONGSEONG RIEUL-KAPYEOUNPIEUP */
    case 0x11D6: /* HANGUL JONGSEONG RIEUL-SSANGSIOS */
    case 0x11D7: /* HANGUL JONGSEONG RIEUL-PANSIOS */
    case 0x11D8: /* HANGUL JONGSEONG RIEUL-KHIEUKH */
    case 0x11D9: /* HANGUL JONGSEONG RIEUL-YEORINHIEUH */
    case 0x11DA: /* HANGUL JONGSEONG MIEUM-KIYEOK */
    case 0x11DB: /* HANGUL JONGSEONG MIEUM-RIEUL */
    case 0x11DC: /* HANGUL JONGSEONG MIEUM-PIEUP */
    case 0x11DD: /* HANGUL JONGSEONG MIEUM-SIOS */
    case 0x11DE: /* HANGUL JONGSEONG MIEUM-SSANGSIOS */
    case 0x11DF: /* HANGUL JONGSEONG MIEUM-PANSIOS */
    case 0x11E0: /* HANGUL JONGSEONG MIEUM-CHIEUCH */
    case 0x11E1: /* HANGUL JONGSEONG MIEUM-HIEUH */
    case 0x11E2: /* HANGUL JONGSEONG KAPYEOUNMIEUM */
    case 0x11E3: /* HANGUL JONGSEONG PIEUP-RIEUL */
    case 0x11E4: /* HANGUL JONGSEONG PIEUP-PHIEUPH */
    case 0x11E5: /* HANGUL JONGSEONG PIEUP-HIEUH */
    case 0x11E6: /* HANGUL JONGSEONG KAPYEOUNPIEUP */
    case 0x11E7: /* HANGUL JONGSEONG SIOS-KIYEOK */
    case 0x11E8: /* HANGUL JONGSEONG SIOS-TIKEUT */
    case 0x11E9: /* HANGUL JONGSEONG SIOS-RIEUL */
    case 0x11EA: /* HANGUL JONGSEONG SIOS-PIEUP */
    case 0x11EB: /* HANGUL JONGSEONG PANSIOS */
    case 0x11EC: /* HANGUL JONGSEONG IEUNG-KIYEOK */
    case 0x11ED: /* HANGUL JONGSEONG IEUNG-SSANGKIYEOK */
    case 0x11EE: /* HANGUL JONGSEONG SSANGIEUNG */
    case 0x11EF: /* HANGUL JONGSEONG IEUNG-KHIEUKH */
    case 0x11F0: /* HANGUL JONGSEONG YESIEUNG */
    case 0x11F1: /* HANGUL JONGSEONG YESIEUNG-SIOS */
    case 0x11F2: /* HANGUL JONGSEONG YESIEUNG-PANSIOS */
    case 0x11F3: /* HANGUL JONGSEONG PHIEUPH-PIEUP */
    case 0x11F4: /* HANGUL JONGSEONG KAPYEOUNPHIEUPH */
    case 0x11F5: /* HANGUL JONGSEONG HIEUH-NIEUN */
    case 0x11F6: /* HANGUL JONGSEONG HIEUH-RIEUL */
    case 0x11F7: /* HANGUL JONGSEONG HIEUH-MIEUM */
    case 0x11F8: /* HANGUL JONGSEONG HIEUH-PIEUP */
    case 0x11F9: /* HANGUL JONGSEONG YEORINHIEUH */
    case 0x1200: /* ETHIOPIC SYLLABLE HA */
    case 0x1201: /* ETHIOPIC SYLLABLE HU */
    case 0x1202: /* ETHIOPIC SYLLABLE HI */
    case 0x1203: /* ETHIOPIC SYLLABLE HAA */
    case 0x1204: /* ETHIOPIC SYLLABLE HEE */
    case 0x1205: /* ETHIOPIC SYLLABLE HE */
    case 0x1206: /* ETHIOPIC SYLLABLE HO */
    case 0x1208: /* ETHIOPIC SYLLABLE LA */
    case 0x1209: /* ETHIOPIC SYLLABLE LU */
    case 0x120A: /* ETHIOPIC SYLLABLE LI */
    case 0x120B: /* ETHIOPIC SYLLABLE LAA */
    case 0x120C: /* ETHIOPIC SYLLABLE LEE */
    case 0x120D: /* ETHIOPIC SYLLABLE LE */
    case 0x120E: /* ETHIOPIC SYLLABLE LO */
    case 0x120F: /* ETHIOPIC SYLLABLE LWA */
    case 0x1210: /* ETHIOPIC SYLLABLE HHA */
    case 0x1211: /* ETHIOPIC SYLLABLE HHU */
    case 0x1212: /* ETHIOPIC SYLLABLE HHI */
    case 0x1213: /* ETHIOPIC SYLLABLE HHAA */
    case 0x1214: /* ETHIOPIC SYLLABLE HHEE */
    case 0x1215: /* ETHIOPIC SYLLABLE HHE */
    case 0x1216: /* ETHIOPIC SYLLABLE HHO */
    case 0x1217: /* ETHIOPIC SYLLABLE HHWA */
    case 0x1218: /* ETHIOPIC SYLLABLE MA */
    case 0x1219: /* ETHIOPIC SYLLABLE MU */
    case 0x121A: /* ETHIOPIC SYLLABLE MI */
    case 0x121B: /* ETHIOPIC SYLLABLE MAA */
    case 0x121C: /* ETHIOPIC SYLLABLE MEE */
    case 0x121D: /* ETHIOPIC SYLLABLE ME */
    case 0x121E: /* ETHIOPIC SYLLABLE MO */
    case 0x121F: /* ETHIOPIC SYLLABLE MWA */
    case 0x1220: /* ETHIOPIC SYLLABLE SZA */
    case 0x1221: /* ETHIOPIC SYLLABLE SZU */
    case 0x1222: /* ETHIOPIC SYLLABLE SZI */
    case 0x1223: /* ETHIOPIC SYLLABLE SZAA */
    case 0x1224: /* ETHIOPIC SYLLABLE SZEE */
    case 0x1225: /* ETHIOPIC SYLLABLE SZE */
    case 0x1226: /* ETHIOPIC SYLLABLE SZO */
    case 0x1227: /* ETHIOPIC SYLLABLE SZWA */
    case 0x1228: /* ETHIOPIC SYLLABLE RA */
    case 0x1229: /* ETHIOPIC SYLLABLE RU */
    case 0x122A: /* ETHIOPIC SYLLABLE RI */
    case 0x122B: /* ETHIOPIC SYLLABLE RAA */
    case 0x122C: /* ETHIOPIC SYLLABLE REE */
    case 0x122D: /* ETHIOPIC SYLLABLE RE */
    case 0x122E: /* ETHIOPIC SYLLABLE RO */
    case 0x122F: /* ETHIOPIC SYLLABLE RWA */
    case 0x1230: /* ETHIOPIC SYLLABLE SA */
    case 0x1231: /* ETHIOPIC SYLLABLE SU */
    case 0x1232: /* ETHIOPIC SYLLABLE SI */
    case 0x1233: /* ETHIOPIC SYLLABLE SAA */
    case 0x1234: /* ETHIOPIC SYLLABLE SEE */
    case 0x1235: /* ETHIOPIC SYLLABLE SE */
    case 0x1236: /* ETHIOPIC SYLLABLE SO */
    case 0x1237: /* ETHIOPIC SYLLABLE SWA */
    case 0x1238: /* ETHIOPIC SYLLABLE SHA */
    case 0x1239: /* ETHIOPIC SYLLABLE SHU */
    case 0x123A: /* ETHIOPIC SYLLABLE SHI */
    case 0x123B: /* ETHIOPIC SYLLABLE SHAA */
    case 0x123C: /* ETHIOPIC SYLLABLE SHEE */
    case 0x123D: /* ETHIOPIC SYLLABLE SHE */
    case 0x123E: /* ETHIOPIC SYLLABLE SHO */
    case 0x123F: /* ETHIOPIC SYLLABLE SHWA */
    case 0x1240: /* ETHIOPIC SYLLABLE QA */
    case 0x1241: /* ETHIOPIC SYLLABLE QU */
    case 0x1242: /* ETHIOPIC SYLLABLE QI */
    case 0x1243: /* ETHIOPIC SYLLABLE QAA */
    case 0x1244: /* ETHIOPIC SYLLABLE QEE */
    case 0x1245: /* ETHIOPIC SYLLABLE QE */
    case 0x1246: /* ETHIOPIC SYLLABLE QO */
    case 0x1248: /* ETHIOPIC SYLLABLE QWA */
    case 0x124A: /* ETHIOPIC SYLLABLE QWI */
    case 0x124B: /* ETHIOPIC SYLLABLE QWAA */
    case 0x124C: /* ETHIOPIC SYLLABLE QWEE */
    case 0x124D: /* ETHIOPIC SYLLABLE QWE */
    case 0x1250: /* ETHIOPIC SYLLABLE QHA */
    case 0x1251: /* ETHIOPIC SYLLABLE QHU */
    case 0x1252: /* ETHIOPIC SYLLABLE QHI */
    case 0x1253: /* ETHIOPIC SYLLABLE QHAA */
    case 0x1254: /* ETHIOPIC SYLLABLE QHEE */
    case 0x1255: /* ETHIOPIC SYLLABLE QHE */
    case 0x1256: /* ETHIOPIC SYLLABLE QHO */
    case 0x1258: /* ETHIOPIC SYLLABLE QHWA */
    case 0x125A: /* ETHIOPIC SYLLABLE QHWI */
    case 0x125B: /* ETHIOPIC SYLLABLE QHWAA */
    case 0x125C: /* ETHIOPIC SYLLABLE QHWEE */
    case 0x125D: /* ETHIOPIC SYLLABLE QHWE */
    case 0x1260: /* ETHIOPIC SYLLABLE BA */
    case 0x1261: /* ETHIOPIC SYLLABLE BU */
    case 0x1262: /* ETHIOPIC SYLLABLE BI */
    case 0x1263: /* ETHIOPIC SYLLABLE BAA */
    case 0x1264: /* ETHIOPIC SYLLABLE BEE */
    case 0x1265: /* ETHIOPIC SYLLABLE BE */
    case 0x1266: /* ETHIOPIC SYLLABLE BO */
    case 0x1267: /* ETHIOPIC SYLLABLE BWA */
    case 0x1268: /* ETHIOPIC SYLLABLE VA */
    case 0x1269: /* ETHIOPIC SYLLABLE VU */
    case 0x126A: /* ETHIOPIC SYLLABLE VI */
    case 0x126B: /* ETHIOPIC SYLLABLE VAA */
    case 0x126C: /* ETHIOPIC SYLLABLE VEE */
    case 0x126D: /* ETHIOPIC SYLLABLE VE */
    case 0x126E: /* ETHIOPIC SYLLABLE VO */
    case 0x126F: /* ETHIOPIC SYLLABLE VWA */
    case 0x1270: /* ETHIOPIC SYLLABLE TA */
    case 0x1271: /* ETHIOPIC SYLLABLE TU */
    case 0x1272: /* ETHIOPIC SYLLABLE TI */
    case 0x1273: /* ETHIOPIC SYLLABLE TAA */
    case 0x1274: /* ETHIOPIC SYLLABLE TEE */
    case 0x1275: /* ETHIOPIC SYLLABLE TE */
    case 0x1276: /* ETHIOPIC SYLLABLE TO */
    case 0x1277: /* ETHIOPIC SYLLABLE TWA */
    case 0x1278: /* ETHIOPIC SYLLABLE CA */
    case 0x1279: /* ETHIOPIC SYLLABLE CU */
    case 0x127A: /* ETHIOPIC SYLLABLE CI */
    case 0x127B: /* ETHIOPIC SYLLABLE CAA */
    case 0x127C: /* ETHIOPIC SYLLABLE CEE */
    case 0x127D: /* ETHIOPIC SYLLABLE CE */
    case 0x127E: /* ETHIOPIC SYLLABLE CO */
    case 0x127F: /* ETHIOPIC SYLLABLE CWA */
    case 0x1280: /* ETHIOPIC SYLLABLE XA */
    case 0x1281: /* ETHIOPIC SYLLABLE XU */
    case 0x1282: /* ETHIOPIC SYLLABLE XI */
    case 0x1283: /* ETHIOPIC SYLLABLE XAA */
    case 0x1284: /* ETHIOPIC SYLLABLE XEE */
    case 0x1285: /* ETHIOPIC SYLLABLE XE */
    case 0x1286: /* ETHIOPIC SYLLABLE XO */
    case 0x1288: /* ETHIOPIC SYLLABLE XWA */
    case 0x128A: /* ETHIOPIC SYLLABLE XWI */
    case 0x128B: /* ETHIOPIC SYLLABLE XWAA */
    case 0x128C: /* ETHIOPIC SYLLABLE XWEE */
    case 0x128D: /* ETHIOPIC SYLLABLE XWE */
    case 0x1290: /* ETHIOPIC SYLLABLE NA */
    case 0x1291: /* ETHIOPIC SYLLABLE NU */
    case 0x1292: /* ETHIOPIC SYLLABLE NI */
    case 0x1293: /* ETHIOPIC SYLLABLE NAA */
    case 0x1294: /* ETHIOPIC SYLLABLE NEE */
    case 0x1295: /* ETHIOPIC SYLLABLE NE */
    case 0x1296: /* ETHIOPIC SYLLABLE NO */
    case 0x1297: /* ETHIOPIC SYLLABLE NWA */
    case 0x1298: /* ETHIOPIC SYLLABLE NYA */
    case 0x1299: /* ETHIOPIC SYLLABLE NYU */
    case 0x129A: /* ETHIOPIC SYLLABLE NYI */
    case 0x129B: /* ETHIOPIC SYLLABLE NYAA */
    case 0x129C: /* ETHIOPIC SYLLABLE NYEE */
    case 0x129D: /* ETHIOPIC SYLLABLE NYE */
    case 0x129E: /* ETHIOPIC SYLLABLE NYO */
    case 0x129F: /* ETHIOPIC SYLLABLE NYWA */
    case 0x12A0: /* ETHIOPIC SYLLABLE GLOTTAL A */
    case 0x12A1: /* ETHIOPIC SYLLABLE GLOTTAL U */
    case 0x12A2: /* ETHIOPIC SYLLABLE GLOTTAL I */
    case 0x12A3: /* ETHIOPIC SYLLABLE GLOTTAL AA */
    case 0x12A4: /* ETHIOPIC SYLLABLE GLOTTAL EE */
    case 0x12A5: /* ETHIOPIC SYLLABLE GLOTTAL E */
    case 0x12A6: /* ETHIOPIC SYLLABLE GLOTTAL O */
    case 0x12A7: /* ETHIOPIC SYLLABLE GLOTTAL WA */
    case 0x12A8: /* ETHIOPIC SYLLABLE KA */
    case 0x12A9: /* ETHIOPIC SYLLABLE KU */
    case 0x12AA: /* ETHIOPIC SYLLABLE KI */
    case 0x12AB: /* ETHIOPIC SYLLABLE KAA */
    case 0x12AC: /* ETHIOPIC SYLLABLE KEE */
    case 0x12AD: /* ETHIOPIC SYLLABLE KE */
    case 0x12AE: /* ETHIOPIC SYLLABLE KO */
    case 0x12B0: /* ETHIOPIC SYLLABLE KWA */
    case 0x12B2: /* ETHIOPIC SYLLABLE KWI */
    case 0x12B3: /* ETHIOPIC SYLLABLE KWAA */
    case 0x12B4: /* ETHIOPIC SYLLABLE KWEE */
    case 0x12B5: /* ETHIOPIC SYLLABLE KWE */
    case 0x12B8: /* ETHIOPIC SYLLABLE KXA */
    case 0x12B9: /* ETHIOPIC SYLLABLE KXU */
    case 0x12BA: /* ETHIOPIC SYLLABLE KXI */
    case 0x12BB: /* ETHIOPIC SYLLABLE KXAA */
    case 0x12BC: /* ETHIOPIC SYLLABLE KXEE */
    case 0x12BD: /* ETHIOPIC SYLLABLE KXE */
    case 0x12BE: /* ETHIOPIC SYLLABLE KXO */
    case 0x12C0: /* ETHIOPIC SYLLABLE KXWA */
    case 0x12C2: /* ETHIOPIC SYLLABLE KXWI */
    case 0x12C3: /* ETHIOPIC SYLLABLE KXWAA */
    case 0x12C4: /* ETHIOPIC SYLLABLE KXWEE */
    case 0x12C5: /* ETHIOPIC SYLLABLE KXWE */
    case 0x12C8: /* ETHIOPIC SYLLABLE WA */
    case 0x12C9: /* ETHIOPIC SYLLABLE WU */
    case 0x12CA: /* ETHIOPIC SYLLABLE WI */
    case 0x12CB: /* ETHIOPIC SYLLABLE WAA */
    case 0x12CC: /* ETHIOPIC SYLLABLE WEE */
    case 0x12CD: /* ETHIOPIC SYLLABLE WE */
    case 0x12CE: /* ETHIOPIC SYLLABLE WO */
    case 0x12D0: /* ETHIOPIC SYLLABLE PHARYNGEAL A */
    case 0x12D1: /* ETHIOPIC SYLLABLE PHARYNGEAL U */
    case 0x12D2: /* ETHIOPIC SYLLABLE PHARYNGEAL I */
    case 0x12D3: /* ETHIOPIC SYLLABLE PHARYNGEAL AA */
    case 0x12D4: /* ETHIOPIC SYLLABLE PHARYNGEAL EE */
    case 0x12D5: /* ETHIOPIC SYLLABLE PHARYNGEAL E */
    case 0x12D6: /* ETHIOPIC SYLLABLE PHARYNGEAL O */
    case 0x12D8: /* ETHIOPIC SYLLABLE ZA */
    case 0x12D9: /* ETHIOPIC SYLLABLE ZU */
    case 0x12DA: /* ETHIOPIC SYLLABLE ZI */
    case 0x12DB: /* ETHIOPIC SYLLABLE ZAA */
    case 0x12DC: /* ETHIOPIC SYLLABLE ZEE */
    case 0x12DD: /* ETHIOPIC SYLLABLE ZE */
    case 0x12DE: /* ETHIOPIC SYLLABLE ZO */
    case 0x12DF: /* ETHIOPIC SYLLABLE ZWA */
    case 0x12E0: /* ETHIOPIC SYLLABLE ZHA */
    case 0x12E1: /* ETHIOPIC SYLLABLE ZHU */
    case 0x12E2: /* ETHIOPIC SYLLABLE ZHI */
    case 0x12E3: /* ETHIOPIC SYLLABLE ZHAA */
    case 0x12E4: /* ETHIOPIC SYLLABLE ZHEE */
    case 0x12E5: /* ETHIOPIC SYLLABLE ZHE */
    case 0x12E6: /* ETHIOPIC SYLLABLE ZHO */
    case 0x12E7: /* ETHIOPIC SYLLABLE ZHWA */
    case 0x12E8: /* ETHIOPIC SYLLABLE YA */
    case 0x12E9: /* ETHIOPIC SYLLABLE YU */
    case 0x12EA: /* ETHIOPIC SYLLABLE YI */
    case 0x12EB: /* ETHIOPIC SYLLABLE YAA */
    case 0x12EC: /* ETHIOPIC SYLLABLE YEE */
    case 0x12ED: /* ETHIOPIC SYLLABLE YE */
    case 0x12EE: /* ETHIOPIC SYLLABLE YO */
    case 0x12F0: /* ETHIOPIC SYLLABLE DA */
    case 0x12F1: /* ETHIOPIC SYLLABLE DU */
    case 0x12F2: /* ETHIOPIC SYLLABLE DI */
    case 0x12F3: /* ETHIOPIC SYLLABLE DAA */
    case 0x12F4: /* ETHIOPIC SYLLABLE DEE */
    case 0x12F5: /* ETHIOPIC SYLLABLE DE */
    case 0x12F6: /* ETHIOPIC SYLLABLE DO */
    case 0x12F7: /* ETHIOPIC SYLLABLE DWA */
    case 0x12F8: /* ETHIOPIC SYLLABLE DDA */
    case 0x12F9: /* ETHIOPIC SYLLABLE DDU */
    case 0x12FA: /* ETHIOPIC SYLLABLE DDI */
    case 0x12FB: /* ETHIOPIC SYLLABLE DDAA */
    case 0x12FC: /* ETHIOPIC SYLLABLE DDEE */
    case 0x12FD: /* ETHIOPIC SYLLABLE DDE */
    case 0x12FE: /* ETHIOPIC SYLLABLE DDO */
    case 0x12FF: /* ETHIOPIC SYLLABLE DDWA */
    case 0x1300: /* ETHIOPIC SYLLABLE JA */
    case 0x1301: /* ETHIOPIC SYLLABLE JU */
    case 0x1302: /* ETHIOPIC SYLLABLE JI */
    case 0x1303: /* ETHIOPIC SYLLABLE JAA */
    case 0x1304: /* ETHIOPIC SYLLABLE JEE */
    case 0x1305: /* ETHIOPIC SYLLABLE JE */
    case 0x1306: /* ETHIOPIC SYLLABLE JO */
    case 0x1307: /* ETHIOPIC SYLLABLE JWA */
    case 0x1308: /* ETHIOPIC SYLLABLE GA */
    case 0x1309: /* ETHIOPIC SYLLABLE GU */
    case 0x130A: /* ETHIOPIC SYLLABLE GI */
    case 0x130B: /* ETHIOPIC SYLLABLE GAA */
    case 0x130C: /* ETHIOPIC SYLLABLE GEE */
    case 0x130D: /* ETHIOPIC SYLLABLE GE */
    case 0x130E: /* ETHIOPIC SYLLABLE GO */
    case 0x1310: /* ETHIOPIC SYLLABLE GWA */
    case 0x1312: /* ETHIOPIC SYLLABLE GWI */
    case 0x1313: /* ETHIOPIC SYLLABLE GWAA */
    case 0x1314: /* ETHIOPIC SYLLABLE GWEE */
    case 0x1315: /* ETHIOPIC SYLLABLE GWE */
    case 0x1318: /* ETHIOPIC SYLLABLE GGA */
    case 0x1319: /* ETHIOPIC SYLLABLE GGU */
    case 0x131A: /* ETHIOPIC SYLLABLE GGI */
    case 0x131B: /* ETHIOPIC SYLLABLE GGAA */
    case 0x131C: /* ETHIOPIC SYLLABLE GGEE */
    case 0x131D: /* ETHIOPIC SYLLABLE GGE */
    case 0x131E: /* ETHIOPIC SYLLABLE GGO */
    case 0x1320: /* ETHIOPIC SYLLABLE THA */
    case 0x1321: /* ETHIOPIC SYLLABLE THU */
    case 0x1322: /* ETHIOPIC SYLLABLE THI */
    case 0x1323: /* ETHIOPIC SYLLABLE THAA */
    case 0x1324: /* ETHIOPIC SYLLABLE THEE */
    case 0x1325: /* ETHIOPIC SYLLABLE THE */
    case 0x1326: /* ETHIOPIC SYLLABLE THO */
    case 0x1327: /* ETHIOPIC SYLLABLE THWA */
    case 0x1328: /* ETHIOPIC SYLLABLE CHA */
    case 0x1329: /* ETHIOPIC SYLLABLE CHU */
    case 0x132A: /* ETHIOPIC SYLLABLE CHI */
    case 0x132B: /* ETHIOPIC SYLLABLE CHAA */
    case 0x132C: /* ETHIOPIC SYLLABLE CHEE */
    case 0x132D: /* ETHIOPIC SYLLABLE CHE */
    case 0x132E: /* ETHIOPIC SYLLABLE CHO */
    case 0x132F: /* ETHIOPIC SYLLABLE CHWA */
    case 0x1330: /* ETHIOPIC SYLLABLE PHA */
    case 0x1331: /* ETHIOPIC SYLLABLE PHU */
    case 0x1332: /* ETHIOPIC SYLLABLE PHI */
    case 0x1333: /* ETHIOPIC SYLLABLE PHAA */
    case 0x1334: /* ETHIOPIC SYLLABLE PHEE */
    case 0x1335: /* ETHIOPIC SYLLABLE PHE */
    case 0x1336: /* ETHIOPIC SYLLABLE PHO */
    case 0x1337: /* ETHIOPIC SYLLABLE PHWA */
    case 0x1338: /* ETHIOPIC SYLLABLE TSA */
    case 0x1339: /* ETHIOPIC SYLLABLE TSU */
    case 0x133A: /* ETHIOPIC SYLLABLE TSI */
    case 0x133B: /* ETHIOPIC SYLLABLE TSAA */
    case 0x133C: /* ETHIOPIC SYLLABLE TSEE */
    case 0x133D: /* ETHIOPIC SYLLABLE TSE */
    case 0x133E: /* ETHIOPIC SYLLABLE TSO */
    case 0x133F: /* ETHIOPIC SYLLABLE TSWA */
    case 0x1340: /* ETHIOPIC SYLLABLE TZA */
    case 0x1341: /* ETHIOPIC SYLLABLE TZU */
    case 0x1342: /* ETHIOPIC SYLLABLE TZI */
    case 0x1343: /* ETHIOPIC SYLLABLE TZAA */
    case 0x1344: /* ETHIOPIC SYLLABLE TZEE */
    case 0x1345: /* ETHIOPIC SYLLABLE TZE */
    case 0x1346: /* ETHIOPIC SYLLABLE TZO */
    case 0x1348: /* ETHIOPIC SYLLABLE FA */
    case 0x1349: /* ETHIOPIC SYLLABLE FU */
    case 0x134A: /* ETHIOPIC SYLLABLE FI */
    case 0x134B: /* ETHIOPIC SYLLABLE FAA */
    case 0x134C: /* ETHIOPIC SYLLABLE FEE */
    case 0x134D: /* ETHIOPIC SYLLABLE FE */
    case 0x134E: /* ETHIOPIC SYLLABLE FO */
    case 0x134F: /* ETHIOPIC SYLLABLE FWA */
    case 0x1350: /* ETHIOPIC SYLLABLE PA */
    case 0x1351: /* ETHIOPIC SYLLABLE PU */
    case 0x1352: /* ETHIOPIC SYLLABLE PI */
    case 0x1353: /* ETHIOPIC SYLLABLE PAA */
    case 0x1354: /* ETHIOPIC SYLLABLE PEE */
    case 0x1355: /* ETHIOPIC SYLLABLE PE */
    case 0x1356: /* ETHIOPIC SYLLABLE PO */
    case 0x1357: /* ETHIOPIC SYLLABLE PWA */
    case 0x1358: /* ETHIOPIC SYLLABLE RYA */
    case 0x1359: /* ETHIOPIC SYLLABLE MYA */
    case 0x135A: /* ETHIOPIC SYLLABLE FYA */
    case 0x13A0: /* CHEROKEE LETTER A */
    case 0x13A1: /* CHEROKEE LETTER E */
    case 0x13A2: /* CHEROKEE LETTER I */
    case 0x13A3: /* CHEROKEE LETTER O */
    case 0x13A4: /* CHEROKEE LETTER U */
    case 0x13A5: /* CHEROKEE LETTER V */
    case 0x13A6: /* CHEROKEE LETTER GA */
    case 0x13A7: /* CHEROKEE LETTER KA */
    case 0x13A8: /* CHEROKEE LETTER GE */
    case 0x13A9: /* CHEROKEE LETTER GI */
    case 0x13AA: /* CHEROKEE LETTER GO */
    case 0x13AB: /* CHEROKEE LETTER GU */
    case 0x13AC: /* CHEROKEE LETTER GV */
    case 0x13AD: /* CHEROKEE LETTER HA */
    case 0x13AE: /* CHEROKEE LETTER HE */
    case 0x13AF: /* CHEROKEE LETTER HI */
    case 0x13B0: /* CHEROKEE LETTER HO */
    case 0x13B1: /* CHEROKEE LETTER HU */
    case 0x13B2: /* CHEROKEE LETTER HV */
    case 0x13B3: /* CHEROKEE LETTER LA */
    case 0x13B4: /* CHEROKEE LETTER LE */
    case 0x13B5: /* CHEROKEE LETTER LI */
    case 0x13B6: /* CHEROKEE LETTER LO */
    case 0x13B7: /* CHEROKEE LETTER LU */
    case 0x13B8: /* CHEROKEE LETTER LV */
    case 0x13B9: /* CHEROKEE LETTER MA */
    case 0x13BA: /* CHEROKEE LETTER ME */
    case 0x13BB: /* CHEROKEE LETTER MI */
    case 0x13BC: /* CHEROKEE LETTER MO */
    case 0x13BD: /* CHEROKEE LETTER MU */
    case 0x13BE: /* CHEROKEE LETTER NA */
    case 0x13BF: /* CHEROKEE LETTER HNA */
    case 0x13C0: /* CHEROKEE LETTER NAH */
    case 0x13C1: /* CHEROKEE LETTER NE */
    case 0x13C2: /* CHEROKEE LETTER NI */
    case 0x13C3: /* CHEROKEE LETTER NO */
    case 0x13C4: /* CHEROKEE LETTER NU */
    case 0x13C5: /* CHEROKEE LETTER NV */
    case 0x13C6: /* CHEROKEE LETTER QUA */
    case 0x13C7: /* CHEROKEE LETTER QUE */
    case 0x13C8: /* CHEROKEE LETTER QUI */
    case 0x13C9: /* CHEROKEE LETTER QUO */
    case 0x13CA: /* CHEROKEE LETTER QUU */
    case 0x13CB: /* CHEROKEE LETTER QUV */
    case 0x13CC: /* CHEROKEE LETTER SA */
    case 0x13CD: /* CHEROKEE LETTER S */
    case 0x13CE: /* CHEROKEE LETTER SE */
    case 0x13CF: /* CHEROKEE LETTER SI */
    case 0x13D0: /* CHEROKEE LETTER SO */
    case 0x13D1: /* CHEROKEE LETTER SU */
    case 0x13D2: /* CHEROKEE LETTER SV */
    case 0x13D3: /* CHEROKEE LETTER DA */
    case 0x13D4: /* CHEROKEE LETTER TA */
    case 0x13D5: /* CHEROKEE LETTER DE */
    case 0x13D6: /* CHEROKEE LETTER TE */
    case 0x13D7: /* CHEROKEE LETTER DI */
    case 0x13D8: /* CHEROKEE LETTER TI */
    case 0x13D9: /* CHEROKEE LETTER DO */
    case 0x13DA: /* CHEROKEE LETTER DU */
    case 0x13DB: /* CHEROKEE LETTER DV */
    case 0x13DC: /* CHEROKEE LETTER DLA */
    case 0x13DD: /* CHEROKEE LETTER TLA */
    case 0x13DE: /* CHEROKEE LETTER TLE */
    case 0x13DF: /* CHEROKEE LETTER TLI */
    case 0x13E0: /* CHEROKEE LETTER TLO */
    case 0x13E1: /* CHEROKEE LETTER TLU */
    case 0x13E2: /* CHEROKEE LETTER TLV */
    case 0x13E3: /* CHEROKEE LETTER TSA */
    case 0x13E4: /* CHEROKEE LETTER TSE */
    case 0x13E5: /* CHEROKEE LETTER TSI */
    case 0x13E6: /* CHEROKEE LETTER TSO */
    case 0x13E7: /* CHEROKEE LETTER TSU */
    case 0x13E8: /* CHEROKEE LETTER TSV */
    case 0x13E9: /* CHEROKEE LETTER WA */
    case 0x13EA: /* CHEROKEE LETTER WE */
    case 0x13EB: /* CHEROKEE LETTER WI */
    case 0x13EC: /* CHEROKEE LETTER WO */
    case 0x13ED: /* CHEROKEE LETTER WU */
    case 0x13EE: /* CHEROKEE LETTER WV */
    case 0x13EF: /* CHEROKEE LETTER YA */
    case 0x13F0: /* CHEROKEE LETTER YE */
    case 0x13F1: /* CHEROKEE LETTER YI */
    case 0x13F2: /* CHEROKEE LETTER YO */
    case 0x13F3: /* CHEROKEE LETTER YU */
    case 0x13F4: /* CHEROKEE LETTER YV */
    case 0x1401: /* CANADIAN SYLLABICS E */
    case 0x1402: /* CANADIAN SYLLABICS AAI */
    case 0x1403: /* CANADIAN SYLLABICS I */
    case 0x1404: /* CANADIAN SYLLABICS II */
    case 0x1405: /* CANADIAN SYLLABICS O */
    case 0x1406: /* CANADIAN SYLLABICS OO */
    case 0x1407: /* CANADIAN SYLLABICS Y-CREE OO */
    case 0x1408: /* CANADIAN SYLLABICS CARRIER EE */
    case 0x1409: /* CANADIAN SYLLABICS CARRIER I */
    case 0x140A: /* CANADIAN SYLLABICS A */
    case 0x140B: /* CANADIAN SYLLABICS AA */
    case 0x140C: /* CANADIAN SYLLABICS WE */
    case 0x140D: /* CANADIAN SYLLABICS WEST-CREE WE */
    case 0x140E: /* CANADIAN SYLLABICS WI */
    case 0x140F: /* CANADIAN SYLLABICS WEST-CREE WI */
    case 0x1410: /* CANADIAN SYLLABICS WII */
    case 0x1411: /* CANADIAN SYLLABICS WEST-CREE WII */
    case 0x1412: /* CANADIAN SYLLABICS WO */
    case 0x1413: /* CANADIAN SYLLABICS WEST-CREE WO */
    case 0x1414: /* CANADIAN SYLLABICS WOO */
    case 0x1415: /* CANADIAN SYLLABICS WEST-CREE WOO */
    case 0x1416: /* CANADIAN SYLLABICS NASKAPI WOO */
    case 0x1417: /* CANADIAN SYLLABICS WA */
    case 0x1418: /* CANADIAN SYLLABICS WEST-CREE WA */
    case 0x1419: /* CANADIAN SYLLABICS WAA */
    case 0x141A: /* CANADIAN SYLLABICS WEST-CREE WAA */
    case 0x141B: /* CANADIAN SYLLABICS NASKAPI WAA */
    case 0x141C: /* CANADIAN SYLLABICS AI */
    case 0x141D: /* CANADIAN SYLLABICS Y-CREE W */
    case 0x141E: /* CANADIAN SYLLABICS GLOTTAL STOP */
    case 0x141F: /* CANADIAN SYLLABICS FINAL ACUTE */
    case 0x1420: /* CANADIAN SYLLABICS FINAL GRAVE */
    case 0x1421: /* CANADIAN SYLLABICS FINAL BOTTOM HALF RING */
    case 0x1422: /* CANADIAN SYLLABICS FINAL TOP HALF RING */
    case 0x1423: /* CANADIAN SYLLABICS FINAL RIGHT HALF RING */
    case 0x1424: /* CANADIAN SYLLABICS FINAL RING */
    case 0x1425: /* CANADIAN SYLLABICS FINAL DOUBLE ACUTE */
    case 0x1426: /* CANADIAN SYLLABICS FINAL DOUBLE SHORT VERTICAL STROKES */
    case 0x1427: /* CANADIAN SYLLABICS FINAL MIDDLE DOT */
    case 0x1428: /* CANADIAN SYLLABICS FINAL SHORT HORIZONTAL STROKE */
    case 0x1429: /* CANADIAN SYLLABICS FINAL PLUS */
    case 0x142A: /* CANADIAN SYLLABICS FINAL DOWN TACK */
    case 0x142B: /* CANADIAN SYLLABICS EN */
    case 0x142C: /* CANADIAN SYLLABICS IN */
    case 0x142D: /* CANADIAN SYLLABICS ON */
    case 0x142E: /* CANADIAN SYLLABICS AN */
    case 0x142F: /* CANADIAN SYLLABICS PE */
    case 0x1430: /* CANADIAN SYLLABICS PAAI */
    case 0x1431: /* CANADIAN SYLLABICS PI */
    case 0x1432: /* CANADIAN SYLLABICS PII */
    case 0x1433: /* CANADIAN SYLLABICS PO */
    case 0x1434: /* CANADIAN SYLLABICS POO */
    case 0x1435: /* CANADIAN SYLLABICS Y-CREE POO */
    case 0x1436: /* CANADIAN SYLLABICS CARRIER HEE */
    case 0x1437: /* CANADIAN SYLLABICS CARRIER HI */
    case 0x1438: /* CANADIAN SYLLABICS PA */
    case 0x1439: /* CANADIAN SYLLABICS PAA */
    case 0x143A: /* CANADIAN SYLLABICS PWE */
    case 0x143B: /* CANADIAN SYLLABICS WEST-CREE PWE */
    case 0x143C: /* CANADIAN SYLLABICS PWI */
    case 0x143D: /* CANADIAN SYLLABICS WEST-CREE PWI */
    case 0x143E: /* CANADIAN SYLLABICS PWII */
    case 0x143F: /* CANADIAN SYLLABICS WEST-CREE PWII */
    case 0x1440: /* CANADIAN SYLLABICS PWO */
    case 0x1441: /* CANADIAN SYLLABICS WEST-CREE PWO */
    case 0x1442: /* CANADIAN SYLLABICS PWOO */
    case 0x1443: /* CANADIAN SYLLABICS WEST-CREE PWOO */
    case 0x1444: /* CANADIAN SYLLABICS PWA */
    case 0x1445: /* CANADIAN SYLLABICS WEST-CREE PWA */
    case 0x1446: /* CANADIAN SYLLABICS PWAA */
    case 0x1447: /* CANADIAN SYLLABICS WEST-CREE PWAA */
    case 0x1448: /* CANADIAN SYLLABICS Y-CREE PWAA */
    case 0x1449: /* CANADIAN SYLLABICS P */
    case 0x144A: /* CANADIAN SYLLABICS WEST-CREE P */
    case 0x144B: /* CANADIAN SYLLABICS CARRIER H */
    case 0x144C: /* CANADIAN SYLLABICS TE */
    case 0x144D: /* CANADIAN SYLLABICS TAAI */
    case 0x144E: /* CANADIAN SYLLABICS TI */
    case 0x144F: /* CANADIAN SYLLABICS TII */
    case 0x1450: /* CANADIAN SYLLABICS TO */
    case 0x1451: /* CANADIAN SYLLABICS TOO */
    case 0x1452: /* CANADIAN SYLLABICS Y-CREE TOO */
    case 0x1453: /* CANADIAN SYLLABICS CARRIER DEE */
    case 0x1454: /* CANADIAN SYLLABICS CARRIER DI */
    case 0x1455: /* CANADIAN SYLLABICS TA */
    case 0x1456: /* CANADIAN SYLLABICS TAA */
    case 0x1457: /* CANADIAN SYLLABICS TWE */
    case 0x1458: /* CANADIAN SYLLABICS WEST-CREE TWE */
    case 0x1459: /* CANADIAN SYLLABICS TWI */
    case 0x145A: /* CANADIAN SYLLABICS WEST-CREE TWI */
    case 0x145B: /* CANADIAN SYLLABICS TWII */
    case 0x145C: /* CANADIAN SYLLABICS WEST-CREE TWII */
    case 0x145D: /* CANADIAN SYLLABICS TWO */
    case 0x145E: /* CANADIAN SYLLABICS WEST-CREE TWO */
    case 0x145F: /* CANADIAN SYLLABICS TWOO */
    case 0x1460: /* CANADIAN SYLLABICS WEST-CREE TWOO */
    case 0x1461: /* CANADIAN SYLLABICS TWA */
    case 0x1462: /* CANADIAN SYLLABICS WEST-CREE TWA */
    case 0x1463: /* CANADIAN SYLLABICS TWAA */
    case 0x1464: /* CANADIAN SYLLABICS WEST-CREE TWAA */
    case 0x1465: /* CANADIAN SYLLABICS NASKAPI TWAA */
    case 0x1466: /* CANADIAN SYLLABICS T */
    case 0x1467: /* CANADIAN SYLLABICS TTE */
    case 0x1468: /* CANADIAN SYLLABICS TTI */
    case 0x1469: /* CANADIAN SYLLABICS TTO */
    case 0x146A: /* CANADIAN SYLLABICS TTA */
    case 0x146B: /* CANADIAN SYLLABICS KE */
    case 0x146C: /* CANADIAN SYLLABICS KAAI */
    case 0x146D: /* CANADIAN SYLLABICS KI */
    case 0x146E: /* CANADIAN SYLLABICS KII */
    case 0x146F: /* CANADIAN SYLLABICS KO */
    case 0x1470: /* CANADIAN SYLLABICS KOO */
    case 0x1471: /* CANADIAN SYLLABICS Y-CREE KOO */
    case 0x1472: /* CANADIAN SYLLABICS KA */
    case 0x1473: /* CANADIAN SYLLABICS KAA */
    case 0x1474: /* CANADIAN SYLLABICS KWE */
    case 0x1475: /* CANADIAN SYLLABICS WEST-CREE KWE */
    case 0x1476: /* CANADIAN SYLLABICS KWI */
    case 0x1477: /* CANADIAN SYLLABICS WEST-CREE KWI */
    case 0x1478: /* CANADIAN SYLLABICS KWII */
    case 0x1479: /* CANADIAN SYLLABICS WEST-CREE KWII */
    case 0x147A: /* CANADIAN SYLLABICS KWO */
    case 0x147B: /* CANADIAN SYLLABICS WEST-CREE KWO */
    case 0x147C: /* CANADIAN SYLLABICS KWOO */
    case 0x147D: /* CANADIAN SYLLABICS WEST-CREE KWOO */
    case 0x147E: /* CANADIAN SYLLABICS KWA */
    case 0x147F: /* CANADIAN SYLLABICS WEST-CREE KWA */
    case 0x1480: /* CANADIAN SYLLABICS KWAA */
    case 0x1481: /* CANADIAN SYLLABICS WEST-CREE KWAA */
    case 0x1482: /* CANADIAN SYLLABICS NASKAPI KWAA */
    case 0x1483: /* CANADIAN SYLLABICS K */
    case 0x1484: /* CANADIAN SYLLABICS KW */
    case 0x1485: /* CANADIAN SYLLABICS SOUTH-SLAVEY KEH */
    case 0x1486: /* CANADIAN SYLLABICS SOUTH-SLAVEY KIH */
    case 0x1487: /* CANADIAN SYLLABICS SOUTH-SLAVEY KOH */
    case 0x1488: /* CANADIAN SYLLABICS SOUTH-SLAVEY KAH */
    case 0x1489: /* CANADIAN SYLLABICS CE */
    case 0x148A: /* CANADIAN SYLLABICS CAAI */
    case 0x148B: /* CANADIAN SYLLABICS CI */
    case 0x148C: /* CANADIAN SYLLABICS CII */
    case 0x148D: /* CANADIAN SYLLABICS CO */
    case 0x148E: /* CANADIAN SYLLABICS COO */
    case 0x148F: /* CANADIAN SYLLABICS Y-CREE COO */
    case 0x1490: /* CANADIAN SYLLABICS CA */
    case 0x1491: /* CANADIAN SYLLABICS CAA */
    case 0x1492: /* CANADIAN SYLLABICS CWE */
    case 0x1493: /* CANADIAN SYLLABICS WEST-CREE CWE */
    case 0x1494: /* CANADIAN SYLLABICS CWI */
    case 0x1495: /* CANADIAN SYLLABICS WEST-CREE CWI */
    case 0x1496: /* CANADIAN SYLLABICS CWII */
    case 0x1497: /* CANADIAN SYLLABICS WEST-CREE CWII */
    case 0x1498: /* CANADIAN SYLLABICS CWO */
    case 0x1499: /* CANADIAN SYLLABICS WEST-CREE CWO */
    case 0x149A: /* CANADIAN SYLLABICS CWOO */
    case 0x149B: /* CANADIAN SYLLABICS WEST-CREE CWOO */
    case 0x149C: /* CANADIAN SYLLABICS CWA */
    case 0x149D: /* CANADIAN SYLLABICS WEST-CREE CWA */
    case 0x149E: /* CANADIAN SYLLABICS CWAA */
    case 0x149F: /* CANADIAN SYLLABICS WEST-CREE CWAA */
    case 0x14A0: /* CANADIAN SYLLABICS NASKAPI CWAA */
    case 0x14A1: /* CANADIAN SYLLABICS C */
    case 0x14A2: /* CANADIAN SYLLABICS SAYISI TH */
    case 0x14A3: /* CANADIAN SYLLABICS ME */
    case 0x14A4: /* CANADIAN SYLLABICS MAAI */
    case 0x14A5: /* CANADIAN SYLLABICS MI */
    case 0x14A6: /* CANADIAN SYLLABICS MII */
    case 0x14A7: /* CANADIAN SYLLABICS MO */
    case 0x14A8: /* CANADIAN SYLLABICS MOO */
    case 0x14A9: /* CANADIAN SYLLABICS Y-CREE MOO */
    case 0x14AA: /* CANADIAN SYLLABICS MA */
    case 0x14AB: /* CANADIAN SYLLABICS MAA */
    case 0x14AC: /* CANADIAN SYLLABICS MWE */
    case 0x14AD: /* CANADIAN SYLLABICS WEST-CREE MWE */
    case 0x14AE: /* CANADIAN SYLLABICS MWI */
    case 0x14AF: /* CANADIAN SYLLABICS WEST-CREE MWI */
    case 0x14B0: /* CANADIAN SYLLABICS MWII */
    case 0x14B1: /* CANADIAN SYLLABICS WEST-CREE MWII */
    case 0x14B2: /* CANADIAN SYLLABICS MWO */
    case 0x14B3: /* CANADIAN SYLLABICS WEST-CREE MWO */
    case 0x14B4: /* CANADIAN SYLLABICS MWOO */
    case 0x14B5: /* CANADIAN SYLLABICS WEST-CREE MWOO */
    case 0x14B6: /* CANADIAN SYLLABICS MWA */
    case 0x14B7: /* CANADIAN SYLLABICS WEST-CREE MWA */
    case 0x14B8: /* CANADIAN SYLLABICS MWAA */
    case 0x14B9: /* CANADIAN SYLLABICS WEST-CREE MWAA */
    case 0x14BA: /* CANADIAN SYLLABICS NASKAPI MWAA */
    case 0x14BB: /* CANADIAN SYLLABICS M */
    case 0x14BC: /* CANADIAN SYLLABICS WEST-CREE M */
    case 0x14BD: /* CANADIAN SYLLABICS MH */
    case 0x14BE: /* CANADIAN SYLLABICS ATHAPASCAN M */
    case 0x14BF: /* CANADIAN SYLLABICS SAYISI M */
    case 0x14C0: /* CANADIAN SYLLABICS NE */
    case 0x14C1: /* CANADIAN SYLLABICS NAAI */
    case 0x14C2: /* CANADIAN SYLLABICS NI */
    case 0x14C3: /* CANADIAN SYLLABICS NII */
    case 0x14C4: /* CANADIAN SYLLABICS NO */
    case 0x14C5: /* CANADIAN SYLLABICS NOO */
    case 0x14C6: /* CANADIAN SYLLABICS Y-CREE NOO */
    case 0x14C7: /* CANADIAN SYLLABICS NA */
    case 0x14C8: /* CANADIAN SYLLABICS NAA */
    case 0x14C9: /* CANADIAN SYLLABICS NWE */
    case 0x14CA: /* CANADIAN SYLLABICS WEST-CREE NWE */
    case 0x14CB: /* CANADIAN SYLLABICS NWA */
    case 0x14CC: /* CANADIAN SYLLABICS WEST-CREE NWA */
    case 0x14CD: /* CANADIAN SYLLABICS NWAA */
    case 0x14CE: /* CANADIAN SYLLABICS WEST-CREE NWAA */
    case 0x14CF: /* CANADIAN SYLLABICS NASKAPI NWAA */
    case 0x14D0: /* CANADIAN SYLLABICS N */
    case 0x14D1: /* CANADIAN SYLLABICS CARRIER NG */
    case 0x14D2: /* CANADIAN SYLLABICS NH */
    case 0x14D3: /* CANADIAN SYLLABICS LE */
    case 0x14D4: /* CANADIAN SYLLABICS LAAI */
    case 0x14D5: /* CANADIAN SYLLABICS LI */
    case 0x14D6: /* CANADIAN SYLLABICS LII */
    case 0x14D7: /* CANADIAN SYLLABICS LO */
    case 0x14D8: /* CANADIAN SYLLABICS LOO */
    case 0x14D9: /* CANADIAN SYLLABICS Y-CREE LOO */
    case 0x14DA: /* CANADIAN SYLLABICS LA */
    case 0x14DB: /* CANADIAN SYLLABICS LAA */
    case 0x14DC: /* CANADIAN SYLLABICS LWE */
    case 0x14DD: /* CANADIAN SYLLABICS WEST-CREE LWE */
    case 0x14DE: /* CANADIAN SYLLABICS LWI */
    case 0x14DF: /* CANADIAN SYLLABICS WEST-CREE LWI */
    case 0x14E0: /* CANADIAN SYLLABICS LWII */
    case 0x14E1: /* CANADIAN SYLLABICS WEST-CREE LWII */
    case 0x14E2: /* CANADIAN SYLLABICS LWO */
    case 0x14E3: /* CANADIAN SYLLABICS WEST-CREE LWO */
    case 0x14E4: /* CANADIAN SYLLABICS LWOO */
    case 0x14E5: /* CANADIAN SYLLABICS WEST-CREE LWOO */
    case 0x14E6: /* CANADIAN SYLLABICS LWA */
    case 0x14E7: /* CANADIAN SYLLABICS WEST-CREE LWA */
    case 0x14E8: /* CANADIAN SYLLABICS LWAA */
    case 0x14E9: /* CANADIAN SYLLABICS WEST-CREE LWAA */
    case 0x14EA: /* CANADIAN SYLLABICS L */
    case 0x14EB: /* CANADIAN SYLLABICS WEST-CREE L */
    case 0x14EC: /* CANADIAN SYLLABICS MEDIAL L */
    case 0x14ED: /* CANADIAN SYLLABICS SE */
    case 0x14EE: /* CANADIAN SYLLABICS SAAI */
    case 0x14EF: /* CANADIAN SYLLABICS SI */
    case 0x14F0: /* CANADIAN SYLLABICS SII */
    case 0x14F1: /* CANADIAN SYLLABICS SO */
    case 0x14F2: /* CANADIAN SYLLABICS SOO */
    case 0x14F3: /* CANADIAN SYLLABICS Y-CREE SOO */
    case 0x14F4: /* CANADIAN SYLLABICS SA */
    case 0x14F5: /* CANADIAN SYLLABICS SAA */
    case 0x14F6: /* CANADIAN SYLLABICS SWE */
    case 0x14F7: /* CANADIAN SYLLABICS WEST-CREE SWE */
    case 0x14F8: /* CANADIAN SYLLABICS SWI */
    case 0x14F9: /* CANADIAN SYLLABICS WEST-CREE SWI */
    case 0x14FA: /* CANADIAN SYLLABICS SWII */
    case 0x14FB: /* CANADIAN SYLLABICS WEST-CREE SWII */
    case 0x14FC: /* CANADIAN SYLLABICS SWO */
    case 0x14FD: /* CANADIAN SYLLABICS WEST-CREE SWO */
    case 0x14FE: /* CANADIAN SYLLABICS SWOO */
    case 0x14FF: /* CANADIAN SYLLABICS WEST-CREE SWOO */
    case 0x1500: /* CANADIAN SYLLABICS SWA */
    case 0x1501: /* CANADIAN SYLLABICS WEST-CREE SWA */
    case 0x1502: /* CANADIAN SYLLABICS SWAA */
    case 0x1503: /* CANADIAN SYLLABICS WEST-CREE SWAA */
    case 0x1504: /* CANADIAN SYLLABICS NASKAPI SWAA */
    case 0x1505: /* CANADIAN SYLLABICS S */
    case 0x1506: /* CANADIAN SYLLABICS ATHAPASCAN S */
    case 0x1507: /* CANADIAN SYLLABICS SW */
    case 0x1508: /* CANADIAN SYLLABICS BLACKFOOT S */
    case 0x1509: /* CANADIAN SYLLABICS MOOSE-CREE SK */
    case 0x150A: /* CANADIAN SYLLABICS NASKAPI SKW */
    case 0x150B: /* CANADIAN SYLLABICS NASKAPI S-W */
    case 0x150C: /* CANADIAN SYLLABICS NASKAPI SPWA */
    case 0x150D: /* CANADIAN SYLLABICS NASKAPI STWA */
    case 0x150E: /* CANADIAN SYLLABICS NASKAPI SKWA */
    case 0x150F: /* CANADIAN SYLLABICS NASKAPI SCWA */
    case 0x1510: /* CANADIAN SYLLABICS SHE */
    case 0x1511: /* CANADIAN SYLLABICS SHI */
    case 0x1512: /* CANADIAN SYLLABICS SHII */
    case 0x1513: /* CANADIAN SYLLABICS SHO */
    case 0x1514: /* CANADIAN SYLLABICS SHOO */
    case 0x1515: /* CANADIAN SYLLABICS SHA */
    case 0x1516: /* CANADIAN SYLLABICS SHAA */
    case 0x1517: /* CANADIAN SYLLABICS SHWE */
    case 0x1518: /* CANADIAN SYLLABICS WEST-CREE SHWE */
    case 0x1519: /* CANADIAN SYLLABICS SHWI */
    case 0x151A: /* CANADIAN SYLLABICS WEST-CREE SHWI */
    case 0x151B: /* CANADIAN SYLLABICS SHWII */
    case 0x151C: /* CANADIAN SYLLABICS WEST-CREE SHWII */
    case 0x151D: /* CANADIAN SYLLABICS SHWO */
    case 0x151E: /* CANADIAN SYLLABICS WEST-CREE SHWO */
    case 0x151F: /* CANADIAN SYLLABICS SHWOO */
    case 0x1520: /* CANADIAN SYLLABICS WEST-CREE SHWOO */
    case 0x1521: /* CANADIAN SYLLABICS SHWA */
    case 0x1522: /* CANADIAN SYLLABICS WEST-CREE SHWA */
    case 0x1523: /* CANADIAN SYLLABICS SHWAA */
    case 0x1524: /* CANADIAN SYLLABICS WEST-CREE SHWAA */
    case 0x1525: /* CANADIAN SYLLABICS SH */
    case 0x1526: /* CANADIAN SYLLABICS YE */
    case 0x1527: /* CANADIAN SYLLABICS YAAI */
    case 0x1528: /* CANADIAN SYLLABICS YI */
    case 0x1529: /* CANADIAN SYLLABICS YII */
    case 0x152A: /* CANADIAN SYLLABICS YO */
    case 0x152B: /* CANADIAN SYLLABICS YOO */
    case 0x152C: /* CANADIAN SYLLABICS Y-CREE YOO */
    case 0x152D: /* CANADIAN SYLLABICS YA */
    case 0x152E: /* CANADIAN SYLLABICS YAA */
    case 0x152F: /* CANADIAN SYLLABICS YWE */
    case 0x1530: /* CANADIAN SYLLABICS WEST-CREE YWE */
    case 0x1531: /* CANADIAN SYLLABICS YWI */
    case 0x1532: /* CANADIAN SYLLABICS WEST-CREE YWI */
    case 0x1533: /* CANADIAN SYLLABICS YWII */
BREAK_SWITCH_UP
    case 0x1534: /* CANADIAN SYLLABICS WEST-CREE YWII */
    case 0x1535: /* CANADIAN SYLLABICS YWO */
    case 0x1536: /* CANADIAN SYLLABICS WEST-CREE YWO */
    case 0x1537: /* CANADIAN SYLLABICS YWOO */
    case 0x1538: /* CANADIAN SYLLABICS WEST-CREE YWOO */
    case 0x1539: /* CANADIAN SYLLABICS YWA */
    case 0x153A: /* CANADIAN SYLLABICS WEST-CREE YWA */
    case 0x153B: /* CANADIAN SYLLABICS YWAA */
    case 0x153C: /* CANADIAN SYLLABICS WEST-CREE YWAA */
    case 0x153D: /* CANADIAN SYLLABICS NASKAPI YWAA */
    case 0x153E: /* CANADIAN SYLLABICS Y */
    case 0x153F: /* CANADIAN SYLLABICS BIBLE-CREE Y */
    case 0x1540: /* CANADIAN SYLLABICS WEST-CREE Y */
    case 0x1541: /* CANADIAN SYLLABICS SAYISI YI */
    case 0x1542: /* CANADIAN SYLLABICS RE */
    case 0x1543: /* CANADIAN SYLLABICS R-CREE RE */
    case 0x1544: /* CANADIAN SYLLABICS WEST-CREE LE */
    case 0x1545: /* CANADIAN SYLLABICS RAAI */
    case 0x1546: /* CANADIAN SYLLABICS RI */
    case 0x1547: /* CANADIAN SYLLABICS RII */
    case 0x1548: /* CANADIAN SYLLABICS RO */
    case 0x1549: /* CANADIAN SYLLABICS ROO */
    case 0x154A: /* CANADIAN SYLLABICS WEST-CREE LO */
    case 0x154B: /* CANADIAN SYLLABICS RA */
    case 0x154C: /* CANADIAN SYLLABICS RAA */
    case 0x154D: /* CANADIAN SYLLABICS WEST-CREE LA */
    case 0x154E: /* CANADIAN SYLLABICS RWAA */
    case 0x154F: /* CANADIAN SYLLABICS WEST-CREE RWAA */
    case 0x1550: /* CANADIAN SYLLABICS R */
    case 0x1551: /* CANADIAN SYLLABICS WEST-CREE R */
    case 0x1552: /* CANADIAN SYLLABICS MEDIAL R */
    case 0x1553: /* CANADIAN SYLLABICS FE */
    case 0x1554: /* CANADIAN SYLLABICS FAAI */
    case 0x1555: /* CANADIAN SYLLABICS FI */
    case 0x1556: /* CANADIAN SYLLABICS FII */
    case 0x1557: /* CANADIAN SYLLABICS FO */
    case 0x1558: /* CANADIAN SYLLABICS FOO */
    case 0x1559: /* CANADIAN SYLLABICS FA */
    case 0x155A: /* CANADIAN SYLLABICS FAA */
    case 0x155B: /* CANADIAN SYLLABICS FWAA */
    case 0x155C: /* CANADIAN SYLLABICS WEST-CREE FWAA */
    case 0x155D: /* CANADIAN SYLLABICS F */
    case 0x155E: /* CANADIAN SYLLABICS THE */
    case 0x155F: /* CANADIAN SYLLABICS N-CREE THE */
    case 0x1560: /* CANADIAN SYLLABICS THI */
    case 0x1561: /* CANADIAN SYLLABICS N-CREE THI */
    case 0x1562: /* CANADIAN SYLLABICS THII */
    case 0x1563: /* CANADIAN SYLLABICS N-CREE THII */
    case 0x1564: /* CANADIAN SYLLABICS THO */
    case 0x1565: /* CANADIAN SYLLABICS THOO */
    case 0x1566: /* CANADIAN SYLLABICS THA */
    case 0x1567: /* CANADIAN SYLLABICS THAA */
    case 0x1568: /* CANADIAN SYLLABICS THWAA */
    case 0x1569: /* CANADIAN SYLLABICS WEST-CREE THWAA */
    case 0x156A: /* CANADIAN SYLLABICS TH */
    case 0x156B: /* CANADIAN SYLLABICS TTHE */
    case 0x156C: /* CANADIAN SYLLABICS TTHI */
    case 0x156D: /* CANADIAN SYLLABICS TTHO */
    case 0x156E: /* CANADIAN SYLLABICS TTHA */
    case 0x156F: /* CANADIAN SYLLABICS TTH */
    case 0x1570: /* CANADIAN SYLLABICS TYE */
    case 0x1571: /* CANADIAN SYLLABICS TYI */
    case 0x1572: /* CANADIAN SYLLABICS TYO */
    case 0x1573: /* CANADIAN SYLLABICS TYA */
    case 0x1574: /* CANADIAN SYLLABICS NUNAVIK HE */
    case 0x1575: /* CANADIAN SYLLABICS NUNAVIK HI */
    case 0x1576: /* CANADIAN SYLLABICS NUNAVIK HII */
    case 0x1577: /* CANADIAN SYLLABICS NUNAVIK HO */
    case 0x1578: /* CANADIAN SYLLABICS NUNAVIK HOO */
    case 0x1579: /* CANADIAN SYLLABICS NUNAVIK HA */
    case 0x157A: /* CANADIAN SYLLABICS NUNAVIK HAA */
    case 0x157B: /* CANADIAN SYLLABICS NUNAVIK H */
    case 0x157C: /* CANADIAN SYLLABICS NUNAVUT H */
    case 0x157D: /* CANADIAN SYLLABICS HK */
    case 0x157E: /* CANADIAN SYLLABICS QAAI */
    case 0x157F: /* CANADIAN SYLLABICS QI */
    case 0x1580: /* CANADIAN SYLLABICS QII */
    case 0x1581: /* CANADIAN SYLLABICS QO */
    case 0x1582: /* CANADIAN SYLLABICS QOO */
    case 0x1583: /* CANADIAN SYLLABICS QA */
    case 0x1584: /* CANADIAN SYLLABICS QAA */
    case 0x1585: /* CANADIAN SYLLABICS Q */
    case 0x1586: /* CANADIAN SYLLABICS TLHE */
    case 0x1587: /* CANADIAN SYLLABICS TLHI */
    case 0x1588: /* CANADIAN SYLLABICS TLHO */
    case 0x1589: /* CANADIAN SYLLABICS TLHA */
    case 0x158A: /* CANADIAN SYLLABICS WEST-CREE RE */
    case 0x158B: /* CANADIAN SYLLABICS WEST-CREE RI */
    case 0x158C: /* CANADIAN SYLLABICS WEST-CREE RO */
    case 0x158D: /* CANADIAN SYLLABICS WEST-CREE RA */
    case 0x158E: /* CANADIAN SYLLABICS NGAAI */
    case 0x158F: /* CANADIAN SYLLABICS NGI */
    case 0x1590: /* CANADIAN SYLLABICS NGII */
    case 0x1591: /* CANADIAN SYLLABICS NGO */
    case 0x1592: /* CANADIAN SYLLABICS NGOO */
    case 0x1593: /* CANADIAN SYLLABICS NGA */
    case 0x1594: /* CANADIAN SYLLABICS NGAA */
    case 0x1595: /* CANADIAN SYLLABICS NG */
    case 0x1596: /* CANADIAN SYLLABICS NNG */
    case 0x1597: /* CANADIAN SYLLABICS SAYISI SHE */
    case 0x1598: /* CANADIAN SYLLABICS SAYISI SHI */
    case 0x1599: /* CANADIAN SYLLABICS SAYISI SHO */
    case 0x159A: /* CANADIAN SYLLABICS SAYISI SHA */
    case 0x159B: /* CANADIAN SYLLABICS WOODS-CREE THE */
    case 0x159C: /* CANADIAN SYLLABICS WOODS-CREE THI */
    case 0x159D: /* CANADIAN SYLLABICS WOODS-CREE THO */
    case 0x159E: /* CANADIAN SYLLABICS WOODS-CREE THA */
    case 0x159F: /* CANADIAN SYLLABICS WOODS-CREE TH */
    case 0x15A0: /* CANADIAN SYLLABICS LHI */
    case 0x15A1: /* CANADIAN SYLLABICS LHII */
    case 0x15A2: /* CANADIAN SYLLABICS LHO */
    case 0x15A3: /* CANADIAN SYLLABICS LHOO */
    case 0x15A4: /* CANADIAN SYLLABICS LHA */
    case 0x15A5: /* CANADIAN SYLLABICS LHAA */
    case 0x15A6: /* CANADIAN SYLLABICS LH */
    case 0x15A7: /* CANADIAN SYLLABICS TH-CREE THE */
    case 0x15A8: /* CANADIAN SYLLABICS TH-CREE THI */
    case 0x15A9: /* CANADIAN SYLLABICS TH-CREE THII */
    case 0x15AA: /* CANADIAN SYLLABICS TH-CREE THO */
    case 0x15AB: /* CANADIAN SYLLABICS TH-CREE THOO */
    case 0x15AC: /* CANADIAN SYLLABICS TH-CREE THA */
    case 0x15AD: /* CANADIAN SYLLABICS TH-CREE THAA */
    case 0x15AE: /* CANADIAN SYLLABICS TH-CREE TH */
    case 0x15AF: /* CANADIAN SYLLABICS AIVILIK B */
    case 0x15B0: /* CANADIAN SYLLABICS BLACKFOOT E */
    case 0x15B1: /* CANADIAN SYLLABICS BLACKFOOT I */
    case 0x15B2: /* CANADIAN SYLLABICS BLACKFOOT O */
    case 0x15B3: /* CANADIAN SYLLABICS BLACKFOOT A */
    case 0x15B4: /* CANADIAN SYLLABICS BLACKFOOT WE */
    case 0x15B5: /* CANADIAN SYLLABICS BLACKFOOT WI */
    case 0x15B6: /* CANADIAN SYLLABICS BLACKFOOT WO */
    case 0x15B7: /* CANADIAN SYLLABICS BLACKFOOT WA */
    case 0x15B8: /* CANADIAN SYLLABICS BLACKFOOT NE */
    case 0x15B9: /* CANADIAN SYLLABICS BLACKFOOT NI */
    case 0x15BA: /* CANADIAN SYLLABICS BLACKFOOT NO */
    case 0x15BB: /* CANADIAN SYLLABICS BLACKFOOT NA */
    case 0x15BC: /* CANADIAN SYLLABICS BLACKFOOT KE */
    case 0x15BD: /* CANADIAN SYLLABICS BLACKFOOT KI */
    case 0x15BE: /* CANADIAN SYLLABICS BLACKFOOT KO */
    case 0x15BF: /* CANADIAN SYLLABICS BLACKFOOT KA */
    case 0x15C0: /* CANADIAN SYLLABICS SAYISI HE */
    case 0x15C1: /* CANADIAN SYLLABICS SAYISI HI */
    case 0x15C2: /* CANADIAN SYLLABICS SAYISI HO */
    case 0x15C3: /* CANADIAN SYLLABICS SAYISI HA */
    case 0x15C4: /* CANADIAN SYLLABICS CARRIER GHU */
    case 0x15C5: /* CANADIAN SYLLABICS CARRIER GHO */
    case 0x15C6: /* CANADIAN SYLLABICS CARRIER GHE */
    case 0x15C7: /* CANADIAN SYLLABICS CARRIER GHEE */
    case 0x15C8: /* CANADIAN SYLLABICS CARRIER GHI */
    case 0x15C9: /* CANADIAN SYLLABICS CARRIER GHA */
    case 0x15CA: /* CANADIAN SYLLABICS CARRIER RU */
    case 0x15CB: /* CANADIAN SYLLABICS CARRIER RO */
    case 0x15CC: /* CANADIAN SYLLABICS CARRIER RE */
    case 0x15CD: /* CANADIAN SYLLABICS CARRIER REE */
    case 0x15CE: /* CANADIAN SYLLABICS CARRIER RI */
    case 0x15CF: /* CANADIAN SYLLABICS CARRIER RA */
    case 0x15D0: /* CANADIAN SYLLABICS CARRIER WU */
    case 0x15D1: /* CANADIAN SYLLABICS CARRIER WO */
    case 0x15D2: /* CANADIAN SYLLABICS CARRIER WE */
    case 0x15D3: /* CANADIAN SYLLABICS CARRIER WEE */
    case 0x15D4: /* CANADIAN SYLLABICS CARRIER WI */
    case 0x15D5: /* CANADIAN SYLLABICS CARRIER WA */
    case 0x15D6: /* CANADIAN SYLLABICS CARRIER HWU */
    case 0x15D7: /* CANADIAN SYLLABICS CARRIER HWO */
    case 0x15D8: /* CANADIAN SYLLABICS CARRIER HWE */
    case 0x15D9: /* CANADIAN SYLLABICS CARRIER HWEE */
    case 0x15DA: /* CANADIAN SYLLABICS CARRIER HWI */
    case 0x15DB: /* CANADIAN SYLLABICS CARRIER HWA */
    case 0x15DC: /* CANADIAN SYLLABICS CARRIER THU */
    case 0x15DD: /* CANADIAN SYLLABICS CARRIER THO */
    case 0x15DE: /* CANADIAN SYLLABICS CARRIER THE */
    case 0x15DF: /* CANADIAN SYLLABICS CARRIER THEE */
    case 0x15E0: /* CANADIAN SYLLABICS CARRIER THI */
    case 0x15E1: /* CANADIAN SYLLABICS CARRIER THA */
    case 0x15E2: /* CANADIAN SYLLABICS CARRIER TTU */
    case 0x15E3: /* CANADIAN SYLLABICS CARRIER TTO */
    case 0x15E4: /* CANADIAN SYLLABICS CARRIER TTE */
    case 0x15E5: /* CANADIAN SYLLABICS CARRIER TTEE */
    case 0x15E6: /* CANADIAN SYLLABICS CARRIER TTI */
    case 0x15E7: /* CANADIAN SYLLABICS CARRIER TTA */
    case 0x15E8: /* CANADIAN SYLLABICS CARRIER PU */
    case 0x15E9: /* CANADIAN SYLLABICS CARRIER PO */
    case 0x15EA: /* CANADIAN SYLLABICS CARRIER PE */
    case 0x15EB: /* CANADIAN SYLLABICS CARRIER PEE */
    case 0x15EC: /* CANADIAN SYLLABICS CARRIER PI */
    case 0x15ED: /* CANADIAN SYLLABICS CARRIER PA */
    case 0x15EE: /* CANADIAN SYLLABICS CARRIER P */
    case 0x15EF: /* CANADIAN SYLLABICS CARRIER GU */
    case 0x15F0: /* CANADIAN SYLLABICS CARRIER GO */
    case 0x15F1: /* CANADIAN SYLLABICS CARRIER GE */
    case 0x15F2: /* CANADIAN SYLLABICS CARRIER GEE */
    case 0x15F3: /* CANADIAN SYLLABICS CARRIER GI */
    case 0x15F4: /* CANADIAN SYLLABICS CARRIER GA */
    case 0x15F5: /* CANADIAN SYLLABICS CARRIER KHU */
    case 0x15F6: /* CANADIAN SYLLABICS CARRIER KHO */
    case 0x15F7: /* CANADIAN SYLLABICS CARRIER KHE */
    case 0x15F8: /* CANADIAN SYLLABICS CARRIER KHEE */
    case 0x15F9: /* CANADIAN SYLLABICS CARRIER KHI */
    case 0x15FA: /* CANADIAN SYLLABICS CARRIER KHA */
    case 0x15FB: /* CANADIAN SYLLABICS CARRIER KKU */
    case 0x15FC: /* CANADIAN SYLLABICS CARRIER KKO */
    case 0x15FD: /* CANADIAN SYLLABICS CARRIER KKE */
    case 0x15FE: /* CANADIAN SYLLABICS CARRIER KKEE */
    case 0x15FF: /* CANADIAN SYLLABICS CARRIER KKI */
    case 0x1600: /* CANADIAN SYLLABICS CARRIER KKA */
    case 0x1601: /* CANADIAN SYLLABICS CARRIER KK */
    case 0x1602: /* CANADIAN SYLLABICS CARRIER NU */
    case 0x1603: /* CANADIAN SYLLABICS CARRIER NO */
    case 0x1604: /* CANADIAN SYLLABICS CARRIER NE */
    case 0x1605: /* CANADIAN SYLLABICS CARRIER NEE */
    case 0x1606: /* CANADIAN SYLLABICS CARRIER NI */
    case 0x1607: /* CANADIAN SYLLABICS CARRIER NA */
    case 0x1608: /* CANADIAN SYLLABICS CARRIER MU */
    case 0x1609: /* CANADIAN SYLLABICS CARRIER MO */
    case 0x160A: /* CANADIAN SYLLABICS CARRIER ME */
    case 0x160B: /* CANADIAN SYLLABICS CARRIER MEE */
    case 0x160C: /* CANADIAN SYLLABICS CARRIER MI */
    case 0x160D: /* CANADIAN SYLLABICS CARRIER MA */
    case 0x160E: /* CANADIAN SYLLABICS CARRIER YU */
    case 0x160F: /* CANADIAN SYLLABICS CARRIER YO */
    case 0x1610: /* CANADIAN SYLLABICS CARRIER YE */
    case 0x1611: /* CANADIAN SYLLABICS CARRIER YEE */
    case 0x1612: /* CANADIAN SYLLABICS CARRIER YI */
    case 0x1613: /* CANADIAN SYLLABICS CARRIER YA */
    case 0x1614: /* CANADIAN SYLLABICS CARRIER JU */
    case 0x1615: /* CANADIAN SYLLABICS SAYISI JU */
    case 0x1616: /* CANADIAN SYLLABICS CARRIER JO */
    case 0x1617: /* CANADIAN SYLLABICS CARRIER JE */
    case 0x1618: /* CANADIAN SYLLABICS CARRIER JEE */
    case 0x1619: /* CANADIAN SYLLABICS CARRIER JI */
    case 0x161A: /* CANADIAN SYLLABICS SAYISI JI */
    case 0x161B: /* CANADIAN SYLLABICS CARRIER JA */
    case 0x161C: /* CANADIAN SYLLABICS CARRIER JJU */
    case 0x161D: /* CANADIAN SYLLABICS CARRIER JJO */
    case 0x161E: /* CANADIAN SYLLABICS CARRIER JJE */
    case 0x161F: /* CANADIAN SYLLABICS CARRIER JJEE */
    case 0x1620: /* CANADIAN SYLLABICS CARRIER JJI */
    case 0x1621: /* CANADIAN SYLLABICS CARRIER JJA */
    case 0x1622: /* CANADIAN SYLLABICS CARRIER LU */
    case 0x1623: /* CANADIAN SYLLABICS CARRIER LO */
    case 0x1624: /* CANADIAN SYLLABICS CARRIER LE */
    case 0x1625: /* CANADIAN SYLLABICS CARRIER LEE */
    case 0x1626: /* CANADIAN SYLLABICS CARRIER LI */
    case 0x1627: /* CANADIAN SYLLABICS CARRIER LA */
    case 0x1628: /* CANADIAN SYLLABICS CARRIER DLU */
    case 0x1629: /* CANADIAN SYLLABICS CARRIER DLO */
    case 0x162A: /* CANADIAN SYLLABICS CARRIER DLE */
    case 0x162B: /* CANADIAN SYLLABICS CARRIER DLEE */
    case 0x162C: /* CANADIAN SYLLABICS CARRIER DLI */
    case 0x162D: /* CANADIAN SYLLABICS CARRIER DLA */
    case 0x162E: /* CANADIAN SYLLABICS CARRIER LHU */
    case 0x162F: /* CANADIAN SYLLABICS CARRIER LHO */
    case 0x1630: /* CANADIAN SYLLABICS CARRIER LHE */
    case 0x1631: /* CANADIAN SYLLABICS CARRIER LHEE */
    case 0x1632: /* CANADIAN SYLLABICS CARRIER LHI */
    case 0x1633: /* CANADIAN SYLLABICS CARRIER LHA */
    case 0x1634: /* CANADIAN SYLLABICS CARRIER TLHU */
    case 0x1635: /* CANADIAN SYLLABICS CARRIER TLHO */
    case 0x1636: /* CANADIAN SYLLABICS CARRIER TLHE */
    case 0x1637: /* CANADIAN SYLLABICS CARRIER TLHEE */
    case 0x1638: /* CANADIAN SYLLABICS CARRIER TLHI */
    case 0x1639: /* CANADIAN SYLLABICS CARRIER TLHA */
    case 0x163A: /* CANADIAN SYLLABICS CARRIER TLU */
    case 0x163B: /* CANADIAN SYLLABICS CARRIER TLO */
    case 0x163C: /* CANADIAN SYLLABICS CARRIER TLE */
    case 0x163D: /* CANADIAN SYLLABICS CARRIER TLEE */
    case 0x163E: /* CANADIAN SYLLABICS CARRIER TLI */
    case 0x163F: /* CANADIAN SYLLABICS CARRIER TLA */
    case 0x1640: /* CANADIAN SYLLABICS CARRIER ZU */
    case 0x1641: /* CANADIAN SYLLABICS CARRIER ZO */
    case 0x1642: /* CANADIAN SYLLABICS CARRIER ZE */
    case 0x1643: /* CANADIAN SYLLABICS CARRIER ZEE */
    case 0x1644: /* CANADIAN SYLLABICS CARRIER ZI */
    case 0x1645: /* CANADIAN SYLLABICS CARRIER ZA */
    case 0x1646: /* CANADIAN SYLLABICS CARRIER Z */
    case 0x1647: /* CANADIAN SYLLABICS CARRIER INITIAL Z */
    case 0x1648: /* CANADIAN SYLLABICS CARRIER DZU */
    case 0x1649: /* CANADIAN SYLLABICS CARRIER DZO */
    case 0x164A: /* CANADIAN SYLLABICS CARRIER DZE */
    case 0x164B: /* CANADIAN SYLLABICS CARRIER DZEE */
    case 0x164C: /* CANADIAN SYLLABICS CARRIER DZI */
    case 0x164D: /* CANADIAN SYLLABICS CARRIER DZA */
    case 0x164E: /* CANADIAN SYLLABICS CARRIER SU */
    case 0x164F: /* CANADIAN SYLLABICS CARRIER SO */
    case 0x1650: /* CANADIAN SYLLABICS CARRIER SE */
    case 0x1651: /* CANADIAN SYLLABICS CARRIER SEE */
    case 0x1652: /* CANADIAN SYLLABICS CARRIER SI */
    case 0x1653: /* CANADIAN SYLLABICS CARRIER SA */
    case 0x1654: /* CANADIAN SYLLABICS CARRIER SHU */
    case 0x1655: /* CANADIAN SYLLABICS CARRIER SHO */
    case 0x1656: /* CANADIAN SYLLABICS CARRIER SHE */
    case 0x1657: /* CANADIAN SYLLABICS CARRIER SHEE */
    case 0x1658: /* CANADIAN SYLLABICS CARRIER SHI */
    case 0x1659: /* CANADIAN SYLLABICS CARRIER SHA */
    case 0x165A: /* CANADIAN SYLLABICS CARRIER SH */
    case 0x165B: /* CANADIAN SYLLABICS CARRIER TSU */
    case 0x165C: /* CANADIAN SYLLABICS CARRIER TSO */
    case 0x165D: /* CANADIAN SYLLABICS CARRIER TSE */
    case 0x165E: /* CANADIAN SYLLABICS CARRIER TSEE */
    case 0x165F: /* CANADIAN SYLLABICS CARRIER TSI */
    case 0x1660: /* CANADIAN SYLLABICS CARRIER TSA */
    case 0x1661: /* CANADIAN SYLLABICS CARRIER CHU */
    case 0x1662: /* CANADIAN SYLLABICS CARRIER CHO */
    case 0x1663: /* CANADIAN SYLLABICS CARRIER CHE */
    case 0x1664: /* CANADIAN SYLLABICS CARRIER CHEE */
    case 0x1665: /* CANADIAN SYLLABICS CARRIER CHI */
    case 0x1666: /* CANADIAN SYLLABICS CARRIER CHA */
    case 0x1667: /* CANADIAN SYLLABICS CARRIER TTSU */
    case 0x1668: /* CANADIAN SYLLABICS CARRIER TTSO */
    case 0x1669: /* CANADIAN SYLLABICS CARRIER TTSE */
    case 0x166A: /* CANADIAN SYLLABICS CARRIER TTSEE */
    case 0x166B: /* CANADIAN SYLLABICS CARRIER TTSI */
    case 0x166C: /* CANADIAN SYLLABICS CARRIER TTSA */
    case 0x166F: /* CANADIAN SYLLABICS QAI */
    case 0x1670: /* CANADIAN SYLLABICS NGAI */
    case 0x1671: /* CANADIAN SYLLABICS NNGI */
    case 0x1672: /* CANADIAN SYLLABICS NNGII */
    case 0x1673: /* CANADIAN SYLLABICS NNGO */
    case 0x1674: /* CANADIAN SYLLABICS NNGOO */
    case 0x1675: /* CANADIAN SYLLABICS NNGA */
    case 0x1676: /* CANADIAN SYLLABICS NNGAA */
    case 0x1681: /* OGHAM LETTER BEITH */
    case 0x1682: /* OGHAM LETTER LUIS */
    case 0x1683: /* OGHAM LETTER FEARN */
    case 0x1684: /* OGHAM LETTER SAIL */
    case 0x1685: /* OGHAM LETTER NION */
    case 0x1686: /* OGHAM LETTER UATH */
    case 0x1687: /* OGHAM LETTER DAIR */
    case 0x1688: /* OGHAM LETTER TINNE */
    case 0x1689: /* OGHAM LETTER COLL */
    case 0x168A: /* OGHAM LETTER CEIRT */
    case 0x168B: /* OGHAM LETTER MUIN */
    case 0x168C: /* OGHAM LETTER GORT */
    case 0x168D: /* OGHAM LETTER NGEADAL */
    case 0x168E: /* OGHAM LETTER STRAIF */
    case 0x168F: /* OGHAM LETTER RUIS */
    case 0x1690: /* OGHAM LETTER AILM */
    case 0x1691: /* OGHAM LETTER ONN */
    case 0x1692: /* OGHAM LETTER UR */
    case 0x1693: /* OGHAM LETTER EADHADH */
    case 0x1694: /* OGHAM LETTER IODHADH */
    case 0x1695: /* OGHAM LETTER EABHADH */
    case 0x1696: /* OGHAM LETTER OR */
    case 0x1697: /* OGHAM LETTER UILLEANN */
    case 0x1698: /* OGHAM LETTER IFIN */
    case 0x1699: /* OGHAM LETTER EAMHANCHOLL */
    case 0x169A: /* OGHAM LETTER PEITH */
    case 0x16A0: /* RUNIC LETTER FEHU FEOH FE F */
    case 0x16A1: /* RUNIC LETTER V */
    case 0x16A2: /* RUNIC LETTER URUZ UR U */
    case 0x16A3: /* RUNIC LETTER YR */
    case 0x16A4: /* RUNIC LETTER Y */
    case 0x16A5: /* RUNIC LETTER W */
    case 0x16A6: /* RUNIC LETTER THURISAZ THURS THORN */
    case 0x16A7: /* RUNIC LETTER ETH */
    case 0x16A8: /* RUNIC LETTER ANSUZ A */
    case 0x16A9: /* RUNIC LETTER OS O */
    case 0x16AA: /* RUNIC LETTER AC A */
    case 0x16AB: /* RUNIC LETTER AESC */
    case 0x16AC: /* RUNIC LETTER LONG-BRANCH-OSS O */
    case 0x16AD: /* RUNIC LETTER SHORT-TWIG-OSS O */
    case 0x16AE: /* RUNIC LETTER O */
    case 0x16AF: /* RUNIC LETTER OE */
    case 0x16B0: /* RUNIC LETTER ON */
    case 0x16B1: /* RUNIC LETTER RAIDO RAD REID R */
    case 0x16B2: /* RUNIC LETTER KAUNA */
    case 0x16B3: /* RUNIC LETTER CEN */
    case 0x16B4: /* RUNIC LETTER KAUN K */
    case 0x16B5: /* RUNIC LETTER G */
    case 0x16B6: /* RUNIC LETTER ENG */
    case 0x16B7: /* RUNIC LETTER GEBO GYFU G */
    case 0x16B8: /* RUNIC LETTER GAR */
    case 0x16B9: /* RUNIC LETTER WUNJO WYNN W */
    case 0x16BA: /* RUNIC LETTER HAGLAZ H */
    case 0x16BB: /* RUNIC LETTER HAEGL H */
    case 0x16BC: /* RUNIC LETTER LONG-BRANCH-HAGALL H */
    case 0x16BD: /* RUNIC LETTER SHORT-TWIG-HAGALL H */
    case 0x16BE: /* RUNIC LETTER NAUDIZ NYD NAUD N */
    case 0x16BF: /* RUNIC LETTER SHORT-TWIG-NAUD N */
    case 0x16C0: /* RUNIC LETTER DOTTED-N */
    case 0x16C1: /* RUNIC LETTER ISAZ IS ISS I */
    case 0x16C2: /* RUNIC LETTER E */
    case 0x16C3: /* RUNIC LETTER JERAN J */
    case 0x16C4: /* RUNIC LETTER GER */
    case 0x16C5: /* RUNIC LETTER LONG-BRANCH-AR AE */
    case 0x16C6: /* RUNIC LETTER SHORT-TWIG-AR A */
    case 0x16C7: /* RUNIC LETTER IWAZ EOH */
    case 0x16C8: /* RUNIC LETTER PERTHO PEORTH P */
    case 0x16C9: /* RUNIC LETTER ALGIZ EOLHX */
    case 0x16CA: /* RUNIC LETTER SOWILO S */
    case 0x16CB: /* RUNIC LETTER SIGEL LONG-BRANCH-SOL S */
    case 0x16CC: /* RUNIC LETTER SHORT-TWIG-SOL S */
    case 0x16CD: /* RUNIC LETTER C */
    case 0x16CE: /* RUNIC LETTER Z */
    case 0x16CF: /* RUNIC LETTER TIWAZ TIR TYR T */
    case 0x16D0: /* RUNIC LETTER SHORT-TWIG-TYR T */
    case 0x16D1: /* RUNIC LETTER D */
    case 0x16D2: /* RUNIC LETTER BERKANAN BEORC BJARKAN B */
    case 0x16D3: /* RUNIC LETTER SHORT-TWIG-BJARKAN B */
    case 0x16D4: /* RUNIC LETTER DOTTED-P */
    case 0x16D5: /* RUNIC LETTER OPEN-P */
    case 0x16D6: /* RUNIC LETTER EHWAZ EH E */
    case 0x16D7: /* RUNIC LETTER MANNAZ MAN M */
    case 0x16D8: /* RUNIC LETTER LONG-BRANCH-MADR M */
    case 0x16D9: /* RUNIC LETTER SHORT-TWIG-MADR M */
    case 0x16DA: /* RUNIC LETTER LAUKAZ LAGU LOGR L */
    case 0x16DB: /* RUNIC LETTER DOTTED-L */
    case 0x16DC: /* RUNIC LETTER INGWAZ */
    case 0x16DD: /* RUNIC LETTER ING */
    case 0x16DE: /* RUNIC LETTER DAGAZ DAEG D */
    case 0x16DF: /* RUNIC LETTER OTHALAN ETHEL O */
    case 0x16E0: /* RUNIC LETTER EAR */
    case 0x16E1: /* RUNIC LETTER IOR */
    case 0x16E2: /* RUNIC LETTER CWEORTH */
    case 0x16E3: /* RUNIC LETTER CALC */
    case 0x16E4: /* RUNIC LETTER CEALC */
    case 0x16E5: /* RUNIC LETTER STAN */
    case 0x16E6: /* RUNIC LETTER LONG-BRANCH-YR */
    case 0x16E7: /* RUNIC LETTER SHORT-TWIG-YR */
    case 0x16E8: /* RUNIC LETTER ICELANDIC-YR */
    case 0x16E9: /* RUNIC LETTER Q */
    case 0x16EA: /* RUNIC LETTER X */
    case 0x1780: /* KHMER LETTER KA */
    case 0x1781: /* KHMER LETTER KHA */
    case 0x1782: /* KHMER LETTER KO */
    case 0x1783: /* KHMER LETTER KHO */
    case 0x1784: /* KHMER LETTER NGO */
    case 0x1785: /* KHMER LETTER CA */
    case 0x1786: /* KHMER LETTER CHA */
    case 0x1787: /* KHMER LETTER CO */
    case 0x1788: /* KHMER LETTER CHO */
    case 0x1789: /* KHMER LETTER NYO */
    case 0x178A: /* KHMER LETTER DA */
    case 0x178B: /* KHMER LETTER TTHA */
    case 0x178C: /* KHMER LETTER DO */
    case 0x178D: /* KHMER LETTER TTHO */
    case 0x178E: /* KHMER LETTER NNO */
    case 0x178F: /* KHMER LETTER TA */
    case 0x1790: /* KHMER LETTER THA */
    case 0x1791: /* KHMER LETTER TO */
    case 0x1792: /* KHMER LETTER THO */
    case 0x1793: /* KHMER LETTER NO */
    case 0x1794: /* KHMER LETTER BA */
    case 0x1795: /* KHMER LETTER PHA */
    case 0x1796: /* KHMER LETTER PO */
    case 0x1797: /* KHMER LETTER PHO */
    case 0x1798: /* KHMER LETTER MO */
    case 0x1799: /* KHMER LETTER YO */
    case 0x179A: /* KHMER LETTER RO */
    case 0x179B: /* KHMER LETTER LO */
    case 0x179C: /* KHMER LETTER VO */
    case 0x179D: /* KHMER LETTER SHA */
    case 0x179E: /* KHMER LETTER SSO */
    case 0x179F: /* KHMER LETTER SA */
    case 0x17A0: /* KHMER LETTER HA */
    case 0x17A1: /* KHMER LETTER LA */
    case 0x17A2: /* KHMER LETTER QA */
    case 0x17A3: /* KHMER INDEPENDENT VOWEL QAQ */
    case 0x17A4: /* KHMER INDEPENDENT VOWEL QAA */
    case 0x17A5: /* KHMER INDEPENDENT VOWEL QI */
    case 0x17A6: /* KHMER INDEPENDENT VOWEL QII */
    case 0x17A7: /* KHMER INDEPENDENT VOWEL QU */
    case 0x17A8: /* KHMER INDEPENDENT VOWEL QUK */
    case 0x17A9: /* KHMER INDEPENDENT VOWEL QUU */
    case 0x17AA: /* KHMER INDEPENDENT VOWEL QUUV */
    case 0x17AB: /* KHMER INDEPENDENT VOWEL RY */
    case 0x17AC: /* KHMER INDEPENDENT VOWEL RYY */
    case 0x17AD: /* KHMER INDEPENDENT VOWEL LY */
    case 0x17AE: /* KHMER INDEPENDENT VOWEL LYY */
    case 0x17AF: /* KHMER INDEPENDENT VOWEL QE */
    case 0x17B0: /* KHMER INDEPENDENT VOWEL QAI */
    case 0x17B1: /* KHMER INDEPENDENT VOWEL QOO TYPE ONE */
    case 0x17B2: /* KHMER INDEPENDENT VOWEL QOO TYPE TWO */
    case 0x17B3: /* KHMER INDEPENDENT VOWEL QAU */
    case 0x1820: /* MONGOLIAN LETTER A */
    case 0x1821: /* MONGOLIAN LETTER E */
    case 0x1822: /* MONGOLIAN LETTER I */
    case 0x1823: /* MONGOLIAN LETTER O */
    case 0x1824: /* MONGOLIAN LETTER U */
    case 0x1825: /* MONGOLIAN LETTER OE */
    case 0x1826: /* MONGOLIAN LETTER UE */
    case 0x1827: /* MONGOLIAN LETTER EE */
    case 0x1828: /* MONGOLIAN LETTER NA */
    case 0x1829: /* MONGOLIAN LETTER ANG */
    case 0x182A: /* MONGOLIAN LETTER BA */
    case 0x182B: /* MONGOLIAN LETTER PA */
    case 0x182C: /* MONGOLIAN LETTER QA */
    case 0x182D: /* MONGOLIAN LETTER GA */
    case 0x182E: /* MONGOLIAN LETTER MA */
    case 0x182F: /* MONGOLIAN LETTER LA */
    case 0x1830: /* MONGOLIAN LETTER SA */
    case 0x1831: /* MONGOLIAN LETTER SHA */
    case 0x1832: /* MONGOLIAN LETTER TA */
    case 0x1833: /* MONGOLIAN LETTER DA */
    case 0x1834: /* MONGOLIAN LETTER CHA */
    case 0x1835: /* MONGOLIAN LETTER JA */
    case 0x1836: /* MONGOLIAN LETTER YA */
    case 0x1837: /* MONGOLIAN LETTER RA */
    case 0x1838: /* MONGOLIAN LETTER WA */
    case 0x1839: /* MONGOLIAN LETTER FA */
    case 0x183A: /* MONGOLIAN LETTER KA */
    case 0x183B: /* MONGOLIAN LETTER KHA */
    case 0x183C: /* MONGOLIAN LETTER TSA */
    case 0x183D: /* MONGOLIAN LETTER ZA */
    case 0x183E: /* MONGOLIAN LETTER HAA */
    case 0x183F: /* MONGOLIAN LETTER ZRA */
    case 0x1840: /* MONGOLIAN LETTER LHA */
    case 0x1841: /* MONGOLIAN LETTER ZHI */
    case 0x1842: /* MONGOLIAN LETTER CHI */
    case 0x1843: /* MONGOLIAN LETTER TODO LONG VOWEL SIGN */
    case 0x1844: /* MONGOLIAN LETTER TODO E */
    case 0x1845: /* MONGOLIAN LETTER TODO I */
    case 0x1846: /* MONGOLIAN LETTER TODO O */
    case 0x1847: /* MONGOLIAN LETTER TODO U */
    case 0x1848: /* MONGOLIAN LETTER TODO OE */
    case 0x1849: /* MONGOLIAN LETTER TODO UE */
    case 0x184A: /* MONGOLIAN LETTER TODO ANG */
    case 0x184B: /* MONGOLIAN LETTER TODO BA */
    case 0x184C: /* MONGOLIAN LETTER TODO PA */
    case 0x184D: /* MONGOLIAN LETTER TODO QA */
    case 0x184E: /* MONGOLIAN LETTER TODO GA */
    case 0x184F: /* MONGOLIAN LETTER TODO MA */
    case 0x1850: /* MONGOLIAN LETTER TODO TA */
    case 0x1851: /* MONGOLIAN LETTER TODO DA */
    case 0x1852: /* MONGOLIAN LETTER TODO CHA */
    case 0x1853: /* MONGOLIAN LETTER TODO JA */
    case 0x1854: /* MONGOLIAN LETTER TODO TSA */
    case 0x1855: /* MONGOLIAN LETTER TODO YA */
    case 0x1856: /* MONGOLIAN LETTER TODO WA */
    case 0x1857: /* MONGOLIAN LETTER TODO KA */
    case 0x1858: /* MONGOLIAN LETTER TODO GAA */
    case 0x1859: /* MONGOLIAN LETTER TODO HAA */
    case 0x185A: /* MONGOLIAN LETTER TODO JIA */
    case 0x185B: /* MONGOLIAN LETTER TODO NIA */
    case 0x185C: /* MONGOLIAN LETTER TODO DZA */
    case 0x185D: /* MONGOLIAN LETTER SIBE E */
    case 0x185E: /* MONGOLIAN LETTER SIBE I */
    case 0x185F: /* MONGOLIAN LETTER SIBE IY */
    case 0x1860: /* MONGOLIAN LETTER SIBE UE */
    case 0x1861: /* MONGOLIAN LETTER SIBE U */
    case 0x1862: /* MONGOLIAN LETTER SIBE ANG */
    case 0x1863: /* MONGOLIAN LETTER SIBE KA */
    case 0x1864: /* MONGOLIAN LETTER SIBE GA */
    case 0x1865: /* MONGOLIAN LETTER SIBE HA */
    case 0x1866: /* MONGOLIAN LETTER SIBE PA */
    case 0x1867: /* MONGOLIAN LETTER SIBE SHA */
    case 0x1868: /* MONGOLIAN LETTER SIBE TA */
    case 0x1869: /* MONGOLIAN LETTER SIBE DA */
    case 0x186A: /* MONGOLIAN LETTER SIBE JA */
    case 0x186B: /* MONGOLIAN LETTER SIBE FA */
    case 0x186C: /* MONGOLIAN LETTER SIBE GAA */
    case 0x186D: /* MONGOLIAN LETTER SIBE HAA */
    case 0x186E: /* MONGOLIAN LETTER SIBE TSA */
    case 0x186F: /* MONGOLIAN LETTER SIBE ZA */
    case 0x1870: /* MONGOLIAN LETTER SIBE RAA */
    case 0x1871: /* MONGOLIAN LETTER SIBE CHA */
    case 0x1872: /* MONGOLIAN LETTER SIBE ZHA */
    case 0x1873: /* MONGOLIAN LETTER MANCHU I */
    case 0x1874: /* MONGOLIAN LETTER MANCHU KA */
    case 0x1875: /* MONGOLIAN LETTER MANCHU RA */
    case 0x1876: /* MONGOLIAN LETTER MANCHU FA */
    case 0x1877: /* MONGOLIAN LETTER MANCHU ZHA */
    case 0x1880: /* MONGOLIAN LETTER ALI GALI ANUSVARA ONE */
    case 0x1881: /* MONGOLIAN LETTER ALI GALI VISARGA ONE */
    case 0x1882: /* MONGOLIAN LETTER ALI GALI DAMARU */
    case 0x1883: /* MONGOLIAN LETTER ALI GALI UBADAMA */
    case 0x1884: /* MONGOLIAN LETTER ALI GALI INVERTED UBADAMA */
    case 0x1885: /* MONGOLIAN LETTER ALI GALI BALUDA */
    case 0x1886: /* MONGOLIAN LETTER ALI GALI THREE BALUDA */
    case 0x1887: /* MONGOLIAN LETTER ALI GALI A */
    case 0x1888: /* MONGOLIAN LETTER ALI GALI I */
    case 0x1889: /* MONGOLIAN LETTER ALI GALI KA */
    case 0x188A: /* MONGOLIAN LETTER ALI GALI NGA */
    case 0x188B: /* MONGOLIAN LETTER ALI GALI CA */
    case 0x188C: /* MONGOLIAN LETTER ALI GALI TTA */
    case 0x188D: /* MONGOLIAN LETTER ALI GALI TTHA */
    case 0x188E: /* MONGOLIAN LETTER ALI GALI DDA */
    case 0x188F: /* MONGOLIAN LETTER ALI GALI NNA */
    case 0x1890: /* MONGOLIAN LETTER ALI GALI TA */
    case 0x1891: /* MONGOLIAN LETTER ALI GALI DA */
    case 0x1892: /* MONGOLIAN LETTER ALI GALI PA */
    case 0x1893: /* MONGOLIAN LETTER ALI GALI PHA */
    case 0x1894: /* MONGOLIAN LETTER ALI GALI SSA */
    case 0x1895: /* MONGOLIAN LETTER ALI GALI ZHA */
    case 0x1896: /* MONGOLIAN LETTER ALI GALI ZA */
    case 0x1897: /* MONGOLIAN LETTER ALI GALI AH */
    case 0x1898: /* MONGOLIAN LETTER TODO ALI GALI TA */
    case 0x1899: /* MONGOLIAN LETTER TODO ALI GALI ZHA */
    case 0x189A: /* MONGOLIAN LETTER MANCHU ALI GALI GHA */
    case 0x189B: /* MONGOLIAN LETTER MANCHU ALI GALI NGA */
    case 0x189C: /* MONGOLIAN LETTER MANCHU ALI GALI CA */
    case 0x189D: /* MONGOLIAN LETTER MANCHU ALI GALI JHA */
    case 0x189E: /* MONGOLIAN LETTER MANCHU ALI GALI TTA */
    case 0x189F: /* MONGOLIAN LETTER MANCHU ALI GALI DDHA */
    case 0x18A0: /* MONGOLIAN LETTER MANCHU ALI GALI TA */
    case 0x18A1: /* MONGOLIAN LETTER MANCHU ALI GALI DHA */
    case 0x18A2: /* MONGOLIAN LETTER MANCHU ALI GALI SSA */
    case 0x18A3: /* MONGOLIAN LETTER MANCHU ALI GALI CYA */
    case 0x18A4: /* MONGOLIAN LETTER MANCHU ALI GALI ZHA */
    case 0x18A5: /* MONGOLIAN LETTER MANCHU ALI GALI ZA */
    case 0x18A6: /* MONGOLIAN LETTER ALI GALI HALF U */
    case 0x18A7: /* MONGOLIAN LETTER ALI GALI HALF YA */
    case 0x18A8: /* MONGOLIAN LETTER MANCHU ALI GALI BHA */
    case 0x2135: /* ALEF SYMBOL */
    case 0x2136: /* BET SYMBOL */
    case 0x2137: /* GIMEL SYMBOL */
    case 0x2138: /* DALET SYMBOL */
    case 0x3005: /* IDEOGRAPHIC ITERATION MARK */
    case 0x3006: /* IDEOGRAPHIC CLOSING MARK */
    case 0x3031: /* VERTICAL KANA REPEAT MARK */
    case 0x3032: /* VERTICAL KANA REPEAT WITH VOICED SOUND MARK */
    case 0x3033: /* VERTICAL KANA REPEAT MARK UPPER HALF */
    case 0x3034: /* VERTICAL KANA REPEAT WITH VOICED SOUND MARK UPPER HALF */
    case 0x3035: /* VERTICAL KANA REPEAT MARK LOWER HALF */
    case 0x3041: /* HIRAGANA LETTER SMALL A */
    case 0x3042: /* HIRAGANA LETTER A */
    case 0x3043: /* HIRAGANA LETTER SMALL I */
    case 0x3044: /* HIRAGANA LETTER I */
    case 0x3045: /* HIRAGANA LETTER SMALL U */
    case 0x3046: /* HIRAGANA LETTER U */
    case 0x3047: /* HIRAGANA LETTER SMALL E */
    case 0x3048: /* HIRAGANA LETTER E */
    case 0x3049: /* HIRAGANA LETTER SMALL O */
    case 0x304A: /* HIRAGANA LETTER O */
    case 0x304B: /* HIRAGANA LETTER KA */
    case 0x304C: /* HIRAGANA LETTER GA */
    case 0x304D: /* HIRAGANA LETTER KI */
    case 0x304E: /* HIRAGANA LETTER GI */
    case 0x304F: /* HIRAGANA LETTER KU */
    case 0x3050: /* HIRAGANA LETTER GU */
    case 0x3051: /* HIRAGANA LETTER KE */
    case 0x3052: /* HIRAGANA LETTER GE */
    case 0x3053: /* HIRAGANA LETTER KO */
    case 0x3054: /* HIRAGANA LETTER GO */
    case 0x3055: /* HIRAGANA LETTER SA */
    case 0x3056: /* HIRAGANA LETTER ZA */
    case 0x3057: /* HIRAGANA LETTER SI */
    case 0x3058: /* HIRAGANA LETTER ZI */
    case 0x3059: /* HIRAGANA LETTER SU */
    case 0x305A: /* HIRAGANA LETTER ZU */
    case 0x305B: /* HIRAGANA LETTER SE */
    case 0x305C: /* HIRAGANA LETTER ZE */
    case 0x305D: /* HIRAGANA LETTER SO */
    case 0x305E: /* HIRAGANA LETTER ZO */
    case 0x305F: /* HIRAGANA LETTER TA */
    case 0x3060: /* HIRAGANA LETTER DA */
    case 0x3061: /* HIRAGANA LETTER TI */
    case 0x3062: /* HIRAGANA LETTER DI */
    case 0x3063: /* HIRAGANA LETTER SMALL TU */
    case 0x3064: /* HIRAGANA LETTER TU */
    case 0x3065: /* HIRAGANA LETTER DU */
    case 0x3066: /* HIRAGANA LETTER TE */
    case 0x3067: /* HIRAGANA LETTER DE */
    case 0x3068: /* HIRAGANA LETTER TO */
    case 0x3069: /* HIRAGANA LETTER DO */
    case 0x306A: /* HIRAGANA LETTER NA */
    case 0x306B: /* HIRAGANA LETTER NI */
    case 0x306C: /* HIRAGANA LETTER NU */
    case 0x306D: /* HIRAGANA LETTER NE */
    case 0x306E: /* HIRAGANA LETTER NO */
    case 0x306F: /* HIRAGANA LETTER HA */
    case 0x3070: /* HIRAGANA LETTER BA */
    case 0x3071: /* HIRAGANA LETTER PA */
    case 0x3072: /* HIRAGANA LETTER HI */
    case 0x3073: /* HIRAGANA LETTER BI */
    case 0x3074: /* HIRAGANA LETTER PI */
    case 0x3075: /* HIRAGANA LETTER HU */
    case 0x3076: /* HIRAGANA LETTER BU */
    case 0x3077: /* HIRAGANA LETTER PU */
    case 0x3078: /* HIRAGANA LETTER HE */
    case 0x3079: /* HIRAGANA LETTER BE */
    case 0x307A: /* HIRAGANA LETTER PE */
    case 0x307B: /* HIRAGANA LETTER HO */
    case 0x307C: /* HIRAGANA LETTER BO */
    case 0x307D: /* HIRAGANA LETTER PO */
    case 0x307E: /* HIRAGANA LETTER MA */
    case 0x307F: /* HIRAGANA LETTER MI */
    case 0x3080: /* HIRAGANA LETTER MU */
    case 0x3081: /* HIRAGANA LETTER ME */
    case 0x3082: /* HIRAGANA LETTER MO */
    case 0x3083: /* HIRAGANA LETTER SMALL YA */
    case 0x3084: /* HIRAGANA LETTER YA */
    case 0x3085: /* HIRAGANA LETTER SMALL YU */
    case 0x3086: /* HIRAGANA LETTER YU */
    case 0x3087: /* HIRAGANA LETTER SMALL YO */
    case 0x3088: /* HIRAGANA LETTER YO */
    case 0x3089: /* HIRAGANA LETTER RA */
    case 0x308A: /* HIRAGANA LETTER RI */
    case 0x308B: /* HIRAGANA LETTER RU */
    case 0x308C: /* HIRAGANA LETTER RE */
    case 0x308D: /* HIRAGANA LETTER RO */
    case 0x308E: /* HIRAGANA LETTER SMALL WA */
    case 0x308F: /* HIRAGANA LETTER WA */
    case 0x3090: /* HIRAGANA LETTER WI */
    case 0x3091: /* HIRAGANA LETTER WE */
    case 0x3092: /* HIRAGANA LETTER WO */
    case 0x3093: /* HIRAGANA LETTER N */
    case 0x3094: /* HIRAGANA LETTER VU */
    case 0x309D: /* HIRAGANA ITERATION MARK */
    case 0x309E: /* HIRAGANA VOICED ITERATION MARK */
    case 0x30A1: /* KATAKANA LETTER SMALL A */
    case 0x30A2: /* KATAKANA LETTER A */
    case 0x30A3: /* KATAKANA LETTER SMALL I */
    case 0x30A4: /* KATAKANA LETTER I */
    case 0x30A5: /* KATAKANA LETTER SMALL U */
    case 0x30A6: /* KATAKANA LETTER U */
    case 0x30A7: /* KATAKANA LETTER SMALL E */
    case 0x30A8: /* KATAKANA LETTER E */
    case 0x30A9: /* KATAKANA LETTER SMALL O */
    case 0x30AA: /* KATAKANA LETTER O */
    case 0x30AB: /* KATAKANA LETTER KA */
    case 0x30AC: /* KATAKANA LETTER GA */
    case 0x30AD: /* KATAKANA LETTER KI */
    case 0x30AE: /* KATAKANA LETTER GI */
    case 0x30AF: /* KATAKANA LETTER KU */
    case 0x30B0: /* KATAKANA LETTER GU */
    case 0x30B1: /* KATAKANA LETTER KE */
    case 0x30B2: /* KATAKANA LETTER GE */
    case 0x30B3: /* KATAKANA LETTER KO */
    case 0x30B4: /* KATAKANA LETTER GO */
    case 0x30B5: /* KATAKANA LETTER SA */
    case 0x30B6: /* KATAKANA LETTER ZA */
    case 0x30B7: /* KATAKANA LETTER SI */
    case 0x30B8: /* KATAKANA LETTER ZI */
    case 0x30B9: /* KATAKANA LETTER SU */
    case 0x30BA: /* KATAKANA LETTER ZU */
    case 0x30BB: /* KATAKANA LETTER SE */
    case 0x30BC: /* KATAKANA LETTER ZE */
    case 0x30BD: /* KATAKANA LETTER SO */
    case 0x30BE: /* KATAKANA LETTER ZO */
    case 0x30BF: /* KATAKANA LETTER TA */
    case 0x30C0: /* KATAKANA LETTER DA */
    case 0x30C1: /* KATAKANA LETTER TI */
    case 0x30C2: /* KATAKANA LETTER DI */
    case 0x30C3: /* KATAKANA LETTER SMALL TU */
    case 0x30C4: /* KATAKANA LETTER TU */
    case 0x30C5: /* KATAKANA LETTER DU */
    case 0x30C6: /* KATAKANA LETTER TE */
    case 0x30C7: /* KATAKANA LETTER DE */
    case 0x30C8: /* KATAKANA LETTER TO */
    case 0x30C9: /* KATAKANA LETTER DO */
    case 0x30CA: /* KATAKANA LETTER NA */
    case 0x30CB: /* KATAKANA LETTER NI */
    case 0x30CC: /* KATAKANA LETTER NU */
    case 0x30CD: /* KATAKANA LETTER NE */
    case 0x30CE: /* KATAKANA LETTER NO */
    case 0x30CF: /* KATAKANA LETTER HA */
    case 0x30D0: /* KATAKANA LETTER BA */
    case 0x30D1: /* KATAKANA LETTER PA */
    case 0x30D2: /* KATAKANA LETTER HI */
    case 0x30D3: /* KATAKANA LETTER BI */
    case 0x30D4: /* KATAKANA LETTER PI */
    case 0x30D5: /* KATAKANA LETTER HU */
    case 0x30D6: /* KATAKANA LETTER BU */
    case 0x30D7: /* KATAKANA LETTER PU */
    case 0x30D8: /* KATAKANA LETTER HE */
    case 0x30D9: /* KATAKANA LETTER BE */
    case 0x30DA: /* KATAKANA LETTER PE */
    case 0x30DB: /* KATAKANA LETTER HO */
    case 0x30DC: /* KATAKANA LETTER BO */
    case 0x30DD: /* KATAKANA LETTER PO */
    case 0x30DE: /* KATAKANA LETTER MA */
    case 0x30DF: /* KATAKANA LETTER MI */
    case 0x30E0: /* KATAKANA LETTER MU */
    case 0x30E1: /* KATAKANA LETTER ME */
    case 0x30E2: /* KATAKANA LETTER MO */
    case 0x30E3: /* KATAKANA LETTER SMALL YA */
    case 0x30E4: /* KATAKANA LETTER YA */
    case 0x30E5: /* KATAKANA LETTER SMALL YU */
    case 0x30E6: /* KATAKANA LETTER YU */
    case 0x30E7: /* KATAKANA LETTER SMALL YO */
    case 0x30E8: /* KATAKANA LETTER YO */
    case 0x30E9: /* KATAKANA LETTER RA */
    case 0x30EA: /* KATAKANA LETTER RI */
    case 0x30EB: /* KATAKANA LETTER RU */
    case 0x30EC: /* KATAKANA LETTER RE */
    case 0x30ED: /* KATAKANA LETTER RO */
    case 0x30EE: /* KATAKANA LETTER SMALL WA */
    case 0x30EF: /* KATAKANA LETTER WA */
    case 0x30F0: /* KATAKANA LETTER WI */
    case 0x30F1: /* KATAKANA LETTER WE */
    case 0x30F2: /* KATAKANA LETTER WO */
    case 0x30F3: /* KATAKANA LETTER N */
    case 0x30F4: /* KATAKANA LETTER VU */
    case 0x30F5: /* KATAKANA LETTER SMALL KA */
    case 0x30F6: /* KATAKANA LETTER SMALL KE */
    case 0x30F7: /* KATAKANA LETTER VA */
    case 0x30F8: /* KATAKANA LETTER VI */
    case 0x30F9: /* KATAKANA LETTER VE */
    case 0x30FA: /* KATAKANA LETTER VO */
    case 0x30FC: /* KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    case 0x30FD: /* KATAKANA ITERATION MARK */
    case 0x30FE: /* KATAKANA VOICED ITERATION MARK */
    case 0x3105: /* BOPOMOFO LETTER B */
    case 0x3106: /* BOPOMOFO LETTER P */
    case 0x3107: /* BOPOMOFO LETTER M */
    case 0x3108: /* BOPOMOFO LETTER F */
    case 0x3109: /* BOPOMOFO LETTER D */
    case 0x310A: /* BOPOMOFO LETTER T */
    case 0x310B: /* BOPOMOFO LETTER N */
    case 0x310C: /* BOPOMOFO LETTER L */
    case 0x310D: /* BOPOMOFO LETTER G */
    case 0x310E: /* BOPOMOFO LETTER K */
    case 0x310F: /* BOPOMOFO LETTER H */
    case 0x3110: /* BOPOMOFO LETTER J */
    case 0x3111: /* BOPOMOFO LETTER Q */
    case 0x3112: /* BOPOMOFO LETTER X */
    case 0x3113: /* BOPOMOFO LETTER ZH */
    case 0x3114: /* BOPOMOFO LETTER CH */
    case 0x3115: /* BOPOMOFO LETTER SH */
    case 0x3116: /* BOPOMOFO LETTER R */
    case 0x3117: /* BOPOMOFO LETTER Z */
    case 0x3118: /* BOPOMOFO LETTER C */
    case 0x3119: /* BOPOMOFO LETTER S */
    case 0x311A: /* BOPOMOFO LETTER A */
    case 0x311B: /* BOPOMOFO LETTER O */
    case 0x311C: /* BOPOMOFO LETTER E */
    case 0x311D: /* BOPOMOFO LETTER EH */
    case 0x311E: /* BOPOMOFO LETTER AI */
    case 0x311F: /* BOPOMOFO LETTER EI */
    case 0x3120: /* BOPOMOFO LETTER AU */
    case 0x3121: /* BOPOMOFO LETTER OU */
    case 0x3122: /* BOPOMOFO LETTER AN */
    case 0x3123: /* BOPOMOFO LETTER EN */
    case 0x3124: /* BOPOMOFO LETTER ANG */
    case 0x3125: /* BOPOMOFO LETTER ENG */
    case 0x3126: /* BOPOMOFO LETTER ER */
    case 0x3127: /* BOPOMOFO LETTER I */
    case 0x3128: /* BOPOMOFO LETTER U */
    case 0x3129: /* BOPOMOFO LETTER IU */
    case 0x312A: /* BOPOMOFO LETTER V */
    case 0x312B: /* BOPOMOFO LETTER NG */
    case 0x312C: /* BOPOMOFO LETTER GN */
    case 0x3131: /* HANGUL LETTER KIYEOK */
    case 0x3132: /* HANGUL LETTER SSANGKIYEOK */
    case 0x3133: /* HANGUL LETTER KIYEOK-SIOS */
    case 0x3134: /* HANGUL LETTER NIEUN */
    case 0x3135: /* HANGUL LETTER NIEUN-CIEUC */
    case 0x3136: /* HANGUL LETTER NIEUN-HIEUH */
    case 0x3137: /* HANGUL LETTER TIKEUT */
    case 0x3138: /* HANGUL LETTER SSANGTIKEUT */
    case 0x3139: /* HANGUL LETTER RIEUL */
    case 0x313A: /* HANGUL LETTER RIEUL-KIYEOK */
    case 0x313B: /* HANGUL LETTER RIEUL-MIEUM */
    case 0x313C: /* HANGUL LETTER RIEUL-PIEUP */
    case 0x313D: /* HANGUL LETTER RIEUL-SIOS */
    case 0x313E: /* HANGUL LETTER RIEUL-THIEUTH */
    case 0x313F: /* HANGUL LETTER RIEUL-PHIEUPH */
    case 0x3140: /* HANGUL LETTER RIEUL-HIEUH */
    case 0x3141: /* HANGUL LETTER MIEUM */
    case 0x3142: /* HANGUL LETTER PIEUP */
    case 0x3143: /* HANGUL LETTER SSANGPIEUP */
    case 0x3144: /* HANGUL LETTER PIEUP-SIOS */
    case 0x3145: /* HANGUL LETTER SIOS */
    case 0x3146: /* HANGUL LETTER SSANGSIOS */
    case 0x3147: /* HANGUL LETTER IEUNG */
    case 0x3148: /* HANGUL LETTER CIEUC */
    case 0x3149: /* HANGUL LETTER SSANGCIEUC */
    case 0x314A: /* HANGUL LETTER CHIEUCH */
    case 0x314B: /* HANGUL LETTER KHIEUKH */
    case 0x314C: /* HANGUL LETTER THIEUTH */
    case 0x314D: /* HANGUL LETTER PHIEUPH */
    case 0x314E: /* HANGUL LETTER HIEUH */
    case 0x314F: /* HANGUL LETTER A */
    case 0x3150: /* HANGUL LETTER AE */
    case 0x3151: /* HANGUL LETTER YA */
    case 0x3152: /* HANGUL LETTER YAE */
    case 0x3153: /* HANGUL LETTER EO */
    case 0x3154: /* HANGUL LETTER E */
    case 0x3155: /* HANGUL LETTER YEO */
    case 0x3156: /* HANGUL LETTER YE */
    case 0x3157: /* HANGUL LETTER O */
    case 0x3158: /* HANGUL LETTER WA */
    case 0x3159: /* HANGUL LETTER WAE */
    case 0x315A: /* HANGUL LETTER OE */
    case 0x315B: /* HANGUL LETTER YO */
    case 0x315C: /* HANGUL LETTER U */
    case 0x315D: /* HANGUL LETTER WEO */
    case 0x315E: /* HANGUL LETTER WE */
    case 0x315F: /* HANGUL LETTER WI */
    case 0x3160: /* HANGUL LETTER YU */
    case 0x3161: /* HANGUL LETTER EU */
    case 0x3162: /* HANGUL LETTER YI */
    case 0x3163: /* HANGUL LETTER I */
    case 0x3164: /* HANGUL FILLER */
    case 0x3165: /* HANGUL LETTER SSANGNIEUN */
    case 0x3166: /* HANGUL LETTER NIEUN-TIKEUT */
    case 0x3167: /* HANGUL LETTER NIEUN-SIOS */
    case 0x3168: /* HANGUL LETTER NIEUN-PANSIOS */
    case 0x3169: /* HANGUL LETTER RIEUL-KIYEOK-SIOS */
    case 0x316A: /* HANGUL LETTER RIEUL-TIKEUT */
    case 0x316B: /* HANGUL LETTER RIEUL-PIEUP-SIOS */
    case 0x316C: /* HANGUL LETTER RIEUL-PANSIOS */
    case 0x316D: /* HANGUL LETTER RIEUL-YEORINHIEUH */
    case 0x316E: /* HANGUL LETTER MIEUM-PIEUP */
    case 0x316F: /* HANGUL LETTER MIEUM-SIOS */
    case 0x3170: /* HANGUL LETTER MIEUM-PANSIOS */
    case 0x3171: /* HANGUL LETTER KAPYEOUNMIEUM */
    case 0x3172: /* HANGUL LETTER PIEUP-KIYEOK */
    case 0x3173: /* HANGUL LETTER PIEUP-TIKEUT */
    case 0x3174: /* HANGUL LETTER PIEUP-SIOS-KIYEOK */
    case 0x3175: /* HANGUL LETTER PIEUP-SIOS-TIKEUT */
    case 0x3176: /* HANGUL LETTER PIEUP-CIEUC */
    case 0x3177: /* HANGUL LETTER PIEUP-THIEUTH */
    case 0x3178: /* HANGUL LETTER KAPYEOUNPIEUP */
    case 0x3179: /* HANGUL LETTER KAPYEOUNSSANGPIEUP */
    case 0x317A: /* HANGUL LETTER SIOS-KIYEOK */
    case 0x317B: /* HANGUL LETTER SIOS-NIEUN */
    case 0x317C: /* HANGUL LETTER SIOS-TIKEUT */
    case 0x317D: /* HANGUL LETTER SIOS-PIEUP */
    case 0x317E: /* HANGUL LETTER SIOS-CIEUC */
    case 0x317F: /* HANGUL LETTER PANSIOS */
    case 0x3180: /* HANGUL LETTER SSANGIEUNG */
    case 0x3181: /* HANGUL LETTER YESIEUNG */
    case 0x3182: /* HANGUL LETTER YESIEUNG-SIOS */
    case 0x3183: /* HANGUL LETTER YESIEUNG-PANSIOS */
    case 0x3184: /* HANGUL LETTER KAPYEOUNPHIEUPH */
    case 0x3185: /* HANGUL LETTER SSANGHIEUH */
    case 0x3186: /* HANGUL LETTER YEORINHIEUH */
    case 0x3187: /* HANGUL LETTER YO-YA */
    case 0x3188: /* HANGUL LETTER YO-YAE */
    case 0x3189: /* HANGUL LETTER YO-I */
    case 0x318A: /* HANGUL LETTER YU-YEO */
    case 0x318B: /* HANGUL LETTER YU-YE */
    case 0x318C: /* HANGUL LETTER YU-I */
    case 0x318D: /* HANGUL LETTER ARAEA */
    case 0x318E: /* HANGUL LETTER ARAEAE */
    case 0x31A0: /* BOPOMOFO LETTER BU */
    case 0x31A1: /* BOPOMOFO LETTER ZI */
    case 0x31A2: /* BOPOMOFO LETTER JI */
    case 0x31A3: /* BOPOMOFO LETTER GU */
    case 0x31A4: /* BOPOMOFO LETTER EE */
    case 0x31A5: /* BOPOMOFO LETTER ENN */
    case 0x31A6: /* BOPOMOFO LETTER OO */
    case 0x31A7: /* BOPOMOFO LETTER ONN */
    case 0x31A8: /* BOPOMOFO LETTER IR */
    case 0x31A9: /* BOPOMOFO LETTER ANN */
    case 0x31AA: /* BOPOMOFO LETTER INN */
    case 0x31AB: /* BOPOMOFO LETTER UNN */
    case 0x31AC: /* BOPOMOFO LETTER IM */
    case 0x31AD: /* BOPOMOFO LETTER NGG */
    case 0x31AE: /* BOPOMOFO LETTER AINN */
    case 0x31AF: /* BOPOMOFO LETTER AUNN */
    case 0x31B0: /* BOPOMOFO LETTER AM */
    case 0x31B1: /* BOPOMOFO LETTER OM */
    case 0x31B2: /* BOPOMOFO LETTER ONG */
    case 0x31B3: /* BOPOMOFO LETTER INNN */
    case 0x31B4: /* BOPOMOFO FINAL LETTER P */
    case 0x31B5: /* BOPOMOFO FINAL LETTER T */
    case 0x31B6: /* BOPOMOFO FINAL LETTER K */
    case 0x31B7: /* BOPOMOFO FINAL LETTER H */
    case 0x3400: /* <CJK Ideograph Extension A, First> */
    case 0x4DB5: /* <CJK Ideograph Extension A, Last> */
    case 0x4E00: /* <CJK Ideograph, First> */
    case 0x9FA5: /* <CJK Ideograph, Last> */
    case 0xA000: /* YI SYLLABLE IT */
    case 0xA001: /* YI SYLLABLE IX */
    case 0xA002: /* YI SYLLABLE I */
    case 0xA003: /* YI SYLLABLE IP */
    case 0xA004: /* YI SYLLABLE IET */
    case 0xA005: /* YI SYLLABLE IEX */
    case 0xA006: /* YI SYLLABLE IE */
    case 0xA007: /* YI SYLLABLE IEP */
    case 0xA008: /* YI SYLLABLE AT */
    case 0xA009: /* YI SYLLABLE AX */
    case 0xA00A: /* YI SYLLABLE A */
    case 0xA00B: /* YI SYLLABLE AP */
    case 0xA00C: /* YI SYLLABLE UOX */
    case 0xA00D: /* YI SYLLABLE UO */
    case 0xA00E: /* YI SYLLABLE UOP */
    case 0xA00F: /* YI SYLLABLE OT */
    case 0xA010: /* YI SYLLABLE OX */
    case 0xA011: /* YI SYLLABLE O */
    case 0xA012: /* YI SYLLABLE OP */
    case 0xA013: /* YI SYLLABLE EX */
    case 0xA014: /* YI SYLLABLE E */
    case 0xA015: /* YI SYLLABLE WU */
    case 0xA016: /* YI SYLLABLE BIT */
    case 0xA017: /* YI SYLLABLE BIX */
    case 0xA018: /* YI SYLLABLE BI */
    case 0xA019: /* YI SYLLABLE BIP */
    case 0xA01A: /* YI SYLLABLE BIET */
    case 0xA01B: /* YI SYLLABLE BIEX */
    case 0xA01C: /* YI SYLLABLE BIE */
    case 0xA01D: /* YI SYLLABLE BIEP */
    case 0xA01E: /* YI SYLLABLE BAT */
    case 0xA01F: /* YI SYLLABLE BAX */
    case 0xA020: /* YI SYLLABLE BA */
    case 0xA021: /* YI SYLLABLE BAP */
    case 0xA022: /* YI SYLLABLE BUOX */
    case 0xA023: /* YI SYLLABLE BUO */
    case 0xA024: /* YI SYLLABLE BUOP */
    case 0xA025: /* YI SYLLABLE BOT */
    case 0xA026: /* YI SYLLABLE BOX */
    case 0xA027: /* YI SYLLABLE BO */
    case 0xA028: /* YI SYLLABLE BOP */
    case 0xA029: /* YI SYLLABLE BEX */
    case 0xA02A: /* YI SYLLABLE BE */
    case 0xA02B: /* YI SYLLABLE BEP */
BREAK_SWITCH_UP
    case 0xA02C: /* YI SYLLABLE BUT */
    case 0xA02D: /* YI SYLLABLE BUX */
    case 0xA02E: /* YI SYLLABLE BU */
    case 0xA02F: /* YI SYLLABLE BUP */
    case 0xA030: /* YI SYLLABLE BURX */
    case 0xA031: /* YI SYLLABLE BUR */
    case 0xA032: /* YI SYLLABLE BYT */
    case 0xA033: /* YI SYLLABLE BYX */
    case 0xA034: /* YI SYLLABLE BY */
    case 0xA035: /* YI SYLLABLE BYP */
    case 0xA036: /* YI SYLLABLE BYRX */
    case 0xA037: /* YI SYLLABLE BYR */
    case 0xA038: /* YI SYLLABLE PIT */
    case 0xA039: /* YI SYLLABLE PIX */
    case 0xA03A: /* YI SYLLABLE PI */
    case 0xA03B: /* YI SYLLABLE PIP */
    case 0xA03C: /* YI SYLLABLE PIEX */
    case 0xA03D: /* YI SYLLABLE PIE */
    case 0xA03E: /* YI SYLLABLE PIEP */
    case 0xA03F: /* YI SYLLABLE PAT */
    case 0xA040: /* YI SYLLABLE PAX */
    case 0xA041: /* YI SYLLABLE PA */
    case 0xA042: /* YI SYLLABLE PAP */
    case 0xA043: /* YI SYLLABLE PUOX */
    case 0xA044: /* YI SYLLABLE PUO */
    case 0xA045: /* YI SYLLABLE PUOP */
    case 0xA046: /* YI SYLLABLE POT */
    case 0xA047: /* YI SYLLABLE POX */
    case 0xA048: /* YI SYLLABLE PO */
    case 0xA049: /* YI SYLLABLE POP */
    case 0xA04A: /* YI SYLLABLE PUT */
    case 0xA04B: /* YI SYLLABLE PUX */
    case 0xA04C: /* YI SYLLABLE PU */
    case 0xA04D: /* YI SYLLABLE PUP */
    case 0xA04E: /* YI SYLLABLE PURX */
    case 0xA04F: /* YI SYLLABLE PUR */
    case 0xA050: /* YI SYLLABLE PYT */
    case 0xA051: /* YI SYLLABLE PYX */
    case 0xA052: /* YI SYLLABLE PY */
    case 0xA053: /* YI SYLLABLE PYP */
    case 0xA054: /* YI SYLLABLE PYRX */
    case 0xA055: /* YI SYLLABLE PYR */
    case 0xA056: /* YI SYLLABLE BBIT */
    case 0xA057: /* YI SYLLABLE BBIX */
    case 0xA058: /* YI SYLLABLE BBI */
    case 0xA059: /* YI SYLLABLE BBIP */
    case 0xA05A: /* YI SYLLABLE BBIET */
    case 0xA05B: /* YI SYLLABLE BBIEX */
    case 0xA05C: /* YI SYLLABLE BBIE */
    case 0xA05D: /* YI SYLLABLE BBIEP */
    case 0xA05E: /* YI SYLLABLE BBAT */
    case 0xA05F: /* YI SYLLABLE BBAX */
    case 0xA060: /* YI SYLLABLE BBA */
    case 0xA061: /* YI SYLLABLE BBAP */
    case 0xA062: /* YI SYLLABLE BBUOX */
    case 0xA063: /* YI SYLLABLE BBUO */
    case 0xA064: /* YI SYLLABLE BBUOP */
    case 0xA065: /* YI SYLLABLE BBOT */
    case 0xA066: /* YI SYLLABLE BBOX */
    case 0xA067: /* YI SYLLABLE BBO */
    case 0xA068: /* YI SYLLABLE BBOP */
    case 0xA069: /* YI SYLLABLE BBEX */
    case 0xA06A: /* YI SYLLABLE BBE */
    case 0xA06B: /* YI SYLLABLE BBEP */
    case 0xA06C: /* YI SYLLABLE BBUT */
    case 0xA06D: /* YI SYLLABLE BBUX */
    case 0xA06E: /* YI SYLLABLE BBU */
    case 0xA06F: /* YI SYLLABLE BBUP */
    case 0xA070: /* YI SYLLABLE BBURX */
    case 0xA071: /* YI SYLLABLE BBUR */
    case 0xA072: /* YI SYLLABLE BBYT */
    case 0xA073: /* YI SYLLABLE BBYX */
    case 0xA074: /* YI SYLLABLE BBY */
    case 0xA075: /* YI SYLLABLE BBYP */
    case 0xA076: /* YI SYLLABLE NBIT */
    case 0xA077: /* YI SYLLABLE NBIX */
    case 0xA078: /* YI SYLLABLE NBI */
    case 0xA079: /* YI SYLLABLE NBIP */
    case 0xA07A: /* YI SYLLABLE NBIEX */
    case 0xA07B: /* YI SYLLABLE NBIE */
    case 0xA07C: /* YI SYLLABLE NBIEP */
    case 0xA07D: /* YI SYLLABLE NBAT */
    case 0xA07E: /* YI SYLLABLE NBAX */
    case 0xA07F: /* YI SYLLABLE NBA */
    case 0xA080: /* YI SYLLABLE NBAP */
    case 0xA081: /* YI SYLLABLE NBOT */
    case 0xA082: /* YI SYLLABLE NBOX */
    case 0xA083: /* YI SYLLABLE NBO */
    case 0xA084: /* YI SYLLABLE NBOP */
    case 0xA085: /* YI SYLLABLE NBUT */
    case 0xA086: /* YI SYLLABLE NBUX */
    case 0xA087: /* YI SYLLABLE NBU */
    case 0xA088: /* YI SYLLABLE NBUP */
    case 0xA089: /* YI SYLLABLE NBURX */
    case 0xA08A: /* YI SYLLABLE NBUR */
    case 0xA08B: /* YI SYLLABLE NBYT */
    case 0xA08C: /* YI SYLLABLE NBYX */
    case 0xA08D: /* YI SYLLABLE NBY */
    case 0xA08E: /* YI SYLLABLE NBYP */
    case 0xA08F: /* YI SYLLABLE NBYRX */
    case 0xA090: /* YI SYLLABLE NBYR */
    case 0xA091: /* YI SYLLABLE HMIT */
    case 0xA092: /* YI SYLLABLE HMIX */
    case 0xA093: /* YI SYLLABLE HMI */
    case 0xA094: /* YI SYLLABLE HMIP */
    case 0xA095: /* YI SYLLABLE HMIEX */
    case 0xA096: /* YI SYLLABLE HMIE */
    case 0xA097: /* YI SYLLABLE HMIEP */
    case 0xA098: /* YI SYLLABLE HMAT */
    case 0xA099: /* YI SYLLABLE HMAX */
    case 0xA09A: /* YI SYLLABLE HMA */
    case 0xA09B: /* YI SYLLABLE HMAP */
    case 0xA09C: /* YI SYLLABLE HMUOX */
    case 0xA09D: /* YI SYLLABLE HMUO */
    case 0xA09E: /* YI SYLLABLE HMUOP */
    case 0xA09F: /* YI SYLLABLE HMOT */
    case 0xA0A0: /* YI SYLLABLE HMOX */
    case 0xA0A1: /* YI SYLLABLE HMO */
    case 0xA0A2: /* YI SYLLABLE HMOP */
    case 0xA0A3: /* YI SYLLABLE HMUT */
    case 0xA0A4: /* YI SYLLABLE HMUX */
    case 0xA0A5: /* YI SYLLABLE HMU */
    case 0xA0A6: /* YI SYLLABLE HMUP */
    case 0xA0A7: /* YI SYLLABLE HMURX */
    case 0xA0A8: /* YI SYLLABLE HMUR */
    case 0xA0A9: /* YI SYLLABLE HMYX */
    case 0xA0AA: /* YI SYLLABLE HMY */
    case 0xA0AB: /* YI SYLLABLE HMYP */
    case 0xA0AC: /* YI SYLLABLE HMYRX */
    case 0xA0AD: /* YI SYLLABLE HMYR */
    case 0xA0AE: /* YI SYLLABLE MIT */
    case 0xA0AF: /* YI SYLLABLE MIX */
    case 0xA0B0: /* YI SYLLABLE MI */
    case 0xA0B1: /* YI SYLLABLE MIP */
    case 0xA0B2: /* YI SYLLABLE MIEX */
    case 0xA0B3: /* YI SYLLABLE MIE */
    case 0xA0B4: /* YI SYLLABLE MIEP */
    case 0xA0B5: /* YI SYLLABLE MAT */
    case 0xA0B6: /* YI SYLLABLE MAX */
    case 0xA0B7: /* YI SYLLABLE MA */
    case 0xA0B8: /* YI SYLLABLE MAP */
    case 0xA0B9: /* YI SYLLABLE MUOT */
    case 0xA0BA: /* YI SYLLABLE MUOX */
    case 0xA0BB: /* YI SYLLABLE MUO */
    case 0xA0BC: /* YI SYLLABLE MUOP */
    case 0xA0BD: /* YI SYLLABLE MOT */
    case 0xA0BE: /* YI SYLLABLE MOX */
    case 0xA0BF: /* YI SYLLABLE MO */
    case 0xA0C0: /* YI SYLLABLE MOP */
    case 0xA0C1: /* YI SYLLABLE MEX */
    case 0xA0C2: /* YI SYLLABLE ME */
    case 0xA0C3: /* YI SYLLABLE MUT */
    case 0xA0C4: /* YI SYLLABLE MUX */
    case 0xA0C5: /* YI SYLLABLE MU */
    case 0xA0C6: /* YI SYLLABLE MUP */
    case 0xA0C7: /* YI SYLLABLE MURX */
    case 0xA0C8: /* YI SYLLABLE MUR */
    case 0xA0C9: /* YI SYLLABLE MYT */
    case 0xA0CA: /* YI SYLLABLE MYX */
    case 0xA0CB: /* YI SYLLABLE MY */
    case 0xA0CC: /* YI SYLLABLE MYP */
    case 0xA0CD: /* YI SYLLABLE FIT */
    case 0xA0CE: /* YI SYLLABLE FIX */
    case 0xA0CF: /* YI SYLLABLE FI */
    case 0xA0D0: /* YI SYLLABLE FIP */
    case 0xA0D1: /* YI SYLLABLE FAT */
    case 0xA0D2: /* YI SYLLABLE FAX */
    case 0xA0D3: /* YI SYLLABLE FA */
    case 0xA0D4: /* YI SYLLABLE FAP */
    case 0xA0D5: /* YI SYLLABLE FOX */
    case 0xA0D6: /* YI SYLLABLE FO */
    case 0xA0D7: /* YI SYLLABLE FOP */
    case 0xA0D8: /* YI SYLLABLE FUT */
    case 0xA0D9: /* YI SYLLABLE FUX */
    case 0xA0DA: /* YI SYLLABLE FU */
    case 0xA0DB: /* YI SYLLABLE FUP */
    case 0xA0DC: /* YI SYLLABLE FURX */
    case 0xA0DD: /* YI SYLLABLE FUR */
    case 0xA0DE: /* YI SYLLABLE FYT */
    case 0xA0DF: /* YI SYLLABLE FYX */
    case 0xA0E0: /* YI SYLLABLE FY */
    case 0xA0E1: /* YI SYLLABLE FYP */
    case 0xA0E2: /* YI SYLLABLE VIT */
    case 0xA0E3: /* YI SYLLABLE VIX */
    case 0xA0E4: /* YI SYLLABLE VI */
    case 0xA0E5: /* YI SYLLABLE VIP */
    case 0xA0E6: /* YI SYLLABLE VIET */
    case 0xA0E7: /* YI SYLLABLE VIEX */
    case 0xA0E8: /* YI SYLLABLE VIE */
    case 0xA0E9: /* YI SYLLABLE VIEP */
    case 0xA0EA: /* YI SYLLABLE VAT */
    case 0xA0EB: /* YI SYLLABLE VAX */
    case 0xA0EC: /* YI SYLLABLE VA */
    case 0xA0ED: /* YI SYLLABLE VAP */
    case 0xA0EE: /* YI SYLLABLE VOT */
    case 0xA0EF: /* YI SYLLABLE VOX */
    case 0xA0F0: /* YI SYLLABLE VO */
    case 0xA0F1: /* YI SYLLABLE VOP */
    case 0xA0F2: /* YI SYLLABLE VEX */
    case 0xA0F3: /* YI SYLLABLE VEP */
    case 0xA0F4: /* YI SYLLABLE VUT */
    case 0xA0F5: /* YI SYLLABLE VUX */
    case 0xA0F6: /* YI SYLLABLE VU */
    case 0xA0F7: /* YI SYLLABLE VUP */
    case 0xA0F8: /* YI SYLLABLE VURX */
    case 0xA0F9: /* YI SYLLABLE VUR */
    case 0xA0FA: /* YI SYLLABLE VYT */
    case 0xA0FB: /* YI SYLLABLE VYX */
    case 0xA0FC: /* YI SYLLABLE VY */
    case 0xA0FD: /* YI SYLLABLE VYP */
    case 0xA0FE: /* YI SYLLABLE VYRX */
    case 0xA0FF: /* YI SYLLABLE VYR */
    case 0xA100: /* YI SYLLABLE DIT */
    case 0xA101: /* YI SYLLABLE DIX */
    case 0xA102: /* YI SYLLABLE DI */
    case 0xA103: /* YI SYLLABLE DIP */
    case 0xA104: /* YI SYLLABLE DIEX */
    case 0xA105: /* YI SYLLABLE DIE */
    case 0xA106: /* YI SYLLABLE DIEP */
    case 0xA107: /* YI SYLLABLE DAT */
    case 0xA108: /* YI SYLLABLE DAX */
    case 0xA109: /* YI SYLLABLE DA */
    case 0xA10A: /* YI SYLLABLE DAP */
    case 0xA10B: /* YI SYLLABLE DUOX */
    case 0xA10C: /* YI SYLLABLE DUO */
    case 0xA10D: /* YI SYLLABLE DOT */
    case 0xA10E: /* YI SYLLABLE DOX */
    case 0xA10F: /* YI SYLLABLE DO */
    case 0xA110: /* YI SYLLABLE DOP */
    case 0xA111: /* YI SYLLABLE DEX */
    case 0xA112: /* YI SYLLABLE DE */
    case 0xA113: /* YI SYLLABLE DEP */
    case 0xA114: /* YI SYLLABLE DUT */
    case 0xA115: /* YI SYLLABLE DUX */
    case 0xA116: /* YI SYLLABLE DU */
    case 0xA117: /* YI SYLLABLE DUP */
    case 0xA118: /* YI SYLLABLE DURX */
    case 0xA119: /* YI SYLLABLE DUR */
    case 0xA11A: /* YI SYLLABLE TIT */
    case 0xA11B: /* YI SYLLABLE TIX */
    case 0xA11C: /* YI SYLLABLE TI */
    case 0xA11D: /* YI SYLLABLE TIP */
    case 0xA11E: /* YI SYLLABLE TIEX */
    case 0xA11F: /* YI SYLLABLE TIE */
    case 0xA120: /* YI SYLLABLE TIEP */
    case 0xA121: /* YI SYLLABLE TAT */
    case 0xA122: /* YI SYLLABLE TAX */
    case 0xA123: /* YI SYLLABLE TA */
    case 0xA124: /* YI SYLLABLE TAP */
    case 0xA125: /* YI SYLLABLE TUOT */
    case 0xA126: /* YI SYLLABLE TUOX */
    case 0xA127: /* YI SYLLABLE TUO */
    case 0xA128: /* YI SYLLABLE TUOP */
    case 0xA129: /* YI SYLLABLE TOT */
    case 0xA12A: /* YI SYLLABLE TOX */
    case 0xA12B: /* YI SYLLABLE TO */
    case 0xA12C: /* YI SYLLABLE TOP */
    case 0xA12D: /* YI SYLLABLE TEX */
    case 0xA12E: /* YI SYLLABLE TE */
    case 0xA12F: /* YI SYLLABLE TEP */
    case 0xA130: /* YI SYLLABLE TUT */
    case 0xA131: /* YI SYLLABLE TUX */
    case 0xA132: /* YI SYLLABLE TU */
    case 0xA133: /* YI SYLLABLE TUP */
    case 0xA134: /* YI SYLLABLE TURX */
    case 0xA135: /* YI SYLLABLE TUR */
    case 0xA136: /* YI SYLLABLE DDIT */
    case 0xA137: /* YI SYLLABLE DDIX */
    case 0xA138: /* YI SYLLABLE DDI */
    case 0xA139: /* YI SYLLABLE DDIP */
    case 0xA13A: /* YI SYLLABLE DDIEX */
    case 0xA13B: /* YI SYLLABLE DDIE */
    case 0xA13C: /* YI SYLLABLE DDIEP */
    case 0xA13D: /* YI SYLLABLE DDAT */
    case 0xA13E: /* YI SYLLABLE DDAX */
    case 0xA13F: /* YI SYLLABLE DDA */
    case 0xA140: /* YI SYLLABLE DDAP */
    case 0xA141: /* YI SYLLABLE DDUOX */
    case 0xA142: /* YI SYLLABLE DDUO */
    case 0xA143: /* YI SYLLABLE DDUOP */
    case 0xA144: /* YI SYLLABLE DDOT */
    case 0xA145: /* YI SYLLABLE DDOX */
    case 0xA146: /* YI SYLLABLE DDO */
    case 0xA147: /* YI SYLLABLE DDOP */
    case 0xA148: /* YI SYLLABLE DDEX */
    case 0xA149: /* YI SYLLABLE DDE */
    case 0xA14A: /* YI SYLLABLE DDEP */
    case 0xA14B: /* YI SYLLABLE DDUT */
    case 0xA14C: /* YI SYLLABLE DDUX */
    case 0xA14D: /* YI SYLLABLE DDU */
    case 0xA14E: /* YI SYLLABLE DDUP */
    case 0xA14F: /* YI SYLLABLE DDURX */
    case 0xA150: /* YI SYLLABLE DDUR */
    case 0xA151: /* YI SYLLABLE NDIT */
    case 0xA152: /* YI SYLLABLE NDIX */
    case 0xA153: /* YI SYLLABLE NDI */
    case 0xA154: /* YI SYLLABLE NDIP */
    case 0xA155: /* YI SYLLABLE NDIEX */
    case 0xA156: /* YI SYLLABLE NDIE */
    case 0xA157: /* YI SYLLABLE NDAT */
    case 0xA158: /* YI SYLLABLE NDAX */
    case 0xA159: /* YI SYLLABLE NDA */
    case 0xA15A: /* YI SYLLABLE NDAP */
    case 0xA15B: /* YI SYLLABLE NDOT */
    case 0xA15C: /* YI SYLLABLE NDOX */
    case 0xA15D: /* YI SYLLABLE NDO */
    case 0xA15E: /* YI SYLLABLE NDOP */
    case 0xA15F: /* YI SYLLABLE NDEX */
    case 0xA160: /* YI SYLLABLE NDE */
    case 0xA161: /* YI SYLLABLE NDEP */
    case 0xA162: /* YI SYLLABLE NDUT */
    case 0xA163: /* YI SYLLABLE NDUX */
    case 0xA164: /* YI SYLLABLE NDU */
    case 0xA165: /* YI SYLLABLE NDUP */
    case 0xA166: /* YI SYLLABLE NDURX */
    case 0xA167: /* YI SYLLABLE NDUR */
    case 0xA168: /* YI SYLLABLE HNIT */
    case 0xA169: /* YI SYLLABLE HNIX */
    case 0xA16A: /* YI SYLLABLE HNI */
    case 0xA16B: /* YI SYLLABLE HNIP */
    case 0xA16C: /* YI SYLLABLE HNIET */
    case 0xA16D: /* YI SYLLABLE HNIEX */
    case 0xA16E: /* YI SYLLABLE HNIE */
    case 0xA16F: /* YI SYLLABLE HNIEP */
    case 0xA170: /* YI SYLLABLE HNAT */
    case 0xA171: /* YI SYLLABLE HNAX */
    case 0xA172: /* YI SYLLABLE HNA */
    case 0xA173: /* YI SYLLABLE HNAP */
    case 0xA174: /* YI SYLLABLE HNUOX */
    case 0xA175: /* YI SYLLABLE HNUO */
    case 0xA176: /* YI SYLLABLE HNOT */
    case 0xA177: /* YI SYLLABLE HNOX */
    case 0xA178: /* YI SYLLABLE HNOP */
    case 0xA179: /* YI SYLLABLE HNEX */
    case 0xA17A: /* YI SYLLABLE HNE */
    case 0xA17B: /* YI SYLLABLE HNEP */
    case 0xA17C: /* YI SYLLABLE HNUT */
    case 0xA17D: /* YI SYLLABLE NIT */
    case 0xA17E: /* YI SYLLABLE NIX */
    case 0xA17F: /* YI SYLLABLE NI */
    case 0xA180: /* YI SYLLABLE NIP */
    case 0xA181: /* YI SYLLABLE NIEX */
    case 0xA182: /* YI SYLLABLE NIE */
    case 0xA183: /* YI SYLLABLE NIEP */
    case 0xA184: /* YI SYLLABLE NAX */
    case 0xA185: /* YI SYLLABLE NA */
    case 0xA186: /* YI SYLLABLE NAP */
    case 0xA187: /* YI SYLLABLE NUOX */
    case 0xA188: /* YI SYLLABLE NUO */
    case 0xA189: /* YI SYLLABLE NUOP */
    case 0xA18A: /* YI SYLLABLE NOT */
    case 0xA18B: /* YI SYLLABLE NOX */
    case 0xA18C: /* YI SYLLABLE NO */
    case 0xA18D: /* YI SYLLABLE NOP */
    case 0xA18E: /* YI SYLLABLE NEX */
    case 0xA18F: /* YI SYLLABLE NE */
    case 0xA190: /* YI SYLLABLE NEP */
    case 0xA191: /* YI SYLLABLE NUT */
    case 0xA192: /* YI SYLLABLE NUX */
    case 0xA193: /* YI SYLLABLE NU */
    case 0xA194: /* YI SYLLABLE NUP */
    case 0xA195: /* YI SYLLABLE NURX */
    case 0xA196: /* YI SYLLABLE NUR */
    case 0xA197: /* YI SYLLABLE HLIT */
    case 0xA198: /* YI SYLLABLE HLIX */
    case 0xA199: /* YI SYLLABLE HLI */
    case 0xA19A: /* YI SYLLABLE HLIP */
    case 0xA19B: /* YI SYLLABLE HLIEX */
    case 0xA19C: /* YI SYLLABLE HLIE */
    case 0xA19D: /* YI SYLLABLE HLIEP */
    case 0xA19E: /* YI SYLLABLE HLAT */
    case 0xA19F: /* YI SYLLABLE HLAX */
    case 0xA1A0: /* YI SYLLABLE HLA */
    case 0xA1A1: /* YI SYLLABLE HLAP */
    case 0xA1A2: /* YI SYLLABLE HLUOX */
    case 0xA1A3: /* YI SYLLABLE HLUO */
    case 0xA1A4: /* YI SYLLABLE HLUOP */
    case 0xA1A5: /* YI SYLLABLE HLOX */
    case 0xA1A6: /* YI SYLLABLE HLO */
    case 0xA1A7: /* YI SYLLABLE HLOP */
    case 0xA1A8: /* YI SYLLABLE HLEX */
    case 0xA1A9: /* YI SYLLABLE HLE */
    case 0xA1AA: /* YI SYLLABLE HLEP */
    case 0xA1AB: /* YI SYLLABLE HLUT */
    case 0xA1AC: /* YI SYLLABLE HLUX */
    case 0xA1AD: /* YI SYLLABLE HLU */
    case 0xA1AE: /* YI SYLLABLE HLUP */
    case 0xA1AF: /* YI SYLLABLE HLURX */
    case 0xA1B0: /* YI SYLLABLE HLUR */
    case 0xA1B1: /* YI SYLLABLE HLYT */
    case 0xA1B2: /* YI SYLLABLE HLYX */
    case 0xA1B3: /* YI SYLLABLE HLY */
    case 0xA1B4: /* YI SYLLABLE HLYP */
    case 0xA1B5: /* YI SYLLABLE HLYRX */
    case 0xA1B6: /* YI SYLLABLE HLYR */
    case 0xA1B7: /* YI SYLLABLE LIT */
    case 0xA1B8: /* YI SYLLABLE LIX */
    case 0xA1B9: /* YI SYLLABLE LI */
    case 0xA1BA: /* YI SYLLABLE LIP */
    case 0xA1BB: /* YI SYLLABLE LIET */
    case 0xA1BC: /* YI SYLLABLE LIEX */
    case 0xA1BD: /* YI SYLLABLE LIE */
    case 0xA1BE: /* YI SYLLABLE LIEP */
    case 0xA1BF: /* YI SYLLABLE LAT */
    case 0xA1C0: /* YI SYLLABLE LAX */
    case 0xA1C1: /* YI SYLLABLE LA */
    case 0xA1C2: /* YI SYLLABLE LAP */
    case 0xA1C3: /* YI SYLLABLE LUOT */
    case 0xA1C4: /* YI SYLLABLE LUOX */
    case 0xA1C5: /* YI SYLLABLE LUO */
    case 0xA1C6: /* YI SYLLABLE LUOP */
    case 0xA1C7: /* YI SYLLABLE LOT */
    case 0xA1C8: /* YI SYLLABLE LOX */
    case 0xA1C9: /* YI SYLLABLE LO */
    case 0xA1CA: /* YI SYLLABLE LOP */
    case 0xA1CB: /* YI SYLLABLE LEX */
    case 0xA1CC: /* YI SYLLABLE LE */
    case 0xA1CD: /* YI SYLLABLE LEP */
    case 0xA1CE: /* YI SYLLABLE LUT */
    case 0xA1CF: /* YI SYLLABLE LUX */
    case 0xA1D0: /* YI SYLLABLE LU */
    case 0xA1D1: /* YI SYLLABLE LUP */
    case 0xA1D2: /* YI SYLLABLE LURX */
    case 0xA1D3: /* YI SYLLABLE LUR */
    case 0xA1D4: /* YI SYLLABLE LYT */
    case 0xA1D5: /* YI SYLLABLE LYX */
    case 0xA1D6: /* YI SYLLABLE LY */
    case 0xA1D7: /* YI SYLLABLE LYP */
    case 0xA1D8: /* YI SYLLABLE LYRX */
    case 0xA1D9: /* YI SYLLABLE LYR */
    case 0xA1DA: /* YI SYLLABLE GIT */
    case 0xA1DB: /* YI SYLLABLE GIX */
    case 0xA1DC: /* YI SYLLABLE GI */
    case 0xA1DD: /* YI SYLLABLE GIP */
    case 0xA1DE: /* YI SYLLABLE GIET */
    case 0xA1DF: /* YI SYLLABLE GIEX */
    case 0xA1E0: /* YI SYLLABLE GIE */
    case 0xA1E1: /* YI SYLLABLE GIEP */
    case 0xA1E2: /* YI SYLLABLE GAT */
    case 0xA1E3: /* YI SYLLABLE GAX */
    case 0xA1E4: /* YI SYLLABLE GA */
    case 0xA1E5: /* YI SYLLABLE GAP */
    case 0xA1E6: /* YI SYLLABLE GUOT */
    case 0xA1E7: /* YI SYLLABLE GUOX */
    case 0xA1E8: /* YI SYLLABLE GUO */
    case 0xA1E9: /* YI SYLLABLE GUOP */
    case 0xA1EA: /* YI SYLLABLE GOT */
    case 0xA1EB: /* YI SYLLABLE GOX */
    case 0xA1EC: /* YI SYLLABLE GO */
    case 0xA1ED: /* YI SYLLABLE GOP */
    case 0xA1EE: /* YI SYLLABLE GET */
    case 0xA1EF: /* YI SYLLABLE GEX */
    case 0xA1F0: /* YI SYLLABLE GE */
    case 0xA1F1: /* YI SYLLABLE GEP */
    case 0xA1F2: /* YI SYLLABLE GUT */
    case 0xA1F3: /* YI SYLLABLE GUX */
    case 0xA1F4: /* YI SYLLABLE GU */
    case 0xA1F5: /* YI SYLLABLE GUP */
    case 0xA1F6: /* YI SYLLABLE GURX */
    case 0xA1F7: /* YI SYLLABLE GUR */
    case 0xA1F8: /* YI SYLLABLE KIT */
    case 0xA1F9: /* YI SYLLABLE KIX */
    case 0xA1FA: /* YI SYLLABLE KI */
    case 0xA1FB: /* YI SYLLABLE KIP */
    case 0xA1FC: /* YI SYLLABLE KIEX */
    case 0xA1FD: /* YI SYLLABLE KIE */
    case 0xA1FE: /* YI SYLLABLE KIEP */
    case 0xA1FF: /* YI SYLLABLE KAT */
    case 0xA200: /* YI SYLLABLE KAX */
    case 0xA201: /* YI SYLLABLE KA */
    case 0xA202: /* YI SYLLABLE KAP */
    case 0xA203: /* YI SYLLABLE KUOX */
    case 0xA204: /* YI SYLLABLE KUO */
    case 0xA205: /* YI SYLLABLE KUOP */
    case 0xA206: /* YI SYLLABLE KOT */
    case 0xA207: /* YI SYLLABLE KOX */
    case 0xA208: /* YI SYLLABLE KO */
    case 0xA209: /* YI SYLLABLE KOP */
    case 0xA20A: /* YI SYLLABLE KET */
    case 0xA20B: /* YI SYLLABLE KEX */
    case 0xA20C: /* YI SYLLABLE KE */
    case 0xA20D: /* YI SYLLABLE KEP */
    case 0xA20E: /* YI SYLLABLE KUT */
    case 0xA20F: /* YI SYLLABLE KUX */
    case 0xA210: /* YI SYLLABLE KU */
    case 0xA211: /* YI SYLLABLE KUP */
    case 0xA212: /* YI SYLLABLE KURX */
    case 0xA213: /* YI SYLLABLE KUR */
    case 0xA214: /* YI SYLLABLE GGIT */
    case 0xA215: /* YI SYLLABLE GGIX */
    case 0xA216: /* YI SYLLABLE GGI */
    case 0xA217: /* YI SYLLABLE GGIEX */
    case 0xA218: /* YI SYLLABLE GGIE */
    case 0xA219: /* YI SYLLABLE GGIEP */
    case 0xA21A: /* YI SYLLABLE GGAT */
    case 0xA21B: /* YI SYLLABLE GGAX */
    case 0xA21C: /* YI SYLLABLE GGA */
    case 0xA21D: /* YI SYLLABLE GGAP */
    case 0xA21E: /* YI SYLLABLE GGUOT */
    case 0xA21F: /* YI SYLLABLE GGUOX */
    case 0xA220: /* YI SYLLABLE GGUO */
    case 0xA221: /* YI SYLLABLE GGUOP */
    case 0xA222: /* YI SYLLABLE GGOT */
    case 0xA223: /* YI SYLLABLE GGOX */
    case 0xA224: /* YI SYLLABLE GGO */
    case 0xA225: /* YI SYLLABLE GGOP */
    case 0xA226: /* YI SYLLABLE GGET */
    case 0xA227: /* YI SYLLABLE GGEX */
    case 0xA228: /* YI SYLLABLE GGE */
    case 0xA229: /* YI SYLLABLE GGEP */
    case 0xA22A: /* YI SYLLABLE GGUT */
    case 0xA22B: /* YI SYLLABLE GGUX */
    case 0xA22C: /* YI SYLLABLE GGU */
    case 0xA22D: /* YI SYLLABLE GGUP */
    case 0xA22E: /* YI SYLLABLE GGURX */
    case 0xA22F: /* YI SYLLABLE GGUR */
    case 0xA230: /* YI SYLLABLE MGIEX */
    case 0xA231: /* YI SYLLABLE MGIE */
    case 0xA232: /* YI SYLLABLE MGAT */
    case 0xA233: /* YI SYLLABLE MGAX */
    case 0xA234: /* YI SYLLABLE MGA */
    case 0xA235: /* YI SYLLABLE MGAP */
    case 0xA236: /* YI SYLLABLE MGUOX */
    case 0xA237: /* YI SYLLABLE MGUO */
    case 0xA238: /* YI SYLLABLE MGUOP */
    case 0xA239: /* YI SYLLABLE MGOT */
    case 0xA23A: /* YI SYLLABLE MGOX */
    case 0xA23B: /* YI SYLLABLE MGO */
    case 0xA23C: /* YI SYLLABLE MGOP */
    case 0xA23D: /* YI SYLLABLE MGEX */
    case 0xA23E: /* YI SYLLABLE MGE */
    case 0xA23F: /* YI SYLLABLE MGEP */
    case 0xA240: /* YI SYLLABLE MGUT */
    case 0xA241: /* YI SYLLABLE MGUX */
    case 0xA242: /* YI SYLLABLE MGU */
    case 0xA243: /* YI SYLLABLE MGUP */
    case 0xA244: /* YI SYLLABLE MGURX */
    case 0xA245: /* YI SYLLABLE MGUR */
    case 0xA246: /* YI SYLLABLE HXIT */
    case 0xA247: /* YI SYLLABLE HXIX */
    case 0xA248: /* YI SYLLABLE HXI */
    case 0xA249: /* YI SYLLABLE HXIP */
    case 0xA24A: /* YI SYLLABLE HXIET */
    case 0xA24B: /* YI SYLLABLE HXIEX */
    case 0xA24C: /* YI SYLLABLE HXIE */
    case 0xA24D: /* YI SYLLABLE HXIEP */
    case 0xA24E: /* YI SYLLABLE HXAT */
    case 0xA24F: /* YI SYLLABLE HXAX */
    case 0xA250: /* YI SYLLABLE HXA */
    case 0xA251: /* YI SYLLABLE HXAP */
    case 0xA252: /* YI SYLLABLE HXUOT */
    case 0xA253: /* YI SYLLABLE HXUOX */
    case 0xA254: /* YI SYLLABLE HXUO */
    case 0xA255: /* YI SYLLABLE HXUOP */
    case 0xA256: /* YI SYLLABLE HXOT */
    case 0xA257: /* YI SYLLABLE HXOX */
    case 0xA258: /* YI SYLLABLE HXO */
    case 0xA259: /* YI SYLLABLE HXOP */
    case 0xA25A: /* YI SYLLABLE HXEX */
    case 0xA25B: /* YI SYLLABLE HXE */
    case 0xA25C: /* YI SYLLABLE HXEP */
    case 0xA25D: /* YI SYLLABLE NGIEX */
    case 0xA25E: /* YI SYLLABLE NGIE */
    case 0xA25F: /* YI SYLLABLE NGIEP */
    case 0xA260: /* YI SYLLABLE NGAT */
    case 0xA261: /* YI SYLLABLE NGAX */
    case 0xA262: /* YI SYLLABLE NGA */
    case 0xA263: /* YI SYLLABLE NGAP */
    case 0xA264: /* YI SYLLABLE NGUOT */
    case 0xA265: /* YI SYLLABLE NGUOX */
    case 0xA266: /* YI SYLLABLE NGUO */
    case 0xA267: /* YI SYLLABLE NGOT */
    case 0xA268: /* YI SYLLABLE NGOX */
    case 0xA269: /* YI SYLLABLE NGO */
    case 0xA26A: /* YI SYLLABLE NGOP */
    case 0xA26B: /* YI SYLLABLE NGEX */
    case 0xA26C: /* YI SYLLABLE NGE */
    case 0xA26D: /* YI SYLLABLE NGEP */
    case 0xA26E: /* YI SYLLABLE HIT */
    case 0xA26F: /* YI SYLLABLE HIEX */
    case 0xA270: /* YI SYLLABLE HIE */
    case 0xA271: /* YI SYLLABLE HAT */
    case 0xA272: /* YI SYLLABLE HAX */
    case 0xA273: /* YI SYLLABLE HA */
    case 0xA274: /* YI SYLLABLE HAP */
    case 0xA275: /* YI SYLLABLE HUOT */
    case 0xA276: /* YI SYLLABLE HUOX */
    case 0xA277: /* YI SYLLABLE HUO */
    case 0xA278: /* YI SYLLABLE HUOP */
    case 0xA279: /* YI SYLLABLE HOT */
    case 0xA27A: /* YI SYLLABLE HOX */
    case 0xA27B: /* YI SYLLABLE HO */
    case 0xA27C: /* YI SYLLABLE HOP */
    case 0xA27D: /* YI SYLLABLE HEX */
    case 0xA27E: /* YI SYLLABLE HE */
    case 0xA27F: /* YI SYLLABLE HEP */
    case 0xA280: /* YI SYLLABLE WAT */
    case 0xA281: /* YI SYLLABLE WAX */
    case 0xA282: /* YI SYLLABLE WA */
    case 0xA283: /* YI SYLLABLE WAP */
    case 0xA284: /* YI SYLLABLE WUOX */
    case 0xA285: /* YI SYLLABLE WUO */
    case 0xA286: /* YI SYLLABLE WUOP */
    case 0xA287: /* YI SYLLABLE WOX */
    case 0xA288: /* YI SYLLABLE WO */
    case 0xA289: /* YI SYLLABLE WOP */
    case 0xA28A: /* YI SYLLABLE WEX */
    case 0xA28B: /* YI SYLLABLE WE */
    case 0xA28C: /* YI SYLLABLE WEP */
    case 0xA28D: /* YI SYLLABLE ZIT */
    case 0xA28E: /* YI SYLLABLE ZIX */
    case 0xA28F: /* YI SYLLABLE ZI */
    case 0xA290: /* YI SYLLABLE ZIP */
    case 0xA291: /* YI SYLLABLE ZIEX */
    case 0xA292: /* YI SYLLABLE ZIE */
    case 0xA293: /* YI SYLLABLE ZIEP */
    case 0xA294: /* YI SYLLABLE ZAT */
    case 0xA295: /* YI SYLLABLE ZAX */
    case 0xA296: /* YI SYLLABLE ZA */
    case 0xA297: /* YI SYLLABLE ZAP */
    case 0xA298: /* YI SYLLABLE ZUOX */
    case 0xA299: /* YI SYLLABLE ZUO */
    case 0xA29A: /* YI SYLLABLE ZUOP */
    case 0xA29B: /* YI SYLLABLE ZOT */
    case 0xA29C: /* YI SYLLABLE ZOX */
    case 0xA29D: /* YI SYLLABLE ZO */
    case 0xA29E: /* YI SYLLABLE ZOP */
    case 0xA29F: /* YI SYLLABLE ZEX */
    case 0xA2A0: /* YI SYLLABLE ZE */
    case 0xA2A1: /* YI SYLLABLE ZEP */
    case 0xA2A2: /* YI SYLLABLE ZUT */
    case 0xA2A3: /* YI SYLLABLE ZUX */
    case 0xA2A4: /* YI SYLLABLE ZU */
    case 0xA2A5: /* YI SYLLABLE ZUP */
    case 0xA2A6: /* YI SYLLABLE ZURX */
    case 0xA2A7: /* YI SYLLABLE ZUR */
    case 0xA2A8: /* YI SYLLABLE ZYT */
    case 0xA2A9: /* YI SYLLABLE ZYX */
    case 0xA2AA: /* YI SYLLABLE ZY */
    case 0xA2AB: /* YI SYLLABLE ZYP */
    case 0xA2AC: /* YI SYLLABLE ZYRX */
    case 0xA2AD: /* YI SYLLABLE ZYR */
    case 0xA2AE: /* YI SYLLABLE CIT */
    case 0xA2AF: /* YI SYLLABLE CIX */
    case 0xA2B0: /* YI SYLLABLE CI */
    case 0xA2B1: /* YI SYLLABLE CIP */
    case 0xA2B2: /* YI SYLLABLE CIET */
    case 0xA2B3: /* YI SYLLABLE CIEX */
    case 0xA2B4: /* YI SYLLABLE CIE */
    case 0xA2B5: /* YI SYLLABLE CIEP */
    case 0xA2B6: /* YI SYLLABLE CAT */
    case 0xA2B7: /* YI SYLLABLE CAX */
    case 0xA2B8: /* YI SYLLABLE CA */
    case 0xA2B9: /* YI SYLLABLE CAP */
    case 0xA2BA: /* YI SYLLABLE CUOX */
    case 0xA2BB: /* YI SYLLABLE CUO */
    case 0xA2BC: /* YI SYLLABLE CUOP */
    case 0xA2BD: /* YI SYLLABLE COT */
    case 0xA2BE: /* YI SYLLABLE COX */
    case 0xA2BF: /* YI SYLLABLE CO */
    case 0xA2C0: /* YI SYLLABLE COP */
    case 0xA2C1: /* YI SYLLABLE CEX */
    case 0xA2C2: /* YI SYLLABLE CE */
    case 0xA2C3: /* YI SYLLABLE CEP */
    case 0xA2C4: /* YI SYLLABLE CUT */
    case 0xA2C5: /* YI SYLLABLE CUX */
    case 0xA2C6: /* YI SYLLABLE CU */
    case 0xA2C7: /* YI SYLLABLE CUP */
    case 0xA2C8: /* YI SYLLABLE CURX */
    case 0xA2C9: /* YI SYLLABLE CUR */
    case 0xA2CA: /* YI SYLLABLE CYT */
    case 0xA2CB: /* YI SYLLABLE CYX */
    case 0xA2CC: /* YI SYLLABLE CY */
    case 0xA2CD: /* YI SYLLABLE CYP */
    case 0xA2CE: /* YI SYLLABLE CYRX */
    case 0xA2CF: /* YI SYLLABLE CYR */
    case 0xA2D0: /* YI SYLLABLE ZZIT */
    case 0xA2D1: /* YI SYLLABLE ZZIX */
    case 0xA2D2: /* YI SYLLABLE ZZI */
    case 0xA2D3: /* YI SYLLABLE ZZIP */
    case 0xA2D4: /* YI SYLLABLE ZZIET */
    case 0xA2D5: /* YI SYLLABLE ZZIEX */
    case 0xA2D6: /* YI SYLLABLE ZZIE */
    case 0xA2D7: /* YI SYLLABLE ZZIEP */
    case 0xA2D8: /* YI SYLLABLE ZZAT */
    case 0xA2D9: /* YI SYLLABLE ZZAX */
    case 0xA2DA: /* YI SYLLABLE ZZA */
    case 0xA2DB: /* YI SYLLABLE ZZAP */
    case 0xA2DC: /* YI SYLLABLE ZZOX */
    case 0xA2DD: /* YI SYLLABLE ZZO */
    case 0xA2DE: /* YI SYLLABLE ZZOP */
    case 0xA2DF: /* YI SYLLABLE ZZEX */
    case 0xA2E0: /* YI SYLLABLE ZZE */
    case 0xA2E1: /* YI SYLLABLE ZZEP */
    case 0xA2E2: /* YI SYLLABLE ZZUX */
    case 0xA2E3: /* YI SYLLABLE ZZU */
    case 0xA2E4: /* YI SYLLABLE ZZUP */
    case 0xA2E5: /* YI SYLLABLE ZZURX */
    case 0xA2E6: /* YI SYLLABLE ZZUR */
    case 0xA2E7: /* YI SYLLABLE ZZYT */
    case 0xA2E8: /* YI SYLLABLE ZZYX */
    case 0xA2E9: /* YI SYLLABLE ZZY */
    case 0xA2EA: /* YI SYLLABLE ZZYP */
    case 0xA2EB: /* YI SYLLABLE ZZYRX */
    case 0xA2EC: /* YI SYLLABLE ZZYR */
    case 0xA2ED: /* YI SYLLABLE NZIT */
    case 0xA2EE: /* YI SYLLABLE NZIX */
    case 0xA2EF: /* YI SYLLABLE NZI */
    case 0xA2F0: /* YI SYLLABLE NZIP */
    case 0xA2F1: /* YI SYLLABLE NZIEX */
    case 0xA2F2: /* YI SYLLABLE NZIE */
    case 0xA2F3: /* YI SYLLABLE NZIEP */
    case 0xA2F4: /* YI SYLLABLE NZAT */
    case 0xA2F5: /* YI SYLLABLE NZAX */
    case 0xA2F6: /* YI SYLLABLE NZA */
    case 0xA2F7: /* YI SYLLABLE NZAP */
    case 0xA2F8: /* YI SYLLABLE NZUOX */
    case 0xA2F9: /* YI SYLLABLE NZUO */
    case 0xA2FA: /* YI SYLLABLE NZOX */
    case 0xA2FB: /* YI SYLLABLE NZOP */
    case 0xA2FC: /* YI SYLLABLE NZEX */
    case 0xA2FD: /* YI SYLLABLE NZE */
    case 0xA2FE: /* YI SYLLABLE NZUX */
    case 0xA2FF: /* YI SYLLABLE NZU */
    case 0xA300: /* YI SYLLABLE NZUP */
    case 0xA301: /* YI SYLLABLE NZURX */
    case 0xA302: /* YI SYLLABLE NZUR */
    case 0xA303: /* YI SYLLABLE NZYT */
    case 0xA304: /* YI SYLLABLE NZYX */
    case 0xA305: /* YI SYLLABLE NZY */
    case 0xA306: /* YI SYLLABLE NZYP */
    case 0xA307: /* YI SYLLABLE NZYRX */
    case 0xA308: /* YI SYLLABLE NZYR */
    case 0xA309: /* YI SYLLABLE SIT */
    case 0xA30A: /* YI SYLLABLE SIX */
    case 0xA30B: /* YI SYLLABLE SI */
    case 0xA30C: /* YI SYLLABLE SIP */
    case 0xA30D: /* YI SYLLABLE SIEX */
    case 0xA30E: /* YI SYLLABLE SIE */
    case 0xA30F: /* YI SYLLABLE SIEP */
    case 0xA310: /* YI SYLLABLE SAT */
    case 0xA311: /* YI SYLLABLE SAX */
    case 0xA312: /* YI SYLLABLE SA */
    case 0xA313: /* YI SYLLABLE SAP */
    case 0xA314: /* YI SYLLABLE SUOX */
    case 0xA315: /* YI SYLLABLE SUO */
    case 0xA316: /* YI SYLLABLE SUOP */
    case 0xA317: /* YI SYLLABLE SOT */
    case 0xA318: /* YI SYLLABLE SOX */
    case 0xA319: /* YI SYLLABLE SO */
    case 0xA31A: /* YI SYLLABLE SOP */
    case 0xA31B: /* YI SYLLABLE SEX */
    case 0xA31C: /* YI SYLLABLE SE */
    case 0xA31D: /* YI SYLLABLE SEP */
    case 0xA31E: /* YI SYLLABLE SUT */
    case 0xA31F: /* YI SYLLABLE SUX */
    case 0xA320: /* YI SYLLABLE SU */
    case 0xA321: /* YI SYLLABLE SUP */
    case 0xA322: /* YI SYLLABLE SURX */
    case 0xA323: /* YI SYLLABLE SUR */
    case 0xA324: /* YI SYLLABLE SYT */
    case 0xA325: /* YI SYLLABLE SYX */
    case 0xA326: /* YI SYLLABLE SY */
    case 0xA327: /* YI SYLLABLE SYP */
    case 0xA328: /* YI SYLLABLE SYRX */
    case 0xA329: /* YI SYLLABLE SYR */
    case 0xA32A: /* YI SYLLABLE SSIT */
    case 0xA32B: /* YI SYLLABLE SSIX */
    case 0xA32C: /* YI SYLLABLE SSI */
    case 0xA32D: /* YI SYLLABLE SSIP */
    case 0xA32E: /* YI SYLLABLE SSIEX */
    case 0xA32F: /* YI SYLLABLE SSIE */
    case 0xA330: /* YI SYLLABLE SSIEP */
    case 0xA331: /* YI SYLLABLE SSAT */
    case 0xA332: /* YI SYLLABLE SSAX */
    case 0xA333: /* YI SYLLABLE SSA */
    case 0xA334: /* YI SYLLABLE SSAP */
    case 0xA335: /* YI SYLLABLE SSOT */
    case 0xA336: /* YI SYLLABLE SSOX */
    case 0xA337: /* YI SYLLABLE SSO */
    case 0xA338: /* YI SYLLABLE SSOP */
    case 0xA339: /* YI SYLLABLE SSEX */
    case 0xA33A: /* YI SYLLABLE SSE */
    case 0xA33B: /* YI SYLLABLE SSEP */
    case 0xA33C: /* YI SYLLABLE SSUT */
    case 0xA33D: /* YI SYLLABLE SSUX */
    case 0xA33E: /* YI SYLLABLE SSU */
    case 0xA33F: /* YI SYLLABLE SSUP */
    case 0xA340: /* YI SYLLABLE SSYT */
    case 0xA341: /* YI SYLLABLE SSYX */
    case 0xA342: /* YI SYLLABLE SSY */
    case 0xA343: /* YI SYLLABLE SSYP */
    case 0xA344: /* YI SYLLABLE SSYRX */
    case 0xA345: /* YI SYLLABLE SSYR */
    case 0xA346: /* YI SYLLABLE ZHAT */
    case 0xA347: /* YI SYLLABLE ZHAX */
    case 0xA348: /* YI SYLLABLE ZHA */
    case 0xA349: /* YI SYLLABLE ZHAP */
    case 0xA34A: /* YI SYLLABLE ZHUOX */
    case 0xA34B: /* YI SYLLABLE ZHUO */
    case 0xA34C: /* YI SYLLABLE ZHUOP */
    case 0xA34D: /* YI SYLLABLE ZHOT */
    case 0xA34E: /* YI SYLLABLE ZHOX */
    case 0xA34F: /* YI SYLLABLE ZHO */
    case 0xA350: /* YI SYLLABLE ZHOP */
    case 0xA351: /* YI SYLLABLE ZHET */
    case 0xA352: /* YI SYLLABLE ZHEX */
    case 0xA353: /* YI SYLLABLE ZHE */
    case 0xA354: /* YI SYLLABLE ZHEP */
    case 0xA355: /* YI SYLLABLE ZHUT */
    case 0xA356: /* YI SYLLABLE ZHUX */
    case 0xA357: /* YI SYLLABLE ZHU */
    case 0xA358: /* YI SYLLABLE ZHUP */
    case 0xA359: /* YI SYLLABLE ZHURX */
    case 0xA35A: /* YI SYLLABLE ZHUR */
    case 0xA35B: /* YI SYLLABLE ZHYT */
    case 0xA35C: /* YI SYLLABLE ZHYX */
    case 0xA35D: /* YI SYLLABLE ZHY */
    case 0xA35E: /* YI SYLLABLE ZHYP */
    case 0xA35F: /* YI SYLLABLE ZHYRX */
    case 0xA360: /* YI SYLLABLE ZHYR */
    case 0xA361: /* YI SYLLABLE CHAT */
    case 0xA362: /* YI SYLLABLE CHAX */
    case 0xA363: /* YI SYLLABLE CHA */
    case 0xA364: /* YI SYLLABLE CHAP */
    case 0xA365: /* YI SYLLABLE CHUOT */
    case 0xA366: /* YI SYLLABLE CHUOX */
    case 0xA367: /* YI SYLLABLE CHUO */
    case 0xA368: /* YI SYLLABLE CHUOP */
    case 0xA369: /* YI SYLLABLE CHOT */
    case 0xA36A: /* YI SYLLABLE CHOX */
    case 0xA36B: /* YI SYLLABLE CHO */
    case 0xA36C: /* YI SYLLABLE CHOP */
    case 0xA36D: /* YI SYLLABLE CHET */
    case 0xA36E: /* YI SYLLABLE CHEX */
    case 0xA36F: /* YI SYLLABLE CHE */
    case 0xA370: /* YI SYLLABLE CHEP */
    case 0xA371: /* YI SYLLABLE CHUX */
    case 0xA372: /* YI SYLLABLE CHU */
    case 0xA373: /* YI SYLLABLE CHUP */
    case 0xA374: /* YI SYLLABLE CHURX */
    case 0xA375: /* YI SYLLABLE CHUR */
    case 0xA376: /* YI SYLLABLE CHYT */
    case 0xA377: /* YI SYLLABLE CHYX */
    case 0xA378: /* YI SYLLABLE CHY */
    case 0xA379: /* YI SYLLABLE CHYP */
    case 0xA37A: /* YI SYLLABLE CHYRX */
    case 0xA37B: /* YI SYLLABLE CHYR */
    case 0xA37C: /* YI SYLLABLE RRAX */
    case 0xA37D: /* YI SYLLABLE RRA */
    case 0xA37E: /* YI SYLLABLE RRUOX */
    case 0xA37F: /* YI SYLLABLE RRUO */
    case 0xA380: /* YI SYLLABLE RROT */
    case 0xA381: /* YI SYLLABLE RROX */
    case 0xA382: /* YI SYLLABLE RRO */
    case 0xA383: /* YI SYLLABLE RROP */
    case 0xA384: /* YI SYLLABLE RRET */
    case 0xA385: /* YI SYLLABLE RREX */
    case 0xA386: /* YI SYLLABLE RRE */
    case 0xA387: /* YI SYLLABLE RREP */
    case 0xA388: /* YI SYLLABLE RRUT */
    case 0xA389: /* YI SYLLABLE RRUX */
    case 0xA38A: /* YI SYLLABLE RRU */
    case 0xA38B: /* YI SYLLABLE RRUP */
    case 0xA38C: /* YI SYLLABLE RRURX */
    case 0xA38D: /* YI SYLLABLE RRUR */
    case 0xA38E: /* YI SYLLABLE RRYT */
    case 0xA38F: /* YI SYLLABLE RRYX */
    case 0xA390: /* YI SYLLABLE RRY */
    case 0xA391: /* YI SYLLABLE RRYP */
    case 0xA392: /* YI SYLLABLE RRYRX */
    case 0xA393: /* YI SYLLABLE RRYR */
    case 0xA394: /* YI SYLLABLE NRAT */
    case 0xA395: /* YI SYLLABLE NRAX */
    case 0xA396: /* YI SYLLABLE NRA */
    case 0xA397: /* YI SYLLABLE NRAP */
    case 0xA398: /* YI SYLLABLE NROX */
    case 0xA399: /* YI SYLLABLE NRO */
    case 0xA39A: /* YI SYLLABLE NROP */
    case 0xA39B: /* YI SYLLABLE NRET */
    case 0xA39C: /* YI SYLLABLE NREX */
    case 0xA39D: /* YI SYLLABLE NRE */
    case 0xA39E: /* YI SYLLABLE NREP */
    case 0xA39F: /* YI SYLLABLE NRUT */
    case 0xA3A0: /* YI SYLLABLE NRUX */
    case 0xA3A1: /* YI SYLLABLE NRU */
    case 0xA3A2: /* YI SYLLABLE NRUP */
    case 0xA3A3: /* YI SYLLABLE NRURX */
    case 0xA3A4: /* YI SYLLABLE NRUR */
    case 0xA3A5: /* YI SYLLABLE NRYT */
    case 0xA3A6: /* YI SYLLABLE NRYX */
    case 0xA3A7: /* YI SYLLABLE NRY */
    case 0xA3A8: /* YI SYLLABLE NRYP */
    case 0xA3A9: /* YI SYLLABLE NRYRX */
    case 0xA3AA: /* YI SYLLABLE NRYR */
    case 0xA3AB: /* YI SYLLABLE SHAT */
    case 0xA3AC: /* YI SYLLABLE SHAX */
    case 0xA3AD: /* YI SYLLABLE SHA */
    case 0xA3AE: /* YI SYLLABLE SHAP */
    case 0xA3AF: /* YI SYLLABLE SHUOX */
    case 0xA3B0: /* YI SYLLABLE SHUO */
    case 0xA3B1: /* YI SYLLABLE SHUOP */
    case 0xA3B2: /* YI SYLLABLE SHOT */
    case 0xA3B3: /* YI SYLLABLE SHOX */
    case 0xA3B4: /* YI SYLLABLE SHO */
    case 0xA3B5: /* YI SYLLABLE SHOP */
    case 0xA3B6: /* YI SYLLABLE SHET */
    case 0xA3B7: /* YI SYLLABLE SHEX */
    case 0xA3B8: /* YI SYLLABLE SHE */
    case 0xA3B9: /* YI SYLLABLE SHEP */
    case 0xA3BA: /* YI SYLLABLE SHUT */
    case 0xA3BB: /* YI SYLLABLE SHUX */
    case 0xA3BC: /* YI SYLLABLE SHU */
    case 0xA3BD: /* YI SYLLABLE SHUP */
    case 0xA3BE: /* YI SYLLABLE SHURX */
    case 0xA3BF: /* YI SYLLABLE SHUR */
    case 0xA3C0: /* YI SYLLABLE SHYT */
    case 0xA3C1: /* YI SYLLABLE SHYX */
    case 0xA3C2: /* YI SYLLABLE SHY */
    case 0xA3C3: /* YI SYLLABLE SHYP */
    case 0xA3C4: /* YI SYLLABLE SHYRX */
    case 0xA3C5: /* YI SYLLABLE SHYR */
    case 0xA3C6: /* YI SYLLABLE RAT */
    case 0xA3C7: /* YI SYLLABLE RAX */
    case 0xA3C8: /* YI SYLLABLE RA */
    case 0xA3C9: /* YI SYLLABLE RAP */
    case 0xA3CA: /* YI SYLLABLE RUOX */
    case 0xA3CB: /* YI SYLLABLE RUO */
    case 0xA3CC: /* YI SYLLABLE RUOP */
    case 0xA3CD: /* YI SYLLABLE ROT */
    case 0xA3CE: /* YI SYLLABLE ROX */
    case 0xA3CF: /* YI SYLLABLE RO */
    case 0xA3D0: /* YI SYLLABLE ROP */
    case 0xA3D1: /* YI SYLLABLE REX */
    case 0xA3D2: /* YI SYLLABLE RE */
    case 0xA3D3: /* YI SYLLABLE REP */
    case 0xA3D4: /* YI SYLLABLE RUT */
    case 0xA3D5: /* YI SYLLABLE RUX */
    case 0xA3D6: /* YI SYLLABLE RU */
    case 0xA3D7: /* YI SYLLABLE RUP */
    case 0xA3D8: /* YI SYLLABLE RURX */
    case 0xA3D9: /* YI SYLLABLE RUR */
    case 0xA3DA: /* YI SYLLABLE RYT */
    case 0xA3DB: /* YI SYLLABLE RYX */
    case 0xA3DC: /* YI SYLLABLE RY */
    case 0xA3DD: /* YI SYLLABLE RYP */
    case 0xA3DE: /* YI SYLLABLE RYRX */
    case 0xA3DF: /* YI SYLLABLE RYR */
    case 0xA3E0: /* YI SYLLABLE JIT */
    case 0xA3E1: /* YI SYLLABLE JIX */
    case 0xA3E2: /* YI SYLLABLE JI */
    case 0xA3E3: /* YI SYLLABLE JIP */
    case 0xA3E4: /* YI SYLLABLE JIET */
    case 0xA3E5: /* YI SYLLABLE JIEX */
    case 0xA3E6: /* YI SYLLABLE JIE */
    case 0xA3E7: /* YI SYLLABLE JIEP */
    case 0xA3E8: /* YI SYLLABLE JUOT */
    case 0xA3E9: /* YI SYLLABLE JUOX */
    case 0xA3EA: /* YI SYLLABLE JUO */
    case 0xA3EB: /* YI SYLLABLE JUOP */
    case 0xA3EC: /* YI SYLLABLE JOT */
    case 0xA3ED: /* YI SYLLABLE JOX */
    case 0xA3EE: /* YI SYLLABLE JO */
    case 0xA3EF: /* YI SYLLABLE JOP */
    case 0xA3F0: /* YI SYLLABLE JUT */
    case 0xA3F1: /* YI SYLLABLE JUX */
    case 0xA3F2: /* YI SYLLABLE JU */
    case 0xA3F3: /* YI SYLLABLE JUP */
    case 0xA3F4: /* YI SYLLABLE JURX */
    case 0xA3F5: /* YI SYLLABLE JUR */
    case 0xA3F6: /* YI SYLLABLE JYT */
    case 0xA3F7: /* YI SYLLABLE JYX */
    case 0xA3F8: /* YI SYLLABLE JY */
    case 0xA3F9: /* YI SYLLABLE JYP */
    case 0xA3FA: /* YI SYLLABLE JYRX */
    case 0xA3FB: /* YI SYLLABLE JYR */
    case 0xA3FC: /* YI SYLLABLE QIT */
    case 0xA3FD: /* YI SYLLABLE QIX */
    case 0xA3FE: /* YI SYLLABLE QI */
    case 0xA3FF: /* YI SYLLABLE QIP */
    case 0xA400: /* YI SYLLABLE QIET */
    case 0xA401: /* YI SYLLABLE QIEX */
    case 0xA402: /* YI SYLLABLE QIE */
    case 0xA403: /* YI SYLLABLE QIEP */
    case 0xA404: /* YI SYLLABLE QUOT */
    case 0xA405: /* YI SYLLABLE QUOX */
    case 0xA406: /* YI SYLLABLE QUO */
    case 0xA407: /* YI SYLLABLE QUOP */
    case 0xA408: /* YI SYLLABLE QOT */
    case 0xA409: /* YI SYLLABLE QOX */
    case 0xA40A: /* YI SYLLABLE QO */
    case 0xA40B: /* YI SYLLABLE QOP */
    case 0xA40C: /* YI SYLLABLE QUT */
    case 0xA40D: /* YI SYLLABLE QUX */
    case 0xA40E: /* YI SYLLABLE QU */
    case 0xA40F: /* YI SYLLABLE QUP */
    case 0xA410: /* YI SYLLABLE QURX */
    case 0xA411: /* YI SYLLABLE QUR */
    case 0xA412: /* YI SYLLABLE QYT */
BREAK_SWITCH_UP
    case 0xA413: /* YI SYLLABLE QYX */
    case 0xA414: /* YI SYLLABLE QY */
    case 0xA415: /* YI SYLLABLE QYP */
    case 0xA416: /* YI SYLLABLE QYRX */
    case 0xA417: /* YI SYLLABLE QYR */
    case 0xA418: /* YI SYLLABLE JJIT */
    case 0xA419: /* YI SYLLABLE JJIX */
    case 0xA41A: /* YI SYLLABLE JJI */
    case 0xA41B: /* YI SYLLABLE JJIP */
    case 0xA41C: /* YI SYLLABLE JJIET */
    case 0xA41D: /* YI SYLLABLE JJIEX */
    case 0xA41E: /* YI SYLLABLE JJIE */
    case 0xA41F: /* YI SYLLABLE JJIEP */
    case 0xA420: /* YI SYLLABLE JJUOX */
    case 0xA421: /* YI SYLLABLE JJUO */
    case 0xA422: /* YI SYLLABLE JJUOP */
    case 0xA423: /* YI SYLLABLE JJOT */
    case 0xA424: /* YI SYLLABLE JJOX */
    case 0xA425: /* YI SYLLABLE JJO */
    case 0xA426: /* YI SYLLABLE JJOP */
    case 0xA427: /* YI SYLLABLE JJUT */
    case 0xA428: /* YI SYLLABLE JJUX */
    case 0xA429: /* YI SYLLABLE JJU */
    case 0xA42A: /* YI SYLLABLE JJUP */
    case 0xA42B: /* YI SYLLABLE JJURX */
    case 0xA42C: /* YI SYLLABLE JJUR */
    case 0xA42D: /* YI SYLLABLE JJYT */
    case 0xA42E: /* YI SYLLABLE JJYX */
    case 0xA42F: /* YI SYLLABLE JJY */
    case 0xA430: /* YI SYLLABLE JJYP */
    case 0xA431: /* YI SYLLABLE NJIT */
    case 0xA432: /* YI SYLLABLE NJIX */
    case 0xA433: /* YI SYLLABLE NJI */
    case 0xA434: /* YI SYLLABLE NJIP */
    case 0xA435: /* YI SYLLABLE NJIET */
    case 0xA436: /* YI SYLLABLE NJIEX */
    case 0xA437: /* YI SYLLABLE NJIE */
    case 0xA438: /* YI SYLLABLE NJIEP */
    case 0xA439: /* YI SYLLABLE NJUOX */
    case 0xA43A: /* YI SYLLABLE NJUO */
    case 0xA43B: /* YI SYLLABLE NJOT */
    case 0xA43C: /* YI SYLLABLE NJOX */
    case 0xA43D: /* YI SYLLABLE NJO */
    case 0xA43E: /* YI SYLLABLE NJOP */
    case 0xA43F: /* YI SYLLABLE NJUX */
    case 0xA440: /* YI SYLLABLE NJU */
    case 0xA441: /* YI SYLLABLE NJUP */
    case 0xA442: /* YI SYLLABLE NJURX */
    case 0xA443: /* YI SYLLABLE NJUR */
    case 0xA444: /* YI SYLLABLE NJYT */
    case 0xA445: /* YI SYLLABLE NJYX */
    case 0xA446: /* YI SYLLABLE NJY */
    case 0xA447: /* YI SYLLABLE NJYP */
    case 0xA448: /* YI SYLLABLE NJYRX */
    case 0xA449: /* YI SYLLABLE NJYR */
    case 0xA44A: /* YI SYLLABLE NYIT */
    case 0xA44B: /* YI SYLLABLE NYIX */
    case 0xA44C: /* YI SYLLABLE NYI */
    case 0xA44D: /* YI SYLLABLE NYIP */
    case 0xA44E: /* YI SYLLABLE NYIET */
    case 0xA44F: /* YI SYLLABLE NYIEX */
    case 0xA450: /* YI SYLLABLE NYIE */
    case 0xA451: /* YI SYLLABLE NYIEP */
    case 0xA452: /* YI SYLLABLE NYUOX */
    case 0xA453: /* YI SYLLABLE NYUO */
    case 0xA454: /* YI SYLLABLE NYUOP */
    case 0xA455: /* YI SYLLABLE NYOT */
    case 0xA456: /* YI SYLLABLE NYOX */
    case 0xA457: /* YI SYLLABLE NYO */
    case 0xA458: /* YI SYLLABLE NYOP */
    case 0xA459: /* YI SYLLABLE NYUT */
    case 0xA45A: /* YI SYLLABLE NYUX */
    case 0xA45B: /* YI SYLLABLE NYU */
    case 0xA45C: /* YI SYLLABLE NYUP */
    case 0xA45D: /* YI SYLLABLE XIT */
    case 0xA45E: /* YI SYLLABLE XIX */
    case 0xA45F: /* YI SYLLABLE XI */
    case 0xA460: /* YI SYLLABLE XIP */
    case 0xA461: /* YI SYLLABLE XIET */
    case 0xA462: /* YI SYLLABLE XIEX */
    case 0xA463: /* YI SYLLABLE XIE */
    case 0xA464: /* YI SYLLABLE XIEP */
    case 0xA465: /* YI SYLLABLE XUOX */
    case 0xA466: /* YI SYLLABLE XUO */
    case 0xA467: /* YI SYLLABLE XOT */
    case 0xA468: /* YI SYLLABLE XOX */
    case 0xA469: /* YI SYLLABLE XO */
    case 0xA46A: /* YI SYLLABLE XOP */
    case 0xA46B: /* YI SYLLABLE XYT */
    case 0xA46C: /* YI SYLLABLE XYX */
    case 0xA46D: /* YI SYLLABLE XY */
    case 0xA46E: /* YI SYLLABLE XYP */
    case 0xA46F: /* YI SYLLABLE XYRX */
    case 0xA470: /* YI SYLLABLE XYR */
    case 0xA471: /* YI SYLLABLE YIT */
    case 0xA472: /* YI SYLLABLE YIX */
    case 0xA473: /* YI SYLLABLE YI */
    case 0xA474: /* YI SYLLABLE YIP */
    case 0xA475: /* YI SYLLABLE YIET */
    case 0xA476: /* YI SYLLABLE YIEX */
    case 0xA477: /* YI SYLLABLE YIE */
    case 0xA478: /* YI SYLLABLE YIEP */
    case 0xA479: /* YI SYLLABLE YUOT */
    case 0xA47A: /* YI SYLLABLE YUOX */
    case 0xA47B: /* YI SYLLABLE YUO */
    case 0xA47C: /* YI SYLLABLE YUOP */
    case 0xA47D: /* YI SYLLABLE YOT */
    case 0xA47E: /* YI SYLLABLE YOX */
    case 0xA47F: /* YI SYLLABLE YO */
    case 0xA480: /* YI SYLLABLE YOP */
    case 0xA481: /* YI SYLLABLE YUT */
    case 0xA482: /* YI SYLLABLE YUX */
    case 0xA483: /* YI SYLLABLE YU */
    case 0xA484: /* YI SYLLABLE YUP */
    case 0xA485: /* YI SYLLABLE YURX */
    case 0xA486: /* YI SYLLABLE YUR */
    case 0xA487: /* YI SYLLABLE YYT */
    case 0xA488: /* YI SYLLABLE YYX */
    case 0xA489: /* YI SYLLABLE YY */
    case 0xA48A: /* YI SYLLABLE YYP */
    case 0xA48B: /* YI SYLLABLE YYRX */
    case 0xA48C: /* YI SYLLABLE YYR */
    case 0xAC00: /* <Hangul Syllable, First> */
    case 0xD7A3: /* <Hangul Syllable, Last> */
    case 0xF900: /* CJK COMPATIBILITY IDEOGRAPH-F900 */
    case 0xF901: /* CJK COMPATIBILITY IDEOGRAPH-F901 */
    case 0xF902: /* CJK COMPATIBILITY IDEOGRAPH-F902 */
    case 0xF903: /* CJK COMPATIBILITY IDEOGRAPH-F903 */
    case 0xF904: /* CJK COMPATIBILITY IDEOGRAPH-F904 */
    case 0xF905: /* CJK COMPATIBILITY IDEOGRAPH-F905 */
    case 0xF906: /* CJK COMPATIBILITY IDEOGRAPH-F906 */
    case 0xF907: /* CJK COMPATIBILITY IDEOGRAPH-F907 */
    case 0xF908: /* CJK COMPATIBILITY IDEOGRAPH-F908 */
    case 0xF909: /* CJK COMPATIBILITY IDEOGRAPH-F909 */
    case 0xF90A: /* CJK COMPATIBILITY IDEOGRAPH-F90A */
    case 0xF90B: /* CJK COMPATIBILITY IDEOGRAPH-F90B */
    case 0xF90C: /* CJK COMPATIBILITY IDEOGRAPH-F90C */
    case 0xF90D: /* CJK COMPATIBILITY IDEOGRAPH-F90D */
    case 0xF90E: /* CJK COMPATIBILITY IDEOGRAPH-F90E */
    case 0xF90F: /* CJK COMPATIBILITY IDEOGRAPH-F90F */
    case 0xF910: /* CJK COMPATIBILITY IDEOGRAPH-F910 */
    case 0xF911: /* CJK COMPATIBILITY IDEOGRAPH-F911 */
    case 0xF912: /* CJK COMPATIBILITY IDEOGRAPH-F912 */
    case 0xF913: /* CJK COMPATIBILITY IDEOGRAPH-F913 */
    case 0xF914: /* CJK COMPATIBILITY IDEOGRAPH-F914 */
    case 0xF915: /* CJK COMPATIBILITY IDEOGRAPH-F915 */
    case 0xF916: /* CJK COMPATIBILITY IDEOGRAPH-F916 */
    case 0xF917: /* CJK COMPATIBILITY IDEOGRAPH-F917 */
    case 0xF918: /* CJK COMPATIBILITY IDEOGRAPH-F918 */
    case 0xF919: /* CJK COMPATIBILITY IDEOGRAPH-F919 */
    case 0xF91A: /* CJK COMPATIBILITY IDEOGRAPH-F91A */
    case 0xF91B: /* CJK COMPATIBILITY IDEOGRAPH-F91B */
    case 0xF91C: /* CJK COMPATIBILITY IDEOGRAPH-F91C */
    case 0xF91D: /* CJK COMPATIBILITY IDEOGRAPH-F91D */
    case 0xF91E: /* CJK COMPATIBILITY IDEOGRAPH-F91E */
    case 0xF91F: /* CJK COMPATIBILITY IDEOGRAPH-F91F */
    case 0xF920: /* CJK COMPATIBILITY IDEOGRAPH-F920 */
    case 0xF921: /* CJK COMPATIBILITY IDEOGRAPH-F921 */
    case 0xF922: /* CJK COMPATIBILITY IDEOGRAPH-F922 */
    case 0xF923: /* CJK COMPATIBILITY IDEOGRAPH-F923 */
    case 0xF924: /* CJK COMPATIBILITY IDEOGRAPH-F924 */
    case 0xF925: /* CJK COMPATIBILITY IDEOGRAPH-F925 */
    case 0xF926: /* CJK COMPATIBILITY IDEOGRAPH-F926 */
    case 0xF927: /* CJK COMPATIBILITY IDEOGRAPH-F927 */
    case 0xF928: /* CJK COMPATIBILITY IDEOGRAPH-F928 */
    case 0xF929: /* CJK COMPATIBILITY IDEOGRAPH-F929 */
    case 0xF92A: /* CJK COMPATIBILITY IDEOGRAPH-F92A */
    case 0xF92B: /* CJK COMPATIBILITY IDEOGRAPH-F92B */
    case 0xF92C: /* CJK COMPATIBILITY IDEOGRAPH-F92C */
    case 0xF92D: /* CJK COMPATIBILITY IDEOGRAPH-F92D */
    case 0xF92E: /* CJK COMPATIBILITY IDEOGRAPH-F92E */
    case 0xF92F: /* CJK COMPATIBILITY IDEOGRAPH-F92F */
    case 0xF930: /* CJK COMPATIBILITY IDEOGRAPH-F930 */
    case 0xF931: /* CJK COMPATIBILITY IDEOGRAPH-F931 */
    case 0xF932: /* CJK COMPATIBILITY IDEOGRAPH-F932 */
    case 0xF933: /* CJK COMPATIBILITY IDEOGRAPH-F933 */
    case 0xF934: /* CJK COMPATIBILITY IDEOGRAPH-F934 */
    case 0xF935: /* CJK COMPATIBILITY IDEOGRAPH-F935 */
    case 0xF936: /* CJK COMPATIBILITY IDEOGRAPH-F936 */
    case 0xF937: /* CJK COMPATIBILITY IDEOGRAPH-F937 */
    case 0xF938: /* CJK COMPATIBILITY IDEOGRAPH-F938 */
    case 0xF939: /* CJK COMPATIBILITY IDEOGRAPH-F939 */
    case 0xF93A: /* CJK COMPATIBILITY IDEOGRAPH-F93A */
    case 0xF93B: /* CJK COMPATIBILITY IDEOGRAPH-F93B */
    case 0xF93C: /* CJK COMPATIBILITY IDEOGRAPH-F93C */
    case 0xF93D: /* CJK COMPATIBILITY IDEOGRAPH-F93D */
    case 0xF93E: /* CJK COMPATIBILITY IDEOGRAPH-F93E */
    case 0xF93F: /* CJK COMPATIBILITY IDEOGRAPH-F93F */
    case 0xF940: /* CJK COMPATIBILITY IDEOGRAPH-F940 */
    case 0xF941: /* CJK COMPATIBILITY IDEOGRAPH-F941 */
    case 0xF942: /* CJK COMPATIBILITY IDEOGRAPH-F942 */
    case 0xF943: /* CJK COMPATIBILITY IDEOGRAPH-F943 */
    case 0xF944: /* CJK COMPATIBILITY IDEOGRAPH-F944 */
    case 0xF945: /* CJK COMPATIBILITY IDEOGRAPH-F945 */
    case 0xF946: /* CJK COMPATIBILITY IDEOGRAPH-F946 */
    case 0xF947: /* CJK COMPATIBILITY IDEOGRAPH-F947 */
    case 0xF948: /* CJK COMPATIBILITY IDEOGRAPH-F948 */
    case 0xF949: /* CJK COMPATIBILITY IDEOGRAPH-F949 */
    case 0xF94A: /* CJK COMPATIBILITY IDEOGRAPH-F94A */
    case 0xF94B: /* CJK COMPATIBILITY IDEOGRAPH-F94B */
    case 0xF94C: /* CJK COMPATIBILITY IDEOGRAPH-F94C */
    case 0xF94D: /* CJK COMPATIBILITY IDEOGRAPH-F94D */
    case 0xF94E: /* CJK COMPATIBILITY IDEOGRAPH-F94E */
    case 0xF94F: /* CJK COMPATIBILITY IDEOGRAPH-F94F */
    case 0xF950: /* CJK COMPATIBILITY IDEOGRAPH-F950 */
    case 0xF951: /* CJK COMPATIBILITY IDEOGRAPH-F951 */
    case 0xF952: /* CJK COMPATIBILITY IDEOGRAPH-F952 */
    case 0xF953: /* CJK COMPATIBILITY IDEOGRAPH-F953 */
    case 0xF954: /* CJK COMPATIBILITY IDEOGRAPH-F954 */
    case 0xF955: /* CJK COMPATIBILITY IDEOGRAPH-F955 */
    case 0xF956: /* CJK COMPATIBILITY IDEOGRAPH-F956 */
    case 0xF957: /* CJK COMPATIBILITY IDEOGRAPH-F957 */
    case 0xF958: /* CJK COMPATIBILITY IDEOGRAPH-F958 */
    case 0xF959: /* CJK COMPATIBILITY IDEOGRAPH-F959 */
    case 0xF95A: /* CJK COMPATIBILITY IDEOGRAPH-F95A */
    case 0xF95B: /* CJK COMPATIBILITY IDEOGRAPH-F95B */
    case 0xF95C: /* CJK COMPATIBILITY IDEOGRAPH-F95C */
    case 0xF95D: /* CJK COMPATIBILITY IDEOGRAPH-F95D */
    case 0xF95E: /* CJK COMPATIBILITY IDEOGRAPH-F95E */
    case 0xF95F: /* CJK COMPATIBILITY IDEOGRAPH-F95F */
    case 0xF960: /* CJK COMPATIBILITY IDEOGRAPH-F960 */
    case 0xF961: /* CJK COMPATIBILITY IDEOGRAPH-F961 */
    case 0xF962: /* CJK COMPATIBILITY IDEOGRAPH-F962 */
    case 0xF963: /* CJK COMPATIBILITY IDEOGRAPH-F963 */
    case 0xF964: /* CJK COMPATIBILITY IDEOGRAPH-F964 */
    case 0xF965: /* CJK COMPATIBILITY IDEOGRAPH-F965 */
    case 0xF966: /* CJK COMPATIBILITY IDEOGRAPH-F966 */
    case 0xF967: /* CJK COMPATIBILITY IDEOGRAPH-F967 */
    case 0xF968: /* CJK COMPATIBILITY IDEOGRAPH-F968 */
    case 0xF969: /* CJK COMPATIBILITY IDEOGRAPH-F969 */
    case 0xF96A: /* CJK COMPATIBILITY IDEOGRAPH-F96A */
    case 0xF96B: /* CJK COMPATIBILITY IDEOGRAPH-F96B */
    case 0xF96C: /* CJK COMPATIBILITY IDEOGRAPH-F96C */
    case 0xF96D: /* CJK COMPATIBILITY IDEOGRAPH-F96D */
    case 0xF96E: /* CJK COMPATIBILITY IDEOGRAPH-F96E */
    case 0xF96F: /* CJK COMPATIBILITY IDEOGRAPH-F96F */
    case 0xF970: /* CJK COMPATIBILITY IDEOGRAPH-F970 */
    case 0xF971: /* CJK COMPATIBILITY IDEOGRAPH-F971 */
    case 0xF972: /* CJK COMPATIBILITY IDEOGRAPH-F972 */
    case 0xF973: /* CJK COMPATIBILITY IDEOGRAPH-F973 */
    case 0xF974: /* CJK COMPATIBILITY IDEOGRAPH-F974 */
    case 0xF975: /* CJK COMPATIBILITY IDEOGRAPH-F975 */
    case 0xF976: /* CJK COMPATIBILITY IDEOGRAPH-F976 */
    case 0xF977: /* CJK COMPATIBILITY IDEOGRAPH-F977 */
    case 0xF978: /* CJK COMPATIBILITY IDEOGRAPH-F978 */
    case 0xF979: /* CJK COMPATIBILITY IDEOGRAPH-F979 */
    case 0xF97A: /* CJK COMPATIBILITY IDEOGRAPH-F97A */
    case 0xF97B: /* CJK COMPATIBILITY IDEOGRAPH-F97B */
    case 0xF97C: /* CJK COMPATIBILITY IDEOGRAPH-F97C */
    case 0xF97D: /* CJK COMPATIBILITY IDEOGRAPH-F97D */
    case 0xF97E: /* CJK COMPATIBILITY IDEOGRAPH-F97E */
    case 0xF97F: /* CJK COMPATIBILITY IDEOGRAPH-F97F */
    case 0xF980: /* CJK COMPATIBILITY IDEOGRAPH-F980 */
    case 0xF981: /* CJK COMPATIBILITY IDEOGRAPH-F981 */
    case 0xF982: /* CJK COMPATIBILITY IDEOGRAPH-F982 */
    case 0xF983: /* CJK COMPATIBILITY IDEOGRAPH-F983 */
    case 0xF984: /* CJK COMPATIBILITY IDEOGRAPH-F984 */
    case 0xF985: /* CJK COMPATIBILITY IDEOGRAPH-F985 */
    case 0xF986: /* CJK COMPATIBILITY IDEOGRAPH-F986 */
    case 0xF987: /* CJK COMPATIBILITY IDEOGRAPH-F987 */
    case 0xF988: /* CJK COMPATIBILITY IDEOGRAPH-F988 */
    case 0xF989: /* CJK COMPATIBILITY IDEOGRAPH-F989 */
    case 0xF98A: /* CJK COMPATIBILITY IDEOGRAPH-F98A */
    case 0xF98B: /* CJK COMPATIBILITY IDEOGRAPH-F98B */
    case 0xF98C: /* CJK COMPATIBILITY IDEOGRAPH-F98C */
    case 0xF98D: /* CJK COMPATIBILITY IDEOGRAPH-F98D */
    case 0xF98E: /* CJK COMPATIBILITY IDEOGRAPH-F98E */
    case 0xF98F: /* CJK COMPATIBILITY IDEOGRAPH-F98F */
    case 0xF990: /* CJK COMPATIBILITY IDEOGRAPH-F990 */
    case 0xF991: /* CJK COMPATIBILITY IDEOGRAPH-F991 */
    case 0xF992: /* CJK COMPATIBILITY IDEOGRAPH-F992 */
    case 0xF993: /* CJK COMPATIBILITY IDEOGRAPH-F993 */
    case 0xF994: /* CJK COMPATIBILITY IDEOGRAPH-F994 */
    case 0xF995: /* CJK COMPATIBILITY IDEOGRAPH-F995 */
    case 0xF996: /* CJK COMPATIBILITY IDEOGRAPH-F996 */
    case 0xF997: /* CJK COMPATIBILITY IDEOGRAPH-F997 */
    case 0xF998: /* CJK COMPATIBILITY IDEOGRAPH-F998 */
    case 0xF999: /* CJK COMPATIBILITY IDEOGRAPH-F999 */
    case 0xF99A: /* CJK COMPATIBILITY IDEOGRAPH-F99A */
    case 0xF99B: /* CJK COMPATIBILITY IDEOGRAPH-F99B */
    case 0xF99C: /* CJK COMPATIBILITY IDEOGRAPH-F99C */
    case 0xF99D: /* CJK COMPATIBILITY IDEOGRAPH-F99D */
    case 0xF99E: /* CJK COMPATIBILITY IDEOGRAPH-F99E */
    case 0xF99F: /* CJK COMPATIBILITY IDEOGRAPH-F99F */
    case 0xF9A0: /* CJK COMPATIBILITY IDEOGRAPH-F9A0 */
    case 0xF9A1: /* CJK COMPATIBILITY IDEOGRAPH-F9A1 */
    case 0xF9A2: /* CJK COMPATIBILITY IDEOGRAPH-F9A2 */
    case 0xF9A3: /* CJK COMPATIBILITY IDEOGRAPH-F9A3 */
    case 0xF9A4: /* CJK COMPATIBILITY IDEOGRAPH-F9A4 */
    case 0xF9A5: /* CJK COMPATIBILITY IDEOGRAPH-F9A5 */
    case 0xF9A6: /* CJK COMPATIBILITY IDEOGRAPH-F9A6 */
    case 0xF9A7: /* CJK COMPATIBILITY IDEOGRAPH-F9A7 */
    case 0xF9A8: /* CJK COMPATIBILITY IDEOGRAPH-F9A8 */
    case 0xF9A9: /* CJK COMPATIBILITY IDEOGRAPH-F9A9 */
    case 0xF9AA: /* CJK COMPATIBILITY IDEOGRAPH-F9AA */
    case 0xF9AB: /* CJK COMPATIBILITY IDEOGRAPH-F9AB */
    case 0xF9AC: /* CJK COMPATIBILITY IDEOGRAPH-F9AC */
    case 0xF9AD: /* CJK COMPATIBILITY IDEOGRAPH-F9AD */
    case 0xF9AE: /* CJK COMPATIBILITY IDEOGRAPH-F9AE */
    case 0xF9AF: /* CJK COMPATIBILITY IDEOGRAPH-F9AF */
    case 0xF9B0: /* CJK COMPATIBILITY IDEOGRAPH-F9B0 */
    case 0xF9B1: /* CJK COMPATIBILITY IDEOGRAPH-F9B1 */
    case 0xF9B2: /* CJK COMPATIBILITY IDEOGRAPH-F9B2 */
    case 0xF9B3: /* CJK COMPATIBILITY IDEOGRAPH-F9B3 */
    case 0xF9B4: /* CJK COMPATIBILITY IDEOGRAPH-F9B4 */
    case 0xF9B5: /* CJK COMPATIBILITY IDEOGRAPH-F9B5 */
    case 0xF9B6: /* CJK COMPATIBILITY IDEOGRAPH-F9B6 */
    case 0xF9B7: /* CJK COMPATIBILITY IDEOGRAPH-F9B7 */
    case 0xF9B8: /* CJK COMPATIBILITY IDEOGRAPH-F9B8 */
    case 0xF9B9: /* CJK COMPATIBILITY IDEOGRAPH-F9B9 */
    case 0xF9BA: /* CJK COMPATIBILITY IDEOGRAPH-F9BA */
    case 0xF9BB: /* CJK COMPATIBILITY IDEOGRAPH-F9BB */
    case 0xF9BC: /* CJK COMPATIBILITY IDEOGRAPH-F9BC */
    case 0xF9BD: /* CJK COMPATIBILITY IDEOGRAPH-F9BD */
    case 0xF9BE: /* CJK COMPATIBILITY IDEOGRAPH-F9BE */
    case 0xF9BF: /* CJK COMPATIBILITY IDEOGRAPH-F9BF */
    case 0xF9C0: /* CJK COMPATIBILITY IDEOGRAPH-F9C0 */
    case 0xF9C1: /* CJK COMPATIBILITY IDEOGRAPH-F9C1 */
    case 0xF9C2: /* CJK COMPATIBILITY IDEOGRAPH-F9C2 */
    case 0xF9C3: /* CJK COMPATIBILITY IDEOGRAPH-F9C3 */
    case 0xF9C4: /* CJK COMPATIBILITY IDEOGRAPH-F9C4 */
    case 0xF9C5: /* CJK COMPATIBILITY IDEOGRAPH-F9C5 */
    case 0xF9C6: /* CJK COMPATIBILITY IDEOGRAPH-F9C6 */
    case 0xF9C7: /* CJK COMPATIBILITY IDEOGRAPH-F9C7 */
    case 0xF9C8: /* CJK COMPATIBILITY IDEOGRAPH-F9C8 */
    case 0xF9C9: /* CJK COMPATIBILITY IDEOGRAPH-F9C9 */
    case 0xF9CA: /* CJK COMPATIBILITY IDEOGRAPH-F9CA */
    case 0xF9CB: /* CJK COMPATIBILITY IDEOGRAPH-F9CB */
    case 0xF9CC: /* CJK COMPATIBILITY IDEOGRAPH-F9CC */
    case 0xF9CD: /* CJK COMPATIBILITY IDEOGRAPH-F9CD */
    case 0xF9CE: /* CJK COMPATIBILITY IDEOGRAPH-F9CE */
    case 0xF9CF: /* CJK COMPATIBILITY IDEOGRAPH-F9CF */
    case 0xF9D0: /* CJK COMPATIBILITY IDEOGRAPH-F9D0 */
    case 0xF9D1: /* CJK COMPATIBILITY IDEOGRAPH-F9D1 */
    case 0xF9D2: /* CJK COMPATIBILITY IDEOGRAPH-F9D2 */
    case 0xF9D3: /* CJK COMPATIBILITY IDEOGRAPH-F9D3 */
    case 0xF9D4: /* CJK COMPATIBILITY IDEOGRAPH-F9D4 */
    case 0xF9D5: /* CJK COMPATIBILITY IDEOGRAPH-F9D5 */
    case 0xF9D6: /* CJK COMPATIBILITY IDEOGRAPH-F9D6 */
    case 0xF9D7: /* CJK COMPATIBILITY IDEOGRAPH-F9D7 */
    case 0xF9D8: /* CJK COMPATIBILITY IDEOGRAPH-F9D8 */
    case 0xF9D9: /* CJK COMPATIBILITY IDEOGRAPH-F9D9 */
    case 0xF9DA: /* CJK COMPATIBILITY IDEOGRAPH-F9DA */
    case 0xF9DB: /* CJK COMPATIBILITY IDEOGRAPH-F9DB */
    case 0xF9DC: /* CJK COMPATIBILITY IDEOGRAPH-F9DC */
    case 0xF9DD: /* CJK COMPATIBILITY IDEOGRAPH-F9DD */
    case 0xF9DE: /* CJK COMPATIBILITY IDEOGRAPH-F9DE */
    case 0xF9DF: /* CJK COMPATIBILITY IDEOGRAPH-F9DF */
    case 0xF9E0: /* CJK COMPATIBILITY IDEOGRAPH-F9E0 */
    case 0xF9E1: /* CJK COMPATIBILITY IDEOGRAPH-F9E1 */
    case 0xF9E2: /* CJK COMPATIBILITY IDEOGRAPH-F9E2 */
    case 0xF9E3: /* CJK COMPATIBILITY IDEOGRAPH-F9E3 */
    case 0xF9E4: /* CJK COMPATIBILITY IDEOGRAPH-F9E4 */
    case 0xF9E5: /* CJK COMPATIBILITY IDEOGRAPH-F9E5 */
    case 0xF9E6: /* CJK COMPATIBILITY IDEOGRAPH-F9E6 */
    case 0xF9E7: /* CJK COMPATIBILITY IDEOGRAPH-F9E7 */
    case 0xF9E8: /* CJK COMPATIBILITY IDEOGRAPH-F9E8 */
    case 0xF9E9: /* CJK COMPATIBILITY IDEOGRAPH-F9E9 */
    case 0xF9EA: /* CJK COMPATIBILITY IDEOGRAPH-F9EA */
    case 0xF9EB: /* CJK COMPATIBILITY IDEOGRAPH-F9EB */
    case 0xF9EC: /* CJK COMPATIBILITY IDEOGRAPH-F9EC */
    case 0xF9ED: /* CJK COMPATIBILITY IDEOGRAPH-F9ED */
    case 0xF9EE: /* CJK COMPATIBILITY IDEOGRAPH-F9EE */
    case 0xF9EF: /* CJK COMPATIBILITY IDEOGRAPH-F9EF */
    case 0xF9F0: /* CJK COMPATIBILITY IDEOGRAPH-F9F0 */
    case 0xF9F1: /* CJK COMPATIBILITY IDEOGRAPH-F9F1 */
    case 0xF9F2: /* CJK COMPATIBILITY IDEOGRAPH-F9F2 */
    case 0xF9F3: /* CJK COMPATIBILITY IDEOGRAPH-F9F3 */
    case 0xF9F4: /* CJK COMPATIBILITY IDEOGRAPH-F9F4 */
    case 0xF9F5: /* CJK COMPATIBILITY IDEOGRAPH-F9F5 */
    case 0xF9F6: /* CJK COMPATIBILITY IDEOGRAPH-F9F6 */
    case 0xF9F7: /* CJK COMPATIBILITY IDEOGRAPH-F9F7 */
    case 0xF9F8: /* CJK COMPATIBILITY IDEOGRAPH-F9F8 */
    case 0xF9F9: /* CJK COMPATIBILITY IDEOGRAPH-F9F9 */
    case 0xF9FA: /* CJK COMPATIBILITY IDEOGRAPH-F9FA */
    case 0xF9FB: /* CJK COMPATIBILITY IDEOGRAPH-F9FB */
    case 0xF9FC: /* CJK COMPATIBILITY IDEOGRAPH-F9FC */
    case 0xF9FD: /* CJK COMPATIBILITY IDEOGRAPH-F9FD */
    case 0xF9FE: /* CJK COMPATIBILITY IDEOGRAPH-F9FE */
    case 0xF9FF: /* CJK COMPATIBILITY IDEOGRAPH-F9FF */
    case 0xFA00: /* CJK COMPATIBILITY IDEOGRAPH-FA00 */
    case 0xFA01: /* CJK COMPATIBILITY IDEOGRAPH-FA01 */
    case 0xFA02: /* CJK COMPATIBILITY IDEOGRAPH-FA02 */
    case 0xFA03: /* CJK COMPATIBILITY IDEOGRAPH-FA03 */
    case 0xFA04: /* CJK COMPATIBILITY IDEOGRAPH-FA04 */
    case 0xFA05: /* CJK COMPATIBILITY IDEOGRAPH-FA05 */
    case 0xFA06: /* CJK COMPATIBILITY IDEOGRAPH-FA06 */
    case 0xFA07: /* CJK COMPATIBILITY IDEOGRAPH-FA07 */
    case 0xFA08: /* CJK COMPATIBILITY IDEOGRAPH-FA08 */
    case 0xFA09: /* CJK COMPATIBILITY IDEOGRAPH-FA09 */
    case 0xFA0A: /* CJK COMPATIBILITY IDEOGRAPH-FA0A */
    case 0xFA0B: /* CJK COMPATIBILITY IDEOGRAPH-FA0B */
    case 0xFA0C: /* CJK COMPATIBILITY IDEOGRAPH-FA0C */
    case 0xFA0D: /* CJK COMPATIBILITY IDEOGRAPH-FA0D */
    case 0xFA0E: /* CJK COMPATIBILITY IDEOGRAPH-FA0E */
    case 0xFA0F: /* CJK COMPATIBILITY IDEOGRAPH-FA0F */
    case 0xFA10: /* CJK COMPATIBILITY IDEOGRAPH-FA10 */
    case 0xFA11: /* CJK COMPATIBILITY IDEOGRAPH-FA11 */
    case 0xFA12: /* CJK COMPATIBILITY IDEOGRAPH-FA12 */
    case 0xFA13: /* CJK COMPATIBILITY IDEOGRAPH-FA13 */
    case 0xFA14: /* CJK COMPATIBILITY IDEOGRAPH-FA14 */
    case 0xFA15: /* CJK COMPATIBILITY IDEOGRAPH-FA15 */
    case 0xFA16: /* CJK COMPATIBILITY IDEOGRAPH-FA16 */
    case 0xFA17: /* CJK COMPATIBILITY IDEOGRAPH-FA17 */
    case 0xFA18: /* CJK COMPATIBILITY IDEOGRAPH-FA18 */
    case 0xFA19: /* CJK COMPATIBILITY IDEOGRAPH-FA19 */
    case 0xFA1A: /* CJK COMPATIBILITY IDEOGRAPH-FA1A */
    case 0xFA1B: /* CJK COMPATIBILITY IDEOGRAPH-FA1B */
    case 0xFA1C: /* CJK COMPATIBILITY IDEOGRAPH-FA1C */
    case 0xFA1D: /* CJK COMPATIBILITY IDEOGRAPH-FA1D */
    case 0xFA1E: /* CJK COMPATIBILITY IDEOGRAPH-FA1E */
    case 0xFA1F: /* CJK COMPATIBILITY IDEOGRAPH-FA1F */
    case 0xFA20: /* CJK COMPATIBILITY IDEOGRAPH-FA20 */
    case 0xFA21: /* CJK COMPATIBILITY IDEOGRAPH-FA21 */
    case 0xFA22: /* CJK COMPATIBILITY IDEOGRAPH-FA22 */
    case 0xFA23: /* CJK COMPATIBILITY IDEOGRAPH-FA23 */
    case 0xFA24: /* CJK COMPATIBILITY IDEOGRAPH-FA24 */
    case 0xFA25: /* CJK COMPATIBILITY IDEOGRAPH-FA25 */
    case 0xFA26: /* CJK COMPATIBILITY IDEOGRAPH-FA26 */
    case 0xFA27: /* CJK COMPATIBILITY IDEOGRAPH-FA27 */
    case 0xFA28: /* CJK COMPATIBILITY IDEOGRAPH-FA28 */
    case 0xFA29: /* CJK COMPATIBILITY IDEOGRAPH-FA29 */
    case 0xFA2A: /* CJK COMPATIBILITY IDEOGRAPH-FA2A */
    case 0xFA2B: /* CJK COMPATIBILITY IDEOGRAPH-FA2B */
    case 0xFA2C: /* CJK COMPATIBILITY IDEOGRAPH-FA2C */
    case 0xFA2D: /* CJK COMPATIBILITY IDEOGRAPH-FA2D */
    case 0xFB1D: /* HEBREW LETTER YOD WITH HIRIQ */
    case 0xFB1F: /* HEBREW LIGATURE YIDDISH YOD YOD PATAH */
    case 0xFB20: /* HEBREW LETTER ALTERNATIVE AYIN */
    case 0xFB21: /* HEBREW LETTER WIDE ALEF */
    case 0xFB22: /* HEBREW LETTER WIDE DALET */
    case 0xFB23: /* HEBREW LETTER WIDE HE */
    case 0xFB24: /* HEBREW LETTER WIDE KAF */
    case 0xFB25: /* HEBREW LETTER WIDE LAMED */
    case 0xFB26: /* HEBREW LETTER WIDE FINAL MEM */
    case 0xFB27: /* HEBREW LETTER WIDE RESH */
    case 0xFB28: /* HEBREW LETTER WIDE TAV */
    case 0xFB2A: /* HEBREW LETTER SHIN WITH SHIN DOT */
    case 0xFB2B: /* HEBREW LETTER SHIN WITH SIN DOT */
    case 0xFB2C: /* HEBREW LETTER SHIN WITH DAGESH AND SHIN DOT */
    case 0xFB2D: /* HEBREW LETTER SHIN WITH DAGESH AND SIN DOT */
    case 0xFB2E: /* HEBREW LETTER ALEF WITH PATAH */
    case 0xFB2F: /* HEBREW LETTER ALEF WITH QAMATS */
    case 0xFB30: /* HEBREW LETTER ALEF WITH MAPIQ */
    case 0xFB31: /* HEBREW LETTER BET WITH DAGESH */
    case 0xFB32: /* HEBREW LETTER GIMEL WITH DAGESH */
    case 0xFB33: /* HEBREW LETTER DALET WITH DAGESH */
    case 0xFB34: /* HEBREW LETTER HE WITH MAPIQ */
    case 0xFB35: /* HEBREW LETTER VAV WITH DAGESH */
    case 0xFB36: /* HEBREW LETTER ZAYIN WITH DAGESH */
    case 0xFB38: /* HEBREW LETTER TET WITH DAGESH */
    case 0xFB39: /* HEBREW LETTER YOD WITH DAGESH */
    case 0xFB3A: /* HEBREW LETTER FINAL KAF WITH DAGESH */
    case 0xFB3B: /* HEBREW LETTER KAF WITH DAGESH */
    case 0xFB3C: /* HEBREW LETTER LAMED WITH DAGESH */
    case 0xFB3E: /* HEBREW LETTER MEM WITH DAGESH */
    case 0xFB40: /* HEBREW LETTER NUN WITH DAGESH */
    case 0xFB41: /* HEBREW LETTER SAMEKH WITH DAGESH */
    case 0xFB43: /* HEBREW LETTER FINAL PE WITH DAGESH */
    case 0xFB44: /* HEBREW LETTER PE WITH DAGESH */
    case 0xFB46: /* HEBREW LETTER TSADI WITH DAGESH */
    case 0xFB47: /* HEBREW LETTER QOF WITH DAGESH */
    case 0xFB48: /* HEBREW LETTER RESH WITH DAGESH */
    case 0xFB49: /* HEBREW LETTER SHIN WITH DAGESH */
    case 0xFB4A: /* HEBREW LETTER TAV WITH DAGESH */
    case 0xFB4B: /* HEBREW LETTER VAV WITH HOLAM */
    case 0xFB4C: /* HEBREW LETTER BET WITH RAFE */
    case 0xFB4D: /* HEBREW LETTER KAF WITH RAFE */
    case 0xFB4E: /* HEBREW LETTER PE WITH RAFE */
    case 0xFB4F: /* HEBREW LIGATURE ALEF LAMED */
    case 0xFB50: /* ARABIC LETTER ALEF WASLA ISOLATED FORM */
    case 0xFB51: /* ARABIC LETTER ALEF WASLA FINAL FORM */
    case 0xFB52: /* ARABIC LETTER BEEH ISOLATED FORM */
    case 0xFB53: /* ARABIC LETTER BEEH FINAL FORM */
    case 0xFB54: /* ARABIC LETTER BEEH INITIAL FORM */
    case 0xFB55: /* ARABIC LETTER BEEH MEDIAL FORM */
    case 0xFB56: /* ARABIC LETTER PEH ISOLATED FORM */
    case 0xFB57: /* ARABIC LETTER PEH FINAL FORM */
    case 0xFB58: /* ARABIC LETTER PEH INITIAL FORM */
    case 0xFB59: /* ARABIC LETTER PEH MEDIAL FORM */
    case 0xFB5A: /* ARABIC LETTER BEHEH ISOLATED FORM */
    case 0xFB5B: /* ARABIC LETTER BEHEH FINAL FORM */
    case 0xFB5C: /* ARABIC LETTER BEHEH INITIAL FORM */
    case 0xFB5D: /* ARABIC LETTER BEHEH MEDIAL FORM */
    case 0xFB5E: /* ARABIC LETTER TTEHEH ISOLATED FORM */
    case 0xFB5F: /* ARABIC LETTER TTEHEH FINAL FORM */
    case 0xFB60: /* ARABIC LETTER TTEHEH INITIAL FORM */
    case 0xFB61: /* ARABIC LETTER TTEHEH MEDIAL FORM */
    case 0xFB62: /* ARABIC LETTER TEHEH ISOLATED FORM */
    case 0xFB63: /* ARABIC LETTER TEHEH FINAL FORM */
    case 0xFB64: /* ARABIC LETTER TEHEH INITIAL FORM */
    case 0xFB65: /* ARABIC LETTER TEHEH MEDIAL FORM */
    case 0xFB66: /* ARABIC LETTER TTEH ISOLATED FORM */
    case 0xFB67: /* ARABIC LETTER TTEH FINAL FORM */
    case 0xFB68: /* ARABIC LETTER TTEH INITIAL FORM */
    case 0xFB69: /* ARABIC LETTER TTEH MEDIAL FORM */
    case 0xFB6A: /* ARABIC LETTER VEH ISOLATED FORM */
    case 0xFB6B: /* ARABIC LETTER VEH FINAL FORM */
    case 0xFB6C: /* ARABIC LETTER VEH INITIAL FORM */
    case 0xFB6D: /* ARABIC LETTER VEH MEDIAL FORM */
    case 0xFB6E: /* ARABIC LETTER PEHEH ISOLATED FORM */
    case 0xFB6F: /* ARABIC LETTER PEHEH FINAL FORM */
    case 0xFB70: /* ARABIC LETTER PEHEH INITIAL FORM */
    case 0xFB71: /* ARABIC LETTER PEHEH MEDIAL FORM */
    case 0xFB72: /* ARABIC LETTER DYEH ISOLATED FORM */
    case 0xFB73: /* ARABIC LETTER DYEH FINAL FORM */
    case 0xFB74: /* ARABIC LETTER DYEH INITIAL FORM */
    case 0xFB75: /* ARABIC LETTER DYEH MEDIAL FORM */
    case 0xFB76: /* ARABIC LETTER NYEH ISOLATED FORM */
    case 0xFB77: /* ARABIC LETTER NYEH FINAL FORM */
    case 0xFB78: /* ARABIC LETTER NYEH INITIAL FORM */
    case 0xFB79: /* ARABIC LETTER NYEH MEDIAL FORM */
    case 0xFB7A: /* ARABIC LETTER TCHEH ISOLATED FORM */
    case 0xFB7B: /* ARABIC LETTER TCHEH FINAL FORM */
    case 0xFB7C: /* ARABIC LETTER TCHEH INITIAL FORM */
    case 0xFB7D: /* ARABIC LETTER TCHEH MEDIAL FORM */
    case 0xFB7E: /* ARABIC LETTER TCHEHEH ISOLATED FORM */
    case 0xFB7F: /* ARABIC LETTER TCHEHEH FINAL FORM */
    case 0xFB80: /* ARABIC LETTER TCHEHEH INITIAL FORM */
    case 0xFB81: /* ARABIC LETTER TCHEHEH MEDIAL FORM */
    case 0xFB82: /* ARABIC LETTER DDAHAL ISOLATED FORM */
    case 0xFB83: /* ARABIC LETTER DDAHAL FINAL FORM */
    case 0xFB84: /* ARABIC LETTER DAHAL ISOLATED FORM */
    case 0xFB85: /* ARABIC LETTER DAHAL FINAL FORM */
    case 0xFB86: /* ARABIC LETTER DUL ISOLATED FORM */
    case 0xFB87: /* ARABIC LETTER DUL FINAL FORM */
    case 0xFB88: /* ARABIC LETTER DDAL ISOLATED FORM */
    case 0xFB89: /* ARABIC LETTER DDAL FINAL FORM */
    case 0xFB8A: /* ARABIC LETTER JEH ISOLATED FORM */
    case 0xFB8B: /* ARABIC LETTER JEH FINAL FORM */
    case 0xFB8C: /* ARABIC LETTER RREH ISOLATED FORM */
    case 0xFB8D: /* ARABIC LETTER RREH FINAL FORM */
    case 0xFB8E: /* ARABIC LETTER KEHEH ISOLATED FORM */
    case 0xFB8F: /* ARABIC LETTER KEHEH FINAL FORM */
    case 0xFB90: /* ARABIC LETTER KEHEH INITIAL FORM */
    case 0xFB91: /* ARABIC LETTER KEHEH MEDIAL FORM */
    case 0xFB92: /* ARABIC LETTER GAF ISOLATED FORM */
    case 0xFB93: /* ARABIC LETTER GAF FINAL FORM */
    case 0xFB94: /* ARABIC LETTER GAF INITIAL FORM */
    case 0xFB95: /* ARABIC LETTER GAF MEDIAL FORM */
    case 0xFB96: /* ARABIC LETTER GUEH ISOLATED FORM */
    case 0xFB97: /* ARABIC LETTER GUEH FINAL FORM */
    case 0xFB98: /* ARABIC LETTER GUEH INITIAL FORM */
    case 0xFB99: /* ARABIC LETTER GUEH MEDIAL FORM */
    case 0xFB9A: /* ARABIC LETTER NGOEH ISOLATED FORM */
    case 0xFB9B: /* ARABIC LETTER NGOEH FINAL FORM */
    case 0xFB9C: /* ARABIC LETTER NGOEH INITIAL FORM */
    case 0xFB9D: /* ARABIC LETTER NGOEH MEDIAL FORM */
    case 0xFB9E: /* ARABIC LETTER NOON GHUNNA ISOLATED FORM */
    case 0xFB9F: /* ARABIC LETTER NOON GHUNNA FINAL FORM */
    case 0xFBA0: /* ARABIC LETTER RNOON ISOLATED FORM */
    case 0xFBA1: /* ARABIC LETTER RNOON FINAL FORM */
    case 0xFBA2: /* ARABIC LETTER RNOON INITIAL FORM */
    case 0xFBA3: /* ARABIC LETTER RNOON MEDIAL FORM */
    case 0xFBA4: /* ARABIC LETTER HEH WITH YEH ABOVE ISOLATED FORM */
    case 0xFBA5: /* ARABIC LETTER HEH WITH YEH ABOVE FINAL FORM */
    case 0xFBA6: /* ARABIC LETTER HEH GOAL ISOLATED FORM */
    case 0xFBA7: /* ARABIC LETTER HEH GOAL FINAL FORM */
    case 0xFBA8: /* ARABIC LETTER HEH GOAL INITIAL FORM */
    case 0xFBA9: /* ARABIC LETTER HEH GOAL MEDIAL FORM */
    case 0xFBAA: /* ARABIC LETTER HEH DOACHASHMEE ISOLATED FORM */
    case 0xFBAB: /* ARABIC LETTER HEH DOACHASHMEE FINAL FORM */
    case 0xFBAC: /* ARABIC LETTER HEH DOACHASHMEE INITIAL FORM */
    case 0xFBAD: /* ARABIC LETTER HEH DOACHASHMEE MEDIAL FORM */
    case 0xFBAE: /* ARABIC LETTER YEH BARREE ISOLATED FORM */
    case 0xFBAF: /* ARABIC LETTER YEH BARREE FINAL FORM */
    case 0xFBB0: /* ARABIC LETTER YEH BARREE WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFBB1: /* ARABIC LETTER YEH BARREE WITH HAMZA ABOVE FINAL FORM */
    case 0xFBD3: /* ARABIC LETTER NG ISOLATED FORM */
    case 0xFBD4: /* ARABIC LETTER NG FINAL FORM */
    case 0xFBD5: /* ARABIC LETTER NG INITIAL FORM */
    case 0xFBD6: /* ARABIC LETTER NG MEDIAL FORM */
    case 0xFBD7: /* ARABIC LETTER U ISOLATED FORM */
    case 0xFBD8: /* ARABIC LETTER U FINAL FORM */
    case 0xFBD9: /* ARABIC LETTER OE ISOLATED FORM */
    case 0xFBDA: /* ARABIC LETTER OE FINAL FORM */
    case 0xFBDB: /* ARABIC LETTER YU ISOLATED FORM */
    case 0xFBDC: /* ARABIC LETTER YU FINAL FORM */
    case 0xFBDD: /* ARABIC LETTER U WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFBDE: /* ARABIC LETTER VE ISOLATED FORM */
    case 0xFBDF: /* ARABIC LETTER VE FINAL FORM */
    case 0xFBE0: /* ARABIC LETTER KIRGHIZ OE ISOLATED FORM */
    case 0xFBE1: /* ARABIC LETTER KIRGHIZ OE FINAL FORM */
    case 0xFBE2: /* ARABIC LETTER KIRGHIZ YU ISOLATED FORM */
    case 0xFBE3: /* ARABIC LETTER KIRGHIZ YU FINAL FORM */
    case 0xFBE4: /* ARABIC LETTER E ISOLATED FORM */
    case 0xFBE5: /* ARABIC LETTER E FINAL FORM */
    case 0xFBE6: /* ARABIC LETTER E INITIAL FORM */
    case 0xFBE7: /* ARABIC LETTER E MEDIAL FORM */
    case 0xFBE8: /* ARABIC LETTER UIGHUR KAZAKH KIRGHIZ ALEF MAKSURA INITIAL FORM */
    case 0xFBE9: /* ARABIC LETTER UIGHUR KAZAKH KIRGHIZ ALEF MAKSURA MEDIAL FORM */
    case 0xFBEA: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH ALEF ISOLATED FORM */
    case 0xFBEB: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH ALEF FINAL FORM */
    case 0xFBEC: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH AE ISOLATED FORM */
    case 0xFBED: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH AE FINAL FORM */
    case 0xFBEE: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH WAW ISOLATED FORM */
    case 0xFBEF: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH WAW FINAL FORM */
    case 0xFBF0: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH U ISOLATED FORM */
    case 0xFBF1: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH U FINAL FORM */
    case 0xFBF2: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH OE ISOLATED FORM */
    case 0xFBF3: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH OE FINAL FORM */
    case 0xFBF4: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH YU ISOLATED FORM */
    case 0xFBF5: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH YU FINAL FORM */
    case 0xFBF6: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH E ISOLATED FORM */
    case 0xFBF7: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH E FINAL FORM */
    case 0xFBF8: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH E INITIAL FORM */
    case 0xFBF9: /* ARABIC LIGATURE UIGHUR KIRGHIZ YEH WITH HAMZA ABOVE WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFBFA: /* ARABIC LIGATURE UIGHUR KIRGHIZ YEH WITH HAMZA ABOVE WITH ALEF MAKSURA FINAL FORM */
    case 0xFBFB: /* ARABIC LIGATURE UIGHUR KIRGHIZ YEH WITH HAMZA ABOVE WITH ALEF MAKSURA INITIAL FORM */
    case 0xFBFC: /* ARABIC LETTER FARSI YEH ISOLATED FORM */
    case 0xFBFD: /* ARABIC LETTER FARSI YEH FINAL FORM */
    case 0xFBFE: /* ARABIC LETTER FARSI YEH INITIAL FORM */
    case 0xFBFF: /* ARABIC LETTER FARSI YEH MEDIAL FORM */
    case 0xFC00: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH JEEM ISOLATED FORM */
    case 0xFC01: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH HAH ISOLATED FORM */
    case 0xFC02: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH MEEM ISOLATED FORM */
    case 0xFC03: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC04: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH YEH ISOLATED FORM */
    case 0xFC05: /* ARABIC LIGATURE BEH WITH JEEM ISOLATED FORM */
    case 0xFC06: /* ARABIC LIGATURE BEH WITH HAH ISOLATED FORM */
    case 0xFC07: /* ARABIC LIGATURE BEH WITH KHAH ISOLATED FORM */
    case 0xFC08: /* ARABIC LIGATURE BEH WITH MEEM ISOLATED FORM */
    case 0xFC09: /* ARABIC LIGATURE BEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC0A: /* ARABIC LIGATURE BEH WITH YEH ISOLATED FORM */
    case 0xFC0B: /* ARABIC LIGATURE TEH WITH JEEM ISOLATED FORM */
    case 0xFC0C: /* ARABIC LIGATURE TEH WITH HAH ISOLATED FORM */
    case 0xFC0D: /* ARABIC LIGATURE TEH WITH KHAH ISOLATED FORM */
    case 0xFC0E: /* ARABIC LIGATURE TEH WITH MEEM ISOLATED FORM */
    case 0xFC0F: /* ARABIC LIGATURE TEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC10: /* ARABIC LIGATURE TEH WITH YEH ISOLATED FORM */
    case 0xFC11: /* ARABIC LIGATURE THEH WITH JEEM ISOLATED FORM */
    case 0xFC12: /* ARABIC LIGATURE THEH WITH MEEM ISOLATED FORM */
    case 0xFC13: /* ARABIC LIGATURE THEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC14: /* ARABIC LIGATURE THEH WITH YEH ISOLATED FORM */
    case 0xFC15: /* ARABIC LIGATURE JEEM WITH HAH ISOLATED FORM */
    case 0xFC16: /* ARABIC LIGATURE JEEM WITH MEEM ISOLATED FORM */
    case 0xFC17: /* ARABIC LIGATURE HAH WITH JEEM ISOLATED FORM */
    case 0xFC18: /* ARABIC LIGATURE HAH WITH MEEM ISOLATED FORM */
    case 0xFC19: /* ARABIC LIGATURE KHAH WITH JEEM ISOLATED FORM */
    case 0xFC1A: /* ARABIC LIGATURE KHAH WITH HAH ISOLATED FORM */
    case 0xFC1B: /* ARABIC LIGATURE KHAH WITH MEEM ISOLATED FORM */
    case 0xFC1C: /* ARABIC LIGATURE SEEN WITH JEEM ISOLATED FORM */
    case 0xFC1D: /* ARABIC LIGATURE SEEN WITH HAH ISOLATED FORM */
    case 0xFC1E: /* ARABIC LIGATURE SEEN WITH KHAH ISOLATED FORM */
    case 0xFC1F: /* ARABIC LIGATURE SEEN WITH MEEM ISOLATED FORM */
    case 0xFC20: /* ARABIC LIGATURE SAD WITH HAH ISOLATED FORM */
    case 0xFC21: /* ARABIC LIGATURE SAD WITH MEEM ISOLATED FORM */
    case 0xFC22: /* ARABIC LIGATURE DAD WITH JEEM ISOLATED FORM */
    case 0xFC23: /* ARABIC LIGATURE DAD WITH HAH ISOLATED FORM */
    case 0xFC24: /* ARABIC LIGATURE DAD WITH KHAH ISOLATED FORM */
    case 0xFC25: /* ARABIC LIGATURE DAD WITH MEEM ISOLATED FORM */
    case 0xFC26: /* ARABIC LIGATURE TAH WITH HAH ISOLATED FORM */
    case 0xFC27: /* ARABIC LIGATURE TAH WITH MEEM ISOLATED FORM */
    case 0xFC28: /* ARABIC LIGATURE ZAH WITH MEEM ISOLATED FORM */
    case 0xFC29: /* ARABIC LIGATURE AIN WITH JEEM ISOLATED FORM */
    case 0xFC2A: /* ARABIC LIGATURE AIN WITH MEEM ISOLATED FORM */
    case 0xFC2B: /* ARABIC LIGATURE GHAIN WITH JEEM ISOLATED FORM */
    case 0xFC2C: /* ARABIC LIGATURE GHAIN WITH MEEM ISOLATED FORM */
    case 0xFC2D: /* ARABIC LIGATURE FEH WITH JEEM ISOLATED FORM */
    case 0xFC2E: /* ARABIC LIGATURE FEH WITH HAH ISOLATED FORM */
    case 0xFC2F: /* ARABIC LIGATURE FEH WITH KHAH ISOLATED FORM */
    case 0xFC30: /* ARABIC LIGATURE FEH WITH MEEM ISOLATED FORM */
    case 0xFC31: /* ARABIC LIGATURE FEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC32: /* ARABIC LIGATURE FEH WITH YEH ISOLATED FORM */
    case 0xFC33: /* ARABIC LIGATURE QAF WITH HAH ISOLATED FORM */
    case 0xFC34: /* ARABIC LIGATURE QAF WITH MEEM ISOLATED FORM */
    case 0xFC35: /* ARABIC LIGATURE QAF WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC36: /* ARABIC LIGATURE QAF WITH YEH ISOLATED FORM */
    case 0xFC37: /* ARABIC LIGATURE KAF WITH ALEF ISOLATED FORM */
    case 0xFC38: /* ARABIC LIGATURE KAF WITH JEEM ISOLATED FORM */
    case 0xFC39: /* ARABIC LIGATURE KAF WITH HAH ISOLATED FORM */
    case 0xFC3A: /* ARABIC LIGATURE KAF WITH KHAH ISOLATED FORM */
    case 0xFC3B: /* ARABIC LIGATURE KAF WITH LAM ISOLATED FORM */
    case 0xFC3C: /* ARABIC LIGATURE KAF WITH MEEM ISOLATED FORM */
    case 0xFC3D: /* ARABIC LIGATURE KAF WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC3E: /* ARABIC LIGATURE KAF WITH YEH ISOLATED FORM */
    case 0xFC3F: /* ARABIC LIGATURE LAM WITH JEEM ISOLATED FORM */
    case 0xFC40: /* ARABIC LIGATURE LAM WITH HAH ISOLATED FORM */
    case 0xFC41: /* ARABIC LIGATURE LAM WITH KHAH ISOLATED FORM */
    case 0xFC42: /* ARABIC LIGATURE LAM WITH MEEM ISOLATED FORM */
    case 0xFC43: /* ARABIC LIGATURE LAM WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC44: /* ARABIC LIGATURE LAM WITH YEH ISOLATED FORM */
    case 0xFC45: /* ARABIC LIGATURE MEEM WITH JEEM ISOLATED FORM */
    case 0xFC46: /* ARABIC LIGATURE MEEM WITH HAH ISOLATED FORM */
    case 0xFC47: /* ARABIC LIGATURE MEEM WITH KHAH ISOLATED FORM */
    case 0xFC48: /* ARABIC LIGATURE MEEM WITH MEEM ISOLATED FORM */
    case 0xFC49: /* ARABIC LIGATURE MEEM WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC4A: /* ARABIC LIGATURE MEEM WITH YEH ISOLATED FORM */
    case 0xFC4B: /* ARABIC LIGATURE NOON WITH JEEM ISOLATED FORM */
    case 0xFC4C: /* ARABIC LIGATURE NOON WITH HAH ISOLATED FORM */
    case 0xFC4D: /* ARABIC LIGATURE NOON WITH KHAH ISOLATED FORM */
    case 0xFC4E: /* ARABIC LIGATURE NOON WITH MEEM ISOLATED FORM */
    case 0xFC4F: /* ARABIC LIGATURE NOON WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC50: /* ARABIC LIGATURE NOON WITH YEH ISOLATED FORM */
    case 0xFC51: /* ARABIC LIGATURE HEH WITH JEEM ISOLATED FORM */
    case 0xFC52: /* ARABIC LIGATURE HEH WITH MEEM ISOLATED FORM */
    case 0xFC53: /* ARABIC LIGATURE HEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC54: /* ARABIC LIGATURE HEH WITH YEH ISOLATED FORM */
    case 0xFC55: /* ARABIC LIGATURE YEH WITH JEEM ISOLATED FORM */
    case 0xFC56: /* ARABIC LIGATURE YEH WITH HAH ISOLATED FORM */
    case 0xFC57: /* ARABIC LIGATURE YEH WITH KHAH ISOLATED FORM */
    case 0xFC58: /* ARABIC LIGATURE YEH WITH MEEM ISOLATED FORM */
    case 0xFC59: /* ARABIC LIGATURE YEH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFC5A: /* ARABIC LIGATURE YEH WITH YEH ISOLATED FORM */
    case 0xFC5B: /* ARABIC LIGATURE THAL WITH SUPERSCRIPT ALEF ISOLATED FORM */
    case 0xFC5C: /* ARABIC LIGATURE REH WITH SUPERSCRIPT ALEF ISOLATED FORM */
    case 0xFC5D: /* ARABIC LIGATURE ALEF MAKSURA WITH SUPERSCRIPT ALEF ISOLATED FORM */
    case 0xFC5E: /* ARABIC LIGATURE SHADDA WITH DAMMATAN ISOLATED FORM */
    case 0xFC5F: /* ARABIC LIGATURE SHADDA WITH KASRATAN ISOLATED FORM */
    case 0xFC60: /* ARABIC LIGATURE SHADDA WITH FATHA ISOLATED FORM */
    case 0xFC61: /* ARABIC LIGATURE SHADDA WITH DAMMA ISOLATED FORM */
    case 0xFC62: /* ARABIC LIGATURE SHADDA WITH KASRA ISOLATED FORM */
    case 0xFC63: /* ARABIC LIGATURE SHADDA WITH SUPERSCRIPT ALEF ISOLATED FORM */
    case 0xFC64: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH REH FINAL FORM */
    case 0xFC65: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH ZAIN FINAL FORM */
    case 0xFC66: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH MEEM FINAL FORM */
    case 0xFC67: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH NOON FINAL FORM */
    case 0xFC68: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH ALEF MAKSURA FINAL FORM */
    case 0xFC69: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH YEH FINAL FORM */
    case 0xFC6A: /* ARABIC LIGATURE BEH WITH REH FINAL FORM */
    case 0xFC6B: /* ARABIC LIGATURE BEH WITH ZAIN FINAL FORM */
    case 0xFC6C: /* ARABIC LIGATURE BEH WITH MEEM FINAL FORM */
    case 0xFC6D: /* ARABIC LIGATURE BEH WITH NOON FINAL FORM */
    case 0xFC6E: /* ARABIC LIGATURE BEH WITH ALEF MAKSURA FINAL FORM */
    case 0xFC6F: /* ARABIC LIGATURE BEH WITH YEH FINAL FORM */
    case 0xFC70: /* ARABIC LIGATURE TEH WITH REH FINAL FORM */
    case 0xFC71: /* ARABIC LIGATURE TEH WITH ZAIN FINAL FORM */
    case 0xFC72: /* ARABIC LIGATURE TEH WITH MEEM FINAL FORM */
    case 0xFC73: /* ARABIC LIGATURE TEH WITH NOON FINAL FORM */
    case 0xFC74: /* ARABIC LIGATURE TEH WITH ALEF MAKSURA FINAL FORM */
    case 0xFC75: /* ARABIC LIGATURE TEH WITH YEH FINAL FORM */
    case 0xFC76: /* ARABIC LIGATURE THEH WITH REH FINAL FORM */
    case 0xFC77: /* ARABIC LIGATURE THEH WITH ZAIN FINAL FORM */
    case 0xFC78: /* ARABIC LIGATURE THEH WITH MEEM FINAL FORM */
    case 0xFC79: /* ARABIC LIGATURE THEH WITH NOON FINAL FORM */
    case 0xFC7A: /* ARABIC LIGATURE THEH WITH ALEF MAKSURA FINAL FORM */
    case 0xFC7B: /* ARABIC LIGATURE THEH WITH YEH FINAL FORM */
    case 0xFC7C: /* ARABIC LIGATURE FEH WITH ALEF MAKSURA FINAL FORM */
    case 0xFC7D: /* ARABIC LIGATURE FEH WITH YEH FINAL FORM */
    case 0xFC7E: /* ARABIC LIGATURE QAF WITH ALEF MAKSURA FINAL FORM */
    case 0xFC7F: /* ARABIC LIGATURE QAF WITH YEH FINAL FORM */
    case 0xFC80: /* ARABIC LIGATURE KAF WITH ALEF FINAL FORM */
    case 0xFC81: /* ARABIC LIGATURE KAF WITH LAM FINAL FORM */
    case 0xFC82: /* ARABIC LIGATURE KAF WITH MEEM FINAL FORM */
    case 0xFC83: /* ARABIC LIGATURE KAF WITH ALEF MAKSURA FINAL FORM */
    case 0xFC84: /* ARABIC LIGATURE KAF WITH YEH FINAL FORM */
    case 0xFC85: /* ARABIC LIGATURE LAM WITH MEEM FINAL FORM */
    case 0xFC86: /* ARABIC LIGATURE LAM WITH ALEF MAKSURA FINAL FORM */
    case 0xFC87: /* ARABIC LIGATURE LAM WITH YEH FINAL FORM */
    case 0xFC88: /* ARABIC LIGATURE MEEM WITH ALEF FINAL FORM */
    case 0xFC89: /* ARABIC LIGATURE MEEM WITH MEEM FINAL FORM */
    case 0xFC8A: /* ARABIC LIGATURE NOON WITH REH FINAL FORM */
    case 0xFC8B: /* ARABIC LIGATURE NOON WITH ZAIN FINAL FORM */
    case 0xFC8C: /* ARABIC LIGATURE NOON WITH MEEM FINAL FORM */
    case 0xFC8D: /* ARABIC LIGATURE NOON WITH NOON FINAL FORM */
    case 0xFC8E: /* ARABIC LIGATURE NOON WITH ALEF MAKSURA FINAL FORM */
    case 0xFC8F: /* ARABIC LIGATURE NOON WITH YEH FINAL FORM */
    case 0xFC90: /* ARABIC LIGATURE ALEF MAKSURA WITH SUPERSCRIPT ALEF FINAL FORM */
    case 0xFC91: /* ARABIC LIGATURE YEH WITH REH FINAL FORM */
    case 0xFC92: /* ARABIC LIGATURE YEH WITH ZAIN FINAL FORM */
    case 0xFC93: /* ARABIC LIGATURE YEH WITH MEEM FINAL FORM */
    case 0xFC94: /* ARABIC LIGATURE YEH WITH NOON FINAL FORM */
    case 0xFC95: /* ARABIC LIGATURE YEH WITH ALEF MAKSURA FINAL FORM */
    case 0xFC96: /* ARABIC LIGATURE YEH WITH YEH FINAL FORM */
    case 0xFC97: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH JEEM INITIAL FORM */
    case 0xFC98: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH HAH INITIAL FORM */
    case 0xFC99: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH KHAH INITIAL FORM */
    case 0xFC9A: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH MEEM INITIAL FORM */
    case 0xFC9B: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH HEH INITIAL FORM */
    case 0xFC9C: /* ARABIC LIGATURE BEH WITH JEEM INITIAL FORM */
    case 0xFC9D: /* ARABIC LIGATURE BEH WITH HAH INITIAL FORM */
    case 0xFC9E: /* ARABIC LIGATURE BEH WITH KHAH INITIAL FORM */
    case 0xFC9F: /* ARABIC LIGATURE BEH WITH MEEM INITIAL FORM */
    case 0xFCA0: /* ARABIC LIGATURE BEH WITH HEH INITIAL FORM */
    case 0xFCA1: /* ARABIC LIGATURE TEH WITH JEEM INITIAL FORM */
    case 0xFCA2: /* ARABIC LIGATURE TEH WITH HAH INITIAL FORM */
    case 0xFCA3: /* ARABIC LIGATURE TEH WITH KHAH INITIAL FORM */
    case 0xFCA4: /* ARABIC LIGATURE TEH WITH MEEM INITIAL FORM */
    case 0xFCA5: /* ARABIC LIGATURE TEH WITH HEH INITIAL FORM */
    case 0xFCA6: /* ARABIC LIGATURE THEH WITH MEEM INITIAL FORM */
    case 0xFCA7: /* ARABIC LIGATURE JEEM WITH HAH INITIAL FORM */
    case 0xFCA8: /* ARABIC LIGATURE JEEM WITH MEEM INITIAL FORM */
    case 0xFCA9: /* ARABIC LIGATURE HAH WITH JEEM INITIAL FORM */
    case 0xFCAA: /* ARABIC LIGATURE HAH WITH MEEM INITIAL FORM */
    case 0xFCAB: /* ARABIC LIGATURE KHAH WITH JEEM INITIAL FORM */
    case 0xFCAC: /* ARABIC LIGATURE KHAH WITH MEEM INITIAL FORM */
    case 0xFCAD: /* ARABIC LIGATURE SEEN WITH JEEM INITIAL FORM */
    case 0xFCAE: /* ARABIC LIGATURE SEEN WITH HAH INITIAL FORM */
    case 0xFCAF: /* ARABIC LIGATURE SEEN WITH KHAH INITIAL FORM */
    case 0xFCB0: /* ARABIC LIGATURE SEEN WITH MEEM INITIAL FORM */
    case 0xFCB1: /* ARABIC LIGATURE SAD WITH HAH INITIAL FORM */
    case 0xFCB2: /* ARABIC LIGATURE SAD WITH KHAH INITIAL FORM */
    case 0xFCB3: /* ARABIC LIGATURE SAD WITH MEEM INITIAL FORM */
    case 0xFCB4: /* ARABIC LIGATURE DAD WITH JEEM INITIAL FORM */
    case 0xFCB5: /* ARABIC LIGATURE DAD WITH HAH INITIAL FORM */
    case 0xFCB6: /* ARABIC LIGATURE DAD WITH KHAH INITIAL FORM */
    case 0xFCB7: /* ARABIC LIGATURE DAD WITH MEEM INITIAL FORM */
    case 0xFCB8: /* ARABIC LIGATURE TAH WITH HAH INITIAL FORM */
    case 0xFCB9: /* ARABIC LIGATURE ZAH WITH MEEM INITIAL FORM */
    case 0xFCBA: /* ARABIC LIGATURE AIN WITH JEEM INITIAL FORM */
    case 0xFCBB: /* ARABIC LIGATURE AIN WITH MEEM INITIAL FORM */
    case 0xFCBC: /* ARABIC LIGATURE GHAIN WITH JEEM INITIAL FORM */
    case 0xFCBD: /* ARABIC LIGATURE GHAIN WITH MEEM INITIAL FORM */
    case 0xFCBE: /* ARABIC LIGATURE FEH WITH JEEM INITIAL FORM */
    case 0xFCBF: /* ARABIC LIGATURE FEH WITH HAH INITIAL FORM */
    case 0xFCC0: /* ARABIC LIGATURE FEH WITH KHAH INITIAL FORM */
    case 0xFCC1: /* ARABIC LIGATURE FEH WITH MEEM INITIAL FORM */
    case 0xFCC2: /* ARABIC LIGATURE QAF WITH HAH INITIAL FORM */
    case 0xFCC3: /* ARABIC LIGATURE QAF WITH MEEM INITIAL FORM */
    case 0xFCC4: /* ARABIC LIGATURE KAF WITH JEEM INITIAL FORM */
    case 0xFCC5: /* ARABIC LIGATURE KAF WITH HAH INITIAL FORM */
    case 0xFCC6: /* ARABIC LIGATURE KAF WITH KHAH INITIAL FORM */
    case 0xFCC7: /* ARABIC LIGATURE KAF WITH LAM INITIAL FORM */
    case 0xFCC8: /* ARABIC LIGATURE KAF WITH MEEM INITIAL FORM */
    case 0xFCC9: /* ARABIC LIGATURE LAM WITH JEEM INITIAL FORM */
    case 0xFCCA: /* ARABIC LIGATURE LAM WITH HAH INITIAL FORM */
    case 0xFCCB: /* ARABIC LIGATURE LAM WITH KHAH INITIAL FORM */
    case 0xFCCC: /* ARABIC LIGATURE LAM WITH MEEM INITIAL FORM */
    case 0xFCCD: /* ARABIC LIGATURE LAM WITH HEH INITIAL FORM */
    case 0xFCCE: /* ARABIC LIGATURE MEEM WITH JEEM INITIAL FORM */
    case 0xFCCF: /* ARABIC LIGATURE MEEM WITH HAH INITIAL FORM */
    case 0xFCD0: /* ARABIC LIGATURE MEEM WITH KHAH INITIAL FORM */
    case 0xFCD1: /* ARABIC LIGATURE MEEM WITH MEEM INITIAL FORM */
    case 0xFCD2: /* ARABIC LIGATURE NOON WITH JEEM INITIAL FORM */
    case 0xFCD3: /* ARABIC LIGATURE NOON WITH HAH INITIAL FORM */
    case 0xFCD4: /* ARABIC LIGATURE NOON WITH KHAH INITIAL FORM */
    case 0xFCD5: /* ARABIC LIGATURE NOON WITH MEEM INITIAL FORM */
    case 0xFCD6: /* ARABIC LIGATURE NOON WITH HEH INITIAL FORM */
    case 0xFCD7: /* ARABIC LIGATURE HEH WITH JEEM INITIAL FORM */
    case 0xFCD8: /* ARABIC LIGATURE HEH WITH MEEM INITIAL FORM */
    case 0xFCD9: /* ARABIC LIGATURE HEH WITH SUPERSCRIPT ALEF INITIAL FORM */
    case 0xFCDA: /* ARABIC LIGATURE YEH WITH JEEM INITIAL FORM */
    case 0xFCDB: /* ARABIC LIGATURE YEH WITH HAH INITIAL FORM */
    case 0xFCDC: /* ARABIC LIGATURE YEH WITH KHAH INITIAL FORM */
    case 0xFCDD: /* ARABIC LIGATURE YEH WITH MEEM INITIAL FORM */
    case 0xFCDE: /* ARABIC LIGATURE YEH WITH HEH INITIAL FORM */
    case 0xFCDF: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH MEEM MEDIAL FORM */
    case 0xFCE0: /* ARABIC LIGATURE YEH WITH HAMZA ABOVE WITH HEH MEDIAL FORM */
    case 0xFCE1: /* ARABIC LIGATURE BEH WITH MEEM MEDIAL FORM */
    case 0xFCE2: /* ARABIC LIGATURE BEH WITH HEH MEDIAL FORM */
    case 0xFCE3: /* ARABIC LIGATURE TEH WITH MEEM MEDIAL FORM */
    case 0xFCE4: /* ARABIC LIGATURE TEH WITH HEH MEDIAL FORM */
    case 0xFCE5: /* ARABIC LIGATURE THEH WITH MEEM MEDIAL FORM */
    case 0xFCE6: /* ARABIC LIGATURE THEH WITH HEH MEDIAL FORM */
    case 0xFCE7: /* ARABIC LIGATURE SEEN WITH MEEM MEDIAL FORM */
    case 0xFCE8: /* ARABIC LIGATURE SEEN WITH HEH MEDIAL FORM */
    case 0xFCE9: /* ARABIC LIGATURE SHEEN WITH MEEM MEDIAL FORM */
    case 0xFCEA: /* ARABIC LIGATURE SHEEN WITH HEH MEDIAL FORM */
    case 0xFCEB: /* ARABIC LIGATURE KAF WITH LAM MEDIAL FORM */
    case 0xFCEC: /* ARABIC LIGATURE KAF WITH MEEM MEDIAL FORM */
    case 0xFCED: /* ARABIC LIGATURE LAM WITH MEEM MEDIAL FORM */
    case 0xFCEE: /* ARABIC LIGATURE NOON WITH MEEM MEDIAL FORM */
    case 0xFCEF: /* ARABIC LIGATURE NOON WITH HEH MEDIAL FORM */
    case 0xFCF0: /* ARABIC LIGATURE YEH WITH MEEM MEDIAL FORM */
    case 0xFCF1: /* ARABIC LIGATURE YEH WITH HEH MEDIAL FORM */
    case 0xFCF2: /* ARABIC LIGATURE SHADDA WITH FATHA MEDIAL FORM */
    case 0xFCF3: /* ARABIC LIGATURE SHADDA WITH DAMMA MEDIAL FORM */
    case 0xFCF4: /* ARABIC LIGATURE SHADDA WITH KASRA MEDIAL FORM */
    case 0xFCF5: /* ARABIC LIGATURE TAH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFCF6: /* ARABIC LIGATURE TAH WITH YEH ISOLATED FORM */
    case 0xFCF7: /* ARABIC LIGATURE AIN WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFCF8: /* ARABIC LIGATURE AIN WITH YEH ISOLATED FORM */
    case 0xFCF9: /* ARABIC LIGATURE GHAIN WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFCFA: /* ARABIC LIGATURE GHAIN WITH YEH ISOLATED FORM */
    case 0xFCFB: /* ARABIC LIGATURE SEEN WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFCFC: /* ARABIC LIGATURE SEEN WITH YEH ISOLATED FORM */
    case 0xFCFD: /* ARABIC LIGATURE SHEEN WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFCFE: /* ARABIC LIGATURE SHEEN WITH YEH ISOLATED FORM */
    case 0xFCFF: /* ARABIC LIGATURE HAH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFD00: /* ARABIC LIGATURE HAH WITH YEH ISOLATED FORM */
    case 0xFD01: /* ARABIC LIGATURE JEEM WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFD02: /* ARABIC LIGATURE JEEM WITH YEH ISOLATED FORM */
    case 0xFD03: /* ARABIC LIGATURE KHAH WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFD04: /* ARABIC LIGATURE KHAH WITH YEH ISOLATED FORM */
    case 0xFD05: /* ARABIC LIGATURE SAD WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFD06: /* ARABIC LIGATURE SAD WITH YEH ISOLATED FORM */
    case 0xFD07: /* ARABIC LIGATURE DAD WITH ALEF MAKSURA ISOLATED FORM */
    case 0xFD08: /* ARABIC LIGATURE DAD WITH YEH ISOLATED FORM */
    case 0xFD09: /* ARABIC LIGATURE SHEEN WITH JEEM ISOLATED FORM */
    case 0xFD0A: /* ARABIC LIGATURE SHEEN WITH HAH ISOLATED FORM */
    case 0xFD0B: /* ARABIC LIGATURE SHEEN WITH KHAH ISOLATED FORM */
    case 0xFD0C: /* ARABIC LIGATURE SHEEN WITH MEEM ISOLATED FORM */
    case 0xFD0D: /* ARABIC LIGATURE SHEEN WITH REH ISOLATED FORM */
    case 0xFD0E: /* ARABIC LIGATURE SEEN WITH REH ISOLATED FORM */
    case 0xFD0F: /* ARABIC LIGATURE SAD WITH REH ISOLATED FORM */
    case 0xFD10: /* ARABIC LIGATURE DAD WITH REH ISOLATED FORM */
    case 0xFD11: /* ARABIC LIGATURE TAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD12: /* ARABIC LIGATURE TAH WITH YEH FINAL FORM */
    case 0xFD13: /* ARABIC LIGATURE AIN WITH ALEF MAKSURA FINAL FORM */
    case 0xFD14: /* ARABIC LIGATURE AIN WITH YEH FINAL FORM */
    case 0xFD15: /* ARABIC LIGATURE GHAIN WITH ALEF MAKSURA FINAL FORM */
    case 0xFD16: /* ARABIC LIGATURE GHAIN WITH YEH FINAL FORM */
    case 0xFD17: /* ARABIC LIGATURE SEEN WITH ALEF MAKSURA FINAL FORM */
    case 0xFD18: /* ARABIC LIGATURE SEEN WITH YEH FINAL FORM */
    case 0xFD19: /* ARABIC LIGATURE SHEEN WITH ALEF MAKSURA FINAL FORM */
    case 0xFD1A: /* ARABIC LIGATURE SHEEN WITH YEH FINAL FORM */
    case 0xFD1B: /* ARABIC LIGATURE HAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD1C: /* ARABIC LIGATURE HAH WITH YEH FINAL FORM */
    case 0xFD1D: /* ARABIC LIGATURE JEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD1E: /* ARABIC LIGATURE JEEM WITH YEH FINAL FORM */
    case 0xFD1F: /* ARABIC LIGATURE KHAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD20: /* ARABIC LIGATURE KHAH WITH YEH FINAL FORM */
    case 0xFD21: /* ARABIC LIGATURE SAD WITH ALEF MAKSURA FINAL FORM */
    case 0xFD22: /* ARABIC LIGATURE SAD WITH YEH FINAL FORM */
    case 0xFD23: /* ARABIC LIGATURE DAD WITH ALEF MAKSURA FINAL FORM */
    case 0xFD24: /* ARABIC LIGATURE DAD WITH YEH FINAL FORM */
    case 0xFD25: /* ARABIC LIGATURE SHEEN WITH JEEM FINAL FORM */
    case 0xFD26: /* ARABIC LIGATURE SHEEN WITH HAH FINAL FORM */
    case 0xFD27: /* ARABIC LIGATURE SHEEN WITH KHAH FINAL FORM */
    case 0xFD28: /* ARABIC LIGATURE SHEEN WITH MEEM FINAL FORM */
    case 0xFD29: /* ARABIC LIGATURE SHEEN WITH REH FINAL FORM */
    case 0xFD2A: /* ARABIC LIGATURE SEEN WITH REH FINAL FORM */
    case 0xFD2B: /* ARABIC LIGATURE SAD WITH REH FINAL FORM */
    case 0xFD2C: /* ARABIC LIGATURE DAD WITH REH FINAL FORM */
    case 0xFD2D: /* ARABIC LIGATURE SHEEN WITH JEEM INITIAL FORM */
    case 0xFD2E: /* ARABIC LIGATURE SHEEN WITH HAH INITIAL FORM */
    case 0xFD2F: /* ARABIC LIGATURE SHEEN WITH KHAH INITIAL FORM */
    case 0xFD30: /* ARABIC LIGATURE SHEEN WITH MEEM INITIAL FORM */
    case 0xFD31: /* ARABIC LIGATURE SEEN WITH HEH INITIAL FORM */
    case 0xFD32: /* ARABIC LIGATURE SHEEN WITH HEH INITIAL FORM */
    case 0xFD33: /* ARABIC LIGATURE TAH WITH MEEM INITIAL FORM */
    case 0xFD34: /* ARABIC LIGATURE SEEN WITH JEEM MEDIAL FORM */
    case 0xFD35: /* ARABIC LIGATURE SEEN WITH HAH MEDIAL FORM */
    case 0xFD36: /* ARABIC LIGATURE SEEN WITH KHAH MEDIAL FORM */
    case 0xFD37: /* ARABIC LIGATURE SHEEN WITH JEEM MEDIAL FORM */
    case 0xFD38: /* ARABIC LIGATURE SHEEN WITH HAH MEDIAL FORM */
    case 0xFD39: /* ARABIC LIGATURE SHEEN WITH KHAH MEDIAL FORM */
    case 0xFD3A: /* ARABIC LIGATURE TAH WITH MEEM MEDIAL FORM */
    case 0xFD3B: /* ARABIC LIGATURE ZAH WITH MEEM MEDIAL FORM */
    case 0xFD3C: /* ARABIC LIGATURE ALEF WITH FATHATAN FINAL FORM */
    case 0xFD3D: /* ARABIC LIGATURE ALEF WITH FATHATAN ISOLATED FORM */
    case 0xFD50: /* ARABIC LIGATURE TEH WITH JEEM WITH MEEM INITIAL FORM */
    case 0xFD51: /* ARABIC LIGATURE TEH WITH HAH WITH JEEM FINAL FORM */
    case 0xFD52: /* ARABIC LIGATURE TEH WITH HAH WITH JEEM INITIAL FORM */
    case 0xFD53: /* ARABIC LIGATURE TEH WITH HAH WITH MEEM INITIAL FORM */
    case 0xFD54: /* ARABIC LIGATURE TEH WITH KHAH WITH MEEM INITIAL FORM */
    case 0xFD55: /* ARABIC LIGATURE TEH WITH MEEM WITH JEEM INITIAL FORM */
    case 0xFD56: /* ARABIC LIGATURE TEH WITH MEEM WITH HAH INITIAL FORM */
    case 0xFD57: /* ARABIC LIGATURE TEH WITH MEEM WITH KHAH INITIAL FORM */
    case 0xFD58: /* ARABIC LIGATURE JEEM WITH MEEM WITH HAH FINAL FORM */
    case 0xFD59: /* ARABIC LIGATURE JEEM WITH MEEM WITH HAH INITIAL FORM */
    case 0xFD5A: /* ARABIC LIGATURE HAH WITH MEEM WITH YEH FINAL FORM */
    case 0xFD5B: /* ARABIC LIGATURE HAH WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD5C: /* ARABIC LIGATURE SEEN WITH HAH WITH JEEM INITIAL FORM */
    case 0xFD5D: /* ARABIC LIGATURE SEEN WITH JEEM WITH HAH INITIAL FORM */
    case 0xFD5E: /* ARABIC LIGATURE SEEN WITH JEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD5F: /* ARABIC LIGATURE SEEN WITH MEEM WITH HAH FINAL FORM */
    case 0xFD60: /* ARABIC LIGATURE SEEN WITH MEEM WITH HAH INITIAL FORM */
    case 0xFD61: /* ARABIC LIGATURE SEEN WITH MEEM WITH JEEM INITIAL FORM */
    case 0xFD62: /* ARABIC LIGATURE SEEN WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD63: /* ARABIC LIGATURE SEEN WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD64: /* ARABIC LIGATURE SAD WITH HAH WITH HAH FINAL FORM */
    case 0xFD65: /* ARABIC LIGATURE SAD WITH HAH WITH HAH INITIAL FORM */
    case 0xFD66: /* ARABIC LIGATURE SAD WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD67: /* ARABIC LIGATURE SHEEN WITH HAH WITH MEEM FINAL FORM */
    case 0xFD68: /* ARABIC LIGATURE SHEEN WITH HAH WITH MEEM INITIAL FORM */
    case 0xFD69: /* ARABIC LIGATURE SHEEN WITH JEEM WITH YEH FINAL FORM */
    case 0xFD6A: /* ARABIC LIGATURE SHEEN WITH MEEM WITH KHAH FINAL FORM */
    case 0xFD6B: /* ARABIC LIGATURE SHEEN WITH MEEM WITH KHAH INITIAL FORM */
    case 0xFD6C: /* ARABIC LIGATURE SHEEN WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD6D: /* ARABIC LIGATURE SHEEN WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD6E: /* ARABIC LIGATURE DAD WITH HAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD6F: /* ARABIC LIGATURE DAD WITH KHAH WITH MEEM FINAL FORM */
    case 0xFD70: /* ARABIC LIGATURE DAD WITH KHAH WITH MEEM INITIAL FORM */
    case 0xFD71: /* ARABIC LIGATURE TAH WITH MEEM WITH HAH FINAL FORM */
    case 0xFD72: /* ARABIC LIGATURE TAH WITH MEEM WITH HAH INITIAL FORM */
    case 0xFD73: /* ARABIC LIGATURE TAH WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD74: /* ARABIC LIGATURE TAH WITH MEEM WITH YEH FINAL FORM */
    case 0xFD75: /* ARABIC LIGATURE AIN WITH JEEM WITH MEEM FINAL FORM */
    case 0xFD76: /* ARABIC LIGATURE AIN WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD77: /* ARABIC LIGATURE AIN WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD78: /* ARABIC LIGATURE AIN WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD79: /* ARABIC LIGATURE GHAIN WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD7A: /* ARABIC LIGATURE GHAIN WITH MEEM WITH YEH FINAL FORM */
    case 0xFD7B: /* ARABIC LIGATURE GHAIN WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD7C: /* ARABIC LIGATURE FEH WITH KHAH WITH MEEM FINAL FORM */
    case 0xFD7D: /* ARABIC LIGATURE FEH WITH KHAH WITH MEEM INITIAL FORM */
    case 0xFD7E: /* ARABIC LIGATURE QAF WITH MEEM WITH HAH FINAL FORM */
    case 0xFD7F: /* ARABIC LIGATURE QAF WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD80: /* ARABIC LIGATURE LAM WITH HAH WITH MEEM FINAL FORM */
    case 0xFD81: /* ARABIC LIGATURE LAM WITH HAH WITH YEH FINAL FORM */
    case 0xFD82: /* ARABIC LIGATURE LAM WITH HAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD83: /* ARABIC LIGATURE LAM WITH JEEM WITH JEEM INITIAL FORM */
    case 0xFD84: /* ARABIC LIGATURE LAM WITH JEEM WITH JEEM FINAL FORM */
    case 0xFD85: /* ARABIC LIGATURE LAM WITH KHAH WITH MEEM FINAL FORM */
    case 0xFD86: /* ARABIC LIGATURE LAM WITH KHAH WITH MEEM INITIAL FORM */
    case 0xFD87: /* ARABIC LIGATURE LAM WITH MEEM WITH HAH FINAL FORM */
    case 0xFD88: /* ARABIC LIGATURE LAM WITH MEEM WITH HAH INITIAL FORM */
    case 0xFD89: /* ARABIC LIGATURE MEEM WITH HAH WITH JEEM INITIAL FORM */
    case 0xFD8A: /* ARABIC LIGATURE MEEM WITH HAH WITH MEEM INITIAL FORM */
    case 0xFD8B: /* ARABIC LIGATURE MEEM WITH HAH WITH YEH FINAL FORM */
    case 0xFD8C: /* ARABIC LIGATURE MEEM WITH JEEM WITH HAH INITIAL FORM */
    case 0xFD8D: /* ARABIC LIGATURE MEEM WITH JEEM WITH MEEM INITIAL FORM */
    case 0xFD8E: /* ARABIC LIGATURE MEEM WITH KHAH WITH JEEM INITIAL FORM */
    case 0xFD8F: /* ARABIC LIGATURE MEEM WITH KHAH WITH MEEM INITIAL FORM */
    case 0xFD92: /* ARABIC LIGATURE MEEM WITH JEEM WITH KHAH INITIAL FORM */
    case 0xFD93: /* ARABIC LIGATURE HEH WITH MEEM WITH JEEM INITIAL FORM */
    case 0xFD94: /* ARABIC LIGATURE HEH WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD95: /* ARABIC LIGATURE NOON WITH HAH WITH MEEM INITIAL FORM */
BREAK_SWITCH_UP
    case 0xFD96: /* ARABIC LIGATURE NOON WITH HAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFD97: /* ARABIC LIGATURE NOON WITH JEEM WITH MEEM FINAL FORM */
    case 0xFD98: /* ARABIC LIGATURE NOON WITH JEEM WITH MEEM INITIAL FORM */
    case 0xFD99: /* ARABIC LIGATURE NOON WITH JEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD9A: /* ARABIC LIGATURE NOON WITH MEEM WITH YEH FINAL FORM */
    case 0xFD9B: /* ARABIC LIGATURE NOON WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFD9C: /* ARABIC LIGATURE YEH WITH MEEM WITH MEEM FINAL FORM */
    case 0xFD9D: /* ARABIC LIGATURE YEH WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFD9E: /* ARABIC LIGATURE BEH WITH KHAH WITH YEH FINAL FORM */
    case 0xFD9F: /* ARABIC LIGATURE TEH WITH JEEM WITH YEH FINAL FORM */
    case 0xFDA0: /* ARABIC LIGATURE TEH WITH JEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA1: /* ARABIC LIGATURE TEH WITH KHAH WITH YEH FINAL FORM */
    case 0xFDA2: /* ARABIC LIGATURE TEH WITH KHAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA3: /* ARABIC LIGATURE TEH WITH MEEM WITH YEH FINAL FORM */
    case 0xFDA4: /* ARABIC LIGATURE TEH WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA5: /* ARABIC LIGATURE JEEM WITH MEEM WITH YEH FINAL FORM */
    case 0xFDA6: /* ARABIC LIGATURE JEEM WITH HAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA7: /* ARABIC LIGATURE JEEM WITH MEEM WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA8: /* ARABIC LIGATURE SEEN WITH KHAH WITH ALEF MAKSURA FINAL FORM */
    case 0xFDA9: /* ARABIC LIGATURE SAD WITH HAH WITH YEH FINAL FORM */
    case 0xFDAA: /* ARABIC LIGATURE SHEEN WITH HAH WITH YEH FINAL FORM */
    case 0xFDAB: /* ARABIC LIGATURE DAD WITH HAH WITH YEH FINAL FORM */
    case 0xFDAC: /* ARABIC LIGATURE LAM WITH JEEM WITH YEH FINAL FORM */
    case 0xFDAD: /* ARABIC LIGATURE LAM WITH MEEM WITH YEH FINAL FORM */
    case 0xFDAE: /* ARABIC LIGATURE YEH WITH HAH WITH YEH FINAL FORM */
    case 0xFDAF: /* ARABIC LIGATURE YEH WITH JEEM WITH YEH FINAL FORM */
    case 0xFDB0: /* ARABIC LIGATURE YEH WITH MEEM WITH YEH FINAL FORM */
    case 0xFDB1: /* ARABIC LIGATURE MEEM WITH MEEM WITH YEH FINAL FORM */
    case 0xFDB2: /* ARABIC LIGATURE QAF WITH MEEM WITH YEH FINAL FORM */
    case 0xFDB3: /* ARABIC LIGATURE NOON WITH HAH WITH YEH FINAL FORM */
    case 0xFDB4: /* ARABIC LIGATURE QAF WITH MEEM WITH HAH INITIAL FORM */
    case 0xFDB5: /* ARABIC LIGATURE LAM WITH HAH WITH MEEM INITIAL FORM */
    case 0xFDB6: /* ARABIC LIGATURE AIN WITH MEEM WITH YEH FINAL FORM */
    case 0xFDB7: /* ARABIC LIGATURE KAF WITH MEEM WITH YEH FINAL FORM */
    case 0xFDB8: /* ARABIC LIGATURE NOON WITH JEEM WITH HAH INITIAL FORM */
    case 0xFDB9: /* ARABIC LIGATURE MEEM WITH KHAH WITH YEH FINAL FORM */
    case 0xFDBA: /* ARABIC LIGATURE LAM WITH JEEM WITH MEEM INITIAL FORM */
    case 0xFDBB: /* ARABIC LIGATURE KAF WITH MEEM WITH MEEM FINAL FORM */
    case 0xFDBC: /* ARABIC LIGATURE LAM WITH JEEM WITH MEEM FINAL FORM */
    case 0xFDBD: /* ARABIC LIGATURE NOON WITH JEEM WITH HAH FINAL FORM */
    case 0xFDBE: /* ARABIC LIGATURE JEEM WITH HAH WITH YEH FINAL FORM */
    case 0xFDBF: /* ARABIC LIGATURE HAH WITH JEEM WITH YEH FINAL FORM */
    case 0xFDC0: /* ARABIC LIGATURE MEEM WITH JEEM WITH YEH FINAL FORM */
    case 0xFDC1: /* ARABIC LIGATURE FEH WITH MEEM WITH YEH FINAL FORM */
    case 0xFDC2: /* ARABIC LIGATURE BEH WITH HAH WITH YEH FINAL FORM */
    case 0xFDC3: /* ARABIC LIGATURE KAF WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFDC4: /* ARABIC LIGATURE AIN WITH JEEM WITH MEEM INITIAL FORM */
    case 0xFDC5: /* ARABIC LIGATURE SAD WITH MEEM WITH MEEM INITIAL FORM */
    case 0xFDC6: /* ARABIC LIGATURE SEEN WITH KHAH WITH YEH FINAL FORM */
    case 0xFDC7: /* ARABIC LIGATURE NOON WITH JEEM WITH YEH FINAL FORM */
    case 0xFDF0: /* ARABIC LIGATURE SALLA USED AS KORANIC STOP SIGN ISOLATED FORM */
    case 0xFDF1: /* ARABIC LIGATURE QALA USED AS KORANIC STOP SIGN ISOLATED FORM */
    case 0xFDF2: /* ARABIC LIGATURE ALLAH ISOLATED FORM */
    case 0xFDF3: /* ARABIC LIGATURE AKBAR ISOLATED FORM */
    case 0xFDF4: /* ARABIC LIGATURE MOHAMMAD ISOLATED FORM */
    case 0xFDF5: /* ARABIC LIGATURE SALAM ISOLATED FORM */
    case 0xFDF6: /* ARABIC LIGATURE RASOUL ISOLATED FORM */
    case 0xFDF7: /* ARABIC LIGATURE ALAYHE ISOLATED FORM */
    case 0xFDF8: /* ARABIC LIGATURE WASALLAM ISOLATED FORM */
    case 0xFDF9: /* ARABIC LIGATURE SALLA ISOLATED FORM */
    case 0xFDFA: /* ARABIC LIGATURE SALLALLAHOU ALAYHE WASALLAM */
    case 0xFDFB: /* ARABIC LIGATURE JALLAJALALOUHOU */
    case 0xFE70: /* ARABIC FATHATAN ISOLATED FORM */
    case 0xFE71: /* ARABIC TATWEEL WITH FATHATAN ABOVE */
    case 0xFE72: /* ARABIC DAMMATAN ISOLATED FORM */
    case 0xFE74: /* ARABIC KASRATAN ISOLATED FORM */
    case 0xFE76: /* ARABIC FATHA ISOLATED FORM */
    case 0xFE77: /* ARABIC FATHA MEDIAL FORM */
    case 0xFE78: /* ARABIC DAMMA ISOLATED FORM */
    case 0xFE79: /* ARABIC DAMMA MEDIAL FORM */
    case 0xFE7A: /* ARABIC KASRA ISOLATED FORM */
    case 0xFE7B: /* ARABIC KASRA MEDIAL FORM */
    case 0xFE7C: /* ARABIC SHADDA ISOLATED FORM */
    case 0xFE7D: /* ARABIC SHADDA MEDIAL FORM */
    case 0xFE7E: /* ARABIC SUKUN ISOLATED FORM */
    case 0xFE7F: /* ARABIC SUKUN MEDIAL FORM */
    case 0xFE80: /* ARABIC LETTER HAMZA ISOLATED FORM */
    case 0xFE81: /* ARABIC LETTER ALEF WITH MADDA ABOVE ISOLATED FORM */
    case 0xFE82: /* ARABIC LETTER ALEF WITH MADDA ABOVE FINAL FORM */
    case 0xFE83: /* ARABIC LETTER ALEF WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFE84: /* ARABIC LETTER ALEF WITH HAMZA ABOVE FINAL FORM */
    case 0xFE85: /* ARABIC LETTER WAW WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFE86: /* ARABIC LETTER WAW WITH HAMZA ABOVE FINAL FORM */
    case 0xFE87: /* ARABIC LETTER ALEF WITH HAMZA BELOW ISOLATED FORM */
    case 0xFE88: /* ARABIC LETTER ALEF WITH HAMZA BELOW FINAL FORM */
    case 0xFE89: /* ARABIC LETTER YEH WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFE8A: /* ARABIC LETTER YEH WITH HAMZA ABOVE FINAL FORM */
    case 0xFE8B: /* ARABIC LETTER YEH WITH HAMZA ABOVE INITIAL FORM */
    case 0xFE8C: /* ARABIC LETTER YEH WITH HAMZA ABOVE MEDIAL FORM */
    case 0xFE8D: /* ARABIC LETTER ALEF ISOLATED FORM */
    case 0xFE8E: /* ARABIC LETTER ALEF FINAL FORM */
    case 0xFE8F: /* ARABIC LETTER BEH ISOLATED FORM */
    case 0xFE90: /* ARABIC LETTER BEH FINAL FORM */
    case 0xFE91: /* ARABIC LETTER BEH INITIAL FORM */
    case 0xFE92: /* ARABIC LETTER BEH MEDIAL FORM */
    case 0xFE93: /* ARABIC LETTER TEH MARBUTA ISOLATED FORM */
    case 0xFE94: /* ARABIC LETTER TEH MARBUTA FINAL FORM */
    case 0xFE95: /* ARABIC LETTER TEH ISOLATED FORM */
    case 0xFE96: /* ARABIC LETTER TEH FINAL FORM */
    case 0xFE97: /* ARABIC LETTER TEH INITIAL FORM */
    case 0xFE98: /* ARABIC LETTER TEH MEDIAL FORM */
    case 0xFE99: /* ARABIC LETTER THEH ISOLATED FORM */
    case 0xFE9A: /* ARABIC LETTER THEH FINAL FORM */
    case 0xFE9B: /* ARABIC LETTER THEH INITIAL FORM */
    case 0xFE9C: /* ARABIC LETTER THEH MEDIAL FORM */
    case 0xFE9D: /* ARABIC LETTER JEEM ISOLATED FORM */
    case 0xFE9E: /* ARABIC LETTER JEEM FINAL FORM */
    case 0xFE9F: /* ARABIC LETTER JEEM INITIAL FORM */
    case 0xFEA0: /* ARABIC LETTER JEEM MEDIAL FORM */
    case 0xFEA1: /* ARABIC LETTER HAH ISOLATED FORM */
    case 0xFEA2: /* ARABIC LETTER HAH FINAL FORM */
    case 0xFEA3: /* ARABIC LETTER HAH INITIAL FORM */
    case 0xFEA4: /* ARABIC LETTER HAH MEDIAL FORM */
    case 0xFEA5: /* ARABIC LETTER KHAH ISOLATED FORM */
    case 0xFEA6: /* ARABIC LETTER KHAH FINAL FORM */
    case 0xFEA7: /* ARABIC LETTER KHAH INITIAL FORM */
    case 0xFEA8: /* ARABIC LETTER KHAH MEDIAL FORM */
    case 0xFEA9: /* ARABIC LETTER DAL ISOLATED FORM */
    case 0xFEAA: /* ARABIC LETTER DAL FINAL FORM */
    case 0xFEAB: /* ARABIC LETTER THAL ISOLATED FORM */
    case 0xFEAC: /* ARABIC LETTER THAL FINAL FORM */
    case 0xFEAD: /* ARABIC LETTER REH ISOLATED FORM */
    case 0xFEAE: /* ARABIC LETTER REH FINAL FORM */
    case 0xFEAF: /* ARABIC LETTER ZAIN ISOLATED FORM */
    case 0xFEB0: /* ARABIC LETTER ZAIN FINAL FORM */
    case 0xFEB1: /* ARABIC LETTER SEEN ISOLATED FORM */
    case 0xFEB2: /* ARABIC LETTER SEEN FINAL FORM */
    case 0xFEB3: /* ARABIC LETTER SEEN INITIAL FORM */
    case 0xFEB4: /* ARABIC LETTER SEEN MEDIAL FORM */
    case 0xFEB5: /* ARABIC LETTER SHEEN ISOLATED FORM */
    case 0xFEB6: /* ARABIC LETTER SHEEN FINAL FORM */
    case 0xFEB7: /* ARABIC LETTER SHEEN INITIAL FORM */
    case 0xFEB8: /* ARABIC LETTER SHEEN MEDIAL FORM */
    case 0xFEB9: /* ARABIC LETTER SAD ISOLATED FORM */
    case 0xFEBA: /* ARABIC LETTER SAD FINAL FORM */
    case 0xFEBB: /* ARABIC LETTER SAD INITIAL FORM */
    case 0xFEBC: /* ARABIC LETTER SAD MEDIAL FORM */
    case 0xFEBD: /* ARABIC LETTER DAD ISOLATED FORM */
    case 0xFEBE: /* ARABIC LETTER DAD FINAL FORM */
    case 0xFEBF: /* ARABIC LETTER DAD INITIAL FORM */
    case 0xFEC0: /* ARABIC LETTER DAD MEDIAL FORM */
    case 0xFEC1: /* ARABIC LETTER TAH ISOLATED FORM */
    case 0xFEC2: /* ARABIC LETTER TAH FINAL FORM */
    case 0xFEC3: /* ARABIC LETTER TAH INITIAL FORM */
    case 0xFEC4: /* ARABIC LETTER TAH MEDIAL FORM */
    case 0xFEC5: /* ARABIC LETTER ZAH ISOLATED FORM */
    case 0xFEC6: /* ARABIC LETTER ZAH FINAL FORM */
    case 0xFEC7: /* ARABIC LETTER ZAH INITIAL FORM */
    case 0xFEC8: /* ARABIC LETTER ZAH MEDIAL FORM */
    case 0xFEC9: /* ARABIC LETTER AIN ISOLATED FORM */
    case 0xFECA: /* ARABIC LETTER AIN FINAL FORM */
    case 0xFECB: /* ARABIC LETTER AIN INITIAL FORM */
    case 0xFECC: /* ARABIC LETTER AIN MEDIAL FORM */
    case 0xFECD: /* ARABIC LETTER GHAIN ISOLATED FORM */
    case 0xFECE: /* ARABIC LETTER GHAIN FINAL FORM */
    case 0xFECF: /* ARABIC LETTER GHAIN INITIAL FORM */
    case 0xFED0: /* ARABIC LETTER GHAIN MEDIAL FORM */
    case 0xFED1: /* ARABIC LETTER FEH ISOLATED FORM */
    case 0xFED2: /* ARABIC LETTER FEH FINAL FORM */
    case 0xFED3: /* ARABIC LETTER FEH INITIAL FORM */
    case 0xFED4: /* ARABIC LETTER FEH MEDIAL FORM */
    case 0xFED5: /* ARABIC LETTER QAF ISOLATED FORM */
    case 0xFED6: /* ARABIC LETTER QAF FINAL FORM */
    case 0xFED7: /* ARABIC LETTER QAF INITIAL FORM */
    case 0xFED8: /* ARABIC LETTER QAF MEDIAL FORM */
    case 0xFED9: /* ARABIC LETTER KAF ISOLATED FORM */
    case 0xFEDA: /* ARABIC LETTER KAF FINAL FORM */
    case 0xFEDB: /* ARABIC LETTER KAF INITIAL FORM */
    case 0xFEDC: /* ARABIC LETTER KAF MEDIAL FORM */
    case 0xFEDD: /* ARABIC LETTER LAM ISOLATED FORM */
    case 0xFEDE: /* ARABIC LETTER LAM FINAL FORM */
    case 0xFEDF: /* ARABIC LETTER LAM INITIAL FORM */
    case 0xFEE0: /* ARABIC LETTER LAM MEDIAL FORM */
    case 0xFEE1: /* ARABIC LETTER MEEM ISOLATED FORM */
    case 0xFEE2: /* ARABIC LETTER MEEM FINAL FORM */
    case 0xFEE3: /* ARABIC LETTER MEEM INITIAL FORM */
    case 0xFEE4: /* ARABIC LETTER MEEM MEDIAL FORM */
    case 0xFEE5: /* ARABIC LETTER NOON ISOLATED FORM */
    case 0xFEE6: /* ARABIC LETTER NOON FINAL FORM */
    case 0xFEE7: /* ARABIC LETTER NOON INITIAL FORM */
    case 0xFEE8: /* ARABIC LETTER NOON MEDIAL FORM */
    case 0xFEE9: /* ARABIC LETTER HEH ISOLATED FORM */
    case 0xFEEA: /* ARABIC LETTER HEH FINAL FORM */
    case 0xFEEB: /* ARABIC LETTER HEH INITIAL FORM */
    case 0xFEEC: /* ARABIC LETTER HEH MEDIAL FORM */
    case 0xFEED: /* ARABIC LETTER WAW ISOLATED FORM */
    case 0xFEEE: /* ARABIC LETTER WAW FINAL FORM */
    case 0xFEEF: /* ARABIC LETTER ALEF MAKSURA ISOLATED FORM */
    case 0xFEF0: /* ARABIC LETTER ALEF MAKSURA FINAL FORM */
    case 0xFEF1: /* ARABIC LETTER YEH ISOLATED FORM */
    case 0xFEF2: /* ARABIC LETTER YEH FINAL FORM */
    case 0xFEF3: /* ARABIC LETTER YEH INITIAL FORM */
    case 0xFEF4: /* ARABIC LETTER YEH MEDIAL FORM */
    case 0xFEF5: /* ARABIC LIGATURE LAM WITH ALEF WITH MADDA ABOVE ISOLATED FORM */
    case 0xFEF6: /* ARABIC LIGATURE LAM WITH ALEF WITH MADDA ABOVE FINAL FORM */
    case 0xFEF7: /* ARABIC LIGATURE LAM WITH ALEF WITH HAMZA ABOVE ISOLATED FORM */
    case 0xFEF8: /* ARABIC LIGATURE LAM WITH ALEF WITH HAMZA ABOVE FINAL FORM */
    case 0xFEF9: /* ARABIC LIGATURE LAM WITH ALEF WITH HAMZA BELOW ISOLATED FORM */
    case 0xFEFA: /* ARABIC LIGATURE LAM WITH ALEF WITH HAMZA BELOW FINAL FORM */
    case 0xFEFB: /* ARABIC LIGATURE LAM WITH ALEF ISOLATED FORM */
    case 0xFEFC: /* ARABIC LIGATURE LAM WITH ALEF FINAL FORM */
    case 0xFF66: /* HALFWIDTH KATAKANA LETTER WO */
    case 0xFF67: /* HALFWIDTH KATAKANA LETTER SMALL A */
    case 0xFF68: /* HALFWIDTH KATAKANA LETTER SMALL I */
    case 0xFF69: /* HALFWIDTH KATAKANA LETTER SMALL U */
    case 0xFF6A: /* HALFWIDTH KATAKANA LETTER SMALL E */
    case 0xFF6B: /* HALFWIDTH KATAKANA LETTER SMALL O */
    case 0xFF6C: /* HALFWIDTH KATAKANA LETTER SMALL YA */
    case 0xFF6D: /* HALFWIDTH KATAKANA LETTER SMALL YU */
    case 0xFF6E: /* HALFWIDTH KATAKANA LETTER SMALL YO */
    case 0xFF6F: /* HALFWIDTH KATAKANA LETTER SMALL TU */
    case 0xFF70: /* HALFWIDTH KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    case 0xFF71: /* HALFWIDTH KATAKANA LETTER A */
    case 0xFF72: /* HALFWIDTH KATAKANA LETTER I */
    case 0xFF73: /* HALFWIDTH KATAKANA LETTER U */
    case 0xFF74: /* HALFWIDTH KATAKANA LETTER E */
    case 0xFF75: /* HALFWIDTH KATAKANA LETTER O */
    case 0xFF76: /* HALFWIDTH KATAKANA LETTER KA */
    case 0xFF77: /* HALFWIDTH KATAKANA LETTER KI */
    case 0xFF78: /* HALFWIDTH KATAKANA LETTER KU */
    case 0xFF79: /* HALFWIDTH KATAKANA LETTER KE */
    case 0xFF7A: /* HALFWIDTH KATAKANA LETTER KO */
    case 0xFF7B: /* HALFWIDTH KATAKANA LETTER SA */
    case 0xFF7C: /* HALFWIDTH KATAKANA LETTER SI */
    case 0xFF7D: /* HALFWIDTH KATAKANA LETTER SU */
    case 0xFF7E: /* HALFWIDTH KATAKANA LETTER SE */
    case 0xFF7F: /* HALFWIDTH KATAKANA LETTER SO */
    case 0xFF80: /* HALFWIDTH KATAKANA LETTER TA */
    case 0xFF81: /* HALFWIDTH KATAKANA LETTER TI */
    case 0xFF82: /* HALFWIDTH KATAKANA LETTER TU */
    case 0xFF83: /* HALFWIDTH KATAKANA LETTER TE */
    case 0xFF84: /* HALFWIDTH KATAKANA LETTER TO */
    case 0xFF85: /* HALFWIDTH KATAKANA LETTER NA */
    case 0xFF86: /* HALFWIDTH KATAKANA LETTER NI */
    case 0xFF87: /* HALFWIDTH KATAKANA LETTER NU */
    case 0xFF88: /* HALFWIDTH KATAKANA LETTER NE */
    case 0xFF89: /* HALFWIDTH KATAKANA LETTER NO */
    case 0xFF8A: /* HALFWIDTH KATAKANA LETTER HA */
    case 0xFF8B: /* HALFWIDTH KATAKANA LETTER HI */
    case 0xFF8C: /* HALFWIDTH KATAKANA LETTER HU */
    case 0xFF8D: /* HALFWIDTH KATAKANA LETTER HE */
    case 0xFF8E: /* HALFWIDTH KATAKANA LETTER HO */
    case 0xFF8F: /* HALFWIDTH KATAKANA LETTER MA */
    case 0xFF90: /* HALFWIDTH KATAKANA LETTER MI */
    case 0xFF91: /* HALFWIDTH KATAKANA LETTER MU */
    case 0xFF92: /* HALFWIDTH KATAKANA LETTER ME */
    case 0xFF93: /* HALFWIDTH KATAKANA LETTER MO */
    case 0xFF94: /* HALFWIDTH KATAKANA LETTER YA */
    case 0xFF95: /* HALFWIDTH KATAKANA LETTER YU */
    case 0xFF96: /* HALFWIDTH KATAKANA LETTER YO */
    case 0xFF97: /* HALFWIDTH KATAKANA LETTER RA */
    case 0xFF98: /* HALFWIDTH KATAKANA LETTER RI */
    case 0xFF99: /* HALFWIDTH KATAKANA LETTER RU */
    case 0xFF9A: /* HALFWIDTH KATAKANA LETTER RE */
    case 0xFF9B: /* HALFWIDTH KATAKANA LETTER RO */
    case 0xFF9C: /* HALFWIDTH KATAKANA LETTER WA */
    case 0xFF9D: /* HALFWIDTH KATAKANA LETTER N */
    case 0xFF9E: /* HALFWIDTH KATAKANA VOICED SOUND MARK */
    case 0xFF9F: /* HALFWIDTH KATAKANA SEMI-VOICED SOUND MARK */
    case 0xFFA0: /* HALFWIDTH HANGUL FILLER */
    case 0xFFA1: /* HALFWIDTH HANGUL LETTER KIYEOK */
    case 0xFFA2: /* HALFWIDTH HANGUL LETTER SSANGKIYEOK */
    case 0xFFA3: /* HALFWIDTH HANGUL LETTER KIYEOK-SIOS */
    case 0xFFA4: /* HALFWIDTH HANGUL LETTER NIEUN */
    case 0xFFA5: /* HALFWIDTH HANGUL LETTER NIEUN-CIEUC */
    case 0xFFA6: /* HALFWIDTH HANGUL LETTER NIEUN-HIEUH */
    case 0xFFA7: /* HALFWIDTH HANGUL LETTER TIKEUT */
    case 0xFFA8: /* HALFWIDTH HANGUL LETTER SSANGTIKEUT */
    case 0xFFA9: /* HALFWIDTH HANGUL LETTER RIEUL */
    case 0xFFAA: /* HALFWIDTH HANGUL LETTER RIEUL-KIYEOK */
    case 0xFFAB: /* HALFWIDTH HANGUL LETTER RIEUL-MIEUM */
    case 0xFFAC: /* HALFWIDTH HANGUL LETTER RIEUL-PIEUP */
    case 0xFFAD: /* HALFWIDTH HANGUL LETTER RIEUL-SIOS */
    case 0xFFAE: /* HALFWIDTH HANGUL LETTER RIEUL-THIEUTH */
    case 0xFFAF: /* HALFWIDTH HANGUL LETTER RIEUL-PHIEUPH */
    case 0xFFB0: /* HALFWIDTH HANGUL LETTER RIEUL-HIEUH */
    case 0xFFB1: /* HALFWIDTH HANGUL LETTER MIEUM */
    case 0xFFB2: /* HALFWIDTH HANGUL LETTER PIEUP */
    case 0xFFB3: /* HALFWIDTH HANGUL LETTER SSANGPIEUP */
    case 0xFFB4: /* HALFWIDTH HANGUL LETTER PIEUP-SIOS */
    case 0xFFB5: /* HALFWIDTH HANGUL LETTER SIOS */
    case 0xFFB6: /* HALFWIDTH HANGUL LETTER SSANGSIOS */
    case 0xFFB7: /* HALFWIDTH HANGUL LETTER IEUNG */
    case 0xFFB8: /* HALFWIDTH HANGUL LETTER CIEUC */
    case 0xFFB9: /* HALFWIDTH HANGUL LETTER SSANGCIEUC */
    case 0xFFBA: /* HALFWIDTH HANGUL LETTER CHIEUCH */
    case 0xFFBB: /* HALFWIDTH HANGUL LETTER KHIEUKH */
    case 0xFFBC: /* HALFWIDTH HANGUL LETTER THIEUTH */
    case 0xFFBD: /* HALFWIDTH HANGUL LETTER PHIEUPH */
    case 0xFFBE: /* HALFWIDTH HANGUL LETTER HIEUH */
    case 0xFFC2: /* HALFWIDTH HANGUL LETTER A */
    case 0xFFC3: /* HALFWIDTH HANGUL LETTER AE */
    case 0xFFC4: /* HALFWIDTH HANGUL LETTER YA */
    case 0xFFC5: /* HALFWIDTH HANGUL LETTER YAE */
    case 0xFFC6: /* HALFWIDTH HANGUL LETTER EO */
    case 0xFFC7: /* HALFWIDTH HANGUL LETTER E */
    case 0xFFCA: /* HALFWIDTH HANGUL LETTER YEO */
    case 0xFFCB: /* HALFWIDTH HANGUL LETTER YE */
    case 0xFFCC: /* HALFWIDTH HANGUL LETTER O */
    case 0xFFCD: /* HALFWIDTH HANGUL LETTER WA */
    case 0xFFCE: /* HALFWIDTH HANGUL LETTER WAE */
    case 0xFFCF: /* HALFWIDTH HANGUL LETTER OE */
    case 0xFFD2: /* HALFWIDTH HANGUL LETTER YO */
    case 0xFFD3: /* HALFWIDTH HANGUL LETTER U */
    case 0xFFD4: /* HALFWIDTH HANGUL LETTER WEO */
    case 0xFFD5: /* HALFWIDTH HANGUL LETTER WE */
    case 0xFFD6: /* HALFWIDTH HANGUL LETTER WI */
    case 0xFFD7: /* HALFWIDTH HANGUL LETTER YU */
    case 0xFFDA: /* HALFWIDTH HANGUL LETTER EU */
    case 0xFFDB: /* HALFWIDTH HANGUL LETTER YI */
    case 0xFFDC: /* HALFWIDTH HANGUL LETTER I */
	return 1;
    default:
	return 0;
    }
}

#else

/* Export the interfaces using the wchar_t type for portability
   reasons:  */

int _PyUnicode_IsWhitespace(register const Py_UNICODE ch)
{
    return iswspace(ch);
}

int _PyUnicode_IsLowercase(register const Py_UNICODE ch)
{
    return iswlower(ch);
}

int _PyUnicode_IsUppercase(register const Py_UNICODE ch)
{
    return iswupper(ch);
}

Py_UNICODE _PyUnicode_ToLowercase(register const Py_UNICODE ch)
{
    return towlower(ch);
}

Py_UNICODE _PyUnicode_ToUppercase(register const Py_UNICODE ch)
{
    return towupper(ch);
}

int _PyUnicode_IsAlpha(register const Py_UNICODE ch)
{
    return iswalpha(ch);
}

#endif
