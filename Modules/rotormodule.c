/***********************************************************
Copyright 1994 by Lance Ellinghouse,
Cathedral City, California Republic, United States of America.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of Lance Ellinghouse
not be used in advertising or publicity pertaining to distribution 
of the software without specific, written prior permission.

LANCE ELLINGHOUSE DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL LANCE ELLINGHOUSE BE LIABLE FOR ANY SPECIAL, 
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING 
FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, 
NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION 
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* This creates an encryption and decryption engine I am calling
   a rotor due to the original design was a hardware rotor with
   contacts used in Germany during WWII.

Rotor Module:

-  rotor.newrotor('key') -> rotorobject  (default of 6 rotors)
-  rotor.newrotor('key', num_rotors) -> rotorobject

Rotor Objects:

-  ro.setkey('string') -> None (resets the key as defined in newrotor().
-  ro.encrypt('string') -> encrypted string
-  ro.decrypt('encrypted string') -> unencrypted string

-  ro.encryptmore('string') -> encrypted string
-  ro.decryptmore('encrypted string') -> unencrypted string

NOTE: the {en,de}cryptmore() methods use the setup that was
      established via the {en,de}crypt calls. They will NOT
      re-initalize the rotors unless: 1) They have not been
      initialized with {en,de}crypt since the last setkey() call;
      2) {en,de}crypt has not been called for this rotor yet.

NOTE: you MUST use the SAME key in rotor.newrotor()
      if you wish to decrypt an encrypted string.
      Also, the encrypted string is NOT 0-127 ASCII. 
      It is considered BINARY data.

*/

/* Rotor objects */

#include "Python.h"

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

typedef struct {
	PyObject_HEAD
	int seed[3];
    	short key[5];
	int  isinited;
	int  size;
	int  size_mask;
    	int  rotors;
	unsigned char *e_rotor;		     /* [num_rotors][size] */
	unsigned char *d_rotor;		     /* [num_rotors][size] */
	unsigned char *positions;	     /* [num_rotors] */
	unsigned char *advances;	     /* [num_rotors] */
} Rotorobj;

staticforward PyTypeObject Rotor_Type;

#define is_rotor(v)		((v)->ob_type == &Rotor_Type)


/* This defines the necessary routines to manage rotor objects */

static void
set_seed(Rotorobj *r)
{
	r->seed[0] = r->key[0];
	r->seed[1] = r->key[1];
	r->seed[2] = r->key[2];
	r->isinited = FALSE;
}
	
/* Return the next random number in the range [0.0 .. 1.0) */
static double
r_random(Rotorobj *r)
{
	int x, y, z;
	double val, term;

	x = r->seed[0];
	y = r->seed[1];
	z = r->seed[2];

	x = 171 * (x % 177) - 2 * (x/177);
	y = 172 * (y % 176) - 35 * (y/176);
	z = 170 * (z % 178) - 63 * (z/178);
	
	if (x < 0) x = x + 30269;
	if (y < 0) y = y + 30307;
	if (z < 0) z = z + 30323;
	
	r->seed[0] = x;
	r->seed[1] = y;
	r->seed[2] = z;

	term = (double)(
		(((double)x)/(double)30269.0) + 
		(((double)y)/(double)30307.0) + 
		(((double)z)/(double)30323.0)
		);
	val = term - (double)floor((double)term);

	if (val >= 1.0)
		val = 0.0;

	return val;
}

static short
r_rand(Rotorobj *r, short s)
{
	return (short)((short)(r_random(r) * (double)s) % s);
}

static void
set_key(Rotorobj *r, char *key)
{
	unsigned long k1=995, k2=576, k3=767, k4=671, k5=463;
	size_t i;
	size_t len = strlen(key);

	for (i = 0; i < len; i++) {
		unsigned short ki = Py_CHARMASK(key[i]);

		k1 = (((k1<<3 | k1>>13) + ki) & 65535);
		k2 = (((k2<<3 | k2>>13) ^ ki) & 65535);
		k3 = (((k3<<3 | k3>>13) - ki) & 65535);
		k4 = ((ki - (k4<<3 | k4>>13)) & 65535);
		k5 = (((k5<<3 | k5>>13) ^ ~ki) & 65535);
	}
	r->key[0] = (short)k1;
	r->key[1] = (short)(k2|1);
	r->key[2] = (short)k3;
	r->key[3] = (short)k4;
	r->key[4] = (short)k5;

	set_seed(r);
}



/* These define the interface to a rotor object */
static Rotorobj *
rotorobj_new(int num_rotors, char *key)
{
	Rotorobj *xp;

	xp = PyObject_New(Rotorobj, &Rotor_Type);
	if (xp == NULL)
		return NULL;
	set_key(xp, key);

	xp->size = 256;
	xp->size_mask = xp->size - 1;
	xp->size_mask = 0;
	xp->rotors = num_rotors;
	xp->e_rotor = NULL;
	xp->d_rotor = NULL;
	xp->positions = NULL;
	xp->advances = NULL;

	if (!(xp->e_rotor = PyMem_NEW(unsigned char, num_rotors * xp->size)))
		goto finally;
	if (!(xp->d_rotor = PyMem_NEW(unsigned char, num_rotors * xp->size)))
		goto finally;
	if (!(xp->positions = PyMem_NEW(unsigned char, num_rotors)))
		goto finally;
	if (!(xp->advances = PyMem_NEW(unsigned char, num_rotors)))
		goto finally;

	return xp;

  finally:
	if (xp->e_rotor)
		PyMem_DEL(xp->e_rotor);
	if (xp->d_rotor)
		PyMem_DEL(xp->d_rotor);
	if (xp->positions)
		PyMem_DEL(xp->positions);
	if (xp->advances)
		PyMem_DEL(xp->advances);
	Py_DECREF(xp);
	return (Rotorobj*)PyErr_NoMemory();
}


/* These routines implement the rotor itself */

/*  Here is a fairly sophisticated {en,de}cryption system.  It is based on
    the idea of a "rotor" machine.  A bunch of rotors, each with a
    different permutation of the alphabet, rotate around a different amount
    after encrypting one character.  The current state of the rotors is
    used to encrypt one character.

    The code is smart enough to tell if your alphabet has a number of
    characters equal to a power of two.  If it does, it uses logical
    operations, if not it uses div and mod (both require a division).

    You will need to make two changes to the code 1) convert to c, and
    customize for an alphabet of 255 chars 2) add a filter at the begining,
    and end, which subtracts one on the way in, and adds one on the way
    out.

    You might wish to do some timing studies.  Another viable alternative
    is to "byte stuff" the encrypted data of a normal (perhaps this one)
    encryption routine.

    j'

 */

/* Note: the C code here is a fairly straightforward transliteration of a
 * rotor implemented in lisp.  The original lisp code has been removed from
 * this file to for simplification, but I've kept the docstrings as
 * comments in front of the functions.
 */


/* Set ROTOR to the identity permutation */
static void
RTR_make_id_rotor(Rotorobj *r, unsigned char *rtr)
{
	register int j;
	register int size = r->size;
	for (j = 0; j < size; j++) {
		rtr[j] = (unsigned char)j;
	}
}


/* The current set of encryption rotors */
static void
RTR_e_rotors(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		RTR_make_id_rotor(r, &(r->e_rotor[(i*r->size)]));
	}
}

/* The current set of decryption rotors */
static void
RTR_d_rotors(Rotorobj *r)
{
	register int i, j;
	for (i = 0; i < r->rotors; i++) {
		for (j = 0; j < r->size; j++) {
			r->d_rotor[((i*r->size)+j)] = (unsigned char)j;
		}
	}
}

/* The positions of the rotors at this time */
static void
RTR_positions(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		r->positions[i] = 1;
	}
}

/* The number of positions to advance the rotors at a time */
static void
RTR_advances(Rotorobj *r)
{
	int i;
	for (i = 0; i < r->rotors; i++) {
		r->advances[i] = 1;
	}
}

/* Permute the E rotor, and make the D rotor its inverse
 * see Knuth for explanation of algorithm.
 */
static void
RTR_permute_rotor(Rotorobj *r, unsigned char *e, unsigned char *d)
{
	short i = r->size;
	short q;
	unsigned char j;
	RTR_make_id_rotor(r,e);
	while (2 <= i) {
		q = r_rand(r,i);
		i--;
		j = e[q];
		e[q] = (unsigned char)e[i];
		e[i] = (unsigned char)j;
		d[j] = (unsigned char)i;
	}
	e[0] = (unsigned char)e[0];
	d[(e[0])] = (unsigned char)0;
}

/* Given KEY (a list of 5 16 bit numbers), initialize the rotor machine.
 * Set the advancement, position, and permutation of the rotors
 */
static void
RTR_init(Rotorobj *r)
{
	int i;
	set_seed(r);
	RTR_positions(r);
	RTR_advances(r);
	RTR_e_rotors(r);
	RTR_d_rotors(r);
	for (i = 0; i < r->rotors; i++) {
		r->positions[i] = (unsigned char) r_rand(r, (short)r->size);
		r->advances[i] = (1+(2*(r_rand(r, (short)(r->size/2)))));
		RTR_permute_rotor(r,
				  &(r->e_rotor[(i*r->size)]),
				  &(r->d_rotor[(i*r->size)]));
	}
	r->isinited = TRUE;
}

/* Change the RTR-positions vector, using the RTR-advances vector */
static void
RTR_advance(Rotorobj *r)
{
	register int i=0, temp=0;
	if (r->size_mask) {
		while (i < r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp & r->size_mask;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	} else {
		while (i < r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp%r->size;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	}
}

/* Encrypt the character P with the current rotor machine */
static unsigned char
RTR_e_char(Rotorobj *r, unsigned char p)
{
	register int i=0;
	register unsigned char tp=p;
	if (r->size_mask) {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size) +
				       (((r->positions[i] ^ tp) &
					 r->size_mask))];
			i++;
		}
	} else {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size) +
				       (((r->positions[i] ^ tp) %
					 (unsigned int) r->size))];
			i++;
		}
	}
	RTR_advance(r);
	return ((unsigned char)tp);
}

/* Decrypt the character C with the current rotor machine */
static unsigned char
RTR_d_char(Rotorobj *r, unsigned char c)
{
	register int i = r->rotors - 1;
	register unsigned char tc = c;

	if (r->size_mask) {
		while (0 <= i) {
			tc = (r->positions[i] ^
			      r->d_rotor[(i*r->size)+tc]) & r->size_mask;
			i--;
		}
	} else {
		while (0 <= i) {
			tc = (r->positions[i] ^
			      r->d_rotor[(i*r->size)+tc]) %
				(unsigned int) r->size;
			i--;
		}
	}
	RTR_advance(r);
	return(tc);
}

/* Perform a rotor encryption of the region from BEG to END by KEY */
static void
RTR_e_region(Rotorobj *r, unsigned char *beg, int len, int doinit)
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i = 0; i < len; i++) {
		beg[i] = RTR_e_char(r, beg[i]);
	}
}

/* Perform a rotor decryption of the region from BEG to END by KEY */
static void
RTR_d_region(Rotorobj *r, unsigned char *beg, int len, int doinit)
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i = 0; i < len; i++) {
		beg[i] = RTR_d_char(r, beg[i]);
	}
}



/* Rotor methods */
static void
rotor_dealloc(Rotorobj *xp)
{
	if (xp->e_rotor)
		PyMem_DEL(xp->e_rotor);
	if (xp->d_rotor)
		PyMem_DEL(xp->d_rotor);
	if (xp->positions)
		PyMem_DEL(xp->positions);
	if (xp->advances)
		PyMem_DEL(xp->advances);
	PyObject_Del(xp);
}

static PyObject * 
rotorobj_encrypt(Rotorobj *self, PyObject *args)
{
	char *string = NULL;
	int len = 0;
	PyObject *rtn = NULL;
	char *tmp;

	if (!PyArg_Parse(args, "s#", &string, &len))
		return NULL;
	if (!(tmp = PyMem_NEW(char, len+5))) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(tmp, '\0', len+1);
	memcpy(tmp, string, len);
	RTR_e_region(self, (unsigned char *)tmp, len, TRUE);
	rtn = PyString_FromStringAndSize(tmp, len);
	PyMem_DEL(tmp);
	return(rtn);
}

static PyObject * 
rotorobj_encrypt_more(Rotorobj *self, PyObject *args)
{
	char *string = NULL;
	int len = 0;
	PyObject *rtn = NULL;
	char *tmp;

	if (!PyArg_Parse(args, "s#", &string, &len))
		return NULL;
	if (!(tmp = PyMem_NEW(char, len+5))) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(tmp, '\0', len+1);
	memcpy(tmp, string, len);
	RTR_e_region(self, (unsigned char *)tmp, len, FALSE);
	rtn = PyString_FromStringAndSize(tmp, len);
	PyMem_DEL(tmp);
	return(rtn);
}

static PyObject * 
rotorobj_decrypt(Rotorobj *self, PyObject *args)
{
	char *string = NULL;
	int len = 0;
	PyObject *rtn = NULL;
	char *tmp;

	if (!PyArg_Parse(args, "s#", &string, &len))
		return NULL;
	if (!(tmp = PyMem_NEW(char, len+5))) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(tmp, '\0', len+1);
	memcpy(tmp, string, len);
	RTR_d_region(self, (unsigned char *)tmp, len, TRUE);
	rtn = PyString_FromStringAndSize(tmp, len);
	PyMem_DEL(tmp);
	return(rtn);
}

static PyObject * 
rotorobj_decrypt_more(Rotorobj *self, PyObject *args)
{
	char *string = NULL;
	int len = 0;
	PyObject *rtn = NULL;
	char *tmp;

	if (!PyArg_Parse(args, "s#", &string, &len))
		return NULL;
	if (!(tmp = PyMem_NEW(char, len+5))) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(tmp, '\0', len+1);
	memcpy(tmp, string, len);
	RTR_d_region(self, (unsigned char *)tmp, len, FALSE);
	rtn = PyString_FromStringAndSize(tmp, len);
	PyMem_DEL(tmp);
	return(rtn);
}

static PyObject * 
rotorobj_setkey(Rotorobj *self, PyObject *args)
{
	char *key;

	if (!PyArg_ParseTuple(args, "s:setkey", &key))
		return NULL;

	set_key(self, key);
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef
rotorobj_methods[] = {
	{"encrypt",	(PyCFunction)rotorobj_encrypt},
	{"encryptmore",	(PyCFunction)rotorobj_encrypt_more},
	{"decrypt",	(PyCFunction)rotorobj_decrypt},
	{"decryptmore",	(PyCFunction)rotorobj_decrypt_more},
	{"setkey",	(PyCFunction)rotorobj_setkey, 1},
	{NULL,		NULL}		/* sentinel */
};


/* Return a rotor object's named attribute. */
static PyObject * 
rotorobj_getattr(Rotorobj *s, char *name)
{
	return Py_FindMethod(rotorobj_methods, (PyObject*)s, name);
}


statichere PyTypeObject Rotor_Type = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,				/*ob_size*/
	"rotor",			/*tp_name*/
	sizeof(Rotorobj),		/*tp_size*/
	0,				/*tp_itemsize*/
	/* methods */
	(destructor)rotor_dealloc,	/*tp_dealloc*/
	0,				/*tp_print*/
	(getattrfunc)rotorobj_getattr,	/*tp_getattr*/
	0,				/*tp_setattr*/
	0,				/*tp_compare*/
	0,				/*tp_repr*/
	0,                              /*tp_hash*/
};


static PyObject * 
rotor_rotor(PyObject *self, PyObject *args)
{
	Rotorobj *r;
	char *string;
	int len;
	int num_rotors = 6;

	if (!PyArg_ParseTuple(args, "s#|i:newrotor", &string, &len, &num_rotors))
		return NULL;

	r = rotorobj_new(num_rotors, string);
	return (PyObject *)r;
}



static struct PyMethodDef
rotor_methods[] = {
	{"newrotor",  rotor_rotor, 1},
	{NULL,        NULL}		     /* sentinel */
};


DL_EXPORT(void)
initrotor(void)
{
	(void)Py_InitModule("rotor", rotor_methods);
}
