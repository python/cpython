/* Minimal <term.h> for building the curses module against PDCurses.

   PDCurses does not implement terminfo, but _cursesmodule.c references a
   handful of terminfo functions (setupterm(), tigetstr(), tparm(), ...)
   unconditionally.  PDCurses ships a <term.h> of its own, declaring the same
   stubs it exports from the library; the module cannot use it, because the
   declarations are marked for import from the DLL and the module defines its
   own -- setupterm() has to report success, where PDCurses reports an error.
   Anyone who calls the others gets an error, which is the best that can be
   done without a terminfo database. */

#ifndef PY_PDCURSES_TERM_H
#define PY_PDCURSES_TERM_H 1

#include <curses.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *_termname;
} TERMINAL;

extern TERMINAL *cur_term;

int       del_curterm(TERMINAL *);
int       putp(const char *);
int       restartterm(const char *, int, int *);
TERMINAL *set_curterm(TERMINAL *);
int       setupterm(const char *, int, int *);
int       tigetflag(const char *);
int       tigetnum(const char *);
char     *tigetstr(const char *);
char     *tparm(const char *, long, long, long, long, long,
                long, long, long, long);
int       tputs(const char *, int, int (*)(int));

#ifdef __cplusplus
}
#endif

#endif /* !PY_PDCURSES_TERM_H */
