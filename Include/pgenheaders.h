#ifndef Py_PGENHEADERS_H
#define Py_PGENHEADERS_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Include files and extern declarations used by most of the parser. */

#include "config.h"

/* config.h may or may not define DL_IMPORT */
#ifndef DL_IMPORT	/* declarations for DLL import/export */
#define DL_IMPORT(RTYPE) RTYPE
#endif

#include <stdio.h>
#include <string.h>

#ifdef THINK_C
#define label label_
#undef label
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include "myproto.h"
#include "mymalloc.h"

#include "pydebug.h"

#ifdef HAVE_STDARG_PROTOTYPES
DL_IMPORT(void) PySys_WriteStdout(const char *format, ...);
DL_IMPORT(void) PySys_WriteStderr(const char *format, ...);
#else
/* Better to have no prototypes at all for varargs functions in this case */
DL_IMPORT(void) PySys_WriteStdout();
DL_IMPORT(void) PySys_WriteStderr();
#endif

#define addarc _Py_addarc
#define addbit _Py_addbit
#define adddfa _Py_adddfa
#define addfirstsets _Py_addfirstsets
#define addlabel _Py_addlabel
#define addstate _Py_addstate
#define delbitset _Py_delbitset
#define dumptree _Py_dumptree
#define findlabel _Py_findlabel
#define mergebitset _Py_mergebitset
#define meta_grammar _Py_meta_grammar
#define newbitset _Py_newbitset
#define newgrammar _Py_newgrammar
#define pgen _Py_pgen
#define printgrammar _Py_printgrammar
#define printnonterminals _Py_printnonterminals
#define printtree _Py_printtree
#define samebitset _Py_samebitset
#define showtree _Py_showtree
#define tok_dump _Py_tok_dump
#define translatelabels _Py_translatelabels

#ifdef __cplusplus
}
#endif
#endif /* !Py_PGENHEADERS_H */
