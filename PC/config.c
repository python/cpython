/* -*- C -*- ***********************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
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
#ifdef WITH_CYCLE_GC
extern void initgc();
#endif
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
#ifdef WITH_CYCLE_GC
        {"gc", initgc},
#endif
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
