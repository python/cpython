/* Random objects */

/* ------------------------------------------------------------------
   The code in this module was based on a download from:
	  http://www.math.keio.ac.jp/~matumoto/MT2002/emt19937ar.html

   It was modified in 2002 by Raymond Hettinger as follows:

	* the principal computational lines untouched except for tabbing.

	* renamed genrand_res53() to random_random() and wrapped
	  in python calling/return code.

	* genrand_int32() and the helper functions, init_genrand()
	  and init_by_array(), were declared static, wrapped in
	  Python calling/return code.  also, their global data
	  references were replaced with structure references.

	* unused functions from the original were deleted.
	  new, original C python code was added to implement the
	  Random() interface.

   The following are the verbatim comments from the original code:

   A C-program for MT19937, with initialization improved 2002/1/26.
   Coded by Takuji Nishimura and Makoto Matsumoto.

   Before using, initialize the state by using init_genrand(seed)
   or init_by_array(init_key, key_length).

   Copyright (C) 1997 - 2002, Makoto Matsumoto and Takuji Nishimura,
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

     1. Redistributions of source code must retain the above copyright
	notice, this list of conditions and the following disclaimer.

     2. Redistributions in binary form must reproduce the above copyright
	notice, this list of conditions and the following disclaimer in the
	documentation and/or other materials provided with the distribution.

     3. The names of its contributors may not be used to endorse or promote
	products derived from this software without specific prior written
	permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


   Any feedback is very welcome.
   http://www.math.keio.ac.jp/matumoto/emt.html
   email: matumoto@math.keio.ac.jp
*/

/* ---------------------------------------------------------------*/

#include "Python.h"
#include <time.h>		/* for seeding to current time */

/* Period parameters -- These are all magic.  Don't change. */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL	/* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

typedef struct {
	PyObject_HEAD
	unsigned long state[N];
	int index;
} RandomObject;

static PyTypeObject Random_Type;

#define RandomObject_Check(v)	   ((v)->ob_type == &Random_Type)


/* Random methods */


/* generates a random number on [0,0xffffffff]-interval */
static unsigned long
genrand_int32(RandomObject *self)
{
	unsigned long y;
	static unsigned long mag01[2]={0x0UL, MATRIX_A};
	/* mag01[x] = x * MATRIX_A  for x=0,1 */
	unsigned long *mt;

	mt = self->state;
	if (self->index >= N) { /* generate N words at one time */
		int kk;

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

		self->index = 0;
	}

    y = mt[self->index++];
    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);
    return y;
}

/* random_random is the function named genrand_res53 in the original code;
 * generates a random number on [0,1) with 53-bit resolution; note that
 * 9007199254740992 == 2**53; I assume they're spelling "/2**53" as
 * multiply-by-reciprocal in the (likely vain) hope that the compiler will
 * optimize the division away at compile-time.  67108864 is 2**26.  In
 * effect, a contains 27 random bits shifted left 26, and b fills in the
 * lower 26 bits of the 53-bit numerator.
 * The orginal code credited Isaku Wada for this algorithm, 2002/01/09.
 */
static PyObject *
random_random(RandomObject *self)
{
	unsigned long a=genrand_int32(self)>>5, b=genrand_int32(self)>>6;
    	return PyFloat_FromDouble((a*67108864.0+b)*(1.0/9007199254740992.0));
}

/* initializes mt[N] with a seed */
static void
init_genrand(RandomObject *self, unsigned long s)
{
	int mti;
	unsigned long *mt;

	mt = self->state;
	mt[0]= s & 0xffffffffUL;
	for (mti=1; mti<N; mti++) {
		mt[mti] =
		(1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);
		/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
		/* In the previous versions, MSBs of the seed affect   */
		/* only MSBs of the array mt[]. 		       */
		/* 2002/01/09 modified by Makoto Matsumoto	       */
		mt[mti] &= 0xffffffffUL;
		/* for >32 bit machines */
	}
	self->index = mti;
	return;
}

/* initialize by an array with array-length */
/* init_key is the array for initializing keys */
/* key_length is its length */
static PyObject *
init_by_array(RandomObject *self, unsigned long init_key[], unsigned long key_length)
{
	unsigned int i, j, k;	/* was signed in the original code. RDH 12/16/2002 */
	unsigned long *mt;

	mt = self->state;
	init_genrand(self, 19650218UL);
	i=1; j=0;
	k = (N>key_length ? N : key_length);
	for (; k; k--) {
		mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1664525UL))
			 + init_key[j] + j; /* non linear */
		mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
		i++; j++;
		if (i>=N) { mt[0] = mt[N-1]; i=1; }
		if (j>=key_length) j=0;
	}
	for (k=N-1; k; k--) {
		mt[i] = (mt[i] ^ ((mt[i-1] ^ (mt[i-1] >> 30)) * 1566083941UL))
			 - i; /* non linear */
		mt[i] &= 0xffffffffUL; /* for WORDSIZE > 32 machines */
		i++;
		if (i>=N) { mt[0] = mt[N-1]; i=1; }
	}

    mt[0] = 0x80000000UL; /* MSB is 1; assuring non-zero initial array */
    Py_INCREF(Py_None);
    return Py_None;
}

/*
 * The rest is Python-specific code, neither part of, nor derived from, the
 * Twister download.
 */

static PyObject *
random_seed(RandomObject *self, PyObject *args)
{
	PyObject *result = NULL;	/* guilty until proved innocent */
	PyObject *masklower = NULL;
	PyObject *thirtytwo = NULL;
	PyObject *n = NULL;
	unsigned long *key = NULL;
	unsigned long keymax;		/* # of allocated slots in key */
	unsigned long keyused;		/* # of used slots in key */
	int err;

	PyObject *arg = NULL;

	if (!PyArg_UnpackTuple(args, "seed", 0, 1, &arg))
		return NULL;

	if (arg == NULL || arg == Py_None) {
		time_t now;

		time(&now);
		init_genrand(self, (unsigned long)now);
		Py_INCREF(Py_None);
		return Py_None;
	}
	/* If the arg is an int or long, use its absolute value; else use
	 * the absolute value of its hash code.
	 */
	if (PyInt_Check(arg) || PyLong_Check(arg))
		n = PyNumber_Absolute(arg);
	else {
		long hash = PyObject_Hash(arg);
		if (hash == -1)
			goto Done;
		n = PyLong_FromUnsignedLong((unsigned long)hash);
	}
	if (n == NULL)
		goto Done;

	/* Now split n into 32-bit chunks, from the right.  Each piece is
	 * stored into key, which has a capacity of keymax chunks, of which
	 * keyused are filled.  Alas, the repeated shifting makes this a
	 * quadratic-time algorithm; we'd really like to use
	 * _PyLong_AsByteArray here, but then we'd have to break into the
	 * long representation to figure out how big an array was needed
	 * in advance.
	 */
	keymax = 8; 	/* arbitrary; grows later if needed */
	keyused = 0;
	key = (unsigned long *)PyMem_Malloc(keymax * sizeof(*key));
	if (key == NULL)
		goto Done;

	masklower = PyLong_FromUnsignedLong(0xffffffffU);
	if (masklower == NULL)
		goto Done;
	thirtytwo = PyInt_FromLong(32L);
	if (thirtytwo == NULL)
		goto Done;
	while ((err=PyObject_IsTrue(n))) {
		PyObject *newn;
		PyObject *pychunk;
		unsigned long chunk;

		if (err == -1)
			goto Done;
		pychunk = PyNumber_And(n, masklower);
		if (pychunk == NULL)
			goto Done;
		chunk = PyLong_AsUnsignedLong(pychunk);
		Py_DECREF(pychunk);
		if (chunk == (unsigned long)-1 && PyErr_Occurred())
			goto Done;
		newn = PyNumber_Rshift(n, thirtytwo);
		if (newn == NULL)
			goto Done;
		Py_DECREF(n);
		n = newn;
		if (keyused >= keymax) {
			unsigned long bigger = keymax << 1;
			if ((bigger >> 1) != keymax) {
				PyErr_NoMemory();
				goto Done;
			}
			key = (unsigned long *)PyMem_Realloc(key,
						bigger * sizeof(*key));
			if (key == NULL)
				goto Done;
			keymax = bigger;
		}
		assert(keyused < keymax);
		key[keyused++] = chunk;
	}

	if (keyused == 0)
		key[keyused++] = 0UL;
	result = init_by_array(self, key, keyused);
Done:
	Py_XDECREF(masklower);
	Py_XDECREF(thirtytwo);
	Py_XDECREF(n);
	PyMem_Free(key);
	return result;
}

static PyObject *
random_getstate(RandomObject *self)
{
	PyObject *state;
	PyObject *element;
	int i;

	state = PyTuple_New(N+1);
	if (state == NULL)
		return NULL;
	for (i=0; i<N ; i++) {
		element = PyInt_FromLong((long)(self->state[i]));
		if (element == NULL)
			goto Fail;
		PyTuple_SET_ITEM(state, i, element);
	}
	element = PyInt_FromLong((long)(self->index));
	if (element == NULL)
		goto Fail;
	PyTuple_SET_ITEM(state, i, element);
	return state;

Fail:
	Py_DECREF(state);
	return NULL;
}

static PyObject *
random_setstate(RandomObject *self, PyObject *state)
{
	int i;
	long element;

	if (!PyTuple_Check(state)) {
		PyErr_SetString(PyExc_TypeError,
			"state vector must be a tuple");
		return NULL;
	}
	if (PyTuple_Size(state) != N+1) {
		PyErr_SetString(PyExc_ValueError,
			"state vector is the wrong size");
		return NULL;
	}

	for (i=0; i<N ; i++) {
		element = PyInt_AsLong(PyTuple_GET_ITEM(state, i));
		if (element == -1 && PyErr_Occurred())
			return NULL;
		self->state[i] = (unsigned long)element;
	}

	element = PyInt_AsLong(PyTuple_GET_ITEM(state, i));
	if (element == -1 && PyErr_Occurred())
		return NULL;
	self->index = (int)element;

	Py_INCREF(Py_None);
	return Py_None;
}

/*
Jumpahead should be a fast way advance the generator n-steps ahead, but
lacking a formula for that, the next best is to use n and the existing
state to create a new state far away from the original.

The generator uses constant spaced additive feedback, so shuffling the
state elements ought to produce a state which would not be encountered
(in the near term) by calls to random().  Shuffling is normally
implemented by swapping the ith element with another element ranging
from 0 to i inclusive.  That allows the element to have the possibility
of not being moved.  Since the goal is to produce a new, different
state, the swap element is ranged from 0 to i-1 inclusive.  This assures
that each element gets moved at least once.

To make sure that consecutive calls to jumpahead(n) produce different
states (even in the rare case of involutory shuffles), i+1 is added to
each element at position i.  Successive calls are then guaranteed to
have changing (growing) values as well as shuffled positions.

Finally, the self->index value is set to N so that the generator itself
kicks in on the next call to random().	This assures that all results
have been through the generator and do not just reflect alterations to
the underlying state.
*/

static PyObject *
random_jumpahead(RandomObject *self, PyObject *n)
{
	long i, j;
	PyObject *iobj;
	PyObject *remobj;
	unsigned long *mt, tmp;

	if (!PyInt_Check(n) && !PyLong_Check(n)) {
		PyErr_Format(PyExc_TypeError, "jumpahead requires an "
			     "integer, not '%s'",
			     n->ob_type->tp_name);
		return NULL;
	}

	mt = self->state;
	for (i = N-1; i > 1; i--) {
		iobj = PyInt_FromLong(i);
		if (iobj == NULL)
			return NULL;
		remobj = PyNumber_Remainder(n, iobj);
		Py_DECREF(iobj);
		if (remobj == NULL)
			return NULL;
		j = PyInt_AsLong(remobj);
		Py_DECREF(remobj);
		if (j == -1L && PyErr_Occurred())
			return NULL;
		tmp = mt[i];
		mt[i] = mt[j];
		mt[j] = tmp;
	}

	for (i = 0; i < N; i++)
		mt[i] += i+1;

	self->index = N;
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
random_getrandbits(RandomObject *self, PyObject *args)
{
	int k, i, bytes;
	unsigned long r;
	unsigned char *bytearray;
	PyObject *result;

	if (!PyArg_ParseTuple(args, "i:getrandbits", &k))
		return NULL;

	if (k <= 0) {
		PyErr_SetString(PyExc_ValueError,
				"number of bits must be greater than zero");
		return NULL;
	}

	bytes = ((k - 1) / 32 + 1) * 4;
	bytearray = (unsigned char *)PyMem_Malloc(bytes);
	if (bytearray == NULL) {
		PyErr_NoMemory();
		return NULL;
	}

	/* Fill-out whole words, byte-by-byte to avoid endianness issues */
	for (i=0 ; i<bytes ; i+=4, k-=32) {
		r = genrand_int32(self);
		if (k < 32)
			r >>= (32 - k);
		bytearray[i+0] = (unsigned char)r;
		bytearray[i+1] = (unsigned char)(r >> 8);
		bytearray[i+2] = (unsigned char)(r >> 16);
		bytearray[i+3] = (unsigned char)(r >> 24);
	}

	/* little endian order to match bytearray assignment order */
	result = _PyLong_FromByteArray(bytearray, bytes, 1, 0);
	PyMem_Free(bytearray);
	return result;
}

static PyObject *
random_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	RandomObject *self;
	PyObject *tmp;

	self = (RandomObject *)type->tp_alloc(type, 0);
	if (self == NULL)
		return NULL;
	tmp = random_seed(self, args);
	if (tmp == NULL) {
		Py_DECREF(self);
		return NULL;
	}
	Py_DECREF(tmp);
	return (PyObject *)self;
}

static PyMethodDef random_methods[] = {
	{"random",	(PyCFunction)random_random,  METH_NOARGS,
		PyDoc_STR("random() -> x in the interval [0, 1).")},
	{"seed",	(PyCFunction)random_seed,  METH_VARARGS,
		PyDoc_STR("seed([n]) -> None.  Defaults to current time.")},
	{"getstate",	(PyCFunction)random_getstate,  METH_NOARGS,
		PyDoc_STR("getstate() -> tuple containing the current state.")},
	{"setstate",	  (PyCFunction)random_setstate,  METH_O,
		PyDoc_STR("setstate(state) -> None.  Restores generator state.")},
	{"jumpahead",	(PyCFunction)random_jumpahead,	METH_O,
		PyDoc_STR("jumpahead(int) -> None.  Create new state from "
			  "existing state and integer.")},
	{"getrandbits",	(PyCFunction)random_getrandbits,  METH_VARARGS,
		PyDoc_STR("getrandbits(k) -> x.  Generates a long int with "
			  "k random bits.")},
	{NULL,		NULL}		/* sentinel */
};

PyDoc_STRVAR(random_doc,
"Random() -> create a random number generator with its own internal state.");

static PyTypeObject Random_Type = {
	PyObject_HEAD_INIT(NULL)
	0,				/*ob_size*/
	"_random.Random",		/*tp_name*/
	sizeof(RandomObject),		/*tp_basicsize*/
	0,				/*tp_itemsize*/
	/* methods */
	0,				/*tp_dealloc*/
	0,				/*tp_print*/
	0,				/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	0,				/*tp_as_number*/
	0,				/*tp_as_sequence*/
	0,				/*tp_as_mapping*/
	0,				/*tp_hash*/
	0,				/*tp_call*/
	0,				/*tp_str*/
	PyObject_GenericGetAttr,	/*tp_getattro*/
	0,				/*tp_setattro*/
	0,				/*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,	/*tp_flags*/
	random_doc,			/*tp_doc*/
	0,				/*tp_traverse*/
	0,				/*tp_clear*/
	0,				/*tp_richcompare*/
	0,				/*tp_weaklistoffset*/
	0,				/*tp_iter*/
	0,				/*tp_iternext*/
	random_methods, 		/*tp_methods*/
	0,				/*tp_members*/
	0,				/*tp_getset*/
	0,				/*tp_base*/
	0,				/*tp_dict*/
	0,				/*tp_descr_get*/
	0,				/*tp_descr_set*/
	0,				/*tp_dictoffset*/
	0,				/*tp_init*/
	0,				/*tp_alloc*/
	random_new,			/*tp_new*/
	_PyObject_Del,			/*tp_free*/
	0,				/*tp_is_gc*/
};

PyDoc_STRVAR(module_doc,
"Module implements the Mersenne Twister random number generator.");

PyMODINIT_FUNC
init_random(void)
{
	PyObject *m;

	if (PyType_Ready(&Random_Type) < 0)
		return;
	m = Py_InitModule3("_random", NULL, module_doc);
	Py_INCREF(&Random_Type);
	PyModule_AddObject(m, "Random", (PyObject *)&Random_Type);
}
