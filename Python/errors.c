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
     but this means that when such an error code is reported by UNIX the
     user gets a confusing message
   - errno is a global variable, which makes extensions to a multi-
     threading environment difficult; e.g., in IRIX, multi-threaded
     programs must use the function getoserror() (sp.?) instead of
     looking in errno
   - there is no portable way to add new error numbers for specic
     situations -- the value space for errno is reserved to the OS, yet
     the way to turn module-specific errors into a module-specific
     exception requires module-specific values for errno
   - there is no way to add a more situation-specific message to an
     error.
  
  The new interface solves all these problems.  To return an error, a
  built-in function calls err_set(exception), err_set(valexception,
  value) or err_setstr(exception, string), and returns NULL.  These
  functions save the value for later use by puterrno().  To adapt this
  scheme to a multi-threaded environment, only the implementation of
  err_setval() has to be changed.
*/

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "stringobject.h"
#include "errors.h"

/* Last exception stored by err_setval() */

static object *last_exception;
static object *last_exc_val;

void
err_setval(exception, value)
	object *exception;
	object *value;
{
	if (last_exception != NULL)
		DECREF(last_exception);
	if (exception != NULL)
		INCREF(exception);
	last_exception = exception;
	
	if (last_exc_val != NULL)
		DECREF(last_exc_val);
	if (value != NULL)
		INCREF(value);
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
	if (value != NULL)
		DECREF(value);
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
	if (last_exception != NULL) {
		DECREF(last_exception);
		last_exception = NULL;
	}
	if (last_exc_val != NULL) {
		DECREF(last_exc_val);
		last_exc_val = NULL;
	}
}

/* Convenience functions to set a type error exception and return 0 */

int
err_badarg()
{
	err_setstr(TypeError, "illegal argument type for built-in function");
	return 0;
}

object *
err_nomem()
{
	err_setstr(MemoryError, "in built-in function");
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
	if (v != NULL)
		DECREF(v);
	return NULL;
}
