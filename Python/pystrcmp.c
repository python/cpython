/* Cross platform case insensitive string compare functions
 */

#include "Python.h"

int
PyOS_mystrnicmp(const char *s1, const char *s2, Py_ssize_t size)
{
    const unsigned char *p1, *p2;
    if (size == 0)
        return 0;
    p1 = (void *)s1;
    p2 = (void *)s2;
    for (; (--size > 0) && (tolower(*p1) == tolower(*p2)); p1++, p2++) {
        if (!*p1 || !*p2)
            break;
    }
    return tolower(*p1) - tolower(*p2);
}

int
PyOS_mystricmp(const char *s1, const char *s2)
{
    const unsigned char *p1 = (void *)s1, *p2 = (void *)s2;
    for (; *p1 && (tolower(*p1) == tolower(*p2)); p1++, p2++) {
        ;
    }
    return (tolower(*p1) - tolower(*p2));
}
