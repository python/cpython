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

/* Functions used by cgen output */

#include <stdio.h>

#include "PROTO.h"
#include "object.h"
#include "intobject.h"
#include "floatobject.h"
#include "stringobject.h"
#include "tupleobject.h"
#include "listobject.h"
#include "methodobject.h"
#include "moduleobject.h"
#include "modsupport.h"
#include "import.h"
#include "cgensupport.h"
#include "errors.h"


/* Functions to construct return values */

object *
mknewcharobject(c)
	int c;
{
	char ch[1];
	ch[0] = c;
	return newsizedstringobject(ch, 1);
}

/* Functions to extract arguments.
   These needs to know the total number of arguments supplied,
   since the argument list is a tuple only of there is more than
   one argument. */

int
getiobjectarg(args, nargs, i, p_arg)
	register object *args;
	int nargs, i;
	object **p_arg;
{
	if (nargs != 1) {
		if (args == NULL || !is_tupleobject(args) ||
				nargs != gettuplesize(args) ||
				i < 0 || i >= nargs) {
			return err_badarg();
		}
		else {
			args = gettupleitem(args, i);
		}
	}
	if (args == NULL) {
		return err_badarg();
	}
	*p_arg = args;
	return 1;
}

int
getilongarg(args, nargs, i, p_arg)
	register object *args;
	int nargs, i;
	long *p_arg;
{
	if (nargs != 1) {
		if (args == NULL || !is_tupleobject(args) ||
				nargs != gettuplesize(args) ||
				i < 0 || i >= nargs) {
			return err_badarg();
		}
		args = gettupleitem(args, i);
	}
	if (args == NULL || !is_intobject(args)) {
		return err_badarg();
	}
	*p_arg = getintvalue(args);
	return 1;
}

int
getishortarg(args, nargs, i, p_arg)
	register object *args;
	int nargs, i;
	short *p_arg;
{
	long x;
	if (!getilongarg(args, nargs, i, &x))
		return 0;
	*p_arg = x;
	return 1;
}

static int
extractdouble(v, p_arg)
	register object *v;
	double *p_arg;
{
	if (v == NULL) {
		/* Fall through to error return at end of function */
	}
	else if (is_floatobject(v)) {
		*p_arg = GETFLOATVALUE((floatobject *)v);
		return 1;
	}
	else if (is_intobject(v)) {
		*p_arg = GETINTVALUE((intobject *)v);
		return 1;
	}
	return err_badarg();
}

static int
extractfloat(v, p_arg)
	register object *v;
	float *p_arg;
{
	if (v == NULL) {
		/* Fall through to error return at end of function */
	}
	else if (is_floatobject(v)) {
		*p_arg = GETFLOATVALUE((floatobject *)v);
		return 1;
	}
	else if (is_intobject(v)) {
		*p_arg = GETINTVALUE((intobject *)v);
		return 1;
	}
	return err_badarg();
}

int
getifloatarg(args, nargs, i, p_arg)
	register object *args;
	int nargs, i;
	float *p_arg;
{
	object *v;
	float x;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (!extractfloat(v, &x))
		return 0;
	*p_arg = x;
	return 1;
}

int
getistringarg(args, nargs, i, p_arg)
	object *args;
	int nargs, i;
	string *p_arg;
{
	object *v;
	if (!getiobjectarg(args, nargs, i, &v))
		return NULL;
	if (!is_stringobject(v)) {
		return err_badarg();
	}
	*p_arg = getstringvalue(v);
	return 1;
}

int
getichararg(args, nargs, i, p_arg)
	object *args;
	int nargs, i;
	char *p_arg;
{
	string x;
	if (!getistringarg(args, nargs, i, &x))
		return 0;
	if (x[0] == '\0' || x[1] != '\0') {
		/* Not exactly one char */
		return err_badarg();
	}
	*p_arg = x[0];
	return 1;
}

int
getilongarraysize(args, nargs, i, p_arg)
	object *args;
	int nargs, i;
	long *p_arg;
{
	object *v;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (is_tupleobject(v)) {
		*p_arg = gettuplesize(v);
		return 1;
	}
	if (is_listobject(v)) {
		*p_arg = getlistsize(v);
		return 1;
	}
	return err_badarg();
}

int
getishortarraysize(args, nargs, i, p_arg)
	object *args;
	int nargs, i;
	short *p_arg;
{
	long x;
	if (!getilongarraysize(args, nargs, i, &x))
		return 0;
	*p_arg = x;
	return 1;
}

/* XXX The following four are too similar.  Should share more code. */

int
getilongarray(args, nargs, i, n, p_arg)
	object *args;
	int nargs, i;
	int n;
	long *p_arg; /* [n] */
{
	object *v, *w;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (is_tupleobject(v)) {
		if (gettuplesize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = gettupleitem(v, i);
			if (!is_intobject(w)) {
				return err_badarg();
			}
			p_arg[i] = getintvalue(w);
		}
		return 1;
	}
	else if (is_listobject(v)) {
		if (getlistsize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = getlistitem(v, i);
			if (!is_intobject(w)) {
				return err_badarg();
			}
			p_arg[i] = getintvalue(w);
		}
		return 1;
	}
	else {
		return err_badarg();
	}
}

int
getishortarray(args, nargs, i, n, p_arg)
	object *args;
	int nargs, i;
	int n;
	short *p_arg; /* [n] */
{
	object *v, *w;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (is_tupleobject(v)) {
		if (gettuplesize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = gettupleitem(v, i);
			if (!is_intobject(w)) {
				return err_badarg();
			}
			p_arg[i] = getintvalue(w);
		}
		return 1;
	}
	else if (is_listobject(v)) {
		if (getlistsize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = getlistitem(v, i);
			if (!is_intobject(w)) {
				return err_badarg();
			}
			p_arg[i] = getintvalue(w);
		}
		return 1;
	}
	else {
		return err_badarg();
	}
}

int
getidoublearray(args, nargs, i, n, p_arg)
	object *args;
	int nargs, i;
	int n;
	double *p_arg; /* [n] */
{
	object *v, *w;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (is_tupleobject(v)) {
		if (gettuplesize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = gettupleitem(v, i);
			if (!extractdouble(w, &p_arg[i]))
				return 0;
		}
		return 1;
	}
	else if (is_listobject(v)) {
		if (getlistsize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = getlistitem(v, i);
			if (!extractdouble(w, &p_arg[i]))
				return 0;
		}
		return 1;
	}
	else {
		return err_badarg();
	}
}

int
getifloatarray(args, nargs, i, n, p_arg)
	object *args;
	int nargs, i;
	int n;
	float *p_arg; /* [n] */
{
	object *v, *w;
	if (!getiobjectarg(args, nargs, i, &v))
		return 0;
	if (is_tupleobject(v)) {
		if (gettuplesize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = gettupleitem(v, i);
			if (!extractfloat(w, &p_arg[i]))
				return 0;
		}
		return 1;
	}
	else if (is_listobject(v)) {
		if (getlistsize(v) != n) {
			return err_badarg();
		}
		for (i = 0; i < n; i++) {
			w = getlistitem(v, i);
			if (!extractfloat(w, &p_arg[i]))
				return 0;
		}
		return 1;
	}
	else {
		return err_badarg();
	}
}
