#ifndef _Py_PATCHLEVEL_H
#define _Py_PATCHLEVEL_H
/* Python version identification scheme.

   When the major or minor version changes, the VERSION variable in
   configure.ac must also be changed.

   There is also (independent) API version information in modsupport.h.
*/

/* Values for PY_RELEASE_LEVEL */
#define PY_RELEASE_LEVEL_ALPHA  0xA
#define PY_RELEASE_LEVEL_BETA   0xB
#define PY_RELEASE_LEVEL_GAMMA  0xC     /* For release candidates */
#define PY_RELEASE_LEVEL_FINAL  0xF     /* Serial should be 0 here */
                                        /* Higher for patch releases */

/* Version parsed out into numeric values */
/*--start constants--*/
#define PY_MAJOR_VERSION        3
#define PY_MINOR_VERSION        15
#define PY_MICRO_VERSION        0
#define PY_RELEASE_LEVEL        PY_RELEASE_LEVEL_ALPHA
#define PY_RELEASE_SERIAL       0

/* Version as a string */
#define PY_VERSION              "3.15.0a0"
/*--end constants--*/


#define _Py_PACK_FULL_VERSION(X, Y, Z, LEVEL, SERIAL) ( \
    (((X) & 0xff) << 24) |                              \
    (((Y) & 0xff) << 16) |                              \
    (((Z) & 0xff) << 8) |                               \
    (((LEVEL) & 0xf) << 4) |                            \
    (((SERIAL) & 0xf) << 0))

/* Version as a single 4-byte hex number, e.g. 0x010502B2 == 1.5.2b2.
   Use this for numeric comparisons, e.g. #if PY_VERSION_HEX >= ... */
#define PY_VERSION_HEX _Py_PACK_FULL_VERSION( \
    PY_MAJOR_VERSION,                         \
    PY_MINOR_VERSION,                         \
    PY_MICRO_VERSION,                         \
    PY_RELEASE_LEVEL,                         \
    PY_RELEASE_SERIAL)

// Public Py_PACK_VERSION is declared in pymacro.h; it needs <inttypes.h>.

#endif //_Py_PATCHLEVEL_H
