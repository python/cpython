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
extern void initaudioop();
extern void initbinascii();
extern void initcmath();
extern void initerrno();
extern void initimageop();
extern void initmath();
extern void initmd5();
extern void initnew();
extern void initnt();
extern void initos2();
extern void initoperator();
extern void initposix();
extern void initregex();
extern void initreop();
extern void initrgbimg();
extern void initrotor();
extern void initsignal();
extern void initselect();
extern void init_socket();
extern void initsoundex();
extern void initstrop();
extern void initstruct();
extern void inittime();
extern void initthread();
extern void initcStringIO();
extern void initcPickle();
#ifdef WIN32
extern void initmsvcrt();
#endif

/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init();
extern void initimp();

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
        {"new", initnew},
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
        {"regex", initregex},
        {"reop", initreop},
//        {"rgbimg", initrgbimg},
//        {"rotor", initrotor},
        {"signal", initsignal},
#ifdef USE_SOCKET
        {"_socket", init_socket},
        {"select", initselect},
#endif
        {"soundex", initsoundex},
        {"strop", initstrop},
        {"struct", initstruct},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
        {"cPickle", initcPickle},
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
