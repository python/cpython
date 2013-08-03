/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
 *
 * See the _sre.c file for information on usage and redistribution.
 */

#ifndef SRE_INCLUDED
#define SRE_INCLUDED

#include "sre_constants.h"

/* size of a code word (must be unsigned short or larger, and
   large enough to hold a UCS4 character) */
#ifdef Py_USING_UNICODE
# define SRE_CODE Py_UCS4
# if SIZEOF_SIZE_T > 4
#  define SRE_MAXREPEAT (~(SRE_CODE)0)
# else
#  define SRE_MAXREPEAT ((SRE_CODE)PY_SSIZE_T_MAX)
# endif
#else
# define SRE_CODE unsigned int
# if SIZEOF_SIZE_T > SIZEOF_INT
#  define SRE_MAXREPEAT (~(SRE_CODE)0)
# else
#  define SRE_MAXREPEAT ((SRE_CODE)PY_SSIZE_T_MAX)
# endif
#endif

typedef struct {
    PyObject_VAR_HEAD
    Py_ssize_t groups; /* must be first! */
    PyObject* groupindex;
    PyObject* indexgroup;
    /* compatibility */
    PyObject* pattern; /* pattern source (or None) */
    int flags; /* flags used when compiling pattern source */
    PyObject *weakreflist; /* List of weak references */
    /* pattern code */
    Py_ssize_t codesize;
    SRE_CODE code[1];
} PatternObject;

#define PatternObject_GetCode(o) (((PatternObject*)(o))->code)

typedef struct {
    PyObject_VAR_HEAD
    PyObject* string; /* link to the target string (must be first) */
    PyObject* regs; /* cached list of matching spans */
    PatternObject* pattern; /* link to the regex (pattern) object */
    Py_ssize_t pos, endpos; /* current target slice */
    Py_ssize_t lastindex; /* last index marker seen by the engine (-1 if none) */
    Py_ssize_t groups; /* number of groups (start/end marks) */
    Py_ssize_t mark[1];
} MatchObject;

typedef unsigned int (*SRE_TOLOWER_HOOK)(unsigned int ch);

/* FIXME: <fl> shouldn't be a constant, really... */
#define SRE_MARK_SIZE 200

typedef struct SRE_REPEAT_T {
    Py_ssize_t count;
    SRE_CODE* pattern; /* points to REPEAT operator arguments */
    void* last_ptr; /* helper to check for infinite loops */
    struct SRE_REPEAT_T *prev; /* points to previous repeat context */
} SRE_REPEAT;

typedef struct {
    /* string pointers */
    void* ptr; /* current position (also end of current slice) */
    void* beginning; /* start of original string */
    void* start; /* start of current slice */
    void* end; /* end of original string */
    /* attributes for the match object */
    PyObject* string;
    Py_ssize_t pos, endpos;
    /* character size */
    int charsize;
    /* registers */
    Py_ssize_t lastindex;
    Py_ssize_t lastmark;
    void* mark[SRE_MARK_SIZE];
    /* dynamically allocated stuff */
    char* data_stack;
    size_t data_stack_size;
    size_t data_stack_base;
    /* current repeat context */
    SRE_REPEAT *repeat;
    /* hooks */
    SRE_TOLOWER_HOOK lower;
} SRE_STATE;

typedef struct {
    PyObject_HEAD
    PyObject* pattern;
    SRE_STATE state;
} ScannerObject;

#endif
