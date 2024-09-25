#ifndef Py_INTERNAL_CODECS_H
#define Py_INTERNAL_CODECS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_lock.h"          // PyMutex

/* Initialize codecs-related state for the given interpreter, including
   registering the first codec search function. Must be called before any other
   PyCodec-related functions, and while only one thread is active. */
extern PyStatus _PyCodec_InitRegistry(PyInterpreterState *interp);

/* Finalize codecs-related state for the given interpreter. No PyCodec-related
   functions other than PyCodec_Unregister() may be called after this. */
extern void _PyCodec_Fini(PyInterpreterState *interp);

extern PyObject* _PyCodec_Lookup(const char *encoding);

/* Text codec specific encoding and decoding API.

   Checks the encoding against a list of codecs which do not
   implement a str<->bytes encoding before attempting the
   operation.

   Please note that these APIs are internal and should not
   be used in Python C extensions.

   XXX (ncoghlan): should we make these, or something like them, public
   in Python 3.5+?

 */
extern PyObject* _PyCodec_LookupTextEncoding(
   const char *encoding,
   const char *alternate_command);

extern PyObject* _PyCodec_EncodeText(
   PyObject *object,
   const char *encoding,
   const char *errors);

extern PyObject* _PyCodec_DecodeText(
   PyObject *object,
   const char *encoding,
   const char *errors);

/* These two aren't actually text encoding specific, but _io.TextIOWrapper
 * is the only current API consumer.
 */
extern PyObject* _PyCodecInfo_GetIncrementalDecoder(
   PyObject *codec_info,
   const char *errors);

extern PyObject* _PyCodecInfo_GetIncrementalEncoder(
   PyObject *codec_info,
   const char *errors);

// Per-interpreter state used by codecs.c.
struct codecs_state {
    // A list of callable objects used to search for codecs.
    PyObject *search_path;

    // A dict mapping codec names to codecs returned from a callable in
    // search_path.
    PyObject *search_cache;

    // A dict mapping error handling strategies to functions to implement them.
    PyObject *error_registry;

#ifdef Py_GIL_DISABLED
    // Used to safely delete a specific item from search_path.
    PyMutex search_path_mutex;
#endif

    // Whether or not the rest of the state is initialized.
    int initialized;
};

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_CODECS_H */
