/*
 * tclStringTrim.h --
 *
 *	This file contains the definition of what characters are to be trimmed
 *	from a string by [string trim] by default. It's only needed by Tcl's
 *	implementation; it does not form a public or private API at all.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 1998-2000 Scriptics Corporation.
 * Copyright (c) 2002 ActiveState Corporation.
 * Copyright (c) 2003-2013 Donal K. Fellows.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef TCL_STRING_TRIM_H
#define TCL_STRING_TRIM_H

/*
 * Default set of characters to trim in [string trim] and friends. This is a
 * UTF-8 literal string containing all Unicode space characters. [TIP #413]
 */

MODULE_SCOPE const char tclDefaultTrimSet[];

/*
 * The whitespace trimming set used when [concat]enating. This is a subset of
 * the above, and deliberately so.
 */

#define CONCAT_TRIM_SET " \f\v\r\t\n"

#endif /* TCL_STRING_TRIM_H */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
