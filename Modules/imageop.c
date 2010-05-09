
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
static PyObject *ImageopDict;

/**
 * Check a coordonnate, make sure that (0 < value).
 * Return 0 on error.
 */
static int
check_coordonnate(int value, const char* name)
{
    if ( 0 < value)
        return 1;
    PyErr_Format(PyExc_ValueError, "%s value is negative or nul", name);
    return 0;
}

/**
 * Check integer overflow to make sure that product == x*y*size.
 * Return 0 on error.
 */
static int
check_multiply_size(int product, int x, const char* xname, int y, const char* yname, int size)
{
    if ( !check_coordonnate(x, xname) )
        return 0;
    if ( !check_coordonnate(y, yname) )
        return 0;
    if ( size == (product / y) / x )
        return 1;
    PyErr_SetString(ImageopError, "String has incorrect length");
    return 0;
}

/**
 * Check integer overflow to make sure that product == x*y.
 * Return 0 on error.
 */
static int
check_multiply(int product, int x, int y)
{
    return check_multiply_size(product, x, "x", y, "y", 1);
}

/* If this function returns true (the default if anything goes wrong), we're
   behaving in a backward-compatible way with respect to how multi-byte pixels
   are stored in the strings.  The code in this module was originally written
   for an SGI which is a big-endian system, and so the old code assumed that
   4-byte integers hold the R, G, and B values in a particular order.
   However, on little-endian systems the order is reversed, and so not
   actually compatible with what gl.lrectwrite and imgfile expect.
   (gl.lrectwrite and imgfile are also SGI-specific, however, it is
   conceivable that the data handled here comes from or goes to an SGI or that
   it is otherwise used in the expectation that the byte order in the strings
   is as specified.)

   The function returns the value of the module variable
   "backward_compatible", or 1 if the variable does not exist or is not an
   int.
 */

static int
imageop_backward_compatible(void)
{
    static PyObject *bcos;
    PyObject *bco;
    long rc;

    if (ImageopDict == NULL) /* "cannot happen" */
        return 1;
    if (bcos == NULL) {
        /* cache string object for future use */
        bcos = PyString_FromString("backward_compatible");
        if (bcos == NULL)
            return 1;
    }
    bco = PyDict_GetItem(ImageopDict, bcos);
    if (bco == NULL)
        return 1;
    if (!PyInt_Check(bco))
        return 1;
    rc = PyInt_AsLong(bco);
    if (PyErr_Occurred()) {
        /* not an integer, or too large, or something */
        PyErr_Clear();
        rc = 1;
    }
    return rc != 0;             /* convert to values 0, 1 */
}

static PyObject *
imageop_crop(PyObject *self, PyObject *args)
{
    char *cp, *ncp;
    short *nsp;
    Py_Int32 *nlp;
    int len, size, x, y, newx1, newx2, newy1, newy2, nlen;
    int ix, iy, xstep, ystep;
    PyObject *rv;

    if ( !PyArg_ParseTuple(args, "s#iiiiiii", &cp, &len, &size, &x, &y,
                      &newx1, &newy1, &newx2, &newy2) )
        return 0;

    if ( size != 1 && size != 2 && size != 4 ) {
        PyErr_SetString(ImageopError, "Size should be 1, 2 or 4");
        return 0;
    }
    if ( !check_multiply_size(len, x, "x", y, "y", size) )
        return 0;

    xstep = (newx1 < newx2)? 1 : -1;
    ystep = (newy1 < newy2)? 1 : -1;

    nlen = (abs(newx2-newx1)+1)*(abs(newy2-newy1)+1)*size;
    if ( !check_multiply_size(nlen, abs(newx2-newx1)+1, "abs(newx2-newx1)+1", abs(newy2-newy1)+1, "abs(newy2-newy1)+1", size) )
        return 0;
    rv = PyString_FromStringAndSize(NULL, nlen);
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
    int len, size, x, y, newx, newy, nlen;
    int ix, iy;
    int oix, oiy;
    PyObject *rv;

    if ( !PyArg_ParseTuple(args, "s#iiiii",
                      &cp, &len, &size, &x, &y, &newx, &newy) )
        return 0;

    if ( size != 1 && size != 2 && size != 4 ) {
        PyErr_SetString(ImageopError, "Size should be 1, 2 or 4");
        return 0;
    }
    if ( !check_multiply_size(len, x, "x", y, "y", size) )
        return 0;
    nlen = newx*newy*size;
    if ( !check_multiply_size(nlen, newx, "newx", newy, "newy", size) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, nlen);
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


    if ( !PyArg_ParseTuple(args, "s#iii", &cp, &len, &width, &maxx, &maxy) )
        return 0;

    if ( width != 1 && width != 4 ) {
        PyErr_SetString(ImageopError, "Size should be 1 or 4");
        return 0;
    }
    if ( !check_multiply_size(len, maxx, "max", maxy, "maxy", width) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, len);
    if ( rv == 0 )
        return 0;
    ncp = (unsigned char *)PyString_AsString(rv);

    if ( width == 1 ) {
        memcpy(ncp, cp, maxx);                  /* Copy first line */
        ncp += maxx;
        for (y=1; y<maxy; y++) {                /* Interpolate other lines */
            for(x=0; x<maxx; x++) {
                i = y*maxx + x;
                *ncp++ = ((int)cp[i] + (int)cp[i-maxx]) >> 1;
            }
        }
    } else {
        memcpy(ncp, cp, maxx*4);                        /* Copy first line */
        ncp += maxx*4;
        for (y=1; y<maxy; y++) {                /* Interpolate other lines */
            for(x=0; x<maxx; x++) {
                i = (y*maxx + x)*4 + 1;
                *ncp++ = 0;                     /* Skip alfa comp */
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


    if ( !PyArg_ParseTuple(args, "s#iii", &cp, &len, &x, &y, &tres) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;

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


    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;

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


    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;

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


    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;

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


    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;

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

    if ( !PyArg_ParseTuple(args, "s#iiii", &cp, &len, &x, &y, &v0, &v1) )
        return 0;

    nlen = x*y;
    if ( !check_multiply(nlen, x, y) )
        return 0;
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

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    nlen = x*y;
    if ( !check_multiply(nlen, x, y) ) {
        return 0;
    }
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

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    nlen = x*y;
    if ( !check_multiply(nlen, x, y) )
        return 0;
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
    unsigned char *cp;
    unsigned char *ncp;
    PyObject *rv;
    int i, r, g, b;
    int backward_compatible = imageop_backward_compatible();

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply_size(len, x, "x", y, "y", 4) )
        return 0;
    nlen = x*y;
    if ( !check_multiply(nlen, x, y) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, nlen);
    if ( rv == 0 )
        return 0;
    ncp = (unsigned char *)PyString_AsString(rv);

    for ( i=0; i < nlen; i++ ) {
        /* Bits in source: aaaaaaaa BBbbbbbb GGGggggg RRRrrrrr */
        if (backward_compatible) {
            Py_UInt32 value = * (Py_UInt32 *) cp;
            cp += 4;
            r = (int) ((value & 0xff) / 255. * 7. + .5);
            g = (int) (((value >> 8) & 0xff) / 255. * 7. + .5);
            b = (int) (((value >> 16) & 0xff) / 255. * 3. + .5);
        } else {
            cp++;                       /* skip alpha channel */
            b = (int) (*cp++ / 255. * 3. + .5);
            g = (int) (*cp++ / 255. * 7. + .5);
            r = (int) (*cp++ / 255. * 7. + .5);
        }
        *ncp++ = (unsigned char)((r<<5) | (b<<3) | g);
    }
    return rv;
}

static PyObject *
imageop_rgb82rgb(PyObject *self, PyObject *args)
{
    int x, y, len, nlen;
    unsigned char *cp;
    unsigned char *ncp;
    PyObject *rv;
    int i, r, g, b;
    unsigned char value;
    int backward_compatible = imageop_backward_compatible();

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;
    nlen = x*y*4;
    if ( !check_multiply_size(nlen, x, "x", y, "y", 4) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, nlen);
    if ( rv == 0 )
        return 0;
    ncp = (unsigned char *)PyString_AsString(rv);

    for ( i=0; i < len; i++ ) {
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
        if (backward_compatible) {
            Py_UInt32 nvalue = r | (g<<8) | (b<<16);
            * (Py_UInt32 *) ncp = nvalue;
            ncp += 4;
        } else {
            *ncp++ = 0;
            *ncp++ = b;
            *ncp++ = g;
            *ncp++ = r;
        }
    }
    return rv;
}

static PyObject *
imageop_rgb2grey(PyObject *self, PyObject *args)
{
    int x, y, len, nlen;
    unsigned char *cp;
    unsigned char *ncp;
    PyObject *rv;
    int i, r, g, b;
    int nvalue;
    int backward_compatible = imageop_backward_compatible();

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply_size(len, x, "x", y, "y", 4) )
        return 0;
    nlen = x*y;
    if ( !check_multiply(nlen, x, y) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, nlen);
    if ( rv == 0 )
        return 0;
    ncp = (unsigned char *)PyString_AsString(rv);

    for ( i=0; i < nlen; i++ ) {
        if (backward_compatible) {
            Py_UInt32 value = * (Py_UInt32 *) cp;
            cp += 4;
            r = (int) ((value & 0xff) / 255. * 7. + .5);
            g = (int) (((value >> 8) & 0xff) / 255. * 7. + .5);
            b = (int) (((value >> 16) & 0xff) / 255. * 3. + .5);
        } else {
            cp++;                       /* skip alpha channel */
            b = *cp++;
            g = *cp++;
            r = *cp++;
        }
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
    unsigned char *ncp;
    PyObject *rv;
    int i;
    unsigned char value;
    int backward_compatible = imageop_backward_compatible();

    if ( !PyArg_ParseTuple(args, "s#ii", &cp, &len, &x, &y) )
        return 0;

    if ( !check_multiply(len, x, y) )
        return 0;
    nlen = x*y*4;
    if ( !check_multiply_size(nlen, x, "x", y, "y", 4) )
        return 0;

    rv = PyString_FromStringAndSize(NULL, nlen);
    if ( rv == 0 )
        return 0;
    ncp = (unsigned char *)PyString_AsString(rv);

    for ( i=0; i < len; i++ ) {
        value = *cp++;
        if (backward_compatible) {
            * (Py_UInt32 *) ncp = (Py_UInt32) value | ((Py_UInt32) value << 8 ) | ((Py_UInt32) value << 16);
            ncp += 4;
        } else {
            *ncp++ = 0;
            *ncp++ = value;
            *ncp++ = value;
            *ncp++ = value;
        }
    }
    return rv;
}

static PyMethodDef imageop_methods[] = {
    { "crop",                   imageop_crop, METH_VARARGS },
    { "scale",                  imageop_scale, METH_VARARGS },
    { "grey2mono",              imageop_grey2mono, METH_VARARGS },
    { "grey2grey2",             imageop_grey2grey2, METH_VARARGS },
    { "grey2grey4",             imageop_grey2grey4, METH_VARARGS },
    { "dither2mono",            imageop_dither2mono, METH_VARARGS },
    { "dither2grey2",           imageop_dither2grey2, METH_VARARGS },
    { "mono2grey",              imageop_mono2grey, METH_VARARGS },
    { "grey22grey",             imageop_grey22grey, METH_VARARGS },
    { "grey42grey",             imageop_grey42grey, METH_VARARGS },
    { "tovideo",                imageop_tovideo, METH_VARARGS },
    { "rgb2rgb8",               imageop_rgb2rgb8, METH_VARARGS },
    { "rgb82rgb",               imageop_rgb82rgb, METH_VARARGS },
    { "rgb2grey",               imageop_rgb2grey, METH_VARARGS },
    { "grey2rgb",               imageop_grey2rgb, METH_VARARGS },
    { 0,                    0 }
};


PyMODINIT_FUNC
initimageop(void)
{
    PyObject *m;

    if (PyErr_WarnPy3k("the imageop module has been removed in "
                       "Python 3.0", 2) < 0)
        return;

    m = Py_InitModule("imageop", imageop_methods);
    if (m == NULL)
        return;
    ImageopDict = PyModule_GetDict(m);
    ImageopError = PyErr_NewException("imageop.error", NULL, NULL);
    if (ImageopError != NULL)
        PyDict_SetItemString(ImageopDict, "error", ImageopError);
}
