#include <Python.h>

static PyObject * hashFunction(PyObject *self, PyObject *args, PyObject *kw)
{
	PyStringObject *a;
	register int len;
	register unsigned char *p;
	register unsigned long x;
	unsigned long ulSeed;
	unsigned long cchSeed;
	unsigned long cHashElements;

	if (!PyArg_ParseTuple(args, "llOl:hash", 
			      &ulSeed, &cchSeed, &a, &cHashElements))
	  return NULL;
	if (!PyString_Check(a))
	{
		PyErr_SetString(PyExc_TypeError, "arg 3 needs to be a string");
		return NULL;
	}
	
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = ulSeed;
	while (--len >= 0)
	{
	    /* (1000003 * x) ^ *p++ 
  	     * translated to handle > 32 bit longs 
	     */
	    x = (0xf4243 * x);
	    x = x & 0xFFFFFFFF;
	    x = x ^ *p++;
	}
	x ^= a->ob_size + cchSeed;
	if (x == 0xFFFFFFFF)
	  x = 0xfffffffe;
	if (x & 0x80000000) 
	{
	      /* Emulate Python 32-bit signed (2's complement) 
	       * modulo operation 
	       */
	      x = (~x & 0xFFFFFFFF) + 1;
	      x %= cHashElements;
	      if (x != 0)
	      {
	          x = x + (~cHashElements & 0xFFFFFFFF) + 1;
	          x = (~x & 0xFFFFFFFF) + 1;
              }
	}
	else
	  x %= cHashElements;
	return PyInt_FromLong((long)x);
}

static PyObject * calcSeed(PyObject *self, PyObject *args, PyObject *kw)
{
	PyStringObject *a;
	register int len;
	register unsigned char *p;
	register unsigned long x;

	if (!PyString_Check(args))
	{
		PyErr_SetString(PyExc_TypeError, "arg 1 expected a string, but didn't get it.");
		return NULL;
	}

	a = (PyStringObject *)args;
	
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = (*p << 7) & 0xFFFFFFFF;
	while (--len >= 0)
	{
	    /* (1000003 * x) ^ *p++ 
  	     * translated to handle > 32 bit longs 
	     */
	    x = (0xf4243 * x);
	    x = x & 0xFFFFFFFF;
	    x = x ^ *p++;
	}
	return PyInt_FromLong((long)x);
}


static struct PyMethodDef hashMethods[] = {
  { "calcSeed", calcSeed, 0, NULL },
  { "hash", hashFunction, 0, NULL },
  { NULL, NULL, 0, NULL } /* sentinel */
};

#ifdef _MSC_VER
_declspec(dllexport)
#endif
void initperfhash(void)
{
        PyObject *m;

        m = Py_InitModule4("perfhash", hashMethods,
                                           NULL, NULL, PYTHON_API_VERSION);
        if ( m == NULL )
            Py_FatalError("can't initialize module perfhash");
}











