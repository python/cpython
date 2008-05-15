
/* IMGFILE module - Interface to sgi libimage */

/* XXX This module should be done better at some point. It should return
** an object of image file class, and have routines to manipulate these
** image files in a neater way (so you can get rgb images off a greyscale
** file, for instance, or do a straight display without having to get the
** image bits into python, etc).
**
** Warning: this module is very non-reentrant (esp. the readscaled stuff)
*/

#include "Python.h"

#include <gl/image.h>

#include "/usr/people/4Dgifts/iristools/include/izoom.h"

/* Bunch of missing extern decls; keep gcc -Wall happy... */
extern void i_seterror();
extern void iclose();
extern void filterzoom();
extern void putrow();
extern void getrow();

static PyObject * ImgfileError; /* Exception we raise for various trouble */

static int top_to_bottom;	/* True if we want top-to-bottom images */

/* The image library does not always call the error hander :-(,
   therefore we have a global variable indicating that it was called.
   It is cleared by imgfile_open(). */

static int error_called;


/* The error handler */

static void
imgfile_error(char *str)
{
	PyErr_SetString(ImgfileError, str);
	error_called = 1;
	return;	/* To imglib, which will return a failure indicator */
}


/* Open an image file and return a pointer to it.
   Make sure we raise an exception if we fail. */

static IMAGE *
imgfile_open(char *fname)
{
	IMAGE *image;
	i_seterror(imgfile_error);
	error_called = 0;
	errno = 0;
	if ( (image = iopen(fname, "r")) == NULL ) {
		/* Error may already be set by imgfile_error */
		if ( !error_called ) {
			if (errno)
				PyErr_SetFromErrno(ImgfileError);
			else
				PyErr_SetString(ImgfileError,
						"Can't open image file");
		}
		return NULL;
	}
	return image;
}

static PyObject *
imgfile_ttob(PyObject *self, PyObject *args)
{
	int newval;
	PyObject *rv;
    
	if (!PyArg_ParseTuple(args, "i:ttob", &newval))
		return NULL;
	rv = PyInt_FromLong(top_to_bottom);
	top_to_bottom = newval;
	return rv;
}

static PyObject *
imgfile_read(PyObject *self, PyObject *args)
{
	char *fname;
	PyObject *rv;
	int xsize, ysize, zsize;
	char *cdatap;
	long *idatap;
	static short rs[8192], gs[8192], bs[8192];
	int x, y;
	IMAGE *image;
	int yfirst, ylast, ystep;

	if ( !PyArg_ParseTuple(args, "s:read", &fname) )
		return NULL;
    
	if ( (image = imgfile_open(fname)) == NULL )
		return NULL;
    
	if ( image->colormap != CM_NORMAL ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can only handle CM_NORMAL images");
		return NULL;
	}
	if ( BPP(image->type) != 1 ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can't handle imgfiles with bpp!=1");
		return NULL;
	}
	xsize = image->xsize;
	ysize = image->ysize;
	zsize = image->zsize;
	if ( zsize != 1 && zsize != 3) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can only handle 1 or 3 byte pixels");
		return NULL;
	}
	if ( xsize > 8192 ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can't handle image with > 8192 columns");
		return NULL;
	}

	if ( zsize == 3 ) zsize = 4;
	rv = PyString_FromStringAndSize((char *)NULL, xsize*ysize*zsize);
	if ( rv == NULL ) {
		iclose(image);
		return NULL;
	}
	cdatap = PyString_AsString(rv);
	idatap = (long *)cdatap;

	if (top_to_bottom) {
		yfirst = ysize-1;
		ylast = -1;
		ystep = -1;
	} else {
		yfirst = 0;
		ylast = ysize;
		ystep = 1;
	}
	for ( y=yfirst; y != ylast && !error_called; y += ystep ) {
		if ( zsize == 1 ) {
			getrow(image, rs, y, 0);
			for(x=0; x<xsize; x++ )
				*cdatap++ = rs[x];
		} else {
			getrow(image, rs, y, 0);
			getrow(image, gs, y, 1);
			getrow(image, bs, y, 2);
			for(x=0; x<xsize; x++ )
				*idatap++ = (rs[x] & 0xff)  |
					((gs[x] & 0xff)<<8) |
					((bs[x] & 0xff)<<16);
		}
	}
	iclose(image);
	if ( error_called ) {
		Py_DECREF(rv);
		return NULL;
	}
	return rv;
}

static IMAGE *glob_image;
static long *glob_datap;
static int glob_width, glob_z, glob_ysize;

static void
xs_get(short *buf, int y)
{
	if (top_to_bottom)
		getrow(glob_image, buf, (glob_ysize-1-y), glob_z);
	else
		getrow(glob_image, buf, y, glob_z);
}

static void
xs_put_c(short *buf, int y)
{
	char *datap = (char *)glob_datap + y*glob_width;
	int width = glob_width;

	while ( width-- )
		*datap++ = (*buf++) & 0xff;
}

static void
xs_put_0(short *buf, int y)
{
	long *datap = glob_datap + y*glob_width;
	int width = glob_width;

	while ( width-- )
		*datap++ = (*buf++) & 0xff;
}
static void
xs_put_12(short *buf, int y)
{
	long *datap = glob_datap + y*glob_width;
	int width = glob_width;

	while ( width-- )
		*datap++ |= ((*buf++) & 0xff) << (glob_z*8);
}

static void
xscale(IMAGE *image, int xsize, int ysize, int zsize,
       long *datap, int xnew, int ynew, int fmode, double blur)
{
	glob_image = image;
	glob_datap = datap;
	glob_width = xnew;
	glob_ysize = ysize;
	if ( zsize == 1 ) {
		glob_z = 0;
		filterzoom(xs_get, xs_put_c, xsize, ysize,
			   xnew, ynew, fmode, blur);
	} else {
		glob_z = 0;
		filterzoom(xs_get, xs_put_0, xsize, ysize,
			   xnew, ynew, fmode, blur);
		glob_z = 1;
		filterzoom(xs_get, xs_put_12, xsize, ysize,
			   xnew, ynew, fmode, blur);
		glob_z = 2;
		filterzoom(xs_get, xs_put_12, xsize, ysize,
			   xnew, ynew, fmode, blur);
	}
}


static PyObject *
imgfile_readscaled(PyObject *self, PyObject *args)
{
	char *fname;
	PyObject *rv;
	int xsize, ysize, zsize;
	char *cdatap;
	long *idatap;
	static short rs[8192], gs[8192], bs[8192];
	int x, y;
	int xwtd, ywtd, xorig, yorig;
	float xfac, yfac;
	IMAGE *image;
	char *filter;
	double blur = 1.0;
	int extended;
	int fmode = 0;
	int yfirst, ylast, ystep;

	/*
	** Parse args. Funny, since arg 4 and 5 are optional
	** (filter name and blur factor). Also, 4 or 5 arguments indicates
	** extended scale algorithm in stead of simple-minded pixel drop/dup.
	*/
	extended = PyTuple_Size(args) >= 4;
	if ( !PyArg_ParseTuple(args, "sii|sd",
			       &fname, &xwtd, &ywtd, &filter, &blur) )
		return NULL;

	/*
	** Check parameters, open file and check type, rows, etc.
	*/
	if ( extended ) {
		if ( strcmp(filter, "impulse") == 0 )
			fmode = IMPULSE;
		else if ( strcmp( filter, "box") == 0 )
			fmode = BOX;
		else if ( strcmp( filter, "triangle") == 0 )
			fmode = TRIANGLE;
		else if ( strcmp( filter, "quadratic") == 0 )
			fmode = QUADRATIC;
		else if ( strcmp( filter, "gaussian") == 0 )
			fmode = GAUSSIAN;
		else {
			PyErr_SetString(ImgfileError, "Unknown filter type");
			return NULL;
		}
	}
    
	if ( (image = imgfile_open(fname)) == NULL )
		return NULL;
    
	if ( image->colormap != CM_NORMAL ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can only handle CM_NORMAL images");
		return NULL;
	}
	if ( BPP(image->type) != 1 ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can't handle imgfiles with bpp!=1");
		return NULL;
	}
	xsize = image->xsize;
	ysize = image->ysize;
	zsize = image->zsize;
	if ( zsize != 1 && zsize != 3) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can only handle 1 or 3 byte pixels");
		return NULL;
	}
	if ( xsize > 8192 ) {
		iclose(image);
		PyErr_SetString(ImgfileError,
				"Can't handle image with > 8192 columns");
		return NULL;
	}

	if ( zsize == 3 ) zsize = 4;
	rv = PyString_FromStringAndSize(NULL, xwtd*ywtd*zsize);
	if ( rv == NULL ) {
		iclose(image);
		return NULL;
	}
	PyFPE_START_PROTECT("readscaled", return 0)
	xfac = (float)xsize/(float)xwtd;
	yfac = (float)ysize/(float)ywtd;
	PyFPE_END_PROTECT(yfac)
	cdatap = PyString_AsString(rv);
	idatap = (long *)cdatap;

	if ( extended ) {
		xscale(image, xsize, ysize, zsize,
		       idatap, xwtd, ywtd, fmode, blur);
	} else {
		if (top_to_bottom) {
			yfirst = ywtd-1;
			ylast = -1;
			ystep = -1;
		} else {
			yfirst = 0;
			ylast = ywtd;
			ystep = 1;
		}
		for ( y=yfirst; y != ylast && !error_called; y += ystep ) {
			yorig = (int)(y*yfac);
			if ( zsize == 1 ) {
				getrow(image, rs, yorig, 0);
				for(x=0; x<xwtd; x++ ) {
					*cdatap++ = rs[(int)(x*xfac)];	
				}
			} else {
				getrow(image, rs, yorig, 0);
				getrow(image, gs, yorig, 1);
				getrow(image, bs, yorig, 2);
				for(x=0; x<xwtd; x++ ) {
					xorig = (int)(x*xfac);
					*idatap++ = (rs[xorig] & 0xff)  |
						((gs[xorig] & 0xff)<<8) |
						((bs[xorig] & 0xff)<<16);
				}
			}
		}
	}
	iclose(image);
	if ( error_called ) {
		Py_DECREF(rv);
		return NULL;
	}
	return rv;
}

static PyObject *
imgfile_getsizes(PyObject *self, PyObject *args)
{
	char *fname;
	PyObject *rv;
	IMAGE *image;
    
	if ( !PyArg_ParseTuple(args, "s:getsizes", &fname) )
		return NULL;
    
	if ( (image = imgfile_open(fname)) == NULL )
		return NULL;
	rv = Py_BuildValue("(iii)", image->xsize, image->ysize, image->zsize);
	iclose(image);
	return rv;
}

static PyObject *
imgfile_write(PyObject *self, PyObject *args)
{
	IMAGE *image;
	char *fname;
	int xsize, ysize, zsize, len;
	char *cdatap;
	long *idatap;
	short rs[8192], gs[8192], bs[8192];
	short r, g, b;
	long rgb;
	int x, y;
	int yfirst, ylast, ystep;


	if ( !PyArg_ParseTuple(args, "ss#iii:write",
			  &fname, &cdatap, &len, &xsize, &ysize, &zsize) )
		return NULL;
    
	if ( zsize != 1 && zsize != 3 ) {
		PyErr_SetString(ImgfileError,
				"Can only handle 1 or 3 byte pixels");
		return NULL;
	}
	if ( len != xsize * ysize * (zsize == 1 ? 1 : 4) ) {
		PyErr_SetString(ImgfileError, "Data does not match sizes");
		return NULL;
	}
	if ( xsize > 8192 ) {
		PyErr_SetString(ImgfileError,
				"Can't handle image with > 8192 columns");
		return NULL;
	}

	error_called = 0;
	errno = 0;
	image =iopen(fname, "w", RLE(1), 3, xsize, ysize, zsize);
	if ( image == 0 ) {
		if ( ! error_called ) {
			if (errno)
				PyErr_SetFromErrno(ImgfileError);
			else
				PyErr_SetString(ImgfileError,
						"Can't create image file");
		}
		return NULL;
	}

	idatap = (long *)cdatap;
    
	if (top_to_bottom) {
		yfirst = ysize-1;
		ylast = -1;
		ystep = -1;
	} else {
		yfirst = 0;
		ylast = ysize;
		ystep = 1;
	}
	for ( y=yfirst; y != ylast && !error_called; y += ystep ) {
		if ( zsize == 1 ) {
			for( x=0; x<xsize; x++ )
				rs[x] = *cdatap++;
			putrow(image, rs, y, 0);
		} else {
			for( x=0; x<xsize; x++ ) {
				rgb = *idatap++;
				r = rgb & 0xff;
				g = (rgb >> 8 ) & 0xff;
				b = (rgb >> 16 ) & 0xff;
				rs[x] = r;
				gs[x] = g;
				bs[x] = b;
			}
			putrow(image, rs, y, 0);
			putrow(image, gs, y, 1);
			putrow(image, bs, y, 2);
		}
	}
	iclose(image);
	if ( error_called )
		return NULL;
	Py_INCREF(Py_None);
	return Py_None;
    
}


static PyMethodDef imgfile_methods[] = {
	{ "getsizes",	imgfile_getsizes, METH_VARARGS },
	{ "read",	imgfile_read, METH_VARARGS },
	{ "readscaled",	imgfile_readscaled, METH_VARARGS},
	{ "write",	imgfile_write, METH_VARARGS },
	{ "ttob",	imgfile_ttob, METH_VARARGS },
	{ NULL,		NULL } /* Sentinel */
};


void
initimgfile(void)
{
	PyObject *m, *d;
	
	if (PyErr_WarnPy3k("the imgfile module has been removed in "
	                   "Python 3.0", 2) < 0)
	    return;
	
	m = Py_InitModule("imgfile", imgfile_methods);
	if (m == NULL)
		return;
	d = PyModule_GetDict(m);
	ImgfileError = PyErr_NewException("imgfile.error", NULL, NULL);
	if (ImgfileError != NULL)
		PyDict_SetItemString(d, "error", ImgfileError);
}



