/* Pcre objects */

#include "Python.h"

#include <assert.h>
#ifndef Py_eval_input
/* For Python 1.4, graminit.h has to be explicitly included */
#include "graminit.h"
#define Py_eval_input eval_input
#endif

#ifndef FOR_PYTHON
#define FOR_PYTHON
#endif

#include "pcre.h"
#include "pcre-int.h"

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
#define STRING                  9

static PcreObject *
newPcreObject(PyObject *args)
{
	PcreObject *self;
	self = PyObject_New(PcreObject, &Pcre_Type);
	if (self == NULL)
		return NULL;
	self->regex = NULL;
	self->regex_extra = NULL;
	return self;
}

/* Pcre methods */

static void
PyPcre_dealloc(PcreObject *self)
{
	if (self->regex) (pcre_free)(self->regex);
	if (self->regex_extra) (pcre_free)(self->regex_extra);
	PyObject_Del(self);
}


static PyObject *
PyPcre_exec(PcreObject *self, PyObject *args)
{
        char *string;
	int stringlen, pos = 0, options=0, endpos = -1, i, count;
	int offsets[100*2]; 
	PyObject *list;

	if (!PyArg_ParseTuple(args, "t#|iiii:match", &string, &stringlen, &pos, &endpos, &options))
		return NULL;
	if (endpos == -1) {endpos = stringlen;}
	count = pcre_exec(self->regex, self->regex_extra, 
			  string, endpos, pos, options,
			  offsets, sizeof(offsets)/sizeof(int) );
	/* If an error occurred during the match, and an exception was raised,
	   just return NULL and leave the exception alone.  The most likely
	   problem to cause this would be running out of memory for
	   the failure stack. */
	if (PyErr_Occurred())
	{
		return NULL;
	}
	if (count==PCRE_ERROR_NOMATCH) {Py_INCREF(Py_None); return Py_None;}
	if (count<0)
	{
		PyObject *errval = Py_BuildValue("si", "Regex execution error", count);
		PyErr_SetObject(ErrorObject, errval);
		Py_XDECREF(errval);
		return NULL;
	}
	
	list=PyList_New(self->num_groups+1);
	if (list==NULL) return NULL;
	for(i=0; i<=self->num_groups; i++)
	{
		PyObject *v;
		int start=offsets[i*2], end=offsets[i*2+1];
		/* If the group wasn't affected by the match, return -1, -1 */
		if (start<0 || count<=i) 
		{start=end=-1;}
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
PyPcre_getattr(PcreObject *self, char *name)
{
	return Py_FindMethod(Pcre_methods, (PyObject *)self, name);
}


staticforward PyTypeObject Pcre_Type = {
	PyObject_HEAD_INIT(NULL)
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
PyPcre_compile(PyObject *self, PyObject *args)
{
	PcreObject *rv;
	PyObject *dictionary;
	char *pattern;
	const char *error;
	
	int options, erroroffset;
	if (!PyArg_ParseTuple(args, "siO!:pcre_compile", &pattern, &options,
			      &PyDict_Type, &dictionary))
		return NULL;
	rv = newPcreObject(args);
	if ( rv == NULL )
		return NULL;

	rv->regex = pcre_compile((char*)pattern, options, 
				 &error, &erroroffset, dictionary);
	if (rv->regex==NULL) 
	{
		Py_DECREF(rv);
		if (!PyErr_Occurred())
		{
			PyObject *errval = Py_BuildValue("si", error, erroroffset);
			PyErr_SetObject(ErrorObject, errval);
			Py_XDECREF(errval);
		}
		return NULL;
	}
	rv->regex_extra=pcre_study(rv->regex, 0, &error);
	if (rv->regex_extra==NULL && error!=NULL) 
	{
		PyObject *errval = Py_BuildValue("si", error, 0);
		Py_DECREF(rv);
		PyErr_SetObject(ErrorObject, errval);
		Py_XDECREF(errval);
		return NULL;
	}
        rv->num_groups = pcre_info(rv->regex, NULL, NULL);
	if (rv->num_groups<0) 
	{
		PyObject *errval = Py_BuildValue("si", error, rv->num_groups);
		PyErr_SetObject(ErrorObject, errval);
		Py_XDECREF(errval);
		Py_DECREF(rv);
		return NULL;
	}
	return (PyObject *)rv;
}

static PyObject *
PyPcre_expand_escape(unsigned char *pattern, int pattern_len,
                     int *indexptr, int *typeptr)
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
	case('n'):
		*indexptr = index;
		return Py_BuildValue("c", (char)10);
	case('v'):
		*indexptr = index;
		return Py_BuildValue("c", (char)11);
	case('r'):
		*indexptr = index;
		return Py_BuildValue("c", (char)13);
	case('f'):
		*indexptr = index;
		return Py_BuildValue("c", (char)12);
	case('a'):
		*indexptr = index;
		return Py_BuildValue("c", (char)7);
	case('b'):
		*indexptr=index;
		return Py_BuildValue("c", (char)8);
	case('\\'):
		*indexptr=index;
		return Py_BuildValue("c", '\\');

	case('x'):
	{
		int x, ch, end;

		x = 0; end = index;
		while ( (end<pattern_len && pcre_ctypes[ pattern[end] ] & ctype_xdigit) != 0)
		{
			ch = pattern[end];
			x = x * 16 + pcre_lcc[ch] -
				(((pcre_ctypes[ch] & ctype_digit) != 0)? '0' : 'W');
			x &= 255;
			end++;
		}
		if (end==index)
		{
			PyErr_SetString(ErrorObject, "\\x must be followed by hex digits");
			return NULL;
		}
		*indexptr = end;
		return Py_BuildValue("c", (char)x);
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
		int end, i;
		int group_num = 0, is_number=0;

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

		if (index==end)		/* Zero-length name */
		{
			/* XXX should include the text of the reference */
			PyErr_SetString(ErrorObject, "zero-length symbolic reference");
			return NULL;
		}
		if ((pcre_ctypes[pattern[index]] & ctype_digit)) /* First char. a digit */
		{
		        is_number = 1;
			group_num = pattern[index] - '0';
		}

		for(i=index+1; i<end; i++)
		{
		        if (is_number && 
			    !(pcre_ctypes[pattern[i]] & ctype_digit) )
			{
				/* XXX should include the text of the reference */
				PyErr_SetString(ErrorObject, "illegal non-digit character in \\g<...> starting with digit");
				return NULL;			       
			}
			else {group_num = group_num * 10 + pattern[i] - '0';}
			if (!(pcre_ctypes[pattern[i]] & ctype_word) )
			{
				/* XXX should include the text of the reference */
				PyErr_SetString(ErrorObject, "illegal symbolic reference");
				return NULL;
			}
		}	
	    
		*typeptr = MEMORY_REFERENCE;
		*indexptr = end+1;
		/* If it's a number, return the integer value of the group */
		if (is_number) return Py_BuildValue("i", group_num);
		/* Otherwise, return a string containing the group name */
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
	  /* It's some unknown escape like \s, so return a string containing
	     \s */
		*typeptr = STRING;
		*indexptr = index;
		return Py_BuildValue("s#", pattern+index-2, 2);
	}
}

static PyObject *
PyPcre_expand(PyObject *self, PyObject *args)
{
	PyObject *results, *match_obj;
	PyObject *repl_obj, *newstring;
	unsigned char *repl;
	int size, total_len, i, start, pos;

	if (!PyArg_ParseTuple(args, "OS:pcre_expand", &match_obj, &repl_obj)) 
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
				int status;
				PyObject *s = PyString_FromStringAndSize(
					(char *)repl+start, i-start);
				if (s == NULL) {
					Py_DECREF(results);
					return NULL;
				}
				status = PyList_Append(results, s);
				Py_DECREF(s);
				if (status < 0) {
					Py_DECREF(results);
					return NULL;
				}
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
				if (r == NULL) {
					Py_DECREF(results);
					return NULL;
				}
				tuple=PyTuple_New(1);
				Py_INCREF(value);
				PyTuple_SetItem(tuple, 0, value);
				result=PyEval_CallObject(r, tuple);
				Py_DECREF(r); Py_DECREF(tuple);
				if (result==NULL)
				{
					/* The group() method triggered an exception of some sort */
					Py_DECREF(results);
					Py_DECREF(value);
					return NULL;
				}
				if (result==Py_None)
				{
					char message[50];
					sprintf(message, 
						"group did not contribute to the match");
					PyErr_SetString(ErrorObject, 
							message);
					Py_DECREF(result);
					Py_DECREF(value);
					Py_DECREF(results);
					return NULL;
				}
				/* typecheck that it's a string! */
				if (!PyString_Check(result))
				{
					Py_DECREF(results);
					Py_DECREF(result);
					PyErr_SetString(ErrorObject, 
							"group() must return a string value for replacement");
					return NULL;
				}
				PyList_Append(results, result);
				total_len += PyString_Size(result);
				Py_DECREF(result);
			}
			break;
			case(STRING):
			  {
			    PyList_Append(results, value);
			    total_len += PyString_Size(value);
			    break;
			  }
			default:
				Py_DECREF(results);
				PyErr_SetString(ErrorObject, 
						"bad escape in replacement");
				return NULL;
			}
			Py_DECREF(value);
			start=i;
			i--; /* Decrement now, because the 'for' loop will increment it */
		}
	} /* endif repl[i]!='\\' */

	if (start!=i)
	{
		int status;
		PyObject *s = PyString_FromStringAndSize((char *)repl+start, 
							 i-start);
		if (s == NULL) {
			Py_DECREF(results);
			return NULL;
		}
		status = PyList_Append(results, s);
		Py_DECREF(s);
		if (status < 0) {
			Py_DECREF(results);
			return NULL;
		}
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
insint(PyObject *d, char *name, int value)
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

DL_EXPORT(void)
initpcre(void)
{
	PyObject *m, *d;

        Pcre_Type.ob_type = &PyType_Type;

	/* Create the module and add the functions */
	m = Py_InitModule("pcre", pcre_methods);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	ErrorObject = PyErr_NewException("pcre.error", NULL, NULL);
	PyDict_SetItemString(d, "error", ErrorObject);

	/* Insert the flags */
	insint(d, "IGNORECASE", PCRE_CASELESS);
	insint(d, "ANCHORED", PCRE_ANCHORED);
	insint(d, "MULTILINE", PCRE_MULTILINE);
	insint(d, "DOTALL", PCRE_DOTALL);
	insint(d, "VERBOSE", PCRE_EXTENDED);
	insint(d, "LOCALE", PCRE_LOCALE);
}

