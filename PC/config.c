/* Module configuration */

/* This file contains the table of built-in modules.
   See init_builtin() in import.c. */

#include "Python.h"

extern void initarray(void);
#ifndef MS_WINI64
extern void initaudioop(void);
#endif
extern void initbinascii(void);
extern void initcmath(void);
extern void initerrno(void);
extern void initgc(void);
extern void initmath(void);
extern void initnt(void);
extern void initoperator(void);
extern void initsignal(void);
extern void init_sha256(void);
extern void init_sha512(void);
extern void inittime(void);
extern void initthread(void);
extern void initcStringIO(void);
#ifdef WIN32
extern void initmsvcrt(void);
extern void init_locale(void);
#endif
extern void init_codecs(void);
extern void init_weakref(void);
extern void initxxsubtype(void);
extern void initzipimport(void);
extern void init_random(void);
extern void inititertools(void);
extern void init_collections(void);
extern void init_heapq(void);
extern void init_bisect(void);
extern void init_symtable(void);
extern void initmmap(void);
extern void init_csv(void);
extern void init_sre(void);
extern void initparser(void);
extern void init_winreg(void);
extern void init_struct(void);
extern void initdatetime(void);
extern void init_functools(void);
extern void initzlib(void);

extern void init_multibytecodec(void);
extern void init_codecs_cn(void);
extern void init_codecs_hk(void);
extern void init_codecs_iso2022(void);
extern void init_codecs_jp(void);
extern void init_codecs_kr(void);
extern void init_codecs_tw(void);
extern void init_subprocess(void);
extern void init_lsprof(void);
extern void init_ast(void);
extern void init_types(void);
extern void init_fileio(void);
extern void initatexit(void);

/* tools/freeze/makeconfig.py marker for additional "extern" */
/* -- ADDMODULE MARKER 1 -- */

extern void PyMarshal_Init(void);
extern void initimp(void);

struct _inittab _PyImport_Inittab[] = {

        {"array", initarray},
	{"_ast", init_ast},
#ifdef MS_WINDOWS
#ifndef MS_WINI64
        {"audioop", initaudioop},
#endif
#endif
        {"binascii", initbinascii},
        {"cmath", initcmath},
        {"errno", initerrno},
        {"gc", initgc},
        {"math", initmath},
        {"nt", initnt}, /* Use the NT os functions, not posix */
        {"operator", initoperator},
        {"signal", initsignal},
        {"_sha256", init_sha256},
        {"_sha512", init_sha512},
        {"time", inittime},
#ifdef WITH_THREAD
        {"thread", initthread},
#endif
        {"cStringIO", initcStringIO},
#ifdef WIN32
        {"msvcrt", initmsvcrt},
        {"_locale", init_locale},
#endif
	/* XXX Should _subprocess go in a WIN32 block?  not WIN64? */
	{"_subprocess", init_subprocess},

        {"_codecs", init_codecs},
	{"_weakref", init_weakref},
	{"_random", init_random},
        {"_bisect", init_bisect},
        {"_heapq", init_heapq},
	{"_lsprof", init_lsprof},
	{"itertools", inititertools},
        {"_collections", init_collections},
	{"_symtable", init_symtable},
	{"mmap", initmmap},
	{"_csv", init_csv},
	{"_sre", init_sre},
	{"parser", initparser},
	{"_winreg", init_winreg},
	{"_struct", init_struct},
	{"datetime", initdatetime},
	{"_functools", init_functools},

	{"xxsubtype", initxxsubtype},
	{"zipimport", initzipimport},
	{"zlib", initzlib},
	
	/* CJK codecs */
	{"_multibytecodec", init_multibytecodec},
	{"_codecs_cn", init_codecs_cn},
	{"_codecs_hk", init_codecs_hk},
	{"_codecs_iso2022", init_codecs_iso2022},
	{"_codecs_jp", init_codecs_jp},
	{"_codecs_kr", init_codecs_kr},
	{"_codecs_tw", init_codecs_tw},

/* tools/freeze/makeconfig.py marker for additional "_inittab" entries */
/* -- ADDMODULE MARKER 2 -- */

        /* This module "lives in" with marshal.c */
        {"marshal", PyMarshal_Init},

        /* This lives it with import.c */
        {"imp", initimp},

        /* These entries are here for sys.builtin_module_names */
        {"__main__", NULL},
        {"__builtin__", NULL},
        {"sys", NULL},
        
        {"_types", init_types},
        {"_fileio", init_fileio},
        {"atexit", initatexit},

        /* Sentinel */
        {0, 0}
};
