/*
 * codecentry.h: Common Codec Entry Routines
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 * $CJKCodecs: codecentry.h,v 1.5 2004/01/17 11:26:10 perky Exp $
 */

#ifdef HAVE_ENCODER_INIT
#define ENCODER_INIT_FUNC(encoding)     encoding##_encode_init
#else
#define ENCODER_INIT_FUNC(encoding)     NULL
#endif

#ifdef HAVE_ENCODER_RESET
#define ENCODER_RESET_FUNC(encoding)    encoding##_encode_reset
#else
#define ENCODER_RESET_FUNC(encoding)    NULL
#endif

#ifdef HAVE_DECODER_INIT
#define DECODER_INIT_FUNC(encoding)     encoding##_decode_init
#else
#define DECODER_INIT_FUNC(encoding)     NULL
#endif

#ifdef HAVE_DECODER_RESET
#define DECODER_RESET_FUNC(encoding)    encoding##_decode_reset
#else
#define DECODER_RESET_FUNC(encoding)    NULL
#endif

#ifdef STRICT_BUILD
#define BEGIN_CODEC_REGISTRY(encoding)                      \
    __BEGIN_CODEC_REGISTRY(encoding, init_codecs_##encoding##_strict)
#else
#define BEGIN_CODEC_REGISTRY(encoding)                      \
    __BEGIN_CODEC_REGISTRY(encoding, init_codecs_##encoding)
#endif

#define __BEGIN_CODEC_REGISTRY(encoding, initname)          \
    static MultibyteCodec __codec = {                       \
        #encoding STRICT_SUFX,                              \
        encoding##_encode,                                  \
        ENCODER_INIT_FUNC(encoding),                        \
        ENCODER_RESET_FUNC(encoding),                       \
        encoding##_decode,                                  \
        DECODER_INIT_FUNC(encoding),                        \
        DECODER_RESET_FUNC(encoding),                       \
    };                                                      \
                                                            \
    static struct PyMethodDef __methods[] = {               \
        {NULL, NULL},                                       \
    };                                                      \
                                                            \
    void                                                    \
    initname(void)                                          \
    {                                                       \
        PyObject    *codec;                                 \
        PyObject    *m = NULL, *mod = NULL, *o = NULL;      \
                                                            \
        m = Py_InitModule("_codecs_" #encoding STRICT_SUFX, __methods);

#define MAPOPEN(locale)                                     \
    mod = PyImport_ImportModule("_codecs_mapdata_" #locale);\
    if (mod == NULL) goto errorexit;                        \
    if (
#define IMPORTMAP_ENCDEC(charset)                           \
    importmap(mod, "__map_" #charset, &charset##encmap,     \
        &charset##decmap) ||
#define IMPORTMAP_ENC(charset)                              \
    importmap(mod, "__map_" #charset, &charset##encmap,     \
        NULL) ||
#define IMPORTMAP_DEC(charset)                              \
    importmap(mod, "__map_" #charset, NULL,                 \
        &charset##decmap) ||
#define MAPCLOSE()                                          \
    0) goto errorexit;                                      \
    Py_DECREF(mod);

#define END_CODEC_REGISTRY(encoding)                        \
    mod = PyImport_ImportModule("_multibytecodec");         \
    if (mod == NULL) goto errorexit;                        \
    o = PyObject_GetAttrString(mod, "__create_codec");      \
    if (o == NULL || !PyCallable_Check(o))                  \
        goto errorexit;                                     \
                                                            \
    codec = createcodec(o, &__codec);                       \
    if (codec == NULL)                                      \
        goto errorexit;                                     \
    PyModule_AddObject(m, "codec", codec);                  \
    Py_DECREF(o); Py_DECREF(mod);                           \
                                                            \
    if (PyErr_Occurred())                                   \
        Py_FatalError("can't initialize the _" #encoding    \
            STRICT_SUFX " module");                         \
                                                            \
    return;                                                 \
                                                            \
errorexit:                                                  \
    Py_XDECREF(m);                                          \
    Py_XDECREF(mod);                                        \
    Py_XDECREF(o);                                          \
}

#define CODEC_REGISTRY(encoding)                            \
    BEGIN_CODEC_REGISTRY(encoding)                          \
    END_CODEC_REGISTRY(encoding)

#ifdef USING_BINARY_PAIR_SEARCH
static DBCHAR
find_pairencmap(ucs2_t body, ucs2_t modifier,
                struct pair_encodemap *haystack, int haystacksize)
{
    int      pos, min, max;
    ucs4_t   value = body << 16 | modifier;

    min = 0;
    max = haystacksize;

    for (pos = haystacksize >> 1; min != max; pos = (min + max) >> 1)
        if (value < haystack[pos].uniseq) {
            if (max == pos) break;
            else max = pos;
        } else if (value > haystack[pos].uniseq) {
            if (min == pos) break;
            else min = pos;
        } else
            break;

    if (value == haystack[pos].uniseq)
        return haystack[pos].code;
    else
        return DBCINV;
}
#endif

#ifndef CODEC_WITHOUT_MAPS
static int
importmap(PyObject *mod, const char *symbol,
          const struct unim_index **encmap, const struct dbcs_index **decmap)
{
    PyObject    *o;

    o = PyObject_GetAttrString(mod, (char*)symbol);
    if (o == NULL)
        return -1;
    else if (!PyCObject_Check(o)) {
        PyErr_SetString(PyExc_ValueError, "map data must be a CObject.");
        return -1;
    } else {
        struct dbcs_map *map;
        map = PyCObject_AsVoidPtr(o);
        if (encmap != NULL)
            *encmap = map->encmap;
        if (decmap != NULL)
            *decmap = map->decmap;
        Py_DECREF(o);
    }

    return 0;
}
#endif

static PyObject *
createcodec(PyObject *cofunc, MultibyteCodec *codec)
{
    PyObject    *args, *r;

    args = PyTuple_New(1);
    if (args == NULL) return NULL;
    PyTuple_SET_ITEM(args, 0, PyCObject_FromVoidPtr(codec, NULL));

    r = PyObject_CallObject(cofunc, args);
    Py_DECREF(args);

    return r;
}
