/*
 * tclWinTest.c --
 *
 *	Contains commands for platform specific tests on Windows.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef USE_TCL_STUBS
#   define USE_TCL_STUBS
#endif
#include "tclInt.h"

/*
 * For TestplatformChmod on Windows
 */
#ifdef _WIN32
#include <aclapi.h>
#endif

/*
 * MinGW 3.4.2 does not define this.
 */
#ifndef INHERITED_ACE
#define INHERITED_ACE (0x10)
#endif

/*
 * Forward declarations of functions defined later in this file:
 */

static int		TesteventloopCmd(ClientData dummy, Tcl_Interp* interp,
			    int objc, Tcl_Obj *const objv[]);
static int		TestvolumetypeCmd(ClientData dummy,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		TestwinclockCmd(ClientData dummy, Tcl_Interp* interp,
			    int objc, Tcl_Obj *const objv[]);
static int		TestwinsleepCmd(ClientData dummy, Tcl_Interp* interp,
			    int objc, Tcl_Obj *const objv[]);
static Tcl_ObjCmdProc	TestExceptionCmd;
static int		TestplatformChmod(const char *nativePath, int pmode);
static int		TestchmodCmd(ClientData dummy, Tcl_Interp* interp,
			    int objc, Tcl_Obj *const objv[]);

/*
 *----------------------------------------------------------------------
 *
 * TclplatformtestInit --
 *
 *	Defines commands that test platform specific functionality for Windows
 *	platforms.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Defines new commands.
 *
 *----------------------------------------------------------------------
 */

int
TclplatformtestInit(
    Tcl_Interp *interp)		/* Interpreter to add commands to. */
{
    /*
     * Add commands for platform specific tests for Windows here.
     */

    Tcl_CreateObjCommand(interp, "testchmod", TestchmodCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "testeventloop", TesteventloopCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "testvolumetype", TestvolumetypeCmd,
	    NULL, NULL);
    Tcl_CreateObjCommand(interp, "testwinclock", TestwinclockCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "testwinsleep", TestwinsleepCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "testexcept", TestExceptionCmd, NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TesteventloopCmd --
 *
 *	This function implements the "testeventloop" command. It is used to
 *	test the Tcl notifier from an "external" event loop (i.e. not
 *	Tcl_DoOneEvent()).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TesteventloopCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static int *framePtr = NULL;/* Pointer to integer on stack frame of
				 * innermost invocation of the "wait"
				 * subcommand. */

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ...");
	return TCL_ERROR;
    }
    if (strcmp(Tcl_GetString(objv[1]), "done") == 0) {
	*framePtr = 1;
    } else if (strcmp(Tcl_GetString(objv[1]), "wait") == 0) {
	int *oldFramePtr, done;
	int oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);

	/*
	 * Save the old stack frame pointer and set up the current frame.
	 */

	oldFramePtr = framePtr;
	framePtr = &done;

	/*
	 * Enter a standard Windows event loop until the flag changes. Note
	 * that we do not explicitly call Tcl_ServiceEvent().
	 */

	done = 0;
	while (!done) {
	    MSG msg;

	    if (!GetMessage(&msg, NULL, 0, 0)) {
		/*
		 * The application is exiting, so repost the quit message and
		 * start unwinding.
		 */

		PostQuitMessage((int) msg.wParam);
		break;
	    }
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	(void) Tcl_SetServiceMode(oldMode);
	framePtr = oldFramePtr;
    } else {
	Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[1]),
		"\": must be done or wait", NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Testvolumetype --
 *
 *	This function implements the "testvolumetype" command. It is used to
 *	check the volume type (FAT, NTFS) of a volume.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestvolumetypeCmd(
    ClientData clientData,	/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
#define VOL_BUF_SIZE 32
    int found;
    char volType[VOL_BUF_SIZE];
    const char *path;

    if (objc > 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "?name?");
	return TCL_ERROR;
    }
    if (objc == 2) {
	/*
	 * path has to be really a proper volume, but we don't get query APIs
	 * for that until NT5
	 */

	path = Tcl_GetString(objv[1]);
    } else {
	path = NULL;
    }
    found = GetVolumeInformationA(path, NULL, 0, NULL, NULL, NULL, volType,
	    VOL_BUF_SIZE);

    if (found == 0) {
	Tcl_AppendResult(interp, "could not get volume type for \"",
		(path?path:""), "\"", NULL);
	TclWinConvertError(GetLastError());
	return TCL_ERROR;
    }
    Tcl_AppendResult(interp, volType, NULL);
    return TCL_OK;
#undef VOL_BUF_SIZE
}

/*
 *----------------------------------------------------------------------
 *
 * TestwinclockCmd --
 *
 *	Command that returns the seconds and microseconds portions of the
 *	system clock and of the Tcl clock so that they can be compared to
 *	validate that the Tcl clock is staying in sync.
 *
 * Usage:
 *	testclock
 *
 * Parameters:
 *	None.
 *
 * Results:
 *	Returns a standard Tcl result comprising a four-element list: the
 *	seconds and microseconds portions of the system clock, and the seconds
 *	and microseconds portions of the Tcl clock.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
TestwinclockCmd(
    ClientData dummy,		/* Unused */
    Tcl_Interp* interp,		/* Tcl interpreter */
    int objc,			/* Argument count */
    Tcl_Obj *const objv[])	/* Argument vector */
{
    static const FILETIME posixEpoch = { 0xD53E8000, 0x019DB1DE };
				/* The Posix epoch, expressed as a Windows
				 * FILETIME */
    Tcl_Time tclTime;		/* Tcl clock */
    FILETIME sysTime;		/* System clock */
    Tcl_Obj *result;		/* Result of the command */
    LARGE_INTEGER t1, t2;
    LARGE_INTEGER p1, p2;

    if (objc != 1) {
	Tcl_WrongNumArgs(interp, 1, objv, "");
	return TCL_ERROR;
    }

    QueryPerformanceCounter(&p1);

    Tcl_GetTime(&tclTime);
    GetSystemTimeAsFileTime(&sysTime);
    t1.LowPart = posixEpoch.dwLowDateTime;
    t1.HighPart = posixEpoch.dwHighDateTime;
    t2.LowPart = sysTime.dwLowDateTime;
    t2.HighPart = sysTime.dwHighDateTime;
    t2.QuadPart -= t1.QuadPart;

    QueryPerformanceCounter(&p2);

    result = Tcl_NewObj();
    Tcl_ListObjAppendElement(interp, result,
	    Tcl_NewIntObj((int) (t2.QuadPart / 10000000)));
    Tcl_ListObjAppendElement(interp, result,
	    Tcl_NewIntObj((int) ((t2.QuadPart / 10) % 1000000)));
    Tcl_ListObjAppendElement(interp, result, Tcl_NewIntObj(tclTime.sec));
    Tcl_ListObjAppendElement(interp, result, Tcl_NewIntObj(tclTime.usec));

    Tcl_ListObjAppendElement(interp, result, Tcl_NewWideIntObj(p1.QuadPart));
    Tcl_ListObjAppendElement(interp, result, Tcl_NewWideIntObj(p2.QuadPart));

    Tcl_SetObjResult(interp, result);

    return TCL_OK;
}

static int
TestwinsleepCmd(
    ClientData clientData,	/* Unused */
    Tcl_Interp* interp,		/* Tcl interpreter */
    int objc,			/* Parameter count */
    Tcl_Obj *const * objv)	/* Parameter vector */
{
    int ms;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "ms");
	return TCL_ERROR;
    }
    if (Tcl_GetIntFromObj(interp, objv[1], &ms) != TCL_OK) {
	return TCL_ERROR;
    }
    Sleep((DWORD) ms);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TestExceptionCmd --
 *
 *	Causes this process to end with the named exception. Used for testing
 *	Tcl_WaitPid().
 *
 * Usage:
 *	testexcept <type>
 *
 * Parameters:
 *	Type of exception.
 *
 * Results:
 *	None, this process closes now and doesn't return.
 *
 * Side effects:
 *	This Tcl process closes, hard... Bang!
 *
 *----------------------------------------------------------------------
 */

static int
TestExceptionCmd(
    ClientData dummy,			/* Unused */
    Tcl_Interp* interp,			/* Tcl interpreter */
    int objc,				/* Argument count */
    Tcl_Obj *const objv[])		/* Argument vector */
{
    static const char *const cmds[] = {
	"access_violation", "datatype_misalignment", "array_bounds",
	"float_denormal", "float_divbyzero", "float_inexact",
	"float_invalidop", "float_overflow", "float_stack", "float_underflow",
	"int_divbyzero", "int_overflow", "private_instruction", "inpageerror",
	"illegal_instruction", "noncontinue", "stack_overflow",
	"invalid_disp", "guard_page", "invalid_handle", "ctrl+c",
	NULL
    };
    static const DWORD exceptions[] = {
	EXCEPTION_ACCESS_VIOLATION, EXCEPTION_DATATYPE_MISALIGNMENT,
	EXCEPTION_ARRAY_BOUNDS_EXCEEDED, EXCEPTION_FLT_DENORMAL_OPERAND,
	EXCEPTION_FLT_DIVIDE_BY_ZERO, EXCEPTION_FLT_INEXACT_RESULT,
	EXCEPTION_FLT_INVALID_OPERATION, EXCEPTION_FLT_OVERFLOW,
	EXCEPTION_FLT_STACK_CHECK, EXCEPTION_FLT_UNDERFLOW,
	EXCEPTION_INT_DIVIDE_BY_ZERO, EXCEPTION_INT_OVERFLOW,
	EXCEPTION_PRIV_INSTRUCTION, EXCEPTION_IN_PAGE_ERROR,
	EXCEPTION_ILLEGAL_INSTRUCTION, EXCEPTION_NONCONTINUABLE_EXCEPTION,
	EXCEPTION_STACK_OVERFLOW, EXCEPTION_INVALID_DISPOSITION,
	EXCEPTION_GUARD_PAGE, EXCEPTION_INVALID_HANDLE, CONTROL_C_EXIT
    };
    int cmd;

    if (objc != 2) {
	Tcl_WrongNumArgs(interp, 0, objv, "<type-of-exception>");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], cmds, "command", 0,
	    &cmd) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Make sure the GPF dialog doesn't popup.
     */

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

    /*
     * As Tcl does not handle structured exceptions, this falls all the way
     * back up the instruction stack to the C run-time portion that called
     * main() where the process will now be terminated with this exception
     * code by the default handler the C run-time provides.
     */

    /* SMASH! */
    RaiseException(exceptions[cmd], EXCEPTION_NONCONTINUABLE, 0, NULL);

    /* NOTREACHED */
    return TCL_OK;
}

static int
TestplatformChmod(
    const char *nativePath,
    int pmode)
{
    static const SECURITY_INFORMATION infoBits = OWNER_SECURITY_INFORMATION
	    | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
    static const DWORD readOnlyMask = FILE_DELETE_CHILD | FILE_ADD_FILE
	    | FILE_ADD_SUBDIRECTORY | FILE_WRITE_EA | FILE_APPEND_DATA
	    | FILE_WRITE_DATA | DELETE;

    /*
     * References to security functions (only available on NT and later).
     */

    const BOOL set_readOnly = !(pmode & 0222);
    BOOL acl_readOnly_found = FALSE, curAclPresent, curAclDefaulted;
    SID_IDENTIFIER_AUTHORITY userSidAuthority = {
	SECURITY_WORLD_SID_AUTHORITY
    };
    BYTE *secDesc = 0;
    DWORD secDescLen, attr, newAclSize;
    ACL_SIZE_INFORMATION ACLSize;
    PACL curAcl, newAcl = 0;
    WORD j;
    SID *userSid = 0;
    char *userDomain = 0;
    int res = 0;

    /*
     * Process the chmod request.
     */

    attr = GetFileAttributesA(nativePath);

    /*
     * nativePath not found
     */

    if (attr == 0xffffffff) {
	res = -1;
	goto done;
    }

    /*
     * If nativePath is not a directory, there is no special handling.
     */

    if (!(attr & FILE_ATTRIBUTE_DIRECTORY)) {
	goto done;
    }

    /*
     * Set the result to error, if the ACL change is successful it will be
     * reset to 0.
     */

    res = -1;

    /*
     * Read the security descriptor for the directory. Note the first call
     * obtains the size of the security descriptor.
     */

    if (!GetFileSecurityA(nativePath, infoBits, NULL, 0, &secDescLen)) {
	DWORD secDescLen2 = 0;

	if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
	    goto done;
	}

	secDesc = ckalloc(secDescLen);
	if (!GetFileSecurityA(nativePath, infoBits,
		(PSECURITY_DESCRIPTOR) secDesc, secDescLen, &secDescLen2)
		|| (secDescLen < secDescLen2)) {
	    goto done;
	}
    }

    /*
     * Get the World SID.
     */

    userSid = ckalloc(GetSidLengthRequired((UCHAR) 1));
    InitializeSid(userSid, &userSidAuthority, (BYTE) 1);
    *(GetSidSubAuthority(userSid, 0)) = SECURITY_WORLD_RID;

    /*
     * If curAclPresent == false then curAcl and curAclDefaulted not valid.
     */

    if (!GetSecurityDescriptorDacl((PSECURITY_DESCRIPTOR) secDesc,
	    &curAclPresent, &curAcl, &curAclDefaulted)) {
	goto done;
    }
    if (!curAclPresent || !curAcl) {
	ACLSize.AclBytesInUse = 0;
	ACLSize.AceCount = 0;
    } else if (!GetAclInformation(curAcl, &ACLSize, sizeof(ACLSize),
	    AclSizeInformation)) {
	goto done;
    }

    /*
     * Allocate memory for the new ACL.
     */

    newAclSize = ACLSize.AclBytesInUse + sizeof(ACCESS_DENIED_ACE)
	    + GetLengthSid(userSid) - sizeof(DWORD);
    newAcl = ckalloc(newAclSize);

    /*
     * Initialize the new ACL.
     */

    if (!InitializeAcl(newAcl, newAclSize, ACL_REVISION)) {
	goto done;
    }

    /*
     * Add denied to make readonly, this will be known as a "read-only tag".
     */

    if (set_readOnly && !AddAccessDeniedAce(newAcl, ACL_REVISION,
	    readOnlyMask, userSid)) {
	goto done;
    }

    acl_readOnly_found = FALSE;
    for (j = 0; j < ACLSize.AceCount; j++) {
	LPVOID pACE2;
	ACE_HEADER *phACE2;

	if (!GetAce(curAcl, j, &pACE2)) {
	    goto done;
	}

	phACE2 = (ACE_HEADER *) pACE2;

	/*
	 * Do NOT propagate inherited ACEs.
	 */

	if (phACE2->AceFlags & INHERITED_ACE) {
	    continue;
	}

	/*
	 * Skip the "read-only tag" restriction (either added above, or it is
	 * being removed).
	 */

	if (phACE2->AceType == ACCESS_DENIED_ACE_TYPE) {
	    ACCESS_DENIED_ACE *pACEd = (ACCESS_DENIED_ACE *) phACE2;

	    if (pACEd->Mask == readOnlyMask
		    && EqualSid(userSid, (PSID) &pACEd->SidStart)) {
		acl_readOnly_found = TRUE;
		continue;
	    }
	}

	/*
	 * Copy the current ACE from the old to the new ACL.
	 */

	if (!AddAce(newAcl, ACL_REVISION, MAXDWORD, (PACL *) pACE2,
		((PACE_HEADER) pACE2)->AceSize)) {
	    goto done;
	}
    }

    /*
     * Apply the new ACL.
     */

    if (set_readOnly == acl_readOnly_found || SetNamedSecurityInfoA(
	    (LPSTR) nativePath, SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
	    NULL, NULL, newAcl, NULL) == ERROR_SUCCESS) {
	res = 0;
    }

  done:
    if (secDesc) {
	ckfree(secDesc);
    }
    if (newAcl) {
	ckfree(newAcl);
    }
    if (userSid) {
	ckfree(userSid);
    }
    if (userDomain) {
	ckfree(userDomain);
    }

    if (res != 0) {
	return res;
    }

    /*
     * Run normal chmod command.
     */

    return chmod(nativePath, pmode);
}

/*
 *---------------------------------------------------------------------------
 *
 * TestchmodCmd --
 *
 *	Implements the "testchmod" cmd. Used when testing "file" command. The
 *	only attribute used by the Windows platform is the user write flag; if
 *	this is not set, the file is made read-only. Otherwise, the file is
 *	made read-write.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Changes permissions of specified files.
 *
 *---------------------------------------------------------------------------
 */

static int
TestchmodCmd(
    ClientData dummy,		/* Not used. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Parameter count */
    Tcl_Obj *const * objv)	/* Parameter vector */
{
    int i, mode;

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "mode file ?file ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIntFromObj(interp, objv[1], &mode) != TCL_OK) {
	return TCL_ERROR;
    }

    for (i = 2; i < objc; i++) {
	Tcl_DString buffer;
	const char *translated;

	translated = Tcl_TranslateFileName(interp, Tcl_GetString(objv[i]), &buffer);
	if (translated == NULL) {
	    return TCL_ERROR;
	}
	if (TestplatformChmod(translated, mode) != 0) {
	    Tcl_AppendResult(interp, translated, ": ", Tcl_PosixError(interp),
		    NULL);
	    return TCL_ERROR;
	}
	Tcl_DStringFree(&buffer);
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
