
/* Integer object implementation */

#include "Python.h"
#include <ctype.h>

long
PyInt_GetMax(void)
{
	return LONG_MAX;	/* To initialize sys.maxint */
}

/* Standard Booleans */

PyIntObject _Py_ZeroStruct = {
	PyObject_HEAD_INIT(&PyInt_Type)
	0
};

PyIntObject _Py_TrueStruct = {
	PyObject_HEAD_INIT(&PyInt_Type)
	1
};

static PyObject *
err_ovf(char *msg)
{
	PyErr_SetString(PyExc_OverflowError, msg);
	return NULL;
}

/* Integers are quite normal objects, to make object handling uniform.
   (Using odd pointers to represent integers would save much space
   but require extra checks for this special case throughout the code.)
   Since, a typical Python program spends much of its time allocating
   and deallocating integers, these operations should be very fast.
   Therefore we use a dedicated allocation scheme with a much lower
   overhead (in space and time) than straight malloc(): a simple
   dedicated free list, filled when necessary with memory from malloc().
*/

#define BLOCK_SIZE	1000	/* 1K less typical malloc overhead */
#define BHEAD_SIZE	8	/* Enough for a 64-bit pointer */
#define N_INTOBJECTS	((BLOCK_SIZE - BHEAD_SIZE) / sizeof(PyIntObject))

struct _intblock {
	struct _intblock *next;
	PyIntObject objects[N_INTOBJECTS];
};

typedef struct _intblock PyIntBlock;

static PyIntBlock *block_list = NULL;
static PyIntObject *free_list = NULL;

static PyIntObject *
fill_free_list(void)
{
	PyIntObject *p, *q;
	/* XXX Int blocks escape the object heap. Use PyObject_MALLOC ??? */
	p = (PyIntObject *) PyMem_MALLOC(sizeof(PyIntBlock));
	if (p == NULL)
		return (PyIntObject *) PyErr_NoMemory();
	((PyIntBlock *)p)->next = block_list;
	block_list = (PyIntBlock *)p;
	p = &((PyIntBlock *)p)->objects[0];
	q = p + N_INTOBJECTS;
	while (--q > p)
		q->ob_type = (struct _typeobject *)(q-1);
	q->ob_type = NULL;
	return p + N_INTOBJECTS - 1;
}

#ifndef NSMALLPOSINTS
#define NSMALLPOSINTS		100
#endif
#ifndef NSMALLNEGINTS
#define NSMALLNEGINTS		1
#endif
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
/* References to small integers are saved in this array so that they
   can be shared.
   The integers that are saved are those in the range
   -NSMALLNEGINTS (inclusive) to NSMALLPOSINTS (not inclusive).
*/
static PyIntObject *small_ints[NSMALLNEGINTS + NSMALLPOSINTS];
#endif
#ifdef COUNT_ALLOCS
int quick_int_allocs, quick_neg_int_allocs;
#endif

PyObject *
PyInt_FromLong(long ival)
{
	register PyIntObject *v;
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS &&
	    (v = small_ints[ival + NSMALLNEGINTS]) != NULL) {
		Py_INCREF(v);
#ifdef COUNT_ALLOCS
		if (ival >= 0)
			quick_int_allocs++;
		else
			quick_neg_int_allocs++;
#endif
		return (PyObject *) v;
	}
#endif
	if (free_list == NULL) {
		if ((free_list = fill_free_list()) == NULL)
			return NULL;
	}
	/* PyObject_New is inlined */
	v = free_list;
	free_list = (PyIntObject *)v->ob_type;
	PyObject_INIT(v, &PyInt_Type);
	v->ob_ival = ival;
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
	if (-NSMALLNEGINTS <= ival && ival < NSMALLPOSINTS) {
		/* save this one for a following allocation */
		Py_INCREF(v);
		small_ints[ival + NSMALLNEGINTS] = v;
	}
#endif
	return (PyObject *) v;
}

static void
int_dealloc(PyIntObject *v)
{
	v->ob_type = (struct _typeobject *)free_list;
	free_list = v;
}

long
PyInt_AsLong(register PyObject *op)
{
	PyNumberMethods *nb;
	PyIntObject *io;
	long val;
	
	if (op && PyInt_Check(op))
		return PyInt_AS_LONG((PyIntObject*) op);
	
	if (op == NULL || (nb = op->ob_type->tp_as_number) == NULL ||
	    nb->nb_int == NULL) {
		PyErr_SetString(PyExc_TypeError, "an integer is required");
		return -1;
	}
	
	io = (PyIntObject*) (*nb->nb_int) (op);
	if (io == NULL)
		return -1;
	if (!PyInt_Check(io)) {
		PyErr_SetString(PyExc_TypeError,
				"nb_int should return int object");
		return -1;
	}
	
	val = PyInt_AS_LONG(io);
	Py_DECREF(io);
	
	return val;
}

PyObject *
PyInt_FromString(char *s, char **pend, int base)
{
	char *end;
	long x;
	char buffer[256]; /* For errors */

	if ((base != 0 && base < 2) || base > 36) {
		PyErr_SetString(PyExc_ValueError, "invalid base for int()");
		return NULL;
	}

	while (*s && isspace(Py_CHARMASK(*s)))
		s++;
	errno = 0;
	if (base == 0 && s[0] == '0')
		x = (long) PyOS_strtoul(s, &end, base);
	else
		x = PyOS_strtol(s, &end, base);
	if (end == s || !isalnum(end[-1]))
		goto bad;
	while (*end && isspace(Py_CHARMASK(*end)))
		end++;
	if (*end != '\0') {
  bad:
		sprintf(buffer, "invalid literal for int(): %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	else if (errno != 0) {
		sprintf(buffer, "int() literal too large: %.200s", s);
		PyErr_SetString(PyExc_ValueError, buffer);
		return NULL;
	}
	if (pend)
		*pend = end;
	return PyInt_FromLong(x);
}

PyObject *
PyInt_FromUnicode(Py_UNICODE *s, int length, int base)
{
	char buffer[256];
	
	if (length >= sizeof(buffer)) {
		PyErr_SetString(PyExc_ValueError,
				"int() literal too large to convert");
		return NULL;
	}
	if (PyUnicode_EncodeDecimal(s, length, buffer, NULL))
		return NULL;
	return PyInt_FromString(buffer, NULL, base);
}

/* Methods */

/* ARGSUSED */
static int
int_print(PyIntObject *v, FILE *fp, int flags)
     /* flags -- not used but required by interface */
{
	fprintf(fp, "%ld", v->ob_ival);
	return 0;
}

static PyObject *
int_repr(PyIntObject *v)
{
	char buf[20];
	sprintf(buf, "%ld", v->ob_ival);
	return PyString_FromString(buf);
}

static int
int_compare(PyIntObject *v, PyIntObject *w)
{
	register long i = v->ob_ival;
	register long j = w->ob_ival;
	return (i < j) ? -1 : (i > j) ? 1 : 0;
}

static long
int_hash(PyIntObject *v)
{
	/* XXX If this is changed, you also need to change the way
	   Python's long, float and complex types are hashed. */
	long x = v -> ob_ival;
	if (x == -1)
		x = -2;
	return x;
}

static PyObject *
int_add(PyIntObject *v, PyIntObject *w)
{
	register long a, b, x;
	a = v->ob_ival;
	b = w->ob_ival;
	x = a + b;
	if ((x^a) < 0 && (x^b) < 0)
		return err_ovf("integer addition");
	return PyInt_FromLong(x);
}

static PyObject *
int_sub(PyIntObject *v, PyIntObject *w)
{
	register long a, b, x;
	a = v->ob_ival;
	b = w->ob_ival;
	x = a - b;
	if ((x^a) < 0 && (x^~b) < 0)
		return err_ovf("integer subtraction");
	return PyInt_FromLong(x);
}

/*
Integer overflow checking used to be done using a double, but on 64
bit machines (where both long and double are 64 bit) this fails
because the double doesn't have enough precision.  John Tromp suggests
the following algorithm:

Suppose again we normalize a and b to be nonnegative.
Let ah and al (bh and bl) be the high and low 32 bits of a (b, resp.).
Now we test ah and bh against zero and get essentially 3 possible outcomes.

1) both ah and bh > 0 : then report overflow

2) both ah and bh = 0 : then compute a*b and report overflow if it comes out
                        negative

3) ah > 0 and bh = 0  : compute ah*bl and report overflow if it's >= 2^31
                        compute al*bl and report overflow if it's negative
                        add (ah*bl)<<32 to al*bl and report overflow if
                        it's negative

In case of no overflow the result is then negated if necessary.

The majority of cases will be 2), in which case this method is the same as
what I suggested before. If multiplication is expensive enough, then the
other method is faster on case 3), but also more work to program, so I
guess the above is the preferred solution.

*/

static PyObject *
int_mul(PyIntObject *v, PyIntObject *w)
{
	long a, b, ah, bh, x, y;
	int s = 1;

	a = v->ob_ival;
	b = w->ob_ival;
	ah = a >> (LONG_BIT/2);
	bh = b >> (LONG_BIT/2);

	/* Quick test for common case: two small positive ints */

	if (ah == 0 && bh == 0) {
		x = a*b;
		if (x < 0)
			goto bad;
		return PyInt_FromLong(x);
	}

	/* Arrange that a >= b >= 0 */

	if (a < 0) {
		a = -a;
		if (a < 0) {
			/* Largest negative */
			if (b == 0 || b == 1) {
				x = a*b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		ah = a >> (LONG_BIT/2);
	}
	if (b < 0) {
		b = -b;
		if (b < 0) {
			/* Largest negative */
			if (a == 0 || (a == 1 && s == 1)) {
				x = a*b;
				goto ok;
			}
			else
				goto bad;
		}
		s = -s;
		bh = b >> (LONG_BIT/2);
	}

	/* 1) both ah and bh > 0 : then report overflow */

	if (ah != 0 && bh != 0)
		goto bad;

	/* 2) both ah and bh = 0 : then compute a*b and report
				   overflow if it comes out negative */

	if (ah == 0 && bh == 0) {
		x = a*b;
		if (x < 0)
			goto bad;
		return PyInt_FromLong(x*s);
	}

	if (a < b) {
		/* Swap */
		x = a;
		a = b;
		b = x;
		ah = bh;
		/* bh not used beyond this point */
	}

	/* 3) ah > 0 and bh = 0  : compute ah*bl and report overflow if
				   it's >= 2^31
                        compute al*bl and report overflow if it's negative
                        add (ah*bl)<<32 to al*bl and report overflow if
                        it's negative
			(NB b == bl in this case, and we make a = al) */

	y = ah*b;
	if (y >= (1L << (LONG_BIT/2 - 1)))
		goto bad;
	a &= (1L << (LONG_BIT/2)) - 1;
	x = a*b;
	if (x < 0)
		goto bad;
	x += y << (LONG_BIT/2);
	if (x < 0)
		goto bad;
 ok:
	return PyInt_FromLong(x * s);

 bad:
	return err_ovf("integer multiplication");
}

static int
i_divmod(register PyIntObject *x, register PyIntObject *y,
         long *p_xdivy, long *p_xmody)
{
	long xi = x->ob_ival;
	long yi = y->ob_ival;
	long xdivy, xmody;
	
	if (yi == 0) {
		PyErr_SetString(PyExc_ZeroDivisionError,
				"integer division or modulo");
		return -1;
	}
	if (yi < 0) {
		if (xi < 0) {
			if (yi == -1 && -xi < 0) {
				/* most negative / -1 */
				err_ovf("integer division");
				return -1;
			}
			xdivy = -xi / -yi;
		}
		else
			xdivy = - (xi / -yi);
	}
	else {
		if (xi < 0)
			xdivy = - (-xi / yi);
		else
			xdivy = xi / yi;
	}
	xmody = xi - xdivy*yi;
	if ((xmody < 0 && yi > 0) || (xmody > 0 && yi < 0)) {
		xmody += yi;
		xdivy -= 1;
	}
	*p_xdivy = xdivy;
	*p_xmody = xmody;
	return 0;
}

static PyObject *
int_div(PyIntObject *x, PyIntObject *y)
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return PyInt_FromLong(d);
}

static PyObject *
int_mod(PyIntObject *x, PyIntObject *y)
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return PyInt_FromLong(m);
}

static PyObject *
int_divmod(PyIntObject *x, PyIntObject *y)
{
	long d, m;
	if (i_divmod(x, y, &d, &m) < 0)
		return NULL;
	return Py_BuildValue("(ll)", d, m);
}

static PyObject *
int_pow(PyIntObject *v, PyIntObject *w, PyIntObject *z)
{
#if 1
	register long iv, iw, iz=0, ix, temp, prev;
	iv = v->ob_ival;
	iw = w->ob_ival;
	if (iw < 0) {
		if (iv)
			PyErr_SetString(PyExc_ValueError,
					"integer to a negative power");
		else
			PyErr_SetString(PyExc_ZeroDivisionError,
					"0 to a negative power");
		return NULL;
	}
 	if ((PyObject *)z != Py_None) {
		iz = z->ob_ival;
		if (iz == 0) {
			PyErr_SetString(PyExc_ValueError,
					"pow(x, y, z) with z==0");
			return NULL;
		}
	}
	/*
	 * XXX: The original exponentiation code stopped looping
	 * when temp hit zero; this code will continue onwards
	 * unnecessarily, but at least it won't cause any errors.
	 * Hopefully the speed improvement from the fast exponentiation
	 * will compensate for the slight inefficiency.
	 * XXX: Better handling of overflows is desperately needed.
	 */
 	temp = iv;
	ix = 1;
	while (iw > 0) {
	 	prev = ix;	/* Save value for overflow check */
	 	if (iw & 1) {	
		 	ix = ix*temp;
			if (temp == 0)
				break; /* Avoid ix / 0 */
			if (ix / temp != prev)
				return err_ovf("integer exponentiation");
		}
	 	iw >>= 1;	/* Shift exponent down by 1 bit */
	        if (iw==0) break;
	 	prev = temp;
	 	temp *= temp;	/* Square the value of temp */
	 	if (prev!=0 && temp/prev!=prev)
			return err_ovf("integer exponentiation");
	 	if (iz) {
			/* If we did a multiplication, perform a modulo */
		 	ix = ix % iz;
		 	temp = temp % iz;
		}
	}
	if (iz) {
	 	PyObject *t1, *t2;
	 	long int div, mod;
	 	t1=PyInt_FromLong(ix); 
		t2=PyInt_FromLong(iz);
	 	if (t1==NULL || t2==NULL ||
	 		i_divmod((PyIntObject *)t1,
				 (PyIntObject *)t2, &div, &mod)<0)
		{
		 	Py_XDECREF(t1);
		 	Py_XDECREF(t2);
			return(NULL);
		}
		Py_DECREF(t1);
		Py_DECREF(t2);
	 	ix=mod;
	}
	return PyInt_FromLong(ix);
#else
	register long iv, iw, ix;
	iv = v->ob_ival;
	iw = w->ob_ival;
	if (iw < 0) {
		PyErr_SetString(PyExc_ValueError,
				"integer to the negative power");
		return NULL;
	}
	if ((PyObject *)z != Py_None) {
		PyErr_SetString(PyExc_TypeError,
				"pow(int, int, int) not yet supported");
		return NULL;
	}
	ix = 1;
	while (--iw >= 0) {
		long prev = ix;
		ix = ix * iv;
		if (iv == 0)
			break; /* 0 to some power -- avoid ix / 0 */
		if (ix / iv != prev)
			return err_ovf("integer exponentiation");
	}
	return PyInt_FromLong(ix);
#endif
}				

static PyObject *
int_neg(PyIntObject *v)
{
	register long a, x;
	a = v->ob_ival;
	x = -a;
	if (a < 0 && x < 0)
		return err_ovf("integer negation");
	return PyInt_FromLong(x);
}

static PyObject *
int_pos(PyIntObject *v)
{
	Py_INCREF(v);
	return (PyObject *)v;
}

static PyObject *
int_abs(PyIntObject *v)
{
	if (v->ob_ival >= 0)
		return int_pos(v);
	else
		return int_neg(v);
}

static int
int_nonzero(PyIntObject *v)
{
	return v->ob_ival != 0;
}

static PyObject *
int_invert(PyIntObject *v)
{
	return PyInt_FromLong(~v->ob_ival);
}

static PyObject *
int_lshift(PyIntObject *v, PyIntObject *w)
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	if (b < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0) {
		Py_INCREF(v);
		return (PyObject *) v;
	}
	if (b >= LONG_BIT) {
		return PyInt_FromLong(0L);
	}
	a = (unsigned long)a << b;
	return PyInt_FromLong(a);
}

static PyObject *
int_rshift(PyIntObject *v, PyIntObject *w)
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	if (b < 0) {
		PyErr_SetString(PyExc_ValueError, "negative shift count");
		return NULL;
	}
	if (a == 0 || b == 0) {
		Py_INCREF(v);
		return (PyObject *) v;
	}
	if (b >= LONG_BIT) {
		if (a < 0)
			a = -1;
		else
			a = 0;
	}
	else {
		a = Py_ARITHMETIC_RIGHT_SHIFT(long, a, b);
	}
	return PyInt_FromLong(a);
}

static PyObject *
int_and(PyIntObject *v, PyIntObject *w)
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return PyInt_FromLong(a & b);
}

static PyObject *
int_xor(PyIntObject *v, PyIntObject *w)
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return PyInt_FromLong(a ^ b);
}

static PyObject *
int_or(PyIntObject *v, PyIntObject *w)
{
	register long a, b;
	a = v->ob_ival;
	b = w->ob_ival;
	return PyInt_FromLong(a | b);
}

static PyObject *
int_int(PyIntObject *v)
{
	Py_INCREF(v);
	return (PyObject *)v;
}

static PyObject *
int_long(PyIntObject *v)
{
	return PyLong_FromLong((v -> ob_ival));
}

static PyObject *
int_float(PyIntObject *v)
{
	return PyFloat_FromDouble((double)(v -> ob_ival));
}

static PyObject *
int_oct(PyIntObject *v)
{
	char buf[100];
	long x = v -> ob_ival;
	if (x == 0)
		strcpy(buf, "0");
	else
		sprintf(buf, "0%lo", x);
	return PyString_FromString(buf);
}

static PyObject *
int_hex(PyIntObject *v)
{
	char buf[100];
	long x = v -> ob_ival;
	sprintf(buf, "0x%lx", x);
	return PyString_FromString(buf);
}

static PyNumberMethods int_as_number = {
	(binaryfunc)int_add, /*nb_add*/
	(binaryfunc)int_sub, /*nb_subtract*/
	(binaryfunc)int_mul, /*nb_multiply*/
	(binaryfunc)int_div, /*nb_divide*/
	(binaryfunc)int_mod, /*nb_remainder*/
	(binaryfunc)int_divmod, /*nb_divmod*/
	(ternaryfunc)int_pow, /*nb_power*/
	(unaryfunc)int_neg, /*nb_negative*/
	(unaryfunc)int_pos, /*nb_positive*/
	(unaryfunc)int_abs, /*nb_absolute*/
	(inquiry)int_nonzero, /*nb_nonzero*/
	(unaryfunc)int_invert, /*nb_invert*/
	(binaryfunc)int_lshift, /*nb_lshift*/
	(binaryfunc)int_rshift, /*nb_rshift*/
	(binaryfunc)int_and, /*nb_and*/
	(binaryfunc)int_xor, /*nb_xor*/
	(binaryfunc)int_or, /*nb_or*/
	0,		/*nb_coerce*/
	(unaryfunc)int_int, /*nb_int*/
	(unaryfunc)int_long, /*nb_long*/
	(unaryfunc)int_float, /*nb_float*/
	(unaryfunc)int_oct, /*nb_oct*/
	(unaryfunc)int_hex, /*nb_hex*/
};

PyTypeObject PyInt_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"int",
	sizeof(PyIntObject),
	0,
	(destructor)int_dealloc, /*tp_dealloc*/
	(printfunc)int_print, /*tp_print*/
	0,		/*tp_getattr*/
	0,		/*tp_setattr*/
	(cmpfunc)int_compare, /*tp_compare*/
	(reprfunc)int_repr, /*tp_repr*/
	&int_as_number,	/*tp_as_number*/
	0,		/*tp_as_sequence*/
	0,		/*tp_as_mapping*/
	(hashfunc)int_hash, /*tp_hash*/
};

void
PyInt_Fini(void)
{
	PyIntObject *p;
	PyIntBlock *list, *next;
	int i;
	int bc, bf;	/* block count, number of freed blocks */
	int irem, isum;	/* remaining unfreed ints per block, total */

#if NSMALLNEGINTS + NSMALLPOSINTS > 0
        PyIntObject **q;

        i = NSMALLNEGINTS + NSMALLPOSINTS;
        q = small_ints;
        while (--i >= 0) {
                Py_XDECREF(*q);
                *q++ = NULL;
        }
#endif
	bc = 0;
	bf = 0;
	isum = 0;
	list = block_list;
	block_list = NULL;
	free_list = NULL;
	while (list != NULL) {
		bc++;
		irem = 0;
		for (i = 0, p = &list->objects[0];
		     i < N_INTOBJECTS;
		     i++, p++) {
			if (PyInt_Check(p) && p->ob_refcnt != 0)
				irem++;
		}
		next = list->next;
		if (irem) {
			list->next = block_list;
			block_list = list;
			for (i = 0, p = &list->objects[0];
			     i < N_INTOBJECTS;
			     i++, p++) {
				if (!PyInt_Check(p) || p->ob_refcnt == 0) {
					p->ob_type = (struct _typeobject *)
						free_list;
					free_list = p;
				}
#if NSMALLNEGINTS + NSMALLPOSINTS > 0
				else if (-NSMALLNEGINTS <= p->ob_ival &&
					 p->ob_ival < NSMALLPOSINTS &&
					 small_ints[p->ob_ival +
						    NSMALLNEGINTS] == NULL) {
					Py_INCREF(p);
					small_ints[p->ob_ival +
						   NSMALLNEGINTS] = p;
				}
#endif
			}
		}
		else {
			PyMem_FREE(list); /* XXX PyObject_FREE ??? */
			bf++;
		}
		isum += irem;
		list = next;
	}
	if (!Py_VerboseFlag)
		return;
	fprintf(stderr, "# cleanup ints");
	if (!isum) {
		fprintf(stderr, "\n");
	}
	else {
		fprintf(stderr,
			": %d unfreed int%s in %d out of %d block%s\n",
			isum, isum == 1 ? "" : "s",
			bc - bf, bc, bc == 1 ? "" : "s");
	}
	if (Py_VerboseFlag > 1) {
		list = block_list;
		while (list != NULL) {
			for (i = 0, p = &list->objects[0];
			     i < N_INTOBJECTS;
			     i++, p++) {
				if (PyInt_Check(p) && p->ob_refcnt != 0)
					fprintf(stderr,
				"#   <int at %p, refcnt=%d, val=%ld>\n",
						p, p->ob_refcnt, p->ob_ival);
			}
			list = list->next;
		}
	}
}
