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

/* MPZ module */

/* This module provides an interface to an alternate Multi-Precision
   library, GNU MP in this case */

/* XXX note: everywhere where mpz_size is called,
   sizeof (limb) == sizeof (long)  has been assumed. */
   

/* MPZ objects */

#include "allobjects.h"
#include "modsupport.h"		/* For getargs() etc. */
#include "rename1.h"
#include <assert.h>
#include <sys/types.h>		/* For size_t */

/*
**	These are the cpp-flags used in this file...
**
**
** MPZ_MDIV_BUG		works around the mpz_m{div,mod,...} routines.
** 			This bug has been fixed in a later release of
** 			GMP.
** 
** MPZ_GET_STR_BUG	mpz_get_str corrupts memory, seems to be fixed
** 			in a later release
** 
** MPZ_DEBUG		generates a bunch of diagnostic messages
** 
** MPZ_SPARE_MALLOC	if set, results in extra code that tries to
** 			minimize the creation of extra objects.
** 
** MPZ_TEST_DIV		extra diagnostic output on stderr, when division
** 			routines are involved
** 
** MPZ_LIB_DOES_CHECKING	if set, assumes that mpz library doesn't call
** 			alloca with arg < 0 (when casted to a signed
** 			integral type).
** 
** MPZ_CONVERSIONS_AS_METHODS	if set, presents the conversions as
** 			methods. e.g., `mpz(5).long() == 5L'
** 			Later, Guido provided an interface to the
** 			standard functions. So this flag has no been
** 			cleared, and `long(mpz(5)) == 5L'
** 
** MP_TEST_ALLOC	If set, you would discover why MPZ_GET_STR_BUG
**			is needed
** 
** MAKEDUMMYINT		Must be set if dynamic linking will be used
*/


/*
** IMHO, mpz_m{div,mod,divmod}() do the wrong things when the denominator < 0
** I assume that this will be fixed in a future release
*/
/*#define MPZ_MDIV_BUG fixed the (for me) nexessary parts in libgmp.a */
/*
** IMO, mpz_get_str() assumes a bit too large target space, if he doesn't
** allocate it himself
*/
#define MPZ_GET_STR_BUG

#include "gmp.h"
typedef struct {
	OB_HEAD
        MP_INT	mpz;		/* the actual number */
} mpzobject;

staticforward typeobject MPZtype;

#define is_mpzobject(v)		((v)->ob_type == &MPZtype)

static const char initialiser_name[] = "mpz";

/* #define MPZ_DEBUG */

static mpzobject *
newmpzobject()
{
	mpzobject *mpzp;


#ifdef MPZ_DEBUG
	fputs( "mpz_object() called...\n", stderr );
#endif /* def MPZ_DEBUG */
	mpzp = NEWOBJ(mpzobject, &MPZtype);
	if (mpzp == NULL)
		return NULL;

	mpz_init(&mpzp->mpz);	/* actual initialisation */
	return mpzp;
} /* newmpzobject() */

#ifdef MPZ_GET_STR_BUG
#include "gmp-impl.h"
#include "longlong.h"
#endif /* def MPZ_GET_STR_BUG */

static object *
mpz_format(objp, base, withname)
	object *objp;
	int base;
	unsigned char withname;
{
	mpzobject *mpzp = (mpzobject *)objp;
	stringobject *strobjp;
	int i;
	int cmpres;
	int taglong;
	char *cp;
	char prefix[5], *tcp;


	tcp = &prefix[0];

	if (mpzp == NULL || !is_mpzobject(mpzp)) {
		err_badcall();
		return NULL;
	}

	assert(base >= 2 && base <= 36);

	if (withname)
		i = strlen(initialiser_name) + 2; /* e.g. 'mpz(' + ')' */
	else
		i = 0;

	if ((cmpres = mpz_cmp_si(&mpzp->mpz, 0L)) == 0)
		base = 10;	/* '0' in every base, right */
	else if (cmpres < 0) {
		*tcp++ = '-';
		i += 1;		/* space to hold '-' */
	}

#ifdef MPZ_DEBUG
	fprintf(stderr, "mpz_format: mpz_sizeinbase %d\n",
		(int)mpz_sizeinbase(&mpzp->mpz, base));
#endif /* def MPZ_DEBUG */
#ifdef MPZ_GET_STR_BUG
	i += ((size_t) abs(mpzp->mpz.size) * BITS_PER_MP_LIMB
	      * __mp_bases[base].chars_per_bit_exactly) + 1;
#else /* def MPZ_GET_STR_BUG */
	i += (int)mpz_sizeinbase(&mpzp->mpz, base);
#endif /* def MPZ_GET_STR_BUG else */

	if (base == 16) {
		*tcp++ = '0';
		*tcp++ = 'x';
		i += 2;		/* space to hold '0x' */
	}
	else if (base == 8) {
		*tcp++ = '0';
		i += 1;		/* space to hold the extra '0' */
	}
	else if (base > 10) {
		*tcp++ = '0' + base / 10;
		*tcp++ = '0' + base % 10;
		*tcp++ = '#';
		i += 3;		/* space to hold e.g. '12#' */
	}
	else if (base < 10) {
		*tcp++ = '0' + base;
		*tcp++ = '#';
		i += 2;		/* space to hold e.g. '6#' */
	}

	/*
	** the following code looks if we need a 'L' attached to the number
	** it will also attach an 'L' to the value -0x80000000
	*/
	taglong = 0;
	if (mpz_size(&mpzp->mpz) > 1
	    || (long)mpz_get_ui(&mpzp->mpz) < 0L) {
		taglong = 1;
		i += 1;		/* space to hold 'L' */
	}

#ifdef MPZ_DEBUG
	fprintf(stderr, "mpz_format: requesting string size %d\n", i);
#endif /* def MPZ_DEBUG */	
	if ((strobjp = (stringobject *)newsizedstringobject((char *)0, i))
	    == NULL)
		return NULL;

	/* get the beginning of the string memory and start copying things */
	cp = GETSTRINGVALUE(strobjp);
	if (withname) {
		strcpy(cp, initialiser_name);
		cp += strlen(initialiser_name);
		*cp++ = '('; /*')'*/
	}

	/* copy the already prepared prefix; e.g. sign and base indicator */
	*tcp = '\0';
	strcpy(cp, prefix);
	cp += tcp - prefix;

	/* since' we have the sign already, let the lib think it's a positive
		number */
	if (cmpres < 0)
		mpz_neg(&mpzp->mpz,&mpzp->mpz);	/* hack Hack HAck HACk HACK */
	(void)mpz_get_str(cp, base, &mpzp->mpz);
	if (cmpres < 0)
		mpz_neg(&mpzp->mpz,&mpzp->mpz);	/* hack Hack HAck HACk HACK */
#ifdef MPZ_DEBUG
	fprintf(stderr, "mpz_format: base (ultim) %d, mpz_get_str: %s\n",
		base, cp);
#endif /* def MPZ_DEBUG */
	cp += strlen(cp);

	if (taglong)
		*cp++ = 'L';
	if (withname)
		*cp++ = /*'('*/ ')';

	*cp = '\0';

#ifdef MPZ_DEBUG
	fprintf(stderr,
		"mpz_format: cp (str end) 0x%x, begin 0x%x, diff %d, i %d\n",
		cp, GETSTRINGVALUE(strobjp), cp - GETSTRINGVALUE(strobjp), i);
#endif /* def MPZ_DEBUG */	
	assert(cp - GETSTRINGVALUE(strobjp) <= i);

	if (cp - GETSTRINGVALUE(strobjp) != i) {
		strobjp->ob_size -= i - (cp - GETSTRINGVALUE(strobjp));
	}

	return (object *)strobjp;
} /* mpz_format() */

/* MPZ methods */

static void
mpz_dealloc(mpzp)
	mpzobject *mpzp;
{
#ifdef MPZ_DEBUG
	fputs( "mpz_dealloc() called...\n", stderr );
#endif /* def MPZ_DEBUG */
	mpz_clear(&mpzp->mpz);
	DEL(mpzp);
} /* mpz_dealloc() */


/* pointers to frequently used values 0, 1 and -1 */
static mpzobject *mpz_value_zero, *mpz_value_one, *mpz_value_mone;

static int
mpz_compare(a, b)
	mpzobject *a, *b;
{
	int cmpres;


	/* guido sez it's better to return -1, 0 or 1 */
	return (cmpres = mpz_cmp( &a->mpz, &b->mpz )) == 0 ? 0
		: cmpres > 0 ? 1 : -1;
} /* mpz_compare() */

static object *
mpz_addition(a, b)
	mpzobject *a;
	mpzobject *b;
{
	mpzobject *z;

	
#ifdef MPZ_SPARE_MALLOC
	if (mpz_cmp_ui(&a->mpz, (unsigned long int)0) == 0) {
		INCREF(b);
		return (object *)b;
	}

	if (mpz_cmp_ui(&b->mpz, (unsigned long int)0) == 0) {
		INCREF(a);
		return (object *)a;
	}
#endif /* def MPZ_SPARE_MALLOC */

	if ((z = newmpzobject()) == NULL)
		return NULL;
	
	mpz_add(&z->mpz, &a->mpz, &b->mpz);
	return (object *)z;
} /* mpz_addition() */

static object *
mpz_substract(a, b)
	mpzobject *a;
	mpzobject *b;
{
	mpzobject *z;

	
#ifdef MPZ_SPARE_MALLOC
	if (mpz_cmp_ui(&b->mpz, (unsigned long int)0) == 0) {
		INCREF(a);
		return (object *)a;
	}
#endif /* MPZ_SPARE_MALLOC */	

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_sub(&z->mpz, &a->mpz, &b->mpz);
	return (object *)z;
} /* mpz_substract() */

static object *
mpz_multiply(a, b)
	mpzobject *a;
	mpzobject *b;
{
#ifdef MPZ_SPARE_MALLOC
	int cmpres;
#endif /* def MPZ_SPARE_MALLOC */
	mpzobject *z;


#ifdef MPZ_SPARE_MALLOC
	if ((cmpres = mpz_cmp_ui(&a->mpz, (unsigned long int)0)) == 0) {
		INCREF(mpz_value_zero);
		return (object *)mpz_value_zero;
	}
	if (cmpres > 0 && mpz_cmp_ui(&a->mpz, (unsigned long int)1) == 0) {
		INCREF(b);
		return (object *)b;
	}

	if ((cmpres = mpz_cmp_ui(&b->mpz, (unsigned long_int)0)) == 0) {
		INCREF(mpz_value_zero);
		return (object *)mpz_value_zero;
	}
	if (cmpres > 0 && mpz_cmp_ui(&b->mpz, (unsigned long int)1) == 0) {
		INCREF(a);
		return (object *)a;
	}
#endif /* MPZ_SPARE_MALLOC */

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_mul( &z->mpz, &a->mpz, &b->mpz );
	return (object *)z;
	
} /* mpz_multiply() */

static object *
mpz_divide(a, b)
	mpzobject *a;
	mpzobject *b;
{
#ifdef MPZ_SPARE_MALLOC
	int cmpres;
#endif /* def MPZ_SPARE_MALLOC */
	mpzobject *z;


	if ((
#ifdef MPZ_SPARE_MALLOC
	     cmpres =
#endif /* def MPZ_SPARE_MALLOC */
	     mpz_cmp_ui(&b->mpz, (unsigned long int)0)) == 0) {
		err_setstr(ZeroDivisionError, "mpz./ by zero");
		return NULL;
	}
#ifdef MPZ_SPARE_MALLOC
	if (cmpres > 0 && mpz_cmp_ui(&b->mpz(unsigned long int)1) == 0) {
		INCREF(a);
		return (object *)a;
	}
#endif /* def MPZ_SPARE_MALLOC */

	if ((z = newmpzobject()) == NULL)
		return NULL;

#ifdef MPZ_TEST_DIV
	fputs("mpz_divide:  div result", stderr);
	mpz_div(&z->mpz, &a->mpz, &b->mpz);
	mpz_out_str(stderr, 10, &z->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */
#ifdef MPZ_MDIV_BUG
	if ((mpz_cmp_ui(&a->mpz, (unsigned long int)0) < 0)
	    != (mpz_cmp_ui(&b->mpz, (unsigned long int)0) < 0)) {
		/*
		** numerator has other sign than denominator: we have
		** to look at the remainder for a correction, since mpz_mdiv
		** also calls mpz_divmod, I can as well do it myself
		*/
		MP_INT tmpmpz;


		mpz_init(&tmpmpz);
		mpz_divmod(&z->mpz, &tmpmpz, &a->mpz, &b->mpz);

		if (mpz_cmp_ui(&tmpmpz, (unsigned long int)0) != 0)
			mpz_sub_ui(&z->mpz, &z->mpz, (unsigned long int)1);

		mpz_clear(&tmpmpz);
	}
	else
		mpz_div(&z->mpz, &a->mpz, &b->mpz);
		/* the ``naive'' implementation does it right for operands
		   having the same sign */

#else /* def MPZ_MDIV_BUG */
	mpz_mdiv(&z->mpz, &a->mpz, &b->mpz);
#endif /* def MPZ_MDIV_BUG else */
#ifdef MPZ_TEST_DIV
	fputs("mpz_divide: mdiv result", stderr);
	mpz_out_str(stderr, 10, &z->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */
	return (object *)z;
	
} /* mpz_divide() */

static object *
mpz_remainder(a, b)
	mpzobject *a;
	mpzobject *b;
{
#ifdef MPZ_SPARE_MALLOC
	int cmpres;
#endif /* def MPZ_SPARE_MALLOC */	
	mpzobject *z;

	
	if ((
#ifdef MPZ_SPARE_MALLOC	     
	     cmpres =
#endif /* def MPZ_SPARE_MALLOC */	
	     mpz_cmp_ui(&b->mpz, (unsigned long int)0)) == 0) {
		err_setstr(ZeroDivisionError, "mpz.% by zero");
		return NULL;
	}
#ifdef MPZ_SPARE_MALLOC
	if (cmpres > 0) {
		if ((cmpres = mpz_cmp_ui(&b->mpz, (unsigned long int)2)) == 0) {
			INCREF(mpz_value_one);
			return (object *)mpz_value_one;
		}
		if (cmpres < 0) {
			/* b must be 1 now */
			INCREF(mpz_value_zero);
			return (object *)mpz_value_zero;
		}
	}
#endif /* def MPZ_SPARE_MALLOC */	

	if ((z = newmpzobject()) == NULL)
		return NULL;

#ifdef MPZ_TEST_DIV
	fputs("mpz_remain:  mod result", stderr);
	mpz_mod(&z->mpz, &a->mpz, &b->mpz);
	mpz_out_str(stderr, 10, &z->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */
#ifdef MPZ_MDIV_BUG

	/* the ``naive'' implementation does it right for operands
	   having the same sign */
	mpz_mod(&z->mpz, &a->mpz, &b->mpz);

	/* assumption: z, a and b all point to different locations */
	if ((mpz_cmp_ui(&a->mpz, (unsigned long int)0) < 0)
	    != (mpz_cmp_ui(&b->mpz, (unsigned long int)0) < 0)
	    && mpz_cmp_ui(&z->mpz, (unsigned long int)0) != 0)
		mpz_add(&z->mpz, &z->mpz, &b->mpz);
		/*
		** numerator has other sign than denominator: we have
		** to look at the remainder for a correction, since mpz_mdiv
		** also calls mpz_divmod, I can as well do it myself
		*/
#else /* def MPZ_MDIV_BUG */
	mpz_mmod(&z->mpz, &a->mpz, &b->mpz);
#endif /* def MPZ_MDIV_BUG else */
#ifdef MPZ_TEST_DIV
	fputs("mpz_remain: mmod result", stderr);
	mpz_out_str(stderr, 10, &z->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */
	return (object *)z;
	
} /* mpz_remainder() */

static object *
mpz_div_and_mod(a, b)
	mpzobject *a;
	mpzobject *b;
{
	object *z = NULL;
	mpzobject *x = NULL, *y = NULL;


	if (mpz_cmp_ui(&b->mpz, (unsigned long int)0) == 0) {
		err_setstr(ZeroDivisionError, "mpz.divmod by zero");
		return NULL;
	}

	if ((z = newtupleobject(2)) == NULL
	    || (x = newmpzobject()) == NULL
	    || (y = newmpzobject()) == NULL) {
		XDECREF(z);
		XDECREF(x);
		XDECREF(y);
		return NULL;
	}

#ifdef MPZ_TEST_DIV
	fputs("mpz_divmod:  dm  result", stderr);
	mpz_divmod(&x->mpz, &y->mpz, &a->mpz, &b->mpz);
	mpz_out_str(stderr, 10, &x->mpz);
	putc('\n', stderr);
	mpz_out_str(stderr, 10, &y->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */
#ifdef MPZ_MDIV_BUG
	mpz_divmod(&x->mpz, &y->mpz, &a->mpz, &b->mpz);
	if ((mpz_cmp_ui(&a->mpz, (unsigned long int)0) < 0)
	    != (mpz_cmp_ui(&b->mpz, (unsigned long int)0) < 0)
	    && mpz_cmp_ui(&y->mpz, (unsigned long int)0) != 0) {
		/*
		** numerator has other sign than denominator: we have
		** to look at the remainder for a correction.
		*/
		mpz_add(&y->mpz, &y->mpz, &b->mpz);
		mpz_sub_ui(&x->mpz, &x->mpz, (unsigned long int)1);
	}
#else /* def MPZ_MDIV_BUG */
	mpz_mdivmod( &x->mpz, &y->mpz, &a->mpz, &b->mpz );
#endif /* def MPZ_MDIV_BUG else */
#ifdef MPZ_TEST_DIV
	fputs("mpz_divmod: mdm  result", stderr);
	mpz_out_str(stderr, 10, &x->mpz);
	putc('\n', stderr);
	mpz_out_str(stderr, 10, &y->mpz);
	putc('\n', stderr);
#endif /* def MPZ_TEST_DIV */

	(void)settupleitem(z, 0, (object *)x);
	(void)settupleitem(z, 1, (object *)y);
	
	return z;
} /* mpz_div_and_mod() */

static object *
mpz_power(a, b, m)
	mpzobject *a;
	mpzobject *b;
        mpzobject *m;
{
	mpzobject *z;
	int cmpres;
	long int longtmp1, longtmp2;

 	if ((object *)m!=Py_None)
 	  {
 	    mpzobject *z2;
	    INCREF(Py_None);
 	    z=mpz_power(a, b, (mpzobject *)Py_None);
	    DECREF(Py_None);
 	    if (z==NULL) return(z);
 	    z2=mpz_remainder(z, m);
 	    DECREF(z);
 	    return((object *)z2);
 	  }	    

	if ((cmpres = mpz_cmp_ui(&b->mpz, (unsigned long int)0)) == 0) {
		/* the gnu-mp lib sets pow(0,0) to 0, we to 1 */

		INCREF(mpz_value_one);
		return (object *)mpz_value_one;
	}
		
	if (cmpres < 0) {
		err_setstr(ValueError, "mpz.pow to negative exponent");
		return NULL;
	}

	if ((cmpres = mpz_cmp_ui(&a->mpz, (unsigned long int)0)) == 0) {
		/* the base is 0 */

		INCREF(mpz_value_zero);
		return (object *)mpz_value_zero;
	}
	else if (cmpres > 0
		 && mpz_cmp_ui(&a->mpz, (unsigned long int)1) == 0) {
		/* the base is 1 */

		INCREF(mpz_value_one);
		return (object *)mpz_value_one;
	}
	else if (cmpres < 0
		 && mpz_cmp_si(&a->mpz, (long int)-1) == 0) {

		MP_INT tmpmpz;
		/* the base is -1: pow(-1, any) == 1,-1 for even,uneven b */
		/* XXX this code needs to be optimized: what's better?
		   mpz_mmod_ui or mpz_mod_2exp, I choose for the latter
		   for *un*obvious reasons */

		/* is the exponent even? */
		mpz_init(&tmpmpz);

		/* look to the remainder after a division by (1 << 1) */
		mpz_mod_2exp(&tmpmpz, &b->mpz, (unsigned long int)1);

		if (mpz_cmp_ui(&tmpmpz, (unsigned int)0) == 0) {
			mpz_clear(&tmpmpz);
			INCREF(mpz_value_one);
			return (object *)mpz_value_one;
		}
		mpz_clear(&tmpmpz);
		INCREF(mpz_value_mone);
		return (object *)mpz_value_mone;
	}

#ifdef MPZ_LIB_DOES_CHECKING
	/* check if it's doable: sizeof(exp) > sizeof(long) &&
	   abs(base) > 1 ?? --> No Way */
	if (mpz_size(&b->mpz) > 1)
		return (object *)err_nomem();
#else /* def MPZ_LIB_DOES_CHECKING */
	/* wet finger method */
	if (mpz_cmp_ui(&b->mpz, (unsigned long int)0x10000) >= 0) {
		err_setstr(ValueError, "mpz.pow outrageous exponent");
		return NULL;
	}
#endif /* def MPZ_LIB_DOES_CHECKING else */

	if ((z = newmpzobject()) == NULL)
		return NULL;
	
	mpz_pow_ui(&z->mpz, &a->mpz, mpz_get_ui(&b->mpz));
	
	return (object *)z;
} /* mpz_power() */


static object *
mpz_negative(v)
	mpzobject *v;
{
	mpzobject *z;

	
#ifdef MPZ_SPARE_MALLOC
	if (mpz_cmp_ui(&v->mpz, (unsigned long int)0) == 0) {
		/* -0 == 0 */
		INCREF(v);
		return (object *)v;
	}
#endif /* def MPZ_SPARE_MALLOC */

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_neg(&z->mpz, &v->mpz);
	return (object *)z;
} /* mpz_negative() */


static object *
mpz_positive(v)
	mpzobject *v;
{
	INCREF(v);
	return (object *)v;
} /* mpz_positive() */


static object *
mpz_absolute(v)
	mpzobject *v;
{
	mpzobject *z;

	
	if (mpz_cmp_ui(&v->mpz, (unsigned long int)0) >= 0) {
		INCREF(v);
		return (object *)v;
	}

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_neg(&z->mpz, &v->mpz);
	return (object *)z;
} /* mpz_absolute() */

static int
mpz_nonzero(v)
	mpzobject *v;
{
	return mpz_cmp_ui(&v->mpz, (unsigned long int)0) != 0;
} /* mpz_nonzero() */
		
static object *
mpz_invert(v)
	mpzobject *v;
{
	mpzobject *z;


	/* I think mpz_com does exactly what needed */
	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_com(&z->mpz, &v->mpz);
	return (object *)z;
} /* mpz_invert() */

static object *
mpz_lshift(a, b)
	mpzobject *a;
	mpzobject *b;
{
	int cmpres;
	mpzobject *z;


	if ((cmpres = mpz_cmp_ui(&b->mpz, (unsigned long int)0)) == 0) {
		/* a << 0 == a */
		INCREF(a);
		return (object *)a;
	}

	if (cmpres < 0) {
		err_setstr(ValueError, "mpz.<< negative shift count");
		return NULL;
	}

#ifdef MPZ_LIB_DOES_CHECKING
	if (mpz_size(&b->mpz) > 1)
		return (object *)err_nomem();
#else /* def MPZ_LIB_DOES_CHECKING */
	/* wet finger method */
	if (mpz_cmp_ui(&b->mpz, (unsigned long int)0x10000) >= 0) {
		err_setstr(ValueError, "mpz.<< outrageous shift count");
		return NULL;
	}
#endif /* def MPZ_LIB_DOES_CHECKING else */

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_mul_2exp(&z->mpz, &a->mpz, mpz_get_ui(&b->mpz));
	return (object *)z;
} /* mpz_lshift() */

static object *
mpz_rshift(a, b)
	mpzobject *a;
	mpzobject *b;
{
	int cmpres;
	mpzobject *z;


	if ((cmpres = mpz_cmp_ui(&b->mpz, (unsigned long int)0)) == 0) {
		/* a >> 0 == a */
		INCREF(a);
		return (object *)a;
	}

	if (cmpres < 0) {
		err_setstr(ValueError, "mpz.>> negative shift count");
		return NULL;
	}

	if (mpz_size(&b->mpz) > 1)
		return (object *)err_nomem();

	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_div_2exp(&z->mpz, &a->mpz, mpz_get_ui(&b->mpz));
	return (object *)z;
} /* mpz_rshift() */

static object *
mpz_andfunc(a, b)
	mpzobject *a;
	mpzobject *b;
{
	mpzobject *z;


	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_and(&z->mpz, &a->mpz, &b->mpz);
	return (object *)z;
} /* mpz_andfunc() */

/* hack Hack HAck HACk HACK, XXX this code is dead slow */
void
mpz_xor(res, op1, op2)
	MP_INT *res;
	const MP_INT *op1;
	const MP_INT *op2;
{
	MP_INT tmpmpz;
	
	mpz_init(&tmpmpz);

	mpz_and(res, op1, op2);
	mpz_com(&tmpmpz, res);
	mpz_ior(res, op1, op2);
	mpz_and(res, res, &tmpmpz);

	mpz_clear(&tmpmpz);
} /* mpz_xor() HACK */

static object *
mpz_xorfunc(a, b)
	mpzobject *a;
	mpzobject *b;
{
	mpzobject *z;


	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_xor(&z->mpz, &a->mpz, &b->mpz);
	return (object *)z;
} /* mpz_xorfunc() */

static object *
mpz_orfunc(a, b)
	mpzobject *a;
	mpzobject *b;
{
	mpzobject *z;


	if ((z = newmpzobject()) == NULL)
		return NULL;

	mpz_ior(&z->mpz, &a->mpz, &b->mpz);
	return (object *)z;
} /* mpz_orfunc() */

/* MPZ initialisation */

#include "longintrepr.h"

static object *
MPZ_mpz(self, args)
	object *self;
	object *args;
{
	mpzobject *mpzp;
	object *objp;


#ifdef MPZ_DEBUG
	fputs("MPZ_mpz() called...\n", stderr);
#endif /* def MPZ_DEBUG */

	if (!getargs(args, "O", &objp))
		return NULL;

	/* at least we know it's some object */
	/* note DON't DECREF args NEITHER objp */

	if (is_intobject(objp)) {
		long lval;

		if (!getargs(objp, "l", &lval))
			return NULL;
		
		if (lval == (long)0) {
			INCREF(mpz_value_zero);
			mpzp = mpz_value_zero;
		}
		else if (lval == (long)1) {
			INCREF(mpz_value_one);
			mpzp = mpz_value_one;
		}			
		else if ((mpzp = newmpzobject()) == NULL)
			return NULL;
		else mpz_set_si(&mpzp->mpz, lval);
	}
	else if (is_longobject(objp)) {
		MP_INT mplongdigit;
		int i;
		unsigned char isnegative;
		

		if ((mpzp = newmpzobject()) == NULL)
			return NULL;

		mpz_set_si(&mpzp->mpz, 0L);
		mpz_init(&mplongdigit);
		
		/* how we're gonna handle this? */
		if (isnegative = ((i = ((longobject *)objp)->ob_size) < 0) )
			i = -i;

		while (i--) {
			mpz_set_ui(&mplongdigit,
				   (unsigned long)
				   ((longobject *)objp)->ob_digit[i]);
			mpz_mul_2exp(&mplongdigit,&mplongdigit,
				     (unsigned long int)i * SHIFT);
			mpz_ior(&mpzp->mpz, &mpzp->mpz, &mplongdigit);
		}

		if (isnegative)
			mpz_neg(&mpzp->mpz, &mpzp->mpz);

		/* get rid of allocation for tmp variable */
		mpz_clear(&mplongdigit);
	}
	else if (is_stringobject(objp)) {
		char *cp;
		int len;
		MP_INT mplongdigit;
		
		if (!getargs(objp, "s#", &cp, &len))
			return NULL;

		if ((mpzp = newmpzobject()) == NULL)
			return NULL;

		mpz_set_si(&mpzp->mpz, 0L);
		mpz_init(&mplongdigit);
		
		/* let's do it the same way as with the long conversion:
		   without thinking how it can be faster (-: :-) */

		cp += len;
		while (len--) {
			mpz_set_ui(&mplongdigit, (unsigned long)*--cp );
			mpz_mul_2exp(&mplongdigit,&mplongdigit,
				     (unsigned long int)len * 8);
			mpz_ior(&mpzp->mpz, &mpzp->mpz, &mplongdigit);
		}

		/* get rid of allocation for tmp variable */
		mpz_clear(&mplongdigit);
	}
	else if (is_mpzobject(objp)) {
		INCREF(objp);
		mpzp = (mpzobject *)objp;
	}
	else {
		err_setstr(TypeError,
			   "mpz.mpz() expects integer, long, string or mpz object argument");
		return NULL;
	}


#ifdef MPZ_DEBUG
	fputs("MPZ_mpz: created mpz=", stderr);
	mpz_out_str(stderr, 10, &mpzp->mpz);
	putc('\n', stderr);
#endif /* def MPZ_DEBUG */
	return (object *)mpzp;
} /* MPZ_mpz() */

static mpzobject *
mpz_mpzcoerce(z)
	object *z;
{
	/* shortcut: 9 out of 10 times the type is already ok */
	if (is_mpzobject(z)) {
		INCREF(z);
		return (mpzobject *)z;	/* coercion succeeded */
	}

	/* what types do we accept?: intobjects and longobjects */
	if (is_intobject(z) || is_longobject(z))
		return (mpzobject *)MPZ_mpz((object *)NULL, z);

	err_setstr(TypeError, "number coercion (to mpzobject) failed");
	return NULL;
} /* mpz_mpzcoerce() */
	
/* Forward */
static void mpz_divm PROTO((MP_INT *res, const MP_INT *num,
			    const MP_INT *den, const MP_INT *mod));

static object *
MPZ_powm(self, args)
	object *self;
	object *args;
{
	object *base, *exp, *mod;
	mpzobject *mpzbase = NULL, *mpzexp = NULL, *mpzmod = NULL;
	mpzobject *z;
	int tstres;

	
	if (!getargs(args, "(OOO)", &base, &exp, &mod))
		return NULL;

	if ((mpzbase = mpz_mpzcoerce(base)) == NULL
	    || (mpzexp = mpz_mpzcoerce(exp)) == NULL
	    || (mpzmod = mpz_mpzcoerce(mod)) == NULL
	    || (z = newmpzobject()) == NULL) {
		XDECREF(mpzbase);
		XDECREF(mpzexp);
		XDECREF(mpzmod);
		return NULL;
	}

	if ((tstres=mpz_cmp_ui(&mpzexp->mpz, (unsigned long int)0)) == 0) {
		INCREF(mpz_value_one);
		return (object *)mpz_value_one;
	}

	if (tstres < 0) {
		MP_INT absexp;
		/* negative exp */

		mpz_init_set(&absexp, &mpzexp->mpz);
		mpz_abs(&absexp, &absexp);
		mpz_powm(&z->mpz, &mpzbase->mpz, &absexp, &mpzmod->mpz);

		mpz_divm(&z->mpz, &mpz_value_one->mpz, &z->mpz, &mpzmod->mpz);
		
		mpz_clear(&absexp);
	}
	else {
		mpz_powm(&z->mpz, &mpzbase->mpz, &mpzexp->mpz, &mpzmod->mpz);
	}
		
	DECREF(mpzbase);
	DECREF(mpzexp);
	DECREF(mpzmod);

	return (object *)z;
} /* MPZ_powm() */


static object *
MPZ_gcd(self, args)
	object *self;
	object *args;
{
	object *op1, *op2;
	mpzobject *mpzop1 = NULL, *mpzop2 = NULL;
	mpzobject *z;

	
	if (!getargs(args, "(OO)", &op1, &op2))
		return NULL;

	if ((mpzop1 = mpz_mpzcoerce(op1)) == NULL
	    || (mpzop2 = mpz_mpzcoerce(op2)) == NULL
	    || (z = newmpzobject()) == NULL) {
		XDECREF(mpzop1);
		XDECREF(mpzop2);
		return NULL;
	}

	/* ok, we have three mpzobjects, and an initialised result holder */
	mpz_gcd(&z->mpz, &mpzop1->mpz, &mpzop2->mpz);

	DECREF(mpzop1);
	DECREF(mpzop2);

	return (object *)z;
} /* MPZ_gcd() */


static object *
MPZ_gcdext(self, args)
	object *self;
	object *args;
{
	object *op1, *op2, *z = NULL;
	mpzobject *mpzop1 = NULL, *mpzop2 = NULL;
	mpzobject *g = NULL, *s = NULL, *t = NULL;

	
	if (!getargs(args, "(OO)", &op1, &op2))
		return NULL;

	if ((mpzop1 = mpz_mpzcoerce(op1)) == NULL
	    || (mpzop2 = mpz_mpzcoerce(op2)) == NULL
	    || (z = newtupleobject(3)) == NULL
	    || (g = newmpzobject()) == NULL
	    || (s = newmpzobject()) == NULL
	    || (t = newmpzobject()) == NULL) {
		XDECREF(mpzop1);
		XDECREF(mpzop2);
		XDECREF(z);
		XDECREF(g);
		XDECREF(s);
		/*XDECREF(t);*/
		return NULL;
	}

	mpz_gcdext(&g->mpz, &s->mpz, &t->mpz, &mpzop1->mpz, &mpzop2->mpz);

	DECREF(mpzop1);
	DECREF(mpzop2);

	(void)settupleitem(z, 0, (object *)g);
	(void)settupleitem(z, 1, (object *)s);
	(void)settupleitem(z, 2, (object *)t);

	return (object *)z;
} /* MPZ_gcdext() */


static object *
MPZ_sqrt(self, args)
	object *self;
	object *args;
{
	object *op;
	mpzobject *mpzop = NULL;
	mpzobject *z;

	
	if (!getargs(args, "O", &op))
		return NULL;

	if ((mpzop = mpz_mpzcoerce(op)) == NULL
	    || (z = newmpzobject()) == NULL) {
		XDECREF(mpzop);
		return NULL;
	}

	mpz_sqrt(&z->mpz, &mpzop->mpz);

	DECREF(mpzop);

	return (object *)z;
} /* MPZ_sqrt() */


static object *
MPZ_sqrtrem(self, args)
	object *self;
	object *args;
{
	object *op, *z = NULL;
	mpzobject *mpzop = NULL;
	mpzobject *root = NULL, *rem = NULL;

	
	if (!getargs(args, "O", &op))
		return NULL;

	if ((mpzop = mpz_mpzcoerce(op)) == NULL
	    || (z = newtupleobject(2)) == NULL
	    || (root = newmpzobject()) == NULL
	    || (rem = newmpzobject()) == NULL) {
		XDECREF(mpzop);
		XDECREF(z);
		XDECREF(root);
		/*XDECREF(rem);*/
		return NULL;
	}

	mpz_sqrtrem(&root->mpz, &rem->mpz, &mpzop->mpz);

	DECREF(mpzop);

	(void)settupleitem(z, 0, (object *)root);
	(void)settupleitem(z, 1, (object *)rem);

	return (object *)z;
} /* MPZ_sqrtrem() */


static void
#if __STDC__
mpz_divm(MP_INT *res, const MP_INT *num, const MP_INT *den, const MP_INT *mod)
#else
mpz_divm(res, num, den, mod)
	MP_INT *res;
	const MP_INT *num;
	const MP_INT *den;
	const MP_INT *mod;
#endif
{
	MP_INT s0, s1, q, r, x, d0, d1;

	mpz_init_set(&s0, num);
	mpz_init_set_ui(&s1, 0);
	mpz_init(&q);
	mpz_init(&r);
	mpz_init(&x);
	mpz_init_set(&d0, den);
	mpz_init_set(&d1, mod);

	while (d1.size != 0) {
		mpz_divmod(&q, &r, &d0, &d1);
		mpz_set(&d0, &d1);
		mpz_set(&d1, &r);

		mpz_mul(&x, &s1, &q);
		mpz_sub(&x, &s0, &x);
		mpz_set(&s0, &s1);
		mpz_set(&s1, &x);
	}

	if (d0.size != 1 || d0.d[0] != 1)
		res->size = 0;	/* trouble: the gcd != 1; set s to zero */
	else {
#ifdef MPZ_MDIV_BUG
		/* watch out here! first check the signs, and then perform
		   the mpz_mod() since mod could point to res */
		if ((s0.size < 0) != (mod->size < 0)) {
			mpz_mod(res, &s0, mod);

			if (res->size)
				mpz_add(res, res, mod);
		}
		else
			mpz_mod(res, &s0, mod);
		
#else /* def MPZ_MDIV_BUG */
		mpz_mmod(res, &s0, mod);
#endif /* def MPZ_MDIV_BUG else */
	}

	mpz_clear(&s0);
	mpz_clear(&s1);
	mpz_clear(&q);
	mpz_clear(&r);
	mpz_clear(&x);
	mpz_clear(&d0);
	mpz_clear(&d1);
} /* mpz_divm() */


static object *
MPZ_divm(self, args)
	object *self;
	object *args;
{
	object *num, *den, *mod;
	mpzobject *mpznum, *mpzden, *mpzmod = NULL;
	mpzobject *z = NULL;

	
	if (!getargs(args, "(OOO)", &num, &den, &mod))
		return NULL;

	if ((mpznum = mpz_mpzcoerce(num)) == NULL
	    || (mpzden = mpz_mpzcoerce(den)) == NULL
	    || (mpzmod = mpz_mpzcoerce(mod)) == NULL
	    || (z = newmpzobject()) == NULL ) {
		XDECREF(mpznum);
		XDECREF(mpzden);
		XDECREF(mpzmod);
		return NULL;
	}
	
	mpz_divm(&z->mpz, &mpznum->mpz, &mpzden->mpz, &mpzmod->mpz);

	DECREF(mpznum);
	DECREF(mpzden);
	DECREF(mpzmod);

	if (mpz_cmp_ui(&z->mpz, (unsigned long int)0) == 0) {
		DECREF(z);
		err_setstr(ValueError, "gcd(den, mod) != 1 or num == 0");
		return NULL;
	}

	return (object *)z;
} /* MPZ_divm() */


/* MPZ methods-as-attributes */
#ifdef MPZ_CONVERSIONS_AS_METHODS
static object *
mpz_int(self, args)
	mpzobject *self;
	object *args;
#else /* def MPZ_CONVERSIONS_AS_METHODS */
static object *
mpz_int(self)
	mpzobject *self;
#endif /* def MPZ_CONVERSIONS_AS_METHODS else */
{
	long sli;


#ifdef MPZ_CONVERSIONS_AS_METHODS
	if (!getnoarg(args))
		return NULL;
#endif /* def MPZ_CONVERSIONS_AS_METHODS */

	if (mpz_size(&self->mpz) > 1
	    || (sli = (long)mpz_get_ui(&self->mpz)) < (long)0 ) {
		err_setstr(ValueError, "mpz.int() arg too long to convert");
		return NULL;
	}

	if (mpz_cmp_ui(&self->mpz, (unsigned long)0) < 0)
		sli = -sli;

	return newintobject(sli);
} /* mpz_int() */
	
static object *
#ifdef MPZ_CONVERSIONS_AS_METHODS
mpz_long(self, args)
	mpzobject *self;
	object *args;
#else /* def MPZ_CONVERSIONS_AS_METHODS */
mpz_long(self)
	mpzobject *self;
#endif /* def MPZ_CONVERSIONS_AS_METHODS else */
{
	int i, isnegative;
	unsigned long int uli;
	longobject *longobjp;
	int ldcount;
	int bitpointer, newbitpointer;
	MP_INT mpzscratch;


#ifdef MPZ_CONVERSIONS_AS_METHODS
	if (!getnoarg(args))
		return NULL;
#endif /* def MPZ_CONVERSIONS_AS_METHODS */

	/* determine length of python-long to be allocated */
	if ((longobjp = alloclongobject(i = (int)
			    ((mpz_size(&self->mpz) * BITS_PER_MP_LIMB
			      + SHIFT - 1) /
			     SHIFT))) == NULL)
		return NULL;

	/* determine sign, and copy self to scratch var */
	mpz_init_set(&mpzscratch, &self->mpz);
	if (isnegative = (mpz_cmp_ui(&self->mpz, (unsigned long int)0) < 0))
		mpz_neg(&mpzscratch, &mpzscratch);

	/* let those bits come, let those bits go,
	   e.g. dismantle mpzscratch, build longobject */

	bitpointer = 0;		/* the number of valid bits in stock */
	newbitpointer = 0;
	ldcount = 0;		/* the python-long limb counter */
	uli = (unsigned long int)0;
	while (i--) {
		longobjp->ob_digit[ldcount] = uli & MASK;

		/* check if we've had enough bits for this digit */
		if (bitpointer < SHIFT) {
			uli = mpz_get_ui(&mpzscratch);
			longobjp->ob_digit[ldcount] |=
				(uli << bitpointer) & MASK;
			uli >>= SHIFT-bitpointer;
			bitpointer += BITS_PER_MP_LIMB;
			mpz_div_2exp(&mpzscratch, &mpzscratch,
				     BITS_PER_MP_LIMB);
		}
		else
			uli >>= SHIFT;
		bitpointer -= SHIFT;
		ldcount++;
	}

	assert(mpz_cmp_ui(&mpzscratch, (unsigned long int)0) == 0);
	mpz_clear(&mpzscratch);
	assert(ldcount <= longobjp->ob_size);

	/* long_normalize() is file-static */
	/* longobjp = long_normalize(longobjp); */
	while (ldcount > 0 && longobjp->ob_digit[ldcount-1] == 0)
		ldcount--;
	longobjp->ob_size = ldcount;
	

	if (isnegative)
		longobjp->ob_size = -longobjp->ob_size;

	return (object *)longobjp;
	
} /* mpz_long() */


/* I would have avoided pow() anyways, so ... */
static const double multiplier = 256.0 * 256.0 * 256.0 * 256.0;
	
#ifdef MPZ_CONVERSIONS_AS_METHODS
static object *
mpz_float(self, args)
	mpzobject *self;
	object *args;
#else /* def MPZ_CONVERSIONS_AS_METHODS */
static object *
mpz_float(self)
	mpzobject *self;
#endif /* def MPZ_CONVERSIONS_AS_METHODS else */
{
	int i, isnegative;
	double x;
	double mulstate;
	MP_INT mpzscratch;


#ifdef MPZ_CONVERSIONS_AS_METHODS
	if (!getnoarg(args))
		return NULL;
#endif /* def MPZ_CONVERSIONS_AS_METHODS */

	i = (int)mpz_size(&self->mpz);
	
	/* determine sign, and copy abs(self) to scratch var */
	if (isnegative = (mpz_cmp_ui(&self->mpz, (unsigned long int)0) < 0)) {
		mpz_init(&mpzscratch);
		mpz_neg(&mpzscratch, &self->mpz);
	}
	else
		mpz_init_set(&mpzscratch, &self->mpz);

	/* let those bits come, let those bits go,
	   e.g. dismantle mpzscratch, build floatobject */

	x = 0.0;
	mulstate = 1.0;
	while (i--) {
		x += mulstate * mpz_get_ui(&mpzscratch);
		mulstate *= multiplier;
		mpz_div_2exp(&mpzscratch, &mpzscratch, BITS_PER_MP_LIMB);
	}

	assert(mpz_cmp_ui(&mpzscratch, (unsigned long int)0) == 0);
	mpz_clear(&mpzscratch);

	if (isnegative)
		x = -x;

	return newfloatobject(x);
	
} /* mpz_float() */

#ifdef MPZ_CONVERSIONS_AS_METHODS
static object *
mpz_hex(self, args)
	mpzobject *self;
	object *args;
#else /* def MPZ_CONVERSIONS_AS_METHODS */
static object *
mpz_hex(self)
	mpzobject *self;
#endif /* def MPZ_CONVERSIONS_AS_METHODS else */
{
#ifdef MPZ_CONVERSIONS_AS_METHODS
	if (!getnoarg(args))
		return NULL;
#endif /* def MPZ_CONVERSIONS_AS_METHODS */
	
	return mpz_format(self, 16, (unsigned char)1);
} /* mpz_hex() */
	
#ifdef MPZ_CONVERSIONS_AS_METHODS
static object *
mpz_oct(self, args)
	mpzobject *self;
	object *args;
#else /* def MPZ_CONVERSIONS_AS_METHODS */
static object *
mpz_oct(self)
	mpzobject *self;
#endif /* def MPZ_CONVERSIONS_AS_METHODS else */
{
#ifdef MPZ_CONVERSIONS_AS_METHODS
	if (!getnoarg(args))
		return NULL;
#endif /* def MPZ_CONVERSIONS_AS_METHODS */
	
	return mpz_format(self, 8, (unsigned char)1);
} /* mpz_oct() */
	
static object *
mpz_binary(self, args)
	mpzobject *self;
	object *args;
{
	int size;
	stringobject *strobjp;
	char *cp;
	MP_INT mp;
	unsigned long ldigit;
	
	if (!getnoarg(args))
		return NULL;

	if (mpz_cmp_ui(&self->mpz, (unsigned long int)0) < 0) {
		err_setstr(ValueError, "mpz.binary() arg must be >= 0");
		return NULL;
	}

	mpz_init_set(&mp, &self->mpz);
	size = (int)mpz_size(&mp);

	if ((strobjp = (stringobject *)
	     newsizedstringobject((char *)0,
				  size * sizeof (unsigned long int))) == NULL)
		return NULL;

	/* get the beginning of the string memory and start copying things */
	cp = GETSTRINGVALUE(strobjp);

	/* this has been programmed using a (fairly) decent lib-i/f it could
	   be must faster if we looked into the GMP lib */
	while (size--) {
		ldigit = mpz_get_ui(&mp);
		mpz_div_2exp(&mp, &mp, BITS_PER_MP_LIMB);
		*cp++ = (unsigned char)(ldigit & 0xFF);
		*cp++ = (unsigned char)((ldigit >>= 8) & 0xFF);
		*cp++ = (unsigned char)((ldigit >>= 8) & 0xFF);
		*cp++ = (unsigned char)((ldigit >>= 8) & 0xFF);
	}

	while (strobjp->ob_size && !*--cp)
		strobjp->ob_size--;

	return (object *)strobjp;
} /* mpz_binary() */
	

static struct methodlist mpz_methods[] = {
#ifdef MPZ_CONVERSIONS_AS_METHODS
	{"int",			mpz_int},
	{"long",		mpz_long},
	{"float",		mpz_float},
	{"hex",			mpz_hex},
	{"oct",			mpz_oct},
#endif /* def MPZ_CONVERSIONS_AS_METHODS */
	{"binary",		(object *(*)(object *, object *))mpz_binary},
	{NULL,			NULL}		/* sentinel */
};

static object *
mpz_getattr(self, name)
	mpzobject *self;
	char *name;
{
	return findmethod(mpz_methods, (object *)self, name);
} /* mpz_getattr() */


static int
mpz_coerce(pv, pw)
	object **pv;
	object **pw;
{
	object *z;

#ifdef MPZ_DEBUG
	fputs("mpz_coerce() called...\n", stderr);
#endif /* def MPZ_DEBUG */

	assert(is_mpzobject(*pv));

	/* always convert other arg to mpz value, except for floats */
	if (!is_floatobject(*pw)) {
		if ((z = (object *)mpz_mpzcoerce(*pw)) == NULL)
			return -1;	/* -1: an error always has been set */
		
		INCREF(*pv);
		*pw = z;
	}
	else {
		if ((z = mpz_float(*pv, NULL)) == NULL)
			return -1;

		INCREF(*pw);
		*pv = z;
	}
	return 0;		/* coercion succeeded */

} /* mpz_coerce() */


static object *
mpz_repr(v)
	object *v;
{
	return mpz_format(v, 10, (unsigned char)1);
} /* mpz_repr() */



#define UF (unaryfunc)
#define BF (binaryfunc)
#define TF (ternaryfunc)
#define IF (inquiry)
#define CF (coercion)

static number_methods mpz_as_number = {
	BF mpz_addition,	/*nb_add*/
	BF mpz_substract,	/*nb_subtract*/
	BF mpz_multiply,	/*nb_multiply*/
	BF mpz_divide,		/*nb_divide*/
	BF mpz_remainder,	/*nb_remainder*/
	BF mpz_div_and_mod,	/*nb_divmod*/
	TF mpz_power,		/*nb_power*/
	UF mpz_negative,	/*nb_negative*/
	UF mpz_positive,	/*tp_positive*/
	UF mpz_absolute,	/*tp_absolute*/
	IF mpz_nonzero,		/*tp_nonzero*/
	UF mpz_invert,		/*nb_invert*/
	BF mpz_lshift,		/*nb_lshift*/
	BF mpz_rshift,		/*nb_rshift*/
	BF mpz_andfunc,		/*nb_and*/
	BF mpz_xorfunc,		/*nb_xor*/
	BF mpz_orfunc,		/*nb_or*/
	CF mpz_coerce,		/*nb_coerce*/
#ifndef MPZ_CONVERSIONS_AS_METHODS
	UF mpz_int,		/*nb_int*/
	UF mpz_long,		/*nb_long*/
	UF mpz_float,		/*nb_float*/
	UF mpz_oct,		/*nb_oct*/
	UF mpz_hex,		/*nb_hex*/
#endif /* ndef MPZ_CONVERSIONS_AS_METHODS */
};

static typeobject MPZtype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"mpz",			/*tp_name*/
	sizeof(mpzobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)mpz_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)mpz_getattr, /*tp_getattr*/
	0,			/*tp_setattr*/
	(cmpfunc)mpz_compare,	/*tp_compare*/
	(reprfunc)mpz_repr,	/*tp_repr*/
        &mpz_as_number, 	/*tp_as_number*/
};

/* List of functions exported by this module */

static struct methodlist mpz_functions[] = {
#if 0
	{initialiser_name,	MPZ_mpz},
#else /* 0 */
	/* until guido ``fixes'' struct methodlist */
	{(char *)initialiser_name,	MPZ_mpz},
#endif /* 0 else */	
	{"powm",		MPZ_powm},
	{"gcd",			MPZ_gcd},
	{"gcdext",		MPZ_gcdext},
	{"sqrt",		MPZ_sqrt},
	{"sqrtrem",		MPZ_sqrtrem},
	{"divm",		MPZ_divm},
	{NULL,			NULL}		 /* Sentinel */
};


/* #define MP_TEST_ALLOC */

#ifdef MP_TEST_ALLOC
#define MP_TEST_SIZE		4
static const char mp_test_magic[MP_TEST_SIZE] = {'\xAA','\xAA','\xAA','\xAA'};
static mp_test_error( location )
	int *location;
{
	/* assumptions: *alloc returns address dividable by 4,
	mpz_* routines allocate in chunks dividable by four */
	fprintf(stderr, "MP_TEST_ERROR: location holds 0x%08d\n", *location );
	fatal("MP_TEST_ERROR");
} /* static mp_test_error() */
#define MP_EXTRA_ALLOC(size)	((size) + MP_TEST_SIZE)
#define MP_SET_TEST(basep,size)	(void)memcpy( ((char *)(basep))+(size), mp_test_magic, MP_TEST_SIZE)
#define MP_DO_TEST(basep,size)	if ( !memcmp( ((char *)(basep))+(size), mp_test_magic, MP_TEST_SIZE ) ) \
					; \
				else \
					mp_test_error((int *)((char *)(basep) + size))
#else /* def MP_TEST_ALLOC */
#define MP_EXTRA_ALLOC(size)	(size)
#define MP_SET_TEST(basep,size)
#define MP_DO_TEST(basep,size)
#endif /* def MP_TEST_ALLOC else */

void *mp_allocate( alloc_size )
	size_t	alloc_size;
{
	void *res;

#ifdef MPZ_DEBUG
	fprintf(stderr, "mp_allocate  :                             size %ld\n",
		alloc_size);
#endif /* def MPZ_DEBUG */	

	if ( (res = malloc(MP_EXTRA_ALLOC(alloc_size))) == NULL )
		fatal("mp_allocate failure");

#ifdef MPZ_DEBUG
	fprintf(stderr, "mp_allocate  :     address 0x%08x\n", res);
#endif /* def MPZ_DEBUG */	

	MP_SET_TEST(res,alloc_size);
	
	return res;
} /* mp_allocate() */


void *mp_reallocate( ptr, old_size, new_size )
	void *ptr;
	size_t old_size;
	size_t new_size;
{
	void *res;

#ifdef MPZ_DEBUG
	fprintf(stderr, "mp_reallocate: old address 0x%08x, old size %ld\n",
		ptr, old_size);
#endif /* def MPZ_DEBUG */	

	MP_DO_TEST(ptr, old_size);
	
	if ( (res = realloc(ptr, MP_EXTRA_ALLOC(new_size))) == NULL )
		fatal("mp_reallocate failure");

#ifdef MPZ_DEBUG
	fprintf(stderr, "mp_reallocate: new address 0x%08x, new size %ld\n",
		res, new_size);
#endif /* def MPZ_DEBUG */	

	MP_SET_TEST(res, new_size);

	return res;
} /* mp_reallocate() */


void mp_free( ptr, size )
	void *ptr;
	size_t size;
{

#ifdef MPZ_DEBUG
	fprintf(stderr, "mp_free      : old address 0x%08x, old size %ld\n",
		ptr, size);
#endif /* def MPZ_DEBUG */	

	MP_DO_TEST(ptr, size);
	free(ptr);
} /* mp_free() */



/* Initialize this module. */

void
initmpz()
{
#ifdef MPZ_DEBUG
	fputs( "initmpz() called...\n", stderr );
#endif /* def MPZ_DEBUG */

	mp_set_memory_functions( mp_allocate, mp_reallocate, mp_free );
	(void)initmodule("mpz", mpz_functions);

	/* create some frequently used constants */
	if ((mpz_value_zero = newmpzobject()) == NULL)
		fatal("initmpz: can't initialize mpz constants");
	mpz_set_ui(&mpz_value_zero->mpz, (unsigned long int)0);

	if ((mpz_value_one = newmpzobject()) == NULL)
		fatal("initmpz: can't initialize mpz constants");
	mpz_set_ui(&mpz_value_one->mpz, (unsigned long int)1);

	if ((mpz_value_mone = newmpzobject()) == NULL)
		fatal("initmpz: can't initialize mpz constants");
	mpz_set_si(&mpz_value_mone->mpz, (long)-1);

} /* initmpz() */
#ifdef MAKEDUMMYINT
int _mpz_dummy_int;	/* XXX otherwise, we're .bss-less (DYNLOAD->Jack?) */
#endif /* def MAKEDUMMYINT */
