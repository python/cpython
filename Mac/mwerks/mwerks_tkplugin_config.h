/*
** Special config-file for _tkinter plugin.
*/

#define USE_GUSI2		/* Stdio implemented with GUSI 2 */
/* #define USE_GUSI1	/* Stdio implemented with GUSI 1.X */
#if defined(USE_GUSI1) || defined(USE_GUSI2)
#define USE_GUSI
#endif
#define WITH_THREAD		/* Use thread support (needs GUSI 2, not GUSI 1) */
#define USE_TK			/* Include _tkinter module in core Python */
#define MAC_TCL			/* This *must* be on if USE_TK is on */
#define WITHOUT_FRAMEWORKS	/* Use old-style Universal Header includes, not Carbon/Carbon.h */
