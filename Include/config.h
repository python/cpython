/* Include/config.h.  Generated automatically by configure.  */
/* NOTE: config.h.in is converted into config.h by the configure
   script in the toplevel directory.

   On non-UNIX systems, manually copy config.h.in to config.h, and
   edit the latter to reflect the actual configuration of your system.

   Then arrange that the symbol HAVE_CONFIG_H is defined during
   compilation (usually by passing an argument of the form
   `-DHAVE_CONFIG_H' to the compiler, but this is necessarily
   system-dependent).  */


/* Types which have no traditional name -- edit the definition if necessary */

#define RETSIGTYPE void		/* int or void: return of signal handlers */


/* Types which are often defined in <sys/types.h> -- either define as
   some variant of int or leave undefined.  Uncomment a definition if
   your <sys/types.h> does not define the type */

/* #define mode_t int */
/* #define off_t long */
/* #define pid_t int */
/* #define size_t unsigned */
/* #define uid_t int */
/* #define gid_t int */


/* Feature test symbols -- either define as 1 or leave undefined */

/*	symbol name:		#define as 1 if: */

/* #undef	STDC_HEADERS */  		/* the standard C header files exist
				   (in particular, <stdlib.h>,
				   <stdarg.h>, <string.h> and <float.h>) */

/* #undef	HAVE_DLFCN_H */  		/* <dlfcn.h> exists */
#define	HAVE_SIGNAL_H 1  		/* <signal.h> exists */
#define	HAVE_STDARG_H 1  		/* <stdarg.h> exists (else need <varargs.h>) */
#define	HAVE_STDLIB_H 1  		/* <stdlib.h> exists */
#define	HAVE_UNISTD_H 1  		/* <unistd.h> exists */
#define	HAVE_UTIME_H 1  		/* <utime.h> exists */

#define	HAVE_SYS_PARAM_H 1  	/* <sys/param.h> exists */
/* #undef	HAVE_SYS_SELECT_H */  	/* <sys/select.h> exists */
#define	HAVE_SYS_TIMES_H 1  	/* <sys/times.h> exists */
/* #undef	HAVE_SYS_TIME_H */  	/* <sys/time.h> exists */
#define	HAVE_SYS_UTSNAME_H 1  	/* <sys.utsname.h> exists */

#define	TIME_WITH_SYS_TIME 1  	/* <sys/time.h> and <time.h> can be included
				   together */

/* #undef	HAVE_TM_ZONE */  		/* struct tm has a tm_zone member */
#define	HAVE_TZNAME 1  		/* extern char *tzname[] exists */

#define	HAVE_CLOCK 1  		/* clock() exists */
/* #undef	HAVE_FTIME */  		/* ftime() exists */
#define	HAVE_GETPGRP 1  		/* getpgrp() exists */
#define	HAVE_GETTIMEOFDAY 1  	/* gettimeofday() exists */
#define	HAVE_LSTAT 1  		/* lstat() exists */
#define	HAVE_PROTOTYPES 1  	/* the compiler understands prototypes */
#define	HAVE_READLINK 1  		/* readlink() exists */
#define	HAVE_SELECT 1  		/* select() exists */
#define	HAVE_SETPGID 1  		/* setpgid() exists */
#define	HAVE_SETPGRP 1  		/* setpgrp() exists */
#define	HAVE_SETSID 1  		/* setsid() exists */
#define	HAVE_SYMLINK 1  		/* symlink() exists */
/* #undef	HAVE_SIGINTERRUPT */  	/* siginterrupt() exists */
#define	HAVE_TCGETPGRP 1  	/* tcgetpgrp() exists */
#define	HAVE_TCSETPGRP 1  	/* tcsetpgrp() exists */
#define	HAVE_TIMES 1  		/* times() exists */
#define	HAVE_UNAME 1  		/* uname() exists */
#define	HAVE_WAITPID 1  		/* waitpid() exists */

/* #undef	GETPGRP_HAVE_ARG */  	/* getpgrp() must be called as getpgrp(0)
				   (and setpgrp() as setpgrp(0, 0)) */

#define	WITH_READLINE 1  		/* GNU readline() should be used */
/* #undef	USE_THREAD */		/* Build in thread support */
/* #undef	SOLARIS */			/* This is SOLARIS 2.x */
