/*
 * tclStrToD.c --
 *
 *	This file contains a collection of procedures for managing conversions
 *	to/from floating-point in Tcl. They include TclParseNumber, which
 *	parses numbers from strings; TclDoubleDigits, which formats numbers
 *	into strings of digits, and procedures for interconversion among
 *	'double' and 'mp_int' types.
 *
 * Copyright (c) 2005 by Kevin B. Kenny. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tommath.h"
#include <math.h>

/*
 * Define KILL_OCTAL to suppress interpretation of numbers with leading zero
 * as octal. (Ceterum censeo: numeros octonarios delendos esse.)
 */

#undef	KILL_OCTAL

/*
 * This code supports (at least hypothetically), IBM, Cray, VAX and IEEE-754
 * floating point; of these, only IEEE-754 can represent NaN. IEEE-754 can be
 * uniquely determined by radix and by the widths of significand and exponent.
 */

#if (FLT_RADIX == 2) && (DBL_MANT_DIG == 53) && (DBL_MAX_EXP == 1024)
#   define IEEE_FLOATING_POINT
#endif

/*
 * Rounding controls. (Thanks a lot, Intel!)
 */

#ifdef __i386
/*
 * gcc on x86 needs access to rounding controls, because of a questionable
 * feature where it retains intermediate results as IEEE 'long double' values
 * somewhat unpredictably. It is tempting to include fpu_control.h, but that
 * file exists only on Linux; it is missing on Cygwin and MinGW. Most gcc-isms
 * and ix86-isms are factored out here.
 */

#if defined(__GNUC__)
typedef unsigned int	fpu_control_t __attribute__ ((__mode__ (__HI__)));

#define _FPU_GETCW(cw)	__asm__ __volatile__ ("fnstcw %0" : "=m" (*&cw))
#define _FPU_SETCW(cw)	__asm__ __volatile__ ("fldcw %0" : : "m" (*&cw))
#   define FPU_IEEE_ROUNDING	0x027f
#   define ADJUST_FPU_CONTROL_WORD
#define TCL_IEEE_DOUBLE_ROUNDING \
    fpu_control_t roundTo53Bits = FPU_IEEE_ROUNDING;	\
    fpu_control_t oldRoundingMode;			\
    _FPU_GETCW(oldRoundingMode);			\
    _FPU_SETCW(roundTo53Bits)
#define TCL_DEFAULT_DOUBLE_ROUNDING \
    _FPU_SETCW(oldRoundingMode)

/*
 * Sun ProC needs sunmath for rounding control on x86 like gcc above.
 */
#elif defined(__sun)
#include <sunmath.h>
#define TCL_IEEE_DOUBLE_ROUNDING \
    ieee_flags("set","precision","double",NULL)
#define TCL_DEFAULT_DOUBLE_ROUNDING \
    ieee_flags("clear","precision",NULL,NULL)

/*
 * Other platforms are assumed to always operate in full IEEE mode, so we make
 * the macros to go in and out of that mode do nothing.
 */

#else /* !__GNUC__ && !__sun */
#define TCL_IEEE_DOUBLE_ROUNDING	((void) 0)
#define TCL_DEFAULT_DOUBLE_ROUNDING	((void) 0)
#endif
#else /* !__i386 */
#define TCL_IEEE_DOUBLE_ROUNDING	((void) 0)
#define TCL_DEFAULT_DOUBLE_ROUNDING	((void) 0)
#endif

/*
 * MIPS floating-point units need special settings in control registers to use
 * gradual underflow as we expect.  This fix is for the MIPSpro compiler.
 */

#if defined(__sgi) && defined(_COMPILER_VERSION)
#include <sys/fpu.h>
#endif

/*
 * HP's PA_RISC architecture uses 7ff4000000000000 to represent a quiet NaN.
 * Everyone else uses 7ff8000000000000. (Why, HP, why?)
 */

#ifdef __hppa
#   define NAN_START	0x7ff4
#   define NAN_MASK	(((Tcl_WideUInt) 1) << 50)
#else
#   define NAN_START	0x7ff8
#   define NAN_MASK	(((Tcl_WideUInt) 1) << 51)
#endif

/*
 * Constants used by this file (most of which are only ever calculated at
 * runtime).
 */

/* Magic constants */

#define LOG10_2 0.3010299956639812
#define TWO_OVER_3LOG10 0.28952965460216784
#define LOG10_3HALVES_PLUS_FUDGE 0.1760912590558

/*
 * Definitions of the parts of an IEEE754-format floating point number.
 */

#define SIGN_BIT 	0x80000000
				/* Mask for the sign bit in the first word of
				 * a double. */
#define EXP_MASK	0x7ff00000
				/* Mask for the exponent field in the first
				 * word of a double. */
#define EXP_SHIFT	20	/* Shift count to make the exponent an
				 * integer. */
#define HIDDEN_BIT	(((Tcl_WideUInt) 0x00100000) << 32)
				/* Hidden 1 bit for the significand. */
#define HI_ORDER_SIG_MASK 0x000fffff
				/* Mask for the high-order part of the
				 * significand in the first word of a
				 * double. */
#define SIG_MASK	(((Tcl_WideUInt) HI_ORDER_SIG_MASK << 32) \
			| 0xffffffff)
				/* Mask for the 52-bit significand. */
#define FP_PRECISION	53	/* Number of bits of significand plus the
				 * hidden bit. */
#define EXPONENT_BIAS	0x3ff	/* Bias of the exponent 0. */

/*
 * Derived quantities.
 */

#define TEN_PMAX	22	/* floor(FP_PRECISION*log(2)/log(5)) */
#define QUICK_MAX	14	/* floor((FP_PRECISION-1)*log(2)/log(10))-1 */
#define BLETCH		0x10	/* Highest power of two that is greater than
				 * DBL_MAX_10_EXP, divided by 16. */
#define DIGIT_GROUP	8	/* floor(DIGIT_BIT*log(2)/log(10)) */

/*
 * Union used to dismantle floating point numbers.
 */

typedef union Double {
    struct {
#ifdef WORDS_BIGENDIAN
	int word0;
	int word1;
#else
	int word1;
	int word0;
#endif
    } w;
    double d;
    Tcl_WideUInt q;
} Double;

static int maxpow10_wide;	/* The powers of ten that can be represented
				 * exactly as wide integers. */
static Tcl_WideUInt *pow10_wide;
#define MAXPOW	22
static double pow10vals[MAXPOW+1];
				/* The powers of ten that can be represented
				 * exactly as IEEE754 doubles. */
static int mmaxpow;		/* Largest power of ten that can be
				 * represented exactly in a 'double'. */
static int log10_DIGIT_MAX;	/* The number of decimal digits that fit in an
				 * mp_digit. */
static int log2FLT_RADIX;	/* Logarithm of the floating point radix. */
static int mantBits;		/* Number of bits in a double's significand */
static mp_int pow5[9];		/* Table of powers of 5**(2**n), up to
				 * 5**256 */
static double tiny = 0.0;	/* The smallest representable double. */
static int maxDigits;		/* The maximum number of digits to the left of
				 * the decimal point of a double. */
static int minDigits;		/* The maximum number of digits to the right
				 * of the decimal point in a double. */
static const double pow_10_2_n[] = {	/* Inexact higher powers of ten. */
    1.0,
    100.0,
    10000.0,
    1.0e+8,
    1.0e+16,
    1.0e+32,
    1.0e+64,
    1.0e+128,
    1.0e+256
};

static int n770_fp;		/* Flag is 1 on Nokia N770 floating point.
				 * Nokia's floating point has the words
				 * reversed: if big-endian is 7654 3210,
				 * and little-endian is       0123 4567,
				 * then Nokia's FP is         4567 0123;
				 * little-endian within the 32-bit words but
				 * big-endian between them. */

/*
 * Table of powers of 5 that are small enough to fit in an mp_digit.
 */

static const mp_digit dpow5[13] = {
               1,              5,             25,            125,
             625,           3125,          15625,          78125,
          390625,        1953125,        9765625,       48828125,
       244140625
};

/*
 * Table of powers: pow5_13[n] = 5**(13*2**(n+1))
 */

static mp_int pow5_13[5];	/* Table of powers: 5**13, 5**26, 5**52,
				 * 5**104, 5**208 */
static const double tens[] = {
    1e00, 1e01, 1e02, 1e03, 1e04, 1e05, 1e06, 1e07, 1e08, 1e09,
    1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
    1e20, 1e21, 1e22
};

static const int itens [] = {
    1,
    10,
    100,
    1000,
    10000,
    100000,
    1000000,
    10000000,
    100000000
};

static const double bigtens[] = {
    1e016, 1e032, 1e064, 1e128, 1e256
};
#define N_BIGTENS 5

static const int log2pow5[27] = {
    01,  3,  5,  7, 10, 12, 14, 17, 19, 21,
    24, 26, 28, 31, 33, 35, 38, 40, 42, 45,
    47, 49, 52, 54, 56, 59, 61
};
#define N_LOG2POW5 27

static const Tcl_WideUInt wuipow5[27] = {
    (Tcl_WideUInt) 1,		/* 5**0 */
    (Tcl_WideUInt) 5,
    (Tcl_WideUInt) 25,
    (Tcl_WideUInt) 125,
    (Tcl_WideUInt) 625,
    (Tcl_WideUInt) 3125,	/* 5**5 */
    (Tcl_WideUInt) 3125*5,
    (Tcl_WideUInt) 3125*25,
    (Tcl_WideUInt) 3125*125,
    (Tcl_WideUInt) 3125*625,
    (Tcl_WideUInt) 3125*3125,	/* 5**10 */
    (Tcl_WideUInt) 3125*3125*5,
    (Tcl_WideUInt) 3125*3125*25,
    (Tcl_WideUInt) 3125*3125*125,
    (Tcl_WideUInt) 3125*3125*625,
    (Tcl_WideUInt) 3125*3125*3125, /* 5**15 */
    (Tcl_WideUInt) 3125*3125*3125*5,
    (Tcl_WideUInt) 3125*3125*3125*25,
    (Tcl_WideUInt) 3125*3125*3125*125,
    (Tcl_WideUInt) 3125*3125*3125*625,
    (Tcl_WideUInt) 3125*3125*3125*3125,	/* 5**20 */
    (Tcl_WideUInt) 3125*3125*3125*3125*5,
    (Tcl_WideUInt) 3125*3125*3125*3125*25,
    (Tcl_WideUInt) 3125*3125*3125*3125*125,
    (Tcl_WideUInt) 3125*3125*3125*3125*625,
    (Tcl_WideUInt) 3125*3125*3125*3125*3125,  /* 5**25 */
    (Tcl_WideUInt) 3125*3125*3125*3125*3125*5 /* 5**26 */
};

/*
 * Static functions defined in this file.
 */

static int		AccumulateDecimalDigit(unsigned, int,
			    Tcl_WideUInt *, mp_int *, int);
static double		MakeHighPrecisionDouble(int signum,
			    mp_int *significand, int nSigDigs, int exponent);
static double		MakeLowPrecisionDouble(int signum,
			    Tcl_WideUInt significand, int nSigDigs,
			    int exponent);
#ifdef IEEE_FLOATING_POINT
static double		MakeNaN(int signum, Tcl_WideUInt tag);
#endif
static double		RefineApproximation(double approx,
			    mp_int *exactSignificand, int exponent);
static void		MulPow5(mp_int *, unsigned, mp_int *);
static int 		NormalizeRightward(Tcl_WideUInt *);
static int		RequiredPrecision(Tcl_WideUInt);
static void		DoubleToExpAndSig(double, Tcl_WideUInt *, int *,
			    int *);
static void		TakeAbsoluteValue(Double *, int *);
static char *		FormatInfAndNaN(Double *, int *, char **);
static char *		FormatZero(int *, char **);
static int		ApproximateLog10(Tcl_WideUInt, int, int);
static int		BetterLog10(double, int, int *);
static void		ComputeScale(int, int, int *, int *, int *, int *);
static void		SetPrecisionLimits(int, int, int *, int *, int *,
			    int *);
static char *		BumpUp(char *, char *, int *);
static int		AdjustRange(double *, int);
static char *		ShorteningQuickFormat(double, int, int, double,
			    char *, int *);
static char *		StrictQuickFormat(double, int, int, double,
			    char *, int *);
static char *		QuickConversion(double, int, int, int, int, int, int,
			    int *, char **);
static void		CastOutPowersOf2(int *, int *, int *);
static char *		ShorteningInt64Conversion(Double *, int, Tcl_WideUInt,
			    int, int, int, int, int, int, int, int, int,
			    int, int, int *, char **);
static char *		StrictInt64Conversion(Double *, int, Tcl_WideUInt,
			    int, int, int, int, int, int,
			    int, int, int *, char **);
static int		ShouldBankerRoundUpPowD(mp_int *, int, int);
static int		ShouldBankerRoundUpToNextPowD(mp_int *, mp_int *,
			    int, int, int, mp_int *);
static char *		ShorteningBignumConversionPowD(Double *dPtr,
			    int convType, Tcl_WideUInt bw, int b2, int b5,
			    int m2plus, int m2minus, int m5,
			    int sd, int k, int len,
			    int ilim, int ilim1, int *decpt,
			    char **endPtr);
static char *		StrictBignumConversionPowD(Double *dPtr, int convType,
			    Tcl_WideUInt bw, int b2, int b5,
			    int sd, int k, int len,
			    int ilim, int ilim1, int *decpt,
			    char **endPtr);
static int		ShouldBankerRoundUp(mp_int *, mp_int *, int);
static int		ShouldBankerRoundUpToNext(mp_int *, mp_int *,
			    mp_int *, int, int, mp_int *);
static char *		ShorteningBignumConversion(Double *dPtr, int convType,
			    Tcl_WideUInt bw, int b2,
			    int m2plus, int m2minus,
			    int s2, int s5, int k, int len,
			    int ilim, int ilim1, int *decpt,
			    char **endPtr);
static char *		StrictBignumConversion(Double *dPtr, int convType,
			    Tcl_WideUInt bw, int b2,
			    int s2, int s5, int k, int len,
			    int ilim, int ilim1, int *decpt,
			    char **endPtr);
static double		BignumToBiasedFrExp(const mp_int *big, int *machexp);
static double		Pow10TimesFrExp(int exponent, double fraction,
			    int *machexp);
static double		SafeLdExp(double fraction, int exponent);
#ifdef IEEE_FLOATING_POINT
static Tcl_WideUInt	Nokia770Twiddle(Tcl_WideUInt w);
#endif

/*
 *----------------------------------------------------------------------
 *
 * TclParseNumber --
 *
 *	Scans bytes, interpreted as characters in Tcl's internal encoding, and
 *	parses the longest prefix that is the string representation of a
 *	number in a format recognized by Tcl.
 *
 *	The arguments bytes, numBytes, and objPtr are the inputs which
 *	determine the string to be parsed. If bytes is non-NULL, it points to
 *	the first byte to be scanned. If bytes is NULL, then objPtr must be
 *	non-NULL, and the string representation of objPtr will be scanned
 *	(generated first, if necessary). The numBytes argument determines the
 *	number of bytes to be scanned. If numBytes is negative, the first NUL
 *	byte encountered will terminate the scan. If numBytes is non-negative,
 *	then no more than numBytes bytes will be scanned.
 *
 *	The argument flags is an input that controls the numeric formats
 *	recognized by the parser. The flag bits are:
 *
 *	- TCL_PARSE_INTEGER_ONLY:	accept only integer values; reject
 *		strings that denote floating point values (or accept only the
 *		leading portion of them that are integer values).
 *	- TCL_PARSE_SCAN_PREFIXES:	ignore the prefixes 0b and 0o that are
 *		not part of the [scan] command's vocabulary. Use only in
 *		combination with TCL_PARSE_INTEGER_ONLY.
 *	- TCL_PARSE_BINARY_ONLY:	parse only in the binary format, whether
 *		or not a prefix is present that would lead to binary parsing.
 *		Use only in combination with TCL_PARSE_INTEGER_ONLY.
 *	- TCL_PARSE_OCTAL_ONLY:		parse only in the octal format, whether
 *		or not a prefix is present that would lead to octal parsing.
 *		Use only in combination with TCL_PARSE_INTEGER_ONLY.
 *	- TCL_PARSE_HEXADECIMAL_ONLY:	parse only in the hexadecimal format,
 *		whether or not a prefix is present that would lead to
 *		hexadecimal parsing. Use only in combination with
 *		TCL_PARSE_INTEGER_ONLY.
 *	- TCL_PARSE_DECIMAL_ONLY:	parse only in the decimal format, no
 *		matter whether a 0 prefix would normally force a different
 *		base.
 *	- TCL_PARSE_NO_WHITESPACE:	reject any leading/trailing whitespace
 *
 *	The arguments interp and expected are inputs that control error
 *	message generation. If interp is NULL, no error message will be
 *	generated. If interp is non-NULL, then expected must also be non-NULL.
 *	When TCL_ERROR is returned, an error message will be left in the
 *	result of interp, and the expected argument will appear in the error
 *	message as the thing TclParseNumber expected, but failed to find in
 *	the string.
 *
 *	The arguments objPtr and endPtrPtr as well as the return code are the
 *	outputs.
 *
 *	When the parser cannot find any prefix of the string that matches a
 *	format it is looking for, TCL_ERROR is returned and an error message
 *	may be generated and returned as described above. The contents of
 *	objPtr will not be changed. If endPtrPtr is non-NULL, a pointer to the
 *	character in the string that terminated the scan will be written to
 *	*endPtrPtr.
 *
 *	When the parser determines that the entire string matches a format it
 *	is looking for, TCL_OK is returned, and if objPtr is non-NULL, then
 *	the internal rep and Tcl_ObjType of objPtr are set to the "canonical"
 *	numeric value that matches the scanned string. If endPtrPtr is not
 *	NULL, a pointer to the end of the string will be written to *endPtrPtr
 *	(that is, either bytes+numBytes or a pointer to a terminating NUL
 *	byte).
 *
 *	When the parser determines that a partial string matches a format it
 *	is looking for, the value of endPtrPtr determines what happens:
 *
 *	- If endPtrPtr is NULL, then TCL_ERROR is returned, with error message
 *		generation as above.
 *
 *	- If endPtrPtr is non-NULL, then TCL_OK is returned and objPtr
 *		internals are set as above. Also, a pointer to the first
 *		character following the parsed numeric string is written to
 *		*endPtrPtr.
 *
 *	In some cases where the string being scanned is the string rep of
 *	objPtr, this routine can leave objPtr in an inconsistent state where
 *	its string rep and its internal rep do not agree. In these cases the
 *	internal rep will be in agreement with only some substring of the
 *	string rep. This might happen if the caller passes in a non-NULL bytes
 *	value that points somewhere into the string rep. It might happen if
 *	the caller passes in a numBytes value that limits the scan to only a
 *	prefix of the string rep. Or it might happen if a non-NULL value of
 *	endPtrPtr permits a TCL_OK return from only a partial string match. It
 *	is the responsibility of the caller to detect and correct such
 *	inconsistencies when they can and do arise.
 *
 * Results:
 *	Returns a standard Tcl result.
 *
 * Side effects:
 *	The string representaton of objPtr may be generated.
 *
 *	The internal representation and Tcl_ObjType of objPtr may be changed.
 *	This may involve allocation and/or freeing of memory.
 *
 *----------------------------------------------------------------------
 */

int
TclParseNumber(
    Tcl_Interp *interp,		/* Used for error reporting. May be NULL. */
    Tcl_Obj *objPtr,		/* Object to receive the internal rep. */
    const char *expected,	/* Description of the type of number the
				 * caller expects to be able to parse
				 * ("integer", "boolean value", etc.). */
    const char *bytes,		/* Pointer to the start of the string to
				 * scan. */
    int numBytes,		/* Maximum number of bytes to scan, see
				 * above. */
    const char **endPtrPtr,	/* Place to store pointer to the character
				 * that terminated the scan. */
    int flags)			/* Flags governing the parse. */
{
    enum State {
	INITIAL, SIGNUM, ZERO, ZERO_X,
	ZERO_O, ZERO_B, BINARY,
	HEXADECIMAL, OCTAL, BAD_OCTAL, DECIMAL,
	LEADING_RADIX_POINT, FRACTION,
	EXPONENT_START, EXPONENT_SIGNUM, EXPONENT,
	sI, sIN, sINF, sINFI, sINFIN, sINFINI, sINFINIT, sINFINITY
#ifdef IEEE_FLOATING_POINT
	, sN, sNA, sNAN, sNANPAREN, sNANHEX, sNANFINISH
#endif
    } state = INITIAL;
    enum State acceptState = INITIAL;

    int signum = 0;		/* Sign of the number being parsed. */
    Tcl_WideUInt significandWide = 0;
				/* Significand of the number being parsed (if
				 * no overflow). */
    mp_int significandBig;	/* Significand of the number being parsed (if
				 * it overflows significandWide). */
    int significandOverflow = 0;/* Flag==1 iff significandBig is used. */
    Tcl_WideUInt octalSignificandWide = 0;
				/* Significand of an octal number; needed
				 * because we don't know whether a number with
				 * a leading zero is octal or decimal until
				 * we've scanned forward to a '.' or 'e'. */
    mp_int octalSignificandBig;	/* Significand of octal number once
				 * octalSignificandWide overflows. */
    int octalSignificandOverflow = 0;
				/* Flag==1 if octalSignificandBig is used. */
    int numSigDigs = 0;		/* Number of significant digits in the decimal
				 * significand. */
    int numTrailZeros = 0;	/* Number of trailing zeroes at the current
				 * point in the parse. */
    int numDigitsAfterDp = 0;	/* Number of digits scanned after the decimal
				 * point. */
    int exponentSignum = 0;	/* Signum of the exponent of a floating point
				 * number. */
    long exponent = 0;		/* Exponent of a floating point number. */
    const char *p;		/* Pointer to next character to scan. */
    size_t len;			/* Number of characters remaining after p. */
    const char *acceptPoint;	/* Pointer to position after last character in
				 * an acceptable number. */
    size_t acceptLen;		/* Number of characters following that
				 * point. */
    int status = TCL_OK;	/* Status to return to caller. */
    char d = 0;			/* Last hexadecimal digit scanned; initialized
				 * to avoid a compiler warning. */
    int shift = 0;		/* Amount to shift when accumulating binary */
    int explicitOctal = 0;

#define ALL_BITS	(~(Tcl_WideUInt)0)
#define MOST_BITS	(ALL_BITS >> 1)

    /*
     * Initialize bytes to start of the object's string rep if the caller
     * didn't pass anything else.
     */

    if (bytes == NULL) {
	bytes = TclGetString(objPtr);
    }

    p = bytes;
    len = numBytes;
    acceptPoint = p;
    acceptLen = len;
    while (1) {
	char c = len ? *p : '\0';
	switch (state) {

	case INITIAL:
	    /*
	     * Initial state. Acceptable characters are +, -, digits, period,
	     * I, N, and whitespace.
	     */

	    if (TclIsSpaceProc(c)) {
		if (flags & TCL_PARSE_NO_WHITESPACE) {
		    goto endgame;
		}
		break;
	    } else if (c == '+') {
		state = SIGNUM;
		break;
	    } else if (c == '-') {
		signum = 1;
		state = SIGNUM;
		break;
	    }
	    /* FALLTHROUGH */

	case SIGNUM:
	    /*
	     * Scanned a leading + or -. Acceptable characters are digits,
	     * period, I, and N.
	     */

	    if (c == '0') {
		if (flags & TCL_PARSE_DECIMAL_ONLY) {
		    state = DECIMAL;
		} else {
		    state = ZERO;
		}
		break;
	    } else if (flags & TCL_PARSE_HEXADECIMAL_ONLY) {
		goto zerox;
	    } else if (flags & TCL_PARSE_BINARY_ONLY) {
		goto zerob;
	    } else if (flags & TCL_PARSE_OCTAL_ONLY) {
		goto zeroo;
	    } else if (isdigit(UCHAR(c))) {
		significandWide = c - '0';
		numSigDigs = 1;
		state = DECIMAL;
		break;
	    } else if (flags & TCL_PARSE_INTEGER_ONLY) {
		goto endgame;
	    } else if (c == '.') {
		state = LEADING_RADIX_POINT;
		break;
	    } else if (c == 'I' || c == 'i') {
		state = sI;
		break;
#ifdef IEEE_FLOATING_POINT
	    } else if (c == 'N' || c == 'n') {
		state = sN;
		break;
#endif
	    }
	    goto endgame;

	case ZERO:
	    /*
	     * Scanned a leading zero (perhaps with a + or -). Acceptable
	     * inputs are digits, period, X, b, and E. If 8 or 9 is
	     * encountered, the number can't be octal. This state and the
	     * OCTAL state differ only in whether they recognize 'X' and 'b'.
	     */

	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (c == 'x' || c == 'X') {
		if (flags & (TCL_PARSE_OCTAL_ONLY|TCL_PARSE_BINARY_ONLY)) {
		    goto endgame;
		}
		state = ZERO_X;
		break;
	    }
	    if (flags & TCL_PARSE_HEXADECIMAL_ONLY) {
		goto zerox;
	    }
	    if (flags & TCL_PARSE_SCAN_PREFIXES) {
		goto zeroo;
	    }
	    if (c == 'b' || c == 'B') {
		if (flags & TCL_PARSE_OCTAL_ONLY) {
		    goto endgame;
		}
		state = ZERO_B;
		break;
	    }
	    if (flags & TCL_PARSE_BINARY_ONLY) {
		goto zerob;
	    }
	    if (c == 'o' || c == 'O') {
		explicitOctal = 1;
		state = ZERO_O;
		break;
	    }
#ifdef KILL_OCTAL
	    goto decimal;
#endif
	    /* FALLTHROUGH */

	case OCTAL:
	    /*
	     * Scanned an optional + or -, followed by a string of octal
	     * digits. Acceptable inputs are more digits, period, or E. If 8
	     * or 9 is encountered, commit to floating point.
	     */

	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    /* FALLTHROUGH */
	case ZERO_O:
	zeroo:
	    if (c == '0') {
		numTrailZeros++;
		state = OCTAL;
		break;
	    } else if (c >= '1' && c <= '7') {
		if (objPtr != NULL) {
		    shift = 3 * (numTrailZeros + 1);
		    significandOverflow = AccumulateDecimalDigit(
			    (unsigned)(c-'0'), numTrailZeros,
			    &significandWide, &significandBig,
			    significandOverflow);

		    if (!octalSignificandOverflow) {
			/*
			 * Shifting by more bits than are in the value being
			 * shifted is at least de facto nonportable. Check for
			 * too large shifts first.
			 */

			if ((octalSignificandWide != 0)
				&& (((size_t)shift >=
					CHAR_BIT*sizeof(Tcl_WideUInt))
				|| (octalSignificandWide >
					(~(Tcl_WideUInt)0 >> shift)))) {
			    octalSignificandOverflow = 1;
			    TclBNInitBignumFromWideUInt(&octalSignificandBig,
				    octalSignificandWide);
			}
		    }
		    if (!octalSignificandOverflow) {
			octalSignificandWide =
				(octalSignificandWide << shift) + (c - '0');
		    } else {
			mp_mul_2d(&octalSignificandBig, shift,
				&octalSignificandBig);
			mp_add_d(&octalSignificandBig, (mp_digit)(c - '0'),
				&octalSignificandBig);
		    }
		}
		if (numSigDigs != 0) {
		    numSigDigs += numTrailZeros+1;
		} else {
		    numSigDigs = 1;
		}
		numTrailZeros = 0;
		state = OCTAL;
		break;
	    }
	    /* FALLTHROUGH */

	case BAD_OCTAL:
	    if (explicitOctal) {
		/*
		 * No forgiveness for bad digits in explicitly octal numbers.
		 */

		goto endgame;
	    }
	    if (flags & TCL_PARSE_INTEGER_ONLY) {
		/*
		 * No seeking floating point when parsing only integer.
		 */

		goto endgame;
	    }
#ifndef KILL_OCTAL

	    /*
	     * Scanned a number with a leading zero that contains an 8, 9,
	     * radix point or E. This is an invalid octal number, but might
	     * still be floating point.
	     */

	    if (c == '0') {
		numTrailZeros++;
		state = BAD_OCTAL;
		break;
	    } else if (isdigit(UCHAR(c))) {
		if (objPtr != NULL) {
		    significandOverflow = AccumulateDecimalDigit(
			    (unsigned)(c-'0'), numTrailZeros,
			    &significandWide, &significandBig,
			    significandOverflow);
		}
		if (numSigDigs != 0) {
		    numSigDigs += (numTrailZeros + 1);
		} else {
		    numSigDigs = 1;
		}
		numTrailZeros = 0;
		state = BAD_OCTAL;
		break;
	    } else if (c == '.') {
		state = FRACTION;
		break;
	    } else if (c == 'E' || c == 'e') {
		state = EXPONENT_START;
		break;
	    }
#endif
	    goto endgame;

	    /*
	     * Scanned 0x. If state is HEXADECIMAL, scanned at least one
	     * character following the 0x. The only acceptable inputs are
	     * hexadecimal digits.
	     */

	case HEXADECIMAL:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    /* FALLTHROUGH */

	case ZERO_X:
	zerox:
	    if (c == '0') {
		numTrailZeros++;
		state = HEXADECIMAL;
		break;
	    } else if (isdigit(UCHAR(c))) {
		d = (c-'0');
	    } else if (c >= 'A' && c <= 'F') {
		d = (c-'A'+10);
	    } else if (c >= 'a' && c <= 'f') {
		d = (c-'a'+10);
	    } else {
		goto endgame;
	    }
	    if (objPtr != NULL) {
		shift = 4 * (numTrailZeros + 1);
		if (!significandOverflow) {
		    /*
		     * Shifting by more bits than are in the value being
		     * shifted is at least de facto nonportable. Check for too
		     * large shifts first.
		     */

		    if (significandWide != 0 &&
			    ((size_t)shift >= CHAR_BIT*sizeof(Tcl_WideUInt) ||
			    significandWide > (~(Tcl_WideUInt)0 >> shift))) {
			significandOverflow = 1;
			TclBNInitBignumFromWideUInt(&significandBig,
				significandWide);
		    }
		}
		if (!significandOverflow) {
		    significandWide = (significandWide << shift) + d;
		} else {
		    mp_mul_2d(&significandBig, shift, &significandBig);
		    mp_add_d(&significandBig, (mp_digit) d, &significandBig);
		}
	    }
	    numTrailZeros = 0;
	    state = HEXADECIMAL;
	    break;

	case BINARY:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	case ZERO_B:
	zerob:
	    if (c == '0') {
		numTrailZeros++;
		state = BINARY;
		break;
	    } else if (c != '1') {
		goto endgame;
	    }
	    if (objPtr != NULL) {
		shift = numTrailZeros + 1;
		if (!significandOverflow) {
		    /*
		     * Shifting by more bits than are in the value being
		     * shifted is at least de facto nonportable. Check for too
		     * large shifts first.
		     */

		    if (significandWide != 0 &&
			    ((size_t)shift >= CHAR_BIT*sizeof(Tcl_WideUInt) ||
			    significandWide > (~(Tcl_WideUInt)0 >> shift))) {
			significandOverflow = 1;
			TclBNInitBignumFromWideUInt(&significandBig,
				significandWide);
		    }
		}
		if (!significandOverflow) {
		    significandWide = (significandWide << shift) + 1;
		} else {
		    mp_mul_2d(&significandBig, shift, &significandBig);
		    mp_add_d(&significandBig, (mp_digit) 1, &significandBig);
		}
	    }
	    numTrailZeros = 0;
	    state = BINARY;
	    break;

	case DECIMAL:
	    /*
	     * Scanned an optional + or - followed by a string of decimal
	     * digits.
	     */

#ifdef KILL_OCTAL
	decimal:
#endif
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (c == '0') {
		numTrailZeros++;
		state = DECIMAL;
		break;
	    } else if (isdigit(UCHAR(c))) {
		if (objPtr != NULL) {
		    significandOverflow = AccumulateDecimalDigit(
			    (unsigned)(c - '0'), numTrailZeros,
			    &significandWide, &significandBig,
			    significandOverflow);
		}
		numSigDigs += numTrailZeros+1;
		numTrailZeros = 0;
		state = DECIMAL;
		break;
	    } else if (flags & TCL_PARSE_INTEGER_ONLY) {
		goto endgame;
	    } else if (c == '.') {
		state = FRACTION;
		break;
	    } else if (c == 'E' || c == 'e') {
		state = EXPONENT_START;
		break;
	    }
	    goto endgame;

	    /*
	     * Found a decimal point. If no digits have yet been scanned, E is
	     * not allowed; otherwise, it introduces the exponent. If at least
	     * one digit has been found, we have a possible complete number.
	     */

	case FRACTION:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (c == 'E' || c=='e') {
		state = EXPONENT_START;
		break;
	    }
	    /* FALLTHROUGH */

	case LEADING_RADIX_POINT:
	    if (c == '0') {
		numDigitsAfterDp++;
		numTrailZeros++;
		state = FRACTION;
		break;
	    } else if (isdigit(UCHAR(c))) {
		numDigitsAfterDp++;
		if (objPtr != NULL) {
		    significandOverflow = AccumulateDecimalDigit(
			    (unsigned)(c-'0'), numTrailZeros,
			    &significandWide, &significandBig,
			    significandOverflow);
		}
		if (numSigDigs != 0) {
		    numSigDigs += numTrailZeros+1;
		} else {
		    numSigDigs = 1;
		}
		numTrailZeros = 0;
		state = FRACTION;
		break;
	    }
	    goto endgame;

	case EXPONENT_START:
	    /*
	     * Scanned the E at the start of an exponent. Make sure a legal
	     * character follows before using the C library strtol routine,
	     * which allows whitespace.
	     */

	    if (c == '+') {
		state = EXPONENT_SIGNUM;
		break;
	    } else if (c == '-') {
		exponentSignum = 1;
		state = EXPONENT_SIGNUM;
		break;
	    }
	    /* FALLTHROUGH */

	case EXPONENT_SIGNUM:
	    /*
	     * Found the E at the start of the exponent, followed by a sign
	     * character.
	     */

	    if (isdigit(UCHAR(c))) {
		exponent = c - '0';
		state = EXPONENT;
		break;
	    }
	    goto endgame;

	case EXPONENT:
	    /*
	     * Found an exponent with at least one digit. Accumulate it,
	     * making sure to hard-pin it to LONG_MAX on overflow.
	     */

	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (isdigit(UCHAR(c))) {
		if (exponent < (LONG_MAX - 9) / 10) {
		    exponent = 10 * exponent + (c - '0');
		} else {
		    exponent = LONG_MAX;
		}
		state = EXPONENT;
		break;
	    }
	    goto endgame;

	    /*
	     * Parse out INFINITY by simply spelling it out. INF is accepted
	     * as an abbreviation; other prefices are not.
	     */

	case sI:
	    if (c == 'n' || c == 'N') {
		state = sIN;
		break;
	    }
	    goto endgame;
	case sIN:
	    if (c == 'f' || c == 'F') {
		state = sINF;
		break;
	    }
	    goto endgame;
	case sINF:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (c == 'i' || c == 'I') {
		state = sINFI;
		break;
	    }
	    goto endgame;
	case sINFI:
	    if (c == 'n' || c == 'N') {
		state = sINFIN;
		break;
	    }
	    goto endgame;
	case sINFIN:
	    if (c == 'i' || c == 'I') {
		state = sINFINI;
		break;
	    }
	    goto endgame;
	case sINFINI:
	    if (c == 't' || c == 'T') {
		state = sINFINIT;
		break;
	    }
	    goto endgame;
	case sINFINIT:
	    if (c == 'y' || c == 'Y') {
		state = sINFINITY;
		break;
	    }
	    goto endgame;

	    /*
	     * Parse NaN's.
	     */
#ifdef IEEE_FLOATING_POINT
	case sN:
	    if (c == 'a' || c == 'A') {
		state = sNA;
		break;
	    }
	    goto endgame;
	case sNA:
	    if (c == 'n' || c == 'N') {
		state = sNAN;
		break;
	    }
	    goto endgame;
	case sNAN:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    if (c == '(') {
		state = sNANPAREN;
		break;
	    }
	    goto endgame;

	    /*
	     * Parse NaN(hexdigits)
	     */
	case sNANHEX:
	    if (c == ')') {
		state = sNANFINISH;
		break;
	    }
	    /* FALLTHROUGH */
	case sNANPAREN:
	    if (TclIsSpaceProc(c)) {
		break;
	    }
	    if (numSigDigs < 13) {
		if (c >= '0' && c <= '9') {
		    d = c - '0';
		} else if (c >= 'a' && c <= 'f') {
		    d = 10 + c - 'a';
		} else if (c >= 'A' && c <= 'F') {
		    d = 10 + c - 'A';
		} else {
		    goto endgame;
		}
		numSigDigs++;
		significandWide = (significandWide << 4) + d;
		state = sNANHEX;
		break;
	    }
	    goto endgame;
	case sNANFINISH:
#endif

	case sINFINITY:
	    acceptState = state;
	    acceptPoint = p;
	    acceptLen = len;
	    goto endgame;
	}
	p++;
	len--;
    }

  endgame:
    if (acceptState == INITIAL) {
	/*
	 * No numeric string at all found.
	 */

	status = TCL_ERROR;
	if (endPtrPtr != NULL) {
	    *endPtrPtr = p;
	}
    } else {
	/*
	 * Back up to the last accepting state in the lexer.
	 */

	p = acceptPoint;
	len = acceptLen;
	if (!(flags & TCL_PARSE_NO_WHITESPACE)) {
	    /*
	     * Accept trailing whitespace.
	     */

	    while (len != 0 && TclIsSpaceProc(*p)) {
		p++;
		len--;
	    }
	}
	if (endPtrPtr == NULL) {
	    if ((len != 0) && ((numBytes > 0) || (*p != '\0'))) {
		status = TCL_ERROR;
	    }
	} else {
	    *endPtrPtr = p;
	}
    }

    /*
     * Generate and store the appropriate internal rep.
     */

    if (status == TCL_OK && objPtr != NULL) {
	TclFreeIntRep(objPtr);
	switch (acceptState) {
	case SIGNUM:
	case BAD_OCTAL:
	case ZERO_X:
	case ZERO_O:
	case ZERO_B:
	case LEADING_RADIX_POINT:
	case EXPONENT_START:
	case EXPONENT_SIGNUM:
	case sI:
	case sIN:
	case sINFI:
	case sINFIN:
	case sINFINI:
	case sINFINIT:
#ifdef IEEE_FLOATING_POINT
	case sN:
	case sNA:
	case sNANPAREN:
	case sNANHEX:
#endif
	    Tcl_Panic("TclParseNumber: bad acceptState %d parsing '%s'",
		    acceptState, bytes);
	case BINARY:
	    shift = numTrailZeros;
	    if (!significandOverflow && significandWide != 0 &&
		    ((size_t)shift >= CHAR_BIT*sizeof(Tcl_WideUInt) ||
		    significandWide > (MOST_BITS + signum) >> shift)) {
		significandOverflow = 1;
		TclBNInitBignumFromWideUInt(&significandBig, significandWide);
	    }
	    if (shift) {
		if (!significandOverflow) {
		    significandWide <<= shift;
		} else {
		    mp_mul_2d(&significandBig, shift, &significandBig);
		}
	    }
	    goto returnInteger;

	case HEXADECIMAL:
	    /*
	     * Returning a hex integer. Final scaling step.
	     */

	    shift = 4 * numTrailZeros;
	    if (!significandOverflow && significandWide !=0 &&
		    ((size_t)shift >= CHAR_BIT*sizeof(Tcl_WideUInt) ||
		    significandWide > (MOST_BITS + signum) >> shift)) {
		significandOverflow = 1;
		TclBNInitBignumFromWideUInt(&significandBig, significandWide);
	    }
	    if (shift) {
		if (!significandOverflow) {
		    significandWide <<= shift;
		} else {
		    mp_mul_2d(&significandBig, shift, &significandBig);
		}
	    }
	    goto returnInteger;

	case OCTAL:
	    /*
	     * Returning an octal integer. Final scaling step.
	     */

	    shift = 3 * numTrailZeros;
	    if (!octalSignificandOverflow && octalSignificandWide != 0 &&
		    ((size_t)shift >= CHAR_BIT*sizeof(Tcl_WideUInt) ||
		    octalSignificandWide > (MOST_BITS + signum) >> shift)) {
		octalSignificandOverflow = 1;
		TclBNInitBignumFromWideUInt(&octalSignificandBig,
			octalSignificandWide);
	    }
	    if (shift) {
		if (!octalSignificandOverflow) {
		    octalSignificandWide <<= shift;
		} else {
		    mp_mul_2d(&octalSignificandBig, shift,
			    &octalSignificandBig);
		}
	    }
	    if (!octalSignificandOverflow) {
		if (octalSignificandWide >
			(Tcl_WideUInt)(((~(unsigned long)0) >> 1) + signum)) {
#ifndef TCL_WIDE_INT_IS_LONG
		    if (octalSignificandWide <= (MOST_BITS + signum)) {
			objPtr->typePtr = &tclWideIntType;
			if (signum) {
			    objPtr->internalRep.wideValue =
				    - (Tcl_WideInt) octalSignificandWide;
			} else {
			    objPtr->internalRep.wideValue =
				    (Tcl_WideInt) octalSignificandWide;
			}
			break;
		    }
#endif
		    TclBNInitBignumFromWideUInt(&octalSignificandBig,
			    octalSignificandWide);
		    octalSignificandOverflow = 1;
		} else {
		    objPtr->typePtr = &tclIntType;
		    if (signum) {
			objPtr->internalRep.longValue =
				- (long) octalSignificandWide;
		    } else {
			objPtr->internalRep.longValue =
				(long) octalSignificandWide;
		    }
		}
	    }
	    if (octalSignificandOverflow) {
		if (signum) {
		    mp_neg(&octalSignificandBig, &octalSignificandBig);
		}
		TclSetBignumIntRep(objPtr, &octalSignificandBig);
	    }
	    break;

	case ZERO:
	case DECIMAL:
	    significandOverflow = AccumulateDecimalDigit(0, numTrailZeros-1,
		    &significandWide, &significandBig, significandOverflow);
	    if (!significandOverflow && (significandWide > MOST_BITS+signum)){
		significandOverflow = 1;
		TclBNInitBignumFromWideUInt(&significandBig, significandWide);
	    }
	returnInteger:
	    if (!significandOverflow) {
		if (significandWide >
			(Tcl_WideUInt)(((~(unsigned long)0) >> 1) + signum)) {
#ifndef TCL_WIDE_INT_IS_LONG
		    if (significandWide <= MOST_BITS+signum) {
			objPtr->typePtr = &tclWideIntType;
			if (signum) {
			    objPtr->internalRep.wideValue =
				    - (Tcl_WideInt) significandWide;
			} else {
			    objPtr->internalRep.wideValue =
				    (Tcl_WideInt) significandWide;
			}
			break;
		    }
#endif
		    TclBNInitBignumFromWideUInt(&significandBig,
			    significandWide);
		    significandOverflow = 1;
		} else {
		    objPtr->typePtr = &tclIntType;
		    if (signum) {
			objPtr->internalRep.longValue =
				- (long) significandWide;
		    } else {
			objPtr->internalRep.longValue =
				(long) significandWide;
		    }
		}
	    }
	    if (significandOverflow) {
		if (signum) {
		    mp_neg(&significandBig, &significandBig);
		}
		TclSetBignumIntRep(objPtr, &significandBig);
	    }
	    break;

	case FRACTION:
	case EXPONENT:

	    /*
	     * Here, we're parsing a floating-point number. 'significandWide'
	     * or 'significandBig' contains the exact significand, according
	     * to whether 'significandOverflow' is set. The desired floating
	     * point value is significand * 10**k, where
	     * k = numTrailZeros+exponent-numDigitsAfterDp.
	     */

	    objPtr->typePtr = &tclDoubleType;
	    if (exponentSignum) {
		exponent = -exponent;
	    }
	    if (!significandOverflow) {
		objPtr->internalRep.doubleValue = MakeLowPrecisionDouble(
			signum, significandWide, numSigDigs,
			numTrailZeros + exponent - numDigitsAfterDp);
	    } else {
		objPtr->internalRep.doubleValue = MakeHighPrecisionDouble(
			signum, &significandBig, numSigDigs,
			numTrailZeros + exponent - numDigitsAfterDp);
	    }
	    break;

	case sINF:
	case sINFINITY:
	    if (signum) {
		objPtr->internalRep.doubleValue = -HUGE_VAL;
	    } else {
		objPtr->internalRep.doubleValue = HUGE_VAL;
	    }
	    objPtr->typePtr = &tclDoubleType;
	    break;

#ifdef IEEE_FLOATING_POINT
	case sNAN:
	case sNANFINISH:
	    objPtr->internalRep.doubleValue = MakeNaN(signum,significandWide);
	    objPtr->typePtr = &tclDoubleType;
	    break;
#endif
	case INITIAL:
	    /* This case only to silence compiler warning. */
	    Tcl_Panic("TclParseNumber: state INITIAL can't happen here");
	}
    }

    /*
     * Format an error message when an invalid number is encountered.
     */

    if (status != TCL_OK) {
	if (interp != NULL) {
	    Tcl_Obj *msg = Tcl_ObjPrintf("expected %s but got \"",
		    expected);

	    Tcl_AppendLimitedToObj(msg, bytes, numBytes, 50, "");
	    Tcl_AppendToObj(msg, "\"", -1);
	    if (state == BAD_OCTAL) {
		Tcl_AppendToObj(msg, " (looks like invalid octal number)", -1);
	    }
	    Tcl_SetObjResult(interp, msg);
	    Tcl_SetErrorCode(interp, "TCL", "VALUE", "NUMBER", NULL);
	}
    }

    /*
     * Free memory.
     */

    if (octalSignificandOverflow) {
	mp_clear(&octalSignificandBig);
    }
    if (significandOverflow) {
	mp_clear(&significandBig);
    }
    return status;
}

/*
 *----------------------------------------------------------------------
 *
 * AccumulateDecimalDigit --
 *
 *	Consume a decimal digit in a number being scanned.
 *
 * Results:
 *	Returns 1 if the number has overflowed to a bignum, 0 if it still fits
 *	in a wide integer.
 *
 * Side effects:
 *	Updates either the wide or bignum representation.
 *
 *----------------------------------------------------------------------
 */

static int
AccumulateDecimalDigit(
    unsigned digit,		/* Digit being scanned. */
    int numZeros,		/* Count of zero digits preceding the digit
				 * being scanned. */
    Tcl_WideUInt *wideRepPtr,	/* Representation of the partial number as a
				 * wide integer. */
    mp_int *bignumRepPtr,	/* Representation of the partial number as a
				 * bignum. */
    int bignumFlag)		/* Flag == 1 if the number overflowed previous
				 * to this digit. */
{
    int i, n;
    Tcl_WideUInt w;

    /*
     * Try wide multiplication first.
     */

    if (!bignumFlag) {
	w = *wideRepPtr;
	if (w == 0) {
	    /*
	     * There's no need to multiply if the multiplicand is zero.
	     */

	    *wideRepPtr = digit;
	    return 0;
	} else if (numZeros >= maxpow10_wide
		|| w > ((~(Tcl_WideUInt)0)-digit)/pow10_wide[numZeros+1]) {
	    /*
	     * Wide multiplication will overflow.  Expand the number to a
	     * bignum and fall through into the bignum case.
	     */

	    TclBNInitBignumFromWideUInt(bignumRepPtr, w);
	} else {
	    /*
	     * Wide multiplication.
	     */

	    *wideRepPtr = w * pow10_wide[numZeros+1] + digit;
	    return 0;
	}
    }

    /*
     * Bignum multiplication.
     */

    if (numZeros < log10_DIGIT_MAX) {
	/*
	 * Up to about 8 zeros - single digit multiplication.
	 */

	mp_mul_d(bignumRepPtr, (mp_digit) pow10_wide[numZeros+1],
		bignumRepPtr);
	mp_add_d(bignumRepPtr, (mp_digit) digit, bignumRepPtr);
    } else {
	/*
	 * More than single digit multiplication. Multiply by the appropriate
	 * small powers of 5, and then shift. Large strings of zeroes are
	 * eaten 256 at a time; this is less efficient than it could be, but
	 * seems implausible. We presume that DIGIT_BIT is at least 27. The
	 * first multiplication, by up to 10**7, is done with a one-DIGIT
	 * multiply (this presumes that DIGIT_BIT >= 24).
	 */

	n = numZeros + 1;
	mp_mul_d(bignumRepPtr, (mp_digit) pow10_wide[n&0x7], bignumRepPtr);
	for (i=3; i<=7; ++i) {
	    if (n & (1 << i)) {
		mp_mul(bignumRepPtr, pow5+i, bignumRepPtr);
	    }
	}
	while (n >= 256) {
	    mp_mul(bignumRepPtr, pow5+8, bignumRepPtr);
	    n -= 256;
	}
	mp_mul_2d(bignumRepPtr, (int)(numZeros+1)&~0x7, bignumRepPtr);
	mp_add_d(bignumRepPtr, (mp_digit) digit, bignumRepPtr);
    }

    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeLowPrecisionDouble --
 *
 *	Makes the double precision number, signum*significand*10**exponent.
 *
 * Results:
 *	Returns the constructed number.
 *
 *	Common cases, where there are few enough digits that the number can be
 *	represented with at most roundoff, are handled specially here. If the
 *	number requires more than one rounded operation to compute, the code
 *	promotes the significand to a bignum and calls MakeHighPrecisionDouble
 *	to do it instead.
 *
 *----------------------------------------------------------------------
 */

static double
MakeLowPrecisionDouble(
    int signum,			/* 1 if the number is negative, 0 otherwise */
    Tcl_WideUInt significand,	/* Significand of the number. */
    int numSigDigs,		/* Number of digits in the significand. */
    int exponent)		/* Power of ten. */
{
    double retval;		/* Value of the number. */
    mp_int significandBig;	/* Significand expressed as a bignum. */

    /*
     * With gcc on x86, the floating point rounding mode is double-extended.
     * This causes the result of double-precision calculations to be rounded
     * twice: once to the precision of double-extended and then again to the
     * precision of double. Double-rounding introduces gratuitous errors of 1
     * ulp, so we need to change rounding mode to 53-bits.
     */

    TCL_IEEE_DOUBLE_ROUNDING;

    /*
     * Test for the easy cases.
     */

    if (numSigDigs <= QUICK_MAX) {
	if (exponent >= 0) {
	    if (exponent <= mmaxpow) {
		/*
		 * The significand is an exact integer, and so is
		 * 10**exponent. The product will be correct to within 1/2 ulp
		 * without special handling.
		 */

		retval = (double)
			((Tcl_WideInt)significand * pow10vals[exponent]);
		goto returnValue;
	    } else {
		int diff = QUICK_MAX - numSigDigs;

		if (exponent-diff <= mmaxpow) {
		    /*
		     * 10**exponent is not an exact integer, but
		     * 10**(exponent-diff) is exact, and so is
		     * significand*10**diff, so we can still compute the value
		     * with only one roundoff.
		     */

		    volatile double factor = (double)
			    ((Tcl_WideInt)significand * pow10vals[diff]);
		    retval = factor * pow10vals[exponent-diff];
		    goto returnValue;
		}
	    }
	} else {
	    if (exponent >= -mmaxpow) {
		/*
		 * 10**-exponent is an exact integer, and so is the
		 * significand. Compute the result by one division, again with
		 * only one rounding.
		 */

		retval = (double)
			((Tcl_WideInt)significand / pow10vals[-exponent]);
		goto returnValue;
	    }
	}
    }

    /*
     * All the easy cases have failed. Promote ths significand to bignum and
     * call MakeHighPrecisionDouble to do it the hard way.
     */

    TclBNInitBignumFromWideUInt(&significandBig, significand);
    retval = MakeHighPrecisionDouble(0, &significandBig, numSigDigs,
	    exponent);
    mp_clear(&significandBig);

    /*
     * Come here to return the computed value.
     */

  returnValue:
    if (signum) {
	retval = -retval;
    }

    /*
     * On gcc on x86, restore the floating point mode word.
     */

    TCL_DEFAULT_DOUBLE_ROUNDING;

    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeHighPrecisionDouble --
 *
 *	Makes the double precision number, signum*significand*10**exponent.
 *
 * Results:
 *	Returns the constructed number.
 *
 *	MakeHighPrecisionDouble is used when arbitrary-precision arithmetic is
 *	needed to ensure correct rounding. It begins by calculating a
 *	low-precision approximation to the desired number, and then refines
 *	the answer in high precision.
 *
 *----------------------------------------------------------------------
 */

static double
MakeHighPrecisionDouble(
    int signum,			/* 1=negative, 0=nonnegative. */
    mp_int *significand,	/* Exact significand of the number. */
    int numSigDigs,		/* Number of significant digits. */
    int exponent)		/* Power of 10 by which to multiply. */
{
    double retval;
    int machexp;		/* Machine exponent of a power of 10. */

    /*
     * With gcc on x86, the floating point rounding mode is double-extended.
     * This causes the result of double-precision calculations to be rounded
     * twice: once to the precision of double-extended and then again to the
     * precision of double. Double-rounding introduces gratuitous errors of 1
     * ulp, so we need to change rounding mode to 53-bits.
     */

    TCL_IEEE_DOUBLE_ROUNDING;

    /*
     * Quick checks for over/underflow.
     */

    if (numSigDigs+exponent-1 > maxDigits) {
	retval = HUGE_VAL;
	goto returnValue;
    }
    if (numSigDigs+exponent-1 < minDigits) {
	retval = 0;
	goto returnValue;
    }

    /*
     * Develop a first approximation to the significand. It is tempting simply
     * to force bignum to double, but that will overflow on input numbers like
     * 1.[string repeat 0 1000]1; while this is a not terribly likely
     * scenario, we still have to deal with it. Use fraction and exponent
     * instead. Once we have the significand, multiply by 10**exponent. Test
     * for overflow. Convert back to a double, and test for underflow.
     */

    retval = BignumToBiasedFrExp(significand, &machexp);
    retval = Pow10TimesFrExp(exponent, retval, &machexp);
    if (machexp > DBL_MAX_EXP*log2FLT_RADIX) {
	retval = HUGE_VAL;
	goto returnValue;
    }
    retval = SafeLdExp(retval, machexp);
	if (tiny == 0.0) {
	    tiny = SafeLdExp(1.0, DBL_MIN_EXP * log2FLT_RADIX - mantBits);
	}
    if (retval < tiny) {
	retval = tiny;
    }

    /*
     * Refine the result twice. (The second refinement should be necessary
     * only if the best approximation is a power of 2 minus 1/2 ulp).
     */

    retval = RefineApproximation(retval, significand, exponent);
    retval = RefineApproximation(retval, significand, exponent);

    /*
     * Come here to return the computed value.
     */

  returnValue:
    if (signum) {
	retval = -retval;
    }

    /*
     * On gcc on x86, restore the floating point mode word.
     */

    TCL_DEFAULT_DOUBLE_ROUNDING;

    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * MakeNaN --
 *
 *	Makes a "Not a Number" given a set of bits to put in the tag bits
 *
 *	Note that a signalling NaN is never returned.
 *
 *----------------------------------------------------------------------
 */

#ifdef IEEE_FLOATING_POINT
static double
MakeNaN(
    int signum,			/* Sign bit (1=negative, 0=nonnegative. */
    Tcl_WideUInt tags)		/* Tag bits to put in the NaN. */
{
    union {
	Tcl_WideUInt iv;
	double dv;
    } theNaN;

    theNaN.iv = tags;
    theNaN.iv &= (((Tcl_WideUInt) 1) << 51) - 1;
    if (signum) {
	theNaN.iv |= ((Tcl_WideUInt) (0x8000 | NAN_START)) << 48;
    } else {
	theNaN.iv |= ((Tcl_WideUInt) NAN_START) << 48;
    }
    if (n770_fp) {
	theNaN.iv = Nokia770Twiddle(theNaN.iv);
    }
    return theNaN.dv;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * RefineApproximation --
 *
 *	Given a poor approximation to a floating point number, returns a
 *	better one. (The better approximation is correct to within 1 ulp, and
 *	is entirely correct if the poor approximation is correct to 1 ulp.)
 *
 * Results:
 *	Returns the improved result.
 *
 *----------------------------------------------------------------------
 */

static double
RefineApproximation(
    double approxResult,	/* Approximate result of conversion. */
    mp_int *exactSignificand,	/* Integer significand. */
    int exponent)		/* Power of 10 to multiply by significand. */
{
    int M2, M5;			/* Powers of 2 and of 5 needed to put the
				 * decimal and binary numbers over a common
				 * denominator. */
    double significand;		/* Sigificand of the binary number. */
    int binExponent;		/* Exponent of the binary number. */
    int msb;			/* Most significant bit position of an
				 * intermediate result. */
    int nDigits;		/* Number of mp_digit's in an intermediate
				 * result. */
    mp_int twoMv;		/* Approx binary value expressed as an exact
				 * integer scaled by the multiplier 2M. */
    mp_int twoMd;		/* Exact decimal value expressed as an exact
				 * integer scaled by the multiplier 2M. */
    int scale;			/* Scale factor for M. */
    int multiplier;		/* Power of two to scale M. */
    double num, den;		/* Numerator and denominator of the correction
				 * term. */
    double quot;		/* Correction term. */
    double minincr;		/* Lower bound on the absolute value of the
				 * correction term. */
    int roundToEven = 0;	/* Flag == TRUE if we need to invoke
				 * "round to even" functionality */
    double rteSignificand;	/* Significand of the round-to-even result */
    int rteExponent;		/* Exponent of the round-to-even result */
    Tcl_WideInt rteSigWide;	/* Wide integer version of the significand
				 * for testing evenness */
    int i;

    /*
     * The first approximation is always low. If we find that it's HUGE_VAL,
     * we're done.
     */

    if (approxResult == HUGE_VAL) {
	return approxResult;
    }

    /*
     * Find a common denominator for the decimal and binary fractions. The
     * common denominator will be 2**M2 + 5**M5.
     */

    significand = frexp(approxResult, &binExponent);
    i = mantBits - binExponent;
    if (i < 0) {
	M2 = 0;
    } else {
	M2 = i;
    }
    if (exponent > 0) {
	M5 = 0;
    } else {
	M5 = -exponent;
	if (M5 - 1 > M2) {
	    M2 = M5 - 1;
	}
    }

    /*
     * The floating point number is significand*2**binExponent. Compute the
     * large integer significand*2**(binExponent+M2+1). The 2**-1 bit of the
     * significand (the most significant) corresponds to the
     * 2**(binExponent+M2 + 1) bit of 2*M2*v. Allocate enough digits to hold
     * that quantity, then convert the significand to a large integer, scaled
     * appropriately. Then multiply by the appropriate power of 5.
     */

    msb = binExponent + M2;	/* 1008 */
    nDigits = msb / DIGIT_BIT + 1;
    mp_init_size(&twoMv, nDigits);
    i = (msb % DIGIT_BIT + 1);
    twoMv.used = nDigits;
    significand *= SafeLdExp(1.0, i);
    while (--nDigits >= 0) {
	twoMv.dp[nDigits] = (mp_digit) significand;
	significand -= (mp_digit) significand;
	significand = SafeLdExp(significand, DIGIT_BIT);
    }
    for (i = 0; i <= 8; ++i) {
	if (M5 & (1 << i)) {
	    mp_mul(&twoMv, pow5+i, &twoMv);
	}
    }

    /*
     * Collect the decimal significand as a high precision integer. The least
     * significant bit corresponds to bit M2+exponent+1 so it will need to be
     * shifted left by that many bits after being multiplied by
     * 5**(M5+exponent).
     */

    mp_init_copy(&twoMd, exactSignificand);
    for (i=0; i<=8; ++i) {
	if ((M5 + exponent) & (1 << i)) {
	    mp_mul(&twoMd, pow5+i, &twoMd);
	}
    }
    mp_mul_2d(&twoMd, M2+exponent+1, &twoMd);
    mp_sub(&twoMd, &twoMv, &twoMd);

    /*
     * The result, 2Mv-2Md, needs to be divided by 2M to yield a correction
     * term. Because 2M may well overflow a double, we need to scale the
     * denominator by a factor of 2**binExponent-mantBits.
     */

    scale = binExponent - mantBits - 1;

    mp_set(&twoMv, 1);
    for (i=0; i<=8; ++i) {
	if (M5 & (1 << i)) {
	    mp_mul(&twoMv, pow5+i, &twoMv);
	}
    }
    multiplier = M2 + scale + 1;
    if (multiplier > 0) {
	mp_mul_2d(&twoMv, multiplier, &twoMv);
    } else if (multiplier < 0) {
	mp_div_2d(&twoMv, -multiplier, &twoMv, NULL);
    }

    switch (mp_cmp_mag(&twoMd, &twoMv)) {
    case MP_LT:
	/*
	 * If the result is less than unity, the error is less than 1/2 unit in
	 * the last place, so there's no correction to make.
	 */
	mp_clear(&twoMd);
	mp_clear(&twoMv);
	return approxResult;
    case MP_EQ:
	/*
	 * If the result is exactly unity, we need to round to even.
	 */
	roundToEven = 1;
	break;
    case MP_GT:
	break;
    }

    if (roundToEven) {
	rteSignificand = frexp(approxResult, &rteExponent);
	rteSigWide = (Tcl_WideInt) ldexp(rteSignificand, FP_PRECISION);
	if ((rteSigWide & 1) == 0) {
	    mp_clear(&twoMd);
	    mp_clear(&twoMv);
	    return approxResult;
	}
    }

    /*
     * Convert the numerator and denominator of the corrector term accurately
     * to floating point numbers.
     */

    num = TclBignumToDouble(&twoMd);
    den = TclBignumToDouble(&twoMv);

    quot = SafeLdExp(num/den, scale);
    minincr = SafeLdExp(1.0, binExponent-mantBits);

    if (quot<0. && quot>-minincr) {
	quot = -minincr;
    } else if (quot>0. && quot<minincr) {
	quot = minincr;
    }

    mp_clear(&twoMd);
    mp_clear(&twoMv);

    return approxResult + quot;
}

/*
 *----------------------------------------------------------------------
 *
 * MultPow5 --
 *
 *	Multiply a bignum by a power of 5.
 *
 * Side effects:
 *	Stores base*5**n in result.
 *
 *----------------------------------------------------------------------
 */

static inline void
MulPow5(
    mp_int *base, 		/* Number to multiply. */
    unsigned n,			/* Power of 5 to multiply by. */
    mp_int *result)		/* Place to store the result. */
{
    mp_int *p = base;
    int n13 = n / 13;
    int r = n % 13;

    if (r != 0) {
	mp_mul_d(p, dpow5[r], result);
	p = result;
    }
    r = 0;
    while (n13 != 0) {
	if (n13 & 1) {
	    mp_mul(p, pow5_13+r, result);
	    p = result;
	}
	n13 >>= 1;
	++r;
    }
    if (p != result) {
	mp_copy(p, result);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * NormalizeRightward --
 *
 *	Shifts a number rightward until it is odd (that is, until the least
 *	significant bit is nonzero.
 *
 * Results:
 *	Returns the number of bit positions by which the number was shifted.
 *
 * Side effects:
 *	Shifts the number in place; *wPtr is replaced by the shifted number.
 *
 *----------------------------------------------------------------------
 */

static inline int
NormalizeRightward(
    Tcl_WideUInt *wPtr)		/* INOUT: Number to shift. */
{
    int rv = 0;
    Tcl_WideUInt w = *wPtr;

    if (!(w & (Tcl_WideUInt) 0xffffffff)) {
	w >>= 32; rv += 32;
    }
    if (!(w & (Tcl_WideUInt) 0xffff)) {
	w >>= 16; rv += 16;
    }
    if (!(w & (Tcl_WideUInt) 0xff)) {
	w >>= 8; rv += 8;
    }
    if (!(w & (Tcl_WideUInt) 0xf)) {
	w >>= 4; rv += 4;
    }
    if (!(w & 0x3)) {
	w >>= 2; rv += 2;
    }
    if (!(w & 0x1)) {
	w >>= 1; ++rv;
    }
    *wPtr = w;
    return rv;
}

/*
 *----------------------------------------------------------------------
 *
 * RequiredPrecision --
 *
 *	Determines the number of bits needed to hold an intger.
 *
 * Results:
 *	Returns the position of the most significant bit (0 - 63).  Returns 0
 *	if the number is zero.
 *
 *----------------------------------------------------------------------
 */

static int
RequiredPrecision(
    Tcl_WideUInt w)		/* Number to interrogate. */
{
    int rv;
    unsigned long wi;

    if (w & ((Tcl_WideUInt) 0xffffffff << 32)) {
	wi = (unsigned long) (w >> 32); rv = 32;
    } else {
	wi = (unsigned long) w; rv = 0;
    }
    if (wi & 0xffff0000) {
	wi >>= 16; rv += 16;
    }
    if (wi & 0xff00) {
	wi >>= 8; rv += 8;
    }
    if (wi & 0xf0) {
	wi >>= 4; rv += 4;
    }
    if (wi & 0xc) {
	wi >>= 2; rv += 2;
    }
    if (wi & 0x2) {
	wi >>= 1; ++rv;
    }
    if (wi & 0x1) {
	++rv;
    }
    return rv;
}

/*
 *----------------------------------------------------------------------
 *
 * DoubleToExpAndSig --
 *
 *	Separates a 'double' into exponent and significand.
 *
 * Side effects:
 *	Stores the significand in '*significand' and the exponent in '*expon'
 *	so that dv == significand * 2.0**expon, and significand is odd.  Also
 *	stores the position of the leftmost 1-bit in 'significand' in 'bits'.
 *
 *----------------------------------------------------------------------
 */

static inline void
DoubleToExpAndSig(
    double dv,			/* Number to convert. */
    Tcl_WideUInt *significand,	/* OUTPUT: Significand of the number. */
    int *expon,			/* OUTPUT: Exponent to multiply the number
				 * by. */
    int *bits)			/* OUTPUT: Number of significant bits. */
{
    Double d;			/* Number being converted. */
    Tcl_WideUInt z;		/* Significand under construction. */
    int de;			/* Exponent of the number. */
    int k;			/* Bit count. */

    d.d = dv;

    /*
     * Extract exponent and significand.
     */

    de = (d.w.word0 & EXP_MASK) >> EXP_SHIFT;
    z = d.q & SIG_MASK;
    if (de != 0) {
	z |= HIDDEN_BIT;
	k = NormalizeRightward(&z);
	*bits = FP_PRECISION - k;
	*expon = k + (de - EXPONENT_BIAS) - (FP_PRECISION-1);
    } else {
	k = NormalizeRightward(&z);
	*expon = k + (de - EXPONENT_BIAS) - (FP_PRECISION-1) + 1;
	*bits = RequiredPrecision(z);
    }
    *significand = z;
}

/*
 *----------------------------------------------------------------------
 *
 * TakeAbsoluteValue --
 *
 *	Takes the absolute value of a 'double' including 0, Inf and NaN
 *
 * Side effects:
 *	The 'double' in *d is replaced with its absolute value. The signum is
 *	stored in 'sign': 1 for negative, 0 for nonnegative.
 *
 *----------------------------------------------------------------------
 */

static inline void
TakeAbsoluteValue(
    Double *d,			/* Number to replace with absolute value. */
    int *sign)			/* Place to put the signum. */
{
    if (d->w.word0 & SIGN_BIT) {
	*sign = 1;
	d->w.word0 &= ~SIGN_BIT;
    } else {
	*sign = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FormatInfAndNaN --
 *
 *	Bailout for formatting infinities and Not-A-Number.
 *
 * Results:
 *	Returns one of the strings 'Infinity' and 'NaN'.  The string returned
 *	must be freed by the caller using 'ckfree'.
 *
 * Side effects:
 *	Stores 9999 in *decpt, and sets '*endPtr' to designate the terminating
 *	NUL byte of the string if 'endPtr' is not NULL.
 *
 *----------------------------------------------------------------------
 */

static inline char *
FormatInfAndNaN(
    Double *d,			/* Exceptional number to format. */
    int *decpt,			/* Decimal point to set to a bogus value. */
    char **endPtr)		/* Pointer to the end of the formatted data */
{
    char *retval;

    *decpt = 9999;
    if (!(d->w.word1) && !(d->w.word0 & HI_ORDER_SIG_MASK)) {
	retval = ckalloc(9);
	strcpy(retval, "Infinity");
	if (endPtr) {
	    *endPtr = retval + 8;
	}
    } else {
	retval = ckalloc(4);
	strcpy(retval, "NaN");
	if (endPtr) {
	    *endPtr = retval + 3;
	}
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatZero --
 *
 *	Bailout to format a zero floating-point number.
 *
 * Results:
 *	Returns the constant string "0"
 *
 * Side effects:
 *	Stores 1 in '*decpt' and puts a pointer to the NUL byte terminating
 *	the string in '*endPtr' if 'endPtr' is not NULL.
 *
 *----------------------------------------------------------------------
 */

static inline char *
FormatZero(
    int *decpt,			/* Location of the decimal point. */
    char **endPtr)		/* Pointer to the end of the formatted data */
{
    char *retval = ckalloc(2);

    strcpy(retval, "0");
    if (endPtr) {
	*endPtr = retval+1;
    }
    *decpt = 0;
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * ApproximateLog10 --
 *
 *	Computes a two-term Taylor series approximation to the common log of a
 *	number, and computes the number's binary log.
 *
 * Results:
 *	Return an approximation to floor(log10(bw*2**be)) that is either exact
 *	or 1 too high.
 *
 *----------------------------------------------------------------------
 */

static inline int
ApproximateLog10(
    Tcl_WideUInt bw,		/* Integer significand of the number. */
    int be,			/* Power of two to scale bw. */
    int bbits)			/* Number of bits of precision in bw. */
{
    int i;			/* Log base 2 of the number. */
    int k;			/* Floor(Log base 10 of the number) */
    double ds;			/* Mantissa of the number. */
    Double d2;

    /*
     * Compute i and d2 such that d = d2*2**i, and 1 < d2 < 2.
     * Compute an approximation to log10(d),
     *   log10(d) ~ log10(2) * i + log10(1.5)
     *            + (significand-1.5)/(1.5 * log(10))
     */

    d2.q = bw << (FP_PRECISION - bbits) & SIG_MASK;
    d2.w.word0 |= (EXPONENT_BIAS) << EXP_SHIFT;
    i = be + bbits - 1;
    ds = (d2.d - 1.5) * TWO_OVER_3LOG10
	    + LOG10_3HALVES_PLUS_FUDGE + LOG10_2 * i;
    k = (int) ds;
    if (k > ds) {
	--k;
    }
    return k;
}

/*
 *----------------------------------------------------------------------
 *
 * BetterLog10 --
 *
 *	Improves the result of ApproximateLog10 for numbers in the range
 *	1 .. 10**(TEN_PMAX)-1
 *
 * Side effects:
 *	Sets k_check to 0 if the new result is known to be exact, and to 1 if
 *	it may still be one too high.
 *
 * Results:
 *	Returns the improved approximation to log10(d).
 *
 *----------------------------------------------------------------------
 */

static inline int
BetterLog10(
    double d,			/* Original number to format. */
    int k,			/* Characteristic(Log base 10) of the
				 * number. */
    int *k_check)		/* Flag == 1 if k is inexact. */
{
    /*
     * Performance hack. If k is in the range 0..TEN_PMAX, then we can use a
     * powers-of-ten table to check it.
     */

    if (k >= 0 && k <= TEN_PMAX) {
	if (d < tens[k]) {
	    k--;
	}
	*k_check = 0;
    } else {
	*k_check = 1;
    }
    return k;
}

/*
 *----------------------------------------------------------------------
 *
 * ComputeScale --
 *
 *	Prepares to format a floating-point number as decimal.
 *
 * Parameters:
 *	floor(log10*x) is k (or possibly k-1).  floor(log2(x) is i.  The
 *	significand of x requires bbits bits to represent.
 *
 * Results:
 *	Determines integers b2, b5, s2, s5 so that sig*2**b2*5**b5/2**s2*2**s5
 *	exactly represents the value of the x/10**k. This value will lie in
 *	the range [1 .. 10), and allows for computing successive digits by
 *	multiplying sig%10 by 10.
 *
 *----------------------------------------------------------------------
 */

static inline void
ComputeScale(
    int be,			/* Exponent part of number: d = bw * 2**be. */
    int k,			/* Characteristic of log10(number). */
    int *b2,			/* OUTPUT: Power of 2 in the numerator. */
    int *b5,			/* OUTPUT: Power of 5 in the numerator. */
    int *s2,			/* OUTPUT: Power of 2 in the denominator. */
    int *s5)			/* OUTPUT: Power of 5 in the denominator. */
{
    /*
     * Scale numerator and denominator powers of 2 so that the input binary
     * number is the ratio of integers.
     */

    if (be <= 0) {
	*b2 = 0;
	*s2 = -be;
    } else {
	*b2 = be;
	*s2 = 0;
    }

    /*
     * Scale numerator and denominator so that the output decimal number is
     * the ratio of integers.
     */

    if (k >= 0) {
	*b5 = 0;
	*s5 = k;
	*s2 += k;
    } else {
	*b2 -= k;
	*b5 = -k;
	*s5 = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetPrecisionLimits --
 *
 *	Determines how many digits of significance should be computed (and,
 *	hence, how much memory need be allocated) for formatting a floating
 *	point number.
 *
 * Given that 'k' is floor(log10(x)):
 * if 'shortest' format is used, there will be at most 18 digits in the
 * result.
 * if 'F' format is used, there will be at most 'ndigits' + k + 1 digits
 * if 'E' format is used, there will be exactly 'ndigits' digits.
 *
 * Side effects:
 *	Adjusts '*ndigitsPtr' to have a valid value. Stores the maximum memory
 *	allocation needed in *iPtr.  Sets '*iLimPtr' to the limiting number of
 *	digits to convert if k has been guessed correctly, and '*iLim1Ptr' to
 *	the limiting number of digits to convert if k has been guessed to be
 *	one too high.
 *
 *----------------------------------------------------------------------
 */

static inline void
SetPrecisionLimits(
    int convType,		/* Type of conversion: TCL_DD_SHORTEST,
				 * TCL_DD_STEELE0, TCL_DD_E_FMT,
				 * TCL_DD_F_FMT. */
    int k,			/* Floor(log10(number to convert)) */
    int *ndigitsPtr,		/* IN/OUT: Number of digits requested (will be
				 *         adjusted if needed). */
    int *iPtr,			/* OUT: Maximum number of digits to return. */
    int *iLimPtr,		/* OUT: Number of digits of significance if
				 *      the bignum method is used.*/
    int *iLim1Ptr)		/* OUT: Number of digits of significance if
				 *      the quick method is used. */
{
    switch (convType) {
    case TCL_DD_SHORTEST0:
    case TCL_DD_STEELE0:
	*iLimPtr = *iLim1Ptr = -1;
	*iPtr = 18;
	*ndigitsPtr = 0;
	break;
    case TCL_DD_E_FORMAT:
	if (*ndigitsPtr <= 0) {
	    *ndigitsPtr = 1;
	}
	*iLimPtr = *iLim1Ptr = *iPtr = *ndigitsPtr;
	break;
    case TCL_DD_F_FORMAT:
	*iPtr = *ndigitsPtr + k + 1;
	*iLimPtr = *iPtr;
	*iLim1Ptr = *iPtr - 1;
	if (*iPtr <= 0) {
	    *iPtr = 1;
	}
	break;
    default:
	*iPtr = -1;
	*iLimPtr = -1;
	*iLim1Ptr = -1;
	Tcl_Panic("impossible conversion type in TclDoubleDigits");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * BumpUp --
 *
 *	Increases a string of digits ending in a series of nines to designate
 *	the next higher number.  xxxxb9999... -> xxxx(b+1)0000...
 *
 * Results:
 *	Returns a pointer to the end of the adjusted string.
 *
 * Side effects:
 *	In the case that the string consists solely of '999999', sets it to
 *	"1" and moves the decimal point (*kPtr) one place to the right.
 *
 *----------------------------------------------------------------------
 */

static inline char *
BumpUp(
    char *s,		    	/* Cursor pointing one past the end of the
				 * string. */
    char *retval,		/* Start of the string of digits. */
    int *kPtr)			/* Position of the decimal point. */
{
    while (*--s == '9') {
	if (s == retval) {
	    ++(*kPtr);
	    *s = '1';
	    return s+1;
	}
    }
    ++*s;
    ++s;
    return s;
}

/*
 *----------------------------------------------------------------------
 *
 * AdjustRange --
 *
 *	Rescales a 'double' in preparation for formatting it using the 'quick'
 *	double-to-string method.
 *
 * Results:
 *	Returns the precision that has been lost in the prescaling as a count
 *	of units in the least significant place.
 *
 *----------------------------------------------------------------------
 */

static inline int
AdjustRange(
    double *dPtr,		/* INOUT: Number to adjust. */
    int k)			/* IN: floor(log10(d)) */
{
    int ieps;			/* Number of roundoff errors that have
				 * accumulated. */
    double d = *dPtr;		/* Number to adjust. */
    double ds;
    int i, j, j1;

    ieps = 2;

    if (k > 0) {
	/*
	 * The number must be reduced to bring it into range.
	 */

	ds = tens[k & 0xf];
	j = k >> 4;
	if (j & BLETCH) {
	    j &= (BLETCH-1);
	    d /= bigtens[N_BIGTENS - 1];
	    ieps++;
	}
	i = 0;
	for (; j != 0; j>>=1) {
	    if (j & 1) {
		ds *= bigtens[i];
		++ieps;
	    }
	    ++i;
	}
	d /= ds;
    } else if ((j1 = -k) != 0) {
	/*
	 * The number must be increased to bring it into range.
	 */

	d *= tens[j1 & 0xf];
	i = 0;
	for (j = j1>>4; j; j>>=1) {
	    if (j & 1) {
		ieps++;
		d *= bigtens[i];
	    }
	    ++i;
	}
    }

    *dPtr = d;
    return ieps;
}

/*
 *----------------------------------------------------------------------
 *
 * ShorteningQuickFormat --
 *
 *	Returns a 'quick' format of a double precision number to a string of
 *	digits, preferring a shorter string of digits if the shorter string is
 *	still within 1/2 ulp of the number.
 *
 * Results:
 *	Returns the string of digits. Returns NULL if the 'quick' method fails
 *	and the bignum method must be used.
 *
 * Side effects:
 *	Stores the position of the decimal point at '*kPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
ShorteningQuickFormat(
    double d,			/* Number to convert. */
    int k,			/* floor(log10(d)) */
    int ilim,			/* Number of significant digits to return. */
    double eps,			/* Estimated roundoff error. */
    char *retval,		/* Buffer to receive the digit string. */
    int *kPtr)			/* Pointer to stash the position of the
				 * decimal point. */
{
    char *s = retval;		/* Cursor in the return value. */
    int digit;			/* Current digit. */
    int i;

    eps = 0.5 / tens[ilim-1] - eps;
    i = 0;
    for (;;) {
	/*
	 * Convert a digit.
	 */

	digit = (int) d;
	d -= digit;
	*s++ = '0' + digit;

	/*
	 * Truncate the conversion if the string of digits is within 1/2 ulp
	 * of the actual value.
	 */

	if (d < eps) {
	    *kPtr = k;
	    return s;
	}
	if ((1. - d) < eps) {
	    *kPtr = k;
	    return BumpUp(s, retval, kPtr);
	}

	/*
	 * Bail out if the conversion fails to converge to a sufficiently
	 * precise value.
	 */

	if (++i >= ilim) {
	    return NULL;
	}

	/*
	 * Bring the next digit to the integer part.
	 */

	eps *= 10;
	d *= 10.0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StrictQuickFormat --
 *
 *	Convert a double precision number of a string of a precise number of
 *	digits, using the 'quick' double precision method.
 *
 * Results:
 *	Returns the digit string, or NULL if the bignum method must be used to
 *	do the formatting.
 *
 * Side effects:
 *	Stores the position of the decimal point in '*kPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
StrictQuickFormat(
    double d,			/* Number to convert. */
    int k,			/* floor(log10(d)) */
    int ilim,			/* Number of significant digits to return. */
    double eps,			/* Estimated roundoff error. */
    char *retval,		/* Start of the digit string. */
    int *kPtr)			/* Pointer to stash the position of the
				 * decimal point. */
{
    char *s = retval;		/* Cursor in the return value. */
    int digit;			/* Current digit of the answer. */
    int i;

    eps *= tens[ilim-1];
    i = 1;
    for (;;) {
	/*
	 * Extract a digit.
	 */

	digit = (int) d;
	d -= digit;
	if (d == 0.0) {
	    ilim = i;
	}
	*s++ = '0' + digit;

	/*
	 * When the given digit count is reached, handle trailing strings of 0
	 * and 9.
	 */

	if (i == ilim) {
	    if (d > 0.5 + eps) {
		*kPtr = k;
		return BumpUp(s, retval, kPtr);
	    } else if (d < 0.5 - eps) {
		while (*--s == '0') {
		    /* do nothing */
		}
		s++;
		*kPtr = k;
		return s;
	    } else {
		return NULL;
	    }
	}

	/*
	 * Advance to the next digit.
	 */

	++i;
	d *= 10.0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * QuickConversion --
 *
 *	Converts a floating point number the 'quick' way, when only a limited
 *	number of digits is required and floating point arithmetic can
 *	therefore be used for the intermediate results.
 *
 * Results:
 *	Returns the converted string, or NULL if the bignum method must be
 *	used.
 *
 *----------------------------------------------------------------------
 */

static inline char *
QuickConversion(
    double e,			/* Number to format. */
    int k,			/* floor(log10(d)), approximately. */
    int k_check,		/* 0 if k is exact, 1 if it may be too high */
    int flags,			/* Flags passed to dtoa:
				 *    TCL_DD_SHORTEN_FLAG */
    int len,			/* Length of the return value. */
    int ilim,			/* Number of digits to store. */
    int ilim1,			/* Number of digits to store if we misguessed
				 * k. */
    int *decpt,			/* OUTPUT: Location of the decimal point. */
    char **endPtr)		/* OUTPUT: Pointer to the terminal null
				 * byte. */
{
    int ieps;			/* Number of 1-ulp roundoff errors that have
				 * accumulated in the calculation. */
    Double eps;			/* Estimated roundoff error. */
    char *retval;		/* Returned string. */
    char *end;			/* Pointer to the terminal null byte in the
				 * returned string. */
    volatile double d;		/* Workaround for a bug in mingw gcc 3.4.5 */

    /*
     * Bring d into the range [1 .. 10).
     */

    ieps = AdjustRange(&e, k);
    d = e;

    /*
     * If the guessed value of k didn't get d into range, adjust it by one. If
     * that leaves us outside the range in which quick format is accurate,
     * bail out.
     */

    if (k_check && d < 1. && ilim > 0) {
	if (ilim1 < 0) {
	    return NULL;
	}
	ilim = ilim1;
	--k;
	d *= 10.0;
	++ieps;
    }

    /*
     * Compute estimated roundoff error.
     */

    eps.d = ieps * d + 7.;
    eps.w.word0 -= (FP_PRECISION-1) << EXP_SHIFT;

    /*
     * Handle the peculiar case where the result has no significant digits.
     */

    retval = ckalloc(len + 1);
    if (ilim == 0) {
	d -= 5.;
	if (d > eps.d) {
	    *retval = '1';
	    *decpt = k;
	    return retval;
	} else if (d < -eps.d) {
	    *decpt = k;
	    return retval;
	} else {
	    ckfree(retval);
	    return NULL;
	}
    }

    /*
     * Format the digit string.
     */

    if (flags & TCL_DD_SHORTEN_FLAG) {
	end = ShorteningQuickFormat(d, k, ilim, eps.d, retval, decpt);
    } else {
	end = StrictQuickFormat(d, k, ilim, eps.d, retval, decpt);
    }
    if (end == NULL) {
	ckfree(retval);
	return NULL;
    }
    *end = '\0';
    if (endPtr != NULL) {
	*endPtr = end;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * CastOutPowersOf2 --
 *
 *	Adjust the factors 'b2', 'm2', and 's2' to cast out common powers of 2
 *	from numerator and denominator in preparation for the 'bignum' method
 *	of floating point conversion.
 *
 *----------------------------------------------------------------------
 */

static inline void
CastOutPowersOf2(
    int *b2,			/* Power of 2 to multiply the significand. */
    int *m2,			/* Power of 2 to multiply 1/2 ulp. */
    int *s2)			/* Power of 2 to multiply the common
				 * denominator. */
{
    int i;

    if (*m2 > 0 && *s2 > 0) {	/* Find the smallest power of 2 in the
				 * numerator. */
	if (*m2 < *s2) {	/* Find the lowest common denominator. */
	    i = *m2;
	} else {
	    i = *s2;
	}
	*b2 -= i;		/* Reduce to lowest terms. */
	*m2 -= i;
	*s2 -= i;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ShorteningInt64Conversion --
 *
 *	Converts a double-precision number to the shortest string of digits
 *	that reconverts exactly to the given number, or to 'ilim' digits if
 *	that will yield a shorter result. The numerator and denominator in
 *	David Gay's conversion algorithm are known to fit in Tcl_WideUInt,
 *	giving considerably faster arithmetic than mp_int's.
 *
 * Results:
 *	Returns the string of significant decimal digits, in newly allocated
 *	memory
 *
 * Side effects:
 *	Stores the location of the decimal point in '*decpt' and the location
 *	of the terminal null byte in '*endPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
ShorteningInt64Conversion(
    Double *dPtr,		/* Original number to convert. */
    int convType,		/* Type of conversion (shortest, Steele,
				 * E format, F format). */
    Tcl_WideUInt bw,		/* Integer significand. */
    int b2, int b5,		/* Scale factor for the significand in the
				 * numerator. */
    int m2plus, int m2minus, int m5,
				/* Scale factors for 1/2 ulp in the numerator
				 * (will be different if bw == 1. */
    int s2, int s5,		/* Scale factors for the denominator. */
    int k,			/* Number of output digits before the decimal
				 * point. */
    int len,			/* Number of digits to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Position of the terminal '\0' at
				 *	   the end of the returned string. */
{
    char *retval = ckalloc(len + 1);
				/* Output buffer. */
    Tcl_WideUInt b = (bw * wuipow5[b5]) << b2;
				/* Numerator of the fraction being
				 * converted. */
    Tcl_WideUInt S = wuipow5[s5] << s2;
				/* Denominator of the fraction being
				 * converted. */
    Tcl_WideUInt mplus, mminus;	/* Ranges for testing whether the result is
				 * within roundoff of being exact. */
    int digit;			/* Current output digit. */
    char *s = retval;		/* Cursor in the output buffer. */
    int i;			/* Current position in the output buffer. */

    /*
     * Adjust if the logarithm was guessed wrong.
     */

    if (b < S) {
	b = 10 * b;
	++m2plus; ++m2minus; ++m5;
	ilim = ilim1;
	--k;
    }

    /*
     * Compute roundoff ranges.
     */

    mplus = wuipow5[m5] << m2plus;
    mminus = wuipow5[m5] << m2minus;

    /*
     * Loop through the digits.
     */

    i = 1;
    for (;;) {
	digit = (int)(b / S);
	if (digit > 10) {
	    Tcl_Panic("wrong digit!");
	}
	b = b % S;

	/*
	 * Does the current digit put us on the low side of the exact value
	 * but within within roundoff of being exact?
	 */

	if (b < mplus || (b == mplus
		&& convType != TCL_DD_STEELE0 && (dPtr->w.word1 & 1) == 0)) {
	    /*
	     * Make sure we shouldn't be rounding *up* instead, in case the
	     * next number above is closer.
	     */

	    if (2 * b > S || (2 * b == S && (digit & 1) != 0)) {
		++digit;
		if (digit == 10) {
		    *s++ = '9';
		    s = BumpUp(s, retval, &k);
		    break;
		}
	    }

	    /*
	     * Stash the current digit.
	     */

	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Does one plus the current digit put us within roundoff of the
	 * number?
	 */

	if (b > S - mminus || (b == S - mminus
		&& convType != TCL_DD_STEELE0 && (dPtr->w.word1 & 1) == 0)) {
	    if (digit == 9) {
		*s++ = '9';
		s = BumpUp(s, retval, &k);
		break;
	    }
	    ++digit;
	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Have we converted all the requested digits?
	 */

	*s++ = '0' + digit;
	if (i == ilim) {
	    if (2*b > S || (2*b == S && (digit & 1) != 0)) {
		s = BumpUp(s, retval, &k);
	    }
	    break;
	}

	/*
	 * Advance to the next digit.
	 */

	b = 10 * b;
	mplus = 10 * mplus;
	mminus = 10 * mminus;
	++i;
    }

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * StrictInt64Conversion --
 *
 *	Converts a double-precision number to a fixed-length string of 'ilim'
 *	digits that reconverts exactly to the given number.  ('ilim' should be
 *	replaced with 'ilim1' in the case where log10(d) has been
 *	overestimated).  The numerator and denominator in David Gay's
 *	conversion algorithm are known to fit in Tcl_WideUInt, giving
 *	considerably faster arithmetic than mp_int's.
 *
 * Results:
 *	Returns the string of significant decimal digits, in newly allocated
 *	memory
 *
 * Side effects:
 *	Stores the location of the decimal point in '*decpt' and the location
 *	of the terminal null byte in '*endPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
StrictInt64Conversion(
    Double *dPtr,		/* Original number to convert. */
    int convType,		/* Type of conversion (shortest, Steele,
				 * E format, F format). */
    Tcl_WideUInt bw,		/* Integer significand. */
    int b2, int b5,		/* Scale factor for the significand in the
				 * numerator. */
    int s2, int s5,		/* Scale factors for the denominator. */
    int k,			/* Number of output digits before the decimal
				 * point. */
    int len,			/* Number of digits to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Position of the terminal '\0' at
				 *	   the end of the returned string. */
{
    char *retval = ckalloc(len + 1);
				/* Output buffer. */
    Tcl_WideUInt b = (bw * wuipow5[b5]) << b2;
				/* Numerator of the fraction being
				 * converted. */
    Tcl_WideUInt S = wuipow5[s5] << s2;
				/* Denominator of the fraction being
				 * converted. */
    int digit;			/* Current output digit. */
    char *s = retval;		/* Cursor in the output buffer. */
    int i;			/* Current position in the output buffer. */

    /*
     * Adjust if the logarithm was guessed wrong.
     */

    if (b < S) {
	b = 10 * b;
	ilim = ilim1;
	--k;
    }

    /*
     * Loop through the digits.
     */

    i = 1;
    for (;;) {
	digit = (int)(b / S);
	if (digit > 10) {
	    Tcl_Panic("wrong digit!");
	}
	b = b % S;

	/*
	 * Have we converted all the requested digits?
	 */

	*s++ = '0' + digit;
	if (i == ilim) {
	    if (2*b > S || (2*b == S && (digit & 1) != 0)) {
		s = BumpUp(s, retval, &k);
	    } else {
		while (*--s == '0') {
		    /* do nothing */
		}
		++s;
	    }
	    break;
	}

	/*
	 * Advance to the next digit.
	 */

	b = 10 * b;
	++i;
    }

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * ShouldBankerRoundUpPowD --
 *
 *	Test whether bankers' rounding should round a digit up. Assumption is
 *	made that the denominator of the fraction being tested is a power of
 *	2**DIGIT_BIT.
 *
 * Results:
 *	Returns 1 iff the fraction is more than 1/2, or if the fraction is
 *	exactly 1/2 and the digit is odd.
 *
 *----------------------------------------------------------------------
 */

static inline int
ShouldBankerRoundUpPowD(
    mp_int *b,			/* Numerator of the fraction. */
    int sd,			/* Denominator is 2**(sd*DIGIT_BIT). */
    int isodd)			/* 1 if the digit is odd, 0 if even. */
{
    int i;
    static const mp_digit topbit = 1 << (DIGIT_BIT - 1);

    if (b->used < sd || (b->dp[sd-1] & topbit) == 0) {
	return 0;
    }
    if (b->dp[sd-1] != topbit) {
	return 1;
    }
    for (i = sd-2; i >= 0; --i) {
	if (b->dp[i] != 0) {
	    return 1;
	}
    }
    return isodd;
}

/*
 *----------------------------------------------------------------------
 *
 * ShouldBankerRoundUpToNextPowD --
 *
 *	Tests whether bankers' rounding will round down in the "denominator is
 *	a power of 2**MP_DIGIT" case.
 *
 * Results:
 *	Returns 1 if the rounding will be performed - which increases the
 *	digit by one - and 0 otherwise.
 *
 *----------------------------------------------------------------------
 */

static inline int
ShouldBankerRoundUpToNextPowD(
    mp_int *b,			/* Numerator of the fraction. */
    mp_int *m,			/* Numerator of the rounding tolerance. */
    int sd,			/* Common denominator is 2**(sd*DIGIT_BIT). */
    int convType,		/* Conversion type: STEELE defeats
				 * round-to-even (not sure why one wants to do
				 * this; I copied it from Gay). FIXME */
    int isodd,			/* 1 if the integer significand is odd. */
    mp_int *temp)		/* Work area for the calculation. */
{
    int i;

    /*
     * Compare B and S-m - which is the same as comparing B+m and S - which we
     * do by computing b+m and doing a bitwhack compare against
     * 2**(DIGIT_BIT*sd)
     */

    mp_add(b, m, temp);
    if (temp->used <= sd) {	/* Too few digits to be > s */
	return 0;
    }
    if (temp->used > sd+1 || temp->dp[sd] > 1) {
				/* >= 2s */
	return 1;
    }
    for (i = sd-1; i >= 0; --i) {
				/* Check for ==s */
	if (temp->dp[i] != 0) {	/* > s */
	    return 1;
	}
    }
    if (convType == TCL_DD_STEELE0) {
				/* Biased rounding. */
	return 0;
    }
    return isodd;
}

/*
 *----------------------------------------------------------------------
 *
 * ShorteningBignumConversionPowD --
 *
 *	Converts a double-precision number to the shortest string of digits
 *	that reconverts exactly to the given number, or to 'ilim' digits if
 *	that will yield a shorter result. The denominator in David Gay's
 *	conversion algorithm is known to be a power of 2**DIGIT_BIT, and hence
 *	the division in the main loop may be replaced by a digit shift and
 *	mask.
 *
 * Results:
 *	Returns the string of significant decimal digits, in newly allocated
 *	memory
 *
 * Side effects:
 *	Stores the location of the decimal point in '*decpt' and the location
 *	of the terminal null byte in '*endPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
ShorteningBignumConversionPowD(
    Double *dPtr,		/* Original number to convert. */
    int convType,		/* Type of conversion (shortest, Steele,
				 * E format, F format). */
    Tcl_WideUInt bw,		/* Integer significand. */
    int b2, int b5,		/* Scale factor for the significand in the
				 * numerator. */
    int m2plus, int m2minus, int m5,
				/* Scale factors for 1/2 ulp in the numerator
				 * (will be different if bw == 1). */
    int sd,			/* Scale factor for the denominator. */
    int k,			/* Number of output digits before the decimal
				 * point. */
    int len,			/* Number of digits to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Position of the terminal '\0' at
				 *	   the end of the returned string. */
{
    char *retval = ckalloc(len + 1);
				/* Output buffer. */
    mp_int b;			/* Numerator of the fraction being
				 * converted. */
    mp_int mplus, mminus;	/* Bounds for roundoff. */
    mp_digit digit;		/* Current output digit. */
    char *s = retval;		/* Cursor in the output buffer. */
    int i;			/* Index in the output buffer. */
    mp_int temp;
    int r1;

    /*
     * b = bw * 2**b2 * 5**b5
     * mminus = 5**m5
     */

    TclBNInitBignumFromWideUInt(&b, bw);
    mp_init_set_int(&mminus, 1);
    MulPow5(&b, b5, &b);
    mp_mul_2d(&b, b2, &b);

    /*
     * Adjust if the logarithm was guessed wrong.
     */

    if (b.used <= sd) {
	mp_mul_d(&b, 10, &b);
	++m2plus; ++m2minus; ++m5;
	ilim = ilim1;
	--k;
    }

    /*
     * mminus = 5**m5 * 2**m2minus
     * mplus = 5**m5 * 2**m2plus
     */

    mp_mul_2d(&mminus, m2minus, &mminus);
    MulPow5(&mminus, m5, &mminus);
    if (m2plus > m2minus) {
	mp_init_copy(&mplus, &mminus);
	mp_mul_2d(&mplus, m2plus-m2minus, &mplus);
    }
    mp_init(&temp);

    /*
     * Loop through the digits. Do division and mod by s == 2**(sd*DIGIT_BIT)
     * by mp_digit extraction.
     */

    i = 0;
    for (;;) {
	if (b.used <= sd) {
	    digit = 0;
	} else {
	    digit = b.dp[sd];
	    if (b.used > sd+1 || digit >= 10) {
		Tcl_Panic("wrong digit!");
	    }
	    --b.used; mp_clamp(&b);
	}

	/*
	 * Does the current digit put us on the low side of the exact value
	 * but within within roundoff of being exact?
	 */

	r1 = mp_cmp_mag(&b, (m2plus > m2minus)? &mplus : &mminus);
	if (r1 == MP_LT || (r1 == MP_EQ
		&& convType != TCL_DD_STEELE0 && (dPtr->w.word1 & 1) == 0)) {
	    /*
	     * Make sure we shouldn't be rounding *up* instead, in case the
	     * next number above is closer.
	     */

	    if (ShouldBankerRoundUpPowD(&b, sd, digit&1)) {
		++digit;
		if (digit == 10) {
		    *s++ = '9';
		    s = BumpUp(s, retval, &k);
		    break;
		}
	    }

	    /*
	     * Stash the last digit.
	     */

	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Does one plus the current digit put us within roundoff of the
	 * number?
	 */

	if (ShouldBankerRoundUpToNextPowD(&b, &mminus, sd, convType,
		dPtr->w.word1 & 1, &temp)) {
	    if (digit == 9) {
		*s++ = '9';
		s = BumpUp(s, retval, &k);
		break;
	    }
	    ++digit;
	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Have we converted all the requested digits?
	 */

	*s++ = '0' + digit;
	if (i == ilim) {
	    if (ShouldBankerRoundUpPowD(&b, sd, digit&1)) {
		s = BumpUp(s, retval, &k);
	    }
	    break;
	}

	/*
	 * Advance to the next digit.
	 */

	mp_mul_d(&b, 10, &b);
	mp_mul_d(&mminus, 10, &mminus);
	if (m2plus > m2minus) {
	    mp_mul_2d(&mminus, m2plus-m2minus, &mplus);
	}
	++i;
    }

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    if (m2plus > m2minus) {
	mp_clear(&mplus);
    }
    mp_clear_multi(&b, &mminus, &temp, NULL);
    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * StrictBignumConversionPowD --
 *
 *	Converts a double-precision number to a fixed-lengt string of 'ilim'
 *	digits (or 'ilim1' if log10(d) has been overestimated).  The
 *	denominator in David Gay's conversion algorithm is known to be a power
 *	of 2**DIGIT_BIT, and hence the division in the main loop may be
 *	replaced by a digit shift and mask.
 *
 * Results:
 *	Returns the string of significant decimal digits, in newly allocated
 *	memory.
 *
 * Side effects:
 *	Stores the location of the decimal point in '*decpt' and the location
 *	of the terminal null byte in '*endPtr'.
 *
 *----------------------------------------------------------------------
 */

static inline char *
StrictBignumConversionPowD(
    Double *dPtr,		/* Original number to convert. */
    int convType,		/* Type of conversion (shortest, Steele,
				 * E format, F format). */
    Tcl_WideUInt bw,		/* Integer significand. */
    int b2, int b5,		/* Scale factor for the significand in the
				 * numerator. */
    int sd,			/* Scale factor for the denominator. */
    int k,			/* Number of output digits before the decimal
				 * point. */
    int len,			/* Number of digits to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Position of the terminal '\0' at
				 *	   the end of the returned string. */
{
    char *retval = ckalloc(len + 1);
				/* Output buffer. */
    mp_int b;			/* Numerator of the fraction being
				 * converted. */
    mp_digit digit;		/* Current output digit. */
    char *s = retval;		/* Cursor in the output buffer. */
    int i;			/* Index in the output buffer. */
    mp_int temp;

    /*
     * b = bw * 2**b2 * 5**b5
     */

    TclBNInitBignumFromWideUInt(&b, bw);
    MulPow5(&b, b5, &b);
    mp_mul_2d(&b, b2, &b);

    /*
     * Adjust if the logarithm was guessed wrong.
     */

    if (b.used <= sd) {
	mp_mul_d(&b, 10, &b);
	ilim = ilim1;
	--k;
    }
    mp_init(&temp);

    /*
     * Loop through the digits. Do division and mod by s == 2**(sd*DIGIT_BIT)
     * by mp_digit extraction.
     */

    i = 1;
    for (;;) {
	if (b.used <= sd) {
	    digit = 0;
	} else {
	    digit = b.dp[sd];
	    if (b.used > sd+1 || digit >= 10) {
		Tcl_Panic("wrong digit!");
	    }
	    --b.used;
	    mp_clamp(&b);
	}

	/*
	 * Have we converted all the requested digits?
	 */

	*s++ = '0' + digit;
	if (i == ilim) {
	    if (ShouldBankerRoundUpPowD(&b, sd, digit&1)) {
		s = BumpUp(s, retval, &k);
	    }
	    while (*--s == '0') {
		/* do nothing */
	    }
	    ++s;
	    break;
	}

	/*
	 * Advance to the next digit.
	 */

	mp_mul_d(&b, 10, &b);
	++i;
    }

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    mp_clear_multi(&b, &temp, NULL);
    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * ShouldBankerRoundUp --
 *
 *	Tests whether a digit should be rounded up or down when finishing
 *	bignum-based floating point conversion.
 *
 * Results:
 *	Returns 1 if the number needs to be rounded up, 0 otherwise.
 *
 *----------------------------------------------------------------------
 */

static inline int
ShouldBankerRoundUp(
    mp_int *twor,		/* 2x the remainder from thd division that
				 * produced the last digit. */
    mp_int *S,			/* Denominator. */
    int isodd)			/* Flag == 1 if the last digit is odd. */
{
    int r = mp_cmp_mag(twor, S);

    switch (r) {
    case MP_LT:
	return 0;
    case MP_EQ:
	return isodd;
    case MP_GT:
	return 1;
    }
    Tcl_Panic("in ShouldBankerRoundUp, trichotomy fails!");
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ShouldBankerRoundUpToNext --
 *
 *	Tests whether the remainder is great enough to force rounding to the
 *	next higher digit.
 *
 * Results:
 *	Returns 1 if the number should be rounded up, 0 otherwise.
 *
 *----------------------------------------------------------------------
 */

static inline int
ShouldBankerRoundUpToNext(
    mp_int *b,			/* Remainder from the division that produced
				 * the last digit. */
    mp_int *m,			/* Numerator of the rounding tolerance. */
    mp_int *S,			/* Denominator. */
    int convType,		/* Conversion type: STEELE0 defeats
				 * round-to-even. (Not sure why one would want
				 * this; I coped it from Gay). FIXME */
    int isodd,			/* 1 if the integer significand is odd. */
    mp_int *temp)		/* Work area needed for the calculation. */
{
    int r;

    /*
     * Compare b and S-m: this is the same as comparing B+m and S.
     */

    mp_add(b, m, temp);
    r = mp_cmp_mag(temp, S);
    switch(r) {
    case MP_LT:
	return 0;
    case MP_EQ:
	if (convType == TCL_DD_STEELE0) {
	    return 0;
	} else {
	    return isodd;
	}
    case MP_GT:
	return 1;
    }
    Tcl_Panic("in ShouldBankerRoundUpToNext, trichotomy fails!");
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * ShorteningBignumConversion --
 *
 *	Convert a floating point number to a variable-length digit string
 *	using the multiprecision method.
 *
 * Results:
 *	Returns the string of digits.
 *
 * Side effects:
 *	Stores the position of the decimal point in *decpt.  Stores a pointer
 *	to the end of the number in *endPtr.
 *
 *----------------------------------------------------------------------
 */

static inline char *
ShorteningBignumConversion(
    Double *dPtr,		/* Original number being converted. */
    int convType,		/* Conversion type. */
    Tcl_WideUInt bw,		/* Integer significand and exponent. */
    int b2,			/* Scale factor for the significand. */
    int m2plus, int m2minus,	/* Scale factors for 1/2 ulp in numerator. */
    int s2, int s5,		/* Scale factors for denominator. */
    int k,			/* Guessed position of the decimal point. */
    int len,			/* Size of the digit buffer to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Pointer to the end of the number */
{
    char *retval = ckalloc(len+1);
				/* Buffer of digits to return. */
    char *s = retval;		/* Cursor in the return value. */
    mp_int b;			/* Numerator of the result. */
    mp_int mminus;		/* 1/2 ulp below the result. */
    mp_int mplus;		/* 1/2 ulp above the result. */
    mp_int S;			/* Denominator of the result. */
    mp_int dig;			/* Current digit of the result. */
    int digit;			/* Current digit of the result. */
    mp_int temp;		/* Work area. */
    int minit = 1;		/* Fudge factor for when we misguess k. */
    int i;
    int r1;

    /*
     * b = bw * 2**b2 * 5**b5
     * S = 2**s2 * 5*s5
     */

    TclBNInitBignumFromWideUInt(&b, bw);
    mp_mul_2d(&b, b2, &b);
    mp_init_set_int(&S, 1);
    MulPow5(&S, s5, &S); mp_mul_2d(&S, s2, &S);

    /*
     * Handle the case where we guess the position of the decimal point wrong.
     */

    if (mp_cmp_mag(&b, &S) == MP_LT) {
	mp_mul_d(&b, 10, &b);
	minit = 10;
	ilim =ilim1;
	--k;
    }

    /*
     * mminus = 2**m2minus * 5**m5
     */

    mp_init_set_int(&mminus, minit);
    mp_mul_2d(&mminus, m2minus, &mminus);
    if (m2plus > m2minus) {
	mp_init_copy(&mplus, &mminus);
	mp_mul_2d(&mplus, m2plus-m2minus, &mplus);
    }
    mp_init(&temp);

    /*
     * Loop through the digits.
     */

    mp_init(&dig);
    i = 1;
    for (;;) {
	mp_div(&b, &S, &dig, &b);
	if (dig.used > 1 || dig.dp[0] >= 10) {
	    Tcl_Panic("wrong digit!");
	}
	digit = dig.dp[0];

	/*
	 * Does the current digit leave us with a remainder small enough to
	 * round to it?
	 */

	r1 = mp_cmp_mag(&b, (m2plus > m2minus)? &mplus : &mminus);
	if (r1 == MP_LT || (r1 == MP_EQ
		&& convType != TCL_DD_STEELE0 && (dPtr->w.word1 & 1) == 0)) {
	    mp_mul_2d(&b, 1, &b);
	    if (ShouldBankerRoundUp(&b, &S, digit&1)) {
		++digit;
		if (digit == 10) {
		    *s++ = '9';
		    s = BumpUp(s, retval, &k);
		    break;
		}
	    }
	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Does the current digit leave us with a remainder large enough to
	 * commit to rounding up to the next higher digit?
	 */

	if (ShouldBankerRoundUpToNext(&b, &mminus, &S, convType,
		dPtr->w.word1 & 1, &temp)) {
	    ++digit;
	    if (digit == 10) {
		*s++ = '9';
		s = BumpUp(s, retval, &k);
		break;
	    }
	    *s++ = '0' + digit;
	    break;
	}

	/*
	 * Have we converted all the requested digits?
	 */

	*s++ = '0' + digit;
	if (i == ilim) {
	    mp_mul_2d(&b, 1, &b);
	    if (ShouldBankerRoundUp(&b, &S, digit&1)) {
		s = BumpUp(s, retval, &k);
	    }
	    break;
	}

	/*
	 * Advance to the next digit.
	 */

	if (s5 > 0) {
	    /*
	     * Can possibly shorten the denominator.
	     */

	    mp_mul_2d(&b, 1, &b);
	    mp_mul_2d(&mminus, 1, &mminus);
	    if (m2plus > m2minus) {
		mp_mul_2d(&mplus, 1, &mplus);
	    }
	    mp_div_d(&S, 5, &S, NULL);
	    --s5;

	    /*
	     * IDEA: It might possibly be a win to fall back to int64_t
	     *       arithmetic here if S < 2**64/10. But it's a win only for
	     *       a fairly narrow range of magnitudes so perhaps not worth
	     *       bothering.  We already know that we shorten the
	     *       denominator by at least 1 mp_digit, perhaps 2, as we do
	     *       the conversion for 17 digits of significance.
	     * Possible savings:
	     * 10**26   1 trip through loop before fallback possible
	     * 10**27   1 trip
	     * 10**28   2 trips
	     * 10**29   3 trips
	     * 10**30   4 trips
	     * 10**31   5 trips
	     * 10**32   6 trips
	     * 10**33   7 trips
	     * 10**34   8 trips
	     * 10**35   9 trips
	     * 10**36  10 trips
	     * 10**37  11 trips
	     * 10**38  12 trips
	     * 10**39  13 trips
	     * 10**40  14 trips
	     * 10**41  15 trips
	     * 10**42  16 trips
	     * thereafter no gain.
	     */
	} else {
	    mp_mul_d(&b, 10, &b);
	    mp_mul_d(&mminus, 10, &mminus);
	    if (m2plus > m2minus) {
		mp_mul_2d(&mplus, 10, &mplus);
	    }
	}

	++i;
    }

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    if (m2plus > m2minus) {
	mp_clear(&mplus);
    }
    mp_clear_multi(&b, &mminus, &temp, &dig, &S, NULL);
    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * StrictBignumConversion --
 *
 *	Convert a floating point number to a fixed-length digit string using
 *	the multiprecision method.
 *
 * Results:
 *	Returns the string of digits.
 *
 * Side effects:
 *	Stores the position of the decimal point in *decpt.  Stores a pointer
 *	to the end of the number in *endPtr.
 *
 *----------------------------------------------------------------------
 */

static inline char *
StrictBignumConversion(
    Double *dPtr,		/* Original number being converted. */
    int convType,		/* Conversion type. */
    Tcl_WideUInt bw,		/* Integer significand and exponent. */
    int b2,			/* Scale factor for the significand. */
    int s2, int s5,		/* Scale factors for denominator. */
    int k,			/* Guessed position of the decimal point. */
    int len,			/* Size of the digit buffer to allocate. */
    int ilim,			/* Number of digits to convert if b >= s */
    int ilim1,			/* Number of digits to convert if b < s */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    char **endPtr)		/* OUTPUT: Pointer to the end of the number */
{
    char *retval = ckalloc(len+1);
				/* Buffer of digits to return. */
    char *s = retval;		/* Cursor in the return value. */
    mp_int b;			/* Numerator of the result. */
    mp_int S;			/* Denominator of the result. */
    mp_int dig;			/* Current digit of the result. */
    int digit;			/* Current digit of the result. */
    mp_int temp;		/* Work area. */
    int g;			/* Size of the current digit ground. */
    int i, j;

    /*
     * b = bw * 2**b2 * 5**b5
     * S = 2**s2 * 5*s5
     */

    mp_init_multi(&temp, &dig, NULL);
    TclBNInitBignumFromWideUInt(&b, bw);
    mp_mul_2d(&b, b2, &b);
    mp_init_set_int(&S, 1);
    MulPow5(&S, s5, &S); mp_mul_2d(&S, s2, &S);

    /*
     * Handle the case where we guess the position of the decimal point wrong.
     */

    if (mp_cmp_mag(&b, &S) == MP_LT) {
	mp_mul_d(&b, 10, &b);
	ilim =ilim1;
	--k;
    }

    /*
     * Convert the leading digit.
     */

    i = 0;
    mp_div(&b, &S, &dig, &b);
    if (dig.used > 1 || dig.dp[0] >= 10) {
	Tcl_Panic("wrong digit!");
    }
    digit = dig.dp[0];

    /*
     * Is a single digit all that was requested?
     */

    *s++ = '0' + digit;
    if (++i >= ilim) {
	mp_mul_2d(&b, 1, &b);
	if (ShouldBankerRoundUp(&b, &S, digit&1)) {
	    s = BumpUp(s, retval, &k);
	}
    } else {
	for (;;) {
	    /*
	     * Shift by a group of digits.
	     */

	    g = ilim - i;
	    if (g > DIGIT_GROUP) {
		g = DIGIT_GROUP;
	    }
	    if (s5 >= g) {
		mp_div_d(&S, dpow5[g], &S, NULL);
		s5 -= g;
	    } else if (s5 > 0) {
		mp_div_d(&S, dpow5[s5], &S, NULL);
		mp_mul_d(&b, dpow5[g - s5], &b);
		s5 = 0;
	    } else {
		mp_mul_d(&b, dpow5[g], &b);
	    }
	    mp_mul_2d(&b, g, &b);

	    /*
	     * As with the shortening bignum conversion, it's possible at this
	     * point that we will have reduced the denominator to less than
	     * 2**64/10, at which point it would be possible to fall back to
	     * to int64_t arithmetic. But the potential payoff is tremendously
	     * less - unless we're working in F format - because we know that
	     * three groups of digits will always suffice for %#.17e, the
	     * longest format that doesn't introduce empty precision.
	     *
	     * Extract the next group of digits.
	     */

	    mp_div(&b, &S, &dig, &b);
	    if (dig.used > 1) {
		Tcl_Panic("wrong digit!");
	    }
	    digit = dig.dp[0];
	    for (j = g-1; j >= 0; --j) {
		int t = itens[j];

		*s++ = digit / t + '0';
		digit %= t;
	    }
	    i += g;

	    /*
	     * Have we converted all the requested digits?
	     */

	    if (i == ilim) {
		mp_mul_2d(&b, 1, &b);
		if (ShouldBankerRoundUp(&b, &S, digit&1)) {
		    s = BumpUp(s, retval, &k);
		}
		break;
	    }
	}
    }
    while (*--s == '0') {
	/* do nothing */
    }
    ++s;

    /*
     * Endgame - store the location of the decimal point and the end of the
     * string.
     */

    mp_clear_multi(&b, &S, &temp, &dig, NULL);
    *s = '\0';
    *decpt = k;
    if (endPtr) {
	*endPtr = s;
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDoubleDigits --
 *
 *	Core of Tcl's conversion of double-precision floating point numbers to
 *	decimal.
 *
 * Results:
 *	Returns a newly-allocated string of digits.
 *
 * Side effects:
 *	Sets *decpt to the index of the character in the string before the
 *	place that the decimal point should go. If 'endPtr' is not NULL, sets
 *	endPtr to point to the terminating '\0' byte of the string. Sets *sign
 *	to 1 if a minus sign should be printed with the number, or 0 if a plus
 *	sign (or no sign) should appear.
 *
 * This function is a service routine that produces the string of digits for
 * floating-point-to-decimal conversion. It can do a number of things
 * according to the 'flags' argument. Valid values for 'flags' include:
 *	TCL_DD_SHORTEST - This is the default for floating point conversion if
 *		::tcl_precision is 0. It constructs the shortest string of
 *		digits that will reconvert to the given number when scanned.
 *		For floating point numbers that are exactly between two
 *		decimal numbers, it resolves using the 'round to even' rule.
 *		With this value, the 'ndigits' parameter is ignored.
 *	TCL_DD_STEELE - This value is not recommended and may be removed in
 *		the future. It follows the conversion algorithm outlined in
 *		"How to Print Floating-Point Numbers Accurately" by Guy
 *		L. Steele, Jr. and Jon L. White [Proc. ACM SIGPLAN '90,
 *		pp. 112-126]. This rule has the effect of rendering 1e23 as
 *		9.9999999999999999e22 - which is a 'better' approximation in
 *		the sense that it will reconvert correctly even if a
 *		subsequent input conversion is 'round up' or 'round down'
 *		rather than 'round to nearest', but is surprising otherwise.
 *	TCL_DD_E_FORMAT - This value is used to prepare numbers for %e format
 *		conversion (or for default floating->string if tcl_precision
 *		is not 0). It constructs a string of at most 'ndigits' digits,
 *		choosing the one that is closest to the given number (and
 *		resolving ties with 'round to even').  It is allowed to return
 *		fewer than 'ndigits' if the number converts exactly; if the
 *		TCL_DD_E_FORMAT|TCL_DD_SHORTEN_FLAG is supplied instead, it
 *		also returns fewer digits if the shorter string will still
 *		reconvert without loss to the given input number. In any case,
 *		strings of trailing zeroes are suppressed.
 *	TCL_DD_F_FORMAT - This value is used to prepare numbers for %f format
 *		conversion. It requests that conversion proceed until
 *		'ndigits' digits after the decimal point have been converted.
 *		It is possible for this format to result in a zero-length
 *		string if the number is sufficiently small. Again, it is
 *		permissible for TCL_DD_F_FORMAT to return fewer digits for a
 *		number that converts exactly, and changing the argument to
 *		TCL_DD_F_FORMAT|TCL_DD_SHORTEN_FLAG will allow the routine
 *		also to return fewer digits if the shorter string will still
 *		reconvert without loss to the given input number. Strings of
 *		trailing zeroes are suppressed.
 *
 *	To any of these flags may be OR'ed TCL_DD_NO_QUICK; this flag requires
 *	all calculations to be done in exact arithmetic. Normally, E and F
 *	format with fewer than about 14 digits will be done with a quick
 *	floating point approximation and fall back on the exact arithmetic
 *	only if the input number is close enough to the midpoint between two
 *	decimal strings that more precision is needed to resolve which string
 *	is correct.
 *
 * The value stored in the 'decpt' argument on return may be negative
 * (indicating that the decimal point falls to the left of the string) or
 * greater than the length of the string. In addition, the value -9999 is used
 * as a sentinel to indicate that the string is one of the special values
 * "Infinity" and "NaN", and that no decimal point should be inserted.
 *
 *----------------------------------------------------------------------
 */

char *
TclDoubleDigits(
    double dv,			/* Number to convert. */
    int ndigits,		/* Number of digits requested. */
    int flags,			/* Conversion flags. */
    int *decpt,			/* OUTPUT: Position of the decimal point. */
    int *sign,			/* OUTPUT: 1 if the result is negative. */
    char **endPtr)		/* OUTPUT: If not NULL, receives a pointer to
				 *	   one character beyond the end of the
				 *	   returned string. */
{
    int convType = (flags & TCL_DD_CONVERSION_TYPE_MASK);
				/* Type of conversion being performed:
				 * TCL_DD_SHORTEST0, TCL_DD_STEELE0,
				 * TCL_DD_E_FORMAT, or TCL_DD_F_FORMAT. */
    Double d;			/* Union for deconstructing doubles. */
    Tcl_WideUInt bw;		/* Integer significand. */
    int be;			/* Power of 2 by which b must be multiplied */
    int bbits;			/* Number of bits needed to represent b. */
    int denorm;			/* Flag == 1 iff the input number was
				 * denormalized. */
    int k;			/* Estimate of floor(log10(d)). */
    int k_check;		/* Flag == 1 if d is near enough to a power of
				 * ten that k must be checked. */
    int b2, b5, s2, s5;		/* Powers of 2 and 5 in the numerator and
				 * denominator of intermediate results. */
    int ilim = -1, ilim1 = -1;	/* Number of digits to convert, and number to
				 * convert if log10(d) has been
				 * overestimated. */
    char *retval;		/* Return value from this function. */
    int i = -1;

    /*
     * Put the input number into a union for bit-whacking.
     */

    d.d = dv;

    /*
     * Handle the cases of negative numbers (by taking the absolute value:
     * this includes -Inf and -NaN!), infinity, Not a Number, and zero.
     */

    TakeAbsoluteValue(&d, sign);
    if ((d.w.word0 & EXP_MASK) == EXP_MASK) {
	return FormatInfAndNaN(&d, decpt, endPtr);
    }
    if (d.d == 0.0) {
	return FormatZero(decpt, endPtr);
    }

    /*
     * Unpack the floating point into a wide integer and an exponent.
     * Determine the number of bits that the big integer requires, and compute
     * a quick approximation (which may be one too high) of ceil(log10(d.d)).
     */

    denorm = ((d.w.word0 & EXP_MASK) == 0);
    DoubleToExpAndSig(d.d, &bw, &be, &bbits);
    k = ApproximateLog10(bw, be, bbits);
    k = BetterLog10(d.d, k, &k_check);

    /* At this point, we have:
     *	  d is the number to convert.
     *    bw are significand and exponent: d == bw*2**be,
     *    bbits is the length of bw: 2**bbits-1 <= bw < 2**bbits
     *	  k is either ceil(log10(d)) or ceil(log10(d))+1. k_check is 0 if we
     *      know that k is exactly ceil(log10(d)) and 1 if we need to check.
     *    We want a rational number
     *      r = b * 10**(1-k) = bw * 2**b2 * 5**b5 / (2**s2 / 5**s5),
     *    with b2, b5, s2, s5 >= 0.  Note that the most significant decimal
     *    digit is floor(r) and that successive digits can be obtained by
     *    setting r <- 10*floor(r) (or b <= 10 * (b % S)).  Find appropriate
     *    b2, b5, s2, s5.
     */

    ComputeScale(be, k, &b2, &b5, &s2, &s5);

    /*
     * Correct an incorrect caller-supplied 'ndigits'.  Also determine:
     *	i = The maximum number of decimal digits that will be returned in the
     *      formatted string.  This is k + 1 + ndigits for F format, 18 for
     *      shortest and Steele, and ndigits for E format.
     *  ilim = The number of significant digits to convert if k has been
     *         guessed correctly. This is -1 for shortest and Steele (which
     *         stop when all significance has been lost), 'ndigits' for E
     *         format, and 'k + 1 + ndigits' for F format.
     *  ilim1 = The minimum number of significant digits to convert if k has
     *	        been guessed 1 too high. This, too, is -1 for shortest and
     *	        Steele, and 'ndigits' for E format, but it's 'ndigits-1' for F
     *	        format.
     */

    SetPrecisionLimits(convType, k, &ndigits, &i, &ilim, &ilim1);

    /*
     * Try to do low-precision conversion in floating point rather than
     * resorting to expensive multiprecision arithmetic.
     */

    if (ilim >= 0 && ilim <= QUICK_MAX && !(flags & TCL_DD_NO_QUICK)) {
	retval = QuickConversion(d.d, k, k_check, flags, i, ilim, ilim1,
		decpt, endPtr);
	if (retval != NULL) {
	    return retval;
	}
    }

    /*
     * For shortening conversions, determine the upper and lower bounds for
     * the remainder at which we can stop.
     *   m+ = (2**m2plus * 5**m5) / (2**s2 * 5**s5) is the limit on the high
     *        side, and
     *   m- = (2**m2minus * 5**m5) / (2**s2 * 5**s5) is the limit on the low
     *        side.
     * We may need to increase s2 to put m2plus, m2minus, b2 over a common
     * denominator.
     */

    if (flags & TCL_DD_SHORTEN_FLAG) {
	int m2minus = b2;
	int m2plus;
	int m5 = b5;
	int len = i;

	/*
	 * Find the quantity i so that (2**i*5**b5)/(2**s2*5**s5) is 1/2 unit
	 * in the least significant place of the floating point number.
	 */

	if (denorm) {
	    i = be + EXPONENT_BIAS + (FP_PRECISION-1);
	} else {
	    i = 1 + FP_PRECISION - bbits;
	}
	b2 += i;
	s2 += i;

	/*
	 * Reduce the fractions to lowest terms, since the above calculation
	 * may have left excess powers of 2 in numerator and denominator.
	 */

	CastOutPowersOf2(&b2, &m2minus, &s2);

	/*
	 * In the special case where bw==1, the nearest floating point number
	 * to it on the low side is 1/4 ulp below it. Adjust accordingly.
	 */

	m2plus = m2minus;
	if (!denorm && bw == 1) {
	    ++b2;
	    ++s2;
	    ++m2plus;
	}

	if (s5+1 < N_LOG2POW5 && s2+1 + log2pow5[s5+1] <= 64) {
	    /*
	     * If 10*2**s2*5**s5 == 2**(s2+1)+5**(s5+1) fits in a 64-bit word,
	     * then all our intermediate calculations can be done using exact
	     * 64-bit arithmetic with no need for expensive multiprecision
	     * operations. (This will be true for all numbers in the range
	     * [1.0e-3 .. 1.0e+24]).
	     */

	    return ShorteningInt64Conversion(&d, convType, bw, b2, b5, m2plus,
		    m2minus, m5, s2, s5, k, len, ilim, ilim1, decpt, endPtr);
	} else if (s5 == 0) {
	    /*
	     * The denominator is a power of 2, so we can replace division by
	     * digit shifts. First we round up s2 to a multiple of DIGIT_BIT,
	     * and adjust m2 and b2 accordingly. Then we launch into a version
	     * of the comparison that's specialized for the 'power of mp_digit
	     * in the denominator' case.
	     */

	    if (s2 % DIGIT_BIT != 0) {
		int delta = DIGIT_BIT - (s2 % DIGIT_BIT);

		b2 += delta;
		m2plus += delta;
		m2minus += delta;
		s2 += delta;
	    }
	    return ShorteningBignumConversionPowD(&d, convType, bw, b2, b5,
		    m2plus, m2minus, m5, s2/DIGIT_BIT, k, len, ilim, ilim1,
		    decpt, endPtr);
	} else {
	    /*
	     * Alas, there's no helpful special case; use full-up bignum
	     * arithmetic for the conversion.
	     */

	    return ShorteningBignumConversion(&d, convType, bw, b2, m2plus,
		    m2minus, s2, s5, k, len, ilim, ilim1, decpt, endPtr);
	}
    } else {
	/*
	 * Non-shortening conversion.
	 */

	int len = i;

	/*
	 * Reduce numerator and denominator to lowest terms.
	 */

	if (b2 >= s2 && s2 > 0) {
	    b2 -= s2; s2 = 0;
	} else if (s2 >= b2 && b2 > 0) {
	    s2 -= b2; b2 = 0;
	}

	if (s5+1 < N_LOG2POW5 && s2+1 + log2pow5[s5+1] <= 64) {
	    /*
	     * If 10*2**s2*5**s5 == 2**(s2+1)+5**(s5+1) fits in a 64-bit word,
	     * then all our intermediate calculations can be done using exact
	     * 64-bit arithmetic with no need for expensive multiprecision
	     * operations.
	     */

	    return StrictInt64Conversion(&d, convType, bw, b2, b5, s2, s5, k,
		    len, ilim, ilim1, decpt, endPtr);
	} else if (s5 == 0) {
	    /*
	     * The denominator is a power of 2, so we can replace division by
	     * digit shifts. First we round up s2 to a multiple of DIGIT_BIT,
	     * and adjust m2 and b2 accordingly. Then we launch into a version
	     * of the comparison that's specialized for the 'power of mp_digit
	     * in the denominator' case.
	     */

	    if (s2 % DIGIT_BIT != 0) {
		int delta = DIGIT_BIT - (s2 % DIGIT_BIT);

		b2 += delta;
		s2 += delta;
	    }
	    return StrictBignumConversionPowD(&d, convType, bw, b2, b5,
		    s2/DIGIT_BIT, k, len, ilim, ilim1, decpt, endPtr);
	} else {
	    /*
	     * There are no helpful special cases, but at least we know in
	     * advance how many digits we will convert. We can run the
	     * conversion in steps of DIGIT_GROUP digits, so as to have many
	     * fewer mp_int divisions.
	     */

	    return StrictBignumConversion(&d, convType, bw, b2, s2, s5, k,
		    len, ilim, ilim1, decpt, endPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitDoubleConversion --
 *
 *	Initializes constants that are needed for conversions to and from
 *	'double'
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The log base 2 of the floating point radix, the number of bits in a
 *	double mantissa, and a table of the powers of five and ten are
 *	computed and stored.
 *
 *----------------------------------------------------------------------
 */

void
TclInitDoubleConversion(void)
{
    int i;
    int x;
    Tcl_WideUInt u;
    double d;
#ifdef IEEE_FLOATING_POINT
    union {
	double dv;
	Tcl_WideUInt iv;
    } bitwhack;
#endif
#if defined(__sgi) && defined(_COMPILER_VERSION)
    union fpc_csr mipsCR;

    mipsCR.fc_word = get_fpc_csr();
    mipsCR.fc_struct.flush = 0;
    set_fpc_csr(mipsCR.fc_word);
#endif

    /*
     * Initialize table of powers of 10 expressed as wide integers.
     */

    maxpow10_wide = (int)
	    floor(sizeof(Tcl_WideUInt) * CHAR_BIT * log(2.) / log(10.));
    pow10_wide = ckalloc((maxpow10_wide + 1) * sizeof(Tcl_WideUInt));
    u = 1;
    for (i = 0; i < maxpow10_wide; ++i) {
	pow10_wide[i] = u;
	u *= 10;
    }
    pow10_wide[i] = u;

    /*
     * Determine how many bits of precision a double has, and how many decimal
     * digits that represents.
     */

    if (frexp((double) FLT_RADIX, &log2FLT_RADIX) != 0.5) {
	Tcl_Panic("This code doesn't work on a decimal machine!");
    }
    log2FLT_RADIX--;
    mantBits = DBL_MANT_DIG * log2FLT_RADIX;
    d = 1.0;

    /*
     * Initialize a table of powers of ten that can be exactly represented in
     * a double.
     */

    x = (int) (DBL_MANT_DIG * log((double) FLT_RADIX) / log(5.0));
    if (x < MAXPOW) {
	mmaxpow = x;
    } else {
	mmaxpow = MAXPOW;
    }
    for (i=0 ; i<=mmaxpow ; ++i) {
	pow10vals[i] = d;
	d *= 10.0;
    }

    /*
     * Initialize a table of large powers of five.
     */

    for (i=0; i<9; ++i) {
	mp_init(pow5 + i);
    }
    mp_set(pow5, 5);
    for (i=0; i<8; ++i) {
	mp_sqr(pow5+i, pow5+i+1);
    }
    mp_init_set_int(pow5_13, 1220703125);
    for (i = 1; i < 5; ++i) {
	mp_init(pow5_13 + i);
	mp_sqr(pow5_13 + i - 1, pow5_13 + i);
    }

    /*
     * Determine the number of decimal digits to the left and right of the
     * decimal point in the largest and smallest double, the smallest double
     * that differs from zero, and the number of mp_digits needed to represent
     * the significand of a double.
     */

    maxDigits = (int) ((DBL_MAX_EXP * log((double) FLT_RADIX)
	    + 0.5 * log(10.)) / log(10.));
    minDigits = (int) floor((DBL_MIN_EXP - DBL_MANT_DIG)
	    * log((double) FLT_RADIX) / log(10.));
    log10_DIGIT_MAX = (int) floor(DIGIT_BIT * log(2.) / log(10.));

    /*
     * Nokia 770's software-emulated floating point is "middle endian": the
     * bytes within a 32-bit word are little-endian (like the native
     * integers), but the two words of a 'double' are presented most
     * significant word first.
     */

#ifdef IEEE_FLOATING_POINT
    bitwhack.dv = 1.000000238418579;
				/* 3ff0 0000 4000 0000 */
    if ((bitwhack.iv >> 32) == 0x3ff00000) {
	n770_fp = 0;
    } else if ((bitwhack.iv & 0xffffffff) == 0x3ff00000) {
	n770_fp = 1;
    } else {
	Tcl_Panic("unknown floating point word order on this machine");
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeDoubleConversion --
 *
 *	Cleans up this file on exit.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	Memory allocated by TclInitDoubleConversion is freed.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeDoubleConversion(void)
{
    int i;

    ckfree(pow10_wide);
    for (i=0; i<9; ++i) {
	mp_clear(pow5 + i);
    }
    for (i=0; i < 5; ++i) {
	mp_clear(pow5_13 + i);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitBignumFromDouble --
 *
 *	Extracts the integer part of a double and converts it to an arbitrary
 *	precision integer.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Initializes the bignum supplied, and stores the converted number in
 *	it.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_InitBignumFromDouble(
    Tcl_Interp *interp,		/* For error message. */
    double d,			/* Number to convert. */
    mp_int *b)			/* Place to store the result. */
{
    double fract;
    int expt;

    /*
     * Infinite values can't convert to bignum.
     */

    if (TclIsInfinite(d)) {
	if (interp != NULL) {
	    const char *s = "integer value too large to represent";

	    Tcl_SetObjResult(interp, Tcl_NewStringObj(s, -1));
	    Tcl_SetErrorCode(interp, "ARITH", "IOVERFLOW", s, NULL);
	}
	return TCL_ERROR;
    }

    fract = frexp(d,&expt);
    if (expt <= 0) {
	mp_init(b);
	mp_zero(b);
    } else {
	Tcl_WideInt w = (Tcl_WideInt) ldexp(fract, mantBits);
	int shift = expt - mantBits;

	TclBNInitBignumFromWideInt(b, w);
	if (shift < 0) {
	    mp_div_2d(b, -shift, b, NULL);
	} else if (shift > 0) {
	    mp_mul_2d(b, shift, b);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclBignumToDouble --
 *
 *	Convert an arbitrary-precision integer to a native floating point
 *	number.
 *
 * Results:
 *	Returns the converted number. Sets errno to ERANGE if the number is
 *	too large to convert.
 *
 *----------------------------------------------------------------------
 */

double
TclBignumToDouble(
    const mp_int *a)			/* Integer to convert. */
{
    mp_int b;
    int bits, shift, i, lsb;
    double r;


    /*
     * We need a 'mantBits'-bit significand.  Determine what shift will
     * give us that.
     */

    bits = mp_count_bits(a);
    if (bits > DBL_MAX_EXP*log2FLT_RADIX) {
	errno = ERANGE;
	if (a->sign == MP_ZPOS) {
	    return HUGE_VAL;
	} else {
	    return -HUGE_VAL;
	}
    }
    shift = mantBits - bits;

    /*
     * If shift > 0, shift the significand left by the requisite number of
     * bits.  If shift == 0, the significand is already exactly 'mantBits'
     * in length.  If shift < 0, we will need to shift the significand right
     * by the requisite number of bits, and round it. If the '1-shift'
     * least significant bits are 0, but the 'shift'th bit is nonzero,
     * then the significand lies exactly between two values and must be
     * 'rounded to even'.
     */

    mp_init(&b);
    if (shift == 0) {
	mp_copy(a, &b);
    } else if (shift > 0) {
	mp_mul_2d(a, shift, &b);
    } else if (shift < 0) {
	lsb = mp_cnt_lsb(a);
	if (lsb == -1-shift) {

	    /*
	     * Round to even
	     */

	    mp_div_2d(a, -shift, &b, NULL);
	    if (mp_isodd(&b)) {
		if (b.sign == MP_ZPOS) {
		    mp_add_d(&b, 1, &b);
		} else {
		    mp_sub_d(&b, 1, &b);
		}
	    }
	} else {

	    /*
	     * Ordinary rounding
	     */

	    mp_div_2d(a, -1-shift, &b, NULL);
	    if (b.sign == MP_ZPOS) {
		mp_add_d(&b, 1, &b);
	    } else {
		mp_sub_d(&b, 1, &b);
	    }
	    mp_div_2d(&b, 1, &b, NULL);
	}
    }

    /*
     * Accumulate the result, one mp_digit at a time.
     */

    r = 0.0;
    for (i=b.used-1 ; i>=0 ; --i) {
	r = ldexp(r, DIGIT_BIT) + b.dp[i];
    }
    mp_clear(&b);

    /*
     * Scale the result to the correct number of bits.
     */

    r = ldexp(r, bits - mantBits);

    /*
     * Return the result with the appropriate sign.
     */

    if (a->sign == MP_ZPOS) {
	return r;
    } else {
	return -r;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclCeil --
 *
 *	Computes the smallest floating point number that is at least the
 *	mp_int argument.
 *
 * Results:
 *	Returns the floating point number.
 *
 *----------------------------------------------------------------------
 */

double
TclCeil(
    const mp_int *a)			/* Integer to convert. */
{
    double r = 0.0;
    mp_int b;

    mp_init(&b);
    if (mp_cmp_d(a, 0) == MP_LT) {
	mp_neg(a, &b);
	r = -TclFloor(&b);
    } else {
	int bits = mp_count_bits(a);

	if (bits > DBL_MAX_EXP*log2FLT_RADIX) {
	    r = HUGE_VAL;
	} else {
	    int i, exact = 1, shift = mantBits - bits;

	    if (shift > 0) {
		mp_mul_2d(a, shift, &b);
	    } else if (shift < 0) {
		mp_int d;
		mp_init(&d);
		mp_div_2d(a, -shift, &b, &d);
		exact = mp_iszero(&d);
		mp_clear(&d);
	    } else {
		mp_copy(a, &b);
	    }
	    if (!exact) {
		mp_add_d(&b, 1, &b);
	    }
	    for (i=b.used-1 ; i>=0 ; --i) {
		r = ldexp(r, DIGIT_BIT) + b.dp[i];
	    }
	    r = ldexp(r, bits - mantBits);
	}
    }
    mp_clear(&b);
    return r;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFloor --
 *
 *	Computes the largest floating point number less than or equal to the
 *	mp_int argument.
 *
 * Results:
 *	Returns the floating point value.
 *
 *----------------------------------------------------------------------
 */

double
TclFloor(
    const mp_int *a)			/* Integer to convert. */
{
    double r = 0.0;
    mp_int b;

    mp_init(&b);
    if (mp_cmp_d(a, 0) == MP_LT) {
	mp_neg(a, &b);
	r = -TclCeil(&b);
    } else {
	int bits = mp_count_bits(a);

	if (bits > DBL_MAX_EXP*log2FLT_RADIX) {
	    r = DBL_MAX;
	} else {
	    int i, shift = mantBits - bits;

	    if (shift > 0) {
		mp_mul_2d(a, shift, &b);
	    } else if (shift < 0) {
		mp_div_2d(a, -shift, &b, NULL);
	    } else {
		mp_copy(a, &b);
	    }
	    for (i=b.used-1 ; i>=0 ; --i) {
		r = ldexp(r, DIGIT_BIT) + b.dp[i];
	    }
	    r = ldexp(r, bits - mantBits);
	}
    }
    mp_clear(&b);
    return r;
}

/*
 *----------------------------------------------------------------------
 *
 * BignumToBiasedFrExp --
 *
 *	Convert an arbitrary-precision integer to a native floating point
 *	number in the range [0.5,1) times a power of two. NOTE: Intentionally
 *	converts to a number that's a few ulp too small, so that
 *	RefineApproximation will not overflow near the high end of the
 *	machine's arithmetic range.
 *
 * Results:
 *	Returns the converted number.
 *
 * Side effects:
 *	Stores the exponent of two in 'machexp'.
 *
 *----------------------------------------------------------------------
 */

static double
BignumToBiasedFrExp(
    const mp_int *a,		/* Integer to convert. */
    int *machexp)		/* Power of two. */
{
    mp_int b;
    int bits;
    int shift;
    int i;
    double r;

    /*
     * Determine how many bits we need, and extract that many from the input.
     * Round to nearest unit in the last place.
     */

    bits = mp_count_bits(a);
    shift = mantBits - 2 - bits;
    mp_init(&b);
    if (shift > 0) {
	mp_mul_2d(a, shift, &b);
    } else if (shift < 0) {
	mp_div_2d(a, -shift, &b, NULL);
    } else {
	mp_copy(a, &b);
    }

    /*
     * Accumulate the result, one mp_digit at a time.
     */

    r = 0.0;
    for (i=b.used-1; i>=0; --i) {
	r = ldexp(r, DIGIT_BIT) + b.dp[i];
    }
    mp_clear(&b);

    /*
     * Return the result with the appropriate sign.
     */

    *machexp = bits - mantBits + 2;
    return ((a->sign == MP_ZPOS) ? r : -r);
}

/*
 *----------------------------------------------------------------------
 *
 * Pow10TimesFrExp --
 *
 *	Multiply a power of ten by a number expressed as fraction and
 *	exponent.
 *
 * Results:
 *	Returns the significand of the result.
 *
 * Side effects:
 *	Overwrites the 'machexp' parameter with the exponent of the result.
 *
 * Assumes that 'exponent' is such that 10**exponent would be a double, even
 * though 'fraction*10**(machexp+exponent)' might overflow.
 *
 *----------------------------------------------------------------------
 */

static double
Pow10TimesFrExp(
    int exponent,		/* Power of 10 to multiply by. */
    double fraction,		/* Significand of multiplicand. */
    int *machexp)		/* On input, exponent of multiplicand. On
				 * output, exponent of result. */
{
    int i, j;
    int expt = *machexp;
    double retval = fraction;

    if (exponent > 0) {
	/*
	 * Multiply by 10**exponent.
	 */

	retval = frexp(retval * pow10vals[exponent&0xf], &j);
	expt += j;
	for (i=4; i<9; ++i) {
	    if (exponent & (1<<i)) {
		retval = frexp(retval * pow_10_2_n[i], &j);
		expt += j;
	    }
	}
    } else if (exponent < 0) {
	/*
	 * Divide by 10**-exponent.
	 */

	retval = frexp(retval / pow10vals[(-exponent) & 0xf], &j);
	expt += j;
	for (i=4; i<9; ++i) {
	    if ((-exponent) & (1<<i)) {
		retval = frexp(retval / pow_10_2_n[i], &j);
		expt += j;
	    }
	}
    }

    *machexp = expt;
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * SafeLdExp --
 *
 *	Do an 'ldexp' operation, but handle denormals gracefully.
 *
 * Results:
 *	Returns the appropriately scaled value.
 *
 *	On some platforms, 'ldexp' fails when presented with a number too
 *	small to represent as a normalized double. This routine does 'ldexp'
 *	in two steps for those numbers, to return correctly denormalized
 *	values.
 *
 *----------------------------------------------------------------------
 */

static double
SafeLdExp(
    double fract,
    int expt)
{
    int minexpt = DBL_MIN_EXP * log2FLT_RADIX;
    volatile double a, b, retval;

    if (expt < minexpt) {
	a = ldexp(fract, expt - mantBits - minexpt);
	b = ldexp(1.0, mantBits + minexpt);
	retval = a * b;
    } else {
	retval = ldexp(fract, expt);
    }
    return retval;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFormatNaN --
 *
 *	Makes the string representation of a "Not a Number"
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Stores the string representation in the supplied buffer, which must be
 *	at least TCL_DOUBLE_SPACE characters.
 *
 *----------------------------------------------------------------------
 */

void
TclFormatNaN(
    double value,		/* The Not-a-Number to format. */
    char *buffer)		/* String representation. */
{
#ifndef IEEE_FLOATING_POINT
    strcpy(buffer, "NaN");
    return;
#else
    union {
	double dv;
	Tcl_WideUInt iv;
    } bitwhack;

    bitwhack.dv = value;
    if (n770_fp) {
	bitwhack.iv = Nokia770Twiddle(bitwhack.iv);
    }
    if (bitwhack.iv & ((Tcl_WideUInt) 1 << 63)) {
	bitwhack.iv &= ~ ((Tcl_WideUInt) 1 << 63);
	*buffer++ = '-';
    }
    *buffer++ = 'N';
    *buffer++ = 'a';
    *buffer++ = 'N';
    bitwhack.iv &= (((Tcl_WideUInt) 1) << 51) - 1;
    if (bitwhack.iv != 0) {
	sprintf(buffer, "(%" TCL_LL_MODIFIER "x)", bitwhack.iv);
    } else {
	*buffer = '\0';
    }
#endif /* IEEE_FLOATING_POINT */
}

/*
 *----------------------------------------------------------------------
 *
 * Nokia770Twiddle --
 *
 *	Transpose the two words of a number for Nokia 770 floating point
 *	handling.
 *
 *----------------------------------------------------------------------
 */
#ifdef IEEE_FLOATING_POINT
static Tcl_WideUInt
Nokia770Twiddle(
    Tcl_WideUInt w)		/* Number to transpose. */
{
    return (((w >> 32) & 0xffffffff) | (w << 32));
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * TclNokia770Doubles --
 *
 *	Transpose the two words of a number for Nokia 770 floating point
 *	handling.
 *
 *----------------------------------------------------------------------
 */

int
TclNokia770Doubles(void)
{
    return n770_fp;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
