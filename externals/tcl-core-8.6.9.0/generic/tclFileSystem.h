/*
 * tclFileSystem.h --
 *
 *	This file contains the common defintions and prototypes for use by
 *	Tcl's filesystem and path handling layers.
 *
 * Copyright (c) 2003 Vince Darley.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef _TCLFILESYSTEM
#define _TCLFILESYSTEM

#include "tcl.h"

/*
 * The internal TclFS API provides routines for handling and manipulating
 * paths efficiently, taking direct advantage of the "path" Tcl_Obj type.
 *
 * These functions are not exported at all at present.
 */

MODULE_SCOPE int	TclFSCwdPointerEquals(Tcl_Obj **pathPtrPtr);
MODULE_SCOPE int	TclFSNormalizeToUniquePath(Tcl_Interp *interp,
			    Tcl_Obj *pathPtr, int startAt);
MODULE_SCOPE Tcl_Obj *	TclFSMakePathRelative(Tcl_Interp *interp,
			    Tcl_Obj *pathPtr, Tcl_Obj *cwdPtr);
MODULE_SCOPE int	TclFSEnsureEpochOk(Tcl_Obj *pathPtr,
			    const Tcl_Filesystem **fsPtrPtr);
MODULE_SCOPE void	TclFSSetPathDetails(Tcl_Obj *pathPtr,
			    const Tcl_Filesystem *fsPtr, ClientData clientData);
MODULE_SCOPE Tcl_Obj *	TclFSNormalizeAbsolutePath(Tcl_Interp *interp,
			    Tcl_Obj *pathPtr);
MODULE_SCOPE size_t	TclFSEpoch(void);

/*
 * Private shared variables for use by tclIOUtil.c and tclPathObj.c
 */

MODULE_SCOPE const Tcl_Filesystem tclNativeFilesystem;

/*
 * Private shared functions for use by tclIOUtil.c, tclPathObj.c and
 * tclFileName.c, and any platform-specific filesystem code.
 */

MODULE_SCOPE Tcl_PathType TclFSGetPathType(Tcl_Obj *pathPtr,
			    const Tcl_Filesystem **filesystemPtrPtr,
			    int *driveNameLengthPtr);
MODULE_SCOPE Tcl_PathType TclFSNonnativePathType(const char *pathPtr,
			    int pathLen, const Tcl_Filesystem **filesystemPtrPtr,
			    int *driveNameLengthPtr, Tcl_Obj **driveNameRef);
MODULE_SCOPE Tcl_PathType TclGetPathType(Tcl_Obj *pathPtr,
			    const Tcl_Filesystem **filesystemPtrPtr,
			    int *driveNameLengthPtr, Tcl_Obj **driveNameRef);
MODULE_SCOPE int	TclFSEpochOk(size_t filesystemEpoch);
MODULE_SCOPE int	TclFSCwdIsNative(void);
MODULE_SCOPE Tcl_Obj *	TclWinVolumeRelativeNormalize(Tcl_Interp *interp,
			    const char *path, Tcl_Obj **useThisCwdPtr);

MODULE_SCOPE Tcl_FSPathInFilesystemProc TclNativePathInFilesystem;
MODULE_SCOPE Tcl_FSCreateInternalRepProc TclNativeCreateNativeRep;

#endif /* _TCLFILESYSTEM */

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
