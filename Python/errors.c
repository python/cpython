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

/* Error handling -- see also run.c */

/* New error handling interface.

   The following problem exists (existed): methods of built-in modules
   are called with 'self' and 'args' arguments, but without a context
   argument, so they have no way to raise a specific exception.
   The same is true for the object implementations: no context argument.
   The old convention was to set 'errno' and to return NULL.
   The caller (usually call_function() in eval.c) detects the NULL
   return value and then calls puterrno(ctx) to turn the errno value
   into a true exception.  Problems with this approach are:
   - it used standard errno values to indicate Python-specific errors,
     but this means that when such an error code is reported by a system
     call (e.g., in module posix), the user gets a confusing message
   - errno is a global variable, which makes extensions to a multi-
     threading environment difficult; e.g., in IRIX, multi-threaded
     programs must use the function oserror() instead of looking in errno
   - there is no portable way to add new error numbers for specic
     situations -- the value space for errno is reserved to the OS, yet
     the way to turn module-specific errors into a module-specific
     exception requires module-specific values for errno
   - there is no way to add a more situation-specific message to an
     error.
  
  The new interface solves all these problems.  To return an error, a
  built-in function calls err_set(exception), err_setval(exception,
  value) or err_setstr(exception, string), and returns NULL.  These
  functions save the value for later use by puterrno().  To adapt this
  scheme to a multi-threaded environment, only the implementation of
  err_setval() has to be changed.
*/

#include "allobjects.h"
#include "traceback.h"

#include <errno.h>

#ifdef SYMANTEC__CFM68K__
#pragma lib_export on
#endif

#ifdef macintosh
/* Replace strerror with a Mac specific routine.
   XXX PROBLEM: some positive errors have a meaning for MacOS,
   but some library routines set Unix error numbers...
*/
extern char *PyMac_StrError PROTO((int));
#undef strerror
#define strerror PyMac_StrError
#endif /* macintosh */

#ifndef __STDC__
extern char *strerror PROTO((int));
#endif

/* Last exception stored by err_setval() */

static object *last_exception;
static object *last_exc_val;

void
err_restore(exception, value, traceback)
	object *exception;
	object *value;
	object *traceback;
{
	err_clear();

	last_exception = exception;
	last_exc_val = value;
	(void) tb_store(traceback);
	XDECREF(traceback);
}

void
err_setval(exception, value)
	object *exception;
	object *value;
{
	XINCREF(exception);
	XINCREF(value);
	err_restore(exception, value, (object *)NULL);
}

void
err_set(exception)
	object *exception;
{
	err_setval(exception, (object *)NULL);
}

void
err_setstr(exception, string)
	object *exception;
	char *string;
{
	object *value = newstringobject(string);
	err_setval(exception, value);
	XDECREF(value);
}


object *
err_occurred()
{
	return last_exception;
}

void
err_fetch(p_exc, p_val, p_tb)
	object **p_exc;
	object **p_val;
	object **p_tb;
{
	*p_exc = last_exception;
	last_exception = NULL;
	*p_val = last_exc_val;
	last_exc_val = NULL;
	*p_tb = tb_fetch();
}

void
err_clear()
{
	object *tb;
	XDECREF(last_exception);
	last_exception = NULL;
	XDECREF(last_exc_val);
	last_exc_val = NULL;
	/* Also clear interpreter stack trace */
	tb = tb_fetch();
	XDECREF(tb);
}

/* Convenience functions to set a type error exception and return 0 */

int
err_badarg()
{
	err_setstr(TypeError, "illegal argument type for built-in operation");
	return 0;
}

object *
err_nomem()
{
	err_set(MemoryError);
	return NULL;
}

object *
err_errno(exc)
	object *exc;
{
	object *v;
	int i = errno;
#ifdef EINTR
	if (i == EINTR && sigcheck())
		return NULL;
#endif
	v = mkvalue("(is)", i, strerror(i));
	if (v != NULL) {
		err_setval(exc, v);
		DECREF(v);
	}
	return NULL;
}

void
err_badcall()
{
	fatal("err_badcall() called");
	err_setstr(SystemError, "bad argument to internal function");
}
