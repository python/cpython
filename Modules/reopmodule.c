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

#define IGNORECASE 0x01
#define MULTILINE  0x02
#define DOTALL     0x04
#define VERBOSE    0x08

#define NORMAL			0
#define CHARCLASS		1
#define REPLACEMENT		2

#define CHAR 			0
#define MEMORY_REFERENCE 	1
#define SYNTAX 			2
#define NOT_SYNTAX 		3
#define SET			4
#define WORD_BOUNDARY		5
#define NOT_WORD_BOUNDARY	6
#define BEGINNING_OF_BUFFER	7
#define END_OF_BUFFER		8

static unsigned char *reop_casefold;

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
	unsigned char *string;
	int fastmaplen, stringlen;
	int can_be_null, anchor, i;
	int flags, pos, result;
	struct re_pattern_buffer bufp;
	struct re_registers re_regs;
	PyObject *modules = NULL;
	PyObject *reopmodule = NULL;
	PyObject *reopdict = NULL;
	PyObject *casefold = NULL;
	
	if (!PyArg_Parse(args, "(s#iiis#is#i)", 
			 &(bufp.buffer), &(bufp.allocated), 
			 &(bufp.num_registers), &flags, &can_be_null,
			 &(bufp.fastmap), &fastmaplen,
			 &anchor,
			 &string, &stringlen, 
			 &pos))
	  return NULL;

	/* XXX sanity-check the input data */
	bufp.used=bufp.allocated;
	if (flags & IGNORECASE)
	{
		if ((modules = PyImport_GetModuleDict()) == NULL)
			return NULL;

		if ((reopmodule = PyDict_GetItemString(modules,
						       "reop")) == NULL)
			return NULL;

		if ((reopdict = PyModule_GetDict(reopmodule)) == NULL)
			return NULL;

		if ((casefold = PyDict_GetItemString(reopdict,
						     "casefold")) == NULL)
			return NULL;

		bufp.translate = PyString_AsString(casefold);
	}
	else
		bufp.translate=NULL;
	bufp.fastmap_accurate=1;
	bufp.can_be_null=can_be_null;
	bufp.uses_registers=1;
	bufp.anchor=anchor;
	
	for(i=0; i<bufp.num_registers; i++) {
		re_regs.start[i]=-1;
		re_regs.end[i]=-1;
	}
	
	result = re_match(&bufp, 
			  string, stringlen, pos, 
			  &re_regs);

	if (result < -1) {
		/* Failure like stack overflow */
	        if (!PyErr_Occurred())
	  	        PyErr_SetString(ReopError, "match failure");
		return NULL;
	}
	if (result == -1) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	return makeresult(&re_regs, bufp.num_registers);
}

#if 0
static PyObject *
reop_optimize(self, args)
	PyObject *self;
	PyObject *args;
{
  unsigned char *buffer;
  int buflen;
  struct re_pattern_buffer bufp;

  PyObject *opt_code;
  
  if (!PyArg_Parse(args, "(s#)", &buffer, &buflen)) return NULL;
  /* Create a new string for the optimized code */
  opt_code=PyString_FromStringAndSize(buffer, buflen);
  if (opt_code!=NULL)
    {
      bufp.buffer = PyString_AsString(opt_code);
      bufp.used=bufp.allocated=buflen;
      
    }
  return opt_code;
  
}
#endif

static PyObject *
reop_search(self, args)
	PyObject *self;
	PyObject *args;
{
	unsigned char *string;
	int fastmaplen, stringlen;
	int can_be_null, anchor, i;
	int flags, pos, result;
	struct re_pattern_buffer bufp;
	struct re_registers re_regs;
	PyObject *modules = NULL;
	PyObject *reopmodule = NULL;
	PyObject *reopdict = NULL;
	PyObject *casefold = NULL;
	
	if (!PyArg_Parse(args, "(s#iiis#is#i)", 
			 &(bufp.buffer), &(bufp.allocated), 
			 &(bufp.num_registers), &flags, &can_be_null,
			 &(bufp.fastmap), &fastmaplen,
			 &anchor,
			 &string, &stringlen, 
			 &pos))
	  return NULL;

	/* XXX sanity-check the input data */
	bufp.used=bufp.allocated;
	if (flags & IGNORECASE)
	{
		if ((modules = PyImport_GetModuleDict()) == NULL)
			return NULL;

		if ((reopmodule = PyDict_GetItemString(modules,
						       "reop")) == NULL)
			return NULL;

		if ((reopdict = PyModule_GetDict(reopmodule)) == NULL)
			return NULL;

		if ((casefold = PyDict_GetItemString(reopdict,
						     "casefold")) == NULL)
			return NULL;

		bufp.translate = PyString_AsString(casefold);
	}
	else
		bufp.translate=NULL;
	bufp.fastmap_accurate=1;
	bufp.can_be_null=can_be_null;
	bufp.uses_registers=1;
	bufp.anchor=anchor;

	for(i = 0; i < bufp.num_registers; i++) {
		re_regs.start[i] = -1;
		re_regs.end[i] = -1;
	}
	
	result = re_search(&bufp, 
			   string, stringlen, pos, stringlen-pos,
			   &re_regs);

	if (result < -1) {
		/* Failure like stack overflow */
	        if (!PyErr_Occurred())
	  	        PyErr_SetString(ReopError, "match failure");
		return NULL;
	}

	if (result == -1) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	return makeresult(&re_regs, bufp.num_registers);
}

static PyObject *
reop_expand_escape(self, args)
	PyObject *self;
	PyObject *args;
{
  unsigned char c, *pattern;
  int index, context=NORMAL, pattern_len;

  if (!PyArg_ParseTuple(args, "s#i|i", &pattern, &pattern_len, &index,
			&context)) 
    return NULL;
  if (pattern_len<=index)
    {
      PyErr_SetString(ReopError, "escape ends too soon");
      return NULL;
    }
  c=pattern[index]; index++;
  switch (c)
    {
    case('t'):
      return Py_BuildValue("ici", CHAR, (char)9, index);
      break;
    case('n'):
      return Py_BuildValue("ici", CHAR, (char)10, index);
      break;
    case('v'):
      return Py_BuildValue("ici", CHAR, (char)11, index);
      break;
    case('r'):
      return Py_BuildValue("ici", CHAR, (char)13, index);
      break;
    case('f'):
      return Py_BuildValue("ici", CHAR, (char)12, index);
      break;
    case('a'):
      return Py_BuildValue("ici", CHAR, (char)7, index);
      break;
    case('x'):
      {
	int end, length;
	unsigned char *string;
	PyObject *v, *result;

	end=index; 
	while (end<pattern_len && 
	       ( re_syntax_table[ pattern[end] ] & Shexdigit ) )
	  end++;
	if (end==index)
	  {
	    PyErr_SetString(ReopError, "\\x must be followed by hex digits");
	    return NULL;
	  }
	length=end-index;
	string=malloc(length+4+1);
	if (string==NULL)
	  {
	    PyErr_SetString(PyExc_MemoryError, "can't allocate memory for \\x string");
	    return NULL;
	  }
	/* Create a string containing "\x<hexdigits>", which will be
	   passed to eval() */
	string[0]=string[length+3]='"';
	string[1]='\\';
	string[length+4]='\0';
	memcpy(string+2, pattern+index-1, length+1);
	v=PyRun_String(string, Py_eval_input, 
		       PyEval_GetGlobals(), PyEval_GetLocals());
	free(string);
	/* The evaluation raised an exception */
	if (v==NULL) return NULL;
	result=Py_BuildValue("iOi", CHAR, v, end);
	Py_DECREF(v);
	return result;
      }
      break;

    case('b'):
      if (context!=NORMAL)
	return Py_BuildValue("ici", CHAR, (char)8, index);
      else 
	{
	  unsigned char empty_string[1];
	  empty_string[0]='\0';
	  return Py_BuildValue("isi", WORD_BOUNDARY, empty_string, index);
	}
      break;
    case('B'):
      if (context!=NORMAL)
	return Py_BuildValue("ici", CHAR, 'B', index);
      else 
	{
	  unsigned char empty_string[1];
	  empty_string[0]='\0';
	  return Py_BuildValue("isi", NOT_WORD_BOUNDARY, empty_string, index);
	}
      break;
    case('A'):
      if (context!=NORMAL)
	return Py_BuildValue("ici", CHAR, 'A', index);
      else 
	{
	  unsigned char empty_string[1];
	  empty_string[0]='\0';
	  return Py_BuildValue("isi", BEGINNING_OF_BUFFER, empty_string, index);
	}
      break;
    case('Z'):
      if (context!=NORMAL)
	return Py_BuildValue("ici", CHAR, 'Z', index);
      else 
	{
	  unsigned char empty_string[1];
	  empty_string[0]='\0';
	  return Py_BuildValue("isi", END_OF_BUFFER, empty_string, index);
	}
      break;
    case('E'):    case('G'):    case('L'):    case('Q'):
    case('U'):    case('l'):    case('u'):
      {
	char message[50];
	sprintf(message, "\\%c is not allowed", c);
	PyErr_SetString(ReopError, message);
	return NULL;
      }

    case ('w'):
      if (context==NORMAL)
	return Py_BuildValue("iii", SYNTAX, Sword, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 'w', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if (re_syntax_table[i] & Sword) 
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;
    case ('W'):
      if (context==NORMAL)
	return Py_BuildValue("iii", NOT_SYNTAX, Sword, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 'W', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if (! (re_syntax_table[i] & Sword))
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;
    case ('s'):
      if (context==NORMAL)
	return Py_BuildValue("iii", SYNTAX, Swhitespace, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 's', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if (re_syntax_table[i] & Swhitespace) 
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;
    case ('S'):
      if (context==NORMAL)
	return Py_BuildValue("iii", NOT_SYNTAX, Swhitespace, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 'S', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if (! (re_syntax_table[i] & Swhitespace) )
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;

    case ('d'):
      if (context==NORMAL)
	return Py_BuildValue("iii", SYNTAX, Sdigit, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 'd', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if (re_syntax_table[i] & Sdigit) 
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;
    case ('D'):
      if (context==NORMAL)
	return Py_BuildValue("iii", NOT_SYNTAX, Sdigit, index);
      if (context!=CHARCLASS)
	return Py_BuildValue("ici", CHAR, 'D', index);
      {
	/* context==CHARCLASS */
	unsigned char set[256];
	int i, j;
	for(i=j=0; i<256; i++)
	  if ( !(re_syntax_table[i] & Sdigit) )
	    {
	      set[j++] = i;
	    }
	return Py_BuildValue("is#i", SET, set, j, index);
      }
      break;

    case('g'):
      {
	int end, valid, i;
	if (context!=REPLACEMENT)
	  return Py_BuildValue("ici", CHAR, 'g', index);
	if (pattern_len<=index)
	  {
	    PyErr_SetString(ReopError, "unfinished symbolic reference");
	    return NULL;
	  }
	if (pattern[index]!='<')
	  {
	    PyErr_SetString(ReopError, "missing < in symbolic reference");
	    return NULL;
	  }
	index++;
	end=index;
	while (end<pattern_len && pattern[end]!='>')
	  end++;
	if (end==pattern_len)
	  {
	    PyErr_SetString(ReopError, "unfinished symbolic reference");
	    return NULL;
	  }
	valid=1;
	if (index==end		/* Zero-length name */
	    || !(re_syntax_table[pattern[index]] & Sword) /* First char. not alphanumeric */
	    || (re_syntax_table[pattern[index]] & Sdigit) ) /* First char. a digit */
	  valid=0;

	for(i=index+1; i<end; i++)
	  {
	    if (!(re_syntax_table[pattern[i]] & Sword) )
	      valid=0;
	  }	
	if (!valid)
	  {
	    /* XXX should include the text of the reference */
	    PyErr_SetString(ReopError, "illegal symbolic reference");
	    return NULL;
	  }
	    
	return Py_BuildValue("is#i", MEMORY_REFERENCE, 
			             pattern+index, end-index, 
			             end+1);
      }
    break;

    case('0'):
      {
	/* \0 always indicates an octal escape, so we consume up to 3
	   characters, as long as they're all octal digits */
	int octval=0, i;
	index--;
	for(i=index;
	    i<=index+2 && i<pattern_len 
	      && (re_syntax_table[ pattern[i] ] & Soctaldigit );
	    i++)
	  {
	    octval = octval * 8 + pattern[i] - '0';
	  }
	if (octval>255)
	  {
	    PyErr_SetString(ReopError, "octal value out of range");
	    return NULL;
	  }
	return Py_BuildValue("ici", CHAR, (unsigned char)octval, i);
      }
      break;
    case('1'):    case('2'):    case('3'):    case('4'):
    case('5'):    case('6'):    case('7'):    case('8'):
    case('9'):
      {
	/* Handle \?, where ? is from 1 through 9 */
	int value=0;
	index--;
	/* If it's at least a two-digit reference, like \34, it might
           either be a 3-digit octal escape (\123) or a 2-digit
           decimal memory reference (\34) */

	if ( (index+1) <pattern_len && 
	    (re_syntax_table[ pattern[index+1] ] & Sdigit) )
	  {
	    if ( (index+2) <pattern_len && 
		(re_syntax_table[ pattern[index+2] ] & Soctaldigit) &&
		(re_syntax_table[ pattern[index+1] ] & Soctaldigit) &&
		(re_syntax_table[ pattern[index  ] ] & Soctaldigit)
		)
	      {
		/* 3 octal digits */
		value= 8*8*(pattern[index  ]-'0') +
		         8*(pattern[index+1]-'0') +
		           (pattern[index+2]-'0');
		if (value>255)
		  {
		    PyErr_SetString(ReopError, "octal value out of range");
		    return NULL;
		  }
		return Py_BuildValue("ici", CHAR, (unsigned char)value, index+3);
	      }
	    else
	      {
		/* 2-digit form, so it's a memory reference */
		if (context==CHARCLASS)
		  {
		    PyErr_SetString(ReopError, "cannot reference a register "
				    "from inside a character class");
		    return NULL;
		  }
		value= 10*(pattern[index  ]-'0') +
		          (pattern[index+1]-'0');
		if (value<1 || RE_NREGS<=value)
		  {
		    PyErr_SetString(ReopError, "memory reference out of range");
		    return NULL;
		  }
		return Py_BuildValue("iii", MEMORY_REFERENCE, 
				     value, index+2);
	      }
	  }
	else 
	  {
	    /* Single-digit form, like \2, so it's a memory reference */
	    if (context==CHARCLASS)
	      {
		PyErr_SetString(ReopError, "cannot reference a register "
				"from inside a character class");
		return NULL;
	      }
	    return Py_BuildValue("iii", MEMORY_REFERENCE, 
				 pattern[index]-'0', index+1);
	  }
      }
      break;

    default:
	return Py_BuildValue("ici", CHAR, c, index);
	break;
    }
}

static PyObject *
reop__expand(self, args)
	PyObject *self;
	PyObject *args;
{
  PyObject *results, *match_obj;
  PyObject *repl_obj, *newstring;
  unsigned char *repl;
  int size, total_len, i, start, pos;

  if (!PyArg_ParseTuple(args, "OS", &match_obj, &repl_obj)) 
    return NULL;

  repl=PyString_AsString(repl_obj);
  size=PyString_Size(repl_obj);
  results=PyList_New(0);
  if (results==NULL) return NULL;
  for(start=total_len=i=0; i<size; i++)
    {
      if (repl[i]=='\\')
	{
	  PyObject *args, *t, *value;
	  int escape_type;

	  if (start!=i)
	    {
	      PyList_Append(results, 
			    PyString_FromStringAndSize(repl+start, i-start));
	      total_len += i-start;
	    }
	  i++;
	  args=Py_BuildValue("Oii", repl_obj, i, REPLACEMENT);
	  t=reop_expand_escape(NULL, args);
	  Py_DECREF(args);
	  if (t==NULL)
	    {
	      /* reop_expand_escape triggered an exception of some sort,
		 so just return */
	      Py_DECREF(results);
	      return NULL;
	    }
	  value=PyTuple_GetItem(t, 1);
	  escape_type=PyInt_AsLong(PyTuple_GetItem(t, 0));
	  switch (escape_type)
	    {
	    case (CHAR):
	      PyList_Append(results, value);
	      total_len += PyString_Size(value);
	      break;
	    case(MEMORY_REFERENCE):
	      {
		PyObject *r, *tuple, *result;
		r=PyObject_GetAttrString(match_obj, "group");
		tuple=PyTuple_New(1);
		Py_INCREF(value);
		PyTuple_SetItem(tuple, 0, value);
		result=PyEval_CallObject(r, tuple);
		Py_DECREF(r); Py_DECREF(tuple);
		if (result==NULL)
		  {
		    /* The group() method trigged an exception of some sort */
		    Py_DECREF(results);
		    return NULL;
		  }
		if (result==Py_None)
		  {
		    char message[50];
		    sprintf(message, 
			    "group %li did not contribute to the match",
			    PyInt_AsLong(value));
		    PyErr_SetString(ReopError, 
				    message);
		    Py_DECREF(result);
		    Py_DECREF(t);
		    Py_DECREF(results);
		    return NULL;
		  }
		/* xxx typecheck that it's a string! */
		PyList_Append(results, result);
		total_len += PyString_Size(result);
		Py_DECREF(result);
	      }
	      break;
	    default:
	      Py_DECREF(t);
	      Py_DECREF(results);
	      PyErr_SetString(ReopError, 
			      "bad escape in replacement");
	      return NULL;
	    }
	  i=start=PyInt_AsLong(PyTuple_GetItem(t, 2));
	  i--; /* Decrement now, because the 'for' loop will increment it */
	  Py_DECREF(t);
	}
    } /* endif repl[i]!='\\' */

  if (start!=i)
    {
      PyList_Append(results, PyString_FromStringAndSize(repl+start, i-start));
      total_len += i-start;
    }

  /* Whew!  Now we've constructed a list containing various pieces of
     strings that will make up our final result.  So, iterate over 
     the list concatenating them.  A new string measuring total_len
     bytes is allocated and filled in. */
     
  newstring=PyString_FromStringAndSize(NULL, total_len);
  if (newstring==NULL)
    {
      Py_DECREF(results);
      return NULL;
    }

  repl=PyString_AsString(newstring);
  for (pos=i=0; i<PyList_Size(results); i++)
    {
      PyObject *item=PyList_GetItem(results, i);
      memcpy(repl+pos, PyString_AsString(item), PyString_Size(item) );
      pos += PyString_Size(item);
    }
  Py_DECREF(results);
  return newstring;
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
  unsigned char *start;

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
	{"expand_escape", reop_expand_escape, 1},
	{"_expand", reop__expand, 1},
#if 0
	{"_optimize",	reop_optimize, 0},
	{"split",  reop_split, 0},
	{"splitx",  reop_splitx, 0},
#endif
	{NULL,		NULL}		     /* sentinel */
};

void
initreop()
{
	PyObject *m, *d, *k, *v, *o;
	int i;
	unsigned char *s;
	unsigned char j[2];

	re_compile_initialize();

	m = Py_InitModule("reop", reop_global_methods);
	d = PyModule_GetDict(m);
	
	/* Initialize reop.error exception */
	v = ReopError = PyString_FromString("reop.error");
	if (v == NULL || PyDict_SetItemString(d, "error", v) != 0)
		goto finally;
	
	/* Initialize reop.casefold constant */
	if (!(v = PyString_FromStringAndSize((unsigned char *)NULL, 256)))
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

	/* Initialize the syntax table */

	o = PyDict_New();
	if (o == NULL)
	   goto finally;

	j[1] = '\0';
	for (i = 0; i < 256; i++)
	{
	   j[0] = i;
	   k = PyString_FromStringAndSize(j, 1);
	   if (k == NULL)
	      goto finally;
	   v = PyInt_FromLong(re_syntax_table[i]);
	   if (v == NULL)
	      goto finally;
	   if (PyDict_SetItem(o, k, v) < 0)
	      goto finally;
	   Py_DECREF(k);
	   Py_DECREF(v);
	}

	if (PyDict_SetItemString(d, "syntax_table", o) < 0)
	   goto finally;
	Py_DECREF(o);

	v = PyInt_FromLong(Sword);
	if (v == NULL)
	   goto finally;

	if (PyDict_SetItemString(d, "word", v) < 0)
	   goto finally;
	Py_DECREF(v);

	v = PyInt_FromLong(Swhitespace);
	if (v == NULL)
	   goto finally;

	if (PyDict_SetItemString(d, "whitespace", v) < 0)
	   goto finally;
	Py_DECREF(v);

	v = PyInt_FromLong(Sdigit);
	if (v == NULL)
	   goto finally;

	if (PyDict_SetItemString(d, "digit", v) < 0)
	   goto finally;
	Py_DECREF(v);

	PyDict_SetItemString(d, "NORMAL", PyInt_FromLong(NORMAL));
	PyDict_SetItemString(d, "CHARCLASS", PyInt_FromLong(CHARCLASS));
	PyDict_SetItemString(d, "REPLACEMENT", PyInt_FromLong(REPLACEMENT));

	PyDict_SetItemString(d, "CHAR", PyInt_FromLong(CHAR));
	PyDict_SetItemString(d, "MEMORY_REFERENCE", PyInt_FromLong(MEMORY_REFERENCE));
	PyDict_SetItemString(d, "SYNTAX", PyInt_FromLong(SYNTAX));
	PyDict_SetItemString(d, "NOT_SYNTAX", PyInt_FromLong(NOT_SYNTAX));
	PyDict_SetItemString(d, "SET", PyInt_FromLong(SET));
	PyDict_SetItemString(d, "WORD_BOUNDARY", PyInt_FromLong(WORD_BOUNDARY));
	PyDict_SetItemString(d, "NOT_WORD_BOUNDARY", PyInt_FromLong(NOT_WORD_BOUNDARY));
	PyDict_SetItemString(d, "BEGINNING_OF_BUFFER", PyInt_FromLong(BEGINNING_OF_BUFFER));
	PyDict_SetItemString(d, "END_OF_BUFFER", PyInt_FromLong(END_OF_BUFFER));

	if (!PyErr_Occurred())
		return;

  finally:
	Py_FatalError("can't initialize reop module");
}

