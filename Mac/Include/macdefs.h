/* Useful #includes and #defines for programming a set of Unix
   look-alike file system access functions on the Macintosh.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
*/

#include <Types.h>
#include <Files.h>
#include <OSUtils.h>

#include <errno.h>
#include <string.h>
#ifdef __MWERKS__
#include "errno_unix.h"
#include <TextUtils.h>
#endif

/* We may be able to use a std routine in think, don't know */
extern unsigned char *Pstring(char *);
extern char *getbootvol(void);
extern char *getwd(char *);
#ifndef USE_GUSI
extern int sync(void);
#endif

/* Universal constants: */
#define MAXPATH 256
#ifndef __MSL__
#define TRUE 1
#define FALSE 0
#endif
#ifndef NULL
#define NULL 0
#endif
#define EOS '\0'
#define SEP ':'
