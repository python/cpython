#include <Python.h>

static PyObject * hashFunction(PyObject *self, PyObject *args, PyObject *kw)
{
	PyStringObject *a;
	register int len;
	register unsigned char *p;
	register long x;
	long lSeed;
	unsigned long cchSeed;

	if (!PyArg_ParseTuple(args, "iiO:hash", &lSeed, &cchSeed, &a))
	  return NULL;
	if (!PyString_Check(a))
	{
		PyErr_SetString(PyExc_TypeError, "arg 3 needs to be a string");
		return NULL;
	}
	
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = lSeed;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	x ^= a->ob_size + cchSeed;
	if (x == -1)
		x = -2;
	return PyInt_FromLong(x);
}

static PyObject * calcSeed(PyObject *self, PyObject *args, PyObject *kw)
{
	PyStringObject *a;
	register int len;
	register unsigned char *p;
	register long x;

	if (!PyString_Check(args))
	{
		PyErr_SetString(PyExc_TypeError, "arg 1 expected a string, but didn't get it.");
		return NULL;
	}

	a = (PyStringObject *)args;
	
	len = a->ob_size;
	p = (unsigned char *) a->ob_sval;
	x = *p << 7;
	while (--len >= 0)
		x = (1000003*x) ^ *p++;
	return PyInt_FromLong(x);
}


static struct PyMethodDef hashMethods[] = {
  { "calcSeed", calcSeed, 0, NULL },
  { "hash", hashFunction, 0, NULL },
  { NULL, NULL, 0, NULL } /* sentinel */
};

#ifdef _MSC_VER
_declspec(dllexport)
#endif
void initperfhash()
{
        PyObject *m;

        m = Py_InitModule4("perfhash", hashMethods,
                                           NULL, NULL, PYTHON_API_VERSION);
        if ( m == NULL )
            Py_FatalError("can't initialize module hashModule");
}
