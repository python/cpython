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

typedef union {
	void *p;
	int i;
	unsigned char c[8];
	ucs2_t u2[4];
	ucs4_t u4[2];
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
			Py_UNICODE **outbuf, Py_ssize_t outleft);
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

#define MAXDECPENDING	8
typedef struct {
	PyObject_HEAD
	MultibyteCodec *codec;
	MultibyteCodec_State state;
	unsigned char pending[MAXDECPENDING];
	Py_ssize_t pendingsize;
	PyObject *stream, *errors;
} MultibyteStreamReaderObject;

#define MAXENCPENDING	2
typedef struct {
	PyObject_HEAD
	MultibyteCodec *codec;
	MultibyteCodec_State state;
	Py_UNICODE pending[MAXENCPENDING];
	Py_ssize_t pendingsize;
	PyObject *stream, *errors;
} MultibyteStreamWriterObject;

/* positive values for illegal sequences */
#define MBERR_TOOSMALL		(-1) /* insufficient output buffer space */
#define MBERR_TOOFEW		(-2) /* incomplete input buffer */
#define MBERR_INTERNAL		(-3) /* internal runtime error */

#define ERROR_STRICT		(PyObject *)(1)
#define ERROR_IGNORE		(PyObject *)(2)
#define ERROR_REPLACE		(PyObject *)(3)
#define ERROR_MAX		ERROR_REPLACE

#define MBENC_FLUSH		0x0001 /* encode all characters encodable */
#define MBENC_MAX		MBENC_FLUSH

#ifdef __cplusplus
}
#endif
#endif
