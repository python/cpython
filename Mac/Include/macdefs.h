/* Useful #includes and #defines for programming a set of Unix
   look-alike file system access functions on the Macintosh.
   Public domain by Guido van Rossum, CWI, Amsterdam (July 1987).
*/

#include <Types.h>
#include <Files.h>
#include <OSUtils.h>

#ifndef MPW
#include <pascal.h>
#endif

#include <errno.h>
#include <string.h>

/* Difference in origin between Mac and Unix clocks: */
#define TIMEDIFF ((unsigned long) \
	(((1970-1904)*365 + (1970-1904)/4) * 24 * 3600))

/* Macro to find out whether we can do HFS-only calls: */
#define FSFCBLen (* (short *) 0x3f6)
#define hfsrunning() (FSFCBLen > 0)

/* Universal constants: */
#define MAXPATH 256
#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
#define EOS '\0'
#define SEP ':'

#if 0 // doesn't work
/* Call Macsbug: */
pascal void Debugger() extern 0xA9FF;
#endif
