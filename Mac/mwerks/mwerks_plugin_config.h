/*
** Config file for dynamically-loaded ppc/cfm68k plugin modules.
*/

#define USE_GUSI		/* Stdio implemented with GUSI */
#define USE_MSL			/* Use MSL libraries */
#ifdef USE_MSL
#define MSL_USE_PRECOMPILED_HEADERS 0	/* Don't use precomp headers: we include our own */
#include <ansi_prefix.mac.h>
#endif
