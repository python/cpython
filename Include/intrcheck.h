#ifndef Py_INTRCHECK_H
#define Py_INTRCHECK_H
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

extern DL_IMPORT(int) PyOS_InterruptOccurred Py_PROTO((void));
extern DL_IMPORT(void) PyOS_InitInterrupts Py_PROTO((void));
DL_IMPORT(void) PyOS_AfterFork Py_PROTO((void));

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTRCHECK_H */
