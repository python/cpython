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

extern void initarray(void);
extern void initaudioop(void);
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initimageop(void);
extern void initmath(void);
extern void initmd5(void);
extern void initnt(void);
extern void initos2(void);
extern void initoperator(void);
extern void initposix(void);
extern void initrgbimg(void);
extern void initsignal(void);
extern void initselect(void);
extern void init_socket(void);
extern void initstruct(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
extern void initpcre(void);
#ifdef WIN32
extern void initmsvcrt(void);
#endif

/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {

        {"array", initarray},
#ifdef M_I386
        {"audioop", initaudioop},
#endif
        {"binascii", initbinascii},
        {"cmath", initcmath},
        {"errno", initerrno},
//        {"imageop", initimageop},
        {"math", initmath},
        {"md5", initmd5},
#if defined(MS_WINDOWS) || defined(__BORLANDC__) || defined(__WATCOMC__)
        {"nt", initnt}, /* Use the NT os functions, not posix */
#else
#if defined(PYOS_OS2)
        {"os2", initos2},
#else
        {"posix", initposix},
#endif
#endif
        {"operator", initoperator},
//        {"rgbimg", initrgbimg},
        {"signal", initsignal},
#ifdef USE_SOCKET
        {"_socket", init_socket},
        {"select", initselect},
#endif
        {"struct", initstruct},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
        {"pcre", initpcre},
#ifdef WIN32
        {"msvcrt", initmsvcrt},
#endif

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
