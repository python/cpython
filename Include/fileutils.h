#ifndef Py_FILEUTILS_H
#define Py_FILEUTILS_H

/*******************************
 * stat() and fstat() fiddling *
 *******************************/

#ifdef HAVE_SYS_STAT_H
#  include <sys/stat.h>           // S_ISREG()
#elif defined(HAVE_STAT_H)
#  include <stat.h>               // S_ISREG()
#endif

#ifndef S_IFMT
   // VisualAge C/C++ Failed to Define MountType Field in sys/stat.h.
#  define S_IFMT 0170000
#endif
#ifndef S_IFLNK
   // Windows doesn't define S_IFLNK, but fileutils.c maps
   // IO_REPARSE_TAG_SYMLINK to S_IFLNK.
#  define S_IFLNK 0120000
#endif
#ifndef S_ISREG
#  define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#  define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#  define S_ISCHR(x) (((x) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISLNK
#  define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)
#endif


// Move this down here since some C++ #include's don't like to be included
// inside an extern "C".
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
PyAPI_FUNC(wchar_t *) Py_DecodeLocale(
    const char *arg,
    size_t *size);

PyAPI_FUNC(char*) Py_EncodeLocale(
    const wchar_t *text,
    size_t *error_pos);
#endif

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_FILEUTILS_H
#  include "cpython/fileutils.h"
#  undef Py_CPYTHON_FILEUTILS_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_FILEUTILS_H */
