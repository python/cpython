/*
** mwfopenrf - Open resource fork as stdio file for CodeWarrior.
**
** Jack Jansen, CWI, August 1995.
*/

#ifdef __MWERKS__
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
