/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

#include "rename2.h"
#include "mymalloc.h"

#ifdef THINK_C
#define CONSOLE_IO
#endif

#include <stdio.h>
#include <string.h>

#ifdef CONSOLE_IO
#include <console.h>
#endif

#ifdef __MWERKS__
#include <SIOUX.h>
#endif

extern char *fileargument;

main(argc, argv)
	int argc;
	char **argv;
{
#ifdef USE_MAC_SHARED_LIBRARY
	PyMac_AddLibResources();
#endif
#ifdef __MWERKS__
	SIOUXSettings.asktosaveonclose = 0;
	SIOUXSettings.showstatusline = 0;
	SIOUXSettings.tabspaces = 4;
#endif
#ifdef USE_STDWIN
#ifdef THINK_C
	/* This is done to initialize the Think console I/O library before stdwin.
	   If we don't do this, the console I/O library will only be usable for
	   output, and the interactive version of the interpreter will quit
	   immediately because it sees an EOF from stdin.
	   The disadvantage is that when using STDWIN, your stdwin menus will
	   appear next to the console I/O's File and Edit menus, and you will have
	   an empty console window in your application (though it can be removed
	   by clever use of console library I believe).
	   Remove this line if you want to be able to double-click Python scripts
	   that use STDWIN and never use stdin for input.
	   (A more dynamic solution may be possible e.g. based on bits in the
	   SIZE resource or whatever...  Have fun, and let me know if you find
	   a better way!) */
	printf("\n");
#endif
#ifdef BUILD_APPLET_TEMPLATE
	/* Make argv[0] and [1] be application name. The "argument" will later
	** be recognized as APPL type and interpreted as being a .pyc file.
	** XXXX Should be changed. Argv[0] should be the shared lib location or
	** something, so we can find our Lib directory, etc.
	*/
	{
		char *progname;
		extern char *getappname();
		
		progname = getappname();
		if ( (argv = (char **)malloc(3*sizeof(char *))) == NULL ) {
			fprintf(stderr, "No memory\n");
			exit(1);
		}
		argv[0] = malloc(strlen(progname)+1);
		argv[1] = malloc(strlen(progname)+1);
		argv[2] = NULL;
		if ( argv[0] == NULL || argv[1] == NULL ) {
			fprintf(stderr, "No memory\n");
			exit(1);
		}
		strcpy(argv[0], progname);
		strcpy(argv[1], progname);
		argc = 2;
	}
#else
	/* Use STDWIN's wargs() to set argc/argv to list of files to open */
	wargs(&argc, &argv);
#endif
	/* Put About Python... in Apple menu */
	{
		extern char *about_message;
		extern char *about_item;
		extern char *getversion(), *getcopyright();
		static char buf[1024];
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
	if ( argc > 1 )
		fileargument = argv[1];  /* Mod by Jack to do chdir */
	realmain(argc, argv);
}
