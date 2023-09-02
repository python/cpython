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

#endif /* !TKINTER_H */
