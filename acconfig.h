/* Leave this blank line here -- autoheader needs it! */


/* Define if you have the Mach cthreads package */
#undef C_THREADS

/* Define if --enable-ipv6 is specified */
#undef ENABLE_IPV6

/* Define this if you have gethostbyname() */
#undef HAVE_GETHOSTBYNAME

/* Define this if you have some version of gethostbyname_r() */
#undef HAVE_GETHOSTBYNAME_R

/* Define if you have termios available */
#undef HAVE_TERMIOS_H

/* Define as the integral type used for Unicode representation. */
#undef PY_UNICODE_TYPE

/* Define as the size of the unicode type. */
#undef Py_UNICODE_SIZE

/* Define to force use of thread-safe errno, h_errno, and other functions */
#undef _REENTRANT

/* sizeof(void *) */
#undef SIZEOF_VOID_P

/* Define to `int' if <sys/types.h> doesn't define.  */
#undef socklen_t

/* Define for SOLARIS 2.x */
#undef SOLARIS

/* Define if you want to use BSD db. */
#undef WITH_LIBDB

/* Define if you want to use ndbm. */
#undef WITH_LIBNDBM

/* Define if you want to compile in rudimentary thread support */
#undef WITH_THREAD


/* Leave that blank line there-- autoheader needs it! */

@BOTTOM@

#ifdef __CYGWIN__
#ifdef USE_DL_IMPORT
#define DL_IMPORT(RTYPE) __declspec(dllimport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#else
#define DL_IMPORT(RTYPE) __declspec(dllexport) RTYPE
#define DL_EXPORT(RTYPE) __declspec(dllexport) RTYPE
#endif
#endif

/* Define the macros needed if on a UnixWare 7.x system. */
#if defined(__USLC__) && defined(__SCO_VERSION__)
#define STRICT_SYSV_CURSES /* Don't use ncurses extensions */
#endif

