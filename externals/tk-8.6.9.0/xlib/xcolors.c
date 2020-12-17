/*
 * xcolors.c --
 *
 *	This file contains the routines used to map from X color names to RGB
 *	and pixel values.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 * Copyright (c) 2012 by Jan Nijtmans
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkInt.h"

/*
 * Index array. For each of the characters 'a'-'y', this table gives the first
 * color starting with that character in the xColors table.
 */

static const unsigned char az[] = {
    0, 5, 13, 21, 45, 46, 50, 60, 62, 65, 66,
    67, 91, 106, 109, 115, 126, 127, 130, 144, 149, 150, 152, 155, 156, 158
};

/*
 * Define an array that defines the mapping from color names to RGB values.
 * Note that this array must be kept sorted alphabetically so that the
 * binary search used in XParseColor will succeed.
 *
 * Each color definition consists of exactly 32 characters, and starts with
 * the color name, but without its first character (that character can be
 * reproduced from the az index array). The last byte holds the number
 * of additional color variants. For example "azure1" up to "azure4" are
 * handled by the same table entry as "azure". From the last byte backwards,
 * each group of 3 bytes contain the rgb values of the main color and
 * the available variants.
 *
 * The colors gray and grey have more than 8 variants. gray1 up to gray8
 * are handled by this table, above that is handled especially.
 */

typedef char elem[32];

static const elem xColors[] = {
    /* Colors starting with 'a' */
    "liceBlue\0                   \360\370\377",
    "ntiqueWhite\0    \213\203\170\315\300\260\356\337\314\377\357\333\372\353\327\4",
    "qua\0                        \000\377\377",
    "quamarine\0      \105\213\164\146\315\252\166\356\306\177\377\324\177\377\324\4",
    "zure\0           \203\213\213\301\315\315\340\356\356\360\377\377\360\377\377\4",
    /* Colors starting with 'b' */
    "eige\0                       \365\365\334",
    "isque\0          \213\175\153\315\267\236\356\325\267\377\344\304\377\344\304\4",
    "lack\0                       \000\000\000",
    "lanchedAlmond\0              \377\353\315",
    "lue\0            \000\000\213\000\000\315\000\000\356\000\000\377\000\000\377\4",
    "lueViolet\0                  \212\053\342",
    "rown\0           \213\043\043\315\063\063\356\073\073\377\100\100\245\052\052\4",
    "urlywood\0       \213\163\125\315\252\175\356\305\221\377\323\233\336\270\207\4",
    /* Colors starting with 'c' */
    "adetBlue\0       \123\206\213\172\305\315\216\345\356\230\365\377\137\236\240\4",
    "hartreuse\0      \105\213\000\146\315\000\166\356\000\177\377\000\177\377\000\4",
    "hocolate\0       \213\105\023\315\146\035\356\166\041\377\177\044\322\151\036\4",
    "oral\0           \213\076\057\315\133\105\356\152\120\377\162\126\377\177\120\4",
    "ornflowerBlue\0              \144\225\355",
    "ornsilk\0        \213\210\170\315\310\261\356\350\315\377\370\334\377\370\334\4",
    "rimson\0                     \334\024\074",
    "yan\0            \000\213\213\000\315\315\000\356\356\000\377\377\000\377\377\4",
    /* Colors starting with 'd' */
    "arkBlue\0                    \000\000\213",
    "arkCyan\0                    \000\213\213",
    "arkGoldenrod\0   \213\145\010\315\225\014\356\255\016\377\271\017\270\206\013\4",
    "arkGray\0                    \251\251\251",
    "arkGreen\0                   \000\144\000",
    "arkGrey\0                    \251\251\251",
    "arkKhaki\0                   \275\267\153",
    "arkMagenta\0                 \213\000\213",
    "arkOliveGreen\0  \156\213\075\242\315\132\274\356\150\312\377\160\125\153\057\4",
    "arkOrange\0      \213\105\000\315\146\000\356\166\000\377\177\000\377\214\000\4",
    "arkOrchid\0      \150\042\213\232\062\315\262\072\356\277\076\377\231\062\314\4",
    "arkRed\0                     \213\000\000",
    "arkSalmon\0                  \351\226\172",
    "arkSeaGreen\0    \151\213\151\233\315\233\264\356\264\301\377\301\217\274\217\4",
    "arkSlateBlue\0               \110\075\213",
    "arkSlateGray\0   \122\213\213\171\315\315\215\356\356\227\377\377\057\117\117\4",
    "arkSlateGrey\0               \057\117\117",
    "arkTurquoise\0               \000\316\321",
    "arkViolet\0                  \224\000\323",
    "eepPink\0        \213\012\120\315\020\166\356\022\211\377\024\223\377\024\223\4",
    "eepSkyBlue\0     \000\150\213\000\232\315\000\262\356\000\277\377\000\277\377\4",
    "imGray\0                     \151\151\151",
    "imGrey\0                     \151\151\151",
    "odgerBlue\0      \020\116\213\030\164\315\034\206\356\036\220\377\036\220\377\4",
    /* Colors starting with 'e' */
    "\377" /* placeholder */,
    /* Colors starting with 'f' */
    "irebrick\0       \213\032\032\315\046\046\356\054\054\377\060\060\262\042\042\4",
    "loralWhite\0                 \377\372\360",
    "orestGreen\0                 \042\213\042",
    "uchsia\0                     \377\000\377",
    /* Colors starting with 'g' */
    "ainsboro\0                   \334\334\334",
    "hostWhite\0                  \370\370\377",
    "old\0            \213\165\000\315\255\000\356\311\000\377\327\000\377\327\000\4",
    "oldenrod\0       \213\151\024\315\233\035\356\264\042\377\301\045\332\245\040\4",
    "ray\0\024\024\024\022\022\022\017\017\017\015\015\015\012\012\012"
	    "\010\010\010\005\005\005\003\003\003\200\200\200\10",
    "ray0\0                       \000\000\000",
    "reen\0           \000\213\000\000\315\000\000\356\000\000\377\000\000\200\000\4",
    "reenYellow\0                 \255\377\057",
    "rey\0\024\024\024\022\022\022\017\017\017\015\015\015\012\012\012"
	    "\010\010\010\005\005\005\003\003\003\200\200\200\10",
    "rey0\0                       \000\000\000",
    /* Colors starting with 'h' */
    "oneydew\0        \203\213\203\301\315\301\340\356\340\360\377\360\360\377\360\4",
    "otPink\0         \213\072\142\315\140\220\356\152\247\377\156\264\377\151\264\4",
    /* Colors starting with 'i' */
    "ndianRed\0       \213\072\072\315\125\125\356\143\143\377\152\152\315\134\134\4",
    "ndigo\0                      \113\000\202",
    "vory\0           \213\213\203\315\315\301\356\356\340\377\377\360\377\377\360\4",
    /* Colors starting with 'j' */
    "\377" /* placeholder */,
    /* Colors starting with 'k' */
    "haki\0           \213\206\116\315\306\163\356\346\205\377\366\217\360\346\214\4",
    /* Colors starting with 'l' */
    "avender\0                    \346\346\372",
    "avenderBlush\0   \213\203\206\315\301\305\356\340\345\377\360\365\377\360\365\4",
    "awnGreen\0                   \174\374\000",
    "emonChiffon\0    \213\211\160\315\311\245\356\351\277\377\372\315\377\372\315\4",
    "ightBlue\0       \150\203\213\232\300\315\262\337\356\277\357\377\255\330\346\4",
    "ightCoral\0                  \360\200\200",
    "ightCyan\0       \172\213\213\264\315\315\321\356\356\340\377\377\340\377\377\4",
    "ightGoldenrod\0  \213\201\114\315\276\160\356\334\202\377\354\213\356\335\202\4",
    "ightGoldenrodYellow\0        \372\372\322",
    "ightGray\0                   \323\323\323",
    "ightGreen\0                  \220\356\220",
    "ightGrey\0                   \323\323\323",
    "ightPink\0       \213\137\145\315\214\225\356\242\255\377\256\271\377\266\301\4",
    "ightSalmon\0     \213\127\102\315\201\142\356\225\162\377\240\172\377\240\172\4",
    "ightSeaGreen\0               \040\262\252",
    "ightSkyBlue\0    \140\173\213\215\266\315\244\323\356\260\342\377\207\316\372\4",
    "ightSlateBlue\0              \204\160\377",
    "ightSlateGray\0              \167\210\231",
    "ightSlateGrey\0              \167\210\231",
    "ightSteelBlue\0  \156\173\213\242\265\315\274\322\356\312\341\377\260\304\336\4",
    "ightYellow\0     \213\213\172\315\315\264\356\356\321\377\377\340\377\377\340\4",
    "ime\0                        \000\377\000",
    "imeGreen\0                   \062\315\062",
    "inen\0                       \372\360\346",
    /* Colors starting with 'm' */
    "agenta\0         \213\000\213\315\000\315\356\000\356\377\000\377\377\000\377\4",
    "aroon\0          \213\034\142\315\051\220\356\060\247\377\064\263\200\000\000\4",
    "ediumAquamarine\0            \146\315\252",
    "ediumBlue\0                  \000\000\315",
    "ediumOrchid\0    \172\067\213\264\122\315\321\137\356\340\146\377\272\125\323\4",
    "ediumPurple\0    \135\107\213\211\150\315\237\171\356\253\202\377\223\160\333\4",
    "ediumSeaGreen\0              \074\263\161",
    "ediumSlateBlue\0             \173\150\356",
    "ediumSpringGreen\0           \000\372\232",
    "ediumTurquoise\0             \110\321\314",
    "ediumVioletRed\0             \307\025\205",
    "idnightBlue\0                \031\031\160",
    "intCream\0                   \365\377\372",
    "istyRose\0       \213\175\173\315\267\265\356\325\322\377\344\341\377\344\341\4",
    "occasin\0                    \377\344\265",
    /* Colors starting with 'n' */
    "avajoWhite\0     \213\171\136\315\263\213\356\317\241\377\336\255\377\336\255\4",
    "avy\0                        \000\000\200",
    "avyBlue\0                    \000\000\200",
    /* Colors starting with 'o' */
    "ldLace\0                     \375\365\346",
    "live\0                       \200\200\000",
    "liveDrab\0       \151\213\042\232\315\062\263\356\072\300\377\076\153\216\043\4",
    "range\0          \213\132\000\315\205\000\356\232\000\377\245\000\377\245\000\4",
    "rangeRed\0       \213\045\000\315\067\000\356\100\000\377\105\000\377\105\000\4",
    "rchid\0          \213\107\211\315\151\311\356\172\351\377\203\372\332\160\326\4",
    /* Colors starting with 'p' */
    "aleGoldenrod\0               \356\350\252",
    "aleGreen\0       \124\213\124\174\315\174\220\356\220\232\377\232\230\373\230\4",
    "aleTurquoise\0   \146\213\213\226\315\315\256\356\356\273\377\377\257\356\356\4",
    "aleVioletRed\0   \213\107\135\315\150\211\356\171\237\377\202\253\333\160\223\4",
    "apayaWhip\0                  \377\357\325",
    "eachPuff\0       \213\167\145\315\257\225\356\313\255\377\332\271\377\332\271\4",
    "eru\0                        \315\205\077",
    "ink\0            \213\143\154\315\221\236\356\251\270\377\265\305\377\300\313\4",
    "lum\0            \213\146\213\315\226\315\356\256\356\377\273\377\335\240\335\4",
    "owderBlue\0                  \260\340\346",
    "urple\0          \125\032\213\175\046\315\221\054\356\233\060\377\200\000\200\4",
    /* Colors starting with 'q' */
    "\377" /* placeholder */,
    /* Colors starting with 'r' */
    "ed\0             \213\000\000\315\000\000\356\000\000\377\000\000\377\000\000\4",
    "osyBrown\0       \213\151\151\315\233\233\356\264\264\377\301\301\274\217\217\4",
    "oyalBlue\0       \047\100\213\072\137\315\103\156\356\110\166\377\101\151\341\4",
    /* Colors starting with 's' */
    "addleBrown\0                 \213\105\023",
    "almon\0          \213\114\071\315\160\124\356\202\142\377\214\151\372\200\162\4",
    "andyBrown\0                  \364\244\140",
    "eaGreen\0        \056\213\127\103\315\200\116\356\224\124\377\237\056\213\127\4",
    "eashell\0        \213\206\202\315\305\277\356\345\336\377\365\356\377\365\356\4",
    "ienna\0          \213\107\046\315\150\071\356\171\102\377\202\107\240\122\055\4",
    "ilver\0                      \300\300\300",
    "kyBlue\0         \112\160\213\154\246\315\176\300\356\207\316\377\207\316\353\4",
    "lateBlue\0       \107\074\213\151\131\315\172\147\356\203\157\377\152\132\315\4",
    "lateGray\0       \154\173\213\237\266\315\271\323\356\306\342\377\160\200\220\4",
    "lateGrey\0                   \160\200\220",
    "now\0            \213\211\211\315\311\311\356\351\351\377\372\372\377\372\372\4",
    "pringGreen\0     \000\213\105\000\315\146\000\356\166\000\377\177\000\377\177\4",
    "teelBlue\0       \066\144\213\117\224\315\134\254\356\143\270\377\106\202\264\4",
    /* Colors starting with 't' */
    "an\0             \213\132\053\315\205\077\356\232\111\377\245\117\322\264\214\4",
    "eal\0                        \000\200\200",
    "histle\0         \213\173\213\315\265\315\356\322\356\377\341\377\330\277\330\4",
    "omato\0          \213\066\046\315\117\071\356\134\102\377\143\107\377\143\107\4",
    "urquoise\0       \000\206\213\000\305\315\000\345\356\000\365\377\100\340\320\4",
    /* Colors starting with 'u' */
    "\377" /* placeholder */,
    /* Colors starting with 'v' */
    "iolet\0                      \356\202\356",
    "ioletRed\0       \213\042\122\315\062\170\356\072\214\377\076\226\320\040\220\4",
    /* Colors starting with 'w' */
    "heat\0           \213\176\146\315\272\226\356\330\256\377\347\272\365\336\263\4",
    "hite\0                       \377\377\377",
    "hiteSmoke\0                  \365\365\365",
    /* Colors starting with 'x' */
    "\377" /* placeholder */,
    /* Colors starting with 'y' */
    "ellow\0          \213\213\000\315\315\000\356\356\000\377\377\000\377\377\000\4",
    "ellowGreen\0                 \232\315\062\0"
};

/*
 *----------------------------------------------------------------------
 *
 * XParseColor --
 *
 *	Partial implementation of X color name parsing interface.
 *
 * Results:
 *	Returns non-zero on success.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 *
 * This only handles hex-strings without 0x prefix. Luckily, that's just what
 * we need.
 */

static Tcl_WideInt
parseHex64bit(
    const char *spec,
    char **p)
{
    Tcl_WideInt result = 0;
    char c;
    while ((c = *spec)) {
	if ((c >= '0') && (c <= '9')) {
	    c -= '0';
	} else if ((c >= 'A') && (c <= 'F')) {
	    c += (10 - 'A');
	} else if ((c >= 'a') && (c <= 'f')) {
	    c += (10 - 'a');
	} else {
	    break;
	}
	result = (result << 4) + c;
	++spec;
    }
    *p = (char *) spec;
    return result;
}

static int
colorcmp(
    const char *spec,
    const char *pname,
    int *special)
{
    int r;
    int c, d;
    int notequal = 0;
    int num = 0;

    do {
	d = *pname++;
	c = (*spec == ' ');
	if (c) {
	    spec++;
	}
	if ((unsigned)(d - 'A') <= (unsigned)('Z' - 'A')) {
	    d += 'a' - 'A';
	} else if (c) {
	    /*
	     * A space doesn't match a lowercase, but we don't know yet
	     * whether we should return a negative or positive number. That
	     * depends on what follows.
	     */

	    notequal = 1;
	}
	c = *spec++;
	if ((unsigned)(c - 'A') <= (unsigned)('Z' - 'A')) {
	    c += 'a' - 'A';
	} else if (((unsigned)(c - '1') <= (unsigned)('9' - '1'))) {
	    if (d == '0') {
	    	d += 10;
	    } else if (!d) {
		num = c - '0';
		while ((unsigned)((c = *spec++) - '0') <= (unsigned)('9' - '0')) {
		    num = num * 10 + c - '0';
		}
	    }
	}
	r = c - d;
    } while (!r && d);

    if (!r && notequal) {
	/*
	 * Strings are equal, but difference in spacings only. We should still
	 * report not-equal, so "burly wood" is not a valid color.
	 */

	r = 1;
    }
    *special = num;
    return r;
}

#define RED(p)		((unsigned char) (p)[0])
#define GREEN(p)	((unsigned char) (p)[1])
#define BLUE(p)		((unsigned char) (p)[2])
#define US(expr)	((unsigned short) (expr))

Status
XParseColor(
    Display *display,
    Colormap map,
    const char *spec,
    XColor *colorPtr)
{
    if (spec[0] == '#') {
	char *p;
	Tcl_WideInt value = parseHex64bit(++spec, &p);

	/*
	 * If *p does not point to the end of the string, there were invalid
	 * digits in the spec. Ergo, it is not a vailid color string.
	 * (Bug f0188aca9e)
	 */

	if (*p != '\0') {
	    return 0;
	}

	switch ((int)(p-spec)) {
	case 3:
	    colorPtr->red = US(((value >> 8) & 0xf) * 0x1111);
	    colorPtr->green = US(((value >> 4) & 0xf) * 0x1111);
	    colorPtr->blue = US((value & 0xf) * 0x1111);
	    break;
	case 6:
	    colorPtr->red = US(((value >> 16) & 0xff) | ((value >> 8) & 0xff00));
	    colorPtr->green = US(((value >> 8) & 0xff) | (value & 0xff00));
	    colorPtr->blue = US((value & 0xff) | (value << 8));
	    break;
	case 9:
	    colorPtr->red = US(((value >> 32) & 0xf) | ((value >> 20) & 0xfff0));
	    colorPtr->green = US(((value >> 20) & 0xf) | ((value >> 8) & 0xfff0));
	    colorPtr->blue = US(((value >> 8) & 0xf) | (value << 4));
	    break;
	case 12:
	    colorPtr->red = US(value >> 32);
	    colorPtr->green = US(value >> 16);
	    colorPtr->blue = US(value);
	    break;
	default:
	    return 0;
	}
    } else {
	/*
	 * Perform a binary search on the sorted array of colors.
	 * size = current size of search range
	 * p    = pointer to current element being considered.
	 */

	int size, num;
	const elem *p;
	const char *q;
	int r = (spec[0] - 'A') & 0xdf;

	if (r >= (int) sizeof(az) - 1) {
	    return 0;
	}
	size = az[r + 1] - az[r];
	p = &xColors[(az[r + 1] + az[r]) >> 1];
	r = colorcmp(spec + 1, *p, &num);

	while (r != 0) {
	    if (r < 0) {
		size = (size >> 1);
		p -= ((size + 1) >> 1);
	    } else {
		--size;
		size = (size >> 1);
		p += ((size + 2) >> 1);
	    }
	    if (!size) {
		return 0;
	    }
	    r = colorcmp(spec + 1, *p, &num);
	}
	if (num > (*p)[31]) {
	    if (((*p)[31] != 8) || num > 100) {
	    	return 0;
	    }
	    num = (num * 255 + 50) / 100;
	    if ((num == 230) || (num == 128)) {
	    	/*
		 * Those two entries have a deviation i.r.t the table.
		 */

		num--;
	    }
	    num |= (num << 8);
	    colorPtr->red = colorPtr->green = colorPtr->blue = num;
	} else {
	    q = *p + 28 - num * 3;
	    colorPtr->red = ((RED(q) << 8) | RED(q));
	    colorPtr->green = ((GREEN(q) << 8) | GREEN(q));
	    colorPtr->blue = ((BLUE(q) << 8) | BLUE(q));
	}
    }
    colorPtr->pixel = TkpGetPixel(colorPtr);
    colorPtr->flags = DoRed|DoGreen|DoBlue;
    colorPtr->pad = 0;
    return 1;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
