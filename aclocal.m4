# Code swiped wholesale from the GCC project, see
# http://gcc.gnu.org/bugzilla/show_bug.cgi?id=12100

# This file can go away once autoconf 2.58 is out and being used -
# it's reported that this is fixed in the autoconf cvs already.

# AC_LANG_FUNC_LINK_TRY(C)(FUNCTION)
# ----------------------------------
# Don't include <ctype.h> because on OSF/1 3.0 it includes
# <sys/types.h> which includes <sys/select.h> which contains a
# prototype for select.  Similarly for bzero.
#
# A similar problem afflicts HP/UX, but it also hits <sys/time.h>
#
# This test used to merely assign f=$1 in main(), but that was
# optimized away by HP unbundled cc A.05.36 for ia64 under +O3,
# presumably on the basis that there's no need to do that store if the
# program is about to exit.  Conversely, the AIX linker optimizes an
# unused external declaration that initializes f=$1.  So this test
# program has both an external initialization of f, and a use of f in
# main that affects the exit status.
#
m4_define([AC_LANG_FUNC_LINK_TRY(C)],
[AC_LANG_PROGRAM(
[/* System header to define __stub macros and hopefully few prototypes,
    which can conflict with char $1 (); below.
    Prefer <limits.h> to <assert.h> if __STDC__ is defined, since
    <limits.h> exists even on freestanding compilers.  Under hpux,
    including <limits.h> includes <sys/time.h> and causes problems
    checking for functions defined therein.  */
#if defined (__STDC__) && !defined (_HPUX_SOURCE)
# include <limits.h>
#else
# include <assert.h>
#endif
/* Override any gcc2 internal prototype to avoid an error.  */
#ifdef __cplusplus
extern "C"
{
#endif
/* We use char because int might match the return type of a gcc2
   builtin and then its argument prototype would still apply.  */
char $1 ();
/* The GNU C library defines this for functions which it implements
    to always fail with ENOSYS.  Some functions are actually named
    something starting with __ and the normal name is an alias.  */
#if defined (__stub_$1) || defined (__stub___$1)
choke me
#else
char (*f) () = $1;
#endif
#ifdef __cplusplus
}
#endif
], [return f != $1;])])


