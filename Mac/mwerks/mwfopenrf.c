/***********************************************************
Copyright 1991-1997 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** mwfopenrf - Open resource fork as stdio file for CodeWarrior.
*/

#if defined(__MWERKS__) && !defined(USE_GUSI)
#include <stdio.h>
#include <unix.h>
#include <errno.h>
#include "errno_unix.h"

FILE *
fopenRF(name, mode)
	char *name;
	char *mode;
{
	int fd;
	int modebits = -1;
	int extramodebits = 0;
	char *modep;
	
	for(modep=mode; *modep; modep++) {
		switch(*modep) {
		case 'r': modebits = O_RDONLY; break;
		case 'w': modebits = O_WRONLY; extramodebits |= O_CREAT|O_TRUNC; break;
		case 'a': modebits = O_RDONLY;
				  extramodebits |= O_CREAT|O_APPEND;
				  extramodebits &= ~O_TRUNC;
				  break;
		case '+': modebits = O_RDWR; 
				  extramodebits &= ~O_TRUNC;
				  break;
		case 'b': extramodebits |= O_BINARY;
				  break;
		default:
				  errno = EINVAL;
				  return NULL;
		}
	}
	if ( modebits == -1 ) {
		errno = EINVAL;
		return NULL;
	}
	fd = open(name, modebits|extramodebits|O_RSRC);
	if ( fd < 0 )
		return NULL;
	return fdopen(fd, mode);
}
#endif /* __MWERKS__ */
