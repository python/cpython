/* These variables used to be used when Python was built with --with-fpectl,
 * but support for that was dropped in 3.7. We continue to define them,
 * though, because they may be referenced by extensions using the stable ABI.
 */

#ifdef HAVE_SETJMP_H
#include <setjmp.h>

jmp_buf PyFPE_jbuf;
#endif

int PyFPE_counter;

double
PyFPE_dummy(void *dummy)
{
    return 1.0;
}
