#ifndef Py_OSDEFS_H
#define Py_OSDEFS_H
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

/* Operating system dependencies */

#ifdef macintosh
#define SEP ':'
#define MAXPATHLEN 256
/* Mod by Jack: newline is less likely to occur in filenames than space */
#define DELIM '\n'
#endif

/* Mod by chrish: QNX has WATCOM, but isn't DOS */
#if !defined(__QNX__)
#if defined(MS_WINDOWS) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__DJGPP__) || defined(PYOS_OS2)
#define SEP '\\'
#define ALTSEP '/'
#define MAXPATHLEN 256
#define DELIM ';'
#endif
#endif

/* Filename separator */
#ifndef SEP
#define SEP '/'
#endif

/* Max pathname length */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* Search path entry delimiter */
#ifndef DELIM
#define DELIM ':'
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_OSDEFS_H */
