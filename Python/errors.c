/***********************************************************
Copyright 1991 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

#include <errno.h>
#ifndef errno
extern int errno;
#endif

#include "errcode.h"

extern char *strerror PROTO((int));

/* Last exception stored by err_setval() */

static object *last_exception;
static object *last_exc_val;

void
err_setval(exception, value)
	object *exception;
	object *value;
{
	XDECREF(last_exception);
	XINCREF(exception);
	last_exception = exception;
	
	XDECREF(last_exc_val);
	XINCREF(value);
	last_exc_val = value;
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

int
err_occurred()
{
	return last_exception != NULL;
}

void
err_get(p_exc, p_val)
	object **p_exc;
	object **p_val;
{
	*p_exc = last_exception;
	last_exception = NULL;
	*p_val = last_exc_val;
	last_exc_val = NULL;
}

void
err_clear()
{
	XDECREF(last_exception);
	last_exception = NULL;
	XDECREF(last_exc_val);
	last_exc_val = NULL;
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
	object *v = newtupleobject(2);
	if (v != NULL) {
		settupleitem(v, 0, newintobject((long)errno));
		settupleitem(v, 1, newstringobject(strerror(errno)));
	}
	err_setval(exc, v);
	XDECREF(v);
	return NULL;
}

void
err_badcall()
{
	err_setstr(SystemError, "bad argument to internal function");
}

/* Set the error appropriate to the given input error code (see errcode.h) */

void
err_input(err)
	int err;
{
	switch (err) {
	case E_DONE:
	case E_OK:
		break;
	case E_SYNTAX:
		err_setstr(RuntimeError, "syntax error");
		break;
	case E_TOKEN:
		err_setstr(RuntimeError, "illegal token");
		break;
	case E_INTR:
		err_set(KeyboardInterrupt);
		break;
	case E_NOMEM:
		err_nomem();
		break;
	case E_EOF:
		err_set(EOFError);
		break;
	default:
		err_setstr(RuntimeError, "unknown input error");
		break;
	}
}
