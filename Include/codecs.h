#ifndef Py_CODECREGISTRY_H
#define Py_CODECREGISTRY_H
#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------

   Python Codec Registry and support functions


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

   ------------------------------------------------------------------------ */

extern DL_IMPORT(int) PyCodec_Register(
       PyObject *search_function
       );

extern DL_IMPORT(PyObject *) _PyCodec_Lookup(
       const char *encoding
       );

extern DL_IMPORT(PyObject *) PyCodec_Encoder(
       const char *encoding
       );

extern DL_IMPORT(PyObject *) PyCodec_Decoder(
       const char *encoding
       );

extern DL_IMPORT(PyObject *) PyCodec_StreamReader(
       const char *encoding,
       PyObject *stream,
       const char *errors
       );

extern DL_IMPORT(PyObject *) PyCodec_Encode(
       PyObject *object,
       const char *encoding,
       const char *errors
       );

extern DL_IMPORT(PyObject *) PyCodec_Decode(
       PyObject *object,
       const char *encoding,
       const char *errors
       );

#ifdef __cplusplus
}
#endif
#endif /* !Py_CODECREGISTRY_H */
