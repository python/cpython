#if defined(__osf__) && defined(__arch64__)

// Native cc headers require requesting pieces of C99.
// Native cc support is hypothetical. gcc4 is being used.
#define __STDC_CONSTANT_MACROS 1
#define __STDC_LIMIT_MACROS 1
#define __STDC_FORMAT_MACROS 1

#undef _ISO_C_SOURCE
#define _ISO_C_SOURCE 199901
#define _OSF_SOURCE 1
#define _ANSI_C_SOURCE 1
#define _XOPEN_SOURCE 700       // highest value in /usr/include is 500 but match Python elsewhere
#define _POSIX_C_SOURCE 200809L // highest value in /usr/include is 199506L but match Python elsewhere
// Unfortunately defining _XOPEN_SOURCE_EXTENDED both hides and reveals symbols.
// _XOPEN_SOURCE_EXTENDED is generally defined by /usr/include/standards.h
// that /usr/include/all includes*.h first includes.
// There is no good solution. That it hides symbols is probably an error.
// On balance it seems the path of less resistance is let
// _XOPEN_SOURCE_EXTENDED be defined and duplicate the small amount
// of content that it suppresses.
#include <sys/types.h>
#include <stdint.h>
// sys/types.h defines _XOPEN_SOURCE_EXTENDED
// pyconfig.h will define it again, producing many warnings
#undef _XOPEN_SOURCE_EXTENDED

#ifdef __cplusplus
extern "C" {
#endif

// NSIG is in /usr/include/signal.h behind ifndef _XOPEN_SOURCE_EXTENDED; verbatim:
// TODO: How to evade patchcheck which damaged verbatim copy?
#define NSIG    (SIGMAX+1)      /* prefer __sys_nsig; see above */

double copysign(double, double); // /usr/include/math.h ifndef _XOPEN_SOURCE_EXTENDED
int initgroups(char*, gid_t);    // /usr/include/grp.h ifndef _XOPEN_SOURCE_EXTENDED
int plock(int);                  // documented but not declared anywhere
#define HAVE_BROKEN_UNSETENV 1   // unsetenv returns void, not int; configure fails because ifdef _BSD

#ifdef __GNUC__
double round(double); // /usr/include.dtk/math.h and /usr/lib/cmplrs/cc.dtk/EIDF.h, unlikely usable with gcc
int finite(double);   // /usr/include.dtk/math.h, unlikely usable with gcc

// from /usr/include.dtk/inttypes.h, not likely usable with gcc
// 32 == int
// 64 == long == long long == max
#define PRId32          "d"
#define PRId64          "ld"
#define PRIi32          "i"
#define PRIi64          "li"
#define PRIu32          "u"
#define PRIu64          "lu"
#define PRIx32          "x"
#define PRIx64          "lx"
#define PRIX32          "X"
// incorrect system definition: "X"
#define PRIX64          "lX"
#define PRIXPTR         "lX"

#endif // __GNUC__

#define CLOCK_MONOTONIC CLOCK_REALTIME // no known monotonic time source; fake it poorly for py_get_monotonic_clock

int setenv(const char*, const char*, int); // /usr/include/stdlib.h ifdef _BSD
void unsetenv(const char*);                // /usr/include/stdlib.h ifdef _BSD

#ifdef __cplusplus
} // extern "C"
#endif

#endif // osf && 64
