
#ifndef Py_CURSES_H
#define Py_CURSES_H

#ifdef __APPLE__
/*
** On Mac OS X 10.2 [n]curses.h and stdlib.h use different guards
** against multiple definition of wchar_t.
*/
#ifdef _BSD_WCHAR_T_DEFINED_
#define _WCHAR_T
#endif
#endif /* __APPLE__ */

/* On FreeBSD, [n]curses.h and stdlib.h/wchar.h use different guards
   against multiple definition of wchar_t and wint_t. */
#if defined(__FreeBSD__) && defined(_XOPEN_SOURCE_EXTENDED)
# ifndef __wchar_t
#   define __wchar_t
# endif
# ifndef __wint_t
#   define __wint_t
# endif
#endif

#if defined(WINDOW_HAS_FLAGS) && defined(__APPLE__)
/* gh-109617, gh-115383: we can rely on the default value for NCURSES_OPAQUE on
   most platforms, but not on macOS. This is because, starting with Xcode 15,
   Apple-provided ncurses.h comes from ncurses 6 (which defaults to opaque
   structs) but can still be linked to older versions of ncurses dynamic
   libraries which don't provide functions such as is_pad() to deal with opaque
   structs. Setting NCURSES_OPAQUE to 0 is harmless in all ncurses releases to
   this date (provided that a thread-safe implementation is not required), but
   this might change in the future. This fix might become irrelevant once
   support for macOS 13 or earlier is dropped. */
#define NCURSES_OPAQUE 0
#endif

#if defined(HAVE_NCURSESW_NCURSES_H)
#  include <ncursesw/ncurses.h>
#elif defined(HAVE_NCURSESW_CURSES_H)
#  include <ncursesw/curses.h>
#elif defined(HAVE_NCURSES_NCURSES_H)
#  include <ncurses/ncurses.h>
#elif defined(HAVE_NCURSES_CURSES_H)
#  include <ncurses/curses.h>
#elif defined(HAVE_NCURSES_H)
#  include <ncurses.h>
#elif defined(HAVE_CURSES_H)
#  include <curses.h>
#endif

#ifdef NCURSES_VERSION
/* configure was checking <curses.h>, but we will
   use <ncurses.h>, which has some or all these features. */
#if !defined(WINDOW_HAS_FLAGS) && \
    (NCURSES_VERSION_PATCH+0 < 20070303 || !(NCURSES_OPAQUE+0))
/* the WINDOW flags field was always accessible in ncurses prior to 20070303;
   after that, it depends on the value of NCURSES_OPAQUE. */
#define WINDOW_HAS_FLAGS 1
#endif
#if !defined(HAVE_CURSES_IS_PAD) && NCURSES_VERSION_PATCH+0 >= 20090906
#define HAVE_CURSES_IS_PAD 1
#endif
#ifndef MVWDELCH_IS_EXPRESSION
#define MVWDELCH_IS_EXPRESSION 1
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PyCurses_API_pointers 4

/* Type declarations */

typedef struct PyCursesWindowObject {
    PyObject_HEAD
    WINDOW *win;
    char *encoding;
    struct PyCursesWindowObject *orig;
} PyCursesWindowObject;

#define PyCurses_CAPSULE_NAME "_curses._C_API"


#ifdef CURSES_MODULE
/* This section is used when compiling _cursesmodule.c */

#else
/* This section is used in modules that use the _cursesmodule API */

static void **PyCurses_API;

#define PyCursesWindow_Type (*_PyType_CAST(PyCurses_API[0]))
#define PyCursesSetupTermCalled  {if (! ((int (*)(void))PyCurses_API[1]) () ) return NULL;}
#define PyCursesInitialised      {if (! ((int (*)(void))PyCurses_API[2]) () ) return NULL;}
#define PyCursesInitialisedColor {if (! ((int (*)(void))PyCurses_API[3]) () ) return NULL;}

#define PyCursesWindow_Check(v)     Py_IS_TYPE((v), &PyCursesWindow_Type)

#define import_curses() \
    PyCurses_API = (void **)PyCapsule_Import(PyCurses_CAPSULE_NAME, 1);

#endif

/* general error messages */
static const char catchall_ERR[]  = "curses function returned ERR";
static const char catchall_NULL[] = "curses function returned NULL";

#if defined(CURSES_MODULE) || defined(CURSES_PANEL_MODULE)
/* Error messages shared by the curses package */
#  define CURSES_ERROR_FORMAT           "%s() returned %s"
#  define CURSES_ERROR_VERBOSE_FORMAT   "%s() (called by %s()) returned %s"
#  define CURSES_ERROR_MUST_CALL_FORMAT "must call %s() first"
#endif

#ifdef __cplusplus
}
#endif

#endif /* !defined(Py_CURSES_H) */

