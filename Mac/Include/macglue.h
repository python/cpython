#include <Files.h>
#include <Types.h>
#include <Resources.h>

char *macstrerror PROTO((int));			/* strerror with mac errors */
object *PyErr_Mac PROTO((object *, int));	/* Exception with a mac error */
int PyMac_Idle PROTO((void));			/* Idle routine */
int GetOSType PROTO((object *, ResType *));	/* argument parser for OSType */
int GetStr255 PROTO((object *, Str255));	/* argument parser for Str255 */
int GetFSSpec PROTO((object *, FSSpec *));	/* argument parser for FSSpec */
object *PyMac_BuildFSSpec PROTO((FSSpec *));	/* Convert FSSpec to python object */

