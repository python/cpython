/***********************************************************
Copyright 1991, 1992 by Stichting Mathematisch Centrum, Amsterdam, The
Netherlands.

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

/* imageopmodule - Various operations on pictures */

#ifdef sun
#define signed
#endif

#include "allobjects.h"
#include "modsupport.h"

#define CHARP(cp, xmax, x, y) ((char *)(cp+y*xmax+x))
#define LONGP(cp, xmax, x, y) ((long *)(cp+4*(y*xmax+x)))

static object *ImageopError;
 
static object *
imageop_crop(self, args)
    object *self;
    object *args;
{
    char *cp, *ncp;
    long *nlp;
    int len, size, x, y, newx1, newx2, newy1, newy2;
    int ix, iy, xstep, ystep;
    object *rv;

    if ( !getargs(args, "(s#iiiiiii)", &cp, &len, &size, &x, &y,
		  &newx1, &newy1, &newx2, &newy2) )
      return 0;
    
    if ( size != 1 && size != 4 ) {
	err_setstr(ImageopError, "Size should be 1 or 4");
	return 0;
    }
    if ( len != size*x*y ) {
	err_setstr(ImageopError, "String has incorrect length");
	return 0;
    }
    xstep = (newx1 < newx2)? 1 : -1;
    ystep = (newy1 < newy2)? 1 : -1;
    
    rv = newsizedstringobject(NULL,
			      (abs(newx2-newx1)+1)*(abs(newy2-newy1)+1)*size);
    if ( rv == 0 )
      return 0;
    ncp = (char *)getstringvalue(rv);
    nlp = (long *)ncp;
    newy2 += ystep;
    newx2 += xstep;
    for( iy = newy1; iy != newy2; iy+=ystep ) {
	for ( ix = newx1; ix != newx2; ix+=xstep ) {
	    if ( iy < 0 || iy >= y || ix < 0 || ix >= x ) {
	      if ( size == 1 ) *ncp++ = 0;
	      else             *nlp++ = 0;
	    } else {
		if ( size == 1 ) *ncp++ = *CHARP(cp, x, ix, iy);
		else             *nlp++ = *LONGP(cp, x, ix, iy);
	    }
	}
    }
    return rv;
}
 
static object *
imageop_scale(self, args)
    object *self;
    object *args;
{
    char *cp, *ncp;
    long *nlp;
    int len, size, x, y, newx, newy;
    int ix, iy;
    int oix, oiy;
    object *rv;

    if ( !getargs(args, "(s#iiiii)", &cp, &len, &size, &x, &y, &newx, &newy) )
      return 0;
    
    if ( size != 1 && size != 4 ) {
	err_setstr(ImageopError, "Size should be 1 or 4");
	return 0;
    }
    if ( len != size*x*y ) {
	err_setstr(ImageopError, "String has incorrect length");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, newx*newy*size);
    if ( rv == 0 )
      return 0;
    ncp = (char *)getstringvalue(rv);
    nlp = (long *)ncp;
    for( iy = 0; iy < newy; iy++ ) {
	for ( ix = 0; ix < newx; ix++ ) {
	    oix = ix * x / newx;
	    oiy = iy * y / newy;
	    if ( size == 1 ) *ncp++ = *CHARP(cp, x, oix, oiy);
	    else             *nlp++ = *LONGP(cp, x, oix, oiy);
	}
    }
    return rv;
}

static object *
imageop_grey2mono(self, args)
    object *self;
    object *args;
{
    int tres, x, y, len;
    unsigned char *cp, *ncp;
    unsigned char ovalue;
    object *rv;
    int i, bit;
   
    
    if ( !getargs(args, "(s#iii)", &cp, &len, &x, &y, &tres) )
      return 0;

    if ( x*y != len ) {
	err_setstr(ImageopError, "String has incorrect length");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, (len+7)/8);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);

    bit = 0x80;
    ovalue = 0;
    for ( i=0; i < len; i++ ) {
	if ( cp[i] > tres )
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

static object *
imageop_dither2mono(self, args)
    object *self;
    object *args;
{
    int sum, x, y, len;
    unsigned char *cp, *ncp;
    unsigned char ovalue;
    object *rv;
    int i, bit;
   
    
    if ( !getargs(args, "(s#ii)", &cp, &len, &x, &y) )
      return 0;

    if ( x*y != len ) {
	err_setstr(ImageopError, "String has incorrect length");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, (len+7)/8);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);

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

static object *
imageop_mono2grey(self, args)
    object *self;
    object *args;
{
    int v0, v1, x, y, len, nlen;
    unsigned char *cp, *ncp;
    unsigned char ovalue;
    object *rv;
    int i, bit, value;
    
    if ( !getargs(args, "(s#iiii)", &cp, &len, &x, &y, &v0, &v1) )
      return 0;

    nlen = x*y;
    if ( (nlen+7)/8 != len ) {
	err_setstr(ImageopError, "String has incorrect length");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, nlen);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);

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

/*
static object *
imageop_mul(self, args)
    object *self;
    object *args;
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

static struct methodlist imageop_methods[] = {
    { "crop",		imageop_crop },
    { "scale",		imageop_scale },
    { "grey2mono",	imageop_grey2mono },
    { "dither2mono",	imageop_dither2mono },
    { "mono2grey",	imageop_mono2grey },
    { 0,          0 }
};


void
initimageop()
{
	object *m, *d;
	m = initmodule("imageop", imageop_methods);
	d = getmoduledict(m);
	ImageopError = newstringobject("imageop.error");
	if ( ImageopError == NULL || dictinsert(d,"error",ImageopError) )
	    fatal("can't define imageop.error");
}
