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
/******************************************************************
Copyright 1992 by Lance Ellinghouse (lance@markv.com)

All Rights Reserved

Permission to use, copy, distribute for any purpose and without fee
is hereby granted, provided that this copyright notice appear in
all copies and that both this copyright notice and this permission
notice appear in supporting documentation.
Permission to make any changes is granted on the basis that all
changes and improvements are also forwarded to Lance Ellinghouse
before distribution.
******************************************************************/

/* This creates an encryption and decryption engine I am calling
   a rotor due to the original design was a harware rotor with
   contacts used in Germany during WWII.

Rotor Module:

-  rotor.newrotor('key') -> rotorobject  (default of 6 rotors)
-  rotor.newrotor('key', num_rotors) -> rotorobject

Rotor Objects:

-  ro.setkey('string') -> None (resets the key as defined in newrotor().
-  ro.encrypt('string') -> encrypted string
-  ro.decrypt('encrypted string') -> unencrypted string

-  ro.encryptmore('string') -> encrypted string
-  ro.decryptmore('encrypted string') -> unencrypted string

NOTE: the {en,de}cryptmore() methods use the setup that was
      established via the {en,de}crypt calls. They will NOT
      re-initalize the rotors unless: 1) They have not been
      initalized with {en,de}crypt since the last setkey() call;
      2) {en,de}crypt has not been called for this rotor yet.

NOTE: you MUST use the SAME key in rotor.newrotor()
      if you wish to decrypt an encrypted string.
      Also, the encrypted string is NOT 0-127 ASCII. 
      It is considered BINARY data.

*/

/* Rotor objects */

#include "allobjects.h"
#include "modsupport.h"
#include <stdio.h>
#include <math.h>
#define TRUE	1
#define FALSE	0

typedef struct {
	OB_HEAD
	int seed[3];
    	short key[5];
	int  isinited;
	int  size;
	int  size_mask;
    	int  rotors;
	unsigned char *e_rotor; /* [num_rotors][size] */
	unsigned char *d_rotor; /* [num_rotors][size] */
	unsigned char *positions; /* [num_rotors] */
	unsigned char *advances; /* [num_rotors] */
} rotorobject;

extern typeobject Rotortype;	/* Really static, forward */

#define is_rotorobject(v)		((v)->ob_type == &Rotortype)

/*
	This defines the necessary routines to manage rotor objects
*/

static void set_seed( r )
rotorobject *r;
{
	r->seed[0] = r->key[0];
	r->seed[1] = r->key[1];
	r->seed[2] = r->key[2];
	r->isinited = FALSE;
}
	
/* Return the next random number in the range [0.0 .. 1.0) */
static float r_random( r )
rotorobject *r;
{
	int x, y, z;
	float val, term;

	x = r->seed[0];
	y = r->seed[1];
	z = r->seed[2];

	x = 171 * (x % 177) - 2 * (x/177);
	y = 172 * (y % 176) - 35 * (y/176);
	z = 170 * (z % 178) - 63 * (z/178);
	
	if (x < 0) x = x + 30269;
	if (y < 0) y = y + 30307;
	if (z < 0) z = z + 30323;
	
	r->seed[0] = x;
	r->seed[1] = y;
	r->seed[2] = z;

	term = (float)(
			(((float)x)/(float)30269.0) + 
			(((float)y)/(float)30307.0) + 
			(((float)z)/(float)30323.0)
			);
	val = term - (float)floor((double)term);

	if (val >= 1.0) val = 0.0;

	return val;
}

static short r_rand(r,s)
rotorobject *r;
short s;
{
	/*short tmp = (short)((int)(r_random(r) * (float)32768.0) % 32768);*/
	short tmp = (short)((short)(r_random(r) * (float)s) % s);
	return tmp;
}

static void set_key(r, key)
rotorobject *r;
char *key;
{
	int k1=995, k2=576, k3=767, k4=671, k5=463;
	int i;
	int len=strlen(key);
	for (i=0;i<len;i++) {
		k1 = (((k1<<3 | k1<<-13) + key[i]) & 65535);
		k2 = (((k2<<3 | k2<<-13) ^ key[i]) & 65535);
		k3 = (((k3<<3 | k3<<-13) - key[i]) & 65535);
		k4 = ((key[i] - (k4<<3 | k4<<-13)) & 65535);
		k5 = (((k5<<3 | k5<<-13) ^ ~key[i]) & 65535);
	}
	r->key[0] = (short)k1;
	r->key[1] = (short)(k2|1);
	r->key[2] = (short)k3;
	r->key[3] = (short)k4;
	r->key[4] = (short)k5;

	set_seed(r);
}

/* These define the interface to a rotor object */
static rotorobject *
newrotorobject(num_rotors, key)
	int num_rotors;
	char *key;
{
	rotorobject *xp;
	xp = NEWOBJ(rotorobject, &Rotortype);
	if (xp == NULL)
		return NULL;
	set_key(xp, key);

	xp->size = 256;
	xp->size_mask = xp->size - 1;
	xp->size_mask = 0;
	xp->rotors = num_rotors;
	xp->e_rotor = NULL;
	xp->d_rotor = NULL;
	xp->positions = NULL;
	xp->advances = NULL;

	xp->e_rotor =
	     (unsigned char *)malloc((num_rotors * (xp->size * sizeof(char))));
	if (xp->e_rotor == (unsigned char *)NULL)
		goto fail;
	xp->d_rotor =
	     (unsigned char *)malloc((num_rotors * (xp->size * sizeof(char))));
	if (xp->d_rotor == (unsigned char *)NULL)
		goto fail;
	xp->positions = (unsigned char *)malloc(num_rotors * sizeof(char));
	if (xp->positions == (unsigned char *)NULL)
		goto fail;
	xp->advances  = (unsigned char *)malloc(num_rotors * sizeof(char));
	if (xp->advances == (unsigned char *)NULL)
		goto fail;
	return xp;
fail:
	DECREF(xp);
	return (rotorobject *)err_nomem();
}

/* These routines impliment the rotor itself */

/*  Here is a fairly sofisticated {en,de}cryption system.  It is bassed
on the idea of a "rotor" machine.  A bunch of rotors, each with a
different permutation of the alphabet, rotate around a different
amount after encrypting one character.  The current state of the
rotors is used to encrypt one character.

  The code is smart enought to tell if your alphabet has a number of
characters equal to a power of two.  If it does, it uses logical
operations, if not it uses div and mod (both require a division).

  You will need to make two changes to the code 1) convert to c, and
customize for an alphabet of 255 chars 2) add a filter at the
begining, and end, which subtracts one on the way in, and adds one on
the way out.

  You might wish to do some timing studies.  Another viable
alternative is to "byte stuff" the encrypted data of a normal (perhaps
this one) encryption routine.

j'
*/

/*(defun RTR-make-id-rotor (rotor)
  "Set ROTOR to the identity permutation"
  (let ((j 0))
    (while (< j RTR-size)
      (aset rotor j j)
      (setq j (+ 1 j)))
    rotor))*/
static void RTR_make_id_rotor(r, rtr)
	rotorobject *r;
	unsigned char *rtr;
{
	register int j;
	register int size = r->size;
	for (j=0;j<size;j++) {
		rtr[j] = (unsigned char)j;
	}
}


/*(defvar RTR-e-rotors 
  (let ((rv (make-vector RTR-number-of-rotors 0))
	(i 0)
	tr)
    (while (< i RTR-number-of-rotors)
      (setq tr (make-vector RTR-size 0))
      (RTR-make-id-rotor tr)
      (aset rv i tr)
      (setq i (+ 1 i)))
    rv)
  "The current set of encryption rotors")*/
static void RTR_e_rotors(r)
	rotorobject *r;
{
	int i;
	for (i=0;i<r->rotors;i++) {
		RTR_make_id_rotor(r,&(r->e_rotor[(i*r->size)]));
	}
}

/*(defvar RTR-d-rotors 
  (let ((rv (make-vector RTR-number-of-rotors 0))
	(i 0)
	tr)
    (while (< i RTR-number-of-rotors)
      (setq tr (make-vector RTR-size 0))
      (setq j 0)
      (while (< j RTR-size)
	(aset tr j j)
	(setq j (+ 1 j)))
      (aset rv i tr)
      (setq i (+ 1 i)))
    rv)
  "The current set of decryption rotors")*/
static void RTR_d_rotors(r)
	rotorobject *r;
{
	register int i, j;
	for (i=0;i<r->rotors;i++) {
		for (j=0;j<r->size;j++) {
			r->d_rotor[((i*r->size)+j)] = (unsigned char)j;
		}
	}
}

/*(defvar RTR-positions (make-vector RTR-number-of-rotors 1)
  "The positions of the rotors at this time")*/
static void RTR_positions(r)
	rotorobject *r;
{
	int i;
	for (i=0;i<r->rotors;i++) {
		r->positions[i] = 1;
	}
}

/*(defvar RTR-advances (make-vector RTR-number-of-rotors 1)
  "The number of positions to advance the rotors at a time")*/
static void RTR_advances(r) 
	rotorobject *r;
{
	int i;
	for (i=0;i<r->rotors;i++) {
		r->advances[i] = 1;
	}
}

/*(defun RTR-permute-rotor (e d)
  "Permute the E rotor, and make the D rotor its inverse"
  ;; see Knuth for explaination of algorythm.
  (RTR-make-id-rotor e)
  (let ((i RTR-size)
	q j)
    (while (<= 2 i)
      (setq q (fair16 i))		; a little tricky, decrement here
      (setq i (- i 1))			; since we have origin 0 array's
      (setq j (aref e q))
      (aset e q (aref e i))
      (aset e i j)
      (aset d j i))
    (aset e 0 (aref e 0))		; don't forget e[0] and d[0]
    (aset d (aref e 0) 0)))*/
static void RTR_permute_rotor(r, e, d)
	rotorobject *r;
	unsigned char *e;
	unsigned char *d;
{
	short i = r->size;
	short q;
	unsigned char j;
	RTR_make_id_rotor(r,e);
	while (2 <= i) {
		q = r_rand(r,i);
		i--;
		j = e[q];
		e[q] = (unsigned char)e[i];
		e[i] = (unsigned char)j;
		d[j] = (unsigned char)i;
	}
	e[0] = (unsigned char)e[0];
	d[(e[0])] = (unsigned char)0;
}

/*(defun RTR-init (key)
  "Given KEY (a list of 5 16 bit numbers), initialize the rotor machine.
Set the advancement, position, and permutation of the rotors"
  (R16-set-state key)
  (let (i)
    (setq i 0)
    (while (< i RTR-number-of-rotors)
      (aset RTR-positions i (fair16 RTR-size))
      (aset RTR-advances i (+ 1 (* 2 (fair16 (/ RTR-size 2)))))
      (message "Initializing rotor %d..." i)
      (RTR-permute-rotor (aref RTR-e-rotors i) (aref RTR-d-rotors i))
      (setq i (+ 1 i)))))*/
static void RTR_init(r)
	rotorobject *r;
{
	int i;
	set_seed(r);
	RTR_positions(r);
	RTR_advances(r);
	RTR_e_rotors(r);
	RTR_d_rotors(r);
	for(i=0;i<r->rotors;i++) {
		r->positions[i] = r_rand(r,r->size);
		r->advances[i] = (1+(2*(r_rand(r,r->size/2))));
		RTR_permute_rotor(r,&(r->e_rotor[(i*r->size)]),&(r->d_rotor[(i*r->size)]));
	}
	r->isinited = TRUE;
}

/*(defun RTR-advance ()
  "Change the RTR-positions vector, using the RTR-advances vector"
  (let ((i 0)
	(temp 0))
    (if RTR-size-mask
	(while (< i RTR-number-of-rotors)
	  (setq temp (+ (aref RTR-positions i) (aref RTR-advances i)))
	  (aset RTR-positions i (logand temp RTR-size-mask))
	  (if (and (>= temp RTR-size)
		   (< i (- RTR-number-of-rotors 1))) 
	      (aset RTR-positions (+ i 1)
		    (+ 1 (aref RTR-positions (+ i 1)))))
	  (setq i (+ i 1)))
      (while (< i RTR-number-of-rotors)
	(setq temp (+ (aref RTR-positions i) (aref RTR-advances i)))
	(aset RTR-positions i (% temp RTR-size))
	(if (and (>= temp RTR-size)
		 (< i (- RTR-number-of-rotors 1))) 
	    (aset RTR-positions (+ i 1)
		  (+ 1 (aref RTR-positions (+ i 1)))))
	(setq i (+ i 1))))))*/
static void RTR_advance(r)
	rotorobject *r;
{
	register int i=0, temp=0;
	if (r->size_mask) {
		while (i<r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp & r->size_mask;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	} else {
		while (i<r->rotors) {
			temp = r->positions[i] + r->advances[i];
			r->positions[i] = temp%r->size;
			if ((temp >= r->size) && (i < (r->rotors - 1))) {
				r->positions[(i+1)] = 1 + r->positions[(i+1)];
			}
			i++;
		}
	}
}

/*(defun RTR-e-char (p)
  "Encrypt the character P with the current rotor machine"
  (let ((i 0))
    (if RTR-size-mask
	(while (< i RTR-number-of-rotors)
	  (setq p (aref (aref RTR-e-rotors i)
			(logand (logxor (aref RTR-positions i)
					p)
				RTR-size-mask)))
	  (setq i (+ 1 i)))
      (while (< i RTR-number-of-rotors)
	  (setq p (aref (aref RTR-e-rotors i)
			(% (logxor (aref RTR-positions i)
				   p)
			   RTR-size)))
	(setq i (+ 1 i))))
    (RTR-advance)
    p))*/
static unsigned char RTR_e_char(r, p)
	rotorobject *r;
	unsigned char p;
{
	register int i=0;
	register unsigned char tp=p;
	if (r->size_mask) {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size)+(((r->positions[i] ^ tp) & r->size_mask))];
			i++;
		}
	} else {
		while (i < r->rotors) {
			tp = r->e_rotor[(i*r->size)+(((r->positions[i] ^ tp) % r->size))];
			i++;
		}
	}
	RTR_advance(r);
	return ((unsigned char)tp);
}

/*(defun RTR-d-char (c)
  "Decrypt the character C with the current rotor machine"
  (let ((i (- RTR-number-of-rotors 1)))
    (if RTR-size-mask
	(while (<= 0 i)
	  (setq c (logand (logxor (aref RTR-positions i)
				  (aref (aref RTR-d-rotors i)
					c))
			  RTR-size-mask))
	  (setq i (- i 1)))
	(while (<= 0 i)
	  (setq c (% (logxor (aref RTR-positions i)
			     (aref (aref RTR-d-rotors i)
				   c))
		     RTR-size))
	  (setq i (- i 1))))
    (RTR-advance)
    c))*/
static unsigned char RTR_d_char(r, c)
	rotorobject *r;
	unsigned char c;
{
	register int i=r->rotors - 1;
	register unsigned char tc=c;
	if (r->size_mask) {
		while (0 <= i) {
			tc = (r->positions[i] ^ r->d_rotor[(i*r->size)+tc]) & r->size_mask;
			i--;
		}
	} else {
		while (0 <= i) {
			tc = (r->positions[i] ^ r->d_rotor[(i*r->size)+tc]) % r->size;
			i--;
		}
	}
	RTR_advance(r);
	return(tc);
}

/*(defun RTR-e-region (beg end key)
  "Perform a rotor encryption of the region from BEG to END by KEY"
  (save-excursion
    (let ((tenth (/ (- end beg) 10)))
      (RTR-init key)
      (goto-char beg)
      ;; ### make it stop evry 10% or so to tell us
      (while (< (point) end)
	(let ((fc (following-char)))
	  (insert-char (RTR-e-char fc) 1)
	  (delete-char 1))))))*/
static void RTR_e_region(r, beg, len, doinit)
	rotorobject *r;
	unsigned char *beg;
	int len;
	int doinit;
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i=0;i<len;i++) {
		beg[i]=RTR_e_char(r,beg[i]);
	}
}

/*(defun RTR-d-region (beg end key)
  "Perform a rotor decryption of the region from BEG to END by KEY"
  (save-excursion
    (progn
      (RTR-init key)
      (goto-char beg)
      (while (< (point) end)
	(let ((fc (following-char)))
	  (insert-char (RTR-d-char fc) 1)
	  (delete-char 1))))))*/
void static RTR_d_region(r, beg, len, doinit)
	rotorobject *r;
	unsigned char *beg;
	int len;
	int doinit;
{
	register int i;
	if (doinit || r->isinited == FALSE)
		RTR_init(r);
	for (i=0;i<len;i++) {
		beg[i]=RTR_d_char(r,beg[i]);
	}
}


/*(defun RTR-key-string-to-ints (key)
  "Convert a string into a list of 4 numbers"
  (let ((k1 995)
	(k2 576)
	(k3 767)
	(k4 671)
	(k5 463)
	(i 0))
    (while (< i (length key))
      (setq k1 (logand (+      (logior (lsh k1 3) (lsh k1 -13)) (aref key i)) 65535))
      (setq k2 (logand (logxor (logior (lsh k2 3) (lsh k2 -13)) (aref key i)) 65535))
      (setq k3 (logand (-      (logior (lsh k3 3) (lsh k3 -13)) (aref key i)) 65535))
      (setq k4 (logand (-      (aref key i) (logior (lsh k4 3) (lsh k4 -13))) 65535))
      (setq k5 (logand (logxor (logior (lsh k5 3) (lsh k5 -13)) (lognot (aref key i))) 65535))
      (setq i (+ i 1)))
    (list k1 (logior 1 k2) k3 k4 k5)))*/
/* This is done in set_key() above */

/*(defun encrypt-region (beg end key)
  "Interactivly encrypt the region"
  (interactive "r\nsKey:")
  (RTR-e-region beg end (RTR-key-string-to-ints key)))*/
static void encrypt_region(r, region, len)
	rotorobject *r;
	unsigned char *region;
	int len;
{
	RTR_e_region(r,region,len,TRUE);
}

/*(defun decrypt-region (beg end key)
  "Interactivly decrypt the region"
  (interactive "r\nsKey:")
  (RTR-d-region beg end (RTR-key-string-to-ints key)))*/
static void decrypt_region(r, region, len)
	rotorobject *r;
	unsigned char *region;
	int len;
{
	RTR_d_region(r,region,len,TRUE);
}

/* Rotor methods */

static void
rotor_dealloc(xp)
	rotorobject *xp;
{
	XDEL(xp->e_rotor);
	XDEL(xp->d_rotor);
	XDEL(xp->positions);
	XDEL(xp->advances);
	DEL(xp);
}

static object *
rotor_encrypt(self, args)
	rotorobject *self;
	object *args;
{
	char *string = (char *)NULL;
	int len = 0;
	object *rtn = (object *)NULL;
	char *tmp;

	if (!getargs(args,"s#",&string, &len))
		return NULL;
	if (!(tmp = (char *)malloc(len+5))) {
		err_nomem();
		return NULL;
	}
	memset(tmp,'\0',len+1);
	memcpy(tmp,string,len);
	RTR_e_region(self,tmp,len, TRUE);
	rtn = newsizedstringobject(tmp,len);
	free(tmp);
	return(rtn);
}

static object *
rotor_encryptmore(self, args)
	rotorobject *self;
	object *args;
{
	char *string = (char *)NULL;
	int len = 0;
	object *rtn = (object *)NULL;
	char *tmp;

	if (!getargs(args,"s#",&string, &len))
		return NULL;
	if (!(tmp = (char *)malloc(len+5))) {
		err_nomem();
		return NULL;
	}
	memset(tmp,'\0',len+1);
	memcpy(tmp,string,len);
	RTR_e_region(self,tmp,len, FALSE);
	rtn = newsizedstringobject(tmp,len);
	free(tmp);
	return(rtn);
}

static object *
rotor_decrypt(self, args)
	rotorobject *self;
	object *args;
{
	char *string = (char *)NULL;
	int len = 0;
	object *rtn = (object *)NULL;
	char *tmp;

	if (!getargs(args,"s#",&string, &len))
		return NULL;
	if (!(tmp = (char *)malloc(len+5))) {
		err_nomem();
		return NULL;
	}
	memset(tmp,'\0',len+1);
	memcpy(tmp,string,len);
	RTR_d_region(self,tmp,len, TRUE);
	rtn = newsizedstringobject(tmp,len);
	free(tmp);
	return(rtn);
}

static object *
rotor_decryptmore(self, args)
	rotorobject *self;
	object *args;
{
	char *string = (char *)NULL;
	int len = 0;
	object *rtn = (object *)NULL;
	char *tmp;

	if (!getargs(args,"s#",&string, &len))
		return NULL;
	if (!(tmp = (char *)malloc(len+5))) {
		err_nomem();
		return NULL;
	}
	memset(tmp,'\0',len+1);
	memcpy(tmp,string,len);
	RTR_d_region(self,tmp,len, FALSE);
	rtn = newsizedstringobject(tmp,len);
	free(tmp);
	return(rtn);
}

static object *
rotor_setkey(self, args)
	rotorobject *self;
	object *args;
{
	char *key;
	char *string;

	if (getargs(args,"s",&string))
		set_key(self,string);
	INCREF(None);
	return None;
}

static struct methodlist rotor_methods[] = {
	{"encrypt",	rotor_encrypt},
	{"encryptmore",	rotor_encryptmore},
	{"decrypt",	rotor_decrypt},
	{"decryptmore",	rotor_decryptmore},
	{"setkey",	rotor_setkey},
	{NULL,		NULL}		/* sentinel */
};


/* Return a rotor object's named attribute. */
static object *
rotor_getattr(s, name)
	rotorobject *s;
	char *name;
{
	return findmethod(rotor_methods, (object *) s, name);
}

static typeobject Rotortype = {
	OB_HEAD_INIT(&Typetype)
	0,			/*ob_size*/
	"rotor",			/*tp_name*/
	sizeof(rotorobject),	/*tp_size*/
	0,			/*tp_itemsize*/
	/* methods */
	rotor_dealloc,	/*tp_dealloc*/
	0,		/*tp_print*/
	rotor_getattr,		/*tp_getattr*/
	0,		/*tp_setattr*/
	0,		/*tp_compare*/
	0,		/*tp_repr*/
};


static object *
rotor_rotor(self, args)
	object *self;
	object *args;
{
	char *string;
	rotorobject *r;
	int len;
	int num_rotors;

	if (getargs(args,"s#", &string, &len)) {
		num_rotors = 6;
	} else {
		err_clear();
		if (!getargs(args,"(s#i)", &string, &len, &num_rotors))
			return NULL;
	}
	r = newrotorobject(num_rotors, string);
	return (object *)r;
}

static struct methodlist rotor_rotor_methods[] = {
	{"newrotor",		rotor_rotor},
	{NULL,			NULL}		 /* Sentinel */
};


/* Initialize this module.
   This is called when the first 'import rotor' is done,
   via a table in config.c, if config.c is compiled with USE_ROTOR
   defined. */

void
initrotor()
{
	object *m;

	m = initmodule("rotor", rotor_rotor_methods);
}
