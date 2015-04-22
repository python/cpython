#ifndef TKINTER_H
#define TKINTER_H

/* This header is used to share some macros between _tkinter.c and
 * tkappinit.c.
 * Be sure to include tk.h before including this header so
 * TK_HEX_VERSION is properly defined. */

/* TK_RELEASE_LEVEL is always one of the following:
 *  TCL_ALPHA_RELEASE   0
 *  TCL_BETA_RELEASE    1
 *  TCL_FINAL_RELEASE   2
 */
#define TK_HEX_VERSION ((TK_MAJOR_VERSION << 24) | \
                        (TK_MINOR_VERSION << 16) | \
                        (TK_RELEASE_LEVEL << 8) | \
                        (TK_RELEASE_SERIAL << 0))

/* TK_VERSION_HEX packs fields in wrong order, not suitable for comparing of
 * non-final releases.  Left for backward compatibility.
 */
#define TK_VERSION_HEX ((TK_MAJOR_VERSION << 24) | \
                        (TK_MINOR_VERSION << 16) | \
                        (TK_RELEASE_SERIAL << 8) | \
                        (TK_RELEASE_LEVEL << 0))

/* Protect Tk 8.4.13 and older from a deadlock that happens when trying
 * to load tk after a failed attempt. */
#if TK_HEX_VERSION < 0x0804020e
#define TKINTER_PROTECT_LOADTK
#define TKINTER_LOADTK_ERRMSG \
        "Calling Tk_Init again after a previous call failed might deadlock"
#endif

#endif /* !TKINTER_H */
