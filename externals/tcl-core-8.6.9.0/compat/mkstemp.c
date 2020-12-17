/*
 * mkstemp.c --
 *
 *	Source code for the "mkstemp" library routine.
 *
 * Copyright (c) 2009 Donal K. Fellows
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

/*
 *----------------------------------------------------------------------
 *
 * mkstemp --
 *
 *	Create an open temporary file from a template.
 *
 * Results:
 *	A file descriptor, or -1 (with errno set) in the case of an error.
 *
 * Side effects:
 *	The template is updated to contain the real filename.
 *
 *----------------------------------------------------------------------
 */

int
mkstemp(
    char *template)		/* Template for filename. */
{
    static const char alphanumerics[] =
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    register char *a, *b;
    int fd, count, alphanumericsLen = strlen(alphanumerics); /* == 62 */

    a = template + strlen(template);
    while (a > template && *(a-1) == 'X') {
	a--;
    }

    if (a == template) {
	errno = ENOENT;
	return -1;
    }

    /*
     * We'll only try up to 10 times; after that, we're suffering from enemy
     * action and should let the caller know.
     */

    count = 10;
    do {
	/*
	 * Replace the X's in the original template with random alphanumeric
	 * digits.
	 */

	for (b=a ; *b ; b++) {
	    float r = rand() / ((float) RAND_MAX);

	    *b = alphanumerics[(int)(r * alphanumericsLen)];
	}

	/*
	 * Template is now realized; try to open (with correct options).
	 */

	fd = open(template, O_RDWR|O_CREAT|O_EXCL, 0600);
    } while (fd == -1 && errno == EEXIST && --count > 0);

    return fd;
}
