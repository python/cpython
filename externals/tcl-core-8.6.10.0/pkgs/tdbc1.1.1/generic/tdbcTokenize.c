/*
 * tdbcTokenize.c --
 *
 *	Code for a Tcl command that will extract subsitutable parameters
 *	from a SQL statement.
 *
 * Copyright (c) 2007 by D. Richard Hipp.
 * Copyright (c) 2010, 2011 by Kevin B. Kenny.
 *
 * Please refer to the file, 'license.terms' for the conditions on
 * redistribution of this file and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) $Id$
 *
 *-----------------------------------------------------------------------------
 */

#include "tdbcInt.h"
#include <ctype.h>

/*
 *-----------------------------------------------------------------------------
 *
 * Tdbc_TokenizeSql --
 *
 *	Tokenizes a SQL statement.
 *
 * Results:
 *	Returns a zero-reference Tcl object that gives the statement in
 *	tokenized form, or NULL if an error occurs.
 *
 * Side effects:
 *	If an error occurs, and 'interp' is not NULL, stores an error
 *	message in the interpreter result.
 *
 * This is demonstration code for a TCL command that will extract
 * host parameters from an SQL statement.
 *
 * A "host parameter" is a variable within the SQL statement.
 * Different systems do host parameters in different ways.  This
 * tokenizer recognizes three forms:
 *
 *            $ident
 *            :ident
 *            @ident
 *
 * In other words, a host parameter is an identifier proceeded
 * by one of the '$', ':', or '@' characters.
 *
 * This function returns a Tcl_Obj representing a list.  The
 * concatenation of the returned list will be equivalent to the
 * input string.  Each element of the list will be either a
 * host parameter, a semicolon, or other text from the SQL
 * statement.
 *
 * Example:
 *
 *      tokenize_sql {SELECT * FROM table1 WHERE :name='bob';}
 *
 * Resulting in:
 *
 *      {SELECT * FROM table1 WHERE } {:name} {=} {'bob'} {;}
 *
 * The tokenizer knows about SQL comments and strings and will
 * not mistake a host parameter or semicolon embedded in a string
 * or comment as a real host parameter or semicolon.
 *
 *-----------------------------------------------------------------------------
 */

TDBCAPI Tcl_Obj*
Tdbc_TokenizeSql(
    Tcl_Interp *interp,
    const char* zSql
){
    Tcl_Obj *resultPtr;
    int i;

    resultPtr = Tcl_NewObj();
    for(i = 0; zSql[i]; i++){
        switch( zSql[i] ){

            /* Skip over quoted strings.  Strings can be quoted in several
            ** ways:    '...'   "..."   [....]
            */
            case '\'':
            case '"':
            case '[': {
                int endChar = zSql[i];
                if (endChar == '[') endChar = ']';
                for(i++; zSql[i] && zSql[i]!=endChar; i++){}
                if (zSql[i] == 0) i--;
                break;
            }

            /* Skip over SQL-style comments: -- to end of line
            */
            case '-': {
                if (zSql[i+1] == '-') {
                     for(i+=2; zSql[i] && zSql[i]!='\n'; i++){}
                     if (zSql[i] == 0) i--;
                }
                break;
            }

            /* Skip over C-style comments
            */
            case '/': {
                if (zSql[i+1] == '*') {
                     i += 3;
                     while (zSql[i] && (zSql[i]!='/' || zSql[i-1]!='*')) {
                         i++;
                     }
                     if (zSql[i] == 0) i--;
                }
                break;
            }

            /* Break up multiple SQL statements at each semicolon */
            case ';': {
                if (i>0 ){
                    Tcl_ListObjAppendElement(interp, resultPtr,
                              Tcl_NewStringObj(zSql, i));
                }
                Tcl_ListObjAppendElement(interp, resultPtr,
                          Tcl_NewStringObj(";",1));
                zSql += i + 1;
                i = -1;
                break;
            }

            /* Any of the characters ':', '$', or '@' which is followed
            ** by an alphanumeric or '_' and is not preceded by the same
            ** is a host parameter. A name following a doubled colon '::'
	    ** is also not a host parameter.
            */
	    case ':': {
		if (i > 0 && zSql[i-1] == ':') break;
	    }
		/* fallthru */

            case '$':
            case '@': {
                if (i>0 && (isalnum((unsigned char)(zSql[i-1]))
			    || zSql[i-1]=='_')) break;
                if (!isalnum((unsigned char)(zSql[i+1]))
		    && zSql[i+1]!='_') break;
                if (i>0 ){
                    Tcl_ListObjAppendElement(interp, resultPtr,
                              Tcl_NewStringObj(zSql, i));
                    zSql += i;
                }
                i = 1;
                while (zSql[i] && (isalnum((unsigned char)(zSql[i]))
				   || zSql[i]=='_')) {
                    i++;
                }
                Tcl_ListObjAppendElement(interp, resultPtr,
                          Tcl_NewStringObj(zSql, i));
                zSql += i;
                i = -1;
                break;
            }
        }
    }
    if (i>0) {
        Tcl_ListObjAppendElement(interp, resultPtr, Tcl_NewStringObj(zSql, i));
    }
    return resultPtr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * TdbcTokenizeObjCmd --
 *
 *	Tcl command to tokenize a SQL statement.
 *
 * Usage:
 *	::tdbc::tokenize statement
 *
 * Results:
 *	Returns a list as from passing the given statement to
 *	Tdbc_TokenizeSql above.
 *
 *-----------------------------------------------------------------------------
 */

MODULE_SCOPE int
TdbcTokenizeObjCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp* interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const objv[]	/* Parameter vector */
) {

    Tcl_Obj* retval;

    /* Check param count */

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "statement");
	return TCL_ERROR;
    }

    /* Parse the statement */

    retval = Tdbc_TokenizeSql(interp, Tcl_GetString(objv[1]));
    if (retval == NULL) {
	return TCL_ERROR;
    }

    /* Return the tokenized statement */

    Tcl_SetObjResult(interp, retval);
    return TCL_OK;

}
