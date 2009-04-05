#ifndef TKINTER_H
#define TKINTER_H

/* This header is used to share some macros between _tkinter.c and
 * tkappinit.c.
 * Be sure to include tk.h before including this header so
 * TK_VERSION_HEX is properly defined. */

/* TK_RELEASE_LEVEL is always one of the following:
 *	TCL_ALPHA_RELEASE   0
 *  TCL_BETA_RELEASE    1
 *  TCL_FINAL_RELEASE   2
 */
#define TK_VERSION_HEX ((TK_MAJOR_VERSION << 24) | \
		(TK_MINOR_VERSION << 16) | \
		(TK_RELEASE_SERIAL << 8) | \
		(TK_RELEASE_LEVEL << 0))

/* Protect Tk 8.4.13 and older from a deadlock that happens when trying
 * to load tk after a failed attempt. */
#if TK_VERSION_HEX < 0x08040e02
#define TKINTER_PROTECT_LOADTK
#define TKINTER_LOADTK_ERRMSG \
	"Calling Tk_Init again after a previous call failed might deadlock"
#endif

#endif /* !TKINTER_H */
