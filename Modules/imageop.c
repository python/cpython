
/* imageopmodule - Various operations on pictures */

#ifdef sun
#define signed
#endif

#include "Python.h"

#if SIZEOF_INT == 4
typedef int Py_Int32;
typedef unsigned int Py_UInt32;
#else
#if SIZEOF_LONG == 4
typedef long Py_Int32;
typedef unsigned long Py_UInt32;
#else
#error "No 4-byte integral type"
#endif
#endif

#define CHARP(cp, xmax, x, y) ((char *)(cp+y*xmax+x))
#define SHORTP(cp, xmax, x, y) ((short *)(cp+2*(y*xmax+x)))
#define LONGP(cp, xmax, x, y) ((Py_Int32 *)(cp+4*(y*xmax+x)))

static PyObject *ImageopError;
 
static PyObject *
imageop_crop(PyObject *self, PyObject *args)
{
	char *cp, *ncp;
	short *nsp;
	Py_Int32 *nlp;
	int len, size, x, y, newx1, newx2, newy1, newy2;
	int ix, iy, xstep, ystep;
	PyObject *rv;

	if ( !PyArg_Parse(args, "(s#iiiiiii)", &cp, &len, &size, &x, &y,
			  &newx1, &newy1, &newx2, &newy2) )
		return 0;
    
	if ( size != 1 && size != 2 && size != 4 ) {
		PyErr_SetString(ImageopError, "Size should be 1, 2 or 4");
		return 0;
	}
	if ( len != size*x*y ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
	xstep = (newx1 < newx2)? 1 : -1;
	ystep = (newy1 < newy2)? 1 : -1;
    
	rv = PyString_FromStringAndSize(NULL,
			     (abs(newx2-newx1)+1)*(abs(newy2-newy1)+1)*size);
	if ( rv == 0 )
		return 0;
	ncp = (char *)PyString_AsString(rv);
	nsp = (short *)ncp;
	nlp = (Py_Int32 *)ncp;
	newy2 += ystep;
	newx2 += xstep;
	for( iy = newy1; iy != newy2; iy+=ystep ) {
		for ( ix = newx1; ix != newx2; ix+=xstep ) {
			if ( iy < 0 || iy >= y || ix < 0 || ix >= x ) {
				if ( size == 1 )
					*ncp++ = 0;
				else
					*nlp++ = 0;
			} else {
				if ( size == 1 )
					*ncp++ = *CHARP(cp, x, ix, iy);
				else if ( size == 2 )
					*nsp++ = *SHORTP(cp, x, ix, iy);
				else
					*nlp++ = *LONGP(cp, x, ix, iy);
			}
		}
	}
	return rv;
}
 
static PyObject *
imageop_scale(PyObject *self, PyObject *args)
{
	char *cp, *ncp;
	short *nsp;
	Py_Int32 *nlp;
	int len, size, x, y, newx, newy;
	int ix, iy;
	int oix, oiy;
	PyObject *rv;

	if ( !PyArg_Parse(args, "(s#iiiii)",
			  &cp, &len, &size, &x, &y, &newx, &newy) )
		return 0;
    
	if ( size != 1 && size != 2 && size != 4 ) {
		PyErr_SetString(ImageopError, "Size should be 1, 2 or 4");
		return 0;
	}
	if ( len != size*x*y ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, newx*newy*size);
	if ( rv == 0 )
		return 0;
	ncp = (char *)PyString_AsString(rv);
	nsp = (short *)ncp;
	nlp = (Py_Int32 *)ncp;
	for( iy = 0; iy < newy; iy++ ) {
		for ( ix = 0; ix < newx; ix++ ) {
			oix = ix * x / newx;
			oiy = iy * y / newy;
			if ( size == 1 )
				*ncp++ = *CHARP(cp, x, oix, oiy);
			else if ( size == 2 )
				*nsp++ = *SHORTP(cp, x, oix, oiy);
			else
				*nlp++ = *LONGP(cp, x, oix, oiy);
		}
	}
	return rv;
}

/* Note: this routine can use a bit of optimizing */

static PyObject *
imageop_tovideo(PyObject *self, PyObject *args)
{
	int maxx, maxy, x, y, len;
	int i;
	unsigned char *cp, *ncp;
	int width;
	PyObject *rv;
   
    
	if ( !PyArg_Parse(args, "(s#iii)", &cp, &len, &width, &maxx, &maxy) )
		return 0;

	if ( width != 1 && width != 4 ) {
		PyErr_SetString(ImageopError, "Size should be 1 or 4");
		return 0;
	}
	if ( maxx*maxy*width != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, len);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	if ( width == 1 ) {
		memcpy(ncp, cp, maxx);		/* Copy first line */
		ncp += maxx;
		for (y=1; y<maxy; y++) {	/* Interpolate other lines */
			for(x=0; x<maxx; x++) {
				i = y*maxx + x;
				*ncp++ = ((int)cp[i] + (int)cp[i-maxx]) >> 1;
			}
		}
	} else {
		memcpy(ncp, cp, maxx*4);		/* Copy first line */
		ncp += maxx*4;
		for (y=1; y<maxy; y++) {	/* Interpolate other lines */
			for(x=0; x<maxx; x++) {
				i = (y*maxx + x)*4 + 1;
				*ncp++ = 0;	/* Skip alfa comp */
				*ncp++ = ((int)cp[i] + (int)cp[i-4*maxx]) >> 1;
				i++;
				*ncp++ = ((int)cp[i] + (int)cp[i-4*maxx]) >> 1;
				i++;
				*ncp++ = ((int)cp[i] + (int)cp[i-4*maxx]) >> 1;
			}
		}
	}
	return rv;
}

static PyObject *
imageop_grey2mono(PyObject *self, PyObject *args)
{
	int tres, x, y, len;
	unsigned char *cp, *ncp;
	unsigned char ovalue;
	PyObject *rv;
	int i, bit;
   
    
	if ( !PyArg_Parse(args, "(s#iii)", &cp, &len, &x, &y, &tres) )
		return 0;

	if ( x*y != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, (len+7)/8);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	bit = 0x80;
	ovalue = 0;
	for ( i=0; i < len; i++ ) {
		if ( (int)cp[i] > tres )
			ovalue |= bit;
		bit >>= 1;
		if ( bit == 0 ) {
			*ncp++ = ovalue;
			bit = 0x80;
			ovalue = 0;
		}
	}
	if ( bit != 0x80 )
		*ncp++ = ovalue;
	return rv;
}

static PyObject *
imageop_grey2grey4(PyObject *self, PyObject *args)
{
	int x, y, len;
	unsigned char *cp, *ncp;
	unsigned char ovalue;
	PyObject *rv;
	int i;
	int pos;
   
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	if ( x*y != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, (len+1)/2);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);
	pos = 0;
	ovalue = 0;
	for ( i=0; i < len; i++ ) {
		ovalue |= ((int)cp[i] & 0xf0) >> pos;
		pos += 4;
		if ( pos == 8 ) {
			*ncp++ = ovalue;
			ovalue = 0;
			pos = 0;
		}
	}
	if ( pos != 0 )
		*ncp++ = ovalue;
	return rv;
}

static PyObject *
imageop_grey2grey2(PyObject *self, PyObject *args)
{
	int x, y, len;
	unsigned char *cp, *ncp;
	unsigned char ovalue;
	PyObject *rv;
	int i;
	int pos;
   
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	if ( x*y != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, (len+3)/4);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);
	pos = 0;
	ovalue = 0;
	for ( i=0; i < len; i++ ) {
		ovalue |= ((int)cp[i] & 0xc0) >> pos;
		pos += 2;
		if ( pos == 8 ) {
			*ncp++ = ovalue;
			ovalue = 0;
			pos = 0;
		}
	}
	if ( pos != 0 )
		*ncp++ = ovalue;
	return rv;
}

static PyObject *
imageop_dither2mono(PyObject *self, PyObject *args)
{
	int sum, x, y, len;
	unsigned char *cp, *ncp;
	unsigned char ovalue;
	PyObject *rv;
	int i, bit;
   
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	if ( x*y != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, (len+7)/8);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	bit = 0x80;
	ovalue = 0;
	sum = 0;
	for ( i=0; i < len; i++ ) {
		sum += cp[i];
		if ( sum >= 256 ) {
			sum -= 256;
			ovalue |= bit;
		}
		bit >>= 1;
		if ( bit == 0 ) {
			*ncp++ = ovalue;
			bit = 0x80;
			ovalue = 0;
		}
	}
	if ( bit != 0x80 )
		*ncp++ = ovalue;
	return rv;
}

static PyObject *
imageop_dither2grey2(PyObject *self, PyObject *args)
{
	int x, y, len;
	unsigned char *cp, *ncp;
	unsigned char ovalue;
	PyObject *rv;
	int i;
	int pos;
	int sum = 0, nvalue;
   
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	if ( x*y != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, (len+3)/4);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);
	pos = 1;
	ovalue = 0;
	for ( i=0; i < len; i++ ) {
		sum += cp[i];
		nvalue = sum & 0x180;
		sum -= nvalue;
		ovalue |= nvalue >> pos;
		pos += 2;
		if ( pos == 9 ) {
			*ncp++ = ovalue;
			ovalue = 0;
			pos = 1;
		}
	}
	if ( pos != 0 )
		*ncp++ = ovalue;
	return rv;
}

static PyObject *
imageop_mono2grey(PyObject *self, PyObject *args)
{
	int v0, v1, x, y, len, nlen;
	unsigned char *cp, *ncp;
	PyObject *rv;
	int i, bit;
    
	if ( !PyArg_Parse(args, "(s#iiii)", &cp, &len, &x, &y, &v0, &v1) )
		return 0;

	nlen = x*y;
	if ( (nlen+7)/8 != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	bit = 0x80;
	for ( i=0; i < nlen; i++ ) {
		if ( *cp & bit )
			*ncp++ = v1;
		else
			*ncp++ = v0;
		bit >>= 1;
		if ( bit == 0 ) {
			bit = 0x80;
			cp++;
		}
	}
	return rv;
}

static PyObject *
imageop_grey22grey(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	unsigned char *cp, *ncp;
	PyObject *rv;
	int i, pos, value = 0, nvalue;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( (nlen+3)/4 != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	pos = 0;
	for ( i=0; i < nlen; i++ ) {
		if ( pos == 0 ) {
			value = *cp++;
			pos = 8;
		}
		pos -= 2;
		nvalue = (value >> pos) & 0x03;
		*ncp++ = nvalue | (nvalue << 2) |
			 (nvalue << 4) | (nvalue << 6);
	}
	return rv;
}

static PyObject *
imageop_grey42grey(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	unsigned char *cp, *ncp;
	PyObject *rv;
	int i, pos, value = 0, nvalue;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( (nlen+1)/2 != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	pos = 0;
	for ( i=0; i < nlen; i++ ) {
		if ( pos == 0 ) {
			value = *cp++;
			pos = 8;
		}
		pos -= 4;
		nvalue = (value >> pos) & 0x0f;
		*ncp++ = nvalue | (nvalue << 4);
	}
	return rv;
}

static PyObject *
imageop_rgb2rgb8(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	Py_UInt32 *cp;
	unsigned char *ncp;
	PyObject *rv;
	int i, r, g, b;
	Py_UInt32 value, nvalue;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( nlen*4 != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	for ( i=0; i < nlen; i++ ) {
		/* Bits in source: aaaaaaaa BBbbbbbb GGGggggg RRRrrrrr */
		value = *cp++;
#if 0
		r = (value >>  5) & 7;
		g = (value >> 13) & 7;
		b = (value >> 22) & 3;
#else
		r = (int) ((value & 0xff) / 255. * 7. + .5);
		g = (int) (((value >> 8) & 0xff) / 255. * 7. + .5);
		b = (int) (((value >> 16) & 0xff) / 255. * 3. + .5);
#endif
		nvalue = (r<<5) | (b<<3) | g;
		*ncp++ = (unsigned char)nvalue;
	}
	return rv;
}

static PyObject *
imageop_rgb82rgb(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	unsigned char *cp;
	Py_UInt32 *ncp;
	PyObject *rv;
	int i, r, g, b;
	Py_UInt32 value, nvalue;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( nlen != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen*4);
	if ( rv == 0 )
		return 0;
	ncp = (Py_UInt32 *)PyString_AsString(rv);

	for ( i=0; i < nlen; i++ ) {
		/* Bits in source: RRRBBGGG
		** Red and Green are multiplied by 36.5, Blue by 85
		*/
		value = *cp++;
		r = (value >> 5) & 7;
		g = (value     ) & 7;
		b = (value >> 3) & 3;
		r = (r<<5) | (r<<3) | (r>>1);
		g = (g<<5) | (g<<3) | (g>>1);
		b = (b<<6) | (b<<4) | (b<<2) | b;
		nvalue = r | (g<<8) | (b<<16);
		*ncp++ = nvalue;
	}
	return rv;
}

static PyObject *
imageop_rgb2grey(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	Py_UInt32 *cp;
	unsigned char *ncp;
	PyObject *rv;
	int i, r, g, b;
	Py_UInt32 value, nvalue;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( nlen*4 != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen);
	if ( rv == 0 )
		return 0;
	ncp = (unsigned char *)PyString_AsString(rv);

	for ( i=0; i < nlen; i++ ) {
		value = *cp++;
		r = (value      ) & 0xff;
		g = (value >>  8) & 0xff;
		b = (value >> 16) & 0xff;
		nvalue = (int)(0.30*r + 0.59*g + 0.11*b);
		if ( nvalue > 255 ) nvalue = 255;
		*ncp++ = (unsigned char)nvalue;
	}
	return rv;
}

static PyObject *
imageop_grey2rgb(PyObject *self, PyObject *args)
{
	int x, y, len, nlen;
	unsigned char *cp;
	Py_UInt32 *ncp;
	PyObject *rv;
	int i;
	Py_UInt32 value;
    
	if ( !PyArg_Parse(args, "(s#ii)", &cp, &len, &x, &y) )
		return 0;

	nlen = x*y;
	if ( nlen != len ) {
		PyErr_SetString(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = PyString_FromStringAndSize(NULL, nlen*4);
	if ( rv == 0 )
		return 0;
	ncp = (Py_UInt32 *)PyString_AsString(rv);

	for ( i=0; i < nlen; i++ ) {
		value = *cp++;
		*ncp++ = value | (value << 8 ) | (value << 16);
	}
	return rv;
}

/*
static object *
imageop_mul(object *self, object *args)
{
	char *cp, *ncp;
	int len, size, x, y;
	object *rv;
	int i;

	if ( !getargs(args, "(s#iii)", &cp, &len, &size, &x, &y) )
		return 0;
    
	if ( size != 1 && size != 4 ) {
		err_setstr(ImageopError, "Size should be 1 or 4");
		return 0;
	}
	if ( len != size*x*y ) {
		err_setstr(ImageopError, "String has incorrect length");
		return 0;
	}
    
	rv = newsizedstringobject(NULL, XXXX);
	if ( rv == 0 )
		return 0;
	ncp = (char *)getstringvalue(rv);
    
    
	for ( i=0; i < len; i += size ) {
	}
	return rv;
}
*/

static PyMethodDef imageop_methods[] = {
	{ "crop",		imageop_crop },
	{ "scale",		imageop_scale },
	{ "grey2mono",	        imageop_grey2mono },
	{ "grey2grey2",	        imageop_grey2grey2 },
	{ "grey2grey4",	        imageop_grey2grey4 },
	{ "dither2mono",	imageop_dither2mono },
	{ "dither2grey2",	imageop_dither2grey2 },
	{ "mono2grey",	        imageop_mono2grey },
	{ "grey22grey",	        imageop_grey22grey },
	{ "grey42grey",	        imageop_grey42grey },
	{ "tovideo",	        imageop_tovideo },
	{ "rgb2rgb8",	        imageop_rgb2rgb8 },
	{ "rgb82rgb",	        imageop_rgb82rgb },
	{ "rgb2grey",	        imageop_rgb2grey },
	{ "grey2rgb",	        imageop_grey2rgb },
	{ 0,                    0 }
};


DL_EXPORT(void)
initimageop(void)
{
	PyObject *m, *d;
	m = Py_InitModule("imageop", imageop_methods);
	d = PyModule_GetDict(m);
	ImageopError = PyErr_NewException("imageop.error", NULL, NULL);
	if (ImageopError != NULL)
		PyDict_SetItemString(d, "error", ImageopError);
}
