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

/* Pcre objects */

#include "Python.h"

#ifndef Py_eval_input
/* For Python 1.4, graminit.h has to be explicitly included */
#include "graminit.h"
#define Py_eval_input eval_input
#endif

#ifndef FOR_PYTHON
#define FOR_PYTHON
#endif

#include "pcre.h"
#include "pcre-internal.h"

static PyObject *ErrorObject;

typedef struct {
	PyObject_HEAD
	pcre *regex;
	pcre_extra *regex_extra;
        int num_groups;
} PcreObject;

staticforward PyTypeObject Pcre_Type;

#define PcreObject_Check(v)	((v)->ob_type == &Pcre_Type)
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


static PcreObject *
newPcreObject(arg)
	PyObject *arg;
{
	PcreObject *self;
	self = PyObject_NEW(PcreObject, &Pcre_Type);
	if (self == NULL)
		return NULL;
	self->regex = NULL;
	self->regex_extra = NULL;
	return self;
}

/* Pcre methods */

static void
PyPcre_dealloc(self)
	PcreObject *self;
{
	if (self->regex) free(self->regex);
	if (self->regex_extra) free(self->regex_extra);
	self->regex=NULL;
	self->regex_extra=NULL;
	PyMem_DEL(self);
}


static PyObject *
PyPcre_exec(self, args)
	PcreObject *self;
	PyObject *args;
{
        unsigned char *string;
	int stringlen, pos=0, options=0, i, count;
	int offsets[100*2]; /* XXX must this be fixed? */
	PyObject *list;

	if (!PyArg_ParseTuple(args, "s#|ii", &string, &stringlen, &pos, &options))
		return NULL;
	count = pcre_exec(self->regex, self->regex_extra, 
			  string+pos, stringlen-pos, options,
			  offsets, sizeof(offsets)/sizeof(int) );
	if (count==PCRE_ERROR_NOMATCH) {Py_INCREF(Py_None); return Py_None;}
	if (count<0)
	  {
	    PyErr_SetObject(ErrorObject, Py_BuildValue("si", "Regex error", count));
	    return NULL;
	  }
	
	list=PyList_New(self->num_groups+1);
	if (list==NULL) return NULL;
	/* XXX count can be >size_offset! */
	for(i=0; i<=self->num_groups; i++)
	  {
	    PyObject *v;
	    int start=offsets[i*2], end=offsets[i*2+1];
	    /* If the group wasn't affected by the match, return -1, -1 */
            if (start<0 || count<=i) 
	      {start=end=-1;}
	    else 
	      {start += pos; end +=pos;}
	    v=Py_BuildValue("ii", start, end);
	    if (v==NULL) {Py_DECREF(list); return NULL;}
	    PyList_SetItem(list, i, v);
	  }
	return list;
}

static PyMethodDef Pcre_methods[] = {
	{"match",	(PyCFunction)PyPcre_exec,	1},
	{NULL,		NULL}		/* sentinel */
};

static PyObject *
PyPcre_getattr(self, name)
	PcreObject *self;
	char *name;
{
	return Py_FindMethod(Pcre_methods, (PyObject *)self, name);
}


staticforward PyTypeObject Pcre_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,			/*ob_size*/
	"Pcre",			/*tp_name*/
	sizeof(PcreObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)PyPcre_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)PyPcre_getattr, /*tp_getattr*/
	0,                      /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
};
/* --------------------------------------------------------------------- */

static PyObject *
PyPcre_compile(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	PcreObject *rv;
	PyObject *dictionary;
	unsigned char *pattern, *newpattern;
	char *error;
	int num_zeros, i, j;
	
	int patternlen, options, erroroffset;
	if (!PyArg_ParseTuple(args, "s#iO!", &pattern, &patternlen, &options,
			      &PyDict_Type, &dictionary))
		return NULL;
	rv = newPcreObject(args);
	if ( rv == NULL )
	    return NULL;

	/* PCRE doesn't like having null bytes in its pattern, so we have to replace 
	   any zeros in the string with the characters '\0'. */
	num_zeros=1;
	for(i=0; i<patternlen; i++) {
	  if (pattern[i]==0) num_zeros++;
	}
	newpattern=malloc(patternlen+num_zeros);
	if (newpattern==NULL) {
	  PyErr_SetString(PyExc_MemoryError, "can't allocate memory for new pattern");
	  return NULL;
	}
	for (i=j=0; i<patternlen; i++, j++)
	  {
	    if (pattern[i]!=0) newpattern[j]=pattern[i];
	    else {
	      newpattern[j++]='\\';
	      newpattern[j]  ='0';
	    }
	  }
	newpattern[j]='\0';

	rv->regex = pcre_compile(newpattern, options, 
				 &error, &erroroffset, dictionary);
	free(newpattern);
	if (rv->regex==NULL) 
	  {
	    PyMem_DEL(rv);
	    if (!PyErr_Occurred())
	      {
		PyErr_SetObject(ErrorObject, Py_BuildValue("si", error, erroroffset));
	      }
	    return NULL;
	  }
	rv->regex_extra=pcre_study(rv->regex, 0, &error);
	if (rv->regex_extra==NULL && error!=NULL) 
	  {
	    PyMem_DEL(rv);
	    PyErr_SetObject(ErrorObject, Py_BuildValue("si", error, 0));
	    return NULL;
	  }
        rv->num_groups = pcre_info(rv->regex, NULL, NULL);
	if (rv->num_groups<0) 
	  {
	    PyErr_SetObject(ErrorObject, Py_BuildValue("si", "Regex error", rv->num_groups));
	    PyMem_DEL(rv);
	    return NULL;
	  }
	return (PyObject *)rv;
}

static PyObject *
PyPcre_expand_escape(pattern, pattern_len, indexptr, typeptr)
     unsigned char *pattern;
     int pattern_len, *indexptr, *typeptr;
{
  unsigned char c;
  int index = *indexptr;
  
  if (pattern_len<=index)
    {
      PyErr_SetString(ErrorObject, "escape ends too soon");
      return NULL;
    }
  c=pattern[index]; index++;
  *typeptr=CHAR;

  switch (c)
    {
    case('t'):
      *indexptr=index;
      return Py_BuildValue("c", (char)9);
      break;
    case('n'):
      *indexptr = index;
      return Py_BuildValue("c", (char)10);
      break;
    case('v'):
      *indexptr = index;
      return Py_BuildValue("c", (char)11);
      break;
    case('r'):
      *indexptr = index;
      return Py_BuildValue("c", (char)13);
      break;
    case('f'):
      *indexptr = index;
      return Py_BuildValue("c", (char)12);
      break;
    case('a'):
      *indexptr = index;
      return Py_BuildValue("c", (char)7);
      break;
    case('b'):
      *indexptr=index;
      return Py_BuildValue("c", (char)8);
      break;

    case('x'):
      {
	int end, length;
	unsigned char *string;
	PyObject *v;

	end=index; 
	while (end<pattern_len && 
	       ( pcre_ctypes[ pattern[end] ] & ctype_xdigit ) )
	  end++;
	if (end==index)
	  {
	    PyErr_SetString(ErrorObject, "\\x must be followed by hex digits");
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
	v=PyRun_String((char *)string, Py_eval_input, 
		       PyEval_GetGlobals(), PyEval_GetLocals());
	free(string);
	/* The evaluation raised an exception */
	if (v==NULL) return NULL;
	*indexptr = end;
	return v;
      }
      break;

    case('E'):    case('G'):    case('L'):    case('Q'):
    case('U'):    case('l'):    case('u'):
      {
	char message[50];
	sprintf(message, "\\%c is not allowed", c);
	PyErr_SetString(ErrorObject, message);
	return NULL;
      }

    case('g'):
      {
	int end, valid, i;
	if (pattern_len<=index)
	  {
	    PyErr_SetString(ErrorObject, "unfinished symbolic reference");
	    return NULL;
	  }
	if (pattern[index]!='<')
	  {
	    PyErr_SetString(ErrorObject, "missing < in symbolic reference");
	    return NULL;
	  }
	index++;
	end=index;
	while (end<pattern_len && pattern[end]!='>')
	  end++;
	if (end==pattern_len)
	  {
	    PyErr_SetString(ErrorObject, "unfinished symbolic reference");
	    return NULL;
	  }
	valid=1;
	if (index==end		/* Zero-length name */
	    || !(pcre_ctypes[pattern[index]] & ctype_word) /* First char. not alphanumeric */
	    || (pcre_ctypes[pattern[index]] & ctype_digit) ) /* First char. a digit */
	  valid=0;

	for(i=index+1; i<end; i++)
	  {
	    if (!(pcre_ctypes[pattern[i]] & ctype_word) )
	      valid=0;
	  }	
	if (!valid)
	  {
	    /* XXX should include the text of the reference */
	    PyErr_SetString(ErrorObject, "illegal symbolic reference");
	    return NULL;
	  }
	    
	*typeptr = MEMORY_REFERENCE;
	*indexptr = end+1;
	return Py_BuildValue("s#", pattern+index, end-index);
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
	      && (pcre_ctypes[ pattern[i] ] & ctype_odigit );
	    i++)
	  {
	    octval = octval * 8 + pattern[i] - '0';
	  }
	if (octval>255)
	  {
	    PyErr_SetString(ErrorObject, "octal value out of range");
	    return NULL;
	  }
	*indexptr = i;
	return Py_BuildValue("c", (unsigned char)octval);
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
	    (pcre_ctypes[ pattern[index+1] ] & ctype_digit) )
	  {
	    if ( (index+2) <pattern_len && 
		(pcre_ctypes[ pattern[index+2] ] & ctype_odigit) &&
		(pcre_ctypes[ pattern[index+1] ] & ctype_odigit) &&
		(pcre_ctypes[ pattern[index  ] ] & ctype_odigit)
		)
	      {
		/* 3 octal digits */
		value= 8*8*(pattern[index  ]-'0') +
		         8*(pattern[index+1]-'0') +
		           (pattern[index+2]-'0');
		if (value>255)
		  {
		    PyErr_SetString(ErrorObject, "octal value out of range");
		    return NULL;
		  }
		*indexptr = index+3;
		return Py_BuildValue("c", (unsigned char)value);
	      }
	    else
	      {
		/* 2-digit form, so it's a memory reference */
		value= 10*(pattern[index  ]-'0') +
		          (pattern[index+1]-'0');
		if (value<1 || EXTRACT_MAX<=value)
		  {
		    PyErr_SetString(ErrorObject, "memory reference out of range");
		    return NULL;
		  }
		*typeptr = MEMORY_REFERENCE;
		*indexptr = index+2;
		return Py_BuildValue("i", value);
	      }
	  }
	else 
	  {
	    /* Single-digit form, like \2, so it's a memory reference */
	    *typeptr = MEMORY_REFERENCE;
	    *indexptr = index+1;
	    return Py_BuildValue("i", pattern[index]-'0');
	  }
      }
      break;

    default:
	*indexptr = index;
	return Py_BuildValue("c", c);
	break;
    }
}

static PyObject *
PyPcre_expand(self, args)
	PyObject *self;
	PyObject *args;
{
  PyObject *results, *match_obj;
  PyObject *repl_obj, *newstring;
  unsigned char *repl;
  int size, total_len, i, start, pos;

  if (!PyArg_ParseTuple(args, "OS", &match_obj, &repl_obj)) 
    return NULL;

  repl=(unsigned char *)PyString_AsString(repl_obj);
  size=PyString_Size(repl_obj);
  results=PyList_New(0);
  if (results==NULL) return NULL;
  for(start=total_len=i=0; i<size; i++)
    {
      if (repl[i]=='\\')
	{
	  PyObject *value;
	  int escape_type;

	  if (start!=i)
	    {
	      PyList_Append(results, 
			    PyString_FromStringAndSize((char *)repl+start, i-start));
	      total_len += i-start;
	    }
	  i++;
	  value=PyPcre_expand_escape(repl, size, &i, &escape_type);
	  if (value==NULL)
	    {
	      /* PyPcre_expand_escape triggered an exception of some sort,
		 so just return */
	      Py_DECREF(results);
	      return NULL;
	    }
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
		    PyErr_SetString(ErrorObject, 
				    message);
		    Py_DECREF(result);
		    Py_DECREF(results);
		    return NULL;
		  }
		/* XXX typecheck that it's a string! */
		PyList_Append(results, result);
		total_len += PyString_Size(result);
		Py_DECREF(result);
	      }
	      break;
	    default:
	      Py_DECREF(results);
	      PyErr_SetString(ErrorObject, 
			      "bad escape in replacement");
	      return NULL;
	    }
	  start=i;
	  i--; /* Decrement now, because the 'for' loop will increment it */
	}
    } /* endif repl[i]!='\\' */

  if (start!=i)
    {
      PyList_Append(results, PyString_FromStringAndSize((char *)repl+start, i-start));
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

  repl=(unsigned char *)PyString_AsString(newstring);
  for (pos=i=0; i<PyList_Size(results); i++)
    {
      PyObject *item=PyList_GetItem(results, i);
      memcpy(repl+pos, PyString_AsString(item), PyString_Size(item) );
      pos += PyString_Size(item);
    }
  Py_DECREF(results);
  return newstring;
}


/* List of functions defined in the module */

static PyMethodDef pcre_methods[] = {
	{"pcre_compile",		PyPcre_compile,		1},
	{"pcre_expand",		PyPcre_expand,		1},
	{NULL,		NULL}		/* sentinel */
};


/*
 * Convenience routine to export an integer value.
 * For simplicity, errors (which are unlikely anyway) are ignored.
 */

static void
insint(d, name, value)
	PyObject * d;
	char * name;
	int value;
{
	PyObject *v = PyInt_FromLong((long) value);
	if (v == NULL) {
		/* Don't bother reporting this error */
		PyErr_Clear();
	}
	else {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}


/* Initialization function for the module (*must* be called initpcre) */

void
initpcre()
{
	PyObject *m, *d;

	/* Create the module and add the functions */
	m = Py_InitModule("pcre", pcre_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyString_FromString("pcre.error");
	PyDict_SetItemString(d, "error", ErrorObject);

	/* Insert the flags */
	insint(d, "IGNORECASE", PCRE_CASELESS);
	insint(d, "ANCHORED", PCRE_ANCHORED);
	insint(d, "MULTILINE", PCRE_MULTILINE);
	insint(d, "DOTALL", PCRE_DOTALL);
	insint(d, "VERBOSE", PCRE_EXTENDED);
	
	/* Insert the opcodes */
	insint(d, "OP_END", OP_END);
	insint(d, "OP_SOD", OP_SOD);
	insint(d, "OP_NOT_WORD_BOUNDARY", OP_NOT_WORD_BOUNDARY);
	insint(d, "OP_WORD_BOUNDARY", OP_WORD_BOUNDARY);
	insint(d, "OP_NOT_DIGIT", OP_NOT_DIGIT);
	insint(d, "OP_NOT_WHITESPACE", OP_NOT_WHITESPACE);
	insint(d, "OP_WHITESPACE", OP_WHITESPACE);
	insint(d, "OP_NOT_WORDCHAR", OP_NOT_WORDCHAR);
	insint(d, "OP_WORDCHAR", OP_WORDCHAR);
	insint(d, "OP_EOD", OP_EOD);
	insint(d, "OP_CIRC", OP_CIRC);
	insint(d, "OP_DOLL", OP_DOLL);
	insint(d, "OP_ANY", OP_ANY);
	insint(d, "OP_CHARS", OP_CHARS);

	insint(d, "OP_STAR", OP_STAR);
	insint(d, "OP_MINSTAR", OP_MINSTAR);
	insint(d, "OP_PLUS", OP_PLUS);
	insint(d, "OP_MINPLUS", OP_MINPLUS);
	insint(d, "OP_QUERY", OP_QUERY);
	insint(d, "OP_MINQUERY", OP_MINQUERY);
	insint(d, "OP_UPTO", OP_UPTO);
	insint(d, "OP_MINUPTO", OP_MINUPTO);
	insint(d, "OP_EXACT", OP_EXACT);

	insint(d, "OP_TYPESTAR", OP_TYPESTAR);
	insint(d, "OP_TYPEMINSTAR", OP_TYPEMINSTAR);
	insint(d, "OP_TYPEPLUS", OP_TYPEPLUS);
	insint(d, "OP_TYPEMINPLUS", OP_TYPEMINPLUS);
	insint(d, "OP_TYPEQUERY", OP_TYPEQUERY);
	insint(d, "OP_TYPEMINQUERY", OP_TYPEMINQUERY);
	insint(d, "OP_TYPEUPTO", OP_TYPEUPTO);
	insint(d, "OP_TYPEMINUPTO", OP_TYPEMINUPTO);
	insint(d, "OP_TYPEEXACT", OP_TYPEEXACT);

	insint(d, "OP_CRSTAR", OP_CRSTAR);
	insint(d, "OP_CRMINSTAR", OP_CRMINSTAR);
	insint(d, "OP_CRPLUS", OP_CRPLUS);
	insint(d, "OP_CRMINPLUS", OP_CRMINPLUS);
	insint(d, "OP_CRQUERY", OP_CRQUERY);
	insint(d, "OP_CRMINQUERY", OP_CRMINQUERY);
	insint(d, "OP_CRRANGE", OP_CRRANGE);
	insint(d, "OP_CRMINRANGE", OP_CRMINRANGE);

	insint(d, "OP_CLASS", OP_CLASS);
	insint(d, "OP_NEGCLASS", OP_NEGCLASS);
	insint(d, "OP_REF", OP_REF);

	insint(d, "OP_ALT", OP_ALT);
	insint(d, "OP_KET", OP_KET);
	insint(d, "OP_KETRMAX", OP_KETRMAX);
	insint(d, "OP_KETRMIN", OP_KETRMIN);

	insint(d, "OP_ASSERT", OP_ASSERT);
	insint(d, "OP_ASSERT_NOT", OP_ASSERT_NOT);

	insint(d, "OP_BRAZERO", OP_BRAZERO);
	insint(d, "OP_BRAMINZERO", OP_BRAMINZERO);
	insint(d, "OP_BRA", OP_BRA);
	
	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module pcre");
}

