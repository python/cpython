#ifndef Py_MYSELECT_H
#define Py_MYSELECT_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.

******************************************************************/

/* Include file for users of select() */

/* NB caller must include <sys/types.h> */

#ifdef HAVE_SYS_SELECT_H

#ifdef SYS_SELECT_WITH_SYS_TIME
#include "mytime.h"
#else /* !SYS_SELECT_WITH_SYS_TIME */
#include <time.h>
#endif /* !SYS_SELECT_WITH_SYS_TIME */

#include <sys/select.h>

#else /* !HAVE_SYS_SELECT_H */

#ifdef USE_GUSI1
/* If we don't have sys/select the definition may be in unistd.h */
#include <GUSI.h>
#endif

#include "mytime.h"

#endif /* !HAVE_SYS_SELECT_H */

/* If the fd manipulation macros aren't defined,
   here is a set that should do the job */

#ifndef	FD_SETSIZE
#define	FD_SETSIZE	256
#endif

#ifndef FD_SET

typedef long fd_mask;

#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */
#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif /* howmany */

typedef	struct fd_set {
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	memset((char *)(p), '\0', sizeof(*(p)))

#endif /* FD_SET */

#ifdef __cplusplus
}
#endif
#endif /* !Py_MYSELECT_H */
