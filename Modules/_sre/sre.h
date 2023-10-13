/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * Copyright (c) 1997-2001 by Secret Labs AB.  All rights reserved.
 *
 * See the sre.c file for information on usage and redistribution.
 */

#ifndef SRE_INCLUDED
#define SRE_INCLUDED

#include "sre_constants.h"

/* size of a code word (must be unsigned short or larger, and
   large enough to hold a UCS4 character) */
#define SRE_CODE Py_UCS4
#if SIZEOF_SIZE_T > 4
# define SRE_MAXREPEAT (~(SRE_CODE)0)
# define SRE_MAXGROUPS ((SRE_CODE)INT32_MAX / 2)
#else
# define SRE_MAXREPEAT ((SRE_CODE)PY_SSIZE_T_MAX)
# define SRE_MAXGROUPS ((SRE_CODE)PY_SSIZE_T_MAX / SIZEOF_VOID_P / 2)
#endif

typedef struct {
    PyObject_VAR_HEAD
    Py_ssize_t groups; /* must be first! */
    PyObject* groupindex; /* dict */
    PyObject* indexgroup; /* tuple */
    /* compatibility */
    PyObject* pattern; /* pattern source (or None) */
    int flags; /* flags used when compiling pattern source */
    PyObject *weakreflist; /* List of weak references */
    int isbytes; /* pattern type (1 - bytes, 0 - string, -1 - None) */
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

typedef struct {
    PyObject_VAR_HEAD
    Py_ssize_t chunks;  /* the number of group references and non-NULL literals
                         * self->chunks <= 2*Py_SIZE(self) + 1 */
    PyObject *literal;
    struct {
        Py_ssize_t index;
        PyObject *literal;  /* NULL if empty */
    } items[0];
} TemplateObject;

typedef struct SRE_REPEAT_T {
    Py_ssize_t count;
    const SRE_CODE* pattern; /* points to REPEAT operator arguments */
    const void* last_ptr; /* helper to check for infinite loops */
    struct SRE_REPEAT_T *prev; /* points to previous repeat context */
} SRE_REPEAT;

typedef struct {
    /* string pointers */
    const void* ptr; /* current position (also end of current slice) */
    const void* beginning; /* start of original string */
    const void* start; /* start of current slice */
    const void* end; /* end of original string */
    /* attributes for the match object */
    PyObject* string;
    Py_buffer buffer;
    Py_ssize_t pos, endpos;
    int isbytes;
    int charsize; /* character size */
    int match_all;
    int must_advance;
    /* marks */
    int lastmark;
    int lastindex;
    const void** mark;
    /* dynamically allocated stuff */
    char* data_stack;
    size_t data_stack_size;
    size_t data_stack_base;
    /* current repeat context */
    SRE_REPEAT *repeat;
    unsigned int sigcount;
} SRE_STATE;

typedef struct {
    PyObject_HEAD
    PyObject* pattern;
    SRE_STATE state;
    int executing;
} ScannerObject;

#endif
