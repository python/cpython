/*
 * cjkcodecs.h: common header for cjkcodecs
 *
 * Written by Hye-Shik Chang <perky@FreeBSD.org>
 */

#ifndef _CJKCODECS_H_
#define _CJKCODECS_H_

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "multibytecodec.h"


/* a unicode "undefined" codepoint */
#define UNIINV	0xFFFE

/* internal-use DBCS codepoints which aren't used by any charsets */
#define NOCHAR	0xFFFF
#define MULTIC	0xFFFE
#define DBCINV	0xFFFD

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
	const ucs4_t *map;
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
	ucs4_t uniseq;
	DBCHAR code;
};

static const MultibyteCodec *codec_list;
static const struct dbcs_map *mapping_list;

#define CODEC_INIT(encoding)						\
	static int encoding##_codec_init(const void *config)

#define ENCODER_INIT(encoding)						\
	static int encoding##_encode_init(				\
		MultibyteCodec_State *state, const void *config)
#define ENCODER(encoding)						\
	static Py_ssize_t encoding##_encode(				\
		MultibyteCodec_State *state, const void *config,	\
		const Py_UNICODE **inbuf, Py_ssize_t inleft,		\
		unsigned char **outbuf, Py_ssize_t outleft, int flags)
#define ENCODER_RESET(encoding)						\
	static Py_ssize_t encoding##_encode_reset(			\
		MultibyteCodec_State *state, const void *config,	\
		unsigned char **outbuf, Py_ssize_t outleft)

#define DECODER_INIT(encoding)						\
	static int encoding##_decode_init(				\
		MultibyteCodec_State *state, const void *config)
#define DECODER(encoding)						\
	static Py_ssize_t encoding##_decode(				\
		MultibyteCodec_State *state, const void *config,	\
		const unsigned char **inbuf, Py_ssize_t inleft,		\
		Py_UNICODE **outbuf, Py_ssize_t outleft)
#define DECODER_RESET(encoding)						\
	static Py_ssize_t encoding##_decode_reset(			\
		MultibyteCodec_State *state, const void *config)

#if Py_UNICODE_SIZE == 4
#define UCS4INVALID(code)	\
	if ((code) > 0xFFFF)	\
	return 1;
#else
#define UCS4INVALID(code)	\
	if (0) ;
#endif

#define NEXT_IN(i)				\
	(*inbuf) += (i);			\
	(inleft) -= (i);
#define NEXT_OUT(o)				\
	(*outbuf) += (o);			\
	(outleft) -= (o);
#define NEXT(i, o)				\
	NEXT_IN(i) NEXT_OUT(o)

#define REQUIRE_INBUF(n)			\
	if (inleft < (n))			\
		return MBERR_TOOFEW;
#define REQUIRE_OUTBUF(n)			\
	if (outleft < (n))			\
		return MBERR_TOOSMALL;

#define IN1 ((*inbuf)[0])
#define IN2 ((*inbuf)[1])
#define IN3 ((*inbuf)[2])
#define IN4 ((*inbuf)[3])

#define OUT1(c) ((*outbuf)[0]) = (c);
#define OUT2(c) ((*outbuf)[1]) = (c);
#define OUT3(c) ((*outbuf)[2]) = (c);
#define OUT4(c) ((*outbuf)[3]) = (c);

#define WRITE1(c1)		\
	REQUIRE_OUTBUF(1)	\
	(*outbuf)[0] = (c1);
#define WRITE2(c1, c2)		\
	REQUIRE_OUTBUF(2)	\
	(*outbuf)[0] = (c1);	\
	(*outbuf)[1] = (c2);
#define WRITE3(c1, c2, c3)	\
	REQUIRE_OUTBUF(3)	\
	(*outbuf)[0] = (c1);	\
	(*outbuf)[1] = (c2);	\
	(*outbuf)[2] = (c3);
#define WRITE4(c1, c2, c3, c4)	\
	REQUIRE_OUTBUF(4)	\
	(*outbuf)[0] = (c1);	\
	(*outbuf)[1] = (c2);	\
	(*outbuf)[2] = (c3);	\
	(*outbuf)[3] = (c4);

#if Py_UNICODE_SIZE == 2
# define WRITEUCS4(c)						\
	REQUIRE_OUTBUF(2)					\
	(*outbuf)[0] = 0xd800 + (((c) - 0x10000) >> 10);	\
	(*outbuf)[1] = 0xdc00 + (((c) - 0x10000) & 0x3ff);	\
	NEXT_OUT(2)
#else
# define WRITEUCS4(c)						\
	REQUIRE_OUTBUF(1)					\
	**outbuf = (Py_UNICODE)(c);				\
	NEXT_OUT(1)
#endif

#define _TRYMAP_ENC(m, assi, val)				\
	((m)->map != NULL && (val) >= (m)->bottom &&		\
	    (val)<= (m)->top && ((assi) = (m)->map[(val) -	\
	    (m)->bottom]) != NOCHAR)
#define TRYMAP_ENC_COND(charset, assi, uni)			\
	_TRYMAP_ENC(&charset##_encmap[(uni) >> 8], assi, (uni) & 0xff)
#define TRYMAP_ENC(charset, assi, uni)				\
	if TRYMAP_ENC_COND(charset, assi, uni)

#define _TRYMAP_DEC(m, assi, val)				\
	((m)->map != NULL && (val) >= (m)->bottom &&		\
	    (val)<= (m)->top && ((assi) = (m)->map[(val) -	\
	    (m)->bottom]) != UNIINV)
#define TRYMAP_DEC(charset, assi, c1, c2)			\
	if _TRYMAP_DEC(&charset##_decmap[c1], assi, c2)

#define _TRYMAP_ENC_MPLANE(m, assplane, asshi, asslo, val)	\
	((m)->map != NULL && (val) >= (m)->bottom &&		\
	    (val)<= (m)->top &&					\
	    ((assplane) = (m)->map[((val) - (m)->bottom)*3]) != 0 && \
	    (((asshi) = (m)->map[((val) - (m)->bottom)*3 + 1]), 1) && \
	    (((asslo) = (m)->map[((val) - (m)->bottom)*3 + 2]), 1))
#define TRYMAP_ENC_MPLANE(charset, assplane, asshi, asslo, uni)	\
	if _TRYMAP_ENC_MPLANE(&charset##_encmap[(uni) >> 8], \
			   assplane, asshi, asslo, (uni) & 0xff)
#define TRYMAP_DEC_MPLANE(charset, assi, plane, c1, c2)		\
	if _TRYMAP_DEC(&charset##_decmap[plane][c1], assi, c2)

#if Py_UNICODE_SIZE == 2
#define DECODE_SURROGATE(c)					\
	if (c >> 10 == 0xd800 >> 10) { /* high surrogate */	\
		REQUIRE_INBUF(2)				\
		if (IN2 >> 10 == 0xdc00 >> 10) { /* low surrogate */ \
		    c = 0x10000 + ((ucs4_t)(c - 0xd800) << 10) + \
			((ucs4_t)(IN2) - 0xdc00);		\
		}						\
	}
#define GET_INSIZE(c)	((c) > 0xffff ? 2 : 1)
#else
#define DECODE_SURROGATE(c) {;}
#define GET_INSIZE(c)	1
#endif

#define BEGIN_MAPPINGS_LIST static const struct dbcs_map _mapping_list[] = {
#define MAPPING_ENCONLY(enc) {#enc, (void*)enc##_encmap, NULL},
#define MAPPING_DECONLY(enc) {#enc, NULL, (void*)enc##_decmap},
#define MAPPING_ENCDEC(enc) {#enc, (void*)enc##_encmap, (void*)enc##_decmap},
#define END_MAPPINGS_LIST				\
	{"", NULL, NULL} };				\
	static const struct dbcs_map *mapping_list =	\
		(const struct dbcs_map *)_mapping_list;

#define BEGIN_CODECS_LIST static const MultibyteCodec _codec_list[] = {
#define _STATEFUL_METHODS(enc)		\
	enc##_encode,			\
	enc##_encode_init,		\
	enc##_encode_reset,		\
	enc##_decode,			\
	enc##_decode_init,		\
	enc##_decode_reset,
#define _STATELESS_METHODS(enc)		\
	enc##_encode, NULL, NULL,	\
	enc##_decode, NULL, NULL,
#define CODEC_STATEFUL(enc) {		\
	#enc, NULL, NULL,		\
	_STATEFUL_METHODS(enc)		\
},
#define CODEC_STATELESS(enc) {		\
	#enc, NULL, NULL,		\
	_STATELESS_METHODS(enc)		\
},
#define CODEC_STATELESS_WINIT(enc) {	\
	#enc, NULL,			\
	enc##_codec_init,		\
	_STATELESS_METHODS(enc)		\
},
#define END_CODECS_LIST					\
	{"", NULL,} };					\
	static const MultibyteCodec *codec_list =	\
		(const MultibyteCodec *)_codec_list;

static PyObject *
getmultibytecodec(void)
{
	static PyObject *cofunc = NULL;

	if (cofunc == NULL) {
		PyObject *mod = PyImport_ImportModule("_multibytecodec");
		if (mod == NULL)
			return NULL;
		cofunc = PyObject_GetAttrString(mod, "__create_codec");
		Py_DECREF(mod);
	}
	return cofunc;
}

static PyObject *
getcodec(PyObject *self, PyObject *encoding)
{
	PyObject *codecobj, *r, *cofunc;
	const MultibyteCodec *codec;
	const char *enc;

	if (!PyUnicode_Check(encoding)) {
		PyErr_SetString(PyExc_TypeError,
				"encoding name must be a string.");
		return NULL;
	}
	enc = PyUnicode_AsString(encoding, NULL);
	if (enc == NULL)
		return NULL;

	cofunc = getmultibytecodec();
	if (cofunc == NULL)
		return NULL;

	for (codec = codec_list; codec->encoding[0]; codec++)
		if (strcmp(codec->encoding, enc) == 0)
			break;

	if (codec->encoding[0] == '\0') {
		PyErr_SetString(PyExc_LookupError,
				"no such codec is supported.");
		return NULL;
	}

	codecobj = PyCObject_FromVoidPtr((void *)codec, NULL);
	if (codecobj == NULL)
		return NULL;

	r = PyObject_CallFunctionObjArgs(cofunc, codecobj, NULL);
	Py_DECREF(codecobj);

	return r;
}

static struct PyMethodDef __methods[] = {
	{"getcodec", (PyCFunction)getcodec, METH_O, ""},
	{NULL, NULL},
};

static int
register_maps(PyObject *module)
{
	const struct dbcs_map *h;

	for (h = mapping_list; h->charset[0] != '\0'; h++) {
		char mhname[256] = "__map_";
		int r;
		strcpy(mhname + sizeof("__map_") - 1, h->charset);
		r = PyModule_AddObject(module, mhname,
				PyCObject_FromVoidPtr((void *)h, NULL));
		if (r == -1)
			return -1;
	}
	return 0;
}

#ifdef USING_BINARY_PAIR_SEARCH
static DBCHAR
find_pairencmap(ucs2_t body, ucs2_t modifier,
		const struct pair_encodemap *haystack, int haystacksize)
{
	int pos, min, max;
	ucs4_t value = body << 16 | modifier;

	min = 0;
	max = haystacksize;

	for (pos = haystacksize >> 1; min != max; pos = (min + max) >> 1)
		if (value < haystack[pos].uniseq) {
			if (max == pos) break;
			else max = pos;
		}
		else if (value > haystack[pos].uniseq) {
			if (min == pos) break;
			else min = pos;
		}
		else
			break;

		if (value == haystack[pos].uniseq)
			return haystack[pos].code;
		else
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

	mod = PyImport_ImportModule((char *)modname);
	if (mod == NULL)
		return -1;

	o = PyObject_GetAttrString(mod, (char*)symbol);
	if (o == NULL)
		goto errorexit;
	else if (!PyCObject_Check(o)) {
		PyErr_SetString(PyExc_ValueError,
				"map data must be a CObject.");
		goto errorexit;
	}
	else {
		struct dbcs_map *map;
		map = PyCObject_AsVoidPtr(o);
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

#define I_AM_A_MODULE_FOR(loc)						\
	void								\
	init_codecs_##loc(void)						\
	{								\
		PyObject *m = Py_InitModule("_codecs_" #loc, __methods);\
		if (m != NULL)						\
			(void)register_maps(m);				\
	}

#endif
