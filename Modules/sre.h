/*
 * Secret Labs' Regular Expression Engine
 *
 * regular expression matching engine
 *
 * Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
 *
 * See the _sre.c file for information on usage and redistribution.
 */

#ifndef SRE_INCLUDED
#define SRE_INCLUDED

#include "sre_constants.h"

/* size of a code word (must be unsigned short or larger) */
#define SRE_CODE unsigned short

typedef struct {
    PyObject_VAR_HEAD
    int groups;
    PyObject* groupindex;
    PyObject* indexgroup;
    /* compatibility */
    PyObject* pattern; /* pattern source (or None) */
    int flags; /* flags used when compiling pattern source */
    /* pattern code */
    SRE_CODE code[1];
} PatternObject;

#define PatternObject_GetCode(o) (((PatternObject*)(o))->code)

typedef struct {
    PyObject_VAR_HEAD
    PyObject* string; /* link to the target string */
    PyObject* regs; /* cached list of matching spans */
    PatternObject* pattern; /* link to the regex (pattern) object */
    int pos, endpos; /* current target slice */
    int lastindex; /* last index marker seen by the engine (-1 if none) */
    int groups; /* number of groups (start/end marks) */
    int mark[1];
} MatchObject;

typedef unsigned int (*SRE_TOLOWER_HOOK)(unsigned int ch);

/* FIXME: <fl> shouldn't be a constant, really... */
#define SRE_MARK_SIZE 200

typedef struct SRE_REPEAT_T {
    int count;
    SRE_CODE* pattern; /* points to REPEAT operator arguments */
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
    int pos, endpos;
    /* character size */
    int charsize;
    /* registers */
    int lastindex;
    int lastmark;
    void* mark[SRE_MARK_SIZE];
    /* dynamically allocated stuff */
    void** mark_stack;
    int mark_stack_size;
    int mark_stack_base;
    SRE_REPEAT *repeat; /* current repeat context */
    /* hooks */
    SRE_TOLOWER_HOOK lower;
} SRE_STATE;

typedef struct {
    PyObject_HEAD
    PyObject* pattern;
    SRE_STATE state;
} ScannerObject;

#endif
