/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* $Id$ */

/* Regular expression objects */
/* This uses Tatu Ylonen's copyleft-free reimplementation of
   GNU regular expressions */

#include "Python.h"

#include <ctype.h>

#include "regexpr.h"

static PyObject *ReopError;	/* Exception */	

static PyObject *
makeresult(regs, num_regs)
	struct re_registers *regs;
	int num_regs;
{
	PyObject *v;
	int i;
	static PyObject *filler = NULL;

	if (filler == NULL) {
		filler = Py_BuildValue("(ii)", -1, -1);
		if (filler == NULL)
			return NULL;
	}
	v = PyTuple_New(num_regs);
	if (v == NULL)
		return NULL;

	for (i = 0; i < num_regs; i++) {
		int lo = regs->start[i];
		int hi = regs->end[i];
		PyObject *w;
		if (lo == -1 && hi == -1) {
			w = filler;
			Py_INCREF(w);
		}
		else
			w = Py_BuildValue("(ii)", lo, hi);
		if (w == NULL || PyTuple_SetItem(v, i, w) < 0) {
			Py_DECREF(v);
			return NULL;
		}
	}
	return v;
}

static PyObject *
reop_match(self, args)
	PyObject *self;
	PyObject *args;
{
	char *string;
	int fastmaplen, stringlen;
	int can_be_null, anchor, i;
	int num_regs, flags, pos, result;
	struct re_pattern_buffer bufp;
	struct re_registers re_regs;
	
	if (!PyArg_Parse(args, "(s#iiis#is#i)", 
			 &(bufp.buffer), &(bufp.allocated), 
			 &num_regs, &flags, &can_be_null,
			 &(bufp.fastmap), &fastmaplen,
			 &anchor,
			 &string, &stringlen, 
			 &pos))
	  return NULL;

	/* XXX sanity-check the input data */
	bufp.used=bufp.allocated;
	bufp.translate=NULL;
	bufp.fastmap_accurate=1;
	bufp.can_be_null=can_be_null;
	bufp.uses_registers=1;
	bufp.num_registers=num_regs;
	bufp.anchor=anchor;
	
	for(i=0; i<num_regs; i++) {re_regs.start[i]=-1; re_regs.end[i]=-1;}
	
	result = re_match(&bufp, 
			  string, stringlen, pos, 
			  &re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		PyErr_SetString(ReopError, "match failure");
		return NULL;
	}
	return makeresult(&re_regs, num_regs);
}

static PyObject *
reop_search(self, args)
	PyObject *self;
	PyObject *args;
{
	char *string;
	int fastmaplen, stringlen;
	int can_be_null, anchor, i;
	int num_regs, flags, pos, result;
	struct re_pattern_buffer bufp;
	struct re_registers re_regs;
	
	if (!PyArg_Parse(args, "(s#iiis#is#i)", 
			 &(bufp.buffer), &(bufp.allocated), 
			 &num_regs, &flags, &can_be_null,
			 &(bufp.fastmap), &fastmaplen,
			 &anchor,
			 &string, &stringlen, 
			 &pos))
	  return NULL;

	/* XXX sanity-check the input data */
	bufp.used=bufp.allocated;
	bufp.translate=NULL;
	bufp.fastmap_accurate=1;
	bufp.can_be_null=can_be_null;
	bufp.uses_registers=1;
	bufp.num_registers=1;
	bufp.anchor=anchor;

	for(i=0; i<num_regs; i++) {re_regs.start[i]=-1; re_regs.end[i]=-1;}
	
	result = re_search(&bufp, 
			   string, stringlen, pos, stringlen-pos,
			   &re_regs);
	if (result < -1) {
		/* Failure like stack overflow */
		PyErr_SetString(ReopError, "match failure");
		return NULL;
	}
	return makeresult(&re_regs, num_regs);
}

#if 0
/* Functions originally in the regsub module.
   Added June 1, 1997. 
   */

/* A cache of previously used patterns is maintained.  Notice that if
   you change the reop syntax flag, entries in the cache are
   invalidated.  
   XXX Solution: use (syntax flag, pattern) as keys?  Clear the cache
   every so often, or once it gets past a certain size? 
*/

static PyObject *cache_dict=NULL;

/* Accept an object; if it's a reop pattern, Py_INCREF it and return
   it.  If it's a string, a reop object is compiled and cached.
*/
   
static reopobject *
cached_compile(pattern)
     PyObject *pattern;
{
  reopobject *p2;

  if (!PyString_Check(pattern)) 
    {
      /* It's not a string, so assume it's a compiled reop object */
      /* XXX check that! */
      Py_INCREF(pattern);
      return (reopobject*)pattern;
    }
  if (cache_dict==NULL)
    {
      cache_dict=PyDict_New();
      if (cache_dict==NULL) 
	{
	  return (reopobject*)NULL;
	}
    }

  /* See if the pattern has already been cached; if so, return that
     reop object */
  p2=(reopobject*)PyDict_GetItem(cache_dict, pattern);
  if (p2)
    {
      Py_INCREF(p2);
      return (reopobject*)p2;
    }

  /* Compile the pattern and cache it */
  p2=(reopobject*)newreopobject(pattern, NULL, pattern, NULL);
  if (!p2) return p2;
  PyDict_SetItem(cache_dict, pattern, (PyObject*)p2);
  return p2;
}


static PyObject *
internal_split(args, retain)
	PyObject *args;
	int retain;
{
  PyObject *newlist, *s;
  reopobject *pattern;
  int maxsplit=0, count=0, length, next=0, result;
  int match_end=0; /* match_start is defined below */
  char *start;

  if (!PyArg_ParseTuple(args, "s#Oi", &start, &length, &pattern,
			&maxsplit))
    {
      PyErr_Clear();
      if (!PyArg_ParseTuple(args, "s#O", &start, &length, &pattern))
	return NULL;
    }
  pattern=cached_compile((PyObject *)pattern);
  if (!pattern) return NULL;

  newlist=PyList_New(0);
  if (!newlist) return NULL;
  
  do
    {
      result = re_search(&pattern->re_patbuf, 
			     start, length, next, length-next,
			     &pattern->re_regs);
      if (result < -1)
	{  /* Erk... an error happened during the reop search */
	  Py_DECREF(newlist);
	  PyErr_SetString(ReopError, "match failure");
	  return NULL;
	}
      if (next<=result) 
	{
	  int match_start=pattern->re_regs.start[0];
	  int oldmatch_end=match_end;
	  match_end=pattern->re_regs.end[0];

	  if (match_start==match_end) 
	    { /* A zero-length match; increment to the next position */
	      next=result+1;
	      match_end=oldmatch_end;
	      continue;
	    }

	  /* Append the string up to the start of the match */
	  s=PyString_FromStringAndSize(start+oldmatch_end, match_start-oldmatch_end);
	  if (!s) 
	    {
	      Py_DECREF(newlist);
	      return NULL;
	    }
	  PyList_Append(newlist, s);
	  Py_DECREF(s);

	  if (retain)
	    {
	      /* Append a string containing whatever matched */
	      s=PyString_FromStringAndSize(start+match_start, match_end-match_start);
	      if (!s) 
		{
		  Py_DECREF(newlist);
		  return NULL;
		}
	      PyList_Append(newlist, s);
	      Py_DECREF(s);
	    }
	  /* Update the pointer, and increment the count of splits */
	  next=match_end; count++;
	}
    } while (result!=-1 && !(maxsplit && maxsplit==count) &&
	     next<length);
  s=PyString_FromStringAndSize(start+match_end, length-match_end);
  if (!s) 
    {
      Py_DECREF(newlist);
      return NULL;
    }
  PyList_Append(newlist, s);
  Py_DECREF(s);
  Py_DECREF(pattern);
  return newlist;
}

static PyObject *
reop_split(self, args)
	PyObject *self;
	PyObject *args;
{
  return internal_split(args, 0);
}

static PyObject *
reop_splitx(self, args)
	PyObject *self;
	PyObject *args;
{
  return internal_split(args, 1);
}
#endif

static struct PyMethodDef reop_global_methods[] = {
	{"match",	reop_match, 0},
	{"search",	reop_search, 0},
#if 0
	{"split",  reop_split, 0},
	{"splitx",  reop_splitx, 0},
#endif
	{NULL,		NULL}		     /* sentinel */
};

void
initreop()
{
	PyObject *m, *d, *v;
	int i;
	char *s;
	
	m = Py_InitModule("reop", reop_global_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize reop.error exception */
	v = ReopError = PyString_FromString("reop.error");
	if (v == NULL || PyDict_SetItemString(d, "error", v) != 0)
		goto finally;
	
	/* Initialize reop.casefold constant */
	if (!(v = PyString_FromStringAndSize((char *)NULL, 256)))
		goto finally;
	
	if (!(s = PyString_AsString(v)))
		goto finally;

	for (i = 0; i < 256; i++) {
		if (isupper(i))
			s[i] = tolower(i);
		else
			s[i] = i;
	}
	if (PyDict_SetItemString(d, "casefold", v) < 0)
		goto finally;
	Py_DECREF(v);

	if (!PyErr_Occurred())
		return;
  finally:
	Py_FatalError("can't initialize reop module");
}
