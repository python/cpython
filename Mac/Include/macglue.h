#include <Files.h>
#include <Types.h>
#include <Resources.h>

char *macstrerror Py_PROTO((int));			/* strerror with mac errors */
PyObject *PyErr_Mac Py_PROTO((PyObject *, int));	/* Exception with a mac error */
int PyMac_Idle Py_PROTO((void));			/* Idle routine */
int PyMac_GetOSType Py_PROTO((PyObject *, ResType *));	/* argument parser for OSType */
int PyMac_GetStr255 Py_PROTO((PyObject *, Str255));	/* argument parser for Str255 */
int PyMac_GetFSSpec Py_PROTO((PyObject *, FSSpec *));	/* argument parser for FSSpec */
PyObject *PyMac_BuildFSSpec Py_PROTO((FSSpec *));	/* Convert FSSpec to PyObject */
PyObject *PyMac_BuildOSType Py_PROTO((OSType));		/* Convert OSType to PyObject */

#define GetOSType PyMac_GetOSType
#define GetStr255 PyMac_GetStr255
#define GetFSSpec PyMac_GetFSSpec
