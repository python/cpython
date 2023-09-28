#ifndef STRINGS_H
#define STRINGS_H

#include <Python.h>
#include <pycore_ast.h>
#include "pegen.h"

PyObject *_PyPegen_parse_string(Parser *, Token *);
PyObject *_PyPegen_decode_string(Parser *, int, const char *, size_t, Token *);

#endif
