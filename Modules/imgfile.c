/***********************************************************
Copyright 1991, 1992, 1993 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior permission.

STICHTING MATHEMATISCH CENTRUM DISCLAIMS ALL WARRANTIES WITH REGARD TO
THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH CENTRUM BE LIABLE
FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/* IMGFILE module - Interface to sgi libimage */

/* XXX This modele should be done better at some point. It should return
** an object of image file class, and have routines to manipulate these
** image files in a neater way (so you can get rgb images off a greyscale
** file, for instance, or do a straight display without having to get the
** image bits into python, etc).
**
** Warning: this module is very non-reentrant (esp. the readscaled stuff)
*/

#include "allobjects.h"
#include "modsupport.h"

#include <gl/image.h>
#include <errno.h>

#include "/usr/people/4Dgifts/iristools/include/izoom.h"

static object * ImgfileError; /* Exception we raise for various trouble */


/* The image library does not always call the error hander :-(,
   therefore we have a global variable indicating that it was called.
   It is cleared by imgfile_open(). */

static int error_called;


/* The error handler */

static imgfile_error(str)
    char *str;
{
    err_setstr(ImgfileError, str);
    error_called = 1;
    return;	/* To imglib, which will return a failure indictaor */
}


/* Open an image file and return a pointer to it.
   Make sure we raise an exception if we fail. */

static IMAGE *
imgfile_open(fname)
    char *fname;
{
    IMAGE *image;
    i_seterror(imgfile_error);
    error_called = 0;
    errno = 0;
    if ( (image = iopen(fname, "r")) == NULL ) {
	/* Error may already be set by imgfile_error */
	if ( !error_called ) {
	    if (errno)
	        err_errno(ImgfileError);
	    else
	        err_setstr(ImgfileError, "Can't open image file");
        }
	return NULL;
    }
    return image;
}


static object *
imgfile_read(self, args)
    object *self;
    object *args;
{
    char *fname;
    object *rv;
    int xsize, ysize, zsize;
    char *cdatap;
    long *idatap;
    static short rs[8192], gs[8192], bs[8192];
    int x, y;
    IMAGE *image;
    
    if ( !getargs(args, "s", &fname) )
      return NULL;
    
    if ( (image = imgfile_open(fname)) == NULL )
      return NULL;
    
    if ( image->colormap != CM_NORMAL ) {
	iclose(image);
	err_setstr(ImgfileError, "Can only handle CM_NORMAL images");
	return NULL;
    }
    if ( BPP(image->type) != 1 ) {
	iclose(image);
	err_setstr(ImgfileError, "Can't handle imgfiles with bpp!=1");
	return NULL;
    }
    xsize = image->xsize;
    ysize = image->ysize;
    zsize = image->zsize;
    if ( zsize != 1 && zsize != 3) {
	iclose(image);
	err_setstr(ImgfileError, "Can only handle 1 or 3 byte pixels");
	return NULL;
    }
    if ( xsize > 8192 ) {
	iclose(image);
	err_setstr(ImgfileError, "Can't handle image with > 8192 columns");
	return NULL;
    }

    if ( zsize == 3 ) zsize = 4;
    rv = newsizedstringobject((char *)NULL, xsize*ysize*zsize);
    if ( rv == NULL ) {
      iclose(image);
      return NULL;
    }
    cdatap = getstringvalue(rv);
    idatap = (long *)cdatap;
    for ( y=0; y < ysize && !error_called; y++ ) {
	if ( zsize == 1 ) {
	    getrow(image, rs, y, 0);
	    for(x=0; x<xsize; x++ )
	      *cdatap++ = rs[x];
	} else {
	    getrow(image, rs, y, 0);
	    getrow(image, gs, y, 1);
	    getrow(image, bs, y, 2);
	    for(x=0; x<xsize; x++ )
	      *idatap++ = (rs[x] & 0xff)     |
		         ((gs[x] & 0xff)<<8) |
		         ((bs[x] & 0xff)<<16);
	}
    }
    iclose(image);
    if ( error_called ) {
	DECREF(rv);
	return NULL;
    }
    return rv;
}

IMAGE *glob_image;
long *glob_datap;
int glob_width, glob_z;

static
xs_get(buf, y)
    short *buf;
    int y;
{
    getrow(glob_image, buf, y, glob_z);
}

static
xs_put_c(buf, y)
    short *buf;
    int y;
{
    char *datap = (char *)glob_datap + y*glob_width;
    int width = glob_width;

    while ( width-- )
      *datap++ = (*buf++) & 0xff;
}

static
xs_put_0(buf, y)
    short *buf;
    int y;
{
    long *datap = glob_datap + y*glob_width;
    int width = glob_width;

    while ( width-- )
      *datap++ = (*buf++) & 0xff;
}
static
xs_put_12(buf, y)
    short *buf;
    int y;
{
    long *datap = glob_datap + y*glob_width;
    int width = glob_width;

    while ( width-- )
      *datap++ |= ((*buf++) & 0xff) << (glob_z*8);
}

static void
xscale(image, xsize, ysize, zsize, datap, xnew, ynew, fmode, blur)
    IMAGE *image;
    int xsize, ysize, zsize;
    long *datap;
    int xnew, ynew;
    int fmode;
    double blur;
{
    glob_image = image;
    glob_datap = datap;
    glob_width = xnew;
    if ( zsize == 1 ) {
	glob_z = 0;
	filterzoom(xs_get, xs_put_c, xsize, ysize, xnew, ynew, fmode, blur);
    } else {
	glob_z = 0;
	filterzoom(xs_get, xs_put_0, xsize, ysize, xnew, ynew, fmode, blur);
	glob_z = 1;
	filterzoom(xs_get, xs_put_12, xsize, ysize, xnew, ynew, fmode, blur);
	glob_z = 2;
	filterzoom(xs_get, xs_put_12, xsize, ysize, xnew, ynew, fmode, blur);
    }
}


static object *
imgfile_readscaled(self, args)
    object *self;
    object *args;
{
    char *fname;
    object *rv;
    int xsize, ysize, zsize;
    char *cdatap;
    long *idatap;
    static short rs[8192], gs[8192], bs[8192];
    int x, y;
    int xwtd, ywtd, xorig, yorig;
    float xfac, yfac;
    int cnt;
    IMAGE *image;
    char *filter;
    double blur;
    int extended;
    int fmode;

    /*
     ** Parse args. Funny, since arg 4 and 5 are optional
     ** (filter name and blur factor). Also, 4 or 5 arguments indicates
     ** extended scale algorithm in stead of simple-minded pixel drop/dup.
     */
    extended = 0;
    cnt = gettuplesize(args);
    if ( cnt == 5 ) {
	extended = 1;
	if ( !getargs(args, "(siisd)", &fname, &xwtd, &ywtd, &filter, &blur) )
	  return NULL;
    } else if ( cnt == 4 ) {
	extended = 1;
	if ( !getargs(args, "(siis)", &fname, &xwtd, &ywtd, &filter) )
	  return NULL;
	blur = 1.0;
    } else if ( !getargs(args, "(sii)", &fname, &xwtd, &ywtd) )
      return NULL;

    /*
     ** Check parameters, open file and check type, rows, etc.
     */
    if ( extended ) {
	if ( strcmp(filter, "impulse") == 0 ) fmode = IMPULSE;
	else if ( strcmp( filter, "box") == 0 ) fmode = BOX;
	else if ( strcmp( filter, "triangle") == 0 ) fmode = TRIANGLE;
	else if ( strcmp( filter, "quadratic") == 0 ) fmode = QUADRATIC;
	else if ( strcmp( filter, "gaussian") == 0 ) fmode = GAUSSIAN;
	else {
	    err_setstr(ImgfileError, "Unknown filter type");
	    return NULL;
	}
    }
    
    if ( (image = imgfile_open(fname)) == NULL )
      return NULL;
    
    if ( image->colormap != CM_NORMAL ) {
	iclose(image);
	err_setstr(ImgfileError, "Can only handle CM_NORMAL images");
	return NULL;
    }
    if ( BPP(image->type) != 1 ) {
	iclose(image);
	err_setstr(ImgfileError, "Can't handle imgfiles with bpp!=1");
	return NULL;
    }
    xsize = image->xsize;
    ysize = image->ysize;
    zsize = image->zsize;
    if ( zsize != 1 && zsize != 3) {
	iclose(image);
	err_setstr(ImgfileError, "Can only handle 1 or 3 byte pixels");
	return NULL;
    }
    if ( xsize > 8192 ) {
	iclose(image);
	err_setstr(ImgfileError, "Can't handle image with > 8192 columns");
	return NULL;
    }

    if ( zsize == 3 ) zsize = 4;
    rv = newsizedstringobject(NULL, xwtd*ywtd*zsize);
    if ( rv == NULL ) {
	iclose(image);
	return NULL;
    }
    xfac = (float)xsize/(float)xwtd;
    yfac = (float)ysize/(float)ywtd;
    cdatap = getstringvalue(rv);
    idatap = (long *)cdatap;

    if ( extended ) {
	xscale(image, xsize, ysize, zsize, idatap, xwtd, ywtd, fmode, blur);
    } else {
	for ( y=0; y < ywtd && !error_called; y++ ) {
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
		    *idatap++ = (rs[xorig] & 0xff)     |
		      ((gs[xorig] & 0xff)<<8) |
			((bs[xorig] & 0xff)<<16);
		}
	    }
	}
    }
    iclose(image);
    if ( error_called ) {
	DECREF(rv);
	return NULL;
    }
    return rv;
}

static object *
imgfile_getsizes(self, args)
    object *self;
    object *args;
{
    char *fname;
    object *rv;
    IMAGE *image;
    
    if ( !getargs(args, "s", &fname) )
      return NULL;
    
    if ( (image = imgfile_open(fname)) == NULL )
      return NULL;
    rv = mkvalue("(iii)", image->xsize, image->ysize, image->zsize);
    iclose(image);
    return rv;
}

static object *
imgfile_write(self, args)
    object *self;
    object *args;
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

    if ( !getargs(args, "(ss#iii)",
		  &fname, &cdatap, &len, &xsize, &ysize, &zsize) )
      return NULL;
    
    if ( zsize != 1 && zsize != 3 ) {
	err_setstr(ImgfileError, "Can only handle 1 or 3 byte pixels");
	return NULL;
    }
    if ( len != xsize * ysize * (zsize == 1 ? 1 : 4) ) {
	err_setstr(ImgfileError, "Data does not match sizes");
	return NULL;
    }
    if ( xsize > 8192 ) {
	err_setstr(ImgfileError, "Can't handle image with > 8192 columns");
	return NULL;
    }

    error_called = 0;
    errno = 0;
    image =iopen(fname, "w", RLE(1), 3, xsize, ysize, zsize);
    if ( image == 0 ) {
	if ( ! error_called ) {
	    if (errno)
		err_errno(ImgfileError);
	    else
	        err_setstr(ImgfileError, "Can't create image file");
	}
	return NULL;
    }

    idatap = (long *)cdatap;
    
    for( y=0; y<ysize && !error_called; y++ ) {
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
    INCREF(None);
    return None;
    
}


static struct methodlist imgfile_methods[] = {
    { "getsizes",	imgfile_getsizes },
    { "read",		imgfile_read },
    { "readscaled",	imgfile_readscaled, 1},
    { "write",		imgfile_write },
    { NULL,		NULL } /* Sentinel */
};


void
initimgfile()
{
	object *m, *d;
	m = initmodule("imgfile", imgfile_methods);
	d = getmoduledict(m);
	ImgfileError = newstringobject("imgfile.error");
	if ( ImgfileError == NULL || dictinsert(d, "error", ImgfileError) )
	    fatal("can't define imgfile.error");
}
