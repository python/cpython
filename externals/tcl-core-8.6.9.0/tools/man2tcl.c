/*
 * man2tcl.c --
 *
 *	This file contains a program that turns a man page of the form used
 *	for Tcl and Tk into a Tcl script that invokes a Tcl command for each
 *	construct in the man page. The script can then be eval'ed to translate
 *	the manual entry into some other format such as MIF or HTML.
 *
 * Usage:
 *
 *	man2tcl ?fileName?
 *
 * Copyright (c) 1995 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

static char sccsid[] = "@(#) man2tcl.c 1.3 95/08/12 17:34:08";

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/*
 * Imported things that aren't defined in header files:
 */

/*
 * Some <errno.h> define errno to be something complex and thread-aware; in
 * that case we definitely do not want to declare errno ourselves!
 */

#ifndef errno
extern int errno;
#endif

/*
 * Current line number, used for error messages.
 */

static int lineNumber;

/*
 * The variable below is set to 1 if an error occurs anywhere while reading in
 * the file.
 */

static int status;

/*
 * The variable below is set to 1 if output should be generated. If it's 0, it
 * means we're doing a pre-pass to make sure that the file can be properly
 * parsed.
 */

static int writeOutput;

#define PRINT(args)	if (writeOutput) { printf args; }
#define PRINTC(chr)	if (writeOutput) { putc((chr), stdout); }

/*
 * Prototypes for functions defined in this file:
 */

static void		DoMacro(char *line);
static void		DoText(char *line);
static void		QuoteText(char *string, int count);

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This function is the main program, which does all of the work of the
 *	program.
 *
 * Results:
 *	None: exits with a 0 return status to indicate success, or 1 to
 *	indicate that there were problems in the translation.
 *
 * Side effects:
 *	A Tcl script is output to standard output. Error messages may be
 *	output on standard error.
 *
 *----------------------------------------------------------------------
 */

int
main(
    int argc,			/* Number of command-line arguments. */
    char **argv)		/* Values of command-line arguments. */
{
    FILE *f;
#define MAX_LINE_SIZE 4000
    char line[MAX_LINE_SIZE];
    char *p;

    /*
     * Find the file to read, and open it if it isn't stdin.
     */

    if (argc == 1) {
	f = stdin;
    } else if (argc == 2) {
	f = fopen(argv[1], "r");
	if (f == NULL) {
	    fprintf(stderr, "Couldn't read \"%s\": %s\n", argv[1],
		    strerror(errno));
	    exit(1);
	}
    } else {
	fprintf(stderr, "Usage: %s ?fileName?\n", argv[0]);
    }

    /*
     * Make two passes over the file. In the first pass, just check to make
     * sure we can handle everything. If there are problems, generate output
     * and stop. If everything is OK, make a second pass to actually generate
     * output.
     */

    for (writeOutput = 0; writeOutput < 2; writeOutput++) {
	lineNumber = 0;
	status = 0;
	while (fgets(line, MAX_LINE_SIZE, f) != NULL) {
	    for (p = line; *p != 0; p++) {
		if (*p == '\n') {
		    *p = 0;
		    break;
		}
	    }
	    lineNumber++;

	    if (((line[0] == '.') || (line[0] == '\'')) && (line[1] == '\\') && (line[2] == '\"')) {
		/*
		 * This line is a comment. Ignore it.
		 */

		continue;
	    }

	    if (strlen(line) >= MAX_LINE_SIZE -1) {
		fprintf(stderr, "Too long line. Max is %d chars.\n",
			MAX_LINE_SIZE - 1);
		exit(1);
	    }

	    if ((line[0] == '.') || (line[0] == '\'')) {
		/*
		 * This line is a macro invocation.
		 */

		DoMacro(line);
	    } else {
		/*
		 * This line is text, possibly with formatting characters
		 * embedded in it.
		 */

		DoText(line);
	    }
	}
	if (status != 0) {
	    break;
	}
	fseek(f, 0, SEEK_SET);
    }
    exit(status);
}

/*
 *----------------------------------------------------------------------
 *
 * DoMacro --
 *
 *	This function is called to handle a macro invocation. It parses the
 *	arguments to the macro and generates a Tcl command to handle the
 *	invocation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A Tcl command is written to stdout.
 *
 *----------------------------------------------------------------------
 */

static void
DoMacro(
    char *line)			/* The line of text that contains the macro
				 * invocation. */
{
    char *p, *end;
    int quote;

    /*
     * If there is no macro name, then just skip the whole line.
     */

    if ((line[1] == 0) || (isspace(line[1]))) {
	return;
    }

    PRINT(("macro"));
    if (*line != '.') {
	PRINT(("2"));
    }

    /*
     * Parse the arguments to the macro (including the name), in order.
     */

    p = line+1;
    while (1) {
	PRINTC(' ');
	if (*p == '"') {
	    /*
	     * The argument is delimited by quotes.
	     */

	    for (end = p+1; *end != '"'; end++) {
		if (*end == 0) {
		    fprintf(stderr,
			    "Unclosed quote in macro call on line %d.\n",
			    lineNumber);
		    status = 1;
		    break;
		}
	    }
	    QuoteText(p+1, (end-(p+1)));
	} else {
	    quote = 0;
	    for (end = p+1; (*end != 0) && (quote || !isspace(*end)); end++) {
		if (*end == '\'') {
		    quote = !quote;
		}
	    }
	    QuoteText(p, end-p);
	}
	if (*end == 0) {
	    break;
	}
	p = end+1;
	while (isspace(*p)) {
	    /*
	     * Skip empty space before next argument.
	     */

	    p++;
	}
	if (*p == 0) {
	    break;
	}
    }
    PRINTC('\n');
}

/*
 *----------------------------------------------------------------------
 *
 * DoText --
 *
 *	This function is called to handle a line of troff text. It parses the
 *	text, generating Tcl commands for text and for formatting stuff such
 *	as font changes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tcl commands are written to stdout.
 *
 *----------------------------------------------------------------------
 */

static void
DoText(
    char *line)			/* The line of text. */
{
    char *p, *end;

    /*
     * Divide the line up into pieces consisting of backslash sequences, tabs,
     * and other text.
     */

    p = line;
    while (*p != 0) {
	if (*p == '\t') {
	    PRINT(("tab\n"));
	    p++;
	} else if (*p != '\\') {
	    /*
	     * Ordinary text.
	     */

	    for (end = p+1; (*end != '\\') && (*end != 0); end++) {
		/* Empty loop body. */
	    }
	    PRINT(("text "));
	    QuoteText(p, end-p);
	    PRINTC('\n');
	    p = end;
	} else {
	    /*
	     * A backslash sequence. There are particular ones that we
	     * understand; output an error message for anything else and just
	     * ignore the backslash.
	     */

	    p++;
	    if (*p == 'f') {
		/*
		 * Font change.
		 */

		PRINT(("font %c\n", p[1]));
		p += 2;
	    } else if (*p == '-') {
		PRINT(("dash\n"));
		p++;
	    } else if (*p == 'e') {
		PRINT(("text \\\\\n"));
		p++;
	    } else if (*p == '.') {
		PRINT(("text .\n"));
		p++;
	    } else if (*p == '&') {
		p++;
	    } else if (*p == '0') {
		PRINT(("text { }\n"));
		p++;
	    } else if (*p == '(') {
		if ((p[1] == 0) || (p[2] == 0)) {
		    fprintf(stderr, "Bad \\( sequence on line %d.\n",
			    lineNumber);
		    status = 1;
		} else {
		    PRINT(("char {\\(%c%c}\n", p[1], p[2]));
		    p += 3;
		}
	    } else if (*p == 'N' && *(p+1) == '\'') {
		int ch;

		p += 2;
		sscanf(p,"%d",&ch);
		PRINT(("text \\u%04x\n", ch));
		while(*p&&*p!='\'') p++;
		p++;
	    } else if (*p != 0) {
		PRINT(("char {\\%c}\n", *p));
		p++;
	    }
	}
    }
    PRINT(("newline\n"));
}

/*
 *----------------------------------------------------------------------
 *
 * QuoteText --
 *
 *	Copy the "string" argument to stdout, adding quote characters around
 *	any special Tcl characters so that they'll just be treated as ordinary
 *	text.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Text is written to stdout.
 *
 *----------------------------------------------------------------------
 */

static void
QuoteText(
    char *string,		/* The line of text. */
    int count)			/* Number of characters to write from
				 * string. */
{
    if (count == 0) {
	PRINT(("{}"));
	return;
    }
    for ( ; count > 0; string++, count--) {
	switch (*string) {
	case '\\':
	    if (*(string+1) == 'N' && *(string+2) == '\'') {
		int ch;

		string += 3;
		count -= 3;
		sscanf(string,"%d",&ch);
		PRINT(("\\u%04x", ch));
		while(count>0&&*string!='\'') {string++;count--;}
		continue;
	    } else if (*(string+1) == '0') {
		PRINT(("\\ "));
		string++;
		count--;
		continue;
	    }
	case '$': case '[': case '{': case ' ': case ';':
	case '"': case '\t':
	    PRINTC('\\');
	default:
	    PRINTC(*string);
	}
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
