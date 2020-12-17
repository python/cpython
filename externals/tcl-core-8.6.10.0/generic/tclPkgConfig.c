/*
 * tclPkgConfig.c --
 *
 *	This file contains the configuration information to embed into the tcl
 *	binary library.
 *
 * Copyright (c) 2002 Andreas Kupries <andreas_kupries@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/* Note, the definitions in this module are influenced by the following C
 * preprocessor macros:
 *
 * OSCMa  = shortcut for "old style configuration macro activates"
 * NSCMdt = shortcut for "new style configuration macro declares that"
 *
 * - TCL_THREADS		OSCMa compilation as threaded core.
 * - TCL_MEM_DEBUG		OSCMa memory debugging.
 * - TCL_COMPILE_DEBUG		OSCMa debugging of bytecode compiler.
 * - TCL_COMPILE_STATS		OSCMa bytecode compiler statistics.
 *
 * - TCL_CFG_DO64BIT		NSCMdt tcl is compiled for a 64bit system.
 * - NDEBUG		NSCMdt tcl is compiled with symbol info off.
 * - TCL_CFG_OPTIMIZED		NSCMdt tcl is compiled with cc optimizations on
 * - TCL_CFG_PROFILED		NSCMdt tcl is compiled with profiling info.
 *
 * - CFG_RUNTIME_*		Paths to various stuff at runtime.
 * - CFG_INSTALL_*		Paths to various stuff at installation time.
 *
 * - TCL_CFGVAL_ENCODING	string containing the encoding used for the
 *				configuration values.
 */

#include "tclInt.h"

/*
 * Use C preprocessor statements to define the various values for the embedded
 * configuration information.
 */

#ifdef TCL_THREADS
#  define  CFG_THREADED		"1"
#else
#  define  CFG_THREADED		"0"
#endif

#ifdef TCL_MEM_DEBUG
#  define CFG_MEMDEBUG		"1"
#else
#  define CFG_MEMDEBUG		"0"
#endif

#ifdef TCL_COMPILE_DEBUG
#  define CFG_COMPILE_DEBUG	"1"
#else
#  define CFG_COMPILE_DEBUG	"0"
#endif

#ifdef TCL_COMPILE_STATS
#  define CFG_COMPILE_STATS	"1"
#else
#  define CFG_COMPILE_STATS	"0"
#endif

#ifdef TCL_CFG_DO64BIT
#  define CFG_64		"1"
#else
#  define CFG_64		"0"
#endif

#ifndef NDEBUG
#  define CFG_DEBUG		"1"
#else
#  define CFG_DEBUG		"0"
#endif

#ifdef TCL_CFG_OPTIMIZED
#  define CFG_OPTIMIZED		"1"
#else
#  define CFG_OPTIMIZED		"0"
#endif

#ifdef TCL_CFG_PROFILED
#  define CFG_PROFILED		"1"
#else
#  define CFG_PROFILED		"0"
#endif

static Tcl_Config const cfg[] = {
    {"debug",			CFG_DEBUG},
    {"threaded",		CFG_THREADED},
    {"profiled",		CFG_PROFILED},
    {"64bit",			CFG_64},
    {"optimized",		CFG_OPTIMIZED},
    {"mem_debug",		CFG_MEMDEBUG},
    {"compile_debug",		CFG_COMPILE_DEBUG},
    {"compile_stats",		CFG_COMPILE_STATS},

    /* Runtime paths to various stuff */

    {"libdir,runtime",		CFG_RUNTIME_LIBDIR},
    {"bindir,runtime",		CFG_RUNTIME_BINDIR},
    {"scriptdir,runtime",	CFG_RUNTIME_SCRDIR},
    {"includedir,runtime",	CFG_RUNTIME_INCDIR},
    {"docdir,runtime",		CFG_RUNTIME_DOCDIR},

    /* Installation paths to various stuff */

    {"libdir,install",		CFG_INSTALL_LIBDIR},
    {"bindir,install",		CFG_INSTALL_BINDIR},
    {"scriptdir,install",	CFG_INSTALL_SCRDIR},
    {"includedir,install",	CFG_INSTALL_INCDIR},
    {"docdir,install",		CFG_INSTALL_DOCDIR},

    /* Last entry, closes the array */
    {NULL, NULL}
};

void
TclInitEmbeddedConfigurationInformation(
    Tcl_Interp *interp)		/* Interpreter the configuration command is
				 * registered in. */
{
    Tcl_RegisterConfig(interp, "tcl", cfg, TCL_CFGVAL_ENCODING);
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
