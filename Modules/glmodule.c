
/*
Input used to generate the Python module "glmodule.c".
The stub generator is a Python script called "cgen.py".

Each definition must be contained on one line:

<returntype> <name> <type> <arg> <type> <arg>

<returntype> can be: void, short, long (XXX maybe others?)

<type> can be: char, string, short, float, long, or double
	string indicates a null terminated string;
	if <type> is char and <arg> begins with a *, the * is stripped
	and <type> is changed into string

<arg> has the form <mode> or <mode>[<subscript>]
	where <mode> can be
		s: arg is sent
		r: arg is received		(arg is a pointer)
	and <subscript> can be (N and I are numbers):
		N
		argI
		retval
		N*argI
		N*I
		N*retval
	In the case where the subscript consists of two parts
	separated by *, the first part is the width of the matrix, and
	the second part is the length of the matrix.  This order is
	opposite from the order used in C to declare a two-dimensional
	matrix.
*/

/*
 * An attempt has been made to make this module switch threads on qread
 * calls. It is far from safe, though.
 */

#include <gl.h>
#include <device.h>

#ifdef __sgi
extern int devport();
extern int textwritemask();
extern int pagewritemask();
extern int gewrite();
extern int gettp();
#endif

#include "Python.h"
#include "cgensupport.h"

/*
Some stubs are too complicated for the stub generator.
We can include manually written versions of them here.
A line starting with '%' gives the name of the function so the stub
generator can include it in the table of functions.
*/


static PyObject *
gl_qread(PyObject *self, PyObject *args)
{
	long retval;
	short arg1 ;
	Py_BEGIN_ALLOW_THREADS
	retval = qread( & arg1 );
	Py_END_ALLOW_THREADS
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewlongobject(retval));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg1));
	  return v;
	}
}


/*
varray -- an array of v.. calls.
The argument is an array (maybe list or tuple) of points.
Each point must be a tuple or list of coordinates (x, y, z).
The points may be 2- or 3-dimensional but must all have the
same dimension.  Float and int values may be mixed however.
The points are always converted to 3D double precision points
by assuming z=0.0 if necessary (as indicated in the man page),
and for each point v3d() is called.
*/


static PyObject *
gl_varray(PyObject *self, PyObject *args)
{
	PyObject *v, *w=NULL;
	int i, n, width;
	double vec[3];
	PyObject * (*getitem)(PyObject *, int);
	
	if (!PyArg_GetObject(args, 1, 0, &v))
		return NULL;
	
	if (PyList_Check(v)) {
		n = PyList_Size(v);
		getitem = PyList_GetItem;
	}
	else if (PyTuple_Check(v)) {
		n = PyTuple_Size(v);
		getitem = PyTuple_GetItem;
	}
	else {
		PyErr_BadArgument();
		return NULL;
	}
	
	if (n == 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (n > 0)
		w = (*getitem)(v, 0);
	
	width = 0;
	if (w == NULL) {
	}
	else if (PyList_Check(w)) {
		width = PyList_Size(w);
	}
	else if (PyTuple_Check(w)) {
		width = PyTuple_Size(w);
	}
	
	switch (width) {
	case 2:
		vec[2] = 0.0;
		/* Fall through */
	case 3:
		break;
	default:
		PyErr_BadArgument();
		return NULL;
	}
	
	for (i = 0; i < n; i++) {
		w = (*getitem)(v, i);
		if (!PyArg_GetDoubleArray(w, 1, 0, width, vec))
			return NULL;
		v3d(vec);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/*
vnarray, nvarray -- an array of n3f and v3f calls.
The argument is an array (list or tuple) of pairs of points and normals.
Each pair is a tuple (NOT a list) of a point and a normal for that point.
Each point or normal must be a tuple (NOT a list) of coordinates (x, y, z).
Three coordinates must be given.  Float and int values may be mixed.
For each pair, n3f() is called for the normal, and then v3f() is called
for the vector.

vnarray and nvarray differ only in the order of the vector and normal in
the pair: vnarray expects (v, n) while nvarray expects (n, v).
*/

static PyObject *gen_nvarray(); /* Forward */


static PyObject *
gl_nvarray(PyObject *self, PyObject *args)
{
	return gen_nvarray(args, 0);
}


static PyObject *
gl_vnarray(PyObject *self, PyObject *args)
{
	return gen_nvarray(args, 1);
}

/* Generic, internal version of {nv,nv}array: inorm indicates the
   argument order, 0: normal first, 1: vector first. */

static PyObject *
gen_nvarray(PyObject *args, int inorm)
{
	PyObject *v, *w, *wnorm, *wvec;
	int i, n;
	float norm[3], vec[3];
	PyObject * (*getitem)(PyObject *, int);
	
	if (!PyArg_GetObject(args, 1, 0, &v))
		return NULL;
	
	if (PyList_Check(v)) {
		n = PyList_Size(v);
		getitem = PyList_GetItem;
	}
	else if (PyTuple_Check(v)) {
		n = PyTuple_Size(v);
		getitem = PyTuple_GetItem;
	}
	else {
		PyErr_BadArgument();
		return NULL;
	}
	
	for (i = 0; i < n; i++) {
		w = (*getitem)(v, i);
		if (!PyTuple_Check(w) || PyTuple_Size(w) != 2) {
			PyErr_BadArgument();
			return NULL;
		}
		wnorm = PyTuple_GetItem(w, inorm);
		wvec = PyTuple_GetItem(w, 1 - inorm);
		if (!PyArg_GetFloatArray(wnorm, 1, 0, 3, norm) ||
			!PyArg_GetFloatArray(wvec, 1, 0, 3, vec))
			return NULL;
		n3f(norm);
		v3f(vec);
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* nurbssurface(s_knots[], t_knots[], ctl[][], s_order, t_order, type).
   The dimensions of ctl[] are computed as follows:
   [len(s_knots) - s_order], [len(t_knots) - t_order]
*/


static PyObject *
gl_nurbssurface(PyObject *self, PyObject *args)
{
	long arg1 ;
	double * arg2 ;
	long arg3 ;
	double * arg4 ;
	double *arg5 ;
	long arg6 ;
	long arg7 ;
	long arg8 ;
	long ncoords;
	long s_byte_stride, t_byte_stride;
	long s_nctl, t_nctl;
	long s, t;
	PyObject *v, *w, *pt;
	double *pnext;
	if (!PyArg_GetLongArraySize(args, 6, 0, &arg1))
		return NULL;
	if ((arg2 = PyMem_NEW(double, arg1 )) == NULL) {
		return PyErr_NoMemory();
	}
	if (!PyArg_GetDoubleArray(args, 6, 0, arg1 , arg2))
		return NULL;
	if (!PyArg_GetLongArraySize(args, 6, 1, &arg3))
		return NULL;
	if ((arg4 = PyMem_NEW(double, arg3 )) == NULL) {
		return PyErr_NoMemory();
	}
	if (!PyArg_GetDoubleArray(args, 6, 1, arg3 , arg4))
		return NULL;
	if (!PyArg_GetLong(args, 6, 3, &arg6))
		return NULL;
	if (!PyArg_GetLong(args, 6, 4, &arg7))
		return NULL;
	if (!PyArg_GetLong(args, 6, 5, &arg8))
		return NULL;
	if (arg8 == N_XYZ)
		ncoords = 3;
	else if (arg8 == N_XYZW)
		ncoords = 4;
	else {
		PyErr_BadArgument();
		return NULL;
	}
	s_nctl = arg1 - arg6;
	t_nctl = arg3 - arg7;
	if (!PyArg_GetObject(args, 6, 2, &v))
		return NULL;
	if (!PyList_Check(v) || PyList_Size(v) != s_nctl) {
		PyErr_BadArgument();
		return NULL;
	}
	if ((arg5 = PyMem_NEW(double, s_nctl*t_nctl*ncoords )) == NULL) {
		return PyErr_NoMemory();
	}
	pnext = arg5;
	for (s = 0; s < s_nctl; s++) {
		w = PyList_GetItem(v, s);
		if (w == NULL || !PyList_Check(w) ||
					PyList_Size(w) != t_nctl) {
			PyErr_BadArgument();
			return NULL;
		}
		for (t = 0; t < t_nctl; t++) {
			pt = PyList_GetItem(w, t);
			if (!PyArg_GetDoubleArray(pt, 1, 0, ncoords, pnext))
				return NULL;
			pnext += ncoords;
		}
	}
	s_byte_stride = sizeof(double) * ncoords;
	t_byte_stride = s_byte_stride * s_nctl;
	nurbssurface( arg1 , arg2 , arg3 , arg4 ,
		s_byte_stride , t_byte_stride , arg5 , arg6 , arg7 , arg8 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg4);
	PyMem_DEL(arg5);
	Py_INCREF(Py_None);
	return Py_None;
}

/* nurbscurve(knots, ctlpoints, order, type).
   The length of ctlpoints is len(knots)-order. */


static PyObject *
gl_nurbscurve(PyObject *self, PyObject *args)
{
	long arg1 ;
	double * arg2 ;
	long arg3 ;
	double * arg4 ;
	long arg5 ;
	long arg6 ;
	int ncoords, npoints;
	int i;
	PyObject *v;
	double *pnext;
	if (!PyArg_GetLongArraySize(args, 4, 0, &arg1))
		return NULL;
	if ((arg2 = PyMem_NEW(double, arg1 )) == NULL) {
		return PyErr_NoMemory();
	}
	if (!PyArg_GetDoubleArray(args, 4, 0, arg1 , arg2))
		return NULL;
	if (!PyArg_GetLong(args, 4, 2, &arg5))
		return NULL;
	if (!PyArg_GetLong(args, 4, 3, &arg6))
		return NULL;
	if (arg6 == N_ST)
		ncoords = 2;
	else if (arg6 == N_STW)
		ncoords = 3;
	else {
		PyErr_BadArgument();
		return NULL;
	}
	npoints = arg1 - arg5;
	if (!PyArg_GetObject(args, 4, 1, &v))
		return NULL;
	if (!PyList_Check(v) || PyList_Size(v) != npoints) {
		PyErr_BadArgument();
		return NULL;
	}
	if ((arg4 = PyMem_NEW(double, npoints*ncoords )) == NULL) {
		return PyErr_NoMemory();
	}
	pnext = arg4;
	for (i = 0; i < npoints; i++) {
		if (!PyArg_GetDoubleArray(PyList_GetItem(v, i), 1, 0, ncoords, pnext))
			return NULL;
		pnext += ncoords;
	}
	arg3 = (sizeof(double)) * ncoords;
	nurbscurve( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg4);
	Py_INCREF(Py_None);
	return Py_None;
}

/* pwlcurve(points, type).
   Points is a list of points. Type must be N_ST. */


static PyObject *
gl_pwlcurve(PyObject *self, PyObject *args)
{
	PyObject *v;
	long type;
	double *data, *pnext;
	long npoints, ncoords;
	int i;
	if (!PyArg_GetObject(args, 2, 0, &v))
		return NULL;
	if (!PyArg_GetLong(args, 2, 1, &type))
		return NULL;
	if (!PyList_Check(v)) {
		PyErr_BadArgument();
		return NULL;
	}
	npoints = PyList_Size(v);
	if (type == N_ST)
		ncoords = 2;
	else {
		PyErr_BadArgument();
		return NULL;
	}
	if ((data = PyMem_NEW(double, npoints*ncoords)) == NULL) {
		return PyErr_NoMemory();
	}
	pnext = data;
	for (i = 0; i < npoints; i++) {
		if (!PyArg_GetDoubleArray(PyList_GetItem(v, i), 1, 0, ncoords, pnext))
			return NULL;
		pnext += ncoords;
	}
	pwlcurve(npoints, data, sizeof(double)*ncoords, type);
	PyMem_DEL(data);
	Py_INCREF(Py_None);
	return Py_None;
}


/* Picking and Selecting */

static short *pickbuffer = NULL;
static long pickbuffersize;

static PyObject *
pick_select(PyObject *args, void (*func)())
{
	if (!PyArg_GetLong(args, 1, 0, &pickbuffersize))
		return NULL;
	if (pickbuffer != NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			"pick/gselect: already picking/selecting");
		return NULL;
	}
	if ((pickbuffer = PyMem_NEW(short, pickbuffersize)) == NULL) {
		return PyErr_NoMemory();
	}
	(*func)(pickbuffer, pickbuffersize);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *
endpick_select(PyObject *args, long (*func)())
{
	PyObject *v, *w;
	int i, nhits, n;
	if (!PyArg_NoArgs(args))
		return NULL;
	if (pickbuffer == NULL) {
		PyErr_SetString(PyExc_RuntimeError,
			"endpick/endselect: not in pick/select mode");
		return NULL;
	}
	nhits = (*func)(pickbuffer);
	if (nhits < 0) {
		nhits = -nhits; /* How to report buffer overflow otherwise? */
	}
	/* Scan the buffer to see how many integers */
	n = 0;
	for (; nhits > 0; nhits--) {
		n += 1 + pickbuffer[n];
	}
	v = PyList_New(n);
	if (v == NULL)
		return NULL;
	/* XXX Could do it nicer and interpret the data structure here,
	   returning a list of lists. But this can be done in Python... */
	for (i = 0; i < n; i++) {
		w = PyInt_FromLong((long)pickbuffer[i]);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i, w);
	}
	PyMem_DEL(pickbuffer);
	pickbuffer = NULL;
	return v;
}

extern void pick(), gselect();
extern long endpick(), endselect();

static PyObject *gl_pick(PyObject *self, PyObject *args)
{
	return pick_select(args, pick);
}

static PyObject *gl_endpick(PyObject *self, PyObject *args)
{
	return endpick_select(args, endpick);
}

static PyObject *gl_gselect(PyObject *self, PyObject *args)
{
	return pick_select(args, gselect);
}

static PyObject *gl_endselect(PyObject *self, PyObject *args)
{
	return endpick_select(args, endselect);
}


/* XXX The generator botches this one.  Here's a quick hack to fix it. */

/* XXX The generator botches this one.  Here's a quick hack to fix it. */


static PyObject *
gl_getmatrix(PyObject *self, PyObject *args)
{
	Matrix arg1;
	PyObject *v, *w;
	int i, j;
	getmatrix( arg1 );
	v = PyList_New(16);
	if (v == NULL) {
		return PyErr_NoMemory();
	}
	for (i = 0; i < 4; i++) for (j = 0; j < 4; j++) {
		w = mknewfloatobject(arg1[i][j]);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i*4+j, w);
	}
	return v;
}

/* Here's an alternate version that returns a 4x4 matrix instead of
   a vector.  Unfortunately it is incompatible with loadmatrix and
   multmatrix... */


static PyObject *
gl_altgetmatrix(PyObject *self, PyObject *args)
{
	Matrix arg1;
	PyObject *v, *w;
	int i, j;
	getmatrix( arg1 );
	v = PyList_New(4);
	if (v == NULL) {
		return NULL;
	}
	for (i = 0; i < 4; i++) {
		w = PyList_New(4);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i, w);
	}
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			w = mknewfloatobject(arg1[i][j]);
			if (w == NULL) {
				Py_DECREF(v);
				return NULL;
			}
			PyList_SetItem(PyList_GetItem(v, i), j, w);
		}
	}
	return v;
}


static PyObject *
gl_lrectwrite(PyObject *self, PyObject *args)
{
	short x1 ;
	short y1 ;
	short x2 ;
	short y2 ;
	string parray ;
	PyObject *s;
#if 0
	int pixcount;
#endif
	if (!PyArg_GetShort(args, 5, 0, &x1))
		return NULL;
	if (!PyArg_GetShort(args, 5, 1, &y1))
		return NULL;
	if (!PyArg_GetShort(args, 5, 2, &x2))
		return NULL;
	if (!PyArg_GetShort(args, 5, 3, &y2))
		return NULL;
	if (!PyArg_GetString(args, 5, 4, &parray))
		return NULL;
	if (!PyArg_GetObject(args, 5, 4, &s))
		return NULL;
#if 0
/* Don't check this, it breaks experiments with pixmode(PM_SIZE, ...) */
	pixcount = (long)(x2+1-x1) * (long)(y2+1-y1);
	if (!PyString_Check(s) || PyString_Size(s) != pixcount*sizeof(long)) {
		PyErr_SetString(PyExc_RuntimeError,
			   "string arg to lrectwrite has wrong size");
		return NULL;
	}
#endif
	lrectwrite( x1 , y1 , x2 , y2 , (unsigned long *) parray );
	Py_INCREF(Py_None);
	return Py_None;
}


static PyObject *
gl_lrectread(PyObject *self, PyObject *args)
{
	short x1 ;
	short y1 ;
	short x2 ;
	short y2 ;
	PyObject *parray;
	int pixcount;
	if (!PyArg_GetShort(args, 4, 0, &x1))
		return NULL;
	if (!PyArg_GetShort(args, 4, 1, &y1))
		return NULL;
	if (!PyArg_GetShort(args, 4, 2, &x2))
		return NULL;
	if (!PyArg_GetShort(args, 4, 3, &y2))
		return NULL;
	pixcount = (long)(x2+1-x1) * (long)(y2+1-y1);
	parray = PyString_FromStringAndSize((char *)NULL, pixcount*sizeof(long));
	if (parray == NULL)
		return NULL; /* No memory */
	lrectread(x1, y1, x2, y2, (unsigned long *) PyString_AsString(parray));
	return parray;
}


static PyObject *
gl_readdisplay(PyObject *self, PyObject *args)
{
        short x1, y1, x2, y2;
	unsigned long *parray, hints;
	long size, size_ret;
	PyObject *rv;

	if ( !PyArg_Parse(args, "hhhhl", &x1, &y1, &x2, &y2, &hints) )
	  return 0;
	size = (long)(x2+1-x1) * (long)(y2+1-y1);
	rv = PyString_FromStringAndSize((char *)NULL, size*sizeof(long));
	if ( rv == NULL )
	  return NULL;
	parray = (unsigned long *)PyString_AsString(rv);
	size_ret = readdisplay(x1, y1, x2, y2, parray, hints);
	if ( size_ret != size ) {
	    printf("gl_readdisplay: got %ld pixels, expected %ld\n",
		   size_ret, size);
	    PyErr_SetString(PyExc_RuntimeError, "readdisplay returned unexpected length");
	    return NULL;
	}
	return rv;
}

/* Desperately needed, here are tools to compress and decompress
   the data manipulated by lrectread/lrectwrite.

   gl.packrect(width, height, packfactor, bigdata) --> smalldata
		makes 'bigdata' 4*(packfactor**2) times smaller by:
		- turning it into B/W (a factor 4)
		- replacing squares of size pacfactor by one
		  representative

   gl.unpackrect(width, height, packfactor, smalldata) --> bigdata
		is the inverse; the numeric arguments must be *the same*.

   Both work best if width and height are multiples of packfactor
   (in fact unpackrect will leave garbage bytes).
*/


static PyObject *
gl_packrect(PyObject *self, PyObject *args)
{
	long width, height, packfactor;
	char *s;
	PyObject *unpacked, *packed;
	int pixcount, packedcount, x, y, r, g, b;
	unsigned long pixel;
	unsigned char *p;
	unsigned long *parray;
	if (!PyArg_GetLong(args, 4, 0, &width))
		return NULL;
	if (!PyArg_GetLong(args, 4, 1, &height))
		return NULL;
	if (!PyArg_GetLong(args, 4, 2, &packfactor))
		return NULL;
	if (!PyArg_GetString(args, 4, 3, &s)) /* For type checking only */
		return NULL;
	if (!PyArg_GetObject(args, 4, 3, &unpacked))
		return NULL;
	if (width <= 0 || height <= 0 || packfactor <= 0) {
		PyErr_SetString(PyExc_RuntimeError, "packrect args must be > 0");
		return NULL;
	}
	pixcount = width*height;
	packedcount = ((width+packfactor-1)/packfactor) *
		((height+packfactor-1)/packfactor);
	if (PyString_Size(unpacked) != pixcount*sizeof(long)) {
		PyErr_SetString(PyExc_RuntimeError,
			   "string arg to packrect has wrong size");
		return NULL;
	}
	packed = PyString_FromStringAndSize((char *)NULL, packedcount);
	if (packed == NULL)
		return NULL;
	parray = (unsigned long *) PyString_AsString(unpacked);
	p = (unsigned char *) PyString_AsString(packed);
	for (y = 0; y < height; y += packfactor, parray += packfactor*width) {
		for (x = 0; x < width; x += packfactor) {
			pixel = parray[x];
			r = pixel & 0xff;
			g = (pixel >> 8) & 0xff;
			b = (pixel >> 16) & 0xff;
			*p++ = (30*r+59*g+11*b) / 100;
		}
	}
	return packed;
}


static unsigned long unpacktab[256];
static int unpacktab_inited = 0;

static PyObject *
gl_unpackrect(PyObject *self, PyObject *args)
{
	long width, height, packfactor;
	char *s;
	PyObject *unpacked, *packed;
	int pixcount, packedcount;
	register unsigned char *p;
	register unsigned long *parray;
	if (!unpacktab_inited) {
		register int white;
		for (white = 256; --white >= 0; )
			unpacktab[white] = white * 0x010101L;
		unpacktab_inited++;
	}
	if (!PyArg_GetLong(args, 4, 0, &width))
		return NULL;
	if (!PyArg_GetLong(args, 4, 1, &height))
		return NULL;
	if (!PyArg_GetLong(args, 4, 2, &packfactor))
		return NULL;
	if (!PyArg_GetString(args, 4, 3, &s)) /* For type checking only */
		return NULL;
	if (!PyArg_GetObject(args, 4, 3, &packed))
		return NULL;
	if (width <= 0 || height <= 0 || packfactor <= 0) {
		PyErr_SetString(PyExc_RuntimeError, "packrect args must be > 0");
		return NULL;
	}
	pixcount = width*height;
	packedcount = ((width+packfactor-1)/packfactor) *
		((height+packfactor-1)/packfactor);
	if (PyString_Size(packed) != packedcount) {
		PyErr_SetString(PyExc_RuntimeError,
			   "string arg to unpackrect has wrong size");
		return NULL;
	}
	unpacked = PyString_FromStringAndSize((char *)NULL, pixcount*sizeof(long));
	if (unpacked == NULL)
		return NULL;
	parray = (unsigned long *) PyString_AsString(unpacked);
	p = (unsigned char *) PyString_AsString(packed);
	if (packfactor == 1 && width*height > 0) {
		/* Just expand bytes to longs */
		register int x = width * height;
		do {
			*parray++ = unpacktab[*p++];
		} while (--x >= 0);
	}
	else {
		register int y;
		for (y = 0; y < height-packfactor+1;
		     y += packfactor, parray += packfactor*width) {
			register int x;
			for (x = 0; x < width-packfactor+1; x += packfactor) {
				register unsigned long pixel = unpacktab[*p++];
				register int i;
				for (i = packfactor*width; (i-=width) >= 0;) {
					register int j;
					for (j = packfactor; --j >= 0; )
						parray[i+x+j] = pixel;
				}
			}
		}
	}
	return unpacked;
}

static PyObject *
gl_gversion(PyObject *self, PyObject *args)
{
	char buf[20];
	gversion(buf);
	return PyString_FromString(buf);
}


/* void clear - Manual because of clash with termcap */
static PyObject *
gl_clear(PyObject *self, PyObject *args)
{
	__GLclear( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* End of manually written stubs */


/* long getshade */

static PyObject *
gl_getshade(PyObject *self, PyObject *args)
{
	long retval;
	retval = getshade( );
	return mknewlongobject(retval);
}

/* void devport short s long s */

static PyObject *
gl_devport(PyObject *self, PyObject *args)
{
	short arg1 ;
	long arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	devport( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdr2i long s long s */

static PyObject *
gl_rdr2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	rdr2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rectfs short s short s short s short s */

static PyObject *
gl_rectfs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	rectfs( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rects short s short s short s short s */

static PyObject *
gl_rects(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	rects( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmv2i long s long s */

static PyObject *
gl_rmv2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	rmv2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void noport */

static PyObject *
gl_noport(PyObject *self, PyObject *args)
{
	noport( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void popviewport */

static PyObject *
gl_popviewport(PyObject *self, PyObject *args)
{
	popviewport( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void clearhitcode */

static PyObject *
gl_clearhitcode(PyObject *self, PyObject *args)
{
	clearhitcode( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void closeobj */

static PyObject *
gl_closeobj(PyObject *self, PyObject *args)
{
	closeobj( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cursoff */

static PyObject *
gl_cursoff(PyObject *self, PyObject *args)
{
	cursoff( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curson */

static PyObject *
gl_curson(PyObject *self, PyObject *args)
{
	curson( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void doublebuffer */

static PyObject *
gl_doublebuffer(PyObject *self, PyObject *args)
{
	doublebuffer( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void finish */

static PyObject *
gl_finish(PyObject *self, PyObject *args)
{
	finish( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gconfig */

static PyObject *
gl_gconfig(PyObject *self, PyObject *args)
{
	gconfig( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void ginit */

static PyObject *
gl_ginit(PyObject *self, PyObject *args)
{
	ginit( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void greset */

static PyObject *
gl_greset(PyObject *self, PyObject *args)
{
	greset( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void multimap */

static PyObject *
gl_multimap(PyObject *self, PyObject *args)
{
	multimap( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void onemap */

static PyObject *
gl_onemap(PyObject *self, PyObject *args)
{
	onemap( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void popattributes */

static PyObject *
gl_popattributes(PyObject *self, PyObject *args)
{
	popattributes( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void popmatrix */

static PyObject *
gl_popmatrix(PyObject *self, PyObject *args)
{
	popmatrix( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pushattributes */

static PyObject *
gl_pushattributes(PyObject *self, PyObject *args)
{
	pushattributes( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pushmatrix */

static PyObject *
gl_pushmatrix(PyObject *self, PyObject *args)
{
	pushmatrix( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pushviewport */

static PyObject *
gl_pushviewport(PyObject *self, PyObject *args)
{
	pushviewport( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void qreset */

static PyObject *
gl_qreset(PyObject *self, PyObject *args)
{
	qreset( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void RGBmode */

static PyObject *
gl_RGBmode(PyObject *self, PyObject *args)
{
	RGBmode( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void singlebuffer */

static PyObject *
gl_singlebuffer(PyObject *self, PyObject *args)
{
	singlebuffer( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void swapbuffers */

static PyObject *
gl_swapbuffers(PyObject *self, PyObject *args)
{
	swapbuffers( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gsync */

static PyObject *
gl_gsync(PyObject *self, PyObject *args)
{
	gsync( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gflush */

static PyObject *
gl_gflush(PyObject *self, PyObject *args)
{
	gflush( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void tpon */

static PyObject *
gl_tpon(PyObject *self, PyObject *args)
{
	tpon( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void tpoff */

static PyObject *
gl_tpoff(PyObject *self, PyObject *args)
{
	tpoff( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void clkon */

static PyObject *
gl_clkon(PyObject *self, PyObject *args)
{
	clkon( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void clkoff */

static PyObject *
gl_clkoff(PyObject *self, PyObject *args)
{
	clkoff( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void ringbell */

static PyObject *
gl_ringbell(PyObject *self, PyObject *args)
{
	ringbell( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gbegin */

static PyObject *
gl_gbegin(PyObject *self, PyObject *args)
{
	gbegin( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void textinit */

static PyObject *
gl_textinit(PyObject *self, PyObject *args)
{
	textinit( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void initnames */

static PyObject *
gl_initnames(PyObject *self, PyObject *args)
{
	initnames( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pclos */

static PyObject *
gl_pclos(PyObject *self, PyObject *args)
{
	pclos( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void popname */

static PyObject *
gl_popname(PyObject *self, PyObject *args)
{
	popname( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void spclos */

static PyObject *
gl_spclos(PyObject *self, PyObject *args)
{
	spclos( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zclear */

static PyObject *
gl_zclear(PyObject *self, PyObject *args)
{
	zclear( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void screenspace */

static PyObject *
gl_screenspace(PyObject *self, PyObject *args)
{
	screenspace( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void reshapeviewport */

static PyObject *
gl_reshapeviewport(PyObject *self, PyObject *args)
{
	reshapeviewport( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void winpush */

static PyObject *
gl_winpush(PyObject *self, PyObject *args)
{
	winpush( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void winpop */

static PyObject *
gl_winpop(PyObject *self, PyObject *args)
{
	winpop( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void foreground */

static PyObject *
gl_foreground(PyObject *self, PyObject *args)
{
	foreground( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endfullscrn */

static PyObject *
gl_endfullscrn(PyObject *self, PyObject *args)
{
	endfullscrn( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endpupmode */

static PyObject *
gl_endpupmode(PyObject *self, PyObject *args)
{
	endpupmode( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void fullscrn */

static PyObject *
gl_fullscrn(PyObject *self, PyObject *args)
{
	fullscrn( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pupmode */

static PyObject *
gl_pupmode(PyObject *self, PyObject *args)
{
	pupmode( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void winconstraints */

static PyObject *
gl_winconstraints(PyObject *self, PyObject *args)
{
	winconstraints( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pagecolor short s */

static PyObject *
gl_pagecolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	pagecolor( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void textcolor short s */

static PyObject *
gl_textcolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	textcolor( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void color short s */

static PyObject *
gl_color(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	color( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curveit short s */

static PyObject *
gl_curveit(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	curveit( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void font short s */

static PyObject *
gl_font(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	font( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void linewidth short s */

static PyObject *
gl_linewidth(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	linewidth( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setlinestyle short s */

static PyObject *
gl_setlinestyle(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	setlinestyle( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setmap short s */

static PyObject *
gl_setmap(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	setmap( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void swapinterval short s */

static PyObject *
gl_swapinterval(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	swapinterval( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void writemask short s */

static PyObject *
gl_writemask(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	writemask( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void textwritemask short s */

static PyObject *
gl_textwritemask(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	textwritemask( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void qdevice short s */

static PyObject *
gl_qdevice(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	qdevice( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void unqdevice short s */

static PyObject *
gl_unqdevice(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	unqdevice( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curvebasis short s */

static PyObject *
gl_curvebasis(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	curvebasis( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curveprecision short s */

static PyObject *
gl_curveprecision(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	curveprecision( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void loadname short s */

static PyObject *
gl_loadname(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	loadname( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void passthrough short s */

static PyObject *
gl_passthrough(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	passthrough( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pushname short s */

static PyObject *
gl_pushname(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	pushname( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setmonitor short s */

static PyObject *
gl_setmonitor(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	setmonitor( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setshade short s */

static PyObject *
gl_setshade(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	setshade( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setpattern short s */

static PyObject *
gl_setpattern(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	setpattern( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pagewritemask short s */

static PyObject *
gl_pagewritemask(PyObject *self, PyObject *args)
{
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	pagewritemask( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void callobj long s */

static PyObject *
gl_callobj(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	callobj( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void delobj long s */

static PyObject *
gl_delobj(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	delobj( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void editobj long s */

static PyObject *
gl_editobj(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	editobj( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void makeobj long s */

static PyObject *
gl_makeobj(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	makeobj( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void maketag long s */

static PyObject *
gl_maketag(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	maketag( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void chunksize long s */

static PyObject *
gl_chunksize(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	chunksize( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void compactify long s */

static PyObject *
gl_compactify(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	compactify( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void deltag long s */

static PyObject *
gl_deltag(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	deltag( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lsrepeat long s */

static PyObject *
gl_lsrepeat(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	lsrepeat( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void objinsert long s */

static PyObject *
gl_objinsert(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	objinsert( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void objreplace long s */

static PyObject *
gl_objreplace(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	objreplace( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void winclose long s */

static PyObject *
gl_winclose(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	winclose( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void blanktime long s */

static PyObject *
gl_blanktime(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	blanktime( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void freepup long s */

static PyObject *
gl_freepup(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	freepup( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void backbuffer long s */

static PyObject *
gl_backbuffer(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	backbuffer( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void frontbuffer long s */

static PyObject *
gl_frontbuffer(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	frontbuffer( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lsbackup long s */

static PyObject *
gl_lsbackup(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	lsbackup( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void resetls long s */

static PyObject *
gl_resetls(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	resetls( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lampon long s */

static PyObject *
gl_lampon(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	lampon( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lampoff long s */

static PyObject *
gl_lampoff(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	lampoff( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setbell long s */

static PyObject *
gl_setbell(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	setbell( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void blankscreen long s */

static PyObject *
gl_blankscreen(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	blankscreen( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void depthcue long s */

static PyObject *
gl_depthcue(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	depthcue( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zbuffer long s */

static PyObject *
gl_zbuffer(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	zbuffer( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void backface long s */

static PyObject *
gl_backface(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	backface( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmov2i long s long s */

static PyObject *
gl_cmov2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	cmov2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void draw2i long s long s */

static PyObject *
gl_draw2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	draw2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void move2i long s long s */

static PyObject *
gl_move2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	move2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnt2i long s long s */

static PyObject *
gl_pnt2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	pnt2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void patchbasis long s long s */

static PyObject *
gl_patchbasis(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	patchbasis( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void patchprecision long s long s */

static PyObject *
gl_patchprecision(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	patchprecision( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdr2i long s long s */

static PyObject *
gl_pdr2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	pdr2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmv2i long s long s */

static PyObject *
gl_pmv2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	pmv2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdr2i long s long s */

static PyObject *
gl_rpdr2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	rpdr2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmv2i long s long s */

static PyObject *
gl_rpmv2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	rpmv2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt2i long s long s */

static PyObject *
gl_xfpt2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	xfpt2i( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void objdelete long s long s */

static PyObject *
gl_objdelete(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	objdelete( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void patchcurves long s long s */

static PyObject *
gl_patchcurves(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	patchcurves( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void minsize long s long s */

static PyObject *
gl_minsize(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	minsize( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void maxsize long s long s */

static PyObject *
gl_maxsize(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	maxsize( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void keepaspect long s long s */

static PyObject *
gl_keepaspect(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	keepaspect( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void prefsize long s long s */

static PyObject *
gl_prefsize(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	prefsize( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void stepunit long s long s */

static PyObject *
gl_stepunit(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	stepunit( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void fudge long s long s */

static PyObject *
gl_fudge(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	fudge( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void winmove long s long s */

static PyObject *
gl_winmove(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	winmove( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void attachcursor short s short s */

static PyObject *
gl_attachcursor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	attachcursor( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void deflinestyle short s short s */

static PyObject *
gl_deflinestyle(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	deflinestyle( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void noise short s short s */

static PyObject *
gl_noise(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	noise( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void picksize short s short s */

static PyObject *
gl_picksize(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	picksize( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void qenter short s short s */

static PyObject *
gl_qenter(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	qenter( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setdepth short s short s */

static PyObject *
gl_setdepth(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	setdepth( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmov2s short s short s */

static PyObject *
gl_cmov2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	cmov2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void draw2s short s short s */

static PyObject *
gl_draw2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	draw2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void move2s short s short s */

static PyObject *
gl_move2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	move2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdr2s short s short s */

static PyObject *
gl_pdr2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	pdr2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmv2s short s short s */

static PyObject *
gl_pmv2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	pmv2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnt2s short s short s */

static PyObject *
gl_pnt2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	pnt2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdr2s short s short s */

static PyObject *
gl_rdr2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	rdr2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmv2s short s short s */

static PyObject *
gl_rmv2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	rmv2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdr2s short s short s */

static PyObject *
gl_rpdr2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	rpdr2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmv2s short s short s */

static PyObject *
gl_rpmv2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	rpmv2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt2s short s short s */

static PyObject *
gl_xfpt2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	xfpt2s( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmov2 float s float s */

static PyObject *
gl_cmov2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	cmov2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void draw2 float s float s */

static PyObject *
gl_draw2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	draw2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void move2 float s float s */

static PyObject *
gl_move2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	move2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnt2 float s float s */

static PyObject *
gl_pnt2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	pnt2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdr2 float s float s */

static PyObject *
gl_pdr2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	pdr2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmv2 float s float s */

static PyObject *
gl_pmv2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	pmv2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdr2 float s float s */

static PyObject *
gl_rdr2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	rdr2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmv2 float s float s */

static PyObject *
gl_rmv2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	rmv2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdr2 float s float s */

static PyObject *
gl_rpdr2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	rpdr2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmv2 float s float s */

static PyObject *
gl_rpmv2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	rpmv2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt2 float s float s */

static PyObject *
gl_xfpt2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	xfpt2( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void loadmatrix float s[4*4] */

static PyObject *
gl_loadmatrix(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 4 ] ;
	if (!getifloatarray(args, 1, 0, 4 * 4 , (float *) arg1))
		return NULL;
	loadmatrix( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void multmatrix float s[4*4] */

static PyObject *
gl_multmatrix(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 4 ] ;
	if (!getifloatarray(args, 1, 0, 4 * 4 , (float *) arg1))
		return NULL;
	multmatrix( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void crv float s[3*4] */

static PyObject *
gl_crv(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 3 ] ;
	if (!getifloatarray(args, 1, 0, 3 * 4 , (float *) arg1))
		return NULL;
	crv( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rcrv float s[4*4] */

static PyObject *
gl_rcrv(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 4 ] ;
	if (!getifloatarray(args, 1, 0, 4 * 4 , (float *) arg1))
		return NULL;
	rcrv( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void addtopup long s char *s long s */

static PyObject *
gl_addtopup(PyObject *self, PyObject *args)
{
	long arg1 ;
	string arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getistringarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	addtopup( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void charstr char *s */

static PyObject *
gl_charstr(PyObject *self, PyObject *args)
{
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	charstr( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void getport char *s */

static PyObject *
gl_getport(PyObject *self, PyObject *args)
{
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	getport( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long strwidth char *s */

static PyObject *
gl_strwidth(PyObject *self, PyObject *args)
{
	long retval;
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	retval = strwidth( arg1 );
	return mknewlongobject(retval);
}

/* long winopen char *s */

static PyObject *
gl_winopen(PyObject *self, PyObject *args)
{
	long retval;
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	retval = winopen( arg1 );
	return mknewlongobject(retval);
}

/* void wintitle char *s */

static PyObject *
gl_wintitle(PyObject *self, PyObject *args)
{
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	wintitle( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polf long s float s[3*arg1] */

static PyObject *
gl_polf(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (float(*)[3]) PyMem_NEW(float , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 3 * arg1 , (float *) arg2))
		return NULL;
	polf( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polf2 long s float s[2*arg1] */

static PyObject *
gl_polf2(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (float(*)[2]) PyMem_NEW(float , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 2 * arg1 , (float *) arg2))
		return NULL;
	polf2( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void poly long s float s[3*arg1] */

static PyObject *
gl_poly(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (float(*)[3]) PyMem_NEW(float , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 3 * arg1 , (float *) arg2))
		return NULL;
	poly( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void poly2 long s float s[2*arg1] */

static PyObject *
gl_poly2(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (float(*)[2]) PyMem_NEW(float , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 2 * arg1 , (float *) arg2))
		return NULL;
	poly2( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void crvn long s float s[3*arg1] */

static PyObject *
gl_crvn(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (float(*)[3]) PyMem_NEW(float , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 3 * arg1 , (float *) arg2))
		return NULL;
	crvn( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rcrvn long s float s[4*arg1] */

static PyObject *
gl_rcrvn(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 4 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 4;
	if ((arg2 = (float(*)[4]) PyMem_NEW(float , 4 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 1, 0, 4 * arg1 , (float *) arg2))
		return NULL;
	rcrvn( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polf2i long s long s[2*arg1] */

static PyObject *
gl_polf2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (long(*)[2]) PyMem_NEW(long , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 1, 0, 2 * arg1 , (long *) arg2))
		return NULL;
	polf2i( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polfi long s long s[3*arg1] */

static PyObject *
gl_polfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (long(*)[3]) PyMem_NEW(long , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 1, 0, 3 * arg1 , (long *) arg2))
		return NULL;
	polfi( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void poly2i long s long s[2*arg1] */

static PyObject *
gl_poly2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (long(*)[2]) PyMem_NEW(long , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 1, 0, 2 * arg1 , (long *) arg2))
		return NULL;
	poly2i( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polyi long s long s[3*arg1] */

static PyObject *
gl_polyi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (long(*)[3]) PyMem_NEW(long , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 1, 0, 3 * arg1 , (long *) arg2))
		return NULL;
	polyi( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polf2s long s short s[2*arg1] */

static PyObject *
gl_polf2s(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (short(*)[2]) PyMem_NEW(short , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, 2 * arg1 , (short *) arg2))
		return NULL;
	polf2s( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polfs long s short s[3*arg1] */

static PyObject *
gl_polfs(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (short(*)[3]) PyMem_NEW(short , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, 3 * arg1 , (short *) arg2))
		return NULL;
	polfs( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polys long s short s[3*arg1] */

static PyObject *
gl_polys(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 3 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (short(*)[3]) PyMem_NEW(short , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, 3 * arg1 , (short *) arg2))
		return NULL;
	polys( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void poly2s long s short s[2*arg1] */

static PyObject *
gl_poly2s(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 2 ] ;
	if (!getilongarraysize(args, 1, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (short(*)[2]) PyMem_NEW(short , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, 2 * arg1 , (short *) arg2))
		return NULL;
	poly2s( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void defcursor short s u_short s[128] */

static PyObject *
gl_defcursor(PyObject *self, PyObject *args)
{
	short arg1 ;
	unsigned short arg2 [ 128 ] ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarray(args, 2, 1, 128 , (short *) arg2))
		return NULL;
	defcursor( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void writepixels short s u_short s[arg1] */

static PyObject *
gl_writepixels(PyObject *self, PyObject *args)
{
	short arg1 ;
	unsigned short * arg2 ;
	if (!getishortarraysize(args, 1, 0, &arg1))
		return NULL;
	if ((arg2 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, arg1 , (short *) arg2))
		return NULL;
	writepixels( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void defbasis long s float s[4*4] */

static PyObject *
gl_defbasis(PyObject *self, PyObject *args)
{
	long arg1 ;
	float arg2 [ 4 ] [ 4 ] ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarray(args, 2, 1, 4 * 4 , (float *) arg2))
		return NULL;
	defbasis( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gewrite short s short s[arg1] */

static PyObject *
gl_gewrite(PyObject *self, PyObject *args)
{
	short arg1 ;
	short * arg2 ;
	if (!getishortarraysize(args, 1, 0, &arg1))
		return NULL;
	if ((arg2 = PyMem_NEW(short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 1, 0, arg1 , arg2))
		return NULL;
	gewrite( arg1 , arg2 );
	PyMem_DEL(arg2);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rotate short s char s */

static PyObject *
gl_rotate(PyObject *self, PyObject *args)
{
	short arg1 ;
	char arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getichararg(args, 2, 1, &arg2))
		return NULL;
	rotate( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rot float s char s */

static PyObject *
gl_rot(PyObject *self, PyObject *args)
{
	float arg1 ;
	char arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getichararg(args, 2, 1, &arg2))
		return NULL;
	rot( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circfi long s long s long s */

static PyObject *
gl_circfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	circfi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circi long s long s long s */

static PyObject *
gl_circi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	circi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmovi long s long s long s */

static PyObject *
gl_cmovi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	cmovi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void drawi long s long s long s */

static PyObject *
gl_drawi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	drawi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void movei long s long s long s */

static PyObject *
gl_movei(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	movei( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnti long s long s long s */

static PyObject *
gl_pnti(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	pnti( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void newtag long s long s long s */

static PyObject *
gl_newtag(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	newtag( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdri long s long s long s */

static PyObject *
gl_pdri(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	pdri( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmvi long s long s long s */

static PyObject *
gl_pmvi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	pmvi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdri long s long s long s */

static PyObject *
gl_rdri(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	rdri( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmvi long s long s long s */

static PyObject *
gl_rmvi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	rmvi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdri long s long s long s */

static PyObject *
gl_rpdri(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	rpdri( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmvi long s long s long s */

static PyObject *
gl_rpmvi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	rpmvi( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpti long s long s long s */

static PyObject *
gl_xfpti(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	xfpti( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circ float s float s float s */

static PyObject *
gl_circ(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	circ( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circf float s float s float s */

static PyObject *
gl_circf(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	circf( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmov float s float s float s */

static PyObject *
gl_cmov(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	cmov( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void draw float s float s float s */

static PyObject *
gl_draw(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	draw( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void move float s float s float s */

static PyObject *
gl_move(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	move( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnt float s float s float s */

static PyObject *
gl_pnt(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	pnt( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void scale float s float s float s */

static PyObject *
gl_scale(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	scale( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void translate float s float s float s */

static PyObject *
gl_translate(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	translate( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdr float s float s float s */

static PyObject *
gl_pdr(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	pdr( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmv float s float s float s */

static PyObject *
gl_pmv(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	pmv( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdr float s float s float s */

static PyObject *
gl_rdr(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	rdr( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmv float s float s float s */

static PyObject *
gl_rmv(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	rmv( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdr float s float s float s */

static PyObject *
gl_rpdr(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	rpdr( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmv float s float s float s */

static PyObject *
gl_rpmv(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	rpmv( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt float s float s float s */

static PyObject *
gl_xfpt(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	if (!getifloatarg(args, 3, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 3, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 3, 2, &arg3))
		return NULL;
	xfpt( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void RGBcolor short s short s short s */

static PyObject *
gl_RGBcolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	RGBcolor( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void RGBwritemask short s short s short s */

static PyObject *
gl_RGBwritemask(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	RGBwritemask( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setcursor short s short s short s */

static PyObject *
gl_setcursor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	setcursor( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void tie short s short s short s */

static PyObject *
gl_tie(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	tie( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circfs short s short s short s */

static PyObject *
gl_circfs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	circfs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void circs short s short s short s */

static PyObject *
gl_circs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	circs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cmovs short s short s short s */

static PyObject *
gl_cmovs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	cmovs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void draws short s short s short s */

static PyObject *
gl_draws(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	draws( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void moves short s short s short s */

static PyObject *
gl_moves(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	moves( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pdrs short s short s short s */

static PyObject *
gl_pdrs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	pdrs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pmvs short s short s short s */

static PyObject *
gl_pmvs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	pmvs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pnts short s short s short s */

static PyObject *
gl_pnts(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	pnts( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rdrs short s short s short s */

static PyObject *
gl_rdrs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	rdrs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rmvs short s short s short s */

static PyObject *
gl_rmvs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	rmvs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpdrs short s short s short s */

static PyObject *
gl_rpdrs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	rpdrs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpmvs short s short s short s */

static PyObject *
gl_rpmvs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	rpmvs( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpts short s short s short s */

static PyObject *
gl_xfpts(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	xfpts( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curorigin short s short s short s */

static PyObject *
gl_curorigin(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	curorigin( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cyclemap short s short s short s */

static PyObject *
gl_cyclemap(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	if (!getishortarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	cyclemap( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void patch float s[4*4] float s[4*4] float s[4*4] */

static PyObject *
gl_patch(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 4 ] ;
	float arg2 [ 4 ] [ 4 ] ;
	float arg3 [ 4 ] [ 4 ] ;
	if (!getifloatarray(args, 3, 0, 4 * 4 , (float *) arg1))
		return NULL;
	if (!getifloatarray(args, 3, 1, 4 * 4 , (float *) arg2))
		return NULL;
	if (!getifloatarray(args, 3, 2, 4 * 4 , (float *) arg3))
		return NULL;
	patch( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splf long s float s[3*arg1] u_short s[arg1] */

static PyObject *
gl_splf(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 3 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (float(*)[3]) PyMem_NEW(float , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 2, 0, 3 * arg1 , (float *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splf( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splf2 long s float s[2*arg1] u_short s[arg1] */

static PyObject *
gl_splf2(PyObject *self, PyObject *args)
{
	long arg1 ;
	float (* arg2) [ 2 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (float(*)[2]) PyMem_NEW(float , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 2, 0, 2 * arg1 , (float *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splf2( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splfi long s long s[3*arg1] u_short s[arg1] */

static PyObject *
gl_splfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 3 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (long(*)[3]) PyMem_NEW(long , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 2, 0, 3 * arg1 , (long *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splfi( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splf2i long s long s[2*arg1] u_short s[arg1] */

static PyObject *
gl_splf2i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long (* arg2) [ 2 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (long(*)[2]) PyMem_NEW(long , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getilongarray(args, 2, 0, 2 * arg1 , (long *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splf2i( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splfs long s short s[3*arg1] u_short s[arg1] */

static PyObject *
gl_splfs(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 3 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 3;
	if ((arg2 = (short(*)[3]) PyMem_NEW(short , 3 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 0, 3 * arg1 , (short *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splfs( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void splf2s long s short s[2*arg1] u_short s[arg1] */

static PyObject *
gl_splf2s(PyObject *self, PyObject *args)
{
	long arg1 ;
	short (* arg2) [ 2 ] ;
	unsigned short * arg3 ;
	if (!getilongarraysize(args, 2, 0, &arg1))
		return NULL;
	arg1 = arg1 / 2;
	if ((arg2 = (short(*)[2]) PyMem_NEW(short , 2 * arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 0, 2 * arg1 , (short *) arg2))
		return NULL;
	if ((arg3 = PyMem_NEW(unsigned short , arg1 )) == NULL)
		return PyErr_NoMemory();
	if (!getishortarray(args, 2, 1, arg1 , (short *) arg3))
		return NULL;
	splf2s( arg1 , arg2 , arg3 );
	PyMem_DEL(arg2);
	PyMem_DEL(arg3);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rpatch float s[4*4] float s[4*4] float s[4*4] float s[4*4] */

static PyObject *
gl_rpatch(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] [ 4 ] ;
	float arg2 [ 4 ] [ 4 ] ;
	float arg3 [ 4 ] [ 4 ] ;
	float arg4 [ 4 ] [ 4 ] ;
	if (!getifloatarray(args, 4, 0, 4 * 4 , (float *) arg1))
		return NULL;
	if (!getifloatarray(args, 4, 1, 4 * 4 , (float *) arg2))
		return NULL;
	if (!getifloatarray(args, 4, 2, 4 * 4 , (float *) arg3))
		return NULL;
	if (!getifloatarray(args, 4, 3, 4 * 4 , (float *) arg4))
		return NULL;
	rpatch( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void ortho2 float s float s float s float s */

static PyObject *
gl_ortho2(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	ortho2( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rect float s float s float s float s */

static PyObject *
gl_rect(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	rect( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rectf float s float s float s float s */

static PyObject *
gl_rectf(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	rectf( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt4 float s float s float s float s */

static PyObject *
gl_xfpt4(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	xfpt4( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void textport short s short s short s short s */

static PyObject *
gl_textport(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	textport( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void mapcolor short s short s short s short s */

static PyObject *
gl_mapcolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	mapcolor( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void scrmask short s short s short s short s */

static PyObject *
gl_scrmask(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	scrmask( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setvaluator short s short s short s short s */

static PyObject *
gl_setvaluator(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	setvaluator( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void viewport short s short s short s short s */

static PyObject *
gl_viewport(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	viewport( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void shaderange short s short s short s short s */

static PyObject *
gl_shaderange(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	shaderange( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt4s short s short s short s short s */

static PyObject *
gl_xfpt4s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	xfpt4s( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rectfi long s long s long s long s */

static PyObject *
gl_rectfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	rectfi( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void recti long s long s long s long s */

static PyObject *
gl_recti(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	recti( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void xfpt4i long s long s long s long s */

static PyObject *
gl_xfpt4i(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	xfpt4i( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void prefposition long s long s long s long s */

static PyObject *
gl_prefposition(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	prefposition( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arc float s float s float s short s short s */

static PyObject *
gl_arc(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getifloatarg(args, 5, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 5, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arc( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arcf float s float s float s short s short s */

static PyObject *
gl_arcf(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getifloatarg(args, 5, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 5, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arcf( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arcfi long s long s long s short s short s */

static PyObject *
gl_arcfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getilongarg(args, 5, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 5, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arcfi( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arci long s long s long s short s short s */

static PyObject *
gl_arci(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getilongarg(args, 5, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 5, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arci( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bbox2 short s short s float s float s float s float s */

static PyObject *
gl_bbox2(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	float arg3 ;
	float arg4 ;
	float arg5 ;
	float arg6 ;
	if (!getishortarg(args, 6, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 6, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 6, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 6, 3, &arg4))
		return NULL;
	if (!getifloatarg(args, 6, 4, &arg5))
		return NULL;
	if (!getifloatarg(args, 6, 5, &arg6))
		return NULL;
	bbox2( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bbox2i short s short s long s long s long s long s */

static PyObject *
gl_bbox2i(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	long arg3 ;
	long arg4 ;
	long arg5 ;
	long arg6 ;
	if (!getishortarg(args, 6, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 6, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 6, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 6, 3, &arg4))
		return NULL;
	if (!getilongarg(args, 6, 4, &arg5))
		return NULL;
	if (!getilongarg(args, 6, 5, &arg6))
		return NULL;
	bbox2i( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bbox2s short s short s short s short s short s short s */

static PyObject *
gl_bbox2s(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	short arg6 ;
	if (!getishortarg(args, 6, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 6, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 6, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 6, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 6, 4, &arg5))
		return NULL;
	if (!getishortarg(args, 6, 5, &arg6))
		return NULL;
	bbox2s( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void blink short s short s short s short s short s */

static PyObject *
gl_blink(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getishortarg(args, 5, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 5, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	blink( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void ortho float s float s float s float s float s float s */

static PyObject *
gl_ortho(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	float arg5 ;
	float arg6 ;
	if (!getifloatarg(args, 6, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 6, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 6, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 6, 3, &arg4))
		return NULL;
	if (!getifloatarg(args, 6, 4, &arg5))
		return NULL;
	if (!getifloatarg(args, 6, 5, &arg6))
		return NULL;
	ortho( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void window float s float s float s float s float s float s */

static PyObject *
gl_window(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	float arg5 ;
	float arg6 ;
	if (!getifloatarg(args, 6, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 6, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 6, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 6, 3, &arg4))
		return NULL;
	if (!getifloatarg(args, 6, 4, &arg5))
		return NULL;
	if (!getifloatarg(args, 6, 5, &arg6))
		return NULL;
	window( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lookat float s float s float s float s float s float s short s */

static PyObject *
gl_lookat(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	float arg5 ;
	float arg6 ;
	short arg7 ;
	if (!getifloatarg(args, 7, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 7, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 7, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 7, 3, &arg4))
		return NULL;
	if (!getifloatarg(args, 7, 4, &arg5))
		return NULL;
	if (!getifloatarg(args, 7, 5, &arg6))
		return NULL;
	if (!getishortarg(args, 7, 6, &arg7))
		return NULL;
	lookat( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void perspective short s float s float s float s */

static PyObject *
gl_perspective(PyObject *self, PyObject *args)
{
	short arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	perspective( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void polarview float s short s short s short s */

static PyObject *
gl_polarview(PyObject *self, PyObject *args)
{
	float arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	polarview( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arcfs short s short s short s short s short s */

static PyObject *
gl_arcfs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getishortarg(args, 5, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 5, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arcfs( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void arcs short s short s short s short s short s */

static PyObject *
gl_arcs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	if (!getishortarg(args, 5, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 5, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 5, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 5, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 5, 4, &arg5))
		return NULL;
	arcs( arg1 , arg2 , arg3 , arg4 , arg5 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rectcopy short s short s short s short s short s short s */

static PyObject *
gl_rectcopy(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	short arg6 ;
	if (!getishortarg(args, 6, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 6, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 6, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 6, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 6, 4, &arg5))
		return NULL;
	if (!getishortarg(args, 6, 5, &arg6))
		return NULL;
	rectcopy( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void RGBcursor short s short s short s short s short s short s short s */

static PyObject *
gl_RGBcursor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	short arg6 ;
	short arg7 ;
	if (!getishortarg(args, 7, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 7, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 7, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 7, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 7, 4, &arg5))
		return NULL;
	if (!getishortarg(args, 7, 5, &arg6))
		return NULL;
	if (!getishortarg(args, 7, 6, &arg7))
		return NULL;
	RGBcursor( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long getbutton short s */

static PyObject *
gl_getbutton(PyObject *self, PyObject *args)
{
	long retval;
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	retval = getbutton( arg1 );
	return mknewlongobject(retval);
}

/* long getcmmode */

static PyObject *
gl_getcmmode(PyObject *self, PyObject *args)
{
	long retval;
	retval = getcmmode( );
	return mknewlongobject(retval);
}

/* long getlsbackup */

static PyObject *
gl_getlsbackup(PyObject *self, PyObject *args)
{
	long retval;
	retval = getlsbackup( );
	return mknewlongobject(retval);
}

/* long getresetls */

static PyObject *
gl_getresetls(PyObject *self, PyObject *args)
{
	long retval;
	retval = getresetls( );
	return mknewlongobject(retval);
}

/* long getdcm */

static PyObject *
gl_getdcm(PyObject *self, PyObject *args)
{
	long retval;
	retval = getdcm( );
	return mknewlongobject(retval);
}

/* long getzbuffer */

static PyObject *
gl_getzbuffer(PyObject *self, PyObject *args)
{
	long retval;
	retval = getzbuffer( );
	return mknewlongobject(retval);
}

/* long ismex */

static PyObject *
gl_ismex(PyObject *self, PyObject *args)
{
	long retval;
	retval = ismex( );
	return mknewlongobject(retval);
}

/* long isobj long s */

static PyObject *
gl_isobj(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = isobj( arg1 );
	return mknewlongobject(retval);
}

/* long isqueued short s */

static PyObject *
gl_isqueued(PyObject *self, PyObject *args)
{
	long retval;
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	retval = isqueued( arg1 );
	return mknewlongobject(retval);
}

/* long istag long s */

static PyObject *
gl_istag(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = istag( arg1 );
	return mknewlongobject(retval);
}

/* long genobj */

static PyObject *
gl_genobj(PyObject *self, PyObject *args)
{
	long retval;
	retval = genobj( );
	return mknewlongobject(retval);
}

/* long gentag */

static PyObject *
gl_gentag(PyObject *self, PyObject *args)
{
	long retval;
	retval = gentag( );
	return mknewlongobject(retval);
}

/* long getbuffer */

static PyObject *
gl_getbuffer(PyObject *self, PyObject *args)
{
	long retval;
	retval = getbuffer( );
	return mknewlongobject(retval);
}

/* long getcolor */

static PyObject *
gl_getcolor(PyObject *self, PyObject *args)
{
	long retval;
	retval = getcolor( );
	return mknewlongobject(retval);
}

/* long getdisplaymode */

static PyObject *
gl_getdisplaymode(PyObject *self, PyObject *args)
{
	long retval;
	retval = getdisplaymode( );
	return mknewlongobject(retval);
}

/* long getfont */

static PyObject *
gl_getfont(PyObject *self, PyObject *args)
{
	long retval;
	retval = getfont( );
	return mknewlongobject(retval);
}

/* long getheight */

static PyObject *
gl_getheight(PyObject *self, PyObject *args)
{
	long retval;
	retval = getheight( );
	return mknewlongobject(retval);
}

/* long gethitcode */

static PyObject *
gl_gethitcode(PyObject *self, PyObject *args)
{
	long retval;
	retval = gethitcode( );
	return mknewlongobject(retval);
}

/* long getlstyle */

static PyObject *
gl_getlstyle(PyObject *self, PyObject *args)
{
	long retval;
	retval = getlstyle( );
	return mknewlongobject(retval);
}

/* long getlwidth */

static PyObject *
gl_getlwidth(PyObject *self, PyObject *args)
{
	long retval;
	retval = getlwidth( );
	return mknewlongobject(retval);
}

/* long getmap */

static PyObject *
gl_getmap(PyObject *self, PyObject *args)
{
	long retval;
	retval = getmap( );
	return mknewlongobject(retval);
}

/* long getplanes */

static PyObject *
gl_getplanes(PyObject *self, PyObject *args)
{
	long retval;
	retval = getplanes( );
	return mknewlongobject(retval);
}

/* long getwritemask */

static PyObject *
gl_getwritemask(PyObject *self, PyObject *args)
{
	long retval;
	retval = getwritemask( );
	return mknewlongobject(retval);
}

/* long qtest */

static PyObject *
gl_qtest(PyObject *self, PyObject *args)
{
	long retval;
	retval = qtest( );
	return mknewlongobject(retval);
}

/* long getlsrepeat */

static PyObject *
gl_getlsrepeat(PyObject *self, PyObject *args)
{
	long retval;
	retval = getlsrepeat( );
	return mknewlongobject(retval);
}

/* long getmonitor */

static PyObject *
gl_getmonitor(PyObject *self, PyObject *args)
{
	long retval;
	retval = getmonitor( );
	return mknewlongobject(retval);
}

/* long getopenobj */

static PyObject *
gl_getopenobj(PyObject *self, PyObject *args)
{
	long retval;
	retval = getopenobj( );
	return mknewlongobject(retval);
}

/* long getpattern */

static PyObject *
gl_getpattern(PyObject *self, PyObject *args)
{
	long retval;
	retval = getpattern( );
	return mknewlongobject(retval);
}

/* long winget */

static PyObject *
gl_winget(PyObject *self, PyObject *args)
{
	long retval;
	retval = winget( );
	return mknewlongobject(retval);
}

/* long winattach */

static PyObject *
gl_winattach(PyObject *self, PyObject *args)
{
	long retval;
	retval = winattach( );
	return mknewlongobject(retval);
}

/* long getothermonitor */

static PyObject *
gl_getothermonitor(PyObject *self, PyObject *args)
{
	long retval;
	retval = getothermonitor( );
	return mknewlongobject(retval);
}

/* long newpup */

static PyObject *
gl_newpup(PyObject *self, PyObject *args)
{
	long retval;
	retval = newpup( );
	return mknewlongobject(retval);
}

/* long getvaluator short s */

static PyObject *
gl_getvaluator(PyObject *self, PyObject *args)
{
	long retval;
	short arg1 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	retval = getvaluator( arg1 );
	return mknewlongobject(retval);
}

/* void winset long s */

static PyObject *
gl_winset(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	winset( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long dopup long s */

static PyObject *
gl_dopup(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = dopup( arg1 );
	return mknewlongobject(retval);
}

/* void getdepth short r short r */

static PyObject *
gl_getdepth(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	getdepth( & arg1 , & arg2 );
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  return v;
	}
}

/* void getcpos short r short r */

static PyObject *
gl_getcpos(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	getcpos( & arg1 , & arg2 );
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  return v;
	}
}

/* void getsize long r long r */

static PyObject *
gl_getsize(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	getsize( & arg1 , & arg2 );
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewlongobject(arg1));
	  PyTuple_SetItem(v, 1, mknewlongobject(arg2));
	  return v;
	}
}

/* void getorigin long r long r */

static PyObject *
gl_getorigin(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	getorigin( & arg1 , & arg2 );
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewlongobject(arg1));
	  PyTuple_SetItem(v, 1, mknewlongobject(arg2));
	  return v;
	}
}

/* void getviewport short r short r short r short r */

static PyObject *
gl_getviewport(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	getviewport( & arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 4 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg3));
	  PyTuple_SetItem(v, 3, mknewshortobject(arg4));
	  return v;
	}
}

/* void gettp short r short r short r short r */

static PyObject *
gl_gettp(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	gettp( & arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 4 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg3));
	  PyTuple_SetItem(v, 3, mknewshortobject(arg4));
	  return v;
	}
}

/* void getgpos float r float r float r float r */

static PyObject *
gl_getgpos(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	getgpos( & arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 4 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewfloatobject(arg1));
	  PyTuple_SetItem(v, 1, mknewfloatobject(arg2));
	  PyTuple_SetItem(v, 2, mknewfloatobject(arg3));
	  PyTuple_SetItem(v, 3, mknewfloatobject(arg4));
	  return v;
	}
}

/* void winposition long s long s long s long s */

static PyObject *
gl_winposition(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	winposition( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gRGBcolor short r short r short r */

static PyObject *
gl_gRGBcolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	gRGBcolor( & arg1 , & arg2 , & arg3 );
	{ PyObject *v = PyTuple_New( 3 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg3));
	  return v;
	}
}

/* void gRGBmask short r short r short r */

static PyObject *
gl_gRGBmask(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	gRGBmask( & arg1 , & arg2 , & arg3 );
	{ PyObject *v = PyTuple_New( 3 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg3));
	  return v;
	}
}

/* void getscrmask short r short r short r short r */

static PyObject *
gl_getscrmask(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	getscrmask( & arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 4 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg3));
	  PyTuple_SetItem(v, 3, mknewshortobject(arg4));
	  return v;
	}
}

/* void getmcolor short s short r short r short r */

static PyObject *
gl_getmcolor(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 1, 0, &arg1))
		return NULL;
	getmcolor( arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 3 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg2));
	  PyTuple_SetItem(v, 1, mknewshortobject(arg3));
	  PyTuple_SetItem(v, 2, mknewshortobject(arg4));
	  return v;
	}
}

/* void mapw long s short s short s float r float r float r float r float r float r */

static PyObject *
gl_mapw(PyObject *self, PyObject *args)
{
	long arg1 ;
	short arg2 ;
	short arg3 ;
	float arg4 ;
	float arg5 ;
	float arg6 ;
	float arg7 ;
	float arg8 ;
	float arg9 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	mapw( arg1 , arg2 , arg3 , & arg4 , & arg5 , & arg6 , & arg7 , & arg8 , & arg9 );
	{ PyObject *v = PyTuple_New( 6 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewfloatobject(arg4));
	  PyTuple_SetItem(v, 1, mknewfloatobject(arg5));
	  PyTuple_SetItem(v, 2, mknewfloatobject(arg6));
	  PyTuple_SetItem(v, 3, mknewfloatobject(arg7));
	  PyTuple_SetItem(v, 4, mknewfloatobject(arg8));
	  PyTuple_SetItem(v, 5, mknewfloatobject(arg9));
	  return v;
	}
}

/* void mapw2 long s short s short s float r float r */

static PyObject *
gl_mapw2(PyObject *self, PyObject *args)
{
	long arg1 ;
	short arg2 ;
	short arg3 ;
	float arg4 ;
	float arg5 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 3, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 3, 2, &arg3))
		return NULL;
	mapw2( arg1 , arg2 , arg3 , & arg4 , & arg5 );
	{ PyObject *v = PyTuple_New( 2 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewfloatobject(arg4));
	  PyTuple_SetItem(v, 1, mknewfloatobject(arg5));
	  return v;
	}
}

/* void getcursor short r u_short r u_short r long r */

static PyObject *
gl_getcursor(PyObject *self, PyObject *args)
{
	short arg1 ;
	unsigned short arg2 ;
	unsigned short arg3 ;
	long arg4 ;
	getcursor( & arg1 , & arg2 , & arg3 , & arg4 );
	{ PyObject *v = PyTuple_New( 4 );
	  if (v == NULL) return NULL;
	  PyTuple_SetItem(v, 0, mknewshortobject(arg1));
	  PyTuple_SetItem(v, 1, mknewshortobject((short) arg2));
	  PyTuple_SetItem(v, 2, mknewshortobject((short) arg3));
	  PyTuple_SetItem(v, 3, mknewlongobject(arg4));
	  return v;
	}
}

/* void cmode */

static PyObject *
gl_cmode(PyObject *self, PyObject *args)
{
	cmode( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void concave long s */

static PyObject *
gl_concave(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	concave( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void curstype long s */

static PyObject *
gl_curstype(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	curstype( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void drawmode long s */

static PyObject *
gl_drawmode(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	drawmode( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void gammaramp short s[256] short s[256] short s[256] */

static PyObject *
gl_gammaramp(PyObject *self, PyObject *args)
{
	short arg1 [ 256 ] ;
	short arg2 [ 256 ] ;
	short arg3 [ 256 ] ;
	if (!getishortarray(args, 3, 0, 256 , arg1))
		return NULL;
	if (!getishortarray(args, 3, 1, 256 , arg2))
		return NULL;
	if (!getishortarray(args, 3, 2, 256 , arg3))
		return NULL;
	gammaramp( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long getbackface */

static PyObject *
gl_getbackface(PyObject *self, PyObject *args)
{
	long retval;
	retval = getbackface( );
	return mknewlongobject(retval);
}

/* long getdescender */

static PyObject *
gl_getdescender(PyObject *self, PyObject *args)
{
	long retval;
	retval = getdescender( );
	return mknewlongobject(retval);
}

/* long getdrawmode */

static PyObject *
gl_getdrawmode(PyObject *self, PyObject *args)
{
	long retval;
	retval = getdrawmode( );
	return mknewlongobject(retval);
}

/* long getmmode */

static PyObject *
gl_getmmode(PyObject *self, PyObject *args)
{
	long retval;
	retval = getmmode( );
	return mknewlongobject(retval);
}

/* long getsm */

static PyObject *
gl_getsm(PyObject *self, PyObject *args)
{
	long retval;
	retval = getsm( );
	return mknewlongobject(retval);
}

/* long getvideo long s */

static PyObject *
gl_getvideo(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = getvideo( arg1 );
	return mknewlongobject(retval);
}

/* void imakebackground */

static PyObject *
gl_imakebackground(PyObject *self, PyObject *args)
{
	imakebackground( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lmbind short s short s */

static PyObject *
gl_lmbind(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	if (!getishortarg(args, 2, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 2, 1, &arg2))
		return NULL;
	lmbind( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lmdef long s long s long s float s[arg3] */

static PyObject *
gl_lmdef(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	float * arg4 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarraysize(args, 3, 2, &arg3))
		return NULL;
	if ((arg4 = PyMem_NEW(float , arg3 )) == NULL)
		return PyErr_NoMemory();
	if (!getifloatarray(args, 3, 2, arg3 , arg4))
		return NULL;
	lmdef( arg1 , arg2 , arg3 , arg4 );
	PyMem_DEL(arg4);
	Py_INCREF(Py_None);
	return Py_None;
}

/* void mmode long s */

static PyObject *
gl_mmode(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	mmode( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void normal float s[3] */

static PyObject *
gl_normal(PyObject *self, PyObject *args)
{
	float arg1 [ 3 ] ;
	if (!getifloatarray(args, 1, 0, 3 , arg1))
		return NULL;
	normal( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void overlay long s */

static PyObject *
gl_overlay(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	overlay( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void RGBrange short s short s short s short s short s short s short s short s */

static PyObject *
gl_RGBrange(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	short arg6 ;
	short arg7 ;
	short arg8 ;
	if (!getishortarg(args, 8, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 8, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 8, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 8, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 8, 4, &arg5))
		return NULL;
	if (!getishortarg(args, 8, 5, &arg6))
		return NULL;
	if (!getishortarg(args, 8, 6, &arg7))
		return NULL;
	if (!getishortarg(args, 8, 7, &arg8))
		return NULL;
	RGBrange( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setvideo long s long s */

static PyObject *
gl_setvideo(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	setvideo( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void shademodel long s */

static PyObject *
gl_shademodel(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	shademodel( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void underlay long s */

static PyObject *
gl_underlay(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	underlay( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgnclosedline */

static PyObject *
gl_bgnclosedline(PyObject *self, PyObject *args)
{
	bgnclosedline( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgnline */

static PyObject *
gl_bgnline(PyObject *self, PyObject *args)
{
	bgnline( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgnpoint */

static PyObject *
gl_bgnpoint(PyObject *self, PyObject *args)
{
	bgnpoint( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgnpolygon */

static PyObject *
gl_bgnpolygon(PyObject *self, PyObject *args)
{
	bgnpolygon( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgnsurface */

static PyObject *
gl_bgnsurface(PyObject *self, PyObject *args)
{
	bgnsurface( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgntmesh */

static PyObject *
gl_bgntmesh(PyObject *self, PyObject *args)
{
	bgntmesh( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void bgntrim */

static PyObject *
gl_bgntrim(PyObject *self, PyObject *args)
{
	bgntrim( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endclosedline */

static PyObject *
gl_endclosedline(PyObject *self, PyObject *args)
{
	endclosedline( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endline */

static PyObject *
gl_endline(PyObject *self, PyObject *args)
{
	endline( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endpoint */

static PyObject *
gl_endpoint(PyObject *self, PyObject *args)
{
	endpoint( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endpolygon */

static PyObject *
gl_endpolygon(PyObject *self, PyObject *args)
{
	endpolygon( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endsurface */

static PyObject *
gl_endsurface(PyObject *self, PyObject *args)
{
	endsurface( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endtmesh */

static PyObject *
gl_endtmesh(PyObject *self, PyObject *args)
{
	endtmesh( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void endtrim */

static PyObject *
gl_endtrim(PyObject *self, PyObject *args)
{
	endtrim( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void blendfunction long s long s */

static PyObject *
gl_blendfunction(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	blendfunction( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c3f float s[3] */

static PyObject *
gl_c3f(PyObject *self, PyObject *args)
{
	float arg1 [ 3 ] ;
	if (!getifloatarray(args, 1, 0, 3 , arg1))
		return NULL;
	c3f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c3i long s[3] */

static PyObject *
gl_c3i(PyObject *self, PyObject *args)
{
	long arg1 [ 3 ] ;
	if (!getilongarray(args, 1, 0, 3 , arg1))
		return NULL;
	c3i( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c3s short s[3] */

static PyObject *
gl_c3s(PyObject *self, PyObject *args)
{
	short arg1 [ 3 ] ;
	if (!getishortarray(args, 1, 0, 3 , arg1))
		return NULL;
	c3s( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c4f float s[4] */

static PyObject *
gl_c4f(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] ;
	if (!getifloatarray(args, 1, 0, 4 , arg1))
		return NULL;
	c4f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c4i long s[4] */

static PyObject *
gl_c4i(PyObject *self, PyObject *args)
{
	long arg1 [ 4 ] ;
	if (!getilongarray(args, 1, 0, 4 , arg1))
		return NULL;
	c4i( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void c4s short s[4] */

static PyObject *
gl_c4s(PyObject *self, PyObject *args)
{
	short arg1 [ 4 ] ;
	if (!getishortarray(args, 1, 0, 4 , arg1))
		return NULL;
	c4s( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void colorf float s */

static PyObject *
gl_colorf(PyObject *self, PyObject *args)
{
	float arg1 ;
	if (!getifloatarg(args, 1, 0, &arg1))
		return NULL;
	colorf( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void cpack long s */

static PyObject *
gl_cpack(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	cpack( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void czclear long s long s */

static PyObject *
gl_czclear(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	czclear( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void dglclose long s */

static PyObject *
gl_dglclose(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	dglclose( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long dglopen char *s long s */

static PyObject *
gl_dglopen(PyObject *self, PyObject *args)
{
	long retval;
	string arg1 ;
	long arg2 ;
	if (!getistringarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	retval = dglopen( arg1 , arg2 );
	return mknewlongobject(retval);
}

/* long getgdesc long s */

static PyObject *
gl_getgdesc(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = getgdesc( arg1 );
	return mknewlongobject(retval);
}

/* void getnurbsproperty long s float r */

static PyObject *
gl_getnurbsproperty(PyObject *self, PyObject *args)
{
	long arg1 ;
	float arg2 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	getnurbsproperty( arg1 , & arg2 );
	return mknewfloatobject(arg2);
}

/* void glcompat long s long s */

static PyObject *
gl_glcompat(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	glcompat( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void iconsize long s long s */

static PyObject *
gl_iconsize(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	iconsize( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void icontitle char *s */

static PyObject *
gl_icontitle(PyObject *self, PyObject *args)
{
	string arg1 ;
	if (!getistringarg(args, 1, 0, &arg1))
		return NULL;
	icontitle( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lRGBrange short s short s short s short s short s short s long s long s */

static PyObject *
gl_lRGBrange(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	short arg5 ;
	short arg6 ;
	long arg7 ;
	long arg8 ;
	if (!getishortarg(args, 8, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 8, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 8, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 8, 3, &arg4))
		return NULL;
	if (!getishortarg(args, 8, 4, &arg5))
		return NULL;
	if (!getishortarg(args, 8, 5, &arg6))
		return NULL;
	if (!getilongarg(args, 8, 6, &arg7))
		return NULL;
	if (!getilongarg(args, 8, 7, &arg8))
		return NULL;
	lRGBrange( arg1 , arg2 , arg3 , arg4 , arg5 , arg6 , arg7 , arg8 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void linesmooth long s */

static PyObject *
gl_linesmooth(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	linesmooth( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lmcolor long s */

static PyObject *
gl_lmcolor(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	lmcolor( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void logicop long s */

static PyObject *
gl_logicop(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	logicop( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lsetdepth long s long s */

static PyObject *
gl_lsetdepth(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	lsetdepth( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void lshaderange short s short s long s long s */

static PyObject *
gl_lshaderange(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	lshaderange( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void n3f float s[3] */

static PyObject *
gl_n3f(PyObject *self, PyObject *args)
{
	float arg1 [ 3 ] ;
	if (!getifloatarray(args, 1, 0, 3 , arg1))
		return NULL;
	n3f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void noborder */

static PyObject *
gl_noborder(PyObject *self, PyObject *args)
{
	noborder( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pntsmooth long s */

static PyObject *
gl_pntsmooth(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	pntsmooth( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void readsource long s */

static PyObject *
gl_readsource(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	readsource( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void rectzoom float s float s */

static PyObject *
gl_rectzoom(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	if (!getifloatarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	rectzoom( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sbox float s float s float s float s */

static PyObject *
gl_sbox(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	sbox( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sboxi long s long s long s long s */

static PyObject *
gl_sboxi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	sboxi( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sboxs short s short s short s short s */

static PyObject *
gl_sboxs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	sboxs( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sboxf float s float s float s float s */

static PyObject *
gl_sboxf(PyObject *self, PyObject *args)
{
	float arg1 ;
	float arg2 ;
	float arg3 ;
	float arg4 ;
	if (!getifloatarg(args, 4, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 4, 1, &arg2))
		return NULL;
	if (!getifloatarg(args, 4, 2, &arg3))
		return NULL;
	if (!getifloatarg(args, 4, 3, &arg4))
		return NULL;
	sboxf( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sboxfi long s long s long s long s */

static PyObject *
gl_sboxfi(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	long arg4 ;
	if (!getilongarg(args, 4, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 4, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 4, 2, &arg3))
		return NULL;
	if (!getilongarg(args, 4, 3, &arg4))
		return NULL;
	sboxfi( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void sboxfs short s short s short s short s */

static PyObject *
gl_sboxfs(PyObject *self, PyObject *args)
{
	short arg1 ;
	short arg2 ;
	short arg3 ;
	short arg4 ;
	if (!getishortarg(args, 4, 0, &arg1))
		return NULL;
	if (!getishortarg(args, 4, 1, &arg2))
		return NULL;
	if (!getishortarg(args, 4, 2, &arg3))
		return NULL;
	if (!getishortarg(args, 4, 3, &arg4))
		return NULL;
	sboxfs( arg1 , arg2 , arg3 , arg4 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setnurbsproperty long s float s */

static PyObject *
gl_setnurbsproperty(PyObject *self, PyObject *args)
{
	long arg1 ;
	float arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getifloatarg(args, 2, 1, &arg2))
		return NULL;
	setnurbsproperty( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void setpup long s long s long s */

static PyObject *
gl_setpup(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	long arg3 ;
	if (!getilongarg(args, 3, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 3, 1, &arg2))
		return NULL;
	if (!getilongarg(args, 3, 2, &arg3))
		return NULL;
	setpup( arg1 , arg2 , arg3 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void smoothline long s */

static PyObject *
gl_smoothline(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	smoothline( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void subpixel long s */

static PyObject *
gl_subpixel(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	subpixel( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void swaptmesh */

static PyObject *
gl_swaptmesh(PyObject *self, PyObject *args)
{
	swaptmesh( );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long swinopen long s */

static PyObject *
gl_swinopen(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = swinopen( arg1 );
	return mknewlongobject(retval);
}

/* void v2f float s[2] */

static PyObject *
gl_v2f(PyObject *self, PyObject *args)
{
	float arg1 [ 2 ] ;
	if (!getifloatarray(args, 1, 0, 2 , arg1))
		return NULL;
	v2f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v2i long s[2] */

static PyObject *
gl_v2i(PyObject *self, PyObject *args)
{
	long arg1 [ 2 ] ;
	if (!getilongarray(args, 1, 0, 2 , arg1))
		return NULL;
	v2i( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v2s short s[2] */

static PyObject *
gl_v2s(PyObject *self, PyObject *args)
{
	short arg1 [ 2 ] ;
	if (!getishortarray(args, 1, 0, 2 , arg1))
		return NULL;
	v2s( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v3f float s[3] */

static PyObject *
gl_v3f(PyObject *self, PyObject *args)
{
	float arg1 [ 3 ] ;
	if (!getifloatarray(args, 1, 0, 3 , arg1))
		return NULL;
	v3f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v3i long s[3] */

static PyObject *
gl_v3i(PyObject *self, PyObject *args)
{
	long arg1 [ 3 ] ;
	if (!getilongarray(args, 1, 0, 3 , arg1))
		return NULL;
	v3i( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v3s short s[3] */

static PyObject *
gl_v3s(PyObject *self, PyObject *args)
{
	short arg1 [ 3 ] ;
	if (!getishortarray(args, 1, 0, 3 , arg1))
		return NULL;
	v3s( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v4f float s[4] */

static PyObject *
gl_v4f(PyObject *self, PyObject *args)
{
	float arg1 [ 4 ] ;
	if (!getifloatarray(args, 1, 0, 4 , arg1))
		return NULL;
	v4f( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v4i long s[4] */

static PyObject *
gl_v4i(PyObject *self, PyObject *args)
{
	long arg1 [ 4 ] ;
	if (!getilongarray(args, 1, 0, 4 , arg1))
		return NULL;
	v4i( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v4s short s[4] */

static PyObject *
gl_v4s(PyObject *self, PyObject *args)
{
	short arg1 [ 4 ] ;
	if (!getishortarray(args, 1, 0, 4 , arg1))
		return NULL;
	v4s( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void videocmd long s */

static PyObject *
gl_videocmd(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	videocmd( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long windepth long s */

static PyObject *
gl_windepth(PyObject *self, PyObject *args)
{
	long retval;
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	retval = windepth( arg1 );
	return mknewlongobject(retval);
}

/* void wmpack long s */

static PyObject *
gl_wmpack(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	wmpack( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zdraw long s */

static PyObject *
gl_zdraw(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	zdraw( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zfunction long s */

static PyObject *
gl_zfunction(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	zfunction( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zsource long s */

static PyObject *
gl_zsource(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	zsource( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void zwritemask long s */

static PyObject *
gl_zwritemask(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	zwritemask( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v2d double s[2] */

static PyObject *
gl_v2d(PyObject *self, PyObject *args)
{
	double arg1 [ 2 ] ;
	if (!getidoublearray(args, 1, 0, 2 , arg1))
		return NULL;
	v2d( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v3d double s[3] */

static PyObject *
gl_v3d(PyObject *self, PyObject *args)
{
	double arg1 [ 3 ] ;
	if (!getidoublearray(args, 1, 0, 3 , arg1))
		return NULL;
	v3d( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void v4d double s[4] */

static PyObject *
gl_v4d(PyObject *self, PyObject *args)
{
	double arg1 [ 4 ] ;
	if (!getidoublearray(args, 1, 0, 4 , arg1))
		return NULL;
	v4d( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* void pixmode long s long s */

static PyObject *
gl_pixmode(PyObject *self, PyObject *args)
{
	long arg1 ;
	long arg2 ;
	if (!getilongarg(args, 2, 0, &arg1))
		return NULL;
	if (!getilongarg(args, 2, 1, &arg2))
		return NULL;
	pixmode( arg1 , arg2 );
	Py_INCREF(Py_None);
	return Py_None;
}

/* long qgetfd */

static PyObject *
gl_qgetfd(PyObject *self, PyObject *args)
{
	long retval;
	retval = qgetfd( );
	return mknewlongobject(retval);
}

/* void dither long s */

static PyObject *
gl_dither(PyObject *self, PyObject *args)
{
	long arg1 ;
	if (!getilongarg(args, 1, 0, &arg1))
		return NULL;
	dither( arg1 );
	Py_INCREF(Py_None);
	return Py_None;
}

static struct PyMethodDef gl_methods[] = {
	{"qread", gl_qread},
	{"varray", gl_varray},
	{"nvarray", gl_nvarray},
	{"vnarray", gl_vnarray},
	{"nurbssurface", gl_nurbssurface},
	{"nurbscurve", gl_nurbscurve},
	{"pwlcurve", gl_pwlcurve},
	{"pick", gl_pick},
	{"endpick", gl_endpick},
	{"gselect", gl_gselect},
	{"endselect", gl_endselect},
	{"getmatrix", gl_getmatrix},
	{"altgetmatrix", gl_altgetmatrix},
	{"lrectwrite", gl_lrectwrite},
	{"lrectread", gl_lrectread},
	{"readdisplay", gl_readdisplay},
	{"packrect", gl_packrect},
	{"unpackrect", gl_unpackrect},
	{"gversion", gl_gversion},
	{"clear", gl_clear},
	{"getshade", gl_getshade},
	{"devport", gl_devport},
	{"rdr2i", gl_rdr2i},
	{"rectfs", gl_rectfs},
	{"rects", gl_rects},
	{"rmv2i", gl_rmv2i},
	{"noport", gl_noport},
	{"popviewport", gl_popviewport},
	{"clearhitcode", gl_clearhitcode},
	{"closeobj", gl_closeobj},
	{"cursoff", gl_cursoff},
	{"curson", gl_curson},
	{"doublebuffer", gl_doublebuffer},
	{"finish", gl_finish},
	{"gconfig", gl_gconfig},
	{"ginit", gl_ginit},
	{"greset", gl_greset},
	{"multimap", gl_multimap},
	{"onemap", gl_onemap},
	{"popattributes", gl_popattributes},
	{"popmatrix", gl_popmatrix},
	{"pushattributes", gl_pushattributes},
	{"pushmatrix", gl_pushmatrix},
	{"pushviewport", gl_pushviewport},
	{"qreset", gl_qreset},
	{"RGBmode", gl_RGBmode},
	{"singlebuffer", gl_singlebuffer},
	{"swapbuffers", gl_swapbuffers},
	{"gsync", gl_gsync},
	{"gflush", gl_gflush},
	{"tpon", gl_tpon},
	{"tpoff", gl_tpoff},
	{"clkon", gl_clkon},
	{"clkoff", gl_clkoff},
	{"ringbell", gl_ringbell},
	{"gbegin", gl_gbegin},
	{"textinit", gl_textinit},
	{"initnames", gl_initnames},
	{"pclos", gl_pclos},
	{"popname", gl_popname},
	{"spclos", gl_spclos},
	{"zclear", gl_zclear},
	{"screenspace", gl_screenspace},
	{"reshapeviewport", gl_reshapeviewport},
	{"winpush", gl_winpush},
	{"winpop", gl_winpop},
	{"foreground", gl_foreground},
	{"endfullscrn", gl_endfullscrn},
	{"endpupmode", gl_endpupmode},
	{"fullscrn", gl_fullscrn},
	{"pupmode", gl_pupmode},
	{"winconstraints", gl_winconstraints},
	{"pagecolor", gl_pagecolor},
	{"textcolor", gl_textcolor},
	{"color", gl_color},
	{"curveit", gl_curveit},
	{"font", gl_font},
	{"linewidth", gl_linewidth},
	{"setlinestyle", gl_setlinestyle},
	{"setmap", gl_setmap},
	{"swapinterval", gl_swapinterval},
	{"writemask", gl_writemask},
	{"textwritemask", gl_textwritemask},
	{"qdevice", gl_qdevice},
	{"unqdevice", gl_unqdevice},
	{"curvebasis", gl_curvebasis},
	{"curveprecision", gl_curveprecision},
	{"loadname", gl_loadname},
	{"passthrough", gl_passthrough},
	{"pushname", gl_pushname},
	{"setmonitor", gl_setmonitor},
	{"setshade", gl_setshade},
	{"setpattern", gl_setpattern},
	{"pagewritemask", gl_pagewritemask},
	{"callobj", gl_callobj},
	{"delobj", gl_delobj},
	{"editobj", gl_editobj},
	{"makeobj", gl_makeobj},
	{"maketag", gl_maketag},
	{"chunksize", gl_chunksize},
	{"compactify", gl_compactify},
	{"deltag", gl_deltag},
	{"lsrepeat", gl_lsrepeat},
	{"objinsert", gl_objinsert},
	{"objreplace", gl_objreplace},
	{"winclose", gl_winclose},
	{"blanktime", gl_blanktime},
	{"freepup", gl_freepup},
	{"backbuffer", gl_backbuffer},
	{"frontbuffer", gl_frontbuffer},
	{"lsbackup", gl_lsbackup},
	{"resetls", gl_resetls},
	{"lampon", gl_lampon},
	{"lampoff", gl_lampoff},
	{"setbell", gl_setbell},
	{"blankscreen", gl_blankscreen},
	{"depthcue", gl_depthcue},
	{"zbuffer", gl_zbuffer},
	{"backface", gl_backface},
	{"cmov2i", gl_cmov2i},
	{"draw2i", gl_draw2i},
	{"move2i", gl_move2i},
	{"pnt2i", gl_pnt2i},
	{"patchbasis", gl_patchbasis},
	{"patchprecision", gl_patchprecision},
	{"pdr2i", gl_pdr2i},
	{"pmv2i", gl_pmv2i},
	{"rpdr2i", gl_rpdr2i},
	{"rpmv2i", gl_rpmv2i},
	{"xfpt2i", gl_xfpt2i},
	{"objdelete", gl_objdelete},
	{"patchcurves", gl_patchcurves},
	{"minsize", gl_minsize},
	{"maxsize", gl_maxsize},
	{"keepaspect", gl_keepaspect},
	{"prefsize", gl_prefsize},
	{"stepunit", gl_stepunit},
	{"fudge", gl_fudge},
	{"winmove", gl_winmove},
	{"attachcursor", gl_attachcursor},
	{"deflinestyle", gl_deflinestyle},
	{"noise", gl_noise},
	{"picksize", gl_picksize},
	{"qenter", gl_qenter},
	{"setdepth", gl_setdepth},
	{"cmov2s", gl_cmov2s},
	{"draw2s", gl_draw2s},
	{"move2s", gl_move2s},
	{"pdr2s", gl_pdr2s},
	{"pmv2s", gl_pmv2s},
	{"pnt2s", gl_pnt2s},
	{"rdr2s", gl_rdr2s},
	{"rmv2s", gl_rmv2s},
	{"rpdr2s", gl_rpdr2s},
	{"rpmv2s", gl_rpmv2s},
	{"xfpt2s", gl_xfpt2s},
	{"cmov2", gl_cmov2},
	{"draw2", gl_draw2},
	{"move2", gl_move2},
	{"pnt2", gl_pnt2},
	{"pdr2", gl_pdr2},
	{"pmv2", gl_pmv2},
	{"rdr2", gl_rdr2},
	{"rmv2", gl_rmv2},
	{"rpdr2", gl_rpdr2},
	{"rpmv2", gl_rpmv2},
	{"xfpt2", gl_xfpt2},
	{"loadmatrix", gl_loadmatrix},
	{"multmatrix", gl_multmatrix},
	{"crv", gl_crv},
	{"rcrv", gl_rcrv},
	{"addtopup", gl_addtopup},
	{"charstr", gl_charstr},
	{"getport", gl_getport},
	{"strwidth", gl_strwidth},
	{"winopen", gl_winopen},
	{"wintitle", gl_wintitle},
	{"polf", gl_polf},
	{"polf2", gl_polf2},
	{"poly", gl_poly},
	{"poly2", gl_poly2},
	{"crvn", gl_crvn},
	{"rcrvn", gl_rcrvn},
	{"polf2i", gl_polf2i},
	{"polfi", gl_polfi},
	{"poly2i", gl_poly2i},
	{"polyi", gl_polyi},
	{"polf2s", gl_polf2s},
	{"polfs", gl_polfs},
	{"polys", gl_polys},
	{"poly2s", gl_poly2s},
	{"defcursor", gl_defcursor},
	{"writepixels", gl_writepixels},
	{"defbasis", gl_defbasis},
	{"gewrite", gl_gewrite},
	{"rotate", gl_rotate},
	{"rot", gl_rot},
	{"circfi", gl_circfi},
	{"circi", gl_circi},
	{"cmovi", gl_cmovi},
	{"drawi", gl_drawi},
	{"movei", gl_movei},
	{"pnti", gl_pnti},
	{"newtag", gl_newtag},
	{"pdri", gl_pdri},
	{"pmvi", gl_pmvi},
	{"rdri", gl_rdri},
	{"rmvi", gl_rmvi},
	{"rpdri", gl_rpdri},
	{"rpmvi", gl_rpmvi},
	{"xfpti", gl_xfpti},
	{"circ", gl_circ},
	{"circf", gl_circf},
	{"cmov", gl_cmov},
	{"draw", gl_draw},
	{"move", gl_move},
	{"pnt", gl_pnt},
	{"scale", gl_scale},
	{"translate", gl_translate},
	{"pdr", gl_pdr},
	{"pmv", gl_pmv},
	{"rdr", gl_rdr},
	{"rmv", gl_rmv},
	{"rpdr", gl_rpdr},
	{"rpmv", gl_rpmv},
	{"xfpt", gl_xfpt},
	{"RGBcolor", gl_RGBcolor},
	{"RGBwritemask", gl_RGBwritemask},
	{"setcursor", gl_setcursor},
	{"tie", gl_tie},
	{"circfs", gl_circfs},
	{"circs", gl_circs},
	{"cmovs", gl_cmovs},
	{"draws", gl_draws},
	{"moves", gl_moves},
	{"pdrs", gl_pdrs},
	{"pmvs", gl_pmvs},
	{"pnts", gl_pnts},
	{"rdrs", gl_rdrs},
	{"rmvs", gl_rmvs},
	{"rpdrs", gl_rpdrs},
	{"rpmvs", gl_rpmvs},
	{"xfpts", gl_xfpts},
	{"curorigin", gl_curorigin},
	{"cyclemap", gl_cyclemap},
	{"patch", gl_patch},
	{"splf", gl_splf},
	{"splf2", gl_splf2},
	{"splfi", gl_splfi},
	{"splf2i", gl_splf2i},
	{"splfs", gl_splfs},
	{"splf2s", gl_splf2s},
	{"rpatch", gl_rpatch},
	{"ortho2", gl_ortho2},
	{"rect", gl_rect},
	{"rectf", gl_rectf},
	{"xfpt4", gl_xfpt4},
	{"textport", gl_textport},
	{"mapcolor", gl_mapcolor},
	{"scrmask", gl_scrmask},
	{"setvaluator", gl_setvaluator},
	{"viewport", gl_viewport},
	{"shaderange", gl_shaderange},
	{"xfpt4s", gl_xfpt4s},
	{"rectfi", gl_rectfi},
	{"recti", gl_recti},
	{"xfpt4i", gl_xfpt4i},
	{"prefposition", gl_prefposition},
	{"arc", gl_arc},
	{"arcf", gl_arcf},
	{"arcfi", gl_arcfi},
	{"arci", gl_arci},
	{"bbox2", gl_bbox2},
	{"bbox2i", gl_bbox2i},
	{"bbox2s", gl_bbox2s},
	{"blink", gl_blink},
	{"ortho", gl_ortho},
	{"window", gl_window},
	{"lookat", gl_lookat},
	{"perspective", gl_perspective},
	{"polarview", gl_polarview},
	{"arcfs", gl_arcfs},
	{"arcs", gl_arcs},
	{"rectcopy", gl_rectcopy},
	{"RGBcursor", gl_RGBcursor},
	{"getbutton", gl_getbutton},
	{"getcmmode", gl_getcmmode},
	{"getlsbackup", gl_getlsbackup},
	{"getresetls", gl_getresetls},
	{"getdcm", gl_getdcm},
	{"getzbuffer", gl_getzbuffer},
	{"ismex", gl_ismex},
	{"isobj", gl_isobj},
	{"isqueued", gl_isqueued},
	{"istag", gl_istag},
	{"genobj", gl_genobj},
	{"gentag", gl_gentag},
	{"getbuffer", gl_getbuffer},
	{"getcolor", gl_getcolor},
	{"getdisplaymode", gl_getdisplaymode},
	{"getfont", gl_getfont},
	{"getheight", gl_getheight},
	{"gethitcode", gl_gethitcode},
	{"getlstyle", gl_getlstyle},
	{"getlwidth", gl_getlwidth},
	{"getmap", gl_getmap},
	{"getplanes", gl_getplanes},
	{"getwritemask", gl_getwritemask},
	{"qtest", gl_qtest},
	{"getlsrepeat", gl_getlsrepeat},
	{"getmonitor", gl_getmonitor},
	{"getopenobj", gl_getopenobj},
	{"getpattern", gl_getpattern},
	{"winget", gl_winget},
	{"winattach", gl_winattach},
	{"getothermonitor", gl_getothermonitor},
	{"newpup", gl_newpup},
	{"getvaluator", gl_getvaluator},
	{"winset", gl_winset},
	{"dopup", gl_dopup},
	{"getdepth", gl_getdepth},
	{"getcpos", gl_getcpos},
	{"getsize", gl_getsize},
	{"getorigin", gl_getorigin},
	{"getviewport", gl_getviewport},
	{"gettp", gl_gettp},
	{"getgpos", gl_getgpos},
	{"winposition", gl_winposition},
	{"gRGBcolor", gl_gRGBcolor},
	{"gRGBmask", gl_gRGBmask},
	{"getscrmask", gl_getscrmask},
	{"getmcolor", gl_getmcolor},
	{"mapw", gl_mapw},
	{"mapw2", gl_mapw2},
	{"getcursor", gl_getcursor},
	{"cmode", gl_cmode},
	{"concave", gl_concave},
	{"curstype", gl_curstype},
	{"drawmode", gl_drawmode},
	{"gammaramp", gl_gammaramp},
	{"getbackface", gl_getbackface},
	{"getdescender", gl_getdescender},
	{"getdrawmode", gl_getdrawmode},
	{"getmmode", gl_getmmode},
	{"getsm", gl_getsm},
	{"getvideo", gl_getvideo},
	{"imakebackground", gl_imakebackground},
	{"lmbind", gl_lmbind},
	{"lmdef", gl_lmdef},
	{"mmode", gl_mmode},
	{"normal", gl_normal},
	{"overlay", gl_overlay},
	{"RGBrange", gl_RGBrange},
	{"setvideo", gl_setvideo},
	{"shademodel", gl_shademodel},
	{"underlay", gl_underlay},
	{"bgnclosedline", gl_bgnclosedline},
	{"bgnline", gl_bgnline},
	{"bgnpoint", gl_bgnpoint},
	{"bgnpolygon", gl_bgnpolygon},
	{"bgnsurface", gl_bgnsurface},
	{"bgntmesh", gl_bgntmesh},
	{"bgntrim", gl_bgntrim},
	{"endclosedline", gl_endclosedline},
	{"endline", gl_endline},
	{"endpoint", gl_endpoint},
	{"endpolygon", gl_endpolygon},
	{"endsurface", gl_endsurface},
	{"endtmesh", gl_endtmesh},
	{"endtrim", gl_endtrim},
	{"blendfunction", gl_blendfunction},
	{"c3f", gl_c3f},
	{"c3i", gl_c3i},
	{"c3s", gl_c3s},
	{"c4f", gl_c4f},
	{"c4i", gl_c4i},
	{"c4s", gl_c4s},
	{"colorf", gl_colorf},
	{"cpack", gl_cpack},
	{"czclear", gl_czclear},
	{"dglclose", gl_dglclose},
	{"dglopen", gl_dglopen},
	{"getgdesc", gl_getgdesc},
	{"getnurbsproperty", gl_getnurbsproperty},
	{"glcompat", gl_glcompat},
	{"iconsize", gl_iconsize},
	{"icontitle", gl_icontitle},
	{"lRGBrange", gl_lRGBrange},
	{"linesmooth", gl_linesmooth},
	{"lmcolor", gl_lmcolor},
	{"logicop", gl_logicop},
	{"lsetdepth", gl_lsetdepth},
	{"lshaderange", gl_lshaderange},
	{"n3f", gl_n3f},
	{"noborder", gl_noborder},
	{"pntsmooth", gl_pntsmooth},
	{"readsource", gl_readsource},
	{"rectzoom", gl_rectzoom},
	{"sbox", gl_sbox},
	{"sboxi", gl_sboxi},
	{"sboxs", gl_sboxs},
	{"sboxf", gl_sboxf},
	{"sboxfi", gl_sboxfi},
	{"sboxfs", gl_sboxfs},
	{"setnurbsproperty", gl_setnurbsproperty},
	{"setpup", gl_setpup},
	{"smoothline", gl_smoothline},
	{"subpixel", gl_subpixel},
	{"swaptmesh", gl_swaptmesh},
	{"swinopen", gl_swinopen},
	{"v2f", gl_v2f},
	{"v2i", gl_v2i},
	{"v2s", gl_v2s},
	{"v3f", gl_v3f},
	{"v3i", gl_v3i},
	{"v3s", gl_v3s},
	{"v4f", gl_v4f},
	{"v4i", gl_v4i},
	{"v4s", gl_v4s},
	{"videocmd", gl_videocmd},
	{"windepth", gl_windepth},
	{"wmpack", gl_wmpack},
	{"zdraw", gl_zdraw},
	{"zfunction", gl_zfunction},
	{"zsource", gl_zsource},
	{"zwritemask", gl_zwritemask},
	{"v2d", gl_v2d},
	{"v3d", gl_v3d},
	{"v4d", gl_v4d},
	{"pixmode", gl_pixmode},
	{"qgetfd", gl_qgetfd},
	{"dither", gl_dither},
	{NULL, NULL} /* Sentinel */
};

void
initgl(void)
{
	(void) Py_InitModule("gl", gl_methods);
}
