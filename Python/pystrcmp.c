/* Cross platform case insensitive string compare functions
 */

#include "Python.h"

int
PyOS_mystrnicmp(const char *s1, const char *s2, Py_ssize_t size)
{
    if (size == 0)
        return 0;
    while ((--size > 0) &&
           (tolower((unsigned)*s1) == tolower((unsigned)*s2))) {
        if (!*s1++ || !*s2++)
            break;
    }
    return tolower((unsigned)*s1) - tolower((unsigned)*s2);
}

int
PyOS_mystricmp(const char *s1, const char *s2)
{
    while (*s1 && (tolower((unsigned)*s1++) == tolower((unsigned)*s2++))) {
        ;
    }
    return (tolower((unsigned)*s1) - tolower((unsigned)*s2));
}
