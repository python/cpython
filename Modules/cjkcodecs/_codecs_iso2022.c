/*
 * _codecs_iso2022.c: Codecs collection for ISO-2022 encodings.
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#define USING_IMPORTED_MAPS
#define USING_BINARY_PAIR_SEARCH
#define EXTERN_JISX0213_PAIR
#define EMULATE_JISX0213_2000_ENCODE_INVALID MAP_UNMAPPABLE
#define EMULATE_JISX0213_2000_DECODE_INVALID MAP_UNMAPPABLE

#include "cjkcodecs.h"
#include "alg_jisx0201.h"
#include "emu_jisx0213_2000.h"
#include "mappings_jisx0213_pair.h"

/* STATE

   state->c[0-3]

    00000000
    ||^^^^^|
    |+-----+----  G0-3 Character Set
    +-----------  Is G0-3 double byte?

   state->c[4]

    00000000
          ||
          |+----  Locked-Shift?
          +-----  ESC Throughout
*/

#define ESC                     0x1B
#define SO                      0x0E
#define SI                      0x0F
#define LF                      0x0A

#define MAX_ESCSEQLEN           16

#define CHARSET_ISO8859_1       'A'
#define CHARSET_ASCII           'B'
#define CHARSET_ISO8859_7       'F'
#define CHARSET_JISX0201_K      'I'
#define CHARSET_JISX0201_R      'J'

#define CHARSET_GB2312          ('A'|CHARSET_DBCS)
#define CHARSET_JISX0208        ('B'|CHARSET_DBCS)
#define CHARSET_KSX1001         ('C'|CHARSET_DBCS)
#define CHARSET_JISX0212        ('D'|CHARSET_DBCS)
#define CHARSET_GB2312_8565     ('E'|CHARSET_DBCS)
#define CHARSET_CNS11643_1      ('G'|CHARSET_DBCS)
#define CHARSET_CNS11643_2      ('H'|CHARSET_DBCS)
#define CHARSET_JISX0213_2000_1 ('O'|CHARSET_DBCS)
#define CHARSET_JISX0213_2      ('P'|CHARSET_DBCS)
#define CHARSET_JISX0213_2004_1 ('Q'|CHARSET_DBCS)
#define CHARSET_JISX0208_O      ('@'|CHARSET_DBCS)

#define CHARSET_DBCS            0x80
#define ESCMARK(mark)           ((mark) & 0x7f)

#define IS_ESCEND(c)    (((c) >= 'A' && (c) <= 'Z') || (c) == '@')
#define IS_ISO2022ESC(c2) \
        ((c2) == '(' || (c2) == ')' || (c2) == '$' || \
         (c2) == '.' || (c2) == '&')
    /* this is not a complete list of ISO-2022 escape sequence headers.
     * but, it's enough to implement CJK instances of iso-2022. */

#define MAP_UNMAPPABLE          0xFFFF
#define MAP_MULTIPLE_AVAIL      0xFFFE /* for JIS X 0213 */

#define F_SHIFTED               0x01
#define F_ESCTHROUGHOUT         0x02

#define STATE_SETG(dn, v)       do { ((state)->c[dn]) = (v); } while (0)
#define STATE_GETG(dn)          ((state)->c[dn])

#define STATE_G0                STATE_GETG(0)
#define STATE_G1                STATE_GETG(1)
#define STATE_G2                STATE_GETG(2)
#define STATE_G3                STATE_GETG(3)
#define STATE_SETG0(v)          STATE_SETG(0, v)
#define STATE_SETG1(v)          STATE_SETG(1, v)
#define STATE_SETG2(v)          STATE_SETG(2, v)
#define STATE_SETG3(v)          STATE_SETG(3, v)

#define STATE_SETFLAG(f)        do { ((state)->c[4]) |= (f); } while (0)
#define STATE_GETFLAG(f)        ((state)->c[4] & (f))
#define STATE_CLEARFLAG(f)      do { ((state)->c[4]) &= ~(f); } while (0)
#define STATE_CLEARFLAGS()      do { ((state)->c[4]) = 0; } while (0)

#define ISO2022_CONFIG          ((const struct iso2022_config *)config)
#define CONFIG_ISSET(flag)      (ISO2022_CONFIG->flags & (flag))
#define CONFIG_DESIGNATIONS     (ISO2022_CONFIG->designations)

/* iso2022_config.flags */
#define NO_SHIFT                0x01
#define USE_G2                  0x02
#define USE_JISX0208_EXT        0x04

/*-*- internal data structures -*-*/

typedef int (*iso2022_init_func)(void);
typedef Py_UCS4 (*iso2022_decode_func)(const unsigned char *data);
typedef DBCHAR (*iso2022_encode_func)(const Py_UCS4 *data, Py_ssize_t *length);

struct iso2022_designation {
    unsigned char mark;
    unsigned char plane;
    unsigned char width;
    iso2022_init_func initializer;
    iso2022_decode_func decoder;
    iso2022_encode_func encoder;
};

struct iso2022_config {
    int flags;
    const struct iso2022_designation *designations; /* non-ascii desigs */
};

/*-*- iso-2022 codec implementation -*-*/

CODEC_INIT(iso2022)
{
    const struct iso2022_designation *desig;
    for (desig = CONFIG_DESIGNATIONS; desig->mark; desig++)
        if (desig->initializer != NULL && desig->initializer() != 0)
            return -1;
    return 0;
}

ENCODER_INIT(iso2022)
{
    STATE_CLEARFLAGS();
    STATE_SETG0(CHARSET_ASCII);
    STATE_SETG1(CHARSET_ASCII);
    return 0;
}

ENCODER_RESET(iso2022)
{
    if (STATE_GETFLAG(F_SHIFTED)) {
        WRITEBYTE1(SI);
        NEXT_OUT(1);
        STATE_CLEARFLAG(F_SHIFTED);
    }
    if (STATE_G0 != CHARSET_ASCII) {
        WRITEBYTE3(ESC, '(', 'B');
        NEXT_OUT(3);
        STATE_SETG0(CHARSET_ASCII);
    }
    return 0;
}

ENCODER(iso2022)
{
    while (*inpos < inlen) {
        const struct iso2022_designation *dsg;
        DBCHAR encoded;
        Py_UCS4 c = INCHAR1;
        Py_ssize_t insize;

        if (c < 0x80) {
            if (STATE_G0 != CHARSET_ASCII) {
                WRITEBYTE3(ESC, '(', 'B');
                STATE_SETG0(CHARSET_ASCII);
                NEXT_OUT(3);
            }
            if (STATE_GETFLAG(F_SHIFTED)) {
                WRITEBYTE1(SI);
                STATE_CLEARFLAG(F_SHIFTED);
                NEXT_OUT(1);
            }
            WRITEBYTE1((unsigned char)c);
            NEXT(1, 1);
            continue;
        }

        insize = 1;

        encoded = MAP_UNMAPPABLE;
        for (dsg = CONFIG_DESIGNATIONS; dsg->mark; dsg++) {
            Py_ssize_t length = 1;
            encoded = dsg->encoder(&c, &length);
            if (encoded == MAP_MULTIPLE_AVAIL) {
                /* this implementation won't work for pair
                 * of non-bmp characters. */
                if (inlen - *inpos < 2) {
                    if (!(flags & MBENC_FLUSH))
                        return MBERR_TOOFEW;
                    length = -1;
                }
                else
                    length = 2;
                encoded = dsg->encoder(&c, &length);
                if (encoded != MAP_UNMAPPABLE) {
                    insize = length;
                    break;
                }
            }
            else if (encoded != MAP_UNMAPPABLE)
                break;
        }

        if (!dsg->mark)
            return 1;
        assert(dsg->width == 1 || dsg->width == 2);

        switch (dsg->plane) {
        case 0: /* G0 */
            if (STATE_GETFLAG(F_SHIFTED)) {
                WRITEBYTE1(SI);
                STATE_CLEARFLAG(F_SHIFTED);
                NEXT_OUT(1);
            }
            if (STATE_G0 != dsg->mark) {
                if (dsg->width == 1) {
                    WRITEBYTE3(ESC, '(', ESCMARK(dsg->mark));
                    STATE_SETG0(dsg->mark);
                    NEXT_OUT(3);
                }
                else if (dsg->mark == CHARSET_JISX0208) {
                    WRITEBYTE3(ESC, '$', ESCMARK(dsg->mark));
                    STATE_SETG0(dsg->mark);
                    NEXT_OUT(3);
                }
                else {
                    WRITEBYTE4(ESC, '$', '(',
                        ESCMARK(dsg->mark));
                    STATE_SETG0(dsg->mark);
                    NEXT_OUT(4);
                }
            }
            break;
        case 1: /* G1 */
            if (STATE_G1 != dsg->mark) {
                if (dsg->width == 1) {
                    WRITEBYTE3(ESC, ')', ESCMARK(dsg->mark));
                    STATE_SETG1(dsg->mark);
                    NEXT_OUT(3);
                }
                else {
                    WRITEBYTE4(ESC, '$', ')', ESCMARK(dsg->mark));
                    STATE_SETG1(dsg->mark);
                    NEXT_OUT(4);
                }
            }
            if (!STATE_GETFLAG(F_SHIFTED)) {
                WRITEBYTE1(SO);
                STATE_SETFLAG(F_SHIFTED);
                NEXT_OUT(1);
            }
            break;
        default: /* G2 and G3 is not supported: no encoding in
                  * CJKCodecs are using them yet */
            return MBERR_INTERNAL;
        }

        if (dsg->width == 1) {
            WRITEBYTE1((unsigned char)encoded);
            NEXT_OUT(1);
        }
        else {
            WRITEBYTE2(encoded >> 8, encoded & 0xff);
            NEXT_OUT(2);
        }
        NEXT_INCHAR(insize);
    }

    return 0;
}

DECODER_INIT(iso2022)
{
    STATE_CLEARFLAGS();
    STATE_SETG0(CHARSET_ASCII);
    STATE_SETG1(CHARSET_ASCII);
    STATE_SETG2(CHARSET_ASCII);
    return 0;
}

DECODER_RESET(iso2022)
{
    STATE_SETG0(CHARSET_ASCII);
    STATE_CLEARFLAG(F_SHIFTED);
    return 0;
}

static Py_ssize_t
iso2022processesc(const void *config, MultibyteCodec_State *state,
                  const unsigned char **inbuf, Py_ssize_t *inleft)
{
    unsigned char charset, designation;
    Py_ssize_t i, esclen = 0;

    for (i = 1;i < MAX_ESCSEQLEN;i++) {
        if (i >= *inleft)
            return MBERR_TOOFEW;
        if (IS_ESCEND((*inbuf)[i])) {
            esclen = i + 1;
            break;
        }
        else if (CONFIG_ISSET(USE_JISX0208_EXT) && i+1 < *inleft &&
                 (*inbuf)[i] == '&' && (*inbuf)[i+1] == '@') {
            i += 2;
        }
    }

    switch (esclen) {
    case 0:
        return 1; /* unterminated escape sequence */
    case 3:
        if (INBYTE2 == '$') {
            charset = INBYTE3 | CHARSET_DBCS;
            designation = 0;
        }
        else {
            charset = INBYTE3;
            if (INBYTE2 == '(')
                designation = 0;
            else if (INBYTE2 == ')')
                designation = 1;
            else if (CONFIG_ISSET(USE_G2) && INBYTE2 == '.')
                designation = 2;
            else
                return 3;
        }
        break;
    case 4:
        if (INBYTE2 != '$')
            return 4;

        charset = INBYTE4 | CHARSET_DBCS;
        if (INBYTE3 == '(')
            designation = 0;
        else if (INBYTE3 == ')')
            designation = 1;
        else
            return 4;
        break;
    case 6: /* designation with prefix */
        if (CONFIG_ISSET(USE_JISX0208_EXT) &&
            (*inbuf)[3] == ESC && (*inbuf)[4] == '$' &&
            (*inbuf)[5] == 'B') {
            charset = 'B' | CHARSET_DBCS;
            designation = 0;
        }
        else
            return 6;
        break;
    default:
        return esclen;
    }

    /* raise error when the charset is not designated for this encoding */
    if (charset != CHARSET_ASCII) {
        const struct iso2022_designation *dsg;

        for (dsg = CONFIG_DESIGNATIONS; dsg->mark; dsg++) {
            if (dsg->mark == charset)
                break;
        }
        if (!dsg->mark)
            return esclen;
    }

    STATE_SETG(designation, charset);
    *inleft -= esclen;
    (*inbuf) += esclen;
    return 0;
}

#define ISO8859_7_DECODE(c, writer)                                \
    if ((c) < 0xa0) {                                              \
        OUTCHAR(c);                                                \
    } else if ((c) < 0xc0 && (0x288f3bc9L & (1L << ((c)-0xa0)))) { \
        OUTCHAR(c);                                                \
    } else if ((c) >= 0xb4 && (c) <= 0xfe && ((c) >= 0xd4 ||       \
             (0xbffffd77L & (1L << ((c)-0xb4))))) {                \
        OUTCHAR(0x02d0 + (c));                                     \
    } else if ((c) == 0xa1) {                                      \
        OUTCHAR(0x2018);                                           \
    } else if ((c) == 0xa2) {                                      \
        OUTCHAR(0x2019);                                           \
    } else if ((c) == 0xaf) {                                      \
        OUTCHAR(0x2015);                                           \
    }

static Py_ssize_t
iso2022processg2(const void *config, MultibyteCodec_State *state,
                 const unsigned char **inbuf, Py_ssize_t *inleft,
                 _PyUnicodeWriter *writer)
{
    /* not written to use encoder, decoder functions because only few
     * encodings use G2 designations in CJKCodecs */
    if (STATE_G2 == CHARSET_ISO8859_1) {
        if (INBYTE3 < 0x80)
            OUTCHAR(INBYTE3 + 0x80);
        else
            return 3;
    }
    else if (STATE_G2 == CHARSET_ISO8859_7) {
        ISO8859_7_DECODE(INBYTE3 ^ 0x80, writer)
        else
            return 3;
    }
    else if (STATE_G2 == CHARSET_ASCII) {
        if (INBYTE3 & 0x80)
            return 3;
        else
            OUTCHAR(INBYTE3);
    }
    else
        return MBERR_INTERNAL;

    (*inbuf) += 3;
    *inleft -= 3;
    return 0;
}

DECODER(iso2022)
{
    const struct iso2022_designation *dsgcache = NULL;

    while (inleft > 0) {
        unsigned char c = INBYTE1;
        Py_ssize_t err;

        if (STATE_GETFLAG(F_ESCTHROUGHOUT)) {
            /* ESC throughout mode:
             * for non-iso2022 escape sequences */
            OUTCHAR(c); /* assume as ISO-8859-1 */
            NEXT_IN(1);
            if (IS_ESCEND(c)) {
                STATE_CLEARFLAG(F_ESCTHROUGHOUT);
            }
            continue;
        }

        switch (c) {
        case ESC:
            REQUIRE_INBUF(2);
            if (IS_ISO2022ESC(INBYTE2)) {
                err = iso2022processesc(config, state,
                                        inbuf, &inleft);
                if (err != 0)
                    return err;
            }
            else if (CONFIG_ISSET(USE_G2) && INBYTE2 == 'N') {/* SS2 */
                REQUIRE_INBUF(3);
                err = iso2022processg2(config, state,
                                       inbuf, &inleft, writer);
                if (err != 0)
                    return err;
            }
            else {
                OUTCHAR(ESC);
                STATE_SETFLAG(F_ESCTHROUGHOUT);
                NEXT_IN(1);
            }
            break;
        case SI:
            if (CONFIG_ISSET(NO_SHIFT))
                goto bypass;
            STATE_CLEARFLAG(F_SHIFTED);
            NEXT_IN(1);
            break;
        case SO:
            if (CONFIG_ISSET(NO_SHIFT))
                goto bypass;
            STATE_SETFLAG(F_SHIFTED);
            NEXT_IN(1);
            break;
        case LF:
            STATE_CLEARFLAG(F_SHIFTED);
            OUTCHAR(LF);
            NEXT_IN(1);
            break;
        default:
            if (c < 0x20) /* C0 */
                goto bypass;
            else if (c >= 0x80)
                return 1;
            else {
                const struct iso2022_designation *dsg;
                unsigned char charset;
                Py_UCS4 decoded;

                if (STATE_GETFLAG(F_SHIFTED))
                    charset = STATE_G1;
                else
                    charset = STATE_G0;

                if (charset == CHARSET_ASCII) {
bypass:
                    OUTCHAR(c);
                    NEXT_IN(1);
                    break;
                }

                if (dsgcache != NULL &&
                    dsgcache->mark == charset)
                        dsg = dsgcache;
                else {
                    for (dsg = CONFIG_DESIGNATIONS;
                         dsg->mark != charset
#ifdef Py_DEBUG
                            && dsg->mark != '\0'
#endif
                         ; dsg++)
                    {
                        /* noop */
                    }
                    assert(dsg->mark != '\0');
                    dsgcache = dsg;
                }

                REQUIRE_INBUF(dsg->width);
                decoded = dsg->decoder(*inbuf);
                if (decoded == MAP_UNMAPPABLE)
                    return dsg->width;

                if (decoded < 0x10000) {
                    OUTCHAR(decoded);
                }
                else if (decoded < 0x30000) {
                    OUTCHAR(decoded);
                }
                else { /* JIS X 0213 pairs */
                    OUTCHAR2(decoded >> 16, decoded & 0xffff);
                }
                NEXT_IN(dsg->width);
            }
            break;
        }
    }
    return 0;
}

/*-*- mapping table holders -*-*/

#define ENCMAP(enc) static const encode_map *enc##_encmap = NULL;
#define DECMAP(enc) static const decode_map *enc##_decmap = NULL;

/* kr */
ENCMAP(cp949)
DECMAP(ksx1001)

/* jp */
ENCMAP(jisxcommon)
DECMAP(jisx0208)
DECMAP(jisx0212)
ENCMAP(jisx0213_bmp)
DECMAP(jisx0213_1_bmp)
DECMAP(jisx0213_2_bmp)
ENCMAP(jisx0213_emp)
DECMAP(jisx0213_1_emp)
DECMAP(jisx0213_2_emp)

/* cn */
ENCMAP(gbcommon)
DECMAP(gb2312)

/* tw */

/*-*- mapping access functions -*-*/

static int
ksx1001_init(void)
{
    static int initialized = 0;

    if (!initialized && (
                    IMPORT_MAP(kr, cp949, &cp949_encmap, NULL) ||
                    IMPORT_MAP(kr, ksx1001, NULL, &ksx1001_decmap)))
        return -1;
    initialized = 1;
    return 0;
}

static Py_UCS4
ksx1001_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (TRYMAP_DEC(ksx1001, u, data[0], data[1]))
        return u;
    else
        return MAP_UNMAPPABLE;
}

static DBCHAR
ksx1001_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    assert(*length == 1);
    if (*data < 0x10000) {
        if (TRYMAP_ENC(cp949, coded, *data)) {
            if (!(coded & 0x8000))
                return coded;
        }
    }
    return MAP_UNMAPPABLE;
}

static int
jisx0208_init(void)
{
    static int initialized = 0;

    if (!initialized && (
                    IMPORT_MAP(jp, jisxcommon, &jisxcommon_encmap, NULL) ||
                    IMPORT_MAP(jp, jisx0208, NULL, &jisx0208_decmap)))
        return -1;
    initialized = 1;
    return 0;
}

static Py_UCS4
jisx0208_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (data[0] == 0x21 && data[1] == 0x40) /* F/W REVERSE SOLIDUS */
        return 0xff3c;
    else if (TRYMAP_DEC(jisx0208, u, data[0], data[1]))
        return u;
    else
        return MAP_UNMAPPABLE;
}

static DBCHAR
jisx0208_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    assert(*length == 1);
    if (*data < 0x10000) {
        if (*data == 0xff3c) /* F/W REVERSE SOLIDUS */
            return 0x2140;
        else if (TRYMAP_ENC(jisxcommon, coded, *data)) {
            if (!(coded & 0x8000))
                return coded;
        }
    }
    return MAP_UNMAPPABLE;
}

static int
jisx0212_init(void)
{
    static int initialized = 0;

    if (!initialized && (
                    IMPORT_MAP(jp, jisxcommon, &jisxcommon_encmap, NULL) ||
                    IMPORT_MAP(jp, jisx0212, NULL, &jisx0212_decmap)))
        return -1;
    initialized = 1;
    return 0;
}

static Py_UCS4
jisx0212_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (TRYMAP_DEC(jisx0212, u, data[0], data[1]))
        return u;
    else
        return MAP_UNMAPPABLE;
}

static DBCHAR
jisx0212_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    assert(*length == 1);
    if (*data < 0x10000) {
        if (TRYMAP_ENC(jisxcommon, coded, *data)) {
            if (coded & 0x8000)
                return coded & 0x7fff;
        }
    }
    return MAP_UNMAPPABLE;
}

static int
jisx0213_init(void)
{
    static int initialized = 0;

    if (!initialized && (
                    jisx0208_init() ||
                    IMPORT_MAP(jp, jisx0213_bmp,
                               &jisx0213_bmp_encmap, NULL) ||
                    IMPORT_MAP(jp, jisx0213_1_bmp,
                               NULL, &jisx0213_1_bmp_decmap) ||
                    IMPORT_MAP(jp, jisx0213_2_bmp,
                               NULL, &jisx0213_2_bmp_decmap) ||
                    IMPORT_MAP(jp, jisx0213_emp,
                               &jisx0213_emp_encmap, NULL) ||
                    IMPORT_MAP(jp, jisx0213_1_emp,
                               NULL, &jisx0213_1_emp_decmap) ||
                    IMPORT_MAP(jp, jisx0213_2_emp,
                               NULL, &jisx0213_2_emp_decmap) ||
                    IMPORT_MAP(jp, jisx0213_pair, &jisx0213_pair_encmap,
                               &jisx0213_pair_decmap)))
        return -1;
    initialized = 1;
    return 0;
}

#define config ((void *)2000)
static Py_UCS4
jisx0213_2000_1_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    EMULATE_JISX0213_2000_DECODE_PLANE1(u, data[0], data[1])
    else if (data[0] == 0x21 && data[1] == 0x40) /* F/W REVERSE SOLIDUS */
        return 0xff3c;
    else if (TRYMAP_DEC(jisx0208, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_1_bmp, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_1_emp, u, data[0], data[1]))
        u |= 0x20000;
    else if (TRYMAP_DEC(jisx0213_pair, u, data[0], data[1]))
        ;
    else
        return MAP_UNMAPPABLE;
    return u;
}

static Py_UCS4
jisx0213_2000_2_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    EMULATE_JISX0213_2000_DECODE_PLANE2_CHAR(u, data[0], data[1])
    if (TRYMAP_DEC(jisx0213_2_bmp, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_2_emp, u, data[0], data[1]))
        u |= 0x20000;
    else
        return MAP_UNMAPPABLE;
    return u;
}
#undef config

static Py_UCS4
jisx0213_2004_1_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (data[0] == 0x21 && data[1] == 0x40) /* F/W REVERSE SOLIDUS */
        return 0xff3c;
    else if (TRYMAP_DEC(jisx0208, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_1_bmp, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_1_emp, u, data[0], data[1]))
        u |= 0x20000;
    else if (TRYMAP_DEC(jisx0213_pair, u, data[0], data[1]))
        ;
    else
        return MAP_UNMAPPABLE;
    return u;
}

static Py_UCS4
jisx0213_2004_2_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (TRYMAP_DEC(jisx0213_2_bmp, u, data[0], data[1]))
        ;
    else if (TRYMAP_DEC(jisx0213_2_emp, u, data[0], data[1]))
        u |= 0x20000;
    else
        return MAP_UNMAPPABLE;
    return u;
}

static DBCHAR
jisx0213_encoder(const Py_UCS4 *data, Py_ssize_t *length, void *config)
{
    DBCHAR coded;

    switch (*length) {
    case 1: /* first character */
        if (*data >= 0x10000) {
            if ((*data) >> 16 == 0x20000 >> 16) {
                EMULATE_JISX0213_2000_ENCODE_EMP(coded, *data)
                else if (TRYMAP_ENC(jisx0213_emp, coded, (*data) & 0xffff))
                    return coded;
            }
            return MAP_UNMAPPABLE;
        }

        EMULATE_JISX0213_2000_ENCODE_BMP(coded, *data)
        else if (TRYMAP_ENC(jisx0213_bmp, coded, *data)) {
            if (coded == MULTIC)
                return MAP_MULTIPLE_AVAIL;
        }
        else if (TRYMAP_ENC(jisxcommon, coded, *data)) {
            if (coded & 0x8000)
                return MAP_UNMAPPABLE;
        }
        else
            return MAP_UNMAPPABLE;
        return coded;

    case 2: /* second character of unicode pair */
        coded = find_pairencmap((ucs2_t)data[0], (ucs2_t)data[1],
                                jisx0213_pair_encmap, JISX0213_ENCPAIRS);
        if (coded == DBCINV) {
            *length = 1;
            coded = find_pairencmap((ucs2_t)data[0], 0,
                      jisx0213_pair_encmap, JISX0213_ENCPAIRS);
            if (coded == DBCINV)
                return MAP_UNMAPPABLE;
        }
        else
            return coded;

    case -1: /* flush unterminated */
        *length = 1;
        coded = find_pairencmap((ucs2_t)data[0], 0,
                                jisx0213_pair_encmap, JISX0213_ENCPAIRS);
        if (coded == DBCINV)
            return MAP_UNMAPPABLE;
        else
            return coded;
        break;

    default:
        return MAP_UNMAPPABLE;
    }
}

static DBCHAR
jisx0213_2000_1_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded = jisx0213_encoder(data, length, (void *)2000);
    if (coded == MAP_UNMAPPABLE || coded == MAP_MULTIPLE_AVAIL)
        return coded;
    else if (coded & 0x8000)
        return MAP_UNMAPPABLE;
    else
        return coded;
}

static DBCHAR
jisx0213_2000_1_encoder_paironly(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    Py_ssize_t ilength = *length;

    coded = jisx0213_encoder(data, length, (void *)2000);
    switch (ilength) {
    case 1:
        if (coded == MAP_MULTIPLE_AVAIL)
            return MAP_MULTIPLE_AVAIL;
        else
            return MAP_UNMAPPABLE;
    case 2:
        if (*length != 2)
            return MAP_UNMAPPABLE;
        else
            return coded;
    default:
        return MAP_UNMAPPABLE;
    }
}

static DBCHAR
jisx0213_2000_2_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded = jisx0213_encoder(data, length, (void *)2000);
    if (coded == MAP_UNMAPPABLE || coded == MAP_MULTIPLE_AVAIL)
        return coded;
    else if (coded & 0x8000)
        return coded & 0x7fff;
    else
        return MAP_UNMAPPABLE;
}

static DBCHAR
jisx0213_2004_1_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded = jisx0213_encoder(data, length, NULL);
    if (coded == MAP_UNMAPPABLE || coded == MAP_MULTIPLE_AVAIL)
        return coded;
    else if (coded & 0x8000)
        return MAP_UNMAPPABLE;
    else
        return coded;
}

static DBCHAR
jisx0213_2004_1_encoder_paironly(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    Py_ssize_t ilength = *length;

    coded = jisx0213_encoder(data, length, NULL);
    switch (ilength) {
    case 1:
        if (coded == MAP_MULTIPLE_AVAIL)
            return MAP_MULTIPLE_AVAIL;
        else
            return MAP_UNMAPPABLE;
    case 2:
        if (*length != 2)
            return MAP_UNMAPPABLE;
        else
            return coded;
    default:
        return MAP_UNMAPPABLE;
    }
}

static DBCHAR
jisx0213_2004_2_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded = jisx0213_encoder(data, length, NULL);
    if (coded == MAP_UNMAPPABLE || coded == MAP_MULTIPLE_AVAIL)
        return coded;
    else if (coded & 0x8000)
        return coded & 0x7fff;
    else
        return MAP_UNMAPPABLE;
}

static Py_UCS4
jisx0201_r_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    JISX0201_R_DECODE_CHAR(*data, u)
    else
        return MAP_UNMAPPABLE;
    return u;
}

static DBCHAR
jisx0201_r_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    JISX0201_R_ENCODE(*data, coded)
    else
        return MAP_UNMAPPABLE;
    return coded;
}

static Py_UCS4
jisx0201_k_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    JISX0201_K_DECODE_CHAR(*data ^ 0x80, u)
    else
        return MAP_UNMAPPABLE;
    return u;
}

static DBCHAR
jisx0201_k_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    JISX0201_K_ENCODE(*data, coded)
    else
        return MAP_UNMAPPABLE;
    return coded - 0x80;
}

static int
gb2312_init(void)
{
    static int initialized = 0;

    if (!initialized && (
                    IMPORT_MAP(cn, gbcommon, &gbcommon_encmap, NULL) ||
                    IMPORT_MAP(cn, gb2312, NULL, &gb2312_decmap)))
        return -1;
    initialized = 1;
    return 0;
}

static Py_UCS4
gb2312_decoder(const unsigned char *data)
{
    Py_UCS4 u;
    if (TRYMAP_DEC(gb2312, u, data[0], data[1]))
        return u;
    else
        return MAP_UNMAPPABLE;
}

static DBCHAR
gb2312_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    DBCHAR coded;
    assert(*length == 1);
    if (*data < 0x10000) {
        if (TRYMAP_ENC(gbcommon, coded, *data)) {
            if (!(coded & 0x8000))
                return coded;
        }
    }
    return MAP_UNMAPPABLE;
}


static Py_UCS4
dummy_decoder(const unsigned char *data)
{
    return MAP_UNMAPPABLE;
}

static DBCHAR
dummy_encoder(const Py_UCS4 *data, Py_ssize_t *length)
{
    return MAP_UNMAPPABLE;
}

/*-*- registry tables -*-*/

#define REGISTRY_KSX1001_G0     { CHARSET_KSX1001, 0, 2,                \
                  ksx1001_init,                                         \
                  ksx1001_decoder, ksx1001_encoder }
#define REGISTRY_KSX1001_G1     { CHARSET_KSX1001, 1, 2,                \
                  ksx1001_init,                                         \
                  ksx1001_decoder, ksx1001_encoder }
#define REGISTRY_JISX0201_R     { CHARSET_JISX0201_R, 0, 1,             \
                  NULL,                                                 \
                  jisx0201_r_decoder, jisx0201_r_encoder }
#define REGISTRY_JISX0201_K     { CHARSET_JISX0201_K, 0, 1,             \
                  NULL,                                                 \
                  jisx0201_k_decoder, jisx0201_k_encoder }
#define REGISTRY_JISX0208       { CHARSET_JISX0208, 0, 2,               \
                  jisx0208_init,                                        \
                  jisx0208_decoder, jisx0208_encoder }
#define REGISTRY_JISX0208_O     { CHARSET_JISX0208_O, 0, 2,             \
                  jisx0208_init,                                        \
                  jisx0208_decoder, jisx0208_encoder }
#define REGISTRY_JISX0212       { CHARSET_JISX0212, 0, 2,               \
                  jisx0212_init,                                        \
                  jisx0212_decoder, jisx0212_encoder }
#define REGISTRY_JISX0213_2000_1 { CHARSET_JISX0213_2000_1, 0, 2,       \
                  jisx0213_init,                                        \
                  jisx0213_2000_1_decoder,                              \
                  jisx0213_2000_1_encoder }
#define REGISTRY_JISX0213_2000_1_PAIRONLY { CHARSET_JISX0213_2000_1, 0, 2, \
                  jisx0213_init,                                        \
                  jisx0213_2000_1_decoder,                              \
                  jisx0213_2000_1_encoder_paironly }
#define REGISTRY_JISX0213_2000_2 { CHARSET_JISX0213_2, 0, 2,            \
                  jisx0213_init,                                        \
                  jisx0213_2000_2_decoder,                              \
                  jisx0213_2000_2_encoder }
#define REGISTRY_JISX0213_2004_1 { CHARSET_JISX0213_2004_1, 0, 2,       \
                  jisx0213_init,                                        \
                  jisx0213_2004_1_decoder,                              \
                  jisx0213_2004_1_encoder }
#define REGISTRY_JISX0213_2004_1_PAIRONLY { CHARSET_JISX0213_2004_1, 0, 2, \
                  jisx0213_init,                                        \
                  jisx0213_2004_1_decoder,                              \
                  jisx0213_2004_1_encoder_paironly }
#define REGISTRY_JISX0213_2004_2 { CHARSET_JISX0213_2, 0, 2,            \
                  jisx0213_init,                                        \
                  jisx0213_2004_2_decoder,                              \
                  jisx0213_2004_2_encoder }
#define REGISTRY_GB2312         { CHARSET_GB2312, 0, 2,                 \
                  gb2312_init,                                          \
                  gb2312_decoder, gb2312_encoder }
#define REGISTRY_CNS11643_1     { CHARSET_CNS11643_1, 1, 2,             \
                  cns11643_init,                                        \
                  cns11643_1_decoder, cns11643_1_encoder }
#define REGISTRY_CNS11643_2     { CHARSET_CNS11643_2, 2, 2,             \
                  cns11643_init,                                        \
                  cns11643_2_decoder, cns11643_2_encoder }
#define REGISTRY_ISO8859_1      { CHARSET_ISO8859_1, 2, 1,              \
                  NULL, dummy_decoder, dummy_encoder }
#define REGISTRY_ISO8859_7      { CHARSET_ISO8859_7, 2, 1,              \
                  NULL, dummy_decoder, dummy_encoder }
#define REGISTRY_SENTINEL       { 0, }
#define CONFIGDEF(var, attrs)                                           \
    static const struct iso2022_config iso2022_##var##_config = {       \
        attrs, iso2022_##var##_designations                             \
    };

static const struct iso2022_designation iso2022_kr_designations[] = {
    REGISTRY_KSX1001_G1, REGISTRY_SENTINEL
};
CONFIGDEF(kr, 0)

static const struct iso2022_designation iso2022_jp_designations[] = {
    REGISTRY_JISX0208, REGISTRY_JISX0201_R, REGISTRY_JISX0208_O,
    REGISTRY_SENTINEL
};
CONFIGDEF(jp, NO_SHIFT | USE_JISX0208_EXT)

static const struct iso2022_designation iso2022_jp_1_designations[] = {
    REGISTRY_JISX0208, REGISTRY_JISX0212, REGISTRY_JISX0201_R,
    REGISTRY_JISX0208_O, REGISTRY_SENTINEL
};
CONFIGDEF(jp_1, NO_SHIFT | USE_JISX0208_EXT)

static const struct iso2022_designation iso2022_jp_2_designations[] = {
    REGISTRY_JISX0208, REGISTRY_JISX0212, REGISTRY_KSX1001_G0,
    REGISTRY_GB2312, REGISTRY_JISX0201_R, REGISTRY_JISX0208_O,
    REGISTRY_ISO8859_1, REGISTRY_ISO8859_7, REGISTRY_SENTINEL
};
CONFIGDEF(jp_2, NO_SHIFT | USE_G2 | USE_JISX0208_EXT)

static const struct iso2022_designation iso2022_jp_2004_designations[] = {
    REGISTRY_JISX0213_2004_1_PAIRONLY, REGISTRY_JISX0208,
    REGISTRY_JISX0213_2004_1, REGISTRY_JISX0213_2004_2, REGISTRY_SENTINEL
};
CONFIGDEF(jp_2004, NO_SHIFT | USE_JISX0208_EXT)

static const struct iso2022_designation iso2022_jp_3_designations[] = {
    REGISTRY_JISX0213_2000_1_PAIRONLY, REGISTRY_JISX0208,
    REGISTRY_JISX0213_2000_1, REGISTRY_JISX0213_2000_2, REGISTRY_SENTINEL
};
CONFIGDEF(jp_3, NO_SHIFT | USE_JISX0208_EXT)

static const struct iso2022_designation iso2022_jp_ext_designations[] = {
    REGISTRY_JISX0208, REGISTRY_JISX0212, REGISTRY_JISX0201_R,
    REGISTRY_JISX0201_K, REGISTRY_JISX0208_O, REGISTRY_SENTINEL
};
CONFIGDEF(jp_ext, NO_SHIFT | USE_JISX0208_EXT)


BEGIN_MAPPINGS_LIST
  /* no mapping table here */
END_MAPPINGS_LIST

#define ISO2022_CODEC(variation) {              \
    "iso2022_" #variation,                      \
    &iso2022_##variation##_config,              \
    iso2022_codec_init,                         \
    _STATEFUL_METHODS(iso2022)                  \
},

BEGIN_CODECS_LIST
  ISO2022_CODEC(kr)
  ISO2022_CODEC(jp)
  ISO2022_CODEC(jp_1)
  ISO2022_CODEC(jp_2)
  ISO2022_CODEC(jp_2004)
  ISO2022_CODEC(jp_3)
  ISO2022_CODEC(jp_ext)
END_CODECS_LIST

I_AM_A_MODULE_FOR(iso2022)
