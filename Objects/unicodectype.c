/*
   Unicode character type helpers.

   The data contained in the function's switch tables was extracted
   from the Unicode 3.0 data file.

Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

*/

#include "Python.h"

#include "unicodeobject.h"

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

#endif
