/*
** Config file for dynamically-loaded ppc/cfm68k plugin modules.
*/
#define ACCESSOR_CALLS_ARE_FUNCTIONS 1
#define OPAQUE_TOOLBOX_STRUCTS 1
#define TARGET_API_MAC_CARBON 1
#define WITHOUT_FRAMEWORKS /* Use old-style Universal Header includes, not Carbon/Carbon.h */
#define USE_TOOLBOX_OBJECT_GLUE	/* Use glue routines for accessing PyArg_Parse/Py_BuildValue helpers */

#define USE_GUSI2		/* Stdio implemented with GUSI */
#define USE_GUSI
#define WITH_THREAD		/* Use thread support (needs GUSI 2, not GUSI 1) */
#define USE_MSL			/* Use MSL libraries */
#ifdef USE_MSL
#define MSL_USE_PRECOMPILED_HEADERS 0	/* Don't use precomp headers: we include our own */
#include <ansi_prefix.mac.h>
#endif
#ifndef Py_DEBUG
#define NDEBUG
#endif
