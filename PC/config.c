/* -*- C -*- ***********************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray();
#ifndef MS_WIN64
extern void initaudioop();
extern void initbinascii();
#endif
extern void initcmath();
extern void initerrno();
#ifndef MS_WIN64
extern void initimageop();
#endif
extern void initmath();
extern void initmd5();
extern void initnew();
extern void initnt();
extern void initoperator();
extern void initregex();
#ifndef MS_WIN64
extern void initrgbimg();
#endif
extern void initrotor();
extern void initsignal();
extern void initsha();
extern void initstrop();
extern void initstruct();
extern void inittime();
extern void initthread();
extern void initcStringIO();
extern void initcPickle();
extern void initpcre();
#ifdef WIN32
extern void initmsvcrt();
extern void init_locale();
#endif
extern void init_codecs();

/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init();
extern void initimp();

struct _inittab _PyImport_Inittab[] = {

        {"array", initarray},
#ifdef MS_WINDOWS
#ifndef MS_WIN64
        {"audioop", initaudioop},
#endif
#endif
#ifndef MS_WIN64
        {"binascii", initbinascii},
#endif
        {"cmath", initcmath},
        {"errno", initerrno},
#ifndef MS_WIN64
        {"imageop", initimageop},
#endif
        {"math", initmath},
        {"md5", initmd5},
        {"new", initnew},
        {"nt", initnt}, /* Use the NT os functions, not posix */
        {"operator", initoperator},
        {"regex", initregex},
#ifndef MS_WIN64
        {"rgbimg", initrgbimg},
#endif
        {"rotor", initrotor},
        {"signal", initsignal},
        {"sha", initsha},
        {"strop", initstrop},
        {"struct", initstruct},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
        {"cPickle", initcPickle},
        {"pcre", initpcre},
#ifdef WIN32
        {"msvcrt", initmsvcrt},
        {"_locale", init_locale},
#endif

        {"_codecs", init_codecs},

/* -- ADDMODULE MARKER 2 -- */

        /* This module "lives in" with marshal.c */
        {"marshal", PyMarshal_Init},

        /* This lives it with import.c */
        {"imp", initimp},

        /* These entries are here for sys.builtin_module_names */
        {"__main__", NULL},
        {"__builtin__", NULL},
        {"sys", NULL},

        /* Sentinel */
        {0, 0}
};
