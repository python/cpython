/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* Macintosh Python main program */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef THINK_C
#define CONSOLE_IO
#endif

#include <stdio.h>
#include <string.h>

#ifdef CONSOLE_IO
#include <console.h>
#endif

main(argc, argv)
	int argc;
	char **argv;
{
#ifdef USE_STDWIN
	/* Use STDWIN's wargs() to set argc/argv to list of files to open */
	wargs(&argc, &argv);
	/* Put About Python... in Apple menu */
	{
		extern char *about_message;
		extern char *about_item;
		extern char *getversion(), *getcopyright();
		static char buf[256];
		sprintf(buf, "Python %s\r\
\r\
%s\r\
\r\
Author: Guido van Rossum <guido@cwi.nl>\r\
FTP: host ftp.cwi.nl, directory pub/python\r\
Newsgroup: comp.lang.python\r\
\r\
Motto: \"Nobody expects the Spanish Inquisition!\"",
			getversion(), getcopyright());
		about_message = buf;
		about_item = "About Python...";
	}
#endif /* USE_STDWIN */
#ifdef CONSOLE_IO
	if (argc >= 1 && argv[0][0] != '\0') {
		static char buf[256];
		buf[0] = strlen(argv[0]);
		strncpy(buf+1, argv[0], buf[0]);
		console_options.title = (unsigned char *)buf;
	}
	else
		console_options.title = "\pPython";
#endif /* CONSOLE_IO */
	realmain(argc, argv);
}
