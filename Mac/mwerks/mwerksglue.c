/*
** Glue code for MetroWerks CodeWarrior, which misses
** unix-like routines for file-access.
*/

#ifdef __MWERKS__
#include <Types.h>
#include <Files.h>
#include <Strings.h>

#include <stdio.h>
#include <errno.h>

int
fileno(fp)
	FILE *fp;
{
	if (fp==stdin) return 0;
	else if (fp==stdout) return 1;
	else if (fp==stderr) return 2;
	else return 3;
}

int
isatty(fd)
	int fd;
{
	return (fd >= 0 && fd <= 2);
}

int
unlink(old)
	char *old;
{
	OSErr err;
	
	if ((err=FSDelete((ConstStr255Param)Pstring(old), 0)) == noErr)
		return 0;
	errno= err;
	return -1;
}

#endif /* __MWERKS__ */