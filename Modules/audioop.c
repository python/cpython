/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

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

/* audioopmodule - Module to detect peak values in arrays */

#include "allobjects.h"
#include "modsupport.h"

#if defined(__CHAR_UNSIGNED__)
#if defined(signed)
!ERROR!; READ THE SOURCE FILE!;
/* This module currently does not work on systems where only unsigned
   characters are available.  Take it out of Setup.  Sorry. */
#endif
#endif

#include <math.h>

/* Code shamelessly stolen from sox,
** (c) Craig Reese, Joe Campbell and Jeff Poskanzer 1989 */

#define MINLIN -32768
#define MAXLIN 32767
#define LINCLIP(x) do { if ( x < MINLIN ) x = MINLIN ; else if ( x > MAXLIN ) x = MAXLIN; } while ( 0 )

static unsigned char st_linear_to_ulaw( /* int sample */ );

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

static unsigned char
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
audioop_minmax(self, args)
	object *self;
	object *args;
{
	signed char *cp;
	int len, size, val;
	int i;
	int min = 0x7fffffff, max = -0x7fffffff;

	if (!getargs(args, "(s#i)", &cp, &len, &size))
		return NULL;
	if (size != 1 && size != 2 && size != 4) {
		err_setstr(AudioopError, "Size should be 1, 2 or 4");
		return NULL;
	}
	for (i = 0; i < len; i += size) {
		if (size == 1) val = (int) *CHARP(cp, i);
		else if (size == 2) val = (int) *SHORTP(cp, i);
		else if (size == 4) val = (int) *LONGP(cp, i);
		if (val > max) max = val;
		if (val < min) min = val;
	}
	return mkvalue("(ii)", min, max);
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

static double _sum2(a, b, len)
    short *a;
    short *b;
    int len;
{
    int i;
    double sum = 0.0;

    for( i=0; i<len; i++) {
	sum = sum + (double)a[i]*(double)b[i];
    }
    return sum;
}

/*
** Findfit tries to locate a sample within another sample. Its main use
** is in echo-cancellation (to find the feedback of the output signal in
** the input signal).
** The method used is as follows:
**
** let R be the reference signal (length n) and A the input signal (length N)
** with N > n, and let all sums be over i from 0 to n-1.
**
** Now, for each j in {0..N-n} we compute a factor fj so that -fj*R matches A
** as good as possible, i.e. sum( (A[j+i]+fj*R[i])^2 ) is minimal. This
** equation gives fj = sum( A[j+i]R[i] ) / sum(R[i]^2).
**
** Next, we compute the relative distance between the original signal and
** the modified signal and minimize that over j:
** vj = sum( (A[j+i]-fj*R[i])^2 ) / sum( A[j+i]^2 )  =>
** vj = ( sum(A[j+i]^2)*sum(R[i]^2) - sum(A[j+i]R[i])^2 ) / sum( A[j+i]^2 )
**
** In the code variables correspond as follows:
** cp1		A
** cp2		R
** len1		N
** len2		n
** aj_m1	A[j-1]
** aj_lm1	A[j+n-1]
** sum_ri_2	sum(R[i]^2)
** sum_aij_2	sum(A[i+j]^2)
** sum_aij_ri	sum(A[i+j]R[i])
**
** sum_ri is calculated once, sum_aij_2 is updated each step and sum_aij_ri
** is completely recalculated each step.
*/
static object *
audioop_findfit(self, args)
    object *self;
    object *args;
{
    short *cp1, *cp2;
    int len1, len2;
    int j, best_j;
    double aj_m1, aj_lm1;
    double sum_ri_2, sum_aij_2, sum_aij_ri, result, best_result, factor;

    if ( !getargs(args, "(s#s#)", &cp1, &len1, &cp2, &len2) )
      return 0;
    if ( len1 & 1 || len2 & 1 ) {
	err_setstr(AudioopError, "Strings should be even-sized");
	return 0;
    }
    len1 >>= 1;
    len2 >>= 1;
    
    if ( len1 < len2 ) {
	err_setstr(AudioopError, "First sample should be longer");
	return 0;
    }
    sum_ri_2 = _sum2(cp2, cp2, len2);
    sum_aij_2 = _sum2(cp1, cp1, len2);
    sum_aij_ri = _sum2(cp1, cp2, len2);

    result = (sum_ri_2*sum_aij_2 - sum_aij_ri*sum_aij_ri) / sum_aij_2;

    best_result = result;
    best_j = 0;
    j = 0;

    for ( j=1; j<=len1-len2; j++) {
	aj_m1 = (double)cp1[j-1];
	aj_lm1 = (double)cp1[j+len2-1];

	sum_aij_2 = sum_aij_2 + aj_lm1*aj_lm1 - aj_m1*aj_m1;
	sum_aij_ri = _sum2(cp1+j, cp2, len2);

	result = (sum_ri_2*sum_aij_2 - sum_aij_ri*sum_aij_ri) / sum_aij_2;

	if ( result < best_result ) {
	    best_result = result;
	    best_j = j;
	}
	
    }

    factor = _sum2(cp1+best_j, cp2, len2) / sum_ri_2;
    
    return mkvalue("(if)", best_j, factor);
}

/*
** findfactor finds a factor f so that the energy in A-fB is minimal.
** See the comment for findfit for details.
*/
static object *
audioop_findfactor(self, args)
    object *self;
    object *args;
{
    short *cp1, *cp2;
    int len1, len2;
    double sum_ri_2, sum_aij_ri, result;

    if ( !getargs(args, "(s#s#)", &cp1, &len1, &cp2, &len2) )
      return 0;
    if ( len1 & 1 || len2 & 1 ) {
	err_setstr(AudioopError, "Strings should be even-sized");
	return 0;
    }
    if ( len1 != len2 ) {
	err_setstr(AudioopError, "Samples should be same size");
	return 0;
    }
    len2 >>= 1;
    sum_ri_2 = _sum2(cp2, cp2, len2);
    sum_aij_ri = _sum2(cp1, cp2, len2);

    result = sum_aij_ri / sum_ri_2;

    return newfloatobject(result);
}

/*
** findmax returns the index of the n-sized segment of the input sample
** that contains the most energy.
*/
static object *
audioop_findmax(self, args)
    object *self;
    object *args;
{
    short *cp1;
    int len1, len2;
    int j, best_j;
    double aj_m1, aj_lm1;
    double result, best_result;

    if ( !getargs(args, "(s#i)", &cp1, &len1, &len2) )
      return 0;
    if ( len1 & 1 ) {
	err_setstr(AudioopError, "Strings should be even-sized");
	return 0;
    }
    len1 >>= 1;
    
    if ( len1 < len2 ) {
	err_setstr(AudioopError, "Input sample should be longer");
	return 0;
    }

    result = _sum2(cp1, cp1, len2);

    best_result = result;
    best_j = 0;
    j = 0;

    for ( j=1; j<=len1-len2; j++) {
	aj_m1 = (double)cp1[j-1];
	aj_lm1 = (double)cp1[j+len2-1];

	result = result + aj_lm1*aj_lm1 - aj_m1*aj_m1;

	if ( result > best_result ) {
	    best_result = result;
	    best_j = j;
	}
	
    }

    return newintobject(best_j);
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
    int prevval, ncross;

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
audioop_reverse(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    unsigned char *ncp;
    int len, size, val;
    object *rv;
    int i, j;

    if ( !getargs(args, "(s#i)",
		  &cp, &len, &size) )
      return 0;

    if ( size != 1 && size != 2 && size != 4 ) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, len);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);
    
    for ( i=0; i < len; i += size ) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) << 8;
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 16;

	j = len - i - size;
	
	if ( size == 1 )      *CHARP(ncp, j) = (signed char)(val >> 8);
	else if ( size == 2 ) *SHORTP(ncp, j) = (short)(val);
	else if ( size == 4 ) *LONGP(ncp, j) = (long)(val<<16);
    }
    return rv;
}

static object *
audioop_lin2lin(self, args)
    object *self;
    object *args;
{
    signed char *cp;
    unsigned char *ncp;
    int len, size, size2, val;
    object *rv;
    int i, j;

    if ( !getargs(args, "(s#ii)",
		  &cp, &len, &size, &size2) )
      return 0;

    if ( (size != 1 && size != 2 && size != 4) ||
	                       (size2 != 1 && size2 != 2 && size2 != 4)) {
	err_setstr(AudioopError, "Size should be 1, 2 or 4");
	return 0;
    }
    
    rv = newsizedstringobject(NULL, (len/size)*size2);
    if ( rv == 0 )
      return 0;
    ncp = (unsigned char *)getstringvalue(rv);
    
    for ( i=0, j=0; i < len; i += size, j += size2 ) {
	if ( size == 1 )      val = ((int)*CHARP(cp, i)) << 8;
	else if ( size == 2 ) val = (int)*SHORTP(cp, i);
	else if ( size == 4 ) val = ((int)*LONGP(cp, i)) >> 16;

	if ( size2 == 1 )      *CHARP(ncp, j) = (signed char)(val >> 8);
	else if ( size2 == 2 ) *SHORTP(ncp, j) = (short)(val);
	else if ( size2 == 4 ) *LONGP(ncp, j) = (long)(val<<16);
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
    } else if ( !getargs(state, "(ii)", &valpred, &index) )
      return 0;

    step = stepsizeTable[index];
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
    rv = mkvalue("(O(ii))", str, valpred, index);
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
    int len, size, valpred, step, delta, index, sign, vpdiff;
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
    } else if ( !getargs(state, "(ii)", &valpred, &index) )
      return 0;
    
    str = newsizedstringobject(NULL, len*size*2);
    if ( str == 0 )
      return 0;
    ncp = (signed char *)getstringvalue(str);

    step = stepsizeTable[index];
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

    rv = mkvalue("(O(ii))", str, valpred, index);
    DECREF(str);
    return rv;
}

static struct methodlist audioop_methods[] = {
    { "max", audioop_max },
    { "minmax", audioop_minmax },
    { "avg", audioop_avg },
    { "maxpp", audioop_maxpp },
    { "avgpp", audioop_avgpp },
    { "rms", audioop_rms },
    { "findfit", audioop_findfit },
    { "findmax", audioop_findmax },
    { "findfactor", audioop_findfactor },
    { "cross", audioop_cross },
    { "mul", audioop_mul },
    { "add", audioop_add },
    { "bias", audioop_bias },
    { "ulaw2lin", audioop_ulaw2lin },
    { "lin2ulaw", audioop_lin2ulaw },
    { "lin2lin", audioop_lin2lin },
    { "adpcm2lin", audioop_adpcm2lin },
    { "lin2adpcm", audioop_lin2adpcm },
    { "tomono", audioop_tomono },
    { "tostereo", audioop_tostereo },
    { "getsample", audioop_getsample },
    { "reverse", audioop_reverse },
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
