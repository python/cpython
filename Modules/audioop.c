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

/* audioopmodele - Module to detect peak values in arrays */

#ifdef sun
#define signed
#endif

#include <math.h>

#include "allobjects.h"
#include "modsupport.h"

/* Code shamelessly stealen from sox,
** (c) Craig Reese, Joe Campbell and Jeff Poskanzer 1989 */

#define MINLIN -32768
#define MAXLIN 32767
#define LINCLIP(x) do { if ( x < MINLIN ) x = MINLIN ; else if ( x > MAXLIN ) x = MAXLIN; } while ( 0 )

unsigned char st_linear_to_ulaw( /* int sample */ );

/*
** This macro converts from ulaw to 16 bit linear, faster.
**
** Jef Poskanzer
** 23 October 1989
**
** Input: 8 bit ulaw sample
** Output: signed 16 bit linear sample
*/
#define st_ulaw_to_linear(ulawbyte) ulaw_table[ulawbyte]

static int ulaw_table[256] = {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
	56,     48,     40,     32,     24,     16,      8,      0 };

#define ZEROTRAP    /* turn on the trap as per the MIL-STD */
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635

unsigned char
st_linear_to_ulaw( sample )
int sample;
    {
    static int exp_lut[256] = {0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,
                               4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
                               7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7};
    int sign, exponent, mantissa;
    unsigned char ulawbyte;

    /* Get the sample into sign-magnitude. */
    sign = (sample >> 8) & 0x80;		/* set aside the sign */
    if ( sign != 0 ) sample = -sample;		/* get magnitude */
    if ( sample > CLIP ) sample = CLIP;		/* clip the magnitude */

    /* Convert from 16 bit linear to ulaw. */
    sample = sample + BIAS;
    exponent = exp_lut[( sample >> 7 ) & 0xFF];
    mantissa = ( sample >> ( exponent + 3 ) ) & 0x0F;
    ulawbyte = ~ ( sign | ( exponent << 4 ) | mantissa );
#ifdef ZEROTRAP
    if ( ulawbyte == 0 ) ulawbyte = 0x02;	/* optional CCITT trap */
#endif

    return ulawbyte;
    }
/* End of code taken from sox */

/* ADPCM-3 step variation table */
static float newstep[5] = { 0.8, 0.9, 1.0, 1.75, 1.75 };

/* Intel ADPCM step variation table */
static int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};
    
#define CHARP(cp, i) ((signed char *)(cp+i))
#define SHORTP(cp, i) ((short *)(cp+i))
#define LONGP(cp, i) ((long *)(cp+i))



static object *AudioopError;

static object *
audioop_getsample(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val;
    int i;

    if ( !getargs(args, "(s#ii)", &cp, &len, &size, &i) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    if ( i < 0 || i >= len/size ) {
	err_setstr(AudioopError, "Index out of range");
	return 0;
    }
    if ( size == 1 )      val = (int)*CHARP(cp, i);
    else if ( size == 2 ) val = (int)*SHORTP(cp, i*2);
    else if ( size == 4 ) val = (int)*LONGP(cp, i*4);
    return newintobject(val);
}

static object *
audioop_max(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val;
    int i;
    int max = 0;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    for ( i=0; i<len; i+= size) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	if ( val < 0 ) val = (-val);
	if ( val > max ) max = val;
    }
    return newintobject(max);
}

static object *
audioop_avg(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val;
    int i;
    float avg = 0.0;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    for ( i=0; i<len; i+= size) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	avg += val;
    }
    if ( len == 0 )
      val = 0;
    else
      val = (int)(avg / (float)(len/size));
    return newintobject(val);
}

static object *
audioop_rms(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val;
    int i;
    float sum_squares = 0.0;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    for ( i=0; i<len; i+= size) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	sum_squares += (float)val*(float)val;
    }
    if ( len == 0 )
      val = 0;
    else
      val = (int)sqrt(sum_squares / (float)(len/size));
    return newintobject(val);
}

static object *
audioop_avgpp(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val, prevval, prevextremevalid = 0, prevextreme;
    int i;
    float avg = 0.0;
    int diff, prevdiff, extremediff, nextreme = 0;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    /* Compute first delta value ahead. Also automatically makes us
    ** skip the first extreme value
    */
    if ( size == 1 )      prevval = (int)*CHARP(cp, 0);
    else if ( size == 2 ) prevval = (int)*SHORTP(cp, 0);
    else if ( size == 4 ) prevval = (int)*LONGP(cp, 0);
    if ( size == 1 )      val = (int)*CHARP(cp, size);
    else if ( size == 2 ) val = (int)*SHORTP(cp, size);
    else if ( size == 4 ) val = (int)*LONGP(cp, size);
    prevdiff = val - prevval;
    
    for ( i=size; i<len; i+= size) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	diff = val - prevval;
	if ( diff*prevdiff < 0 ) {
	    /* Derivative changed sign. Compute difference to last extreme
	    ** value and remember.
	    */
	    if ( prevextremevalid ) {
		extremediff = prevval - prevextreme;
		if ( extremediff < 0 )
		  extremediff = -extremediff;
		avg += extremediff;
		nextreme++;
	    }
	    prevextremevalid = 1;
	    prevextreme = prevval;
	}
	prevval = val;
	if ( diff != 0 )
	  prevdiff = diff;	
    }
    if ( nextreme == 0 )
      val = 0;
    else
      val = (int)(avg / (float)nextreme);
    return newintobject(val);
}

static object *
audioop_maxpp(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val, prevval, prevextremevalid = 0, prevextreme;
    int i;
    int max = 0;
    int diff, prevdiff, extremediff;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    /* Compute first delta value ahead. Also automatically makes us
    ** skip the first extreme value
    */
    if ( size == 1 )      prevval = (int)*CHARP(cp, 0);
    else if ( size == 2 ) prevval = (int)*SHORTP(cp, 0);
    else if ( size == 4 ) prevval = (int)*LONGP(cp, 0);
    if ( size == 1 )      val = (int)*CHARP(cp, size);
    else if ( size == 2 ) val = (int)*SHORTP(cp, size);
    else if ( size == 4 ) val = (int)*LONGP(cp, size);
    prevdiff = val - prevval;

    for ( i=size; i<len; i+= size) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	diff = val - prevval;
	if ( diff*prevdiff < 0 ) {
	    /* Derivative changed sign. Compute difference to last extreme
	    ** value and remember.
	    */
	    if ( prevextremevalid ) {
		extremediff = prevval - prevextreme;
		if ( extremediff < 0 )
		  extremediff = -extremediff;
		if ( extremediff > max )
		  max = extremediff;
	    }
	    prevextremevalid = 1;
	    prevextreme = prevval;
	}
	prevval = val;
	if ( diff != 0 )
	  prevdiff = diff;
    }
    return newintobject(max);
}

static object *
audioop_cross(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    int len, size, val;
    int i;
    int cross, prevval, ncross;

    if ( !getargs(args, "(s#i)", &cp, &len, &size) )
      return 0;
    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    ncross = -1;
    prevval = 17; /* Anything <> 0,1 */
    for ( i=0; i<len; i+= size) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) >> 7;
	else if ( size == 2 ) val = ((int)*SHORTP(cp, i)) >> 15;
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 31;
	val = val & 1;
	if ( val != prevval ) ncross++;
	prevval = val;
    }
    return newintobject(ncross);
}

static object *
audioop_mul(self, args)
    object *self;
    object *args;
{
    signed char *cp, *ncp;
    int len, size, val;
    double factor, fval, maxval;
    object *rv;
    int i;

    if ( !getargs(args, "(s#id)", &cp, &len, &size, &factor ) )
      return 0;
    
    if ( size == 1 ) maxval = (double) 0x7f;
    else if ( size == 2 ) maxval = (double) 0x7fff;
    else if ( size == 4 ) maxval = (double) 0x7fffffff;
    else {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    
    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	fval = (double)val*factor;
	if ( fval > maxval ) fval = maxval;
	else if ( fval < -maxval ) fval = -maxval;
	val = (int)fval;
	if ( size == 1 )      *CHARP(ncp, i) = (signed char)val;
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)val;
	else if ( size == 4 ) *LONGP(ncp, i) = (long)val;
    }
    return rv;
}

static object *
audioop_tomono(self, args)
    object *self;
    object *args;
{
    signed char *cp, *ncp;
    int len, size, val1, val2;
    double fac1, fac2, fval, maxval;
    object *rv;
    int i;

    if ( !getargs(args, "(s#idd)", &cp, &len, &size, &fac1, &fac2 ) )
      return 0;
    
    if ( size == 1 ) maxval = (double) 0x7f;
    else if ( size == 2 ) maxval = (double) 0x7fff;
    else if ( size == 4 ) maxval = (double) 0x7fffffff;
    else {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len/2);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    
    for ( i=0; i < len; i += size*2 ) {
	if ( size == 1 )      val1 = (int)*CHARP(cp, i);
	else if ( size == 2 ) val1 = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val1 = (int)*LONGP(cp, i);
	if ( size == 1 )      val2 = (int)*CHARP(cp, i+1);
	else if ( size == 2 ) val2 = (int)*SHORTP(cp, i+2);
	else if ( size == 4 ) val2 = (int)*LONGP(cp, i+4);
	fval = (double)val1*fac1 + (double)val2*fac2;
	if ( fval > maxval ) fval = maxval;
	else if ( fval < -maxval ) fval = -maxval;
	val1 = (int)fval;
	if ( size == 1 )      *CHARP(ncp, i/2) = (signed char)val1;
	else if ( size == 2 ) *SHORTP(ncp, i/2) = (short)val1;
	else if ( size == 4 ) *LONGP(ncp, i/2)= (long)val1;
    }
    return rv;
}

static object *
audioop_tostereo(self, args)
    object *self;
    object *args;
{
    signed char *cp, *ncp;
    int len, size, val1, val2, val;
    double fac1, fac2, fval, maxval;
    object *rv;
    int i;

    if ( !getargs(args, "(s#idd)", &cp, &len, &size, &fac1, &fac2 ) )
      return 0;
    
    if ( size == 1 ) maxval = (double) 0x7f;
    else if ( size == 2 ) maxval = (double) 0x7fff;
    else if ( size == 4 ) maxval = (double) 0x7fffffff;
    else {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len*2);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    
    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);

	fval = (double)val*fac1;
	if ( fval > maxval ) fval = maxval;
	else if ( fval < -maxval ) fval = -maxval;
	val1 = (int)fval;

	fval = (double)val*fac2;
	if ( fval > maxval ) fval = maxval;
	else if ( fval < -maxval ) fval = -maxval;
	val2 = (int)fval;

	if ( size == 1 )      *CHARP(ncp, i*2) = (signed char)val1;
	else if ( size == 2 ) *SHORTP(ncp, i*2) = (short)val1;
	else if ( size == 4 ) *LONGP(ncp, i*2) = (long)val1;

	if ( size == 1 )      *CHARP(ncp, i*2+1) = (signed char)val2;
	else if ( size == 2 ) *SHORTP(ncp, i*2+2) = (short)val2;
	else if ( size == 4 ) *LONGP(ncp, i*2+4) = (long)val2;
    }
    return rv;
}

static object *
audioop_add(self, args)
    object *self;
    object *args;
{
    signed char *cp1, *cp2, *ncp;
    int len1, len2, size, val1, val2;
    object *rv;
    int i;

    if ( !getargs(args, "(s#s#i)",
		  &cp1, &len1, &cp2, &len2, &size ) )
      return 0;

    if ( len1 != len2 ) {
	err_setstr(AudioopError, "Lengths should be the same");
	return 0;
    }
    
    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len1);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    
    for ( i=0; i < len1; i += size ) {
	if ( size == 1 )      val1 = (int)*CHARP(cp1, i);
	else if ( size == 2 ) val1 = (int)*SHORTP(cp1, i);
	else if ( size == 4 ) val1 = (int)*LONGP(cp1, i);
	
	if ( size == 1 )      val2 = (int)*CHARP(cp2, i);
	else if ( size == 2 ) val2 = (int)*SHORTP(cp2, i);
	else if ( size == 4 ) val2 = (int)*LONGP(cp2, i);

	if ( size == 1 )      *CHARP(ncp, i) = (signed char)(val1+val2);
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)(val1+val2);
	else if ( size == 4 ) *LONGP(ncp, i) = (long)(val1+val2);
    }
    return rv;
}

static object *
audioop_bias(self, args)
    object *self;
    object *args;
{
    signed char *cp, *ncp;
    int len, size, val;
    object *rv;
    int i;
    int bias;

    if ( !getargs(args, "(s#ii)",
		  &cp, &len, &size , &bias) )
      return 0;

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    
    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = (int)*CHARP(cp, i);
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = (int)*LONGP(cp, i);
	
	if ( size == 1 )      *CHARP(ncp, i) = (signed char)(val+bias);
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)(val+bias);
	else if ( size == 4 ) *LONGP(ncp, i) = (long)(val+bias);
    }
    return rv;
}

static object *
audioop_lin2ulaw(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    unsigned char *ncp;
    int len, size, val;
    object *rv;
    int i;

    if ( !getargs(args, "(s#i)",
		  &cp, &len, &size) )
      return 0;

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len/size);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);
    
    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) << 8;
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 16;

	*ncp++ = st_linear_to_ulaw(val);
    }
    return rv;
}

static object *
audioop_ulaw2lin(self, args)
    object *self;
    object *args;
{
    unsigned char *cp;
    unsigned char cval;
    signed char *ncp;
    int len, size, val;
    object *rv;
    int i;

    if ( !getargs(args, "(s#i)",
		  &cp, &len, &size) )
      return 0;

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len*size);
    if ( rv == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(rv);
    
    for ( i=0; i < len*size; i += size ) {
	cval = *cp++;
	val = st_ulaw_to_linear(cval);
	
	if ( size == 1 )      *CHARP(ncp, i) = (signed char)(val >> 8);
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)(val);
	else if ( size == 4 ) *LONGP(ncp, i) = (long)(val<<16);
    }
    return rv;
}

static object *
audioop_lin2adpcm3(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    signed char *ncp;
    int len, size, val, step, valprev, delta;
    object *rv, *state, *str;
    int i;

    if ( !getargs(args, "(s#iO)",
		  &cp, &len, &size, &state) )
      return 0;
    

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    str = newsizedstringobject(NULL, len/size);
    if ( str == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(str);

    /* Decode state, should have (value, step) */
    if ( state == None ) {
	/* First time, it seems. Set defaults */
	valprev = 0;
	step = 4;	/* The '4' is magic. Dunno it's significance */
    } else if ( !getargs(state, "(ii)", &valprev, &step) )
      return 0;

    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) << 8;
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 16;

	/* Step 1 - compute difference with previous value */
	delta = (val - valprev)/step;

	/* Step 2 - Clamp */
	if ( delta < -4 )
	  delta = -4;
	else if ( delta > 3 )
	  delta = 3;

	/* Step 3 - Update previous value */
	valprev += delta*step;

	/* Step 4 - Clamp previous value to 16 bits */
	if ( valprev > 32767 )
	  valprev = 32767;
	else if ( valprev < -32768 )
	  valprev = -32768;

	/* Step 5 - Update step value */
	step = step * newstep[abs(delta)];
	step++;		/* Don't understand this. */

	/* Step 6 - Output value (as a whole byte, currently) */
	*ncp++ = delta;
    }
    rv = mkvalue("(O(ii))", str, valprev, step);
    DECREF(str);
    return rv;
}

static object *
audioop_adpcm32lin(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    signed char *ncp;
    int len, size, val, valprev, step, delta;
    object *rv, *str, *state;
    int i;

    if ( !getargs(args, "(s#iO)",
		  &cp, &len, &size, &state) )
      return 0;

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    /* Decode state, should have (value, step) */
    if ( state == None ) {
	/* First time, it seems. Set defaults */
	valprev = 0;
	step = 4;	/* The '4' is magic. Dunno it's significance */
    } else if ( !getargs(state, "(ii)", &valprev, &step) )
      return 0;
    
    str = newsizedstringobject(NULL, len*size);
    if ( str == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(str);
    
    for ( i=0; i < len*size; i += size ) {
	/* Step 1 - get the delta value */
	delta = *cp++;

	/* Step 2 - update output value */
	valprev = valprev + delta*step;

	/* Step 3 - clamp output value */
	if ( valprev > 32767 )
	  valprev = 32767;
	else if ( valprev < -32768 )
	  valprev = -32768;

	/* Step 4 - Update step value */
	step = step * newstep[abs(delta)];
	step++;

	/* Step 5 - Output value */
	if ( size == 1 )      *CHARP(ncp, i) = (signed char)(valprev >> 8);
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)(valprev);
	else if ( size == 4 ) *LONGP(ncp, i) = (long)(valprev<<16);
    }

    rv = mkvalue("(O(ii))", str, valprev, step);
    DECREF(str);
    return rv;
}

static object *
audioop_lin2adpcm(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    signed char *ncp;
    int len, size, val, step, valpred, delta, index, sign, vpdiff, diff;
    object *rv, *state, *str;
    int i, outputbuffer, bufferstep;

    if ( !getargs(args, "(s#iO)",
		  &cp, &len, &size, &state) )
      return 0;
    

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    str = newsizedstringobject(NULL, len/(size*2));
    if ( str == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(str);

    /* Decode state, should have (value, step) */
    if ( state == None ) {
	/* First time, it seems. Set defaults */
	valpred = 0;
	step = 7;
	index = 0;
    } else if ( !getargs(state, "(iii)", &valpred, &step, &index) )
      return 0;

    bufferstep = 1;

    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) << 8;
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 16;

	/* Step 1 - compute difference with previous value */
	diff = val - valpred;
	sign = (diff < 0) ? 8 : 0;
	if ( sign ) diff = (-diff);

	/* Step 2 - Divide and clamp */
	/* Note:
	** This code *approximately* computes:
	**    delta = diff*4/step;
	**    vpdiff = (delta+0.5)*step/4;
	** but in shift step bits are dropped. The net result of this is
	** that even if you have fast mul/div hardware you cannot put it to
	** good use since the fixup would be too expensive.
	*/
	delta = 0;
	vpdiff = (step >> 3);
	
	if ( diff >= step ) {
	    delta = 4;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step  ) {
	    delta |= 2;
	    diff -= step;
	    vpdiff += step;
	}
	step >>= 1;
	if ( diff >= step ) {
	    delta |= 1;
	    vpdiff += step;
	}

	/* Step 3 - Update previous value */
	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 4 - Clamp previous value to 16 bits */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;

	/* Step 5 - Assemble value, update index and step values */
	delta |= sign;
	
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;
	step = stepsizeTable[index];

	/* Step 6 - Output value */
	if ( bufferstep ) {
	    outputbuffer = (delta << 4) & 0xf0;
	} else {
	    *ncp++ = (delta & 0x0f) | outputbuffer;
	}
	bufferstep = !bufferstep;
    }
    rv = mkvalue("(O(iii))", str, valpred, step, index);
    DECREF(str);
    return rv;
}

static object *
audioop_adpcm2lin(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    signed char *ncp;
    int len, size, val, valpred, step, delta, index, sign, vpdiff;
    object *rv, *str, *state;
    int i, inputbuffer, bufferstep;

    if ( !getargs(args, "(s#iO)",
		  &cp, &len, &size, &state) )
      return 0;

    if ( size != 1 && size != 2 && size != 4) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    /* Decode state, should have (value, step) */
    if ( state == None ) {
	/* First time, it seems. Set defaults */
	valpred = 0;
	step = 7;
	index = 0;
    } else if ( !getargs(state, "(iii)", &valpred, &step, &index) )
      return 0;
    
    str = newsizedstringobject(NULL, len*size*2);
    if ( str == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(str);

    bufferstep = 0;
    
    for ( i=0; i < len*size*2; i += size ) {
	/* Step 1 - get the delta value and compute next index */
	if ( bufferstep ) {
	    delta = inputbuffer & 0xf;
	} else {
	    inputbuffer = *cp++;
	    delta = (inputbuffer >> 4) & 0xf;
	}

	bufferstep = !bufferstep;

	/* Step 2 - Find new index value (for later) */
	index += indexTable[delta];
	if ( index < 0 ) index = 0;
	if ( index > 88 ) index = 88;

	/* Step 3 - Separate sign and magnitude */
	sign = delta & 8;
	delta = delta & 7;

	/* Step 4 - Compute difference and new predicted value */
	/*
	** Computes 'vpdiff = (delta+0.5)*step/4', but see comment
	** in adpcm_coder.
	*/
	vpdiff = step >> 3;
	if ( delta & 4 ) vpdiff += step;
	if ( delta & 2 ) vpdiff += step>>1;
	if ( delta & 1 ) vpdiff += step>>2;

	if ( sign )
	  valpred -= vpdiff;
	else
	  valpred += vpdiff;

	/* Step 5 - clamp output value */
	if ( valpred > 32767 )
	  valpred = 32767;
	else if ( valpred < -32768 )
	  valpred = -32768;

	/* Step 6 - Update step value */
	step = stepsizeTable[index];

	/* Step 6 - Output value */
	if ( size == 1 )      *CHARP(ncp, i) = (signed char)(valpred >> 8);
	else if ( size == 2 ) *SHORTP(ncp, i) = (short)(valpred);
	else if ( size == 4 ) *LONGP(ncp, i) = (long)(valpred<<16);
    }

    rv = mkvalue("(O(iii))", str, valpred, step, index);
    DECREF(str);
    return rv;
}

static struct methodlist audioop_methods[] = {
    { "max", audioop_max },
    { "avg", audioop_avg },
    { "maxpp", audioop_maxpp },
    { "avgpp", audioop_avgpp },
    { "rms", audioop_rms },
    { "cross", audioop_cross },
    { "mul", audioop_mul },
    { "add", audioop_add },
    { "bias", audioop_bias },
    { "ulaw2lin", audioop_ulaw2lin },
    { "lin2ulaw", audioop_lin2ulaw },
    { "adpcm2lin", audioop_adpcm2lin },
    { "lin2adpcm", audioop_lin2adpcm },
    { "adpcm32lin", audioop_adpcm32lin },
    { "lin2adpcm3", audioop_lin2adpcm3 },
    { "tomono", audioop_tomono },
    { "tostereo", audioop_tostereo },
    { "getsample", audioop_getsample },
    { 0,          0 }
};


void
initaudioop()
{
	object *m, *d;
	m = initmodule("audioop", audioop_methods);
	d = getmoduledict(m);
	AudioopError = newstringobject("audioop.error");
	if ( AudioopError == NULL || dictinsert(d,"error",AudioopError) )
	    fatal("can't define audioop.error");
}
