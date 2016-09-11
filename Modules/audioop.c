
/* audioopmodule - Module to detect peak values in arrays */

#define PY_SSIZE_T_CLEAN

#include "Python.h"

#if defined(__CHAR_UNSIGNED__)
#if defined(signed)
/* This module currently does not work on systems where only unsigned
   characters are available.  Take it out of Setup.  Sorry. */
#endif
#endif

static const int maxvals[] = {0, 0x7F, 0x7FFF, 0x7FFFFF, 0x7FFFFFFF};
/* -1 trick is needed on Windows to support -0x80000000 without a warning */
static const int minvals[] = {0, -0x80, -0x8000, -0x800000, -0x7FFFFFFF-1};
static const unsigned int masks[] = {0, 0xFF, 0xFFFF, 0xFFFFFF, 0xFFFFFFFF};

static int
fbound(double val, double minval, double maxval)
{
    if (val > maxval)
        val = maxval;
    else if (val < minval + 1)
        val = minval;
    return (int)val;
}


/* Code shamelessly stolen from sox, 12.17.7, g711.c
** (c) Craig Reese, Joe Campbell and Jeff Poskanzer 1989 */

/* From g711.c:
 *
 * December 30, 1994:
 * Functions linear2alaw, linear2ulaw have been updated to correctly
 * convert unquantized 16 bit values.
 * Tables for direct u- to A-law and A- to u-law conversions have been
 * corrected.
 * Borge Lindberg, Center for PersonKommunikation, Aalborg University.
 * bli@cpk.auc.dk
 *
 */
#define BIAS 0x84   /* define the add-in bias for 16 bit samples */
#define CLIP 32635
#define SIGN_BIT        (0x80)          /* Sign bit for an A-law byte. */
#define QUANT_MASK      (0xf)           /* Quantization field mask. */
#define SEG_SHIFT       (4)             /* Left shift for segment number. */
#define SEG_MASK        (0x70)          /* Segment field mask. */

static const int16_t seg_aend[8] = {
    0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF
};
static const int16_t seg_uend[8] = {
    0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF
};

static int16_t
search(int16_t val, const int16_t *table, int size)
{
    int i;

    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}
#define st_ulaw2linear16(uc) (_st_ulaw2linear16[uc])
#define st_alaw2linear16(uc) (_st_alaw2linear16[uc])

static const int16_t _st_ulaw2linear16[256] = {
    -32124,  -31100,  -30076,  -29052,  -28028,  -27004,  -25980,
    -24956,  -23932,  -22908,  -21884,  -20860,  -19836,  -18812,
    -17788,  -16764,  -15996,  -15484,  -14972,  -14460,  -13948,
    -13436,  -12924,  -12412,  -11900,  -11388,  -10876,  -10364,
     -9852,   -9340,   -8828,   -8316,   -7932,   -7676,   -7420,
     -7164,   -6908,   -6652,   -6396,   -6140,   -5884,   -5628,
     -5372,   -5116,   -4860,   -4604,   -4348,   -4092,   -3900,
     -3772,   -3644,   -3516,   -3388,   -3260,   -3132,   -3004,
     -2876,   -2748,   -2620,   -2492,   -2364,   -2236,   -2108,
     -1980,   -1884,   -1820,   -1756,   -1692,   -1628,   -1564,
     -1500,   -1436,   -1372,   -1308,   -1244,   -1180,   -1116,
     -1052,    -988,    -924,    -876,    -844,    -812,    -780,
      -748,    -716,    -684,    -652,    -620,    -588,    -556,
      -524,    -492,    -460,    -428,    -396,    -372,    -356,
      -340,    -324,    -308,    -292,    -276,    -260,    -244,
      -228,    -212,    -196,    -180,    -164,    -148,    -132,
      -120,    -112,    -104,     -96,     -88,     -80,     -72,
       -64,     -56,     -48,     -40,     -32,     -24,     -16,
    -8,       0,   32124,   31100,   30076,   29052,   28028,
     27004,   25980,   24956,   23932,   22908,   21884,   20860,
     19836,   18812,   17788,   16764,   15996,   15484,   14972,
     14460,   13948,   13436,   12924,   12412,   11900,   11388,
     10876,   10364,    9852,    9340,    8828,    8316,    7932,
      7676,    7420,    7164,    6908,    6652,    6396,    6140,
      5884,    5628,    5372,    5116,    4860,    4604,    4348,
      4092,    3900,    3772,    3644,    3516,    3388,    3260,
      3132,    3004,    2876,    2748,    2620,    2492,    2364,
      2236,    2108,    1980,    1884,    1820,    1756,    1692,
      1628,    1564,    1500,    1436,    1372,    1308,    1244,
      1180,    1116,    1052,     988,     924,     876,     844,
       812,     780,     748,     716,     684,     652,     620,
       588,     556,     524,     492,     460,     428,     396,
       372,     356,     340,     324,     308,     292,     276,
       260,     244,     228,     212,     196,     180,     164,
       148,     132,     120,     112,     104,      96,      88,
    80,      72,      64,      56,      48,      40,      32,
    24,      16,       8,       0
};

/*
 * linear2ulaw() accepts a 14-bit signed integer and encodes it as u-law data
 * stored in an unsigned char.  This function should only be called with
 * the data shifted such that it only contains information in the lower
 * 14-bits.
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *      Biased Linear Input Code        Compressed Code
 *      ------------------------        ---------------
 *      00000001wxyza                   000wxyz
 *      0000001wxyzab                   001wxyz
 *      000001wxyzabc                   010wxyz
 *      00001wxyzabcd                   011wxyz
 *      0001wxyzabcde                   100wxyz
 *      001wxyzabcdef                   101wxyz
 *      01wxyzabcdefg                   110wxyz
 *      1wxyzabcdefgh                   111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static unsigned char
st_14linear2ulaw(int16_t pcm_val)       /* 2's complement (14-bit range) */
{
    int16_t         mask;
    int16_t         seg;
    unsigned char   uval;

    /* u-law inverts all bits */
    /* Get the sign and the magnitude of the value. */
    if (pcm_val < 0) {
        pcm_val = -pcm_val;
        mask = 0x7F;
    } else {
        mask = 0xFF;
    }
    if ( pcm_val > CLIP ) pcm_val = CLIP;           /* clip the magnitude */
    pcm_val += (BIAS >> 2);

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_uend, 8);

    /*
     * Combine the sign, segment, quantization bits;
     * and complement the code word.
     */
    if (seg >= 8)           /* out of range, return maximum value. */
        return (unsigned char) (0x7F ^ mask);
    else {
        uval = (unsigned char) (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
        return (uval ^ mask);
    }

}

static const int16_t _st_alaw2linear16[256] = {
     -5504,   -5248,   -6016,   -5760,   -4480,   -4224,   -4992,
     -4736,   -7552,   -7296,   -8064,   -7808,   -6528,   -6272,
     -7040,   -6784,   -2752,   -2624,   -3008,   -2880,   -2240,
     -2112,   -2496,   -2368,   -3776,   -3648,   -4032,   -3904,
     -3264,   -3136,   -3520,   -3392,  -22016,  -20992,  -24064,
    -23040,  -17920,  -16896,  -19968,  -18944,  -30208,  -29184,
    -32256,  -31232,  -26112,  -25088,  -28160,  -27136,  -11008,
    -10496,  -12032,  -11520,   -8960,   -8448,   -9984,   -9472,
    -15104,  -14592,  -16128,  -15616,  -13056,  -12544,  -14080,
    -13568,    -344,    -328,    -376,    -360,    -280,    -264,
      -312,    -296,    -472,    -456,    -504,    -488,    -408,
      -392,    -440,    -424,     -88,     -72,    -120,    -104,
       -24,      -8,     -56,     -40,    -216,    -200,    -248,
      -232,    -152,    -136,    -184,    -168,   -1376,   -1312,
     -1504,   -1440,   -1120,   -1056,   -1248,   -1184,   -1888,
     -1824,   -2016,   -1952,   -1632,   -1568,   -1760,   -1696,
      -688,    -656,    -752,    -720,    -560,    -528,    -624,
      -592,    -944,    -912,   -1008,    -976,    -816,    -784,
      -880,    -848,    5504,    5248,    6016,    5760,    4480,
      4224,    4992,    4736,    7552,    7296,    8064,    7808,
      6528,    6272,    7040,    6784,    2752,    2624,    3008,
      2880,    2240,    2112,    2496,    2368,    3776,    3648,
      4032,    3904,    3264,    3136,    3520,    3392,   22016,
     20992,   24064,   23040,   17920,   16896,   19968,   18944,
     30208,   29184,   32256,   31232,   26112,   25088,   28160,
     27136,   11008,   10496,   12032,   11520,    8960,    8448,
      9984,    9472,   15104,   14592,   16128,   15616,   13056,
     12544,   14080,   13568,     344,     328,     376,     360,
       280,     264,     312,     296,     472,     456,     504,
       488,     408,     392,     440,     424,      88,      72,
       120,     104,      24,       8,      56,      40,     216,
       200,     248,     232,     152,     136,     184,     168,
      1376,    1312,    1504,    1440,    1120,    1056,    1248,
      1184,    1888,    1824,    2016,    1952,    1632,    1568,
      1760,    1696,     688,     656,     752,     720,     560,
       528,     624,     592,     944,     912,    1008,     976,
       816,     784,     880,     848
};

/*
 * linear2alaw() accepts a 13-bit signed integer and encodes it as A-law data
 * stored in an unsigned char.  This function should only be called with
 * the data shifted such that it only contains information in the lower
 * 13-bits.
 *
 *              Linear Input Code       Compressed Code
 *      ------------------------        ---------------
 *      0000000wxyza                    000wxyz
 *      0000001wxyza                    001wxyz
 *      000001wxyzab                    010wxyz
 *      00001wxyzabc                    011wxyz
 *      0001wxyzabcd                    100wxyz
 *      001wxyzabcde                    101wxyz
 *      01wxyzabcdef                    110wxyz
 *      1wxyzabcdefg                    111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
static unsigned char
st_linear2alaw(int16_t pcm_val) /* 2's complement (13-bit range) */
{
    int16_t         mask;
    int16_t         seg;
    unsigned char   aval;

    /* A-law using even bit inversion */
    if (pcm_val >= 0) {
        mask = 0xD5;            /* sign (7th) bit = 1 */
    } else {
        mask = 0x55;            /* sign bit = 0 */
        pcm_val = -pcm_val - 1;
    }

    /* Convert the scaled magnitude to segment number. */
    seg = search(pcm_val, seg_aend, 8);

    /* Combine the sign, segment, and quantization bits. */

    if (seg >= 8)           /* out of range, return maximum value. */
        return (unsigned char) (0x7F ^ mask);
    else {
        aval = (unsigned char) seg << SEG_SHIFT;
        if (seg < 2)
            aval |= (pcm_val >> 1) & QUANT_MASK;
        else
            aval |= (pcm_val >> seg) & QUANT_MASK;
        return (aval ^ mask);
    }
}
/* End of code taken from sox */

/* Intel ADPCM step variation table */
static const int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8,
};

static const int stepsizeTable[89] = {
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

#define GETINTX(T, cp, i)  (*(T *)((unsigned char *)(cp) + (i)))
#define SETINTX(T, cp, i, val)  do {                    \
        *(T *)((unsigned char *)(cp) + (i)) = (T)(val); \
    } while (0)


#define GETINT8(cp, i)          GETINTX(signed char, (cp), (i))
#define GETINT16(cp, i)         GETINTX(int16_t, (cp), (i))
#define GETINT32(cp, i)         GETINTX(int32_t, (cp), (i))

#if WORDS_BIGENDIAN
#define GETINT24(cp, i)  (                              \
        ((unsigned char *)(cp) + (i))[2] +              \
        (((unsigned char *)(cp) + (i))[1] << 8) +       \
        (((signed char *)(cp) + (i))[0] << 16) )
#else
#define GETINT24(cp, i)  (                              \
        ((unsigned char *)(cp) + (i))[0] +              \
        (((unsigned char *)(cp) + (i))[1] << 8) +       \
        (((signed char *)(cp) + (i))[2] << 16) )
#endif


#define SETINT8(cp, i, val)     SETINTX(signed char, (cp), (i), (val))
#define SETINT16(cp, i, val)    SETINTX(int16_t, (cp), (i), (val))
#define SETINT32(cp, i, val)    SETINTX(int32_t, (cp), (i), (val))

#if WORDS_BIGENDIAN
#define SETINT24(cp, i, val)  do {                              \
        ((unsigned char *)(cp) + (i))[2] = (int)(val);          \
        ((unsigned char *)(cp) + (i))[1] = (int)(val) >> 8;     \
        ((signed char *)(cp) + (i))[0] = (int)(val) >> 16;      \
    } while (0)
#else
#define SETINT24(cp, i, val)  do {                              \
        ((unsigned char *)(cp) + (i))[0] = (int)(val);          \
        ((unsigned char *)(cp) + (i))[1] = (int)(val) >> 8;     \
        ((signed char *)(cp) + (i))[2] = (int)(val) >> 16;      \
    } while (0)
#endif


#define GETRAWSAMPLE(size, cp, i)  (                    \
        (size == 1) ? (int)GETINT8((cp), (i)) :         \
        (size == 2) ? (int)GETINT16((cp), (i)) :        \
        (size == 3) ? (int)GETINT24((cp), (i)) :        \
                      (int)GETINT32((cp), (i)))

#define SETRAWSAMPLE(size, cp, i, val)  do {    \
        if (size == 1)                          \
            SETINT8((cp), (i), (val));          \
        else if (size == 2)                     \
            SETINT16((cp), (i), (val));         \
        else if (size == 3)                     \
            SETINT24((cp), (i), (val));         \
        else                                    \
            SETINT32((cp), (i), (val));         \
    } while(0)


#define GETSAMPLE32(size, cp, i)  (                     \
        (size == 1) ? (int)GETINT8((cp), (i)) << 24 :   \
        (size == 2) ? (int)GETINT16((cp), (i)) << 16 :  \
        (size == 3) ? (int)GETINT24((cp), (i)) << 8 :   \
                      (int)GETINT32((cp), (i)))

#define SETSAMPLE32(size, cp, i, val)  do {     \
        if (size == 1)                          \
            SETINT8((cp), (i), (val) >> 24);    \
        else if (size == 2)                     \
            SETINT16((cp), (i), (val) >> 16);   \
        else if (size == 3)                     \
            SETINT24((cp), (i), (val) >> 8);    \
        else                                    \
            SETINT32((cp), (i), (val));         \
    } while(0)


static PyObject *AudioopError;

static int
audioop_check_size(int size)
{
    if (size < 1 || size > 4) {
        PyErr_SetString(AudioopError, "Size should be 1, 2, 3 or 4");
        return 0;
    }
    else
        return 1;
}

static int
audioop_check_parameters(Py_ssize_t len, int size)
{
    if (!audioop_check_size(size))
        return 0;
    if (len % size != 0) {
        PyErr_SetString(AudioopError, "not a whole number of frames");
        return 0;
    }
    return 1;
}

/*[clinic input]
module audioop
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8fa8f6611be3591a]*/

/*[clinic input]
audioop.getsample

    fragment: Py_buffer
    width: int
    index: Py_ssize_t
    /

Return the value of sample index from the fragment.
[clinic start generated code]*/

static PyObject *
audioop_getsample_impl(PyObject *module, Py_buffer *fragment, int width,
                       Py_ssize_t index)
/*[clinic end generated code: output=8fe1b1775134f39a input=88edbe2871393549]*/
{
    int val;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    if (index < 0 || index >= fragment->len/width) {
        PyErr_SetString(AudioopError, "Index out of range");
        return NULL;
    }
    val = GETRAWSAMPLE(width, fragment->buf, index*width);
    return PyLong_FromLong(val);
}

/*[clinic input]
audioop.max

    fragment: Py_buffer
    width: int
    /

Return the maximum of the absolute value of all samples in a fragment.
[clinic start generated code]*/

static PyObject *
audioop_max_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=e6c5952714f1c3f0 input=32bea5ea0ac8c223]*/
{
    Py_ssize_t i;
    unsigned int absval, max = 0;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    for (i = 0; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i);
        /* Cast to unsigned before negating. Unsigned overflow is well-
        defined, but signed overflow is not. */
        if (val < 0) absval = (unsigned int)-(int64_t)val;
        else absval = val;
        if (absval > max) max = absval;
    }
    return PyLong_FromUnsignedLong(max);
}

/*[clinic input]
audioop.minmax

    fragment: Py_buffer
    width: int
    /

Return the minimum and maximum values of all samples in the sound fragment.
[clinic start generated code]*/

static PyObject *
audioop_minmax_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=473fda66b15c836e input=89848e9b927a0696]*/
{
    Py_ssize_t i;
    /* -1 trick below is needed on Windows to support -0x80000000 without
    a warning */
    int min = 0x7fffffff, max = -0x7FFFFFFF-1;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    for (i = 0; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i);
        if (val > max) max = val;
        if (val < min) min = val;
    }
    return Py_BuildValue("(ii)", min, max);
}

/*[clinic input]
audioop.avg

    fragment: Py_buffer
    width: int
    /

Return the average over all samples in the fragment.
[clinic start generated code]*/

static PyObject *
audioop_avg_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=4410a4c12c3586e6 input=1114493c7611334d]*/
{
    Py_ssize_t i;
    int avg;
    double sum = 0.0;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    for (i = 0; i < fragment->len; i += width)
        sum += GETRAWSAMPLE(width, fragment->buf, i);
    if (fragment->len == 0)
        avg = 0;
    else
        avg = (int)floor(sum / (double)(fragment->len/width));
    return PyLong_FromLong(avg);
}

/*[clinic input]
audioop.rms

    fragment: Py_buffer
    width: int
    /

Return the root-mean-square of the fragment, i.e. sqrt(sum(S_i^2)/n).
[clinic start generated code]*/

static PyObject *
audioop_rms_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=1e7871c826445698 input=4cc57c6c94219d78]*/
{
    Py_ssize_t i;
    unsigned int res;
    double sum_squares = 0.0;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    for (i = 0; i < fragment->len; i += width) {
        double val = GETRAWSAMPLE(width, fragment->buf, i);
        sum_squares += val*val;
    }
    if (fragment->len == 0)
        res = 0;
    else
        res = (unsigned int)sqrt(sum_squares / (double)(fragment->len/width));
    return PyLong_FromUnsignedLong(res);
}

static double _sum2(const int16_t *a, const int16_t *b, Py_ssize_t len)
{
    Py_ssize_t i;
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
** cp1          A
** cp2          R
** len1         N
** len2         n
** aj_m1        A[j-1]
** aj_lm1       A[j+n-1]
** sum_ri_2     sum(R[i]^2)
** sum_aij_2    sum(A[i+j]^2)
** sum_aij_ri   sum(A[i+j]R[i])
**
** sum_ri is calculated once, sum_aij_2 is updated each step and sum_aij_ri
** is completely recalculated each step.
*/
/*[clinic input]
audioop.findfit

    fragment: Py_buffer
    reference: Py_buffer
    /

Try to match reference as well as possible to a portion of fragment.
[clinic start generated code]*/

static PyObject *
audioop_findfit_impl(PyObject *module, Py_buffer *fragment,
                     Py_buffer *reference)
/*[clinic end generated code: output=5752306d83cbbada input=62c305605e183c9a]*/
{
    const int16_t *cp1, *cp2;
    Py_ssize_t len1, len2;
    Py_ssize_t j, best_j;
    double aj_m1, aj_lm1;
    double sum_ri_2, sum_aij_2, sum_aij_ri, result, best_result, factor;

    if (fragment->len & 1 || reference->len & 1) {
        PyErr_SetString(AudioopError, "Strings should be even-sized");
        return NULL;
    }
    cp1 = (const int16_t *)fragment->buf;
    len1 = fragment->len >> 1;
    cp2 = (const int16_t *)reference->buf;
    len2 = reference->len >> 1;

    if (len1 < len2) {
        PyErr_SetString(AudioopError, "First sample should be longer");
        return NULL;
    }
    sum_ri_2 = _sum2(cp2, cp2, len2);
    sum_aij_2 = _sum2(cp1, cp1, len2);
    sum_aij_ri = _sum2(cp1, cp2, len2);

    result = (sum_ri_2*sum_aij_2 - sum_aij_ri*sum_aij_ri) / sum_aij_2;

    best_result = result;
    best_j = 0;

    for ( j=1; j<=len1-len2; j++) {
        aj_m1 = (double)cp1[j-1];
        aj_lm1 = (double)cp1[j+len2-1];

        sum_aij_2 = sum_aij_2 + aj_lm1*aj_lm1 - aj_m1*aj_m1;
        sum_aij_ri = _sum2(cp1+j, cp2, len2);

        result = (sum_ri_2*sum_aij_2 - sum_aij_ri*sum_aij_ri)
            / sum_aij_2;

        if ( result < best_result ) {
            best_result = result;
            best_j = j;
        }

    }

    factor = _sum2(cp1+best_j, cp2, len2) / sum_ri_2;

    return Py_BuildValue("(nf)", best_j, factor);
}

/*
** findfactor finds a factor f so that the energy in A-fB is minimal.
** See the comment for findfit for details.
*/
/*[clinic input]
audioop.findfactor

    fragment: Py_buffer
    reference: Py_buffer
    /

Return a factor F such that rms(add(fragment, mul(reference, -F))) is minimal.
[clinic start generated code]*/

static PyObject *
audioop_findfactor_impl(PyObject *module, Py_buffer *fragment,
                        Py_buffer *reference)
/*[clinic end generated code: output=14ea95652c1afcf8 input=816680301d012b21]*/
{
    const int16_t *cp1, *cp2;
    Py_ssize_t len;
    double sum_ri_2, sum_aij_ri, result;

    if (fragment->len & 1 || reference->len & 1) {
        PyErr_SetString(AudioopError, "Strings should be even-sized");
        return NULL;
    }
    if (fragment->len != reference->len) {
        PyErr_SetString(AudioopError, "Samples should be same size");
        return NULL;
    }
    cp1 = (const int16_t *)fragment->buf;
    cp2 = (const int16_t *)reference->buf;
    len = fragment->len >> 1;
    sum_ri_2 = _sum2(cp2, cp2, len);
    sum_aij_ri = _sum2(cp1, cp2, len);

    result = sum_aij_ri / sum_ri_2;

    return PyFloat_FromDouble(result);
}

/*
** findmax returns the index of the n-sized segment of the input sample
** that contains the most energy.
*/
/*[clinic input]
audioop.findmax

    fragment: Py_buffer
    length: Py_ssize_t
    /

Search fragment for a slice of specified number of samples with maximum energy.
[clinic start generated code]*/

static PyObject *
audioop_findmax_impl(PyObject *module, Py_buffer *fragment,
                     Py_ssize_t length)
/*[clinic end generated code: output=f008128233523040 input=2f304801ed42383c]*/
{
    const int16_t *cp1;
    Py_ssize_t len1;
    Py_ssize_t j, best_j;
    double aj_m1, aj_lm1;
    double result, best_result;

    if (fragment->len & 1) {
        PyErr_SetString(AudioopError, "Strings should be even-sized");
        return NULL;
    }
    cp1 = (const int16_t *)fragment->buf;
    len1 = fragment->len >> 1;

    if (length < 0 || len1 < length) {
        PyErr_SetString(AudioopError, "Input sample should be longer");
        return NULL;
    }

    result = _sum2(cp1, cp1, length);

    best_result = result;
    best_j = 0;

    for ( j=1; j<=len1-length; j++) {
        aj_m1 = (double)cp1[j-1];
        aj_lm1 = (double)cp1[j+length-1];

        result = result + aj_lm1*aj_lm1 - aj_m1*aj_m1;

        if ( result > best_result ) {
            best_result = result;
            best_j = j;
        }

    }

    return PyLong_FromSsize_t(best_j);
}

/*[clinic input]
audioop.avgpp

    fragment: Py_buffer
    width: int
    /

Return the average peak-peak value over all samples in the fragment.
[clinic start generated code]*/

static PyObject *
audioop_avgpp_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=269596b0d5ae0b2b input=0b3cceeae420a7d9]*/
{
    Py_ssize_t i;
    int prevval, prevextremevalid = 0, prevextreme = 0;
    double sum = 0.0;
    unsigned int avg;
    int diff, prevdiff, nextreme = 0;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    if (fragment->len <= width)
        return PyLong_FromLong(0);
    prevval = GETRAWSAMPLE(width, fragment->buf, 0);
    prevdiff = 17; /* Anything != 0, 1 */
    for (i = width; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i);
        if (val != prevval) {
            diff = val < prevval;
            if (prevdiff == !diff) {
                /* Derivative changed sign. Compute difference to last
                ** extreme value and remember.
                */
                if (prevextremevalid) {
                    if (prevval < prevextreme)
                        sum += (double)((unsigned int)prevextreme -
                                        (unsigned int)prevval);
                    else
                        sum += (double)((unsigned int)prevval -
                                        (unsigned int)prevextreme);
                    nextreme++;
                }
                prevextremevalid = 1;
                prevextreme = prevval;
            }
            prevval = val;
            prevdiff = diff;
        }
    }
    if ( nextreme == 0 )
        avg = 0;
    else
        avg = (unsigned int)(sum / (double)nextreme);
    return PyLong_FromUnsignedLong(avg);
}

/*[clinic input]
audioop.maxpp

    fragment: Py_buffer
    width: int
    /

Return the maximum peak-peak value in the sound fragment.
[clinic start generated code]*/

static PyObject *
audioop_maxpp_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=5b918ed5dbbdb978 input=671a13e1518f80a1]*/
{
    Py_ssize_t i;
    int prevval, prevextremevalid = 0, prevextreme = 0;
    unsigned int max = 0, extremediff;
    int diff, prevdiff;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    if (fragment->len <= width)
        return PyLong_FromLong(0);
    prevval = GETRAWSAMPLE(width, fragment->buf, 0);
    prevdiff = 17; /* Anything != 0, 1 */
    for (i = width; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i);
        if (val != prevval) {
            diff = val < prevval;
            if (prevdiff == !diff) {
                /* Derivative changed sign. Compute difference to
                ** last extreme value and remember.
                */
                if (prevextremevalid) {
                    if (prevval < prevextreme)
                        extremediff = (unsigned int)prevextreme -
                                      (unsigned int)prevval;
                    else
                        extremediff = (unsigned int)prevval -
                                      (unsigned int)prevextreme;
                    if ( extremediff > max )
                        max = extremediff;
                }
                prevextremevalid = 1;
                prevextreme = prevval;
            }
            prevval = val;
            prevdiff = diff;
        }
    }
    return PyLong_FromUnsignedLong(max);
}

/*[clinic input]
audioop.cross

    fragment: Py_buffer
    width: int
    /

Return the number of zero crossings in the fragment passed as an argument.
[clinic start generated code]*/

static PyObject *
audioop_cross_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=5938dcdd74a1f431 input=b1b3f15b83f6b41a]*/
{
    Py_ssize_t i;
    int prevval;
    Py_ssize_t ncross;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    ncross = -1;
    prevval = 17; /* Anything <> 0,1 */
    for (i = 0; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i) < 0;
        if (val != prevval) ncross++;
        prevval = val;
    }
    return PyLong_FromSsize_t(ncross);
}

/*[clinic input]
audioop.mul

    fragment: Py_buffer
    width: int
    factor: double
    /

Return a fragment that has all samples in the original fragment multiplied by the floating-point value factor.
[clinic start generated code]*/

static PyObject *
audioop_mul_impl(PyObject *module, Py_buffer *fragment, int width,
                 double factor)
/*[clinic end generated code: output=6cd48fe796da0ea4 input=c726667baa157d3c]*/
{
    signed char *ncp;
    Py_ssize_t i;
    double maxval, minval;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    maxval = (double) maxvals[width];
    minval = (double) minvals[width];

    rv = PyBytes_FromStringAndSize(NULL, fragment->len);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        double val = GETRAWSAMPLE(width, fragment->buf, i);
        val *= factor;
        val = floor(fbound(val, minval, maxval));
        SETRAWSAMPLE(width, ncp, i, (int)val);
    }
    return rv;
}

/*[clinic input]
audioop.tomono

    fragment: Py_buffer
    width: int
    lfactor: double
    rfactor: double
    /

Convert a stereo fragment to a mono fragment.
[clinic start generated code]*/

static PyObject *
audioop_tomono_impl(PyObject *module, Py_buffer *fragment, int width,
                    double lfactor, double rfactor)
/*[clinic end generated code: output=235c8277216d4e4e input=c4ec949b3f4dddfa]*/
{
    signed char *cp, *ncp;
    Py_ssize_t len, i;
    double maxval, minval;
    PyObject *rv;

    cp = fragment->buf;
    len = fragment->len;
    if (!audioop_check_parameters(len, width))
        return NULL;
    if (((len / width) & 1) != 0) {
        PyErr_SetString(AudioopError, "not a whole number of frames");
        return NULL;
    }

    maxval = (double) maxvals[width];
    minval = (double) minvals[width];

    rv = PyBytes_FromStringAndSize(NULL, len/2);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    for (i = 0; i < len; i += width*2) {
        double val1 = GETRAWSAMPLE(width, cp, i);
        double val2 = GETRAWSAMPLE(width, cp, i + width);
        double val = val1*lfactor + val2*rfactor;
        val = floor(fbound(val, minval, maxval));
        SETRAWSAMPLE(width, ncp, i/2, val);
    }
    return rv;
}

/*[clinic input]
audioop.tostereo

    fragment: Py_buffer
    width: int
    lfactor: double
    rfactor: double
    /

Generate a stereo fragment from a mono fragment.
[clinic start generated code]*/

static PyObject *
audioop_tostereo_impl(PyObject *module, Py_buffer *fragment, int width,
                      double lfactor, double rfactor)
/*[clinic end generated code: output=046f13defa5f1595 input=27b6395ebfdff37a]*/
{
    signed char *ncp;
    Py_ssize_t i;
    double maxval, minval;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    maxval = (double) maxvals[width];
    minval = (double) minvals[width];

    if (fragment->len > PY_SSIZE_T_MAX/2) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }

    rv = PyBytes_FromStringAndSize(NULL, fragment->len*2);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        double val = GETRAWSAMPLE(width, fragment->buf, i);
        int val1 = (int)floor(fbound(val*lfactor, minval, maxval));
        int val2 = (int)floor(fbound(val*rfactor, minval, maxval));
        SETRAWSAMPLE(width, ncp, i*2, val1);
        SETRAWSAMPLE(width, ncp, i*2 + width, val2);
    }
    return rv;
}

/*[clinic input]
audioop.add

    fragment1: Py_buffer
    fragment2: Py_buffer
    width: int
    /

Return a fragment which is the addition of the two samples passed as parameters.
[clinic start generated code]*/

static PyObject *
audioop_add_impl(PyObject *module, Py_buffer *fragment1,
                 Py_buffer *fragment2, int width)
/*[clinic end generated code: output=60140af4d1aab6f2 input=4a8d4bae4c1605c7]*/
{
    signed char *ncp;
    Py_ssize_t i;
    int minval, maxval, newval;
    PyObject *rv;

    if (!audioop_check_parameters(fragment1->len, width))
        return NULL;
    if (fragment1->len != fragment2->len) {
        PyErr_SetString(AudioopError, "Lengths should be the same");
        return NULL;
    }

    maxval = maxvals[width];
    minval = minvals[width];

    rv = PyBytes_FromStringAndSize(NULL, fragment1->len);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment1->len; i += width) {
        int val1 = GETRAWSAMPLE(width, fragment1->buf, i);
        int val2 = GETRAWSAMPLE(width, fragment2->buf, i);

        if (width < 4) {
            newval = val1 + val2;
            /* truncate in case of overflow */
            if (newval > maxval)
                newval = maxval;
            else if (newval < minval)
                newval = minval;
        }
        else {
            double fval = (double)val1 + (double)val2;
            /* truncate in case of overflow */
            newval = (int)floor(fbound(fval, minval, maxval));
        }

        SETRAWSAMPLE(width, ncp, i, newval);
    }
    return rv;
}

/*[clinic input]
audioop.bias

    fragment: Py_buffer
    width: int
    bias: int
    /

Return a fragment that is the original fragment with a bias added to each sample.
[clinic start generated code]*/

static PyObject *
audioop_bias_impl(PyObject *module, Py_buffer *fragment, int width, int bias)
/*[clinic end generated code: output=6e0aa8f68f045093 input=2b5cce5c3bb4838c]*/
{
    signed char *ncp;
    Py_ssize_t i;
    unsigned int val = 0, mask;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    rv = PyBytes_FromStringAndSize(NULL, fragment->len);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    mask = masks[width];

    for (i = 0; i < fragment->len; i += width) {
        if (width == 1)
            val = GETINTX(unsigned char, fragment->buf, i);
        else if (width == 2)
            val = GETINTX(uint16_t, fragment->buf, i);
        else if (width == 3)
            val = ((unsigned int)GETINT24(fragment->buf, i)) & 0xffffffu;
        else {
            assert(width == 4);
            val = GETINTX(uint32_t, fragment->buf, i);
        }

        val += (unsigned int)bias;
        /* wrap around in case of overflow */
        val &= mask;

        if (width == 1)
            SETINTX(unsigned char, ncp, i, val);
        else if (width == 2)
            SETINTX(uint16_t, ncp, i, val);
        else if (width == 3)
            SETINT24(ncp, i, (int)val);
        else {
            assert(width == 4);
            SETINTX(uint32_t, ncp, i, val);
        }
    }
    return rv;
}

/*[clinic input]
audioop.reverse

    fragment: Py_buffer
    width: int
    /

Reverse the samples in a fragment and returns the modified fragment.
[clinic start generated code]*/

static PyObject *
audioop_reverse_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=b44135698418da14 input=668f890cf9f9d225]*/
{
    unsigned char *ncp;
    Py_ssize_t i;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    rv = PyBytes_FromStringAndSize(NULL, fragment->len);
    if (rv == NULL)
        return NULL;
    ncp = (unsigned char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        int val = GETRAWSAMPLE(width, fragment->buf, i);
        SETRAWSAMPLE(width, ncp, fragment->len - i - width, val);
    }
    return rv;
}

/*[clinic input]
audioop.byteswap

    fragment: Py_buffer
    width: int
    /

Convert big-endian samples to little-endian and vice versa.
[clinic start generated code]*/

static PyObject *
audioop_byteswap_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=50838a9e4b87cd4d input=fae7611ceffa5c82]*/
{
    unsigned char *ncp;
    Py_ssize_t i;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    rv = PyBytes_FromStringAndSize(NULL, fragment->len);
    if (rv == NULL)
        return NULL;
    ncp = (unsigned char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        int j;
        for (j = 0; j < width; j++)
            ncp[i + width - 1 - j] = ((unsigned char *)fragment->buf)[i + j];
    }
    return rv;
}

/*[clinic input]
audioop.lin2lin

    fragment: Py_buffer
    width: int
    newwidth: int
    /

Convert samples between 1-, 2-, 3- and 4-byte formats.
[clinic start generated code]*/

static PyObject *
audioop_lin2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                     int newwidth)
/*[clinic end generated code: output=17b14109248f1d99 input=5ce08c8aa2f24d96]*/
{
    unsigned char *ncp;
    Py_ssize_t i, j;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;
    if (!audioop_check_size(newwidth))
        return NULL;

    if (fragment->len/width > PY_SSIZE_T_MAX/newwidth) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }
    rv = PyBytes_FromStringAndSize(NULL, (fragment->len/width)*newwidth);
    if (rv == NULL)
        return NULL;
    ncp = (unsigned char *)PyBytes_AsString(rv);

    for (i = j = 0; i < fragment->len; i += width, j += newwidth) {
        int val = GETSAMPLE32(width, fragment->buf, i);
        SETSAMPLE32(newwidth, ncp, j, val);
    }
    return rv;
}

static int
gcd(int a, int b)
{
    while (b > 0) {
        int tmp = a % b;
        a = b;
        b = tmp;
    }
    return a;
}

/*[clinic input]
audioop.ratecv

    fragment: Py_buffer
    width: int
    nchannels: int
    inrate: int
    outrate: int
    state: object
    weightA: int = 1
    weightB: int = 0
    /

Convert the frame rate of the input fragment.
[clinic start generated code]*/

static PyObject *
audioop_ratecv_impl(PyObject *module, Py_buffer *fragment, int width,
                    int nchannels, int inrate, int outrate, PyObject *state,
                    int weightA, int weightB)
/*[clinic end generated code: output=624038e843243139 input=aff3acdc94476191]*/
{
    char *cp, *ncp;
    Py_ssize_t len;
    int chan, d, *prev_i, *cur_i, cur_o;
    PyObject *samps, *str, *rv = NULL;
    int bytes_per_frame;

    if (!audioop_check_size(width))
        return NULL;
    if (nchannels < 1) {
        PyErr_SetString(AudioopError, "# of channels should be >= 1");
        return NULL;
    }
    if (width > INT_MAX / nchannels) {
        /* This overflow test is rigorously correct because
           both multiplicands are >= 1.  Use the argument names
           from the docs for the error msg. */
        PyErr_SetString(PyExc_OverflowError,
                        "width * nchannels too big for a C int");
        return NULL;
    }
    bytes_per_frame = width * nchannels;
    if (weightA < 1 || weightB < 0) {
        PyErr_SetString(AudioopError,
            "weightA should be >= 1, weightB should be >= 0");
        return NULL;
    }
    assert(fragment->len >= 0);
    if (fragment->len % bytes_per_frame != 0) {
        PyErr_SetString(AudioopError, "not a whole number of frames");
        return NULL;
    }
    if (inrate <= 0 || outrate <= 0) {
        PyErr_SetString(AudioopError, "sampling rate not > 0");
        return NULL;
    }
    /* divide inrate and outrate by their greatest common divisor */
    d = gcd(inrate, outrate);
    inrate /= d;
    outrate /= d;
    /* divide weightA and weightB by their greatest common divisor */
    d = gcd(weightA, weightB);
    weightA /= d;
    weightB /= d;

    if ((size_t)nchannels > SIZE_MAX/sizeof(int)) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }
    prev_i = (int *) PyMem_Malloc(nchannels * sizeof(int));
    cur_i = (int *) PyMem_Malloc(nchannels * sizeof(int));
    if (prev_i == NULL || cur_i == NULL) {
        (void) PyErr_NoMemory();
        goto exit;
    }

    len = fragment->len / bytes_per_frame; /* # of frames */

    if (state == Py_None) {
        d = -outrate;
        for (chan = 0; chan < nchannels; chan++)
            prev_i[chan] = cur_i[chan] = 0;
    }
    else {
        if (!PyArg_ParseTuple(state,
                        "iO!;audioop.ratecv: illegal state argument",
                        &d, &PyTuple_Type, &samps))
            goto exit;
        if (PyTuple_Size(samps) != nchannels) {
            PyErr_SetString(AudioopError,
                            "illegal state argument");
            goto exit;
        }
        for (chan = 0; chan < nchannels; chan++) {
            if (!PyArg_ParseTuple(PyTuple_GetItem(samps, chan),
                                  "ii:ratecv", &prev_i[chan],
                                               &cur_i[chan]))
                goto exit;
        }
    }

    /* str <- Space for the output buffer. */
    if (len == 0)
        str = PyBytes_FromStringAndSize(NULL, 0);
    else {
        /* There are len input frames, so we need (mathematically)
           ceiling(len*outrate/inrate) output frames, and each frame
           requires bytes_per_frame bytes.  Computing this
           without spurious overflow is the challenge; we can
           settle for a reasonable upper bound, though, in this
           case ceiling(len/inrate) * outrate. */

        /* compute ceiling(len/inrate) without overflow */
        Py_ssize_t q = 1 + (len - 1) / inrate;
        if (outrate > PY_SSIZE_T_MAX / q / bytes_per_frame)
            str = NULL;
        else
            str = PyBytes_FromStringAndSize(NULL,
                                            q * outrate * bytes_per_frame);
    }
    if (str == NULL) {
        PyErr_SetString(PyExc_MemoryError,
            "not enough memory for output buffer");
        goto exit;
    }
    ncp = PyBytes_AsString(str);
    cp = fragment->buf;

    for (;;) {
        while (d < 0) {
            if (len == 0) {
                samps = PyTuple_New(nchannels);
                if (samps == NULL)
                    goto exit;
                for (chan = 0; chan < nchannels; chan++)
                    PyTuple_SetItem(samps, chan,
                        Py_BuildValue("(ii)",
                                      prev_i[chan],
                                      cur_i[chan]));
                if (PyErr_Occurred())
                    goto exit;
                /* We have checked before that the length
                 * of the string fits into int. */
                len = (Py_ssize_t)(ncp - PyBytes_AsString(str));
                rv = PyBytes_FromStringAndSize
                    (PyBytes_AsString(str), len);
                Py_DECREF(str);
                str = rv;
                if (str == NULL)
                    goto exit;
                rv = Py_BuildValue("(O(iO))", str, d, samps);
                Py_DECREF(samps);
                Py_DECREF(str);
                goto exit; /* return rv */
            }
            for (chan = 0; chan < nchannels; chan++) {
                prev_i[chan] = cur_i[chan];
                cur_i[chan] = GETSAMPLE32(width, cp, 0);
                cp += width;
                /* implements a simple digital filter */
                cur_i[chan] = (int)(
                    ((double)weightA * (double)cur_i[chan] +
                     (double)weightB * (double)prev_i[chan]) /
                    ((double)weightA + (double)weightB));
            }
            len--;
            d += outrate;
        }
        while (d >= 0) {
            for (chan = 0; chan < nchannels; chan++) {
                cur_o = (int)(((double)prev_i[chan] * (double)d +
                         (double)cur_i[chan] * (double)(outrate - d)) /
                    (double)outrate);
                SETSAMPLE32(width, ncp, 0, cur_o);
                ncp += width;
            }
            d -= inrate;
        }
    }
  exit:
    PyMem_Free(prev_i);
    PyMem_Free(cur_i);
    return rv;
}

/*[clinic input]
audioop.lin2ulaw

    fragment: Py_buffer
    width: int
    /

Convert samples in the audio fragment to u-LAW encoding.
[clinic start generated code]*/

static PyObject *
audioop_lin2ulaw_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=14fb62b16fe8ea8e input=2450d1b870b6bac2]*/
{
    unsigned char *ncp;
    Py_ssize_t i;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    rv = PyBytes_FromStringAndSize(NULL, fragment->len/width);
    if (rv == NULL)
        return NULL;
    ncp = (unsigned char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        int val = GETSAMPLE32(width, fragment->buf, i);
        *ncp++ = st_14linear2ulaw(val >> 18);
    }
    return rv;
}

/*[clinic input]
audioop.ulaw2lin

    fragment: Py_buffer
    width: int
    /

Convert sound fragments in u-LAW encoding to linearly encoded sound fragments.
[clinic start generated code]*/

static PyObject *
audioop_ulaw2lin_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=378356b047521ba2 input=45d53ddce5be7d06]*/
{
    unsigned char *cp;
    signed char *ncp;
    Py_ssize_t i;
    PyObject *rv;

    if (!audioop_check_size(width))
        return NULL;

    if (fragment->len > PY_SSIZE_T_MAX/width) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }
    rv = PyBytes_FromStringAndSize(NULL, fragment->len*width);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);

    cp = fragment->buf;
    for (i = 0; i < fragment->len*width; i += width) {
        int val = st_ulaw2linear16(*cp++) << 16;
        SETSAMPLE32(width, ncp, i, val);
    }
    return rv;
}

/*[clinic input]
audioop.lin2alaw

    fragment: Py_buffer
    width: int
    /

Convert samples in the audio fragment to a-LAW encoding.
[clinic start generated code]*/

static PyObject *
audioop_lin2alaw_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=d076f130121a82f0 input=ffb1ef8bb39da945]*/
{
    unsigned char *ncp;
    Py_ssize_t i;
    PyObject *rv;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    rv = PyBytes_FromStringAndSize(NULL, fragment->len/width);
    if (rv == NULL)
        return NULL;
    ncp = (unsigned char *)PyBytes_AsString(rv);

    for (i = 0; i < fragment->len; i += width) {
        int val = GETSAMPLE32(width, fragment->buf, i);
        *ncp++ = st_linear2alaw(val >> 19);
    }
    return rv;
}

/*[clinic input]
audioop.alaw2lin

    fragment: Py_buffer
    width: int
    /

Convert sound fragments in a-LAW encoding to linearly encoded sound fragments.
[clinic start generated code]*/

static PyObject *
audioop_alaw2lin_impl(PyObject *module, Py_buffer *fragment, int width)
/*[clinic end generated code: output=85c365ec559df647 input=4140626046cd1772]*/
{
    unsigned char *cp;
    signed char *ncp;
    Py_ssize_t i;
    int val;
    PyObject *rv;

    if (!audioop_check_size(width))
        return NULL;

    if (fragment->len > PY_SSIZE_T_MAX/width) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }
    rv = PyBytes_FromStringAndSize(NULL, fragment->len*width);
    if (rv == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(rv);
    cp = fragment->buf;

    for (i = 0; i < fragment->len*width; i += width) {
        val = st_alaw2linear16(*cp++) << 16;
        SETSAMPLE32(width, ncp, i, val);
    }
    return rv;
}

/*[clinic input]
audioop.lin2adpcm

    fragment: Py_buffer
    width: int
    state: object
    /

Convert samples to 4 bit Intel/DVI ADPCM encoding.
[clinic start generated code]*/

static PyObject *
audioop_lin2adpcm_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state)
/*[clinic end generated code: output=cc19f159f16c6793 input=12919d549b90c90a]*/
{
    signed char *ncp;
    Py_ssize_t i;
    int step, valpred, delta,
        index, sign, vpdiff, diff;
    PyObject *rv = NULL, *str;
    int outputbuffer = 0, bufferstep;

    if (!audioop_check_parameters(fragment->len, width))
        return NULL;

    /* Decode state, should have (value, step) */
    if ( state == Py_None ) {
        /* First time, it seems. Set defaults */
        valpred = 0;
        index = 0;
    }
    else if (!PyTuple_Check(state)) {
        PyErr_SetString(PyExc_TypeError, "state must be a tuple or None");
        return NULL;
    }
    else if (!PyArg_ParseTuple(state, "ii", &valpred, &index)) {
        return NULL;
    }
    else if (valpred >= 0x8000 || valpred < -0x8000 ||
             (size_t)index >= Py_ARRAY_LENGTH(stepsizeTable)) {
        PyErr_SetString(PyExc_ValueError, "bad state");
        return NULL;
    }

    str = PyBytes_FromStringAndSize(NULL, fragment->len/(width*2));
    if (str == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(str);

    step = stepsizeTable[index];
    bufferstep = 1;

    for (i = 0; i < fragment->len; i += width) {
        int val = GETSAMPLE32(width, fragment->buf, i) >> 16;

        /* Step 1 - compute difference with previous value */
        if (val < valpred) {
            diff = valpred - val;
            sign = 8;
        }
        else {
            diff = val - valpred;
            sign = 0;
        }

        /* Step 2 - Divide and clamp */
        /* Note:
        ** This code *approximately* computes:
        **    delta = diff*4/step;
        **    vpdiff = (delta+0.5)*step/4;
        ** but in shift step bits are dropped. The net result of this
        ** is that even if you have fast mul/div hardware you cannot
        ** put it to good use since the fixup would be too expensive.
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
    rv = Py_BuildValue("(O(ii))", str, valpred, index);
    Py_DECREF(str);
    return rv;
}

/*[clinic input]
audioop.adpcm2lin

    fragment: Py_buffer
    width: int
    state: object
    /

Decode an Intel/DVI ADPCM coded fragment to a linear fragment.
[clinic start generated code]*/

static PyObject *
audioop_adpcm2lin_impl(PyObject *module, Py_buffer *fragment, int width,
                       PyObject *state)
/*[clinic end generated code: output=3440ea105acb3456 input=f5221144f5ca9ef0]*/
{
    signed char *cp;
    signed char *ncp;
    Py_ssize_t i, outlen;
    int valpred, step, delta, index, sign, vpdiff;
    PyObject *rv, *str;
    int inputbuffer = 0, bufferstep;

    if (!audioop_check_size(width))
        return NULL;

    /* Decode state, should have (value, step) */
    if ( state == Py_None ) {
        /* First time, it seems. Set defaults */
        valpred = 0;
        index = 0;
    }
    else if (!PyTuple_Check(state)) {
        PyErr_SetString(PyExc_TypeError, "state must be a tuple or None");
        return NULL;
    }
    else if (!PyArg_ParseTuple(state, "ii", &valpred, &index)) {
        return NULL;
    }
    else if (valpred >= 0x8000 || valpred < -0x8000 ||
             (size_t)index >= Py_ARRAY_LENGTH(stepsizeTable)) {
        PyErr_SetString(PyExc_ValueError, "bad state");
        return NULL;
    }

    if (fragment->len > (PY_SSIZE_T_MAX/2)/width) {
        PyErr_SetString(PyExc_MemoryError,
                        "not enough memory for output buffer");
        return NULL;
    }
    outlen = fragment->len*width*2;
    str = PyBytes_FromStringAndSize(NULL, outlen);
    if (str == NULL)
        return NULL;
    ncp = (signed char *)PyBytes_AsString(str);
    cp = fragment->buf;

    step = stepsizeTable[index];
    bufferstep = 0;

    for (i = 0; i < outlen; i += width) {
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
        SETSAMPLE32(width, ncp, i, valpred << 16);
    }

    rv = Py_BuildValue("(O(ii))", str, valpred, index);
    Py_DECREF(str);
    return rv;
}

#include "clinic/audioop.c.h"

static PyMethodDef audioop_methods[] = {
    AUDIOOP_MAX_METHODDEF
    AUDIOOP_MINMAX_METHODDEF
    AUDIOOP_AVG_METHODDEF
    AUDIOOP_MAXPP_METHODDEF
    AUDIOOP_AVGPP_METHODDEF
    AUDIOOP_RMS_METHODDEF
    AUDIOOP_FINDFIT_METHODDEF
    AUDIOOP_FINDMAX_METHODDEF
    AUDIOOP_FINDFACTOR_METHODDEF
    AUDIOOP_CROSS_METHODDEF
    AUDIOOP_MUL_METHODDEF
    AUDIOOP_ADD_METHODDEF
    AUDIOOP_BIAS_METHODDEF
    AUDIOOP_ULAW2LIN_METHODDEF
    AUDIOOP_LIN2ULAW_METHODDEF
    AUDIOOP_ALAW2LIN_METHODDEF
    AUDIOOP_LIN2ALAW_METHODDEF
    AUDIOOP_LIN2LIN_METHODDEF
    AUDIOOP_ADPCM2LIN_METHODDEF
    AUDIOOP_LIN2ADPCM_METHODDEF
    AUDIOOP_TOMONO_METHODDEF
    AUDIOOP_TOSTEREO_METHODDEF
    AUDIOOP_GETSAMPLE_METHODDEF
    AUDIOOP_REVERSE_METHODDEF
    AUDIOOP_BYTESWAP_METHODDEF
    AUDIOOP_RATECV_METHODDEF
    { 0,          0 }
};


static struct PyModuleDef audioopmodule = {
    PyModuleDef_HEAD_INIT,
    "audioop",
    NULL,
    -1,
    audioop_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_audioop(void)
{
    PyObject *m, *d;
    m = PyModule_Create(&audioopmodule);
    if (m == NULL)
        return NULL;
    d = PyModule_GetDict(m);
    if (d == NULL)
        return NULL;
    AudioopError = PyErr_NewException("audioop.error", NULL, NULL);
    if (AudioopError != NULL)
         PyDict_SetItemString(d,"error",AudioopError);
    return m;
}
