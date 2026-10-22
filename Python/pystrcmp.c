/* Cross platform case insensitive string compare functions
 */

#include "Python.h"

int
PyOS_mystrnicmp(const char *s1, const char *s2, Py_ssize_t size)
{
    const unsigned char *p1, *p2;
    if (size == 0)
        return 0;
    p1 = (const unsigned char *)s1;
    p2 = (const unsigned char *)s2;
    for (; (--size > 0) && *p1 && *p2 && (Py_TOLOWER(*p1) == Py_TOLOWER(*p2));
         p1++, p2++) {
        ;
    }
    return Py_TOLOWER(*p1) - Py_TOLOWER(*p2);
}

int
PyOS_mystricmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (; *p1 && *p2 && (Py_TOLOWER(*p1) == Py_TOLOWER(*p2)); p1++, p2++) {
        ;
    }
    return (Py_TOLOWER(*p1) - Py_TOLOWER(*p2));
}
