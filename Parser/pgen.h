#ifndef Py_PGEN_H
#define Py_PGEN_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Parser generator interface */

extern grammar *meta_grammar Py_PROTO((void));

struct _node;
extern grammar *pgen Py_PROTO((struct _node *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_PGEN_H */
