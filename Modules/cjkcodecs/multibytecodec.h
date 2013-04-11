/*
 * multibytecodec.h: Common Multibyte Codec Implementation
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#ifndef _PYTHON_MULTIBYTECODEC_H_
#define _PYTHON_MULTIBYTECODEC_H_
#ifdef __cplusplus
extern "C" {
#endif

#ifdef uint16_t
typedef uint16_t ucs2_t, DBCHAR;
#else
typedef unsigned short ucs2_t, DBCHAR;
#endif

typedef union {
    void *p;
    int i;
    unsigned char c[8];
    ucs2_t u2[4];
    Py_UCS4 u4[2];
} MultibyteCodec_State;

typedef int (*mbcodec_init)(const void *config);
typedef Py_ssize_t (*mbencode_func)(MultibyteCodec_State *state,
                        const void *config,
                        const Py_UNICODE **inbuf, Py_ssize_t inleft,
                        unsigned char **outbuf, Py_ssize_t outleft,
                        int flags);
typedef int (*mbencodeinit_func)(MultibyteCodec_State *state,
                                 const void *config);
typedef Py_ssize_t (*mbencodereset_func)(MultibyteCodec_State *state,
                        const void *config,
                        unsigned char **outbuf, Py_ssize_t outleft);
typedef Py_ssize_t (*mbdecode_func)(MultibyteCodec_State *state,
                        const void *config,
                        const unsigned char **inbuf, Py_ssize_t inleft,
                        _PyUnicodeWriter *writer);
typedef int (*mbdecodeinit_func)(MultibyteCodec_State *state,
                                 const void *config);
typedef Py_ssize_t (*mbdecodereset_func)(MultibyteCodec_State *state,
                                         const void *config);

typedef struct {
    const char *encoding;
    const void *config;
    mbcodec_init codecinit;
    mbencode_func encode;
    mbencodeinit_func encinit;
    mbencodereset_func encreset;
    mbdecode_func decode;
    mbdecodeinit_func decinit;
    mbdecodereset_func decreset;
} MultibyteCodec;

typedef struct {
    PyObject_HEAD
    MultibyteCodec *codec;
} MultibyteCodecObject;

#define MultibyteCodec_Check(op) ((op)->ob_type == &MultibyteCodec_Type)

#define _MultibyteStatefulCodec_HEAD            \
    PyObject_HEAD                               \
    MultibyteCodec *codec;                      \
    MultibyteCodec_State state;                 \
    PyObject *errors;
typedef struct {
    _MultibyteStatefulCodec_HEAD
} MultibyteStatefulCodecContext;

#define MAXENCPENDING   2
#define _MultibyteStatefulEncoder_HEAD          \
    _MultibyteStatefulCodec_HEAD                \
    Py_UNICODE pending[MAXENCPENDING];          \
    Py_ssize_t pendingsize;
typedef struct {
    _MultibyteStatefulEncoder_HEAD
} MultibyteStatefulEncoderContext;

#define MAXDECPENDING   8
#define _MultibyteStatefulDecoder_HEAD          \
    _MultibyteStatefulCodec_HEAD                \
    unsigned char pending[MAXDECPENDING];       \
    Py_ssize_t pendingsize;
typedef struct {
    _MultibyteStatefulDecoder_HEAD
} MultibyteStatefulDecoderContext;

typedef struct {
    _MultibyteStatefulEncoder_HEAD
} MultibyteIncrementalEncoderObject;

typedef struct {
    _MultibyteStatefulDecoder_HEAD
} MultibyteIncrementalDecoderObject;

typedef struct {
    _MultibyteStatefulDecoder_HEAD
    PyObject *stream;
} MultibyteStreamReaderObject;

typedef struct {
    _MultibyteStatefulEncoder_HEAD
    PyObject *stream;
} MultibyteStreamWriterObject;

/* positive values for illegal sequences */
#define MBERR_TOOSMALL          (-1) /* insufficient output buffer space */
#define MBERR_TOOFEW            (-2) /* incomplete input buffer */
#define MBERR_INTERNAL          (-3) /* internal runtime error */

#define ERROR_STRICT            (PyObject *)(1)
#define ERROR_IGNORE            (PyObject *)(2)
#define ERROR_REPLACE           (PyObject *)(3)
#define ERROR_ISCUSTOM(p)       ((p) < ERROR_STRICT || ERROR_REPLACE < (p))
#define ERROR_DECREF(p) do {                    \
    if (p != NULL && ERROR_ISCUSTOM(p)) {       \
        Py_DECREF(p);                           \
    }                                           \
} while (0);

#define MBENC_FLUSH             0x0001 /* encode all characters encodable */
#define MBENC_MAX               MBENC_FLUSH

#define PyMultibyteCodec_CAPSULE_NAME "multibytecodec.__map_*"


#ifdef __cplusplus
}
#endif
#endif
