/*
 * cjkcodecs.h: common header for cjkcodecs
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#ifndef _CJKCODECS_H_
#define _CJKCODECS_H_

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "multibytecodec.h"


/* a unicode "undefined" code point */
#define UNIINV  0xFFFE

/* internal-use DBCS code points which aren't used by any charsets */
#define NOCHAR  0xFFFF
#define MULTIC  0xFFFE
#define DBCINV  0xFFFD

/* shorter macros to save source size of mapping tables */
#define U UNIINV
#define N NOCHAR
#define M MULTIC
#define D DBCINV

struct dbcs_index {
    const ucs2_t *map;
    unsigned char bottom, top;
};
typedef struct dbcs_index decode_map;

struct widedbcs_index {
    const Py_UCS4 *map;
    unsigned char bottom, top;
};
typedef struct widedbcs_index widedecode_map;

struct unim_index {
    const DBCHAR *map;
    unsigned char bottom, top;
};
typedef struct unim_index encode_map;

struct unim_index_bytebased {
    const unsigned char *map;
    unsigned char bottom, top;
};

struct dbcs_map {
    const char *charset;
    const struct unim_index *encmap;
    const struct dbcs_index *decmap;
};

struct pair_encodemap {
    Py_UCS4 uniseq;
    DBCHAR code;
};

#ifndef CJK_MOD_SPECIFIC_STATE
#define CJK_MOD_SPECIFIC_STATE
#endif

typedef struct _cjk_mod_state {
    int num_mappings;
    int num_codecs;
    struct dbcs_map *mapping_list;
    MultibyteCodec *codec_list;

    CJK_MOD_SPECIFIC_STATE
} cjkcodecs_module_state;

static inline cjkcodecs_module_state *
get_module_state(PyObject *mod)
{
    void *state = PyModule_GetState(mod);
    assert(state != NULL);
    return (cjkcodecs_module_state *)state;
}

#define CODEC_INIT(encoding)                                            \
    static int encoding##_codec_init(const MultibyteCodec *codec)

#define ENCODER_INIT(encoding)                                          \
    static int encoding##_encode_init(                                  \
        MultibyteCodec_State *state, const MultibyteCodec *codec)
#define ENCODER(encoding)                                               \
    static Py_ssize_t encoding##_encode(                                \
        MultibyteCodec_State *state, const MultibyteCodec *codec,       \
        int kind, const void *data,                                     \
        Py_ssize_t *inpos, Py_ssize_t inlen,                            \
        unsigned char **outbuf, Py_ssize_t outleft, int flags)
#define ENCODER_RESET(encoding)                                         \
    static Py_ssize_t encoding##_encode_reset(                          \
        MultibyteCodec_State *state, const MultibyteCodec *codec,       \
        unsigned char **outbuf, Py_ssize_t outleft)

#define DECODER_INIT(encoding)                                          \
    static int encoding##_decode_init(                                  \
        MultibyteCodec_State *state, const MultibyteCodec *codec)
#define DECODER(encoding)                                               \
    static Py_ssize_t encoding##_decode(                                \
        MultibyteCodec_State *state, const MultibyteCodec *codec,       \
        const unsigned char **inbuf, Py_ssize_t inleft,                 \
        _PyUnicodeWriter *writer)
#define DECODER_RESET(encoding)                                         \
    static Py_ssize_t encoding##_decode_reset(                          \
        MultibyteCodec_State *state, const MultibyteCodec *codec)

#define NEXT_IN(i)                              \
    do {                                        \
        (*inbuf) += (i);                        \
        (inleft) -= (i);                        \
    } while (0)
#define NEXT_INCHAR(i)                          \
    do {                                        \
        (*inpos) += (i);                        \
    } while (0)
#define NEXT_OUT(o)                             \
    do {                                        \
        (*outbuf) += (o);                       \
        (outleft) -= (o);                       \
    } while (0)
#define NEXT(i, o)                              \
    do {                                        \
        NEXT_INCHAR(i);                         \
        NEXT_OUT(o);                            \
    } while (0)

#define REQUIRE_INBUF(n)                        \
    do {                                        \
        if (inleft < (n))                       \
            return MBERR_TOOFEW;                \
    } while (0)

#define REQUIRE_OUTBUF(n)                       \
    do {                                        \
        if (outleft < (n))                      \
            return MBERR_TOOSMALL;              \
    } while (0)

#define INBYTE1 ((*inbuf)[0])
#define INBYTE2 ((*inbuf)[1])
#define INBYTE3 ((*inbuf)[2])
#define INBYTE4 ((*inbuf)[3])

#define INCHAR1 (PyUnicode_READ(kind, data, *inpos))
#define INCHAR2 (PyUnicode_READ(kind, data, *inpos + 1))

#define OUTCHAR(c)                                                         \
    do {                                                                   \
        if (_PyUnicodeWriter_WriteChar(writer, (c)) < 0)                   \
            return MBERR_EXCEPTION;                                         \
    } while (0)

#define OUTCHAR2(c1, c2)                                                   \
    do {                                                                   \
        Py_UCS4 _c1 = (c1);                                                \
        Py_UCS4 _c2 = (c2);                                                \
        if (_PyUnicodeWriter_Prepare(writer, 2, Py_MAX(_c1, c2)) < 0)      \
            return MBERR_EXCEPTION;                                        \
        PyUnicode_WRITE(writer->kind, writer->data, writer->pos, _c1);     \
        PyUnicode_WRITE(writer->kind, writer->data, writer->pos + 1, _c2); \
        writer->pos += 2;                                                  \
    } while (0)

#define OUTBYTEI(c, i)                     \
    do {                                   \
        assert((unsigned char)(c) == (c)); \
        ((*outbuf)[i]) = (c);              \
    } while (0)

#define OUTBYTE1(c) OUTBYTEI(c, 0)
#define OUTBYTE2(c) OUTBYTEI(c, 1)
#define OUTBYTE3(c) OUTBYTEI(c, 2)
#define OUTBYTE4(c) OUTBYTEI(c, 3)

#define WRITEBYTE1(c1)              \
    do {                            \
        REQUIRE_OUTBUF(1);          \
        OUTBYTE1(c1);               \
    } while (0)
#define WRITEBYTE2(c1, c2)          \
    do {                            \
        REQUIRE_OUTBUF(2);          \
        OUTBYTE1(c1);               \
        OUTBYTE2(c2);               \
    } while (0)
#define WRITEBYTE3(c1, c2, c3)      \
    do {                            \
        REQUIRE_OUTBUF(3);          \
        OUTBYTE1(c1);               \
        OUTBYTE2(c2);               \
        OUTBYTE3(c3);               \
    } while (0)
#define WRITEBYTE4(c1, c2, c3, c4)  \
    do {                            \
        REQUIRE_OUTBUF(4);          \
        OUTBYTE1(c1);               \
        OUTBYTE2(c2);               \
        OUTBYTE3(c3);               \
        OUTBYTE4(c4);               \
    } while (0)

#define _TRYMAP_ENC(m, assi, val)                               \
    ((m)->map != NULL && (val) >= (m)->bottom &&                \
        (val)<= (m)->top && ((assi) = (m)->map[(val) -          \
        (m)->bottom]) != NOCHAR)
#define TRYMAP_ENC(charset, assi, uni)                     \
    _TRYMAP_ENC(&charset##_encmap[(uni) >> 8], assi, (uni) & 0xff)
#define TRYMAP_ENC_ST(charset, assi, uni) \
    _TRYMAP_ENC(&(codec->modstate->charset##_encmap)[(uni) >> 8], \
                assi, (uni) & 0xff)

#define _TRYMAP_DEC(m, assi, val)                             \
    ((m)->map != NULL &&                                        \
     (val) >= (m)->bottom &&                                    \
     (val)<= (m)->top &&                                        \
     ((assi) = (m)->map[(val) - (m)->bottom]) != UNIINV)
#define TRYMAP_DEC(charset, assi, c1, c2)                     \
    _TRYMAP_DEC(&charset##_decmap[c1], assi, c2)
#define TRYMAP_DEC_ST(charset, assi, c1, c2) \
    _TRYMAP_DEC(&(codec->modstate->charset##_decmap)[c1], assi, c2)

#define BEGIN_MAPPINGS_LIST(NUM)                                    \
static int                                                          \
add_mappings(cjkcodecs_module_state *st)                            \
{                                                                   \
    int idx = 0;                                                    \
    (void)idx;                                                      \
    st->num_mappings = NUM;                                         \
    st->mapping_list = PyMem_Calloc(NUM, sizeof(struct dbcs_map));  \
    if (st->mapping_list == NULL) {                                 \
        return -1;                                                  \
    }

#define MAPPING_ENCONLY(enc) \
    st->mapping_list[idx++] = (struct dbcs_map){#enc, (void*)enc##_encmap, NULL};
#define MAPPING_DECONLY(enc) \
    st->mapping_list[idx++] = (struct dbcs_map){#enc, NULL, (void*)enc##_decmap};
#define MAPPING_ENCDEC(enc) \
    st->mapping_list[idx++] = (struct dbcs_map){#enc, (void*)enc##_encmap, (void*)enc##_decmap};

#define END_MAPPINGS_LIST               \
    assert(st->num_mappings == idx);    \
    return 0;                           \
}

#define BEGIN_CODECS_LIST(NUM)                                  \
static int                                                      \
add_codecs(cjkcodecs_module_state *st)                          \
{                                                               \
    int idx = 0;                                                \
    (void)idx;                                                  \
    st->num_codecs = NUM;                                       \
    st->codec_list = PyMem_Calloc(NUM, sizeof(MultibyteCodec)); \
    if (st->codec_list == NULL) {                               \
        return -1;                                              \
    }

#define _STATEFUL_METHODS(enc)          \
    enc##_encode,                       \
    enc##_encode_init,                  \
    enc##_encode_reset,                 \
    enc##_decode,                       \
    enc##_decode_init,                  \
    enc##_decode_reset,
#define _STATELESS_METHODS(enc)         \
    enc##_encode, NULL, NULL,           \
    enc##_decode, NULL, NULL,

#define NEXT_CODEC \
    st->codec_list[idx++]

#define CODEC_STATEFUL(enc) \
    NEXT_CODEC = (MultibyteCodec){#enc, NULL, NULL, _STATEFUL_METHODS(enc)};
#define CODEC_STATELESS(enc) \
    NEXT_CODEC = (MultibyteCodec){#enc, NULL, NULL, _STATELESS_METHODS(enc)};
#define CODEC_STATELESS_WINIT(enc) \
    NEXT_CODEC = (MultibyteCodec){#enc, NULL, enc##_codec_init, _STATELESS_METHODS(enc)};

#define END_CODECS_LIST                         \
    assert(st->num_codecs == idx);              \
    for (int i = 0; i < st->num_codecs; i++) {  \
        st->codec_list[i].modstate = st;        \
    }                                           \
    return 0;                                   \
}



static PyObject *
getmultibytecodec(void)
{
    return PyImport_ImportModuleAttrString("_multibytecodec", "__create_codec");
}

static void
destroy_codec_capsule(PyObject *capsule)
{
    void *ptr = PyCapsule_GetPointer(capsule, CODEC_CAPSULE);
    codec_capsule *data = (codec_capsule *)ptr;
    Py_DECREF(data->cjk_module);
    PyMem_Free(ptr);
}

static codec_capsule *
capsulate_codec(PyObject *mod, const MultibyteCodec *codec)
{
    codec_capsule *data = PyMem_Malloc(sizeof(codec_capsule));
    if (data == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    data->codec = codec;
    data->cjk_module = Py_NewRef(mod);
    return data;
}

static PyObject *
_getcodec(PyObject *self, const MultibyteCodec *codec)
{
    PyObject *cofunc = getmultibytecodec();
    if (cofunc == NULL) {
        return NULL;
    }

    codec_capsule *data = capsulate_codec(self, codec);
    if (data == NULL) {
        Py_DECREF(cofunc);
        return NULL;
    }
    PyObject *codecobj = PyCapsule_New(data, CODEC_CAPSULE,
                                       destroy_codec_capsule);
    if (codecobj == NULL) {
        PyMem_Free(data);
        Py_DECREF(cofunc);
        return NULL;
    }

    PyObject *res = PyObject_CallOneArg(cofunc, codecobj);
    Py_DECREF(codecobj);
    Py_DECREF(cofunc);
    return res;
}

static PyObject *
getcodec(PyObject *self, PyObject *encoding)
{
    if (!PyUnicode_Check(encoding)) {
        PyErr_SetString(PyExc_TypeError,
                        "encoding name must be a string.");
        return NULL;
    }
    const char *enc = PyUnicode_AsUTF8(encoding);
    if (enc == NULL) {
        return NULL;
    }

    cjkcodecs_module_state *st = get_module_state(self);
    for (int i = 0; i < st->num_codecs; i++) {
        const MultibyteCodec *codec = &st->codec_list[i];
        if (strcmp(codec->encoding, enc) == 0) {
            return _getcodec(self, codec);
        }
    }

    PyErr_SetString(PyExc_LookupError,
                    "no such codec is supported.");
    return NULL;
}

static int add_mappings(cjkcodecs_module_state *);
static int add_codecs(cjkcodecs_module_state *);

static int
register_maps(PyObject *module)
{
    // Init module state.
    cjkcodecs_module_state *st = get_module_state(module);
    if (add_mappings(st) < 0) {
        return -1;
    }
    if (add_codecs(st) < 0) {
        return -1;
    }

    for (int i = 0; i < st->num_mappings; i++) {
        const struct dbcs_map *h = &st->mapping_list[i];
        char mhname[256] = "__map_";
        strcpy(mhname + sizeof("__map_") - 1, h->charset);

        PyObject *capsule = PyCapsule_New((void *)h, MAP_CAPSULE, NULL);
        if (PyModule_Add(module, mhname, capsule) < 0) {
            return -1;
        }
    }
    return 0;
}

#ifdef USING_BINARY_PAIR_SEARCH
static DBCHAR
find_pairencmap(ucs2_t body, ucs2_t modifier,
                const struct pair_encodemap *haystack, int haystacksize)
{
    int pos, min, max;
    Py_UCS4 value = body << 16 | modifier;

    min = 0;
    max = haystacksize;

    for (pos = haystacksize >> 1; min != max; pos = (min + max) >> 1) {
        if (value < haystack[pos].uniseq) {
            if (max != pos) {
                max = pos;
                continue;
            }
        }
        else if (value > haystack[pos].uniseq) {
            if (min != pos) {
                min = pos;
                continue;
            }
        }
        break;
    }

    if (value == haystack[pos].uniseq) {
        return haystack[pos].code;
    }
    return DBCINV;
}
#endif

#ifdef USING_IMPORTED_MAPS
#define IMPORT_MAP(locale, charset, encmap, decmap) \
    importmap("_codecs_" #locale, "__map_" #charset, \
              (const void**)encmap, (const void**)decmap)

static int
importmap(const char *modname, const char *symbol,
          const void **encmap, const void **decmap)
{
    PyObject *o, *mod;

    mod = PyImport_ImportModule(modname);
    if (mod == NULL)
        return -1;

    o = PyObject_GetAttrString(mod, symbol);
    if (o == NULL)
        goto errorexit;
    else if (!PyCapsule_IsValid(o, MAP_CAPSULE)) {
        PyErr_SetString(PyExc_ValueError,
                        "map data must be a Capsule.");
        goto errorexit;
    }
    else {
        struct dbcs_map *map;
        map = PyCapsule_GetPointer(o, MAP_CAPSULE);
        if (encmap != NULL)
            *encmap = map->encmap;
        if (decmap != NULL)
            *decmap = map->decmap;
        Py_DECREF(o);
    }

    Py_DECREF(mod);
    return 0;

errorexit:
    Py_DECREF(mod);
    return -1;
}
#endif

static int
_cjk_exec(PyObject *module)
{
    return register_maps(module);
}

static void
_cjk_free(void *mod)
{
    cjkcodecs_module_state *st = get_module_state((PyObject *)mod);
    PyMem_Free(st->mapping_list);
    PyMem_Free(st->codec_list);
}

static struct PyMethodDef _cjk_methods[] = {
    {"getcodec", getcodec, METH_O, ""},
    {NULL, NULL},
};

static PyModuleDef_Slot _cjk_slots[] = {
    {Py_mod_exec, _cjk_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

#define I_AM_A_MODULE_FOR(loc)                                          \
    static struct PyModuleDef _cjk_module = {                           \
        PyModuleDef_HEAD_INIT,                                          \
        .m_name = "_codecs_"#loc,                                       \
        .m_size = sizeof(cjkcodecs_module_state),                       \
        .m_methods = _cjk_methods,                                      \
        .m_slots = _cjk_slots,                                          \
        .m_free = _cjk_free,                                            \
    };                                                                  \
                                                                        \
    PyMODINIT_FUNC                                                      \
    PyInit__codecs_##loc(void)                                          \
    {                                                                   \
        return PyModuleDef_Init(&_cjk_module);                          \
    }

#endif
