#ifndef Py_OSDEFS_H
#define Py_OSDEFS_H
#ifdef __cplusplus
extern "C" {
#endif


/* Operating system dependencies */

#ifdef macintosh
#define SEP ':'
#define MAXPATHLEN 256
/* Mod by Jack: newline is less likely to occur in filenames than space */
#define DELIM '\n'
#endif

/* Mod by chrish: QNX has WATCOM, but isn't DOS */
#if !defined(__QNX__)
#if defined(MS_WINDOWS) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__DJGPP__) || defined(PYOS_OS2)
#define SEP '\\'
#define ALTSEP '/'
#define MAXPATHLEN 256
#define DELIM ';'
#endif
#endif

/* Filename separator */
#ifndef SEP
#define SEP '/'
#endif

/* Max pathname length */
#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* Search path entry delimiter */
#ifndef DELIM
#define DELIM ':'
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_OSDEFS_H */
