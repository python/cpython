/*
** Configuration file for small standalone 68k/ppc Python.
**
** Note: enabling the switches below is not enough to enable the
** specific features, you may also need different sets of sources.
*/
#define ACCESSOR_CALLS_ARE_FUNCTIONS 1
#define OPAQUE_TOOLBOX_STRUCTS 1
#define TARGET_API_MAC_CARBON 1
#define TARGET_API_MAC_CARBON_NOTYET 1 /* Things we should do eventually, but not now */

#define USE_ARGV0_CHDIR		/* Workaround for OSXDP4: change dir to argv[0] dir */
#define USE_GUSI2		/* Stdio implemented with GUSI 2 */
/* # define USE_GUSI1	/* Stdio implemented with GUSI 1 */
#define USE_MSL			/* Use Mw Standard Library (as opposed to Plaugher C libraries) */
#define USE_TOOLBOX		/* Include toolbox modules in core Python */
/* #define USE_CORE_TOOLBOX		/* Include minimal set of toolbox modules in core Python */
#define USE_QT			/* Include quicktime modules in core Python */
/* #define USE_WASTE		/* Include waste module in core Python */
/* #define USE_MACSPEECH		/* Include macspeech module in core Python */
/* #define USE_IMG	       	/* Include img modules in core Python */
/* #define USE_MACCTB		/* Include ctb module in core Python */
/* #define USE_STDWIN		/* Include stdwin module in core Python */
/* #define USE_MACTCP		/* Include mactcp (*not* socket) modules in core */
/* #define USE_TK			/* Include _tkinter module in core Python */
/* #define MAC_TCL			/* This *must* be on if USE_TK is on */
/* #define USE_MAC_SHARED_LIBRARY	/* Enable code to add shared-library resources */
/* #define USE_MAC_APPLET_SUPPORT	/* Enable code to run a PYC resource */
/* #define HAVE_DYNAMIC_LOADING		/* Enable dynamically loaded modules */
/* #define USE_GDBM		/* Include the gdbm module */
/* #define USE_ZLIB		/* Include the zlib module */
#define USE_APPEARANCE	/* Enable Appearance support */

#define USE_MSL_MALLOC	/* Disable private malloc. Also disables next two defines */
#ifndef USE_MSL_MALLOC
/* #define USE_MALLOC_DEBUG			/* Enable range checking and other malloc debugging */
#ifdef __powerc
#define USE_CACHE_ALIGNED 8		/* Align on 32-byte boundaries for 604 */
#endif
#endif
#define WITHOUT_FRAMEWORKS /* Use old-style Universal Header includes, not Carbon/Carbon.h */

#ifdef USE_MSL
#define MSL_USE_PRECOMPILED_HEADERS 0	/* Don't use precomp headers: we include our own */
#include <ansi_prefix.mac.h>
#endif
