#ifndef Py_PYTHON_H
#define Py_PYTHON_H
/* Since this is a "meta-include" file, no #ifdef __cplusplus / extern "C" { */


/* Enable compiler features; switching on C lib defines doesn't work
   here, because the symbols haven't necessarily been defined yet. */
#ifndef _GNU_SOURCE
# define _GNU_SOURCE	1
#endif

/* Forcing SUSv2 compatibility still produces problems on some
   platforms, True64 and SGI IRIX begin two of them, so for now the
   define is switched off. */
#if 0
#ifndef _XOPEN_SOURCE
# define _XOPEN_SOURCE	500
#endif
#endif

/* Include nearly all Python header files */

#include "patchlevel.h"
#include "config.h"

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

/* config.h may or may not define DL_IMPORT */
#ifndef DL_IMPORT	/* declarations for DLL import/export */
#define DL_IMPORT(RTYPE) RTYPE
#endif
#ifndef DL_EXPORT	/* declarations for DLL import/export */
#define DL_EXPORT(RTYPE) RTYPE
#endif

#if defined(__sgi) && defined(WITH_THREAD) && !defined(_SGI_MP_SOURCE)
#define _SGI_MP_SOURCE
#endif

#include <stdio.h>
#ifndef NULL
#   error "Python.h requires that stdio.h define NULL."
#endif

#include <string.h>
#include <errno.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
#include <assert.h>

#include "pyport.h"

#include "pymem.h"

#include "object.h"
#include "objimpl.h"

#include "pydebug.h"

#include "unicodeobject.h"
#include "intobject.h"
#include "longobject.h"
#include "floatobject.h"
#ifndef WITHOUT_COMPLEX
#include "complexobject.h"
#endif
#include "rangeobject.h"
#include "stringobject.h"
#include "bufferobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"
#include "cobject.h"
#include "traceback.h"
#include "sliceobject.h"
#include "cellobject.h"

#include "codecs.h"
#include "pyerrors.h"

#include "pystate.h"

#include "modsupport.h"
#include "ceval.h"
#include "pythonrun.h"
#include "sysmodule.h"
#include "intrcheck.h"
#include "import.h"

#include "abstract.h"

#define PyArg_GetInt(v, a)	PyArg_Parse((v), "i", (a))
#define PyArg_NoArgs(v)		PyArg_Parse(v, "")

/* Convert a possibly signed character to a nonnegative int */
/* XXX This assumes characters are 8 bits wide */
#ifdef __CHAR_UNSIGNED__
#define Py_CHARMASK(c)		(c)
#else
#define Py_CHARMASK(c)		((c) & 0xff)
#endif

#include "pyfpe.h"

/* These definitions must match corresponding definitions in graminit.h.
   There's code in compile.c that checks that they are the same. */
#define Py_single_input 256
#define Py_file_input 257
#define Py_eval_input 258

#ifdef HAVE_PTH
/* GNU pth user-space thread support */
#include <pth.h>
#endif
#endif /* !Py_PYTHON_H */
