/* MD5.H - header file for MD5C.C
 */

/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
rights reserved.

License to copy and use this software is granted provided that it
is identified as the "RSA Data Security, Inc. MD5 Message-Digest
Algorithm" in all material mentioning or referencing this software
or this function.

License is also granted to make and use derivative works provided
that such works are identified as "derived from the RSA Data
Security, Inc. MD5 Message-Digest Algorithm" in all material
mentioning or referencing the derived work.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

/* ========== include global.h ========== */
/* GLOBAL.H - RSAREF types and constants
 */

/* POINTER defines a generic pointer type */
typedef unsigned char *POINTER;

/* UINT2 defines a two byte word */
typedef unsigned short int UINT2;

/* UINT4 defines a four byte word */
#if SIZEOF_LONG == 4
typedef unsigned long int UINT4;
#else
#if INT_MAX == 2147483647
typedef unsigned int UINT4;
#endif
/* Too bad if neither is; pyport.h would need to be fixed. */
#endif

/* ========== End global.h; continue md5.h ========== */

/* MD5 context. */
typedef struct {
    UINT4 state[4];                                   /* state (ABCD) */
    UINT4 count[2];        /* number of bits, modulo 2^64 (lsb first) */
    unsigned char buffer[64];                         /* input buffer */
} MD5_CTX;

/* Rename all exported symbols to avoid conflicts with similarly named
   symbols in some systems' standard C libraries... */

#define MD5Init _Py_MD5Init
#define MD5Update _Py_MD5Update
#define MD5Final _Py_MD5Final

void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);
