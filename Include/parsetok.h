#ifndef Py_PARSETOK_H
#define Py_PARSETOK_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Parser-tokenizer link interface */

typedef struct {
	int error;
	char *filename;
	int lineno;
	int offset;
	char *text;
} perrdetail;

extern DL_IMPORT(node *) PyParser_ParseString Py_PROTO((char *, grammar *, int, perrdetail *));
extern DL_IMPORT(node *) PyParser_ParseFile Py_PROTO((FILE *, char *, grammar *, int,
			    char *, char *, perrdetail *));

#ifdef __cplusplus
}
#endif
#endif /* !Py_PARSETOK_H */
