#ifndef Py_PROTO_H
#define Py_PROTO_H
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

#ifdef HAVE_PROTOTYPES
#define Py_PROTO(x) x
#else
#define Py_PROTO(x) ()
#endif

#ifndef Py_FPROTO
#define Py_FPROTO(x) Py_PROTO(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* !Py_PROTO_H */
