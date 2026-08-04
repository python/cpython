/* Terminfo stubs for building the curses module against PDCurses.

   PDCurses has no terminfo database, so these functions cannot do anything
   useful.  It exports stubs of its own, but they are declared for import from
   the DLL and its setupterm() reports an error, which the module needs to
   succeed; these definitions replace them.  Each of the others reports failure
   using the value that terminfo assigns to a missing capability. */

#include <stdio.h>
#include "term.h"

TERMINAL *cur_term = NULL;

int
setupterm(const char *term, int fildes, int *errret)
{
    /* PDCurses has no terminfo database and initscr() works without one, so
       report success: this is the no-op that curses.initscr() expects before
       it calls the real initscr().  The capability queries below still report
       "not found", which is the truthful answer with no terminfo. */
    if (errret != NULL) {
        *errret = 1;            /* 1: terminal is hardcopy/normal, OK */
    }
    return OK;
}

int
del_curterm(TERMINAL *oterm)
{
    return ERR;
}

TERMINAL *
set_curterm(TERMINAL *nterm)
{
    return NULL;
}

int
restartterm(const char *term, int fildes, int *errret)
{
    if (errret != NULL) {
        *errret = 0;
    }
    return ERR;
}

int
tigetflag(const char *capname)
{
    return -1;                  /* -1: capability is not a boolean */
}

int
tigetnum(const char *capname)
{
    return -2;                  /* -2: capability is not numeric */
}

char *
tigetstr(const char *capname)
{
    return (char *)-1;          /* (char *)-1: capability is not a string */
}

char *
tparm(const char *str, long p1, long p2, long p3, long p4, long p5,
      long p6, long p7, long p8, long p9)
{
    return NULL;
}

int
tputs(const char *str, int affcnt, int (*outc)(int))
{
    /* No terminfo padding information is available, so emit the string as-is
       through the caller's output function.  This is enough for putp() to
       write a literal (non-capability) string. */
    if (str == NULL || str == (const char *)-1) {
        return ERR;
    }
    while (*str != '\0') {
        if (outc((unsigned char)*str++) == EOF) {
            return ERR;
        }
    }
    return OK;
}

int
putp(const char *str)
{
    return tputs(str, 1, putchar);
}
