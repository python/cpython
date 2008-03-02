/*
 *   	fastimg -
 *		Faster reading and writing of image files.
 *
 *      This code should work on machines with any byte order.
 *
 *	Could someone make this run real fast using multiple processors 
 *	or how about using memory mapped files to speed it up?
 *
 *				Paul Haeberli - 1991
 *
 *	Changed to return sizes.
 *				Sjoerd Mullender - 1993
 *	Changed to incorporate into Python.
 *				Sjoerd Mullender - 1993
 */
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

#include <string.h>

/*
 *	from image.h
 *
 */
typedef struct {
	unsigned short	imagic;		/* stuff saved on disk . . */
	unsigned short 	type;
	unsigned short 	dim;
	unsigned short 	xsize;
	unsigned short 	ysize;
	unsigned short 	zsize;
	Py_UInt32 	min;
	Py_UInt32 	max;
	Py_UInt32	wastebytes;	
	char 		name[80];
	Py_UInt32	colormap;

	Py_Int32	file;		/* stuff used in core only */
	unsigned short 	flags;
	short		dorev;
	short		x;
	short		y;
	short		z;
	short		cnt;
	unsigned short	*ptr;
	unsigned short	*base;
	unsigned short	*tmpbuf;
	Py_UInt32	offset;
	Py_UInt32	rleend;		/* for rle images */
	Py_UInt32	*rowstart;	/* for rle images */
	Py_Int32	*rowsize;	/* for rle images */
} IMAGE;

#define IMAGIC 	0732

#define TYPEMASK		0xff00
#define BPPMASK			0x00ff
#define ITYPE_VERBATIM		0x0000
#define ITYPE_RLE		0x0100
#define ISRLE(type)		(((type) & 0xff00) == ITYPE_RLE)
#define ISVERBATIM(type)	(((type) & 0xff00) == ITYPE_VERBATIM)
#define BPP(type)		((type) & BPPMASK)
#define RLE(bpp)		(ITYPE_RLE | (bpp))
#define VERBATIM(bpp)		(ITYPE_VERBATIM | (bpp))
/*
 *	end of image.h stuff
 *
 */

#define RINTLUM (79)
#define GINTLUM (156)
#define BINTLUM (21)

#define ILUM(r,g,b)     ((int)(RINTLUM*(r)+GINTLUM*(g)+BINTLUM*(b))>>8)

#define OFFSET_R	3	/* this is byte order dependent */
#define OFFSET_G	2
#define OFFSET_B	1
#define OFFSET_A	0

#define CHANOFFSET(z)	(3-(z))	/* this is byte order dependent */

static void expandrow(unsigned char *, unsigned char *, int);
static void setalpha(unsigned char *, int);
static void copybw(Py_Int32 *, int);
static void interleaverow(unsigned char*, unsigned char*, int, int);
static int compressrow(unsigned char *, unsigned char *, int, int);
static void lumrow(unsigned char *, unsigned char *, int);

#ifdef ADD_TAGS
#define TAGLEN	(5)
#else
#define TAGLEN	(0)
#endif

static PyObject *ImgfileError;

static int reverse_order;

#ifdef ADD_TAGS
/*
 *	addlongimgtag - 
 *		this is used to extract image data from core dumps.
 *
 */
static void 
addlongimgtag(Py_UInt32 *dptr, int xsize, int ysize)
{
	dptr = dptr + (xsize * ysize);
	dptr[0] = 0x12345678;
	dptr[1] = 0x59493333;
	dptr[2] = 0x69434222;
	dptr[3] = xsize;
	dptr[4] = ysize;
}
#endif

/*
 *	byte order independent read/write of shorts and longs.
 *
 */
static unsigned short
getshort(FILE *inf)
{
	unsigned char buf[2];

	fread(buf, 2, 1, inf);
	return (buf[0] << 8) + (buf[1] << 0);
}

static Py_UInt32
getlong(FILE *inf)
{
	unsigned char buf[4];

	fread(buf, 4, 1, inf);
	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + (buf[3] << 0);
}

static void
putshort(FILE *outf, unsigned short val)
{
	unsigned char buf[2];

	buf[0] = (val >> 8);
	buf[1] = (val >> 0);
	fwrite(buf, 2, 1, outf);
}

static int
putlong(FILE *outf, Py_UInt32 val)
{
	unsigned char buf[4];

	buf[0] = (unsigned char) (val >> 24);
	buf[1] = (unsigned char) (val >> 16);
	buf[2] = (unsigned char) (val >> 8);
	buf[3] = (unsigned char) (val >> 0);
	return fwrite(buf, 4, 1, outf);
}

static void
readheader(FILE *inf, IMAGE *image)
{
	memset(image ,0, sizeof(IMAGE));
	image->imagic = getshort(inf);
	image->type   = getshort(inf);
	image->dim    = getshort(inf);
	image->xsize  = getshort(inf);
	image->ysize  = getshort(inf);
	image->zsize  = getshort(inf);
}

static int
writeheader(FILE *outf, IMAGE *image)
{
	IMAGE t;

	memset(&t, 0, sizeof(IMAGE));
	fwrite(&t, sizeof(IMAGE), 1, outf);
	fseek(outf, 0, SEEK_SET);
	putshort(outf, image->imagic);
	putshort(outf, image->type);
	putshort(outf, image->dim);
	putshort(outf, image->xsize);
	putshort(outf, image->ysize);
	putshort(outf, image->zsize);
	putlong(outf, image->min);
	putlong(outf, image->max);
	putlong(outf, 0);
	return fwrite("no name", 8, 1, outf);
}

static int
writetab(FILE *outf, /*unsigned*/ Py_Int32 *tab, int len)
{
	int r = 0;

	while(len) {
		r = putlong(outf, *tab++);
		len--;
	}
	return r;
}

static void
readtab(FILE *inf, /*unsigned*/ Py_Int32 *tab, int len)
{
	while(len) {
		*tab++ = getlong(inf);
		len--;
	}
}

/*
 *	sizeofimage - 
 *		return the xsize and ysize of an iris image file.
 *
 */
static PyObject *
sizeofimage(PyObject *self, PyObject *args)
{
	char *name;
	IMAGE image;
	FILE *inf;

	if (!PyArg_ParseTuple(args, "s:sizeofimage", &name))
		return NULL;

	inf = fopen(name, "rb");
	if (!inf) {
		PyErr_SetString(ImgfileError, "can't open image file");
		return NULL;
	}
	readheader(inf, &image);
	fclose(inf);
	if (image.imagic != IMAGIC) {
		PyErr_SetString(ImgfileError,
				"bad magic number in image file");
		return NULL;
	}
	return Py_BuildValue("(ii)", image.xsize, image.ysize);
}

/*
 *	longimagedata - 
 *		read in a B/W RGB or RGBA iris image file and return a 
 *	pointer to an array of longs.
 *
 */
static PyObject *
longimagedata(PyObject *self, PyObject *args)
{
	char *name;
	unsigned char *base, *lptr;
	unsigned char *rledat = NULL, *verdat = NULL;
	Py_Int32 *starttab = NULL, *lengthtab = NULL;
	FILE *inf = NULL;
	IMAGE image;
	int y, z, tablen, new_size;
	int xsize, ysize, zsize;
	int bpp, rle, cur, badorder;
	int rlebuflen;
	PyObject *rv = NULL;

	if (!PyArg_ParseTuple(args, "s:longimagedata", &name))
		return NULL;

	inf = fopen(name,"rb");
	if (!inf) {
		PyErr_SetString(ImgfileError, "can't open image file");
		return NULL;
	}
	readheader(inf,&image);
	if (image.imagic != IMAGIC) {
		PyErr_SetString(ImgfileError,
				"bad magic number in image file");
		goto finally;
	}
	rle = ISRLE(image.type);
	bpp = BPP(image.type);
	if (bpp != 1) {
		PyErr_SetString(ImgfileError,
				"image must have 1 byte per pix chan");
		goto finally;
	}
	xsize = image.xsize;
	ysize = image.ysize;
	zsize = image.zsize;
	if (rle) {
		tablen = ysize * zsize * sizeof(Py_Int32);
		rlebuflen = (int) (1.05 * xsize +10);
		if ((tablen / sizeof(Py_Int32)) != (ysize * zsize) ||
		    rlebuflen < 0) {
			PyErr_NoMemory();
			goto finally;
		}

		starttab = (Py_Int32 *)malloc(tablen);
		lengthtab = (Py_Int32 *)malloc(tablen);
		rledat = (unsigned char *)malloc(rlebuflen);
		if (!starttab || !lengthtab || !rledat) {
			PyErr_NoMemory();
			goto finally;
		}
		
		fseek(inf, 512, SEEK_SET);
		readtab(inf, starttab, ysize*zsize);
		readtab(inf, lengthtab, ysize*zsize);

		/* check data order */
		cur = 0;
		badorder = 0;
		for(y = 0; y < ysize; y++) {
			for(z = 0; z < zsize; z++) {
				if (starttab[y + z * ysize] < cur) {
					badorder = 1;
					break;
				}
				cur = starttab[y +z * ysize];
			}
			if (badorder)
				break;
		}

		fseek(inf, 512 + 2 * tablen, SEEK_SET);
		cur = 512 + 2 * tablen;
		new_size = xsize * ysize + TAGLEN;
		if (new_size < 0 || (new_size * sizeof(Py_Int32)) < 0) {
			PyErr_NoMemory();
			goto finally;
		}

		rv = PyString_FromStringAndSize((char *)NULL,
				      new_size * sizeof(Py_Int32));
		if (rv == NULL)
			goto finally;

		base = (unsigned char *) PyString_AsString(rv);
#ifdef ADD_TAGS
		addlongimgtag(base,xsize,ysize);
#endif
		if (badorder) {
			for (z = 0; z < zsize; z++) {
				lptr = base;
				if (reverse_order)
					lptr += (ysize - 1) * xsize
						* sizeof(Py_UInt32);
				for (y = 0; y < ysize; y++) {
					int idx = y + z * ysize;
					if (cur != starttab[idx]) {
						fseek(inf,starttab[idx],
						      SEEK_SET);
						cur = starttab[idx];
					}
					if (lengthtab[idx] > rlebuflen) {
						PyErr_SetString(ImgfileError,
							"rlebuf is too small");
						Py_DECREF(rv);
						rv = NULL;
						goto finally;
					}
					fread(rledat, lengthtab[idx], 1, inf);
					cur += lengthtab[idx];
					expandrow(lptr, rledat, 3-z);
					if (reverse_order)
						lptr -= xsize
						      * sizeof(Py_UInt32);
					else
						lptr += xsize
						      * sizeof(Py_UInt32);
				}
			}
		} else {
			lptr = base;
			if (reverse_order)
				lptr += (ysize - 1) * xsize
					* sizeof(Py_UInt32);
			for (y = 0; y < ysize; y++) {
				for(z = 0; z < zsize; z++) {
					int idx = y + z * ysize;
					if (cur != starttab[idx]) {
						fseek(inf, starttab[idx],
						      SEEK_SET);
						cur = starttab[idx];
					}
					fread(rledat, lengthtab[idx], 1, inf);
					cur += lengthtab[idx];
					expandrow(lptr, rledat, 3-z);
				}
				if (reverse_order)
					lptr -= xsize * sizeof(Py_UInt32);
				else
					lptr += xsize * sizeof(Py_UInt32);
			}
		}
		if (zsize == 3) 
			setalpha(base, xsize * ysize);
		else if (zsize < 3) 
			copybw((Py_Int32 *) base, xsize * ysize);
	}
	else {
		new_size = xsize * ysize + TAGLEN;
		if (new_size < 0 || (new_size * sizeof(Py_Int32)) < 0) {
			PyErr_NoMemory();
			goto finally;
		}

		rv = PyString_FromStringAndSize((char *) 0,
						new_size*sizeof(Py_Int32));
		if (rv == NULL)
			goto finally;

		base = (unsigned char *) PyString_AsString(rv);
#ifdef ADD_TAGS
		addlongimgtag(base, xsize, ysize);
#endif
		verdat = (unsigned char *)malloc(xsize);
		fseek(inf, 512, SEEK_SET);
		for (z = 0; z < zsize; z++) {
			lptr = base;
			if (reverse_order)
				lptr += (ysize - 1) * xsize
				        * sizeof(Py_UInt32);
			for (y = 0; y < ysize; y++) {
				fread(verdat, xsize, 1, inf);
				interleaverow(lptr, verdat, 3-z, xsize);
				if (reverse_order)
					lptr -= xsize * sizeof(Py_UInt32);
				else
					lptr += xsize * sizeof(Py_UInt32);
			}
		}
		if (zsize == 3)
			setalpha(base, xsize * ysize);
		else if (zsize < 3) 
			copybw((Py_Int32 *) base, xsize * ysize);
	}
  finally:
	free(starttab);
	free(lengthtab);
	free(rledat);
	free(verdat);
	fclose(inf);
	return rv;
}

/* static utility functions for longimagedata */

static void
interleaverow(unsigned char *lptr, unsigned char *cptr, int z, int n)
{
	lptr += z;
	while (n--) {
		*lptr = *cptr++;
		lptr += 4;
	}
}

static void
copybw(Py_Int32 *lptr, int n)
{
	while (n >= 8) {
		lptr[0] = 0xff000000 + (0x010101 * (lptr[0] & 0xff));
		lptr[1] = 0xff000000 + (0x010101 * (lptr[1] & 0xff));
		lptr[2] = 0xff000000 + (0x010101 * (lptr[2] & 0xff));
		lptr[3] = 0xff000000 + (0x010101 * (lptr[3] & 0xff));
		lptr[4] = 0xff000000 + (0x010101 * (lptr[4] & 0xff));
		lptr[5] = 0xff000000 + (0x010101 * (lptr[5] & 0xff));
		lptr[6] = 0xff000000 + (0x010101 * (lptr[6] & 0xff));
		lptr[7] = 0xff000000 + (0x010101 * (lptr[7] & 0xff));
		lptr += 8;
		n -= 8;
	}
	while (n--) {
		*lptr = 0xff000000 + (0x010101 * (*lptr&0xff));
		lptr++;
	}
}

static void
setalpha(unsigned char *lptr, int n)
{
	while (n >= 8) {
		lptr[0 * 4] = 0xff;
		lptr[1 * 4] = 0xff;
		lptr[2 * 4] = 0xff;
		lptr[3 * 4] = 0xff;
		lptr[4 * 4] = 0xff;
		lptr[5 * 4] = 0xff;
		lptr[6 * 4] = 0xff;
		lptr[7 * 4] = 0xff;
		lptr += 4 * 8;
		n -= 8;
	}
	while (n--) {
		*lptr = 0xff;
		lptr += 4;
	}
}

static void
expandrow(unsigned char *optr, unsigned char *iptr, int z)
{
	unsigned char pixel, count;

	optr += z;
	while (1) {
		pixel = *iptr++;
		if (!(count = (pixel & 0x7f)))
			return;
		if (pixel & 0x80) {
			while (count >= 8) {
				optr[0 * 4] = iptr[0];
				optr[1 * 4] = iptr[1];
				optr[2 * 4] = iptr[2];
				optr[3 * 4] = iptr[3];
				optr[4 * 4] = iptr[4];
				optr[5 * 4] = iptr[5];
				optr[6 * 4] = iptr[6];
				optr[7 * 4] = iptr[7];
				optr += 8 * 4;
				iptr += 8;
				count -= 8;
			}
			while (count--) {
				*optr = *iptr++;
				optr += 4;
			}
		}
		else {
			pixel = *iptr++;
			while (count >= 8) {
				optr[0 * 4] = pixel;
				optr[1 * 4] = pixel;
				optr[2 * 4] = pixel;
				optr[3 * 4] = pixel;
				optr[4 * 4] = pixel;
				optr[5 * 4] = pixel;
				optr[6 * 4] = pixel;
				optr[7 * 4] = pixel;
				optr += 8 * 4;
				count -= 8;
			}
			while (count--) {
				*optr = pixel;
				optr += 4;
			}
		}
	}
}

/*
 *	longstoimage -
 *		copy an array of longs to an iris image file.  Each long
 *	represents one pixel.  xsize and ysize specify the dimensions of
 *	the pixel array.  zsize specifies what kind of image file to
 *	write out.  if zsize is 1, the luminance of the pixels are
 *	calculated, and a single channel black and white image is saved.
 *	If zsize is 3, an RGB image file is saved.  If zsize is 4, an
 *	RGBA image file is saved.
 *
 */
static PyObject *
longstoimage(PyObject *self, PyObject *args)
{
	unsigned char *lptr;
	char *name;
	int xsize, ysize, zsize;
	FILE *outf = NULL;
	IMAGE image;
	int tablen, y, z, pos, len;
	Py_Int32 *starttab = NULL, *lengthtab = NULL;
	unsigned char *rlebuf = NULL;
	unsigned char *lumbuf = NULL;
	int rlebuflen, goodwrite;
	PyObject *retval = NULL;

	if (!PyArg_ParseTuple(args, "s#iiis:longstoimage", &lptr, &len,
			      &xsize, &ysize, &zsize, &name))
		return NULL;

	goodwrite = 1;
	outf = fopen(name, "wb");
	if (!outf) {
		PyErr_SetString(ImgfileError, "can't open output file");
		return NULL;
	}
	tablen = ysize * zsize * sizeof(Py_Int32);
	rlebuflen = (int) (1.05 * xsize + 10);

	if ((tablen / sizeof(Py_Int32)) != (ysize * zsize) ||
	    rlebuflen < 0 || (xsize * sizeof(Py_Int32)) < 0) {
		PyErr_NoMemory();
		goto finally;
	}

	starttab = (Py_Int32 *)malloc(tablen);
	lengthtab = (Py_Int32 *)malloc(tablen);
	rlebuf = (unsigned char *)malloc(rlebuflen);
	lumbuf = (unsigned char *)malloc(xsize * sizeof(Py_Int32));
	if (!starttab || !lengthtab || !rlebuf || !lumbuf) {
		PyErr_NoMemory();
		goto finally;
	}
	
	memset(&image, 0, sizeof(IMAGE));
	image.imagic = IMAGIC; 
	image.type = RLE(1);
	if (zsize>1)
		image.dim = 3;
	else
		image.dim = 2;
	image.xsize = xsize;
	image.ysize = ysize;
	image.zsize = zsize;
	image.min = 0;
	image.max = 255;
	goodwrite *= writeheader(outf, &image);
	pos = 512 + 2 * tablen;
	fseek(outf, pos, SEEK_SET);
	if (reverse_order)
		lptr += (ysize - 1) * xsize * sizeof(Py_UInt32);
	for (y = 0; y < ysize; y++) {
		for (z = 0; z < zsize; z++) {
			if (zsize == 1) {
				lumrow(lptr, lumbuf, xsize);
				len = compressrow(lumbuf, rlebuf,
						  CHANOFFSET(z), xsize);
			} else {
				len = compressrow(lptr, rlebuf,
						  CHANOFFSET(z), xsize);
			}
			if(len > rlebuflen) {
				PyErr_SetString(ImgfileError,
						"rlebuf is too small");
				goto finally;
			}
			goodwrite *= fwrite(rlebuf, len, 1, outf);
			starttab[y + z * ysize] = pos;
			lengthtab[y + z * ysize] = len;
			pos += len;
		}
		if (reverse_order)
			lptr -= xsize * sizeof(Py_UInt32);
		else
			lptr += xsize * sizeof(Py_UInt32);
	}

	fseek(outf, 512, SEEK_SET);
	goodwrite *= writetab(outf, starttab, ysize*zsize);
	goodwrite *= writetab(outf, lengthtab, ysize*zsize);
	if (goodwrite) {
		Py_INCREF(Py_None);
		retval = Py_None;
	} else
		PyErr_SetString(ImgfileError, "not enough space for image");

  finally:
	fclose(outf);
	free(starttab);
	free(lengthtab);
	free(rlebuf);
	free(lumbuf);
	return retval;
}

/* static utility functions for longstoimage */

static void
lumrow(unsigned char *rgbptr, unsigned char *lumptr, int n)
{
	lumptr += CHANOFFSET(0);
	while (n--) {
		*lumptr = ILUM(rgbptr[OFFSET_R],
			       rgbptr[OFFSET_G],
			       rgbptr[OFFSET_B]);
		lumptr += 4;
		rgbptr += 4;
	}
}

static int
compressrow(unsigned char *lbuf, unsigned char *rlebuf, int z, int cnt)
{
	unsigned char *iptr, *ibufend, *sptr, *optr;
	short todo, cc;							
	Py_Int32 count;

	lbuf += z;
	iptr = lbuf;
	ibufend = iptr + cnt * 4;
	optr = rlebuf;

	while(iptr < ibufend) {
		sptr = iptr;
		iptr += 8;
		while ((iptr<ibufend) &&
		       ((iptr[-8]!=iptr[-4]) ||(iptr[-4]!=iptr[0])))
		{
			iptr += 4;
		}
		iptr -= 8;
		count = (iptr - sptr) / 4;
		while (count) {
			todo = count > 126 ? 126 : (short)count;
			count -= todo;
			*optr++ = 0x80 | todo;
			while (todo > 8) {
				optr[0] = sptr[0 * 4];
				optr[1] = sptr[1 * 4];
				optr[2] = sptr[2 * 4];
				optr[3] = sptr[3 * 4];
				optr[4] = sptr[4 * 4];
				optr[5] = sptr[5 * 4];
				optr[6] = sptr[6 * 4];
				optr[7] = sptr[7 * 4];
				optr += 8;
				sptr += 8 * 4;
				todo -= 8;
			}
			while (todo--) {
				*optr++ = *sptr;
				sptr += 4;
			}
		}
		sptr = iptr;
		cc = *iptr;
		iptr += 4;
		while ((iptr < ibufend) && (*iptr == cc))
			iptr += 4;
		count = (iptr - sptr) / 4;
		while (count) {
			todo = count > 126 ? 126 : (short)count;
			count -= todo;
			*optr++ = (unsigned char) todo;
			*optr++ = (unsigned char) cc;
		}
	}
	*optr++ = 0;
	return optr - (unsigned char *)rlebuf;
}

static PyObject *
ttob(PyObject *self, PyObject *args)
{
	int order, oldorder;

	if (!PyArg_ParseTuple(args, "i:ttob", &order))
		return NULL;
	oldorder = reverse_order;
	reverse_order = order;
	return PyInt_FromLong(oldorder);
}

static PyMethodDef
rgbimg_methods[] = {
	{"sizeofimage",	   sizeofimage, METH_VARARGS},
	{"longimagedata",  longimagedata, METH_VARARGS},
	{"longstoimage",   longstoimage, METH_VARARGS},
	{"ttob",	   ttob, METH_VARARGS},
	{NULL,             NULL}	     /* sentinel */
};


PyMODINIT_FUNC
initrgbimg(void)
{
	PyObject *m, *d;
	m = Py_InitModule("rgbimg", rgbimg_methods);
	d = PyModule_GetDict(m);
	ImgfileError = PyErr_NewException("rgbimg.error", NULL, NULL);
	if (ImgfileError != NULL)
		PyDict_SetItemString(d, "error", ImgfileError);
}
