#ifndef STRINGS_H
#define STRINGS_H

#include <Python.h>
#include <Python-ast.h>
#include "pegen.h"

#define EXPRLIST_N_CACHED  64

typedef struct {
    /* Incrementally build an array of expr_ty, so be used in an
       asdl_seq. Cache some small but reasonably sized number of
       expr_ty's, and then after that start dynamically allocating,
       doubling the number allocated each time. Note that the f-string
       f'{0}a{1}' contains 3 expr_ty's: 2 FormattedValue's, and one
       Constant for the literal 'a'. So you add expr_ty's about twice as
       fast as you add expressions in an f-string. */

    Py_ssize_t allocated;  /* Number we've allocated. */
    Py_ssize_t size;       /* Number we've used. */
    expr_ty    *p;         /* Pointer to the memory we're actually
                              using. Will point to 'data' until we
                              start dynamically allocating. */
    expr_ty    data[EXPRLIST_N_CACHED];
} ExprList;

/* The FstringParser is designed to add a mix of strings and
   f-strings, and concat them together as needed. Ultimately, it
   generates an expr_ty. */
typedef struct {
    PyObject *last_str;
    ExprList expr_list;
    int fmode;
} FstringParser;

void _PyPegen_FstringParser_Init(FstringParser *);
int _PyPegen_parsestr(Parser *, int *, int *, PyObject **,
                      const char **, Py_ssize_t *, Token *);
int _PyPegen_FstringParser_ConcatFstring(Parser *, FstringParser *, const char **,
                                const char *, int, int, Token *, Token *,
                                Token *);
int _PyPegen_FstringParser_ConcatAndDel(FstringParser *, PyObject *);
expr_ty _PyPegen_FstringParser_Finish(Parser *, FstringParser *, Token *, Token *);
void _PyPegen_FstringParser_Dealloc(FstringParser *);

#endif
