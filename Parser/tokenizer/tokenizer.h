#ifndef Py_TOKENIZER_H
#define Py_TOKENIZER_H

#include "Python.h"

struct tok_state;

/* Return NUL-terminated retained input, or NULL without setting an exception
   for streaming input or interactive input before its first line. */
const char *_PyTokenizer_RetainedSource(const struct tok_state *);

struct tok_state *_PyTokenizer_FromString(const char *, int, int);
struct tok_state *_PyTokenizer_FromUTF8(const char *, int, int);
struct tok_state *_PyTokenizer_FromReadline(PyObject *, const char *);
struct tok_state *_PyTokenizer_FromFile(FILE *, const char*,
                                              const char *, const char *);
/* Return the declared encoding in PyMem-allocated storage, or NULL.
   An exception is set on error. */
char *_PyTokenizer_FindEncodingFilename(int, PyObject *);

#define tok_dump _Py_tok_dump

#endif /* !Py_TOKENIZER_H */
