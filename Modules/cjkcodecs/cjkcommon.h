/*
 * cjkcommon.h: Common Constants and Macroes for CJK Character Sets
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: cjkcommon.h,v 1.2 2003/12/31 05:46:55 perky Exp $
 */

#ifndef _CJKCOMMON_H_
#define _CJKCOMMON_H_

#ifdef uint32_t
typedef uint32_t ucs4_t;
#else
typedef unsigned int ucs4_t;
#endif

#ifdef uint16_t
typedef uint16_t ucs2_t, DBCHAR;
#else
typedef unsigned short ucs2_t, DBCHAR;
#endif

#define UNIINV  Py_UNICODE_REPLACEMENT_CHARACTER
#define NOCHAR  0xFFFF
#define MULTIC  0xFFFE
#define DBCINV  0xFFFD

struct dbcs_index {
    const ucs2_t   *map;
    unsigned char   bottom, top;
};
typedef struct dbcs_index decode_map;

struct widedbcs_index {
    const ucs4_t   *map;
    unsigned char   bottom, top;
};
typedef struct widedbcs_index widedecode_map;

struct unim_index {
    const DBCHAR   *map;
    unsigned char   bottom, top;
};
typedef struct unim_index encode_map;

struct dbcs_map {
    const char *charset;
    const struct unim_index *encmap;
    const struct dbcs_index *decmap;
};

struct pair_encodemap {
    ucs4_t   uniseq;
    DBCHAR   code;
};

#endif
