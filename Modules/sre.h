/*
 * Secret Labs' Regular Expression Engine
 * $Id$
 *
 * simple regular expression matching engine
 *
 * Copyright (c) 1997-2000 by Secret Labs AB.  All rights reserved.
 *
 * See the _sre.c file for information on usage and redistribution.
 */

#ifndef SRE_INCLUDED
#define SRE_INCLUDED

#include "sre_constants.h"

/* Python objects */

typedef struct {
    PyObject_HEAD
    PyObject* code; /* link to the code string object */
    PyObject* pattern; /* link to the pattern source (or None) */
    int groups;
    PyObject* groupindex;
} PatternObject;

#define PatternObject_GetCode(o) ((void*) PyString_AS_STRING((o)->code))

typedef struct {
    PyObject_HEAD
    PyObject* string; /* link to the target string */
    PatternObject* pattern; /* link to the regex (pattern) object */
    int groups; /* number of groups (start/end marks) */
    int mark[2];
} MatchObject;

#endif

