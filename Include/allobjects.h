#ifndef Py_ALLOBJECTS_H
#define Py_ALLOBJECTS_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Include nearly all Python header files */

/* Some systems (well, NT anyway!) require special declarations for
 data items imported from dynamic modules.  Note that this defn is
 only turned on for the modules built as DL modules, not for python
 itself.
*/
#define DL_IMPORT /* Save lots of #else/#if's */
#ifdef USE_DL_IMPORT
#ifdef NT
#undef DL_IMPORT
#define DL_IMPORT __declspec(dllimport)
#endif /* NT */
#endif /* USE_DL_IMPORT */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <string.h>

#include "myproto.h"

#include "object.h"
#include "objimpl.h"

#include "accessobject.h"
#include "intobject.h"
#include "longobject.h"
#include "floatobject.h"
#include "rangeobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "dictobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "funcobject.h"
#include "classobject.h"
#include "fileobject.h"

#include "errors.h"
#include "mymalloc.h"

#include "modsupport.h"
#include "ceval.h"

#include "rename1.h"

extern void fatal PROTO((char *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_ALLOBJECTS_H */
