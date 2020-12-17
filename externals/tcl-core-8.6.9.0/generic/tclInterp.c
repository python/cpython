/*
 * tclInterp.c --
 *
 *	This file implements the "interp" command which allows creation and
 *	manipulation of Tcl interpreters from within Tcl scripts.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 2004 Donal K. Fellows
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * A pointer to a string that holds an initialization script that if non-NULL
 * is evaluated in Tcl_Init() prior to the built-in initialization script
 * above. This variable can be modified by the function below.
 */

static const char *tclPreInitScript = NULL;

/* Forward declaration */
struct Target;

/*
 * struct Alias:
 *
 * Stores information about an alias. Is stored in the slave interpreter and
 * used by the source command to find the target command in the master when
 * the source command is invoked.
 */

typedef struct Alias {
    Tcl_Obj *token;		/* Token for the alias command in the slave
				 * interp. This used to be the command name in
				 * the slave when the alias was first
				 * created. */
    Tcl_Interp *targetInterp;	/* Interp in which target command will be
				 * invoked. */
    Tcl_Command slaveCmd;	/* Source command in slave interpreter, bound
				 * to command that invokes the target command
				 * in the target interpreter. */
    Tcl_HashEntry *aliasEntryPtr;
				/* Entry for the alias hash table in slave.
				 * This is used by alias deletion to remove
				 * the alias from the slave interpreter alias
				 * table. */
    struct Target *targetPtr;	/* Entry for target command in master. This is
				 * used in the master interpreter to map back
				 * from the target command to aliases
				 * redirecting to it. */
    int objc;			/* Count of Tcl_Obj in the prefix of the
				 * target command to be invoked in the target
				 * interpreter. Additional arguments specified
				 * when calling the alias in the slave interp
				 * will be appended to the prefix before the
				 * command is invoked. */
    Tcl_Obj *objPtr;		/* The first actual prefix object - the target
				 * command name; this has to be at the end of
				 * the structure, which will be extended to
				 * accomodate the remaining objects in the
				 * prefix. */
} Alias;

/*
 *
 * struct Slave:
 *
 * Used by the "interp" command to record and find information about slave
 * interpreters. Maps from a command name in the master to information about a
 * slave interpreter, e.g. what aliases are defined in it.
 */

typedef struct Slave {
    Tcl_Interp *masterInterp;	/* Master interpreter for this slave. */
    Tcl_HashEntry *slaveEntryPtr;
				/* Hash entry in masters slave table for this
				 * slave interpreter. Used to find this
				 * record, and used when deleting the slave
				 * interpreter to delete it from the master's
				 * table. */
    Tcl_Interp	*slaveInterp;	/* The slave interpreter. */
    Tcl_Command interpCmd;	/* Interpreter object command. */
    Tcl_HashTable aliasTable;	/* Table which maps from names of commands in
				 * slave interpreter to struct Alias defined
				 * below. */
} Slave;

/*
 * struct Target:
 *
 * Maps from master interpreter commands back to the source commands in slave
 * interpreters. This is needed because aliases can be created between sibling
 * interpreters and must be deleted when the target interpreter is deleted. In
 * case they would not be deleted the source interpreter would be left with a
 * "dangling pointer". One such record is stored in the Master record of the
 * master interpreter with the master for each alias which directs to a
 * command in the master. These records are used to remove the source command
 * for an from a slave if/when the master is deleted. They are organized in a
 * doubly-linked list attached to the master interpreter.
 */

typedef struct Target {
    Tcl_Command	slaveCmd;	/* Command for alias in slave interp. */
    Tcl_Interp *slaveInterp;	/* Slave Interpreter. */
    struct Target *nextPtr;	/* Next in list of target records, or NULL if
				 * at the end of the list of targets. */
    struct Target *prevPtr;	/* Previous in list of target records, or NULL
				 * if at the start of the list of targets. */
} Target;

/*
 * struct Master:
 *
 * This record is used for two purposes: First, slaveTable (a hashtable) maps
 * from names of commands to slave interpreters. This hashtable is used to
 * store information about slave interpreters of this interpreter, to map over
 * all slaves, etc. The second purpose is to store information about all
 * aliases in slaves (or siblings) which direct to target commands in this
 * interpreter (using the targetsPtr doubly-linked list).
 *
 * NB: the flags field in the interp structure, used with SAFE_INTERP mask
 * denotes whether the interpreter is safe or not. Safe interpreters have
 * restricted functionality, can only create safe slave interpreters and can
 * only load safe extensions.
 */

typedef struct Master {
    Tcl_HashTable slaveTable;	/* Hash table for slave interpreters. Maps
				 * from command names to Slave records. */
    Target *targetsPtr;		/* The head of a doubly-linked list of all the
				 * target records which denote aliases from
				 * slaves or sibling interpreters that direct
				 * to commands in this interpreter. This list
				 * is used to remove dangling pointers from
				 * the slave (or sibling) interpreters when
				 * this interpreter is deleted. */
} Master;

/*
 * The following structure keeps track of all the Master and Slave information
 * on a per-interp basis.
 */

typedef struct InterpInfo {
    Master master;		/* Keeps track of all interps for which this
				 * interp is the Master. */
    Slave slave;		/* Information necessary for this interp to
				 * function as a slave. */
} InterpInfo;

/*
 * Limit callbacks handled by scripts are modelled as structures which are
 * stored in hashes indexed by a two-word key. Note that the type of the
 * 'type' field in the key is not int; this is to make sure that things are
 * likely to work properly on 64-bit architectures.
 */

typedef struct ScriptLimitCallback {
    Tcl_Interp *interp;		/* The interpreter in which to execute the
				 * callback. */
    Tcl_Obj *scriptObj;		/* The script to execute to perform the
				 * user-defined part of the callback. */
    int type;			/* What kind of callback is this. */
    Tcl_HashEntry *entryPtr;	/* The entry in the hash table maintained by
				 * the target interpreter that refers to this
				 * callback record, or NULL if the entry has
				 * already been deleted from that hash
				 * table. */
} ScriptLimitCallback;

typedef struct ScriptLimitCallbackKey {
    Tcl_Interp *interp;		/* The interpreter that the limit callback was
				 * attached to. This is not the interpreter
				 * that the callback runs in! */
    long type;			/* The type of callback that this is. */
} ScriptLimitCallbackKey;

/*
 * TIP#143 limit handler internal representation.
 */

struct LimitHandler {
    int flags;			/* The state of this particular handler. */
    Tcl_LimitHandlerProc *handlerProc;
				/* The handler callback. */
    ClientData clientData;	/* Opaque argument to the handler callback. */
    Tcl_LimitHandlerDeleteProc *deleteProc;
				/* How to delete the clientData. */
    LimitHandler *prevPtr;	/* Previous item in linked list of
				 * handlers. */
    LimitHandler *nextPtr;	/* Next item in linked list of handlers. */
};

/*
 * Values for the LimitHandler flags field.
 *      LIMIT_HANDLER_ACTIVE - Whether the handler is currently being
 *              processed; handlers are never to be entered reentrantly.
 *      LIMIT_HANDLER_DELETED - Whether the handler has been deleted. This
 *              should not normally be observed because when a handler is
 *              deleted it is also spliced out of the list of handlers, but
 *              even so we will be careful.
 */

#define LIMIT_HANDLER_ACTIVE    0x01
#define LIMIT_HANDLER_DELETED   0x02



/*
 * Prototypes for local static functions:
 */

static int		AliasCreate(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Interp *masterInterp,
			    Tcl_Obj *namePtr, Tcl_Obj *targetPtr, int objc,
			    Tcl_Obj *const objv[]);
static int		AliasDelete(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Obj *namePtr);
static int		AliasDescribe(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, Tcl_Obj *objPtr);
static int		AliasList(Tcl_Interp *interp, Tcl_Interp *slaveInterp);
static int		AliasObjCmd(ClientData dummy,
			    Tcl_Interp *currentInterp, int objc,
			    Tcl_Obj *const objv[]);
static int		AliasNRCmd(ClientData dummy,
			    Tcl_Interp *currentInterp, int objc,
			    Tcl_Obj *const objv[]);
static void		AliasObjCmdDeleteProc(ClientData clientData);
static Tcl_Interp *	GetInterp(Tcl_Interp *interp, Tcl_Obj *pathPtr);
static Tcl_Interp *	GetInterp2(Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		InterpInfoDeleteProc(ClientData clientData,
			    Tcl_Interp *interp);
static int		SlaveBgerror(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *const objv[]);
static Tcl_Interp *	SlaveCreate(Tcl_Interp *interp, Tcl_Obj *pathPtr,
			    int safe);
static int		SlaveDebugCmd(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp,
			    int objc, Tcl_Obj *const objv[]);
static int		SlaveEval(Tcl_Interp *interp, Tcl_Interp *slaveInterp,
			    int objc, Tcl_Obj *const objv[]);
static int		SlaveExpose(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *const objv[]);
static int		SlaveHide(Tcl_Interp *interp, Tcl_Interp *slaveInterp,
			    int objc, Tcl_Obj *const objv[]);
static int		SlaveHidden(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp);
static int		SlaveInvokeHidden(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp,
			    const char *namespaceName,
			    int objc, Tcl_Obj *const objv[]);
static int		SlaveMarkTrusted(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp);
static int		SlaveObjCmd(ClientData dummy, Tcl_Interp *interp,
			    int objc, Tcl_Obj *const objv[]);
static void		SlaveObjCmdDeleteProc(ClientData clientData);
static int		SlaveRecursionLimit(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int objc,
			    Tcl_Obj *const objv[]);
static int		SlaveCommandLimitCmd(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int consumedObjc,
			    int objc, Tcl_Obj *const objv[]);
static int		SlaveTimeLimitCmd(Tcl_Interp *interp,
			    Tcl_Interp *slaveInterp, int consumedObjc,
			    int objc, Tcl_Obj *const objv[]);
static void		InheritLimitsFromMaster(Tcl_Interp *slaveInterp,
			    Tcl_Interp *masterInterp);
static void		SetScriptLimitCallback(Tcl_Interp *interp, int type,
			    Tcl_Interp *targetInterp, Tcl_Obj *scriptObj);
static void		CallScriptLimitCallback(ClientData clientData,
			    Tcl_Interp *interp);
static void		DeleteScriptLimitCallback(ClientData clientData);
static void		RunLimitHandlers(LimitHandler *handlerPtr,
			    Tcl_Interp *interp);
static void		TimeLimitCallback(ClientData clientData);

/* NRE enabling */
static Tcl_NRPostProc	NRPostInvokeHidden;
static Tcl_ObjCmdProc	NRInterpCmd;
static Tcl_ObjCmdProc	NRSlaveCmd;


/*
 *----------------------------------------------------------------------
 *
 * TclSetPreInitScript --
 *
 *	This routine is used to change the value of the internal variable,
 *	tclPreInitScript.
 *
 * Results:
 *	Returns the current value of tclPreInitScript.
 *
 * Side effects:
 *	Changes the way Tcl_Init() routine behaves.
 *
 *----------------------------------------------------------------------
 */

const char *
TclSetPreInitScript(
    const char *string)		/* Pointer to a script. */
{
    const char *prevString = tclPreInitScript;
    tclPreInitScript = string;
    return(prevString);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_Init --
 *
 *	This function is typically invoked by Tcl_AppInit functions to find
 *	and source the "init.tcl" script, which should exist somewhere on the
 *	Tcl library path.
 *
 * Results:
 *	Returns a standard Tcl completion code and sets the interp's result if
 *	there is an error.
 *
 * Side effects:
 *	Depends on what's in the init.tcl script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_Init(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
    if (tclPreInitScript != NULL) {
	if (Tcl_Eval(interp, tclPreInitScript) == TCL_ERROR) {
	    return TCL_ERROR;
	}
    }

    /*
     * In order to find init.tcl during initialization, the following script
     * is invoked by Tcl_Init(). It looks in several different directories:
     *
     *	$tcl_library		- can specify a primary location, if set, no
     *				  other locations will be checked. This is the
     *				  recommended way for a program that embeds
     *				  Tcl to specifically tell Tcl where to find
     *				  an init.tcl file.
     *
     *	$env(TCL_LIBRARY)	- highest priority so user can always override
     *				  the search path unless the application has
     *				  specified an exact directory above
     *
     *	$tclDefaultLibrary	- INTERNAL: This variable is set by Tcl on
     *				  those platforms where it can determine at
     *				  runtime the directory where it expects the
     *				  init.tcl file to be. After [tclInit] reads
     *				  and uses this value, it [unset]s it.
     *				  External users of Tcl should not make use of
     *				  the variable to customize [tclInit].
     *
     *	$tcl_libPath		- OBSOLETE: This variable is no longer set by
     *				  Tcl itself, but [tclInit] examines it in
     *				  case some program that embeds Tcl is
     *				  customizing [tclInit] by setting this
     *				  variable to a list of directories in which
     *				  to search.
     *
     *	[tcl::pkgconfig get scriptdir,runtime]
     *				- the directory determined by configure to be
     *				  the place where Tcl's script library is to
     *				  be installed.
     *
     * The first directory on this path that contains a valid init.tcl script
     * will be set as the value of tcl_library.
     *
     * Note that this entire search mechanism can be bypassed by defining an
     * alternate tclInit command before calling Tcl_Init().
     */

    return Tcl_Eval(interp,
"if {[namespace which -command tclInit] eq \"\"} {\n"
"  proc tclInit {} {\n"
"    global tcl_libPath tcl_library env tclDefaultLibrary\n"
"    rename tclInit {}\n"
"    if {[info exists tcl_library]} {\n"
"	set scripts {{set tcl_library}}\n"
"    } else {\n"
"	set scripts {}\n"
"	if {[info exists env(TCL_LIBRARY)] && ($env(TCL_LIBRARY) ne {})} {\n"
"	    lappend scripts {set env(TCL_LIBRARY)}\n"
"	    lappend scripts {\n"
"if {[regexp ^tcl(.*)$ [file tail $env(TCL_LIBRARY)] -> tail] == 0} continue\n"
"if {$tail eq [info tclversion]} continue\n"
"file join [file dirname $env(TCL_LIBRARY)] tcl[info tclversion]}\n"
"	}\n"
"	if {[info exists tclDefaultLibrary]} {\n"
"	    lappend scripts {set tclDefaultLibrary}\n"
"	} else {\n"
"	    lappend scripts {::tcl::pkgconfig get scriptdir,runtime}\n"
"	}\n"
"	lappend scripts {\n"
"set parentDir [file dirname [file dirname [info nameofexecutable]]]\n"
"set grandParentDir [file dirname $parentDir]\n"
"file join $parentDir lib tcl[info tclversion]} \\\n"
"	{file join $grandParentDir lib tcl[info tclversion]} \\\n"
"	{file join $parentDir library} \\\n"
"	{file join $grandParentDir library} \\\n"
"	{file join $grandParentDir tcl[info patchlevel] library} \\\n"
"	{\n"
"file join [file dirname $grandParentDir] tcl[info patchlevel] library}\n"
"	if {[info exists tcl_libPath]\n"
"		&& [catch {llength $tcl_libPath} len] == 0} {\n"
"	    for {set i 0} {$i < $len} {incr i} {\n"
"		lappend scripts [list lindex \\$tcl_libPath $i]\n"
"	    }\n"
"	}\n"
"    }\n"
"    set dirs {}\n"
"    set errors {}\n"
"    foreach script $scripts {\n"
"	lappend dirs [eval $script]\n"
"	set tcl_library [lindex $dirs end]\n"
"	set tclfile [file join $tcl_library init.tcl]\n"
"	if {[file exists $tclfile]} {\n"
"	    if {[catch {uplevel #0 [list source $tclfile]} msg opts]} {\n"
"		append errors \"$tclfile: $msg\n\"\n"
"		append errors \"[dict get $opts -errorinfo]\n\"\n"
"		continue\n"
"	    }\n"
"	    unset -nocomplain tclDefaultLibrary\n"
"	    return\n"
"	}\n"
"    }\n"
"    unset -nocomplain tclDefaultLibrary\n"
"    set msg \"Can't find a usable init.tcl in the following directories: \n\"\n"
"    append msg \"    $dirs\n\n\"\n"
"    append msg \"$errors\n\n\"\n"
"    append msg \"This probably means that Tcl wasn't installed properly.\n\"\n"
"    error $msg\n"
"  }\n"
"}\n"
"tclInit");
}

/*
 *---------------------------------------------------------------------------
 *
 * TclInterpInit --
 *
 *	Initializes the invoking interpreter for using the master, slave and
 *	safe interp facilities. This is called from inside Tcl_CreateInterp().
 *
 * Results:
 *	Always returns TCL_OK for backwards compatibility.
 *
 * Side effects:
 *	Adds the "interp" command to an interpreter and initializes the
 *	interpInfoPtr field of the invoking interpreter.
 *
 *---------------------------------------------------------------------------
 */

int
TclInterpInit(
    Tcl_Interp *interp)		/* Interpreter to initialize. */
{
    InterpInfo *interpInfoPtr;
    Master *masterPtr;
    Slave *slavePtr;

    interpInfoPtr = ckalloc(sizeof(InterpInfo));
    ((Interp *) interp)->interpInfo = interpInfoPtr;

    masterPtr = &interpInfoPtr->master;
    Tcl_InitHashTable(&masterPtr->slaveTable, TCL_STRING_KEYS);
    masterPtr->targetsPtr = NULL;

    slavePtr = &interpInfoPtr->slave;
    slavePtr->masterInterp	= NULL;
    slavePtr->slaveEntryPtr	= NULL;
    slavePtr->slaveInterp	= interp;
    slavePtr->interpCmd		= NULL;
    Tcl_InitHashTable(&slavePtr->aliasTable, TCL_STRING_KEYS);

    Tcl_NRCreateCommand(interp, "interp", Tcl_InterpObjCmd, NRInterpCmd,
	    NULL, NULL);

    Tcl_CallWhenDeleted(interp, InterpInfoDeleteProc, NULL);
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * InterpInfoDeleteProc --
 *
 *	Invoked when an interpreter is being deleted. It releases all storage
 *	used by the master/slave/safe interpreter facilities.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up storage. Sets the interpInfoPtr field of the interp to NULL.
 *
 *---------------------------------------------------------------------------
 */

static void
InterpInfoDeleteProc(
    ClientData clientData,	/* Ignored. */
    Tcl_Interp *interp)		/* Interp being deleted. All commands for
				 * slave interps should already be deleted. */
{
    InterpInfo *interpInfoPtr;
    Slave *slavePtr;
    Master *masterPtr;
    Target *targetPtr;

    interpInfoPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;

    /*
     * There shouldn't be any commands left.
     */

    masterPtr = &interpInfoPtr->master;
    if (masterPtr->slaveTable.numEntries != 0) {
	Tcl_Panic("InterpInfoDeleteProc: still exist commands");
    }
    Tcl_DeleteHashTable(&masterPtr->slaveTable);

    /*
     * Tell any interps that have aliases to this interp that they should
     * delete those aliases. If the other interp was already dead, it would
     * have removed the target record already.
     */

    for (targetPtr = masterPtr->targetsPtr; targetPtr != NULL; ) {
	Target *tmpPtr = targetPtr->nextPtr;
	Tcl_DeleteCommandFromToken(targetPtr->slaveInterp,
		targetPtr->slaveCmd);
	targetPtr = tmpPtr;
    }

    slavePtr = &interpInfoPtr->slave;
    if (slavePtr->interpCmd != NULL) {
	/*
	 * Tcl_DeleteInterp() was called on this interpreter, rather "interp
	 * delete" or the equivalent deletion of the command in the master.
	 * First ensure that the cleanup callback doesn't try to delete the
	 * interp again.
	 */

	slavePtr->slaveInterp = NULL;
	Tcl_DeleteCommandFromToken(slavePtr->masterInterp,
		slavePtr->interpCmd);
    }

    /*
     * There shouldn't be any aliases left.
     */

    if (slavePtr->aliasTable.numEntries != 0) {
	Tcl_Panic("InterpInfoDeleteProc: still exist aliases");
    }
    Tcl_DeleteHashTable(&slavePtr->aliasTable);

    ckfree(interpInfoPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InterpObjCmd --
 *
 *	This function is invoked to process the "interp" Tcl command. See the
 *	user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */
	/* ARGSUSED */
int
Tcl_InterpObjCmd(
    ClientData clientData,		/* Unused. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, NRInterpCmd, clientData, objc, objv);
}

static int
NRInterpCmd(
    ClientData clientData,		/* Unused. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *const objv[])		/* Argument objects. */
{
    Tcl_Interp *slaveInterp;
    int index;
    static const char *const options[] = {
	"alias",	"aliases",	"bgerror",	"cancel",
	"create",	"debug",	"delete",
	"eval",		"exists",	"expose",
	"hide",		"hidden",	"issafe",
	"invokehidden",	"limit",	"marktrusted",	"recursionlimit",
	"slaves",	"share",	"target",	"transfer",
	NULL
    };
    enum option {
	OPT_ALIAS,	OPT_ALIASES,	OPT_BGERROR,	OPT_CANCEL,
	OPT_CREATE,	OPT_DEBUG,	OPT_DELETE,
	OPT_EVAL,	OPT_EXISTS,	OPT_EXPOSE,
	OPT_HIDE,	OPT_HIDDEN,	OPT_ISSAFE,
	OPT_INVOKEHID,	OPT_LIMIT,	OPT_MARKTRUSTED,OPT_RECLIMIT,
	OPT_SLAVES,	OPT_SHARE,	OPT_TARGET,	OPT_TRANSFER
    };

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }
    switch ((enum option) index) {
    case OPT_ALIAS: {
	Tcl_Interp *masterInterp;

	if (objc < 4) {
	aliasArgs:
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "slavePath slaveCmd ?masterPath masterCmd? ?arg ...?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	if (objc == 4) {
	    return AliasDescribe(interp, slaveInterp, objv[3]);
	}
	if ((objc == 5) && (TclGetString(objv[4])[0] == '\0')) {
	    return AliasDelete(interp, slaveInterp, objv[3]);
	}
	if (objc > 5) {
	    masterInterp = GetInterp(interp, objv[4]);
	    if (masterInterp == NULL) {
		return TCL_ERROR;
	    }

	    return AliasCreate(interp, slaveInterp, masterInterp, objv[3],
		    objv[5], objc - 6, objv + 6);
	}
	goto aliasArgs;
    }
    case OPT_ALIASES:
	slaveInterp = GetInterp2(interp, objc, objv);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return AliasList(interp, slaveInterp);
    case OPT_BGERROR:
	if (objc != 3 && objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path ?cmdPrefix?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveBgerror(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_CANCEL: {
	int i, flags;
	Tcl_Obj *resultObjPtr;
	static const char *const cancelOptions[] = {
	    "-unwind",	"--",	NULL
	};
	enum option {
	    OPT_UNWIND,	OPT_LAST
	};

	flags = 0;

	for (i = 2; i < objc; i++) {
	    if (TclGetString(objv[i])[0] != '-') {
		break;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[i], cancelOptions, "option",
		    0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }

	    switch ((enum option) index) {
	    case OPT_UNWIND:
		/*
		 * The evaluation stack in the target interp is to be unwound.
		 */

		flags |= TCL_CANCEL_UNWIND;
		break;
	    case OPT_LAST:
		i++;
		goto endOfForLoop;
	    }
	}

    endOfForLoop:
	if (i < objc - 2) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-unwind? ?--? ?path? ?result?");
	    return TCL_ERROR;
	}

	/*
	 * Did they specify a slave interp to cancel the script in progress
	 * in?  If not, use the current interp.
	 */

	if (i < objc) {
	    slaveInterp = GetInterp(interp, objv[i]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    }
	    i++;
	} else {
	    slaveInterp = interp;
	}

	if (i < objc) {
	    resultObjPtr = objv[i];

	    /*
	     * Tcl_CancelEval removes this reference.
	     */

	    Tcl_IncrRefCount(resultObjPtr);
	    i++;
	} else {
	    resultObjPtr = NULL;
	}

	return Tcl_CancelEval(slaveInterp, resultObjPtr, 0, flags);
    }
    case OPT_CREATE: {
	int i, last, safe;
	Tcl_Obj *slavePtr;
	char buf[16 + TCL_INTEGER_SPACE];
	static const char *const createOptions[] = {
	    "-safe",	"--", NULL
	};
	enum option {
	    OPT_SAFE,	OPT_LAST
	};

	safe = Tcl_IsSafe(interp);

	/*
	 * Weird historical rules: "-safe" is accepted at the end, too.
	 */

	slavePtr = NULL;
	last = 0;
	for (i = 2; i < objc; i++) {
	    if ((last == 0) && (Tcl_GetString(objv[i])[0] == '-')) {
		if (Tcl_GetIndexFromObj(interp, objv[i], createOptions,
			"option", 0, &index) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (index == OPT_SAFE) {
		    safe = 1;
		    continue;
		}
		i++;
		last = 1;
	    }
	    if (slavePtr != NULL) {
		Tcl_WrongNumArgs(interp, 2, objv, "?-safe? ?--? ?path?");
		return TCL_ERROR;
	    }
	    if (i < objc) {
		slavePtr = objv[i];
	    }
	}
	buf[0] = '\0';
	if (slavePtr == NULL) {
	    /*
	     * Create an anonymous interpreter -- we choose its name and the
	     * name of the command. We check that the command name that we use
	     * for the interpreter does not collide with an existing command
	     * in the master interpreter.
	     */

	    for (i = 0; ; i++) {
		Tcl_CmdInfo cmdInfo;

		sprintf(buf, "interp%d", i);
		if (Tcl_GetCommandInfo(interp, buf, &cmdInfo) == 0) {
		    break;
		}
	    }
	    slavePtr = Tcl_NewStringObj(buf, -1);
	}
	if (SlaveCreate(interp, slavePtr, safe) == NULL) {
	    if (buf[0] != '\0') {
		Tcl_DecrRefCount(slavePtr);
	    }
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, slavePtr);
	return TCL_OK;
    }
    case OPT_DEBUG:		/* TIP #378 */
	/*
	 * Currently only -frame supported, otherwise ?-option ?value??
	 */

	if (objc < 3 || objc > 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path ?-frame ?bool??");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveDebugCmd(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_DELETE: {
	int i;
	InterpInfo *iiPtr;

	for (i = 2; i < objc; i++) {
	    slaveInterp = GetInterp(interp, objv[i]);
	    if (slaveInterp == NULL) {
		return TCL_ERROR;
	    } else if (slaveInterp == interp) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"cannot delete the current interpreter", -1));
		Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			"DELETESELF", NULL);
		return TCL_ERROR;
	    }
	    iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
	    Tcl_DeleteCommandFromToken(iiPtr->slave.masterInterp,
		    iiPtr->slave.interpCmd);
	}
	return TCL_OK;
    }
    case OPT_EVAL:
	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path arg ?arg ...?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveEval(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_EXISTS: {
	int exists = 1;

	slaveInterp = GetInterp2(interp, objc, objv);
	if (slaveInterp == NULL) {
	    if (objc > 3) {
		return TCL_ERROR;
	    }
	    Tcl_ResetResult(interp);
	    exists = 0;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(exists));
	return TCL_OK;
    }
    case OPT_EXPOSE:
	if ((objc < 4) || (objc > 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path hiddenCmdName ?cmdName?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveExpose(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_HIDE:
	if ((objc < 4) || (objc > 5)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path cmdName ?hiddenCmdName?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveHide(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_HIDDEN:
	slaveInterp = GetInterp2(interp, objc, objv);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveHidden(interp, slaveInterp);
    case OPT_ISSAFE:
	slaveInterp = GetInterp2(interp, objc, objv);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_IsSafe(slaveInterp)));
	return TCL_OK;
    case OPT_INVOKEHID: {
	int i;
	const char *namespaceName;
	static const char *const hiddenOptions[] = {
	    "-global",	"-namespace",	"--", NULL
	};
	enum hiddenOption {
	    OPT_GLOBAL,	OPT_NAMESPACE,	OPT_LAST
	};

	namespaceName = NULL;
	for (i = 3; i < objc; i++) {
	    if (TclGetString(objv[i])[0] != '-') {
		break;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[i], hiddenOptions, "option",
		    0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index == OPT_GLOBAL) {
		namespaceName = "::";
	    } else if (index == OPT_NAMESPACE) {
		if (++i == objc) { /* There must be more arguments. */
		    break;
		} else {
		    namespaceName = TclGetString(objv[i]);
		}
	    } else {
		i++;
		break;
	    }
	}
	if (objc - i < 1) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "path ?-namespace ns? ?-global? ?--? cmd ?arg ..?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveInvokeHidden(interp, slaveInterp, namespaceName, objc - i,
		objv + i);
    }
    case OPT_LIMIT: {
	static const char *const limitTypes[] = {
	    "commands", "time", NULL
	};
	enum LimitTypes {
	    LIMIT_TYPE_COMMANDS, LIMIT_TYPE_TIME
	};
	int limitType;

	if (objc < 4) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "path limitType ?-option value ...?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[3], limitTypes, "limit type", 0,
		&limitType) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum LimitTypes) limitType) {
	case LIMIT_TYPE_COMMANDS:
	    return SlaveCommandLimitCmd(interp, slaveInterp, 4, objc,objv);
	case LIMIT_TYPE_TIME:
	    return SlaveTimeLimitCmd(interp, slaveInterp, 4, objc, objv);
	}
    }
    case OPT_MARKTRUSTED:
	if (objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveMarkTrusted(interp, slaveInterp);
    case OPT_RECLIMIT:
	if (objc != 3 && objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path ?newlimit?");
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	return SlaveRecursionLimit(interp, slaveInterp, objc - 3, objv + 3);
    case OPT_SLAVES: {
	InterpInfo *iiPtr;
	Tcl_Obj *resultPtr;
	Tcl_HashEntry *hPtr;
	Tcl_HashSearch hashSearch;
	char *string;

	slaveInterp = GetInterp2(interp, objc, objv);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
	resultPtr = Tcl_NewObj();
	hPtr = Tcl_FirstHashEntry(&iiPtr->master.slaveTable, &hashSearch);
	for ( ; hPtr != NULL; hPtr = Tcl_NextHashEntry(&hashSearch)) {
	    string = Tcl_GetHashKey(&iiPtr->master.slaveTable, hPtr);
	    Tcl_ListObjAppendElement(NULL, resultPtr,
		    Tcl_NewStringObj(string, -1));
	}
	Tcl_SetObjResult(interp, resultPtr);
	return TCL_OK;
    }
    case OPT_TRANSFER:
    case OPT_SHARE: {
	Tcl_Interp *masterInterp;	/* The master of the slave. */
	Tcl_Channel chan;

	if (objc != 5) {
	    Tcl_WrongNumArgs(interp, 2, objv, "srcPath channelId destPath");
	    return TCL_ERROR;
	}
	masterInterp = GetInterp(interp, objv[2]);
	if (masterInterp == NULL) {
	    return TCL_ERROR;
	}
	chan = Tcl_GetChannel(masterInterp, TclGetString(objv[3]), NULL);
	if (chan == NULL) {
	    Tcl_TransferResult(masterInterp, TCL_OK, interp);
	    return TCL_ERROR;
	}
	slaveInterp = GetInterp(interp, objv[4]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}
	Tcl_RegisterChannel(slaveInterp, chan);
	if (index == OPT_TRANSFER) {
	    /*
	     * When transferring, as opposed to sharing, we must unhitch the
	     * channel from the interpreter where it started.
	     */

	    if (Tcl_UnregisterChannel(masterInterp, chan) != TCL_OK) {
		Tcl_TransferResult(masterInterp, TCL_OK, interp);
		return TCL_ERROR;
	    }
	}
	return TCL_OK;
    }
    case OPT_TARGET: {
	InterpInfo *iiPtr;
	Tcl_HashEntry *hPtr;
	Alias *aliasPtr;
	const char *aliasName;

	if (objc != 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "path alias");
	    return TCL_ERROR;
	}

	slaveInterp = GetInterp(interp, objv[2]);
	if (slaveInterp == NULL) {
	    return TCL_ERROR;
	}

	aliasName = TclGetString(objv[3]);

	iiPtr = (InterpInfo *) ((Interp *) slaveInterp)->interpInfo;
	hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
	if (hPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "alias \"%s\" in path \"%s\" not found",
		    aliasName, Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ALIAS", aliasName,
		    NULL);
	    return TCL_ERROR;
	}
	aliasPtr = Tcl_GetHashValue(hPtr);
	if (Tcl_GetInterpPath(interp, aliasPtr->targetInterp) != TCL_OK) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "target interpreter for alias \"%s\" in path \"%s\" is "
		    "not my descendant", aliasName, Tcl_GetString(objv[2])));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
		    "TARGETSHROUDED", NULL);
	    return TCL_ERROR;
	}
	return TCL_OK;
    }
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * GetInterp2 --
 *
 *	Helper function for Tcl_InterpObjCmd() to convert the interp name
 *	potentially specified on the command line to an Tcl_Interp.
 *
 * Results:
 *	The return value is the interp specified on the command line, or the
 *	interp argument itself if no interp was specified on the command line.
 *	If the interp could not be found or the wrong number of arguments was
 *	specified on the command line, the return value is NULL and an error
 *	message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Interp *
GetInterp2(
    Tcl_Interp *interp,		/* Default interp if no interp was specified
				 * on the command line. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    if (objc == 2) {
	return interp;
    } else if (objc == 3) {
	return GetInterp(interp, objv[2]);
    } else {
	Tcl_WrongNumArgs(interp, 2, objv, "?path?");
	return NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateAlias --
 *
 *	Creates an alias between two interpreters.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a new alias, manipulates the result field of slaveInterp.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateAlias(
    Tcl_Interp *slaveInterp,	/* Interpreter for source command. */
    const char *slaveCmd,	/* Command to install in slave. */
    Tcl_Interp *targetInterp,	/* Interpreter for target command. */
    const char *targetCmd,	/* Name of target command. */
    int argc,			/* How many additional arguments? */
    const char *const *argv)	/* These are the additional args. */
{
    Tcl_Obj *slaveObjPtr, *targetObjPtr;
    Tcl_Obj **objv;
    int i;
    int result;

    objv = TclStackAlloc(slaveInterp, (unsigned) sizeof(Tcl_Obj *) * argc);
    for (i = 0; i < argc; i++) {
	objv[i] = Tcl_NewStringObj(argv[i], -1);
	Tcl_IncrRefCount(objv[i]);
    }

    slaveObjPtr = Tcl_NewStringObj(slaveCmd, -1);
    Tcl_IncrRefCount(slaveObjPtr);

    targetObjPtr = Tcl_NewStringObj(targetCmd, -1);
    Tcl_IncrRefCount(targetObjPtr);

    result = AliasCreate(slaveInterp, slaveInterp, targetInterp, slaveObjPtr,
	    targetObjPtr, argc, objv);

    for (i = 0; i < argc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    TclStackFree(slaveInterp, objv);
    Tcl_DecrRefCount(targetObjPtr);
    Tcl_DecrRefCount(slaveObjPtr);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateAliasObj --
 *
 *	Object version: Creates an alias between two interpreters.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Creates a new alias.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_CreateAliasObj(
    Tcl_Interp *slaveInterp,	/* Interpreter for source command. */
    const char *slaveCmd,	/* Command to install in slave. */
    Tcl_Interp *targetInterp,	/* Interpreter for target command. */
    const char *targetCmd,	/* Name of target command. */
    int objc,			/* How many additional arguments? */
    Tcl_Obj *const objv[])	/* Argument vector. */
{
    Tcl_Obj *slaveObjPtr, *targetObjPtr;
    int result;

    slaveObjPtr = Tcl_NewStringObj(slaveCmd, -1);
    Tcl_IncrRefCount(slaveObjPtr);

    targetObjPtr = Tcl_NewStringObj(targetCmd, -1);
    Tcl_IncrRefCount(targetObjPtr);

    result = AliasCreate(slaveInterp, slaveInterp, targetInterp, slaveObjPtr,
	    targetObjPtr, objc, objv);

    Tcl_DecrRefCount(slaveObjPtr);
    Tcl_DecrRefCount(targetObjPtr);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAlias --
 *
 *	Gets information about an alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetAlias(
    Tcl_Interp *interp,		/* Interp to start search from. */
    const char *aliasName,	/* Name of alias to find. */
    Tcl_Interp **targetInterpPtr,
				/* (Return) target interpreter. */
    const char **targetNamePtr,	/* (Return) name of target command. */
    int *argcPtr,		/* (Return) count of addnl args. */
    const char ***argvPtr)	/* (Return) additional arguments. */
{
    InterpInfo *iiPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;
    int i, objc;
    Tcl_Obj **objv;

    hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"alias \"%s\" not found", aliasName));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ALIAS", aliasName, NULL);
	return TCL_ERROR;
    }
    aliasPtr = Tcl_GetHashValue(hPtr);
    objc = aliasPtr->objc;
    objv = &aliasPtr->objPtr;

    if (targetInterpPtr != NULL) {
	*targetInterpPtr = aliasPtr->targetInterp;
    }
    if (targetNamePtr != NULL) {
	*targetNamePtr = TclGetString(objv[0]);
    }
    if (argcPtr != NULL) {
	*argcPtr = objc - 1;
    }
    if (argvPtr != NULL) {
	*argvPtr = (const char **)
		ckalloc(sizeof(const char *) * (objc - 1));
	for (i = 1; i < objc; i++) {
	    (*argvPtr)[i - 1] = TclGetString(objv[i]);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetAliasObj --
 *
 *	Object version: Gets information about an alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetAliasObj(
    Tcl_Interp *interp,		/* Interp to start search from. */
    const char *aliasName,	/* Name of alias to find. */
    Tcl_Interp **targetInterpPtr,
				/* (Return) target interpreter. */
    const char **targetNamePtr,	/* (Return) name of target command. */
    int *objcPtr,		/* (Return) count of addnl args. */
    Tcl_Obj ***objvPtr)		/* (Return) additional args. */
{
    InterpInfo *iiPtr = (InterpInfo *) ((Interp *) interp)->interpInfo;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;
    int objc;
    Tcl_Obj **objv;

    hPtr = Tcl_FindHashEntry(&iiPtr->slave.aliasTable, aliasName);
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"alias \"%s\" not found", aliasName));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ALIAS", aliasName, NULL);
	return TCL_ERROR;
    }
    aliasPtr = Tcl_GetHashValue(hPtr);
    objc = aliasPtr->objc;
    objv = &aliasPtr->objPtr;

    if (targetInterpPtr != NULL) {
	*targetInterpPtr = aliasPtr->targetInterp;
    }
    if (targetNamePtr != NULL) {
	*targetNamePtr = TclGetString(objv[0]);
    }
    if (objcPtr != NULL) {
	*objcPtr = objc - 1;
    }
    if (objvPtr != NULL) {
	*objvPtr = objv + 1;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclPreventAliasLoop --
 *
 *	When defining an alias or renaming a command, prevent an alias loop
 *	from being formed.
 *
 * Results:
 *	A standard Tcl object result.
 *
 * Side effects:
 *	If TCL_ERROR is returned, the function also stores an error message in
 *	the interpreter's result object.
 *
 * NOTE:
 *	This function is public internal (instead of being static to this
 *	file) because it is also used from TclRenameCommand.
 *
 *----------------------------------------------------------------------
 */

int
TclPreventAliasLoop(
    Tcl_Interp *interp,		/* Interp in which to report errors. */
    Tcl_Interp *cmdInterp,	/* Interp in which the command is being
				 * defined. */
    Tcl_Command cmd)		/* Tcl command we are attempting to define. */
{
    Command *cmdPtr = (Command *) cmd;
    Alias *aliasPtr, *nextAliasPtr;
    Tcl_Command aliasCmd;
    Command *aliasCmdPtr;

    /*
     * If we are not creating or renaming an alias, then it is always OK to
     * create or rename the command.
     */

    if (cmdPtr->objProc != AliasObjCmd) {
	return TCL_OK;
    }

    /*
     * OK, we are dealing with an alias, so traverse the chain of aliases. If
     * we encounter the alias we are defining (or renaming to) any in the
     * chain then we have a loop.
     */

    aliasPtr = cmdPtr->objClientData;
    nextAliasPtr = aliasPtr;
    while (1) {
	Tcl_Obj *cmdNamePtr;

	/*
	 * If the target of the next alias in the chain is the same as the
	 * source alias, we have a loop.
	 */

	if (Tcl_InterpDeleted(nextAliasPtr->targetInterp)) {
	    /*
	     * The slave interpreter can be deleted while creating the alias.
	     * [Bug #641195]
	     */

	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "cannot define or rename alias \"%s\": interpreter deleted",
		    Tcl_GetCommandName(cmdInterp, cmd)));
	    return TCL_ERROR;
	}
	cmdNamePtr = nextAliasPtr->objPtr;
	aliasCmd = Tcl_FindCommand(nextAliasPtr->targetInterp,
		TclGetString(cmdNamePtr),
		Tcl_GetGlobalNamespace(nextAliasPtr->targetInterp),
		/*flags*/ 0);
	if (aliasCmd == NULL) {
	    return TCL_OK;
	}
	aliasCmdPtr = (Command *) aliasCmd;
	if (aliasCmdPtr == cmdPtr) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "cannot define or rename alias \"%s\": would create a loop",
		    Tcl_GetCommandName(cmdInterp, cmd)));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
		    "ALIASLOOP", NULL);
	    return TCL_ERROR;
	}

	/*
	 * Otherwise, follow the chain one step further. See if the target
	 * command is an alias - if so, follow the loop to its target command.
	 * Otherwise we do not have a loop.
	 */

	if (aliasCmdPtr->objProc != AliasObjCmd) {
	    return TCL_OK;
	}
	nextAliasPtr = aliasCmdPtr->objClientData;
    }

    /* NOTREACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * AliasCreate --
 *
 *	Helper function to do the work to actually create an alias.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	An alias command is created and entered into the alias table for the
 *	slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
AliasCreate(
    Tcl_Interp *interp,		/* Interp for error reporting. */
    Tcl_Interp *slaveInterp,	/* Interp where alias cmd will live or from
				 * which alias will be deleted. */
    Tcl_Interp *masterInterp,	/* Interp in which target command will be
				 * invoked. */
    Tcl_Obj *namePtr,		/* Name of alias cmd. */
    Tcl_Obj *targetNamePtr,	/* Name of target cmd. */
    int objc,			/* Additional arguments to store */
    Tcl_Obj *const objv[])	/* with alias. */
{
    Alias *aliasPtr;
    Tcl_HashEntry *hPtr;
    Target *targetPtr;
    Slave *slavePtr;
    Master *masterPtr;
    Tcl_Obj **prefv;
    int isNew, i;

    aliasPtr = ckalloc(sizeof(Alias) + objc * sizeof(Tcl_Obj *));
    aliasPtr->token = namePtr;
    Tcl_IncrRefCount(aliasPtr->token);
    aliasPtr->targetInterp = masterInterp;

    aliasPtr->objc = objc + 1;
    prefv = &aliasPtr->objPtr;

    *prefv = targetNamePtr;
    Tcl_IncrRefCount(targetNamePtr);
    for (i = 0; i < objc; i++) {
	*(++prefv) = objv[i];
	Tcl_IncrRefCount(objv[i]);
    }

    Tcl_Preserve(slaveInterp);
    Tcl_Preserve(masterInterp);

    if (slaveInterp == masterInterp) {
	aliasPtr->slaveCmd = Tcl_NRCreateCommand(slaveInterp,
		TclGetString(namePtr), AliasObjCmd, AliasNRCmd, aliasPtr,
		AliasObjCmdDeleteProc);
    } else {
    aliasPtr->slaveCmd = Tcl_CreateObjCommand(slaveInterp,
	    TclGetString(namePtr), AliasObjCmd, aliasPtr,
	    AliasObjCmdDeleteProc);
    }

    if (TclPreventAliasLoop(interp, slaveInterp,
	    aliasPtr->slaveCmd) != TCL_OK) {
	/*
	 * Found an alias loop! The last call to Tcl_CreateObjCommand made the
	 * alias point to itself. Delete the command and its alias record. Be
	 * careful to wipe out its client data first, so the command doesn't
	 * try to delete itself.
	 */

	Command *cmdPtr;

	Tcl_DecrRefCount(aliasPtr->token);
	Tcl_DecrRefCount(targetNamePtr);
	for (i = 0; i < objc; i++) {
	    Tcl_DecrRefCount(objv[i]);
	}

	cmdPtr = (Command *) aliasPtr->slaveCmd;
	cmdPtr->clientData = NULL;
	cmdPtr->deleteProc = NULL;
	cmdPtr->deleteData = NULL;
	Tcl_DeleteCommandFromToken(slaveInterp, aliasPtr->slaveCmd);

	ckfree(aliasPtr);

	/*
	 * The result was already set by TclPreventAliasLoop.
	 */

	Tcl_Release(slaveInterp);
	Tcl_Release(masterInterp);
	return TCL_ERROR;
    }

    /*
     * Make an entry in the alias table. If it already exists, retry.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    while (1) {
	Tcl_Obj *newToken;
	const char *string;

	string = TclGetString(aliasPtr->token);
	hPtr = Tcl_CreateHashEntry(&slavePtr->aliasTable, string, &isNew);
	if (isNew != 0) {
	    break;
	}

	/*
	 * The alias name cannot be used as unique token, it is already taken.
	 * We can produce a unique token by prepending "::" repeatedly. This
	 * algorithm is a stop-gap to try to maintain the command name as
	 * token for most use cases, fearful of possible backwards compat
	 * problems. A better algorithm would produce unique tokens that need
	 * not be related to the command name.
	 *
	 * ATTENTION: the tests in interp.test and possibly safe.test depend
	 * on the precise definition of these tokens.
	 */

	TclNewLiteralStringObj(newToken, "::");
	Tcl_AppendObjToObj(newToken, aliasPtr->token);
	Tcl_DecrRefCount(aliasPtr->token);
	aliasPtr->token = newToken;
	Tcl_IncrRefCount(aliasPtr->token);
    }

    aliasPtr->aliasEntryPtr = hPtr;
    Tcl_SetHashValue(hPtr, aliasPtr);

    /*
     * Create the new command. We must do it after deleting any old command,
     * because the alias may be pointing at a renamed alias, as in:
     *
     * interp alias {} foo {} bar		# Create an alias "foo"
     * rename foo zop				# Now rename the alias
     * interp alias {} foo {} zop		# Now recreate "foo"...
     */

    targetPtr = ckalloc(sizeof(Target));
    targetPtr->slaveCmd = aliasPtr->slaveCmd;
    targetPtr->slaveInterp = slaveInterp;

    masterPtr = &((InterpInfo*) ((Interp*) masterInterp)->interpInfo)->master;
    targetPtr->nextPtr = masterPtr->targetsPtr;
    targetPtr->prevPtr = NULL;
    if (masterPtr->targetsPtr != NULL) {
	masterPtr->targetsPtr->prevPtr = targetPtr;
    }
    masterPtr->targetsPtr = targetPtr;
    aliasPtr->targetPtr = targetPtr;

    Tcl_SetObjResult(interp, aliasPtr->token);

    Tcl_Release(slaveInterp);
    Tcl_Release(masterInterp);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasDelete --
 *
 *	Deletes the given alias from the slave interpreter given.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Deletes the alias from the slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static int
AliasDelete(
    Tcl_Interp *interp,		/* Interpreter for result & errors. */
    Tcl_Interp *slaveInterp,	/* Interpreter containing alias. */
    Tcl_Obj *namePtr)		/* Name of alias to delete. */
{
    Slave *slavePtr;
    Alias *aliasPtr;
    Tcl_HashEntry *hPtr;

    /*
     * If the alias has been renamed in the slave, the master can still use
     * the original name (with which it was created) to find the alias to
     * delete it.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    hPtr = Tcl_FindHashEntry(&slavePtr->aliasTable, TclGetString(namePtr));
    if (hPtr == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"alias \"%s\" not found", TclGetString(namePtr)));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "ALIAS",
		TclGetString(namePtr), NULL);
	return TCL_ERROR;
    }
    aliasPtr = Tcl_GetHashValue(hPtr);
    Tcl_DeleteCommandFromToken(slaveInterp, aliasPtr->slaveCmd);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasDescribe --
 *
 *	Sets the interpreter's result object to a Tcl list describing the
 *	given alias in the given interpreter: its target command and the
 *	additional arguments to prepend to any invocation of the alias.
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
AliasDescribe(
    Tcl_Interp *interp,		/* Interpreter for result & errors. */
    Tcl_Interp *slaveInterp,	/* Interpreter containing alias. */
    Tcl_Obj *namePtr)		/* Name of alias to describe. */
{
    Slave *slavePtr;
    Tcl_HashEntry *hPtr;
    Alias *aliasPtr;
    Tcl_Obj *prefixPtr;

    /*
     * If the alias has been renamed in the slave, the master can still use
     * the original name (with which it was created) to find the alias to
     * describe it.
     */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    hPtr = Tcl_FindHashEntry(&slavePtr->aliasTable, Tcl_GetString(namePtr));
    if (hPtr == NULL) {
	return TCL_OK;
    }
    aliasPtr = Tcl_GetHashValue(hPtr);
    prefixPtr = Tcl_NewListObj(aliasPtr->objc, &aliasPtr->objPtr);
    Tcl_SetObjResult(interp, prefixPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasList --
 *
 *	Computes a list of aliases defined in a slave interpreter.
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
AliasList(
    Tcl_Interp *interp,		/* Interp for data return. */
    Tcl_Interp *slaveInterp)	/* Interp whose aliases to compute. */
{
    Tcl_HashEntry *entryPtr;
    Tcl_HashSearch hashSearch;
    Tcl_Obj *resultPtr = Tcl_NewObj();
    Alias *aliasPtr;
    Slave *slavePtr;

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;

    entryPtr = Tcl_FirstHashEntry(&slavePtr->aliasTable, &hashSearch);
    for ( ; entryPtr != NULL; entryPtr = Tcl_NextHashEntry(&hashSearch)) {
	aliasPtr = Tcl_GetHashValue(entryPtr);
	Tcl_ListObjAppendElement(NULL, resultPtr, aliasPtr->token);
    }
    Tcl_SetObjResult(interp, resultPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * AliasObjCmd --
 *
 *	This is the function that services invocations of aliases in a slave
 *	interpreter. One such command exists for each alias. When invoked,
 *	this function redirects the invocation to the target command in the
 *	master interpreter as designated by the Alias record associated with
 *	this command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Causes forwarding of the invocation; all possible side effects may
 *	occur as a result of invoking the command to which the invocation is
 *	forwarded.
 *
 *----------------------------------------------------------------------
 */

static int
AliasNRCmd(
    ClientData clientData,	/* Alias record. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument vector. */
{
    Alias *aliasPtr = clientData;
    int prefc, cmdc, i;
    Tcl_Obj **prefv, **cmdv;
    Tcl_Obj *listPtr;
    List *listRep;
    int flags = TCL_EVAL_INVOKE;

    /*
     * Append the arguments to the command prefix and invoke the command in
     * the target interp's global namespace.
     */

    prefc = aliasPtr->objc;
    prefv = &aliasPtr->objPtr;
    cmdc = prefc + objc - 1;

    listPtr = Tcl_NewListObj(cmdc, NULL);
    listRep = listPtr->internalRep.twoPtrValue.ptr1;
    listRep->elemCount = cmdc;
    cmdv = &listRep->elements;

    prefv = &aliasPtr->objPtr;
    memcpy(cmdv, prefv, (size_t) (prefc * sizeof(Tcl_Obj *)));
    memcpy(cmdv+prefc, objv+1, (size_t) ((objc-1) * sizeof(Tcl_Obj *)));

    for (i=0; i<cmdc; i++) {
	Tcl_IncrRefCount(cmdv[i]);
    }

    /*
     * Use the ensemble rewriting machinery to ensure correct error messages:
     * only the source command should show, not the full target prefix.
     */

    if (TclInitRewriteEnsemble(interp, 1, prefc, objv)) {
	TclNRAddCallback(interp, TclClearRootEnsemble, NULL, NULL, NULL, NULL);
    }
    TclSkipTailcall(interp);
    return Tcl_NREvalObj(interp, listPtr, flags);
}

static int
AliasObjCmd(
    ClientData clientData,	/* Alias record. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument vector. */
{
#define ALIAS_CMDV_PREALLOC 10
    Alias *aliasPtr = clientData;
    Tcl_Interp *targetInterp = aliasPtr->targetInterp;
    int result, prefc, cmdc, i;
    Tcl_Obj **prefv, **cmdv;
    Tcl_Obj *cmdArr[ALIAS_CMDV_PREALLOC];
    Interp *tPtr = (Interp *) targetInterp;
    int isRootEnsemble;

    /*
     * Append the arguments to the command prefix and invoke the command in
     * the target interp's global namespace.
     */

    prefc = aliasPtr->objc;
    prefv = &aliasPtr->objPtr;
    cmdc = prefc + objc - 1;
    if (cmdc <= ALIAS_CMDV_PREALLOC) {
	cmdv = cmdArr;
    } else {
	cmdv = TclStackAlloc(interp, cmdc * sizeof(Tcl_Obj *));
    }

    memcpy(cmdv, prefv, (size_t) (prefc * sizeof(Tcl_Obj *)));
    memcpy(cmdv+prefc, objv+1, (size_t) ((objc-1) * sizeof(Tcl_Obj *)));

    Tcl_ResetResult(targetInterp);

    for (i=0; i<cmdc; i++) {
	Tcl_IncrRefCount(cmdv[i]);
    }

    /*
     * Use the ensemble rewriting machinery to ensure correct error messages:
     * only the source command should show, not the full target prefix.
     */

    isRootEnsemble = TclInitRewriteEnsemble((Tcl_Interp *)tPtr, 1, prefc, objv);

    /*
     * Protect the target interpreter if it isn't the same as the source
     * interpreter so that we can continue to work with it after the target
     * command completes.
     */

    if (targetInterp != interp) {
	Tcl_Preserve(targetInterp);
    }

    /*
     * Execute the target command in the target interpreter.
     */

    result = Tcl_EvalObjv(targetInterp, cmdc, cmdv, TCL_EVAL_INVOKE);

    /*
     * Clean up the ensemble rewrite info if we set it in the first place.
     */

    if (isRootEnsemble) {
	TclResetRewriteEnsemble((Tcl_Interp *)tPtr, 1);
    }

    /*
     * If it was a cross-interpreter alias, we need to transfer the result
     * back to the source interpreter and release the lock we previously set
     * on the target interpreter.
     */

    if (targetInterp != interp) {
	Tcl_TransferResult(targetInterp, result, interp);
	Tcl_Release(targetInterp);
    }

    for (i=0; i<cmdc; i++) {
	Tcl_DecrRefCount(cmdv[i]);
    }
    if (cmdv != cmdArr) {
	TclStackFree(interp, cmdv);
    }
    return result;
#undef ALIAS_CMDV_PREALLOC
}

/*
 *----------------------------------------------------------------------
 *
 * AliasObjCmdDeleteProc --
 *
 *	Is invoked when an alias command is deleted in a slave. Cleans up all
 *	storage associated with this alias.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deletes the alias record and its entry in the alias table for the
 *	interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
AliasObjCmdDeleteProc(
    ClientData clientData)	/* The alias record for this alias. */
{
    Alias *aliasPtr = clientData;
    Target *targetPtr;
    int i;
    Tcl_Obj **objv;

    Tcl_DecrRefCount(aliasPtr->token);
    objv = &aliasPtr->objPtr;
    for (i = 0; i < aliasPtr->objc; i++) {
	Tcl_DecrRefCount(objv[i]);
    }
    Tcl_DeleteHashEntry(aliasPtr->aliasEntryPtr);

    /*
     * Splice the target record out of the target interpreter's master list.
     */

    targetPtr = aliasPtr->targetPtr;
    if (targetPtr->prevPtr != NULL) {
	targetPtr->prevPtr->nextPtr = targetPtr->nextPtr;
    } else {
	Master *masterPtr = &((InterpInfo *) ((Interp *)
		aliasPtr->targetInterp)->interpInfo)->master;

	masterPtr->targetsPtr = targetPtr->nextPtr;
    }
    if (targetPtr->nextPtr != NULL) {
	targetPtr->nextPtr->prevPtr = targetPtr->prevPtr;
    }

    ckfree(targetPtr);
    ckfree(aliasPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateSlave --
 *
 *	Creates a slave interpreter. The slavePath argument denotes the name
 *	of the new slave relative to the current interpreter; the slave is a
 *	direct descendant of the one-before-last component of the path,
 *	e.g. it is a descendant of the current interpreter if the slavePath
 *	argument contains only one component. Optionally makes the slave
 *	interpreter safe.
 *
 * Results:
 *	Returns the interpreter structure created, or NULL if an error
 *	occurred.
 *
 * Side effects:
 *	Creates a new interpreter and a new interpreter object command in the
 *	interpreter indicated by the slavePath argument.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_CreateSlave(
    Tcl_Interp *interp,		/* Interpreter to start search at. */
    const char *slavePath,	/* Name of slave to create. */
    int isSafe)			/* Should new slave be "safe" ? */
{
    Tcl_Obj *pathPtr;
    Tcl_Interp *slaveInterp;

    pathPtr = Tcl_NewStringObj(slavePath, -1);
    slaveInterp = SlaveCreate(interp, pathPtr, isSafe);
    Tcl_DecrRefCount(pathPtr);

    return slaveInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetSlave --
 *
 *	Finds a slave interpreter by its path name.
 *
 * Results:
 *	Returns a Tcl_Interp * for the named interpreter or NULL if not found.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_GetSlave(
    Tcl_Interp *interp,		/* Interpreter to start search from. */
    const char *slavePath)	/* Path of slave to find. */
{
    Tcl_Obj *pathPtr;
    Tcl_Interp *slaveInterp;

    pathPtr = Tcl_NewStringObj(slavePath, -1);
    slaveInterp = GetInterp(interp, pathPtr);
    Tcl_DecrRefCount(pathPtr);

    return slaveInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetMaster --
 *
 *	Finds the master interpreter of a slave interpreter.
 *
 * Results:
 *	Returns a Tcl_Interp * for the master interpreter or NULL if none.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_GetMaster(
    Tcl_Interp *interp)		/* Get the master of this interpreter. */
{
    Slave *slavePtr;		/* Slave record of this interpreter. */

    if (interp == NULL) {
	return NULL;
    }
    slavePtr = &((InterpInfo *) ((Interp *) interp)->interpInfo)->slave;
    return slavePtr->masterInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * TclSetSlaveCancelFlags --
 *
 *	This function marks all slave interpreters belonging to a given
 *	interpreter as being canceled or not canceled, depending on the
 *	provided flags.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TclSetSlaveCancelFlags(
    Tcl_Interp *interp,		/* Set cancel flags of this interpreter. */
    int flags,			/* Collection of OR-ed bits that control
				 * the cancellation of the script. Only
				 * TCL_CANCEL_UNWIND is currently
				 * supported. */
    int force)			/* Non-zero to ignore numLevels for the purpose
				 * of resetting the cancellation flags. */
{
    Master *masterPtr;		/* Master record of given interpreter. */
    Tcl_HashEntry *hPtr;	/* Search element. */
    Tcl_HashSearch hashSearch;	/* Search variable. */
    Slave *slavePtr;		/* Slave record of interpreter. */
    Interp *iPtr;

    if (interp == NULL) {
	return;
    }

    flags &= (CANCELED | TCL_CANCEL_UNWIND);

    masterPtr = &((InterpInfo *) ((Interp *) interp)->interpInfo)->master;

    hPtr = Tcl_FirstHashEntry(&masterPtr->slaveTable, &hashSearch);
    for ( ; hPtr != NULL; hPtr = Tcl_NextHashEntry(&hashSearch)) {
	slavePtr = Tcl_GetHashValue(hPtr);
	iPtr = (Interp *) slavePtr->slaveInterp;

	if (iPtr == NULL) {
	    continue;
	}

	if (flags == 0) {
	    TclResetCancellation((Tcl_Interp *) iPtr, force);
	} else {
	    TclSetCancelFlags(iPtr, flags);
	}

	/*
	 * Now, recursively handle this for the slaves of this slave
	 * interpreter.
	 */

	TclSetSlaveCancelFlags((Tcl_Interp *) iPtr, flags, force);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetInterpPath --
 *
 *	Sets the result of the asking interpreter to a proper Tcl list
 *	containing the names of interpreters between the asking and target
 *	interpreters. The target interpreter must be either the same as the
 *	asking interpreter or one of its slaves (including recursively).
 *
 * Results:
 *	TCL_OK if the target interpreter is the same as, or a descendant of,
 *	the asking interpreter; TCL_ERROR else. This way one can distinguish
 *	between the case where the asking and target interps are the same (an
 *	empty list is the result, and TCL_OK is returned) and when the target
 *	is not a descendant of the asking interpreter (in which case the Tcl
 *	result is an error message and the function returns TCL_ERROR).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetInterpPath(
    Tcl_Interp *askingInterp,	/* Interpreter to start search from. */
    Tcl_Interp *targetInterp)	/* Interpreter to find. */
{
    InterpInfo *iiPtr;

    if (targetInterp == askingInterp) {
	Tcl_SetObjResult(askingInterp, Tcl_NewObj());
	return TCL_OK;
    }
    if (targetInterp == NULL) {
	return TCL_ERROR;
    }
    iiPtr = (InterpInfo *) ((Interp *) targetInterp)->interpInfo;
    if (Tcl_GetInterpPath(askingInterp, iiPtr->slave.masterInterp) != TCL_OK){
	return TCL_ERROR;
    }
    Tcl_ListObjAppendElement(NULL, Tcl_GetObjResult(askingInterp),
	    Tcl_NewStringObj(Tcl_GetHashKey(&iiPtr->master.slaveTable,
		    iiPtr->slave.slaveEntryPtr), -1));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetInterp --
 *
 *	Helper function to find a slave interpreter given a pathname.
 *
 * Results:
 *	Returns the slave interpreter known by that name in the calling
 *	interpreter, or NULL if no interpreter known by that name exists.
 *
 * Side effects:
 *	Assigns to the pointer variable passed in, if not NULL.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Interp *
GetInterp(
    Tcl_Interp *interp,		/* Interp. to start search from. */
    Tcl_Obj *pathPtr)		/* List object containing name of interp. to
				 * be found. */
{
    Tcl_HashEntry *hPtr;	/* Search element. */
    Slave *slavePtr;		/* Interim slave record. */
    Tcl_Obj **objv;
    int objc, i;
    Tcl_Interp *searchInterp;	/* Interim storage for interp. to find. */
    InterpInfo *masterInfoPtr;

    if (TclListObjGetElements(interp, pathPtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }

    searchInterp = interp;
    for (i = 0; i < objc; i++) {
	masterInfoPtr = (InterpInfo *) ((Interp *) searchInterp)->interpInfo;
	hPtr = Tcl_FindHashEntry(&masterInfoPtr->master.slaveTable,
		TclGetString(objv[i]));
	if (hPtr == NULL) {
	    searchInterp = NULL;
	    break;
	}
	slavePtr = Tcl_GetHashValue(hPtr);
	searchInterp = slavePtr->slaveInterp;
	if (searchInterp == NULL) {
	    break;
	}
    }
    if (searchInterp == NULL) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"could not find interpreter \"%s\"", TclGetString(pathPtr)));
	Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "INTERP",
		TclGetString(pathPtr), NULL);
    }
    return searchInterp;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveBgerror --
 *
 *	Helper function to set/query the background error handling command
 *	prefix of an interp
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	When (objc == 1), slaveInterp will be set to a new background handler
 *	of objv[0].
 *
 *----------------------------------------------------------------------
 */

static int
SlaveBgerror(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* Interp in which limit is set/queried. */
    int objc,			/* Set or Query. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    if (objc) {
	int length;

	if (TCL_ERROR == TclListObjLength(NULL, objv[0], &length)
		|| (length < 1)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "cmdPrefix must be list of length >= 1", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
		    "BGERRORFORMAT", NULL);
	    return TCL_ERROR;
	}
	TclSetBgErrorHandler(slaveInterp, objv[0]);
    }
    Tcl_SetObjResult(interp, TclGetBgErrorHandler(slaveInterp));
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveCreate --
 *
 *	Helper function to do the actual work of creating a slave interp and
 *	new object command. Also optionally makes the new slave interpreter
 *	"safe".
 *
 * Results:
 *	Returns the new Tcl_Interp * if successful or NULL if not. If failed,
 *	the result of the invoking interpreter contains an error message.
 *
 * Side effects:
 *	Creates a new slave interpreter and a new object command.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Interp *
SlaveCreate(
    Tcl_Interp *interp,		/* Interp. to start search from. */
    Tcl_Obj *pathPtr,		/* Path (name) of slave to create. */
    int safe)			/* Should we make it "safe"? */
{
    Tcl_Interp *masterInterp, *slaveInterp;
    Slave *slavePtr;
    InterpInfo *masterInfoPtr;
    Tcl_HashEntry *hPtr;
    const char *path;
    int isNew, objc;
    Tcl_Obj **objv;

    if (Tcl_ListObjGetElements(interp, pathPtr, &objc, &objv) != TCL_OK) {
	return NULL;
    }
    if (objc < 2) {
	masterInterp = interp;
	path = TclGetString(pathPtr);
    } else {
	Tcl_Obj *objPtr;

	objPtr = Tcl_NewListObj(objc - 1, objv);
	masterInterp = GetInterp(interp, objPtr);
	Tcl_DecrRefCount(objPtr);
	if (masterInterp == NULL) {
	    return NULL;
	}
	path = TclGetString(objv[objc - 1]);
    }
    if (safe == 0) {
	safe = Tcl_IsSafe(masterInterp);
    }

    masterInfoPtr = (InterpInfo *) ((Interp *) masterInterp)->interpInfo;
    hPtr = Tcl_CreateHashEntry(&masterInfoPtr->master.slaveTable, path,
	    &isNew);
    if (isNew == 0) {
	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"interpreter named \"%s\" already exists, cannot create",
		path));
	return NULL;
    }

    slaveInterp = Tcl_CreateInterp();
    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;
    slavePtr->masterInterp = masterInterp;
    slavePtr->slaveEntryPtr = hPtr;
    slavePtr->slaveInterp = slaveInterp;
    slavePtr->interpCmd = Tcl_NRCreateCommand(masterInterp, path,
	    SlaveObjCmd, NRSlaveCmd, slaveInterp, SlaveObjCmdDeleteProc);
    Tcl_InitHashTable(&slavePtr->aliasTable, TCL_STRING_KEYS);
    Tcl_SetHashValue(hPtr, slavePtr);
    Tcl_SetVar(slaveInterp, "tcl_interactive", "0", TCL_GLOBAL_ONLY);

    /*
     * Inherit the recursion limit.
     */

    ((Interp *) slaveInterp)->maxNestingDepth =
	    ((Interp *) masterInterp)->maxNestingDepth;

    if (safe) {
	if (Tcl_MakeSafe(slaveInterp) == TCL_ERROR) {
	    goto error;
	}
    } else {
	if (Tcl_Init(slaveInterp) == TCL_ERROR) {
	    goto error;
	}

	/*
	 * This will create the "memory" command in slave interpreters if we
	 * compiled with TCL_MEM_DEBUG, otherwise it does nothing.
	 */

	Tcl_InitMemory(slaveInterp);
    }

    /*
     * Inherit the TIP#143 limits.
     */

    InheritLimitsFromMaster(slaveInterp, masterInterp);

    /*
     * The [clock] command presents a safe API, but uses unsafe features in
     * its implementation. This means it has to be implemented in safe interps
     * as an alias to a version in the (trusted) master.
     */

    if (safe) {
	Tcl_Obj *clockObj;
	int status;

	TclNewLiteralStringObj(clockObj, "clock");
	Tcl_IncrRefCount(clockObj);
	status = AliasCreate(interp, slaveInterp, masterInterp, clockObj,
		clockObj, 0, NULL);
	Tcl_DecrRefCount(clockObj);
	if (status != TCL_OK) {
	    goto error2;
	}
    }

    return slaveInterp;

  error:
    Tcl_TransferResult(slaveInterp, TCL_ERROR, interp);
  error2:
    Tcl_DeleteInterp(slaveInterp);

    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveObjCmd --
 *
 *	Command to manipulate an interpreter, e.g. to send commands to it to
 *	be evaluated. One such command exists for each slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See user documentation for details.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveObjCmd(
    ClientData clientData,	/* Slave interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    return Tcl_NRCallObjProc(interp, NRSlaveCmd, clientData, objc, objv);
}

static int
NRSlaveCmd(
    ClientData clientData,	/* Slave interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tcl_Interp *slaveInterp = clientData;
    int index;
    static const char *const options[] = {
	"alias",	"aliases",	"bgerror",	"debug",
	"eval",		"expose",	"hide",		"hidden",
	"issafe",	"invokehidden",	"limit",	"marktrusted",
	"recursionlimit", NULL
    };
    enum options {
	OPT_ALIAS,	OPT_ALIASES,	OPT_BGERROR,	OPT_DEBUG,
	OPT_EVAL,	OPT_EXPOSE,	OPT_HIDE,	OPT_HIDDEN,
	OPT_ISSAFE,	OPT_INVOKEHIDDEN, OPT_LIMIT,	OPT_MARKTRUSTED,
	OPT_RECLIMIT
    };

    if (slaveInterp == NULL) {
	Tcl_Panic("SlaveObjCmd: interpreter has been deleted");
    }

    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
	return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
    case OPT_ALIAS:
	if (objc > 2) {
	    if (objc == 3) {
		return AliasDescribe(interp, slaveInterp, objv[2]);
	    }
	    if (TclGetString(objv[3])[0] == '\0') {
		if (objc == 4) {
		    return AliasDelete(interp, slaveInterp, objv[2]);
		}
	    } else {
		return AliasCreate(interp, slaveInterp, interp, objv[2],
			objv[3], objc - 4, objv + 4);
	    }
	}
	Tcl_WrongNumArgs(interp, 2, objv, "aliasName ?targetName? ?arg ...?");
	return TCL_ERROR;
    case OPT_ALIASES:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return AliasList(interp, slaveInterp);
    case OPT_BGERROR:
	if (objc != 2 && objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?cmdPrefix?");
	    return TCL_ERROR;
	}
	return SlaveBgerror(interp, slaveInterp, objc - 2, objv + 2);
    case OPT_DEBUG:
	/*
	 * TIP #378
	 * Currently only -frame supported, otherwise ?-option ?value? ...?
	 */
	if (objc > 4) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?-frame ?bool??");
	    return TCL_ERROR;
	}
	return SlaveDebugCmd(interp, slaveInterp, objc - 2, objv + 2);
    case OPT_EVAL:
	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "arg ?arg ...?");
	    return TCL_ERROR;
	}
	return SlaveEval(interp, slaveInterp, objc - 2, objv + 2);
    case OPT_EXPOSE:
	if ((objc < 3) || (objc > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "hiddenCmdName ?cmdName?");
	    return TCL_ERROR;
	}
	return SlaveExpose(interp, slaveInterp, objc - 2, objv + 2);
    case OPT_HIDE:
	if ((objc < 3) || (objc > 4)) {
	    Tcl_WrongNumArgs(interp, 2, objv, "cmdName ?hiddenCmdName?");
	    return TCL_ERROR;
	}
	return SlaveHide(interp, slaveInterp, objc - 2, objv + 2);
    case OPT_HIDDEN:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return SlaveHidden(interp, slaveInterp);
    case OPT_ISSAFE:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, Tcl_NewBooleanObj(Tcl_IsSafe(slaveInterp)));
	return TCL_OK;
    case OPT_INVOKEHIDDEN: {
	int i;
	const char *namespaceName;
	static const char *const hiddenOptions[] = {
	    "-global",	"-namespace",	"--", NULL
	};
	enum hiddenOption {
	    OPT_GLOBAL,	OPT_NAMESPACE,	OPT_LAST
	};

	namespaceName = NULL;
	for (i = 2; i < objc; i++) {
	    if (TclGetString(objv[i])[0] != '-') {
		break;
	    }
	    if (Tcl_GetIndexFromObj(interp, objv[i], hiddenOptions, "option",
		    0, &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (index == OPT_GLOBAL) {
		namespaceName = "::";
	    } else if (index == OPT_NAMESPACE) {
		if (++i == objc) { /* There must be more arguments. */
		    break;
		} else {
		    namespaceName = TclGetString(objv[i]);
		}
	    } else {
		i++;
		break;
	    }
	}
	if (objc - i < 1) {
	    Tcl_WrongNumArgs(interp, 2, objv,
		    "?-namespace ns? ?-global? ?--? cmd ?arg ..?");
	    return TCL_ERROR;
	}
	return SlaveInvokeHidden(interp, slaveInterp, namespaceName,
		objc - i, objv + i);
    }
    case OPT_LIMIT: {
	static const char *const limitTypes[] = {
	    "commands", "time", NULL
	};
	enum LimitTypes {
	    LIMIT_TYPE_COMMANDS, LIMIT_TYPE_TIME
	};
	int limitType;

	if (objc < 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "limitType ?-option value ...?");
	    return TCL_ERROR;
	}
	if (Tcl_GetIndexFromObj(interp, objv[2], limitTypes, "limit type", 0,
		&limitType) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum LimitTypes) limitType) {
	case LIMIT_TYPE_COMMANDS:
	    return SlaveCommandLimitCmd(interp, slaveInterp, 3, objc,objv);
	case LIMIT_TYPE_TIME:
	    return SlaveTimeLimitCmd(interp, slaveInterp, 3, objc, objv);
	}
    }
    case OPT_MARKTRUSTED:
	if (objc != 2) {
	    Tcl_WrongNumArgs(interp, 2, objv, NULL);
	    return TCL_ERROR;
	}
	return SlaveMarkTrusted(interp, slaveInterp);
    case OPT_RECLIMIT:
	if (objc != 2 && objc != 3) {
	    Tcl_WrongNumArgs(interp, 2, objv, "?newlimit?");
	    return TCL_ERROR;
	}
	return SlaveRecursionLimit(interp, slaveInterp, objc - 2, objv + 2);
    }

    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveObjCmdDeleteProc --
 *
 *	Invoked when an object command for a slave interpreter is deleted;
 *	cleans up all state associated with the slave interpreter and destroys
 *	the slave interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Cleans up all state associated with the slave interpreter and destroys
 *	the slave interpreter.
 *
 *----------------------------------------------------------------------
 */

static void
SlaveObjCmdDeleteProc(
    ClientData clientData)	/* The SlaveRecord for the command. */
{
    Slave *slavePtr;		/* Interim storage for Slave record. */
    Tcl_Interp *slaveInterp = clientData;
				/* And for a slave interp. */

    slavePtr = &((InterpInfo *) ((Interp *) slaveInterp)->interpInfo)->slave;

    /*
     * Unlink the slave from its master interpreter.
     */

    Tcl_DeleteHashEntry(slavePtr->slaveEntryPtr);

    /*
     * Set to NULL so that when the InterpInfo is cleaned up in the slave it
     * does not try to delete the command causing all sorts of grief. See
     * SlaveRecordDeleteProc().
     */

    slavePtr->interpCmd = NULL;

    if (slavePtr->slaveInterp != NULL) {
	Tcl_DeleteInterp(slavePtr->slaveInterp);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveDebugCmd -- TIP #378
 *
 *	Helper function to handle 'debug' command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	May modify INTERP_DEBUG_FRAME flag in the slave.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveDebugCmd(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* The slave interpreter in which command
				 * will be evaluated. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const debugTypes[] = {
	"-frame", NULL
    };
    enum DebugTypes {
	DEBUG_TYPE_FRAME
    };
    int debugType;
    Interp *iPtr;
    Tcl_Obj *resultPtr;

    iPtr = (Interp *) slaveInterp;
    if (objc == 0) {
	resultPtr = Tcl_NewObj();
	Tcl_ListObjAppendElement(NULL, resultPtr,
		Tcl_NewStringObj("-frame", -1));
	Tcl_ListObjAppendElement(NULL, resultPtr,
		Tcl_NewBooleanObj(iPtr->flags & INTERP_DEBUG_FRAME));
	Tcl_SetObjResult(interp, resultPtr);
    } else {
	if (Tcl_GetIndexFromObj(interp, objv[0], debugTypes, "debug option",
		0, &debugType) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (debugType == DEBUG_TYPE_FRAME) {
	    if (objc == 2) { /* set */
		if (Tcl_GetBooleanFromObj(interp, objv[1], &debugType)
			!= TCL_OK) {
		    return TCL_ERROR;
		}

		/*
		 * Quietly ignore attempts to disable interp debugging.  This
		 * is a one-way switch as frame debug info is maintained in a
		 * stack that must be consistent once turned on.
		 */

		if (debugType) {
		    iPtr->flags |= INTERP_DEBUG_FRAME;
		}
	    }
	    Tcl_SetObjResult(interp,
		    Tcl_NewBooleanObj(iPtr->flags & INTERP_DEBUG_FRAME));
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveEval --
 *
 *	Helper function to evaluate a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Whatever the command does.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveEval(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* The slave interpreter in which command
				 * will be evaluated. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int result;

    /*
     * TIP #285: If necessary, reset the cancellation flags for the slave
     * interpreter now; otherwise, canceling a script in a master interpreter
     * can result in a situation where a slave interpreter can no longer
     * evaluate any scripts unless somebody calls the TclResetCancellation
     * function for that particular Tcl_Interp.
     */

    TclSetSlaveCancelFlags(slaveInterp, 0, 0);

    Tcl_Preserve(slaveInterp);
    Tcl_AllowExceptions(slaveInterp);

    if (objc == 1) {
	/*
	 * TIP #280: Make actual argument location available to eval'd script.
	 */

	Interp *iPtr = (Interp *) interp;
	CmdFrame *invoker = iPtr->cmdFramePtr;
	int word = 0;

	TclArgumentGet(interp, objv[0], &invoker, &word);

	result = TclEvalObjEx(slaveInterp, objv[0], 0, invoker, word);
    } else {
	Tcl_Obj *objPtr = Tcl_ConcatObj(objc, objv);
	Tcl_IncrRefCount(objPtr);
	result = Tcl_EvalObjEx(slaveInterp, objPtr, 0);
	Tcl_DecrRefCount(objPtr);
    }
    Tcl_TransferResult(slaveInterp, result, interp);

    Tcl_Release(slaveInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveExpose --
 *
 *	Helper function to expose a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call scripts in the slave will be able to invoke the newly
 *	exposed command.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveExpose(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* Interp in which command will be exposed. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    const char *name;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"permission denied: safe interpreter cannot expose commands",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "UNSAFE",
		NULL);
	return TCL_ERROR;
    }

    name = TclGetString(objv[(objc == 1) ? 0 : 1]);
    if (Tcl_ExposeCommand(slaveInterp, TclGetString(objv[0]),
	    name) != TCL_OK) {
	Tcl_TransferResult(slaveInterp, TCL_ERROR, interp);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveRecursionLimit --
 *
 *	Helper function to set/query the Recursion limit of an interp
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	When (objc == 1), slaveInterp will be set to a new recursion limit of
 *	objv[0].
 *
 *----------------------------------------------------------------------
 */

static int
SlaveRecursionLimit(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* Interp in which limit is set/queried. */
    int objc,			/* Set or Query. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    Interp *iPtr;
    int limit;

    if (objc) {
	if (Tcl_IsSafe(interp)) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj("permission denied: "
		    "safe interpreters cannot change recursion limit", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "UNSAFE",
		    NULL);
	    return TCL_ERROR;
	}
	if (TclGetIntFromObj(interp, objv[0], &limit) == TCL_ERROR) {
	    return TCL_ERROR;
	}
	if (limit <= 0) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "recursion limit must be > 0", -1));
	    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "BADLIMIT",
		    NULL);
	    return TCL_ERROR;
	}
	Tcl_SetRecursionLimit(slaveInterp, limit);
	iPtr = (Interp *) slaveInterp;
	if (interp == slaveInterp && iPtr->numLevels > limit) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "falling back due to new recursion limit", -1));
	    Tcl_SetErrorCode(interp, "TCL", "RECURSION", NULL);
	    return TCL_ERROR;
	}
	Tcl_SetObjResult(interp, objv[0]);
	return TCL_OK;
    } else {
	limit = Tcl_SetRecursionLimit(slaveInterp, 0);
	Tcl_SetObjResult(interp, Tcl_NewIntObj(limit));
	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveHide --
 *
 *	Helper function to hide a command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call scripts in the slave will no longer be able to invoke
 *	the named command.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveHide(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* Interp in which command will be exposed. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument strings. */
{
    const char *name;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"permission denied: safe interpreter cannot hide commands",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "UNSAFE",
		NULL);
	return TCL_ERROR;
    }

    name = TclGetString(objv[(objc == 1) ? 0 : 1]);
    if (Tcl_HideCommand(slaveInterp, TclGetString(objv[0]), name) != TCL_OK) {
	Tcl_TransferResult(slaveInterp, TCL_ERROR, interp);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveHidden --
 *
 *	Helper function to compute list of hidden commands in a slave
 *	interpreter.
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
SlaveHidden(
    Tcl_Interp *interp,		/* Interp for data return. */
    Tcl_Interp *slaveInterp)	/* Interp whose hidden commands to query. */
{
    Tcl_Obj *listObjPtr = Tcl_NewObj();	/* Local object pointer. */
    Tcl_HashTable *hTblPtr;		/* For local searches. */
    Tcl_HashEntry *hPtr;		/* For local searches. */
    Tcl_HashSearch hSearch;		/* For local searches. */

    hTblPtr = ((Interp *) slaveInterp)->hiddenCmdTablePtr;
    if (hTblPtr != NULL) {
	for (hPtr = Tcl_FirstHashEntry(hTblPtr, &hSearch);
		hPtr != NULL;
		hPtr = Tcl_NextHashEntry(&hSearch)) {
	    Tcl_ListObjAppendElement(NULL, listObjPtr,
		    Tcl_NewStringObj(Tcl_GetHashKey(hTblPtr, hPtr), -1));
	}
    }
    Tcl_SetObjResult(interp, listObjPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveInvokeHidden --
 *
 *	Helper function to invoke a hidden command in a slave interpreter.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Whatever the hidden command does.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveInvokeHidden(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp,	/* The slave interpreter in which command will
				 * be invoked. */
    const char *namespaceName,	/* The namespace to use, if any. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int result;

    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"not allowed to invoke hidden commands from safe interpreter",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "UNSAFE",
		NULL);
	return TCL_ERROR;
    }

    Tcl_Preserve(slaveInterp);
    Tcl_AllowExceptions(slaveInterp);

    if (namespaceName == NULL) {
	NRE_callback *rootPtr = TOP_CB(slaveInterp);

	Tcl_NRAddCallback(interp, NRPostInvokeHidden, slaveInterp,
		rootPtr, NULL, NULL);
	return TclNRInvoke(NULL, slaveInterp, objc, objv);
    } else {
	Namespace *nsPtr, *dummy1, *dummy2;
	const char *tail;

	result = TclGetNamespaceForQualName(slaveInterp, namespaceName, NULL,
		TCL_FIND_ONLY_NS | TCL_GLOBAL_ONLY | TCL_LEAVE_ERR_MSG
		| TCL_CREATE_NS_IF_UNKNOWN, &nsPtr, &dummy1, &dummy2, &tail);
	if (result == TCL_OK) {
	    result = TclObjInvokeNamespace(slaveInterp, objc, objv,
		    (Tcl_Namespace *) nsPtr, TCL_INVOKE_HIDDEN);
	}
    }

    Tcl_TransferResult(slaveInterp, result, interp);

    Tcl_Release(slaveInterp);
    return result;
}

static int
NRPostInvokeHidden(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    Tcl_Interp *slaveInterp = (Tcl_Interp *)data[0];
    NRE_callback *rootPtr = (NRE_callback *)data[1];

    if (interp != slaveInterp) {
	result = TclNRRunCallbacks(slaveInterp, result, rootPtr);
	Tcl_TransferResult(slaveInterp, result, interp);
    }
    Tcl_Release(slaveInterp);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveMarkTrusted --
 *
 *	Helper function to mark a slave interpreter as trusted (unsafe).
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	After this call the hard-wired security checks in the core no longer
 *	prevent the slave from performing certain operations.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveMarkTrusted(
    Tcl_Interp *interp,		/* Interp for error return. */
    Tcl_Interp *slaveInterp)	/* The slave interpreter which will be marked
				 * trusted. */
{
    if (Tcl_IsSafe(interp)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"permission denied: safe interpreter cannot mark trusted",
		-1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "UNSAFE",
		NULL);
	return TCL_ERROR;
    }
    ((Interp *) slaveInterp)->flags &= ~SAFE_INTERP;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IsSafe --
 *
 *	Determines whether an interpreter is safe
 *
 * Results:
 *	1 if it is safe, 0 if it is not.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_IsSafe(
    Tcl_Interp *interp)		/* Is this interpreter "safe" ? */
{
    Interp *iPtr = (Interp *) interp;

    if (iPtr == NULL) {
	return 0;
    }
    return (iPtr->flags & SAFE_INTERP) ? 1 : 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MakeSafe --
 *
 *	Makes its argument interpreter contain only functionality that is
 *	defined to be part of Safe Tcl. Unsafe commands are hidden, the env
 *	array is unset, and the standard channels are removed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hides commands in its argument interpreter, and removes settings and
 *	channels.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_MakeSafe(
    Tcl_Interp *interp)		/* Interpreter to be made safe. */
{
    Tcl_Channel chan;		/* Channel to remove from safe interpreter. */
    Interp *iPtr = (Interp *) interp;
    Tcl_Interp *master = ((InterpInfo*) iPtr->interpInfo)->slave.masterInterp;

    TclHideUnsafeCommands(interp);

    if (master != NULL) {
	/*
	 * Alias these function implementations in the slave to those in the
	 * master; the overall implementations are safe, but they're normally
	 * defined by init.tcl which is not sourced by safe interpreters.
	 * Assume these functions all work. [Bug 2895741]
	 */

	(void) Tcl_Eval(interp,
		"namespace eval ::tcl {namespace eval mathfunc {}}");
	(void) Tcl_CreateAlias(interp, "::tcl::mathfunc::min", master,
		"::tcl::mathfunc::min", 0, NULL);
	(void) Tcl_CreateAlias(interp, "::tcl::mathfunc::max", master,
		"::tcl::mathfunc::max", 0, NULL);
    }

    iPtr->flags |= SAFE_INTERP;

    /*
     * Unsetting variables : (which should not have been set in the first
     * place, but...)
     */

    /*
     * No env array in a safe slave.
     */

    Tcl_UnsetVar(interp, "env", TCL_GLOBAL_ONLY);

    /*
     * Remove unsafe parts of tcl_platform
     */

    Tcl_UnsetVar2(interp, "tcl_platform", "os", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "osVersion", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "machine", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar2(interp, "tcl_platform", "user", TCL_GLOBAL_ONLY);

    /*
     * Unset path informations variables (the only one remaining is [info
     * nameofexecutable])
     */

    Tcl_UnsetVar(interp, "tclDefaultLibrary", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar(interp, "tcl_library", TCL_GLOBAL_ONLY);
    Tcl_UnsetVar(interp, "tcl_pkgPath", TCL_GLOBAL_ONLY);

    /*
     * Remove the standard channels from the interpreter; safe interpreters do
     * not ordinarily have access to stdin, stdout and stderr.
     *
     * NOTE: These channels are not added to the interpreter by the
     * Tcl_CreateInterp call, but may be added later, by another I/O
     * operation. We want to ensure that the interpreter does not have these
     * channels even if it is being made safe after being used for some time..
     */

    chan = Tcl_GetStdChannel(TCL_STDIN);
    if (chan != NULL) {
	Tcl_UnregisterChannel(interp, chan);
    }
    chan = Tcl_GetStdChannel(TCL_STDOUT);
    if (chan != NULL) {
	Tcl_UnregisterChannel(interp, chan);
    }
    chan = Tcl_GetStdChannel(TCL_STDERR);
    if (chan != NULL) {
	Tcl_UnregisterChannel(interp, chan);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitExceeded --
 *
 *	Tests whether any limit has been exceeded in the given interpreter
 *	(i.e. whether the interpreter is currently unable to process further
 *	scripts).
 *
 * Results:
 *	A boolean value.
 *
 * Side effects:
 *	None.
 *
 * Notes:
 *	If you change this function, you MUST also update TclLimitExceeded() in
 *	tclInt.h.
 *----------------------------------------------------------------------
 */

int
Tcl_LimitExceeded(
    Tcl_Interp *interp)
{
    register Interp *iPtr = (Interp *) interp;

    return iPtr->limit.exceeded != 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitReady --
 *
 *	Find out whether any limit has been set on the interpreter, and if so
 *	check whether the granularity of that limit is such that the full
 *	limit check should be carried out.
 *
 * Results:
 *	A boolean value that indicates whether to call Tcl_LimitCheck.
 *
 * Side effects:
 *	Increments the limit granularity counter.
 *
 * Notes:
 *	If you change this function, you MUST also update TclLimitReady() in
 *	tclInt.h.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitReady(
    Tcl_Interp *interp)
{
    register Interp *iPtr = (Interp *) interp;

    if (iPtr->limit.active != 0) {
	register int ticker = ++iPtr->limit.granularityTicker;

	if ((iPtr->limit.active & TCL_LIMIT_COMMANDS) &&
		((iPtr->limit.cmdGranularity == 1) ||
		    (ticker % iPtr->limit.cmdGranularity == 0))) {
	    return 1;
	}
	if ((iPtr->limit.active & TCL_LIMIT_TIME) &&
		((iPtr->limit.timeGranularity == 1) ||
		    (ticker % iPtr->limit.timeGranularity == 0))) {
	    return 1;
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitCheck --
 *
 *	Check all currently set limits in the interpreter (where permitted by
 *	granularity). If a limit is exceeded, call its callbacks and, if the
 *	limit is still exceeded after the callbacks have run, make the
 *	interpreter generate an error that cannot be caught within the limited
 *	interpreter.
 *
 * Results:
 *	A Tcl result value (TCL_OK if no limit is exceeded, and TCL_ERROR if a
 *	limit has been exceeded).
 *
 * Side effects:
 *	May invoke system calls. May invoke other interpreters. May be
 *	reentrant. May put the interpreter into a state where it can no longer
 *	execute commands without outside intervention.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitCheck(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;
    register int ticker = iPtr->limit.granularityTicker;

    if (Tcl_InterpDeleted(interp)) {
	return TCL_OK;
    }

    if ((iPtr->limit.active & TCL_LIMIT_COMMANDS) &&
	    ((iPtr->limit.cmdGranularity == 1) ||
		    (ticker % iPtr->limit.cmdGranularity == 0)) &&
	    (iPtr->limit.cmdCount < iPtr->cmdCount)) {
	iPtr->limit.exceeded |= TCL_LIMIT_COMMANDS;
	Tcl_Preserve(interp);
	RunLimitHandlers(iPtr->limit.cmdHandlers, interp);
	if (iPtr->limit.cmdCount >= iPtr->cmdCount) {
	    iPtr->limit.exceeded &= ~TCL_LIMIT_COMMANDS;
	} else if (iPtr->limit.exceeded & TCL_LIMIT_COMMANDS) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "command count limit exceeded", -1));
	    Tcl_SetErrorCode(interp, "TCL", "LIMIT", "COMMANDS", NULL);
	    Tcl_Release(interp);
	    return TCL_ERROR;
	}
	Tcl_Release(interp);
    }

    if ((iPtr->limit.active & TCL_LIMIT_TIME) &&
	    ((iPtr->limit.timeGranularity == 1) ||
		(ticker % iPtr->limit.timeGranularity == 0))) {
	Tcl_Time now;

	Tcl_GetTime(&now);
	if (iPtr->limit.time.sec < now.sec ||
		(iPtr->limit.time.sec == now.sec &&
		iPtr->limit.time.usec < now.usec)) {
	    iPtr->limit.exceeded |= TCL_LIMIT_TIME;
	    Tcl_Preserve(interp);
	    RunLimitHandlers(iPtr->limit.timeHandlers, interp);
	    if (iPtr->limit.time.sec > now.sec ||
		    (iPtr->limit.time.sec == now.sec &&
		    iPtr->limit.time.usec >= now.usec)) {
		iPtr->limit.exceeded &= ~TCL_LIMIT_TIME;
	    } else if (iPtr->limit.exceeded & TCL_LIMIT_TIME) {
		Tcl_SetObjResult(interp, Tcl_NewStringObj(
			"time limit exceeded", -1));
		Tcl_SetErrorCode(interp, "TCL", "LIMIT", "TIME", NULL);
		Tcl_Release(interp);
		return TCL_ERROR;
	    }
	    Tcl_Release(interp);
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RunLimitHandlers --
 *
 *	Invoke all the limit handlers in a list (for a particular limit).
 *	Note that no particular limit handler callback will be invoked
 *	reentrantly.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the limit handlers.
 *
 *----------------------------------------------------------------------
 */

static void
RunLimitHandlers(
    LimitHandler *handlerPtr,
    Tcl_Interp *interp)
{
    LimitHandler *nextPtr;
    for (; handlerPtr!=NULL ; handlerPtr=nextPtr) {
	if (handlerPtr->flags & (LIMIT_HANDLER_DELETED|LIMIT_HANDLER_ACTIVE)) {
	    /*
	     * Reentrant call or something seriously strange in the delete
	     * code.
	     */

	    nextPtr = handlerPtr->nextPtr;
	    continue;
	}

	/*
	 * Set the ACTIVE flag while running the limit handler itself so we
	 * cannot reentrantly call this handler and know to use the alternate
	 * method of deletion if necessary.
	 */

	handlerPtr->flags |= LIMIT_HANDLER_ACTIVE;
	handlerPtr->handlerProc(handlerPtr->clientData, interp);
	handlerPtr->flags &= ~LIMIT_HANDLER_ACTIVE;

	/*
	 * Rediscover this value; it might have changed during the processing
	 * of a limit handler. We have to record it here because we might
	 * delete the structure below, and reading a value out of a deleted
	 * structure is unsafe (even if actually legal with some
	 * malloc()/free() implementations.)
	 */

	nextPtr = handlerPtr->nextPtr;

	/*
	 * If we deleted the current handler while we were executing it, we
	 * will have spliced it out of the list and set the
	 * LIMIT_HANDLER_DELETED flag.
	 */

	if (handlerPtr->flags & LIMIT_HANDLER_DELETED) {
	    if (handlerPtr->deleteProc != NULL) {
		handlerPtr->deleteProc(handlerPtr->clientData);
	    }
	    ckfree(handlerPtr);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitAddHandler --
 *
 *	Add a callback handler for a particular resource limit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Extends the internal linked list of handlers for a limit.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitAddHandler(
    Tcl_Interp *interp,
    int type,
    Tcl_LimitHandlerProc *handlerProc,
    ClientData clientData,
    Tcl_LimitHandlerDeleteProc *deleteProc)
{
    Interp *iPtr = (Interp *) interp;
    LimitHandler *handlerPtr;

    /*
     * Convert everything into a real deletion callback.
     */

    if (deleteProc == (Tcl_LimitHandlerDeleteProc *) TCL_DYNAMIC) {
	deleteProc = (Tcl_LimitHandlerDeleteProc *) Tcl_Free;
    }
    if (deleteProc == (Tcl_LimitHandlerDeleteProc *) TCL_STATIC) {
	deleteProc = NULL;
    }

    /*
     * Allocate a handler record.
     */

    handlerPtr = ckalloc(sizeof(LimitHandler));
    handlerPtr->flags = 0;
    handlerPtr->handlerProc = handlerProc;
    handlerPtr->clientData = clientData;
    handlerPtr->deleteProc = deleteProc;
    handlerPtr->prevPtr = NULL;

    /*
     * Prepend onto the front of the correct linked list.
     */

    switch (type) {
    case TCL_LIMIT_COMMANDS:
	handlerPtr->nextPtr = iPtr->limit.cmdHandlers;
	if (handlerPtr->nextPtr != NULL) {
	    handlerPtr->nextPtr->prevPtr = handlerPtr;
	}
	iPtr->limit.cmdHandlers = handlerPtr;
	return;

    case TCL_LIMIT_TIME:
	handlerPtr->nextPtr = iPtr->limit.timeHandlers;
	if (handlerPtr->nextPtr != NULL) {
	    handlerPtr->nextPtr->prevPtr = handlerPtr;
	}
	iPtr->limit.timeHandlers = handlerPtr;
	return;
    }

    Tcl_Panic("unknown type of resource limit");
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitRemoveHandler --
 *
 *	Remove a callback handler for a particular resource limit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The handler is spliced out of the internal linked list for the limit,
 *	and if not currently being invoked, deleted. Otherwise it is just
 *	marked for deletion and removed when the limit handler has finished
 *	executing.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitRemoveHandler(
    Tcl_Interp *interp,
    int type,
    Tcl_LimitHandlerProc *handlerProc,
    ClientData clientData)
{
    Interp *iPtr = (Interp *) interp;
    LimitHandler *handlerPtr;

    switch (type) {
    case TCL_LIMIT_COMMANDS:
	handlerPtr = iPtr->limit.cmdHandlers;
	break;
    case TCL_LIMIT_TIME:
	handlerPtr = iPtr->limit.timeHandlers;
	break;
    default:
	Tcl_Panic("unknown type of resource limit");
	return;
    }

    for (; handlerPtr!=NULL ; handlerPtr=handlerPtr->nextPtr) {
	if ((handlerPtr->handlerProc != handlerProc) ||
		(handlerPtr->clientData != clientData)) {
	    continue;
	}

	/*
	 * We've found the handler to delete; mark it as doomed if not already
	 * so marked (which shouldn't actually happen).
	 */

	if (handlerPtr->flags & LIMIT_HANDLER_DELETED) {
	    return;
	}
	handlerPtr->flags |= LIMIT_HANDLER_DELETED;

	/*
	 * Splice the handler out of the doubly-linked list.
	 */

	if (handlerPtr->prevPtr == NULL) {
	    switch (type) {
	    case TCL_LIMIT_COMMANDS:
		iPtr->limit.cmdHandlers = handlerPtr->nextPtr;
		break;
	    case TCL_LIMIT_TIME:
		iPtr->limit.timeHandlers = handlerPtr->nextPtr;
		break;
	    }
	} else {
	    handlerPtr->prevPtr->nextPtr = handlerPtr->nextPtr;
	}
	if (handlerPtr->nextPtr != NULL) {
	    handlerPtr->nextPtr->prevPtr = handlerPtr->prevPtr;
	}

	/*
	 * If nothing is currently executing the handler, delete its client
	 * data and the overall handler structure now. Otherwise it will all
	 * go away when the handler returns.
	 */

	if (!(handlerPtr->flags & LIMIT_HANDLER_ACTIVE)) {
	    if (handlerPtr->deleteProc != NULL) {
		handlerPtr->deleteProc(handlerPtr->clientData);
	    }
	    ckfree(handlerPtr);
	}
	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclLimitRemoveAllHandlers --
 *
 *	Remove all limit callback handlers for an interpreter. This is invoked
 *	as part of deleting the interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Limit handlers are deleted or marked for deletion (as with
 *	Tcl_LimitRemoveHandler).
 *
 *----------------------------------------------------------------------
 */

void
TclLimitRemoveAllHandlers(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;
    LimitHandler *handlerPtr, *nextHandlerPtr;

    /*
     * Delete all command-limit handlers.
     */

    for (handlerPtr=iPtr->limit.cmdHandlers, iPtr->limit.cmdHandlers=NULL;
	    handlerPtr!=NULL; handlerPtr=nextHandlerPtr) {
	nextHandlerPtr = handlerPtr->nextPtr;

	/*
	 * Do not delete here if it has already been marked for deletion.
	 */

	if (handlerPtr->flags & LIMIT_HANDLER_DELETED) {
	    continue;
	}
	handlerPtr->flags |= LIMIT_HANDLER_DELETED;
	handlerPtr->prevPtr = NULL;
	handlerPtr->nextPtr = NULL;

	/*
	 * If nothing is currently executing the handler, delete its client
	 * data and the overall handler structure now. Otherwise it will all
	 * go away when the handler returns.
	 */

	if (!(handlerPtr->flags & LIMIT_HANDLER_ACTIVE)) {
	    if (handlerPtr->deleteProc != NULL) {
		handlerPtr->deleteProc(handlerPtr->clientData);
	    }
	    ckfree(handlerPtr);
	}
    }

    /*
     * Delete all time-limit handlers.
     */

    for (handlerPtr=iPtr->limit.timeHandlers, iPtr->limit.timeHandlers=NULL;
	    handlerPtr!=NULL; handlerPtr=nextHandlerPtr) {
	nextHandlerPtr = handlerPtr->nextPtr;

	/*
	 * Do not delete here if it has already been marked for deletion.
	 */

	if (handlerPtr->flags & LIMIT_HANDLER_DELETED) {
	    continue;
	}
	handlerPtr->flags |= LIMIT_HANDLER_DELETED;
	handlerPtr->prevPtr = NULL;
	handlerPtr->nextPtr = NULL;

	/*
	 * If nothing is currently executing the handler, delete its client
	 * data and the overall handler structure now. Otherwise it will all
	 * go away when the handler returns.
	 */

	if (!(handlerPtr->flags & LIMIT_HANDLER_ACTIVE)) {
	    if (handlerPtr->deleteProc != NULL) {
		handlerPtr->deleteProc(handlerPtr->clientData);
	    }
	    ckfree(handlerPtr);
	}
    }

    /*
     * Delete the timer callback that is used to trap limits that occur in
     * [vwait]s...
     */

    if (iPtr->limit.timeEvent != NULL) {
	Tcl_DeleteTimerHandler(iPtr->limit.timeEvent);
	iPtr->limit.timeEvent = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitTypeEnabled --
 *
 *	Check whether a particular limit has been enabled for an interpreter.
 *
 * Results:
 *	A boolean value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitTypeEnabled(
    Tcl_Interp *interp,
    int type)
{
    Interp *iPtr = (Interp *) interp;

    return (iPtr->limit.active & type) != 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitTypeExceeded --
 *
 *	Check whether a particular limit has been exceeded for an interpreter.
 *
 * Results:
 *	A boolean value (note that Tcl_LimitExceeded will always return
 *	non-zero when this function returns non-zero).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitTypeExceeded(
    Tcl_Interp *interp,
    int type)
{
    Interp *iPtr = (Interp *) interp;

    return (iPtr->limit.exceeded & type) != 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitTypeSet --
 *
 *	Enable a particular limit for an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The limit is turned on and will be checked in future at an interval
 *	determined by the frequency of calling of Tcl_LimitReady and the
 *	granularity of the limit in question.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitTypeSet(
    Tcl_Interp *interp,
    int type)
{
    Interp *iPtr = (Interp *) interp;

    iPtr->limit.active |= type;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitTypeReset --
 *
 *	Disable a particular limit for an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The limit is disabled. If the limit was exceeded when this function
 *	was called, the limit will no longer be exceeded afterwards and the
 *	interpreter will be free to execute further scripts (assuming it isn't
 *	also deleted, of course).
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitTypeReset(
    Tcl_Interp *interp,
    int type)
{
    Interp *iPtr = (Interp *) interp;

    iPtr->limit.active &= ~type;
    iPtr->limit.exceeded &= ~type;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitSetCommands --
 *
 *	Set the command limit for an interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Also resets whether the command limit was exceeded. This might permit
 *	a small amount of further execution in the interpreter even if the
 *	limit itself is theoretically exceeded.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitSetCommands(
    Tcl_Interp *interp,
    int commandLimit)
{
    Interp *iPtr = (Interp *) interp;

    iPtr->limit.cmdCount = commandLimit;
    iPtr->limit.exceeded &= ~TCL_LIMIT_COMMANDS;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitGetCommands --
 *
 *	Get the number of commands that may be executed in the interpreter
 *	before the command-limit is reached.
 *
 * Results:
 *	An upper bound on the number of commands.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitGetCommands(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;

    return iPtr->limit.cmdCount;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitSetTime --
 *
 *	Set the time limit for an interpreter by copying it from the value
 *	pointed to by the timeLimitPtr argument.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Also resets whether the time limit was exceeded. This might permit a
 *	small amount of further execution in the interpreter even if the limit
 *	itself is theoretically exceeded.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitSetTime(
    Tcl_Interp *interp,
    Tcl_Time *timeLimitPtr)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_Time nextMoment;

    memcpy(&iPtr->limit.time, timeLimitPtr, sizeof(Tcl_Time));
    if (iPtr->limit.timeEvent != NULL) {
	Tcl_DeleteTimerHandler(iPtr->limit.timeEvent);
    }
    nextMoment.sec = timeLimitPtr->sec;
    nextMoment.usec = timeLimitPtr->usec+10;
    if (nextMoment.usec >= 1000000) {
	nextMoment.sec++;
	nextMoment.usec -= 1000000;
    }
    iPtr->limit.timeEvent = TclCreateAbsoluteTimerHandler(&nextMoment,
	    TimeLimitCallback, interp);
    iPtr->limit.exceeded &= ~TCL_LIMIT_TIME;
}

/*
 *----------------------------------------------------------------------
 *
 * TimeLimitCallback --
 *
 *	Callback that allows time limits to be enforced even when doing a
 *	blocking wait for events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May put the interpreter into a state where it can no longer execute
 *	commands. May make callbacks into other interpreters.
 *
 *----------------------------------------------------------------------
 */

static void
TimeLimitCallback(
    ClientData clientData)
{
    Tcl_Interp *interp = clientData;
    Interp *iPtr = clientData;
    int code;

    Tcl_Preserve(interp);
    iPtr->limit.timeEvent = NULL;

    /*
     * Must reset the granularity ticker here to force an immediate full
     * check. This is OK because we're swallowing the cost in the overall cost
     * of the event loop. [Bug 2891362]
     */

    iPtr->limit.granularityTicker = 0;

    code = Tcl_LimitCheck(interp);
    if (code != TCL_OK) {
	Tcl_AddErrorInfo(interp, "\n    (while waiting for event)");
	Tcl_BackgroundException(interp, code);
    }
    Tcl_Release(interp);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitGetTime --
 *
 *	Get the current time limit.
 *
 * Results:
 *	The time limit (by it being copied into the variable pointed to by the
 *	timeLimitPtr).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitGetTime(
    Tcl_Interp *interp,
    Tcl_Time *timeLimitPtr)
{
    Interp *iPtr = (Interp *) interp;

    memcpy(timeLimitPtr, &iPtr->limit.time, sizeof(Tcl_Time));
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitSetGranularity --
 *
 *	Set the granularity divisor (which must be positive) for a particular
 *	limit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The granularity is updated.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_LimitSetGranularity(
    Tcl_Interp *interp,
    int type,
    int granularity)
{
    Interp *iPtr = (Interp *) interp;
    if (granularity < 1) {
	Tcl_Panic("limit granularity must be positive");
    }

    switch (type) {
    case TCL_LIMIT_COMMANDS:
	iPtr->limit.cmdGranularity = granularity;
	return;
    case TCL_LIMIT_TIME:
	iPtr->limit.timeGranularity = granularity;
	return;
    }
    Tcl_Panic("unknown type of resource limit");
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LimitGetGranularity --
 *
 *	Get the granularity divisor for a particular limit.
 *
 * Results:
 *	The granularity divisor for the given limit.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_LimitGetGranularity(
    Tcl_Interp *interp,
    int type)
{
    Interp *iPtr = (Interp *) interp;

    switch (type) {
    case TCL_LIMIT_COMMANDS:
	return iPtr->limit.cmdGranularity;
    case TCL_LIMIT_TIME:
	return iPtr->limit.timeGranularity;
    }
    Tcl_Panic("unknown type of resource limit");
    return -1; /* NOT REACHED */
}

/*
 *----------------------------------------------------------------------
 *
 * DeleteScriptLimitCallback --
 *
 *	Callback for when a script limit (a limit callback implemented as a
 *	Tcl script in a master interpreter, as set up from Tcl) is deleted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference to the script callback from the controlling interpreter
 *	is removed.
 *
 *----------------------------------------------------------------------
 */

static void
DeleteScriptLimitCallback(
    ClientData clientData)
{
    ScriptLimitCallback *limitCBPtr = clientData;

    Tcl_DecrRefCount(limitCBPtr->scriptObj);
    if (limitCBPtr->entryPtr != NULL) {
	Tcl_DeleteHashEntry(limitCBPtr->entryPtr);
    }
    ckfree(limitCBPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * CallScriptLimitCallback --
 *
 *	Invoke a script limit callback. Used to implement limit callbacks set
 *	at the Tcl level on child interpreters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the callback script. Errors are reported as background
 *	errors.
 *
 *----------------------------------------------------------------------
 */

static void
CallScriptLimitCallback(
    ClientData clientData,
    Tcl_Interp *interp)		/* Interpreter which failed the limit */
{
    ScriptLimitCallback *limitCBPtr = clientData;
    int code;

    if (Tcl_InterpDeleted(limitCBPtr->interp)) {
	return;
    }
    Tcl_Preserve(limitCBPtr->interp);
    code = Tcl_EvalObjEx(limitCBPtr->interp, limitCBPtr->scriptObj,
	    TCL_EVAL_GLOBAL);
    if (code != TCL_OK && !Tcl_InterpDeleted(limitCBPtr->interp)) {
	Tcl_BackgroundException(limitCBPtr->interp, code);
    }
    Tcl_Release(limitCBPtr->interp);
}

/*
 *----------------------------------------------------------------------
 *
 * SetScriptLimitCallback --
 *
 *	Install (or remove, if scriptObj is NULL) a limit callback script that
 *	is called when the target interpreter exceeds the type of limit
 *	specified. Each interpreter may only have one callback set on another
 *	interpreter through this mechanism (though as many interpreters may be
 *	limited as the programmer chooses overall).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A limit callback implemented as an invokation of a Tcl script in
 *	another interpreter is either installed or removed.
 *
 *----------------------------------------------------------------------
 */

static void
SetScriptLimitCallback(
    Tcl_Interp *interp,
    int type,
    Tcl_Interp *targetInterp,
    Tcl_Obj *scriptObj)
{
    ScriptLimitCallback *limitCBPtr;
    Tcl_HashEntry *hashPtr;
    int isNew;
    ScriptLimitCallbackKey key;
    Interp *iPtr = (Interp *) interp;

    if (interp == targetInterp) {
	Tcl_Panic("installing limit callback to the limited interpreter");
    }

    key.interp = targetInterp;
    key.type = type;

    if (scriptObj == NULL) {
	hashPtr = Tcl_FindHashEntry(&iPtr->limit.callbacks, (char *) &key);
	if (hashPtr != NULL) {
	    Tcl_LimitRemoveHandler(targetInterp, type, CallScriptLimitCallback,
		    Tcl_GetHashValue(hashPtr));
	}
	return;
    }

    hashPtr = Tcl_CreateHashEntry(&iPtr->limit.callbacks, &key,
	    &isNew);
    if (!isNew) {
	limitCBPtr = Tcl_GetHashValue(hashPtr);
	limitCBPtr->entryPtr = NULL;
	Tcl_LimitRemoveHandler(targetInterp, type, CallScriptLimitCallback,
		limitCBPtr);
    }

    limitCBPtr = ckalloc(sizeof(ScriptLimitCallback));
    limitCBPtr->interp = interp;
    limitCBPtr->scriptObj = scriptObj;
    limitCBPtr->entryPtr = hashPtr;
    limitCBPtr->type = type;
    Tcl_IncrRefCount(scriptObj);

    Tcl_LimitAddHandler(targetInterp, type, CallScriptLimitCallback,
	    limitCBPtr, DeleteScriptLimitCallback);
    Tcl_SetHashValue(hashPtr, limitCBPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclRemoveScriptLimitCallbacks --
 *
 *	Remove all script-implemented limit callbacks that make calls back
 *	into the given interpreter. This invoked as part of deleting an
 *	interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The script limit callbacks are removed or marked for later removal.
 *
 *----------------------------------------------------------------------
 */

void
TclRemoveScriptLimitCallbacks(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *hashPtr;
    Tcl_HashSearch search;
    ScriptLimitCallbackKey *keyPtr;

    hashPtr = Tcl_FirstHashEntry(&iPtr->limit.callbacks, &search);
    while (hashPtr != NULL) {
	keyPtr = (ScriptLimitCallbackKey *)
		Tcl_GetHashKey(&iPtr->limit.callbacks, hashPtr);
	Tcl_LimitRemoveHandler(keyPtr->interp, keyPtr->type,
		CallScriptLimitCallback, Tcl_GetHashValue(hashPtr));
	hashPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&iPtr->limit.callbacks);
}

/*
 *----------------------------------------------------------------------
 *
 * TclInitLimitSupport --
 *
 *	Initialise all the parts of the interpreter relating to resource limit
 *	management. This allows an interpreter to both have limits set upon
 *	itself and set limits upon other interpreters.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The resource limit subsystem is initialised for the interpreter.
 *
 *----------------------------------------------------------------------
 */

void
TclInitLimitSupport(
    Tcl_Interp *interp)
{
    Interp *iPtr = (Interp *) interp;

    iPtr->limit.active = 0;
    iPtr->limit.granularityTicker = 0;
    iPtr->limit.exceeded = 0;
    iPtr->limit.cmdCount = 0;
    iPtr->limit.cmdHandlers = NULL;
    iPtr->limit.cmdGranularity = 1;
    memset(&iPtr->limit.time, 0, sizeof(Tcl_Time));
    iPtr->limit.timeHandlers = NULL;
    iPtr->limit.timeEvent = NULL;
    iPtr->limit.timeGranularity = 10;
    Tcl_InitHashTable(&iPtr->limit.callbacks,
	    sizeof(ScriptLimitCallbackKey)/sizeof(int));
}

/*
 *----------------------------------------------------------------------
 *
 * InheritLimitsFromMaster --
 *
 *	Derive the interpreter limit configuration for a slave interpreter
 *	from the limit config for the master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slave interpreter limits are set so that if the master has a
 *	limit, it may not exceed it by handing off work to slave interpreters.
 *	Note that this does not transfer limit callbacks from the master to
 *	the slave.
 *
 *----------------------------------------------------------------------
 */

static void
InheritLimitsFromMaster(
    Tcl_Interp *slaveInterp,
    Tcl_Interp *masterInterp)
{
    Interp *slavePtr = (Interp *) slaveInterp;
    Interp *masterPtr = (Interp *) masterInterp;

    if (masterPtr->limit.active & TCL_LIMIT_COMMANDS) {
	slavePtr->limit.active |= TCL_LIMIT_COMMANDS;
	slavePtr->limit.cmdCount = 0;
	slavePtr->limit.cmdGranularity = masterPtr->limit.cmdGranularity;
    }
    if (masterPtr->limit.active & TCL_LIMIT_TIME) {
	slavePtr->limit.active |= TCL_LIMIT_TIME;
	memcpy(&slavePtr->limit.time, &masterPtr->limit.time,
		sizeof(Tcl_Time));
	slavePtr->limit.timeGranularity = masterPtr->limit.timeGranularity;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveCommandLimitCmd --
 *
 *	Implementation of the [interp limit $i commands] and [$i limit
 *	commands] subcommands. See the interp manual page for a full
 *	description.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on the arguments.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveCommandLimitCmd(
    Tcl_Interp *interp,		/* Current interpreter. */
    Tcl_Interp *slaveInterp,	/* Interpreter being adjusted. */
    int consumedObjc,		/* Number of args already parsed. */
    int objc,			/* Total number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    static const char *const options[] = {
	"-command", "-granularity", "-value", NULL
    };
    enum Options {
	OPT_CMD, OPT_GRAN, OPT_VAL
    };
    Interp *iPtr = (Interp *) interp;
    int index;
    ScriptLimitCallbackKey key;
    ScriptLimitCallback *limitCBPtr;
    Tcl_HashEntry *hPtr;

    /*
     * First, ensure that we are not reading or writing the calling
     * interpreter's limits; it may only manipulate its children. Note that
     * the low level API enforces this with Tcl_Panic, which we want to
     * avoid. [Bug 3398794]
     */

    if (interp == slaveInterp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"limits on current interpreter inaccessible", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "SELF", NULL);
	return TCL_ERROR;
    }

    if (objc == consumedObjc) {
	Tcl_Obj *dictPtr;

	TclNewObj(dictPtr);
	key.interp = slaveInterp;
	key.type = TCL_LIMIT_COMMANDS;
	hPtr = Tcl_FindHashEntry(&iPtr->limit.callbacks, (char *) &key);
	if (hPtr != NULL) {
	    limitCBPtr = Tcl_GetHashValue(hPtr);
	    if (limitCBPtr != NULL && limitCBPtr->scriptObj != NULL) {
		Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[0], -1),
			limitCBPtr->scriptObj);
	    } else {
		goto putEmptyCommandInDict;
	    }
	} else {
	    Tcl_Obj *empty;

	putEmptyCommandInDict:
	    TclNewObj(empty);
	    Tcl_DictObjPut(NULL, dictPtr,
		    Tcl_NewStringObj(options[0], -1), empty);
	}
	Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[1], -1),
		Tcl_NewIntObj(Tcl_LimitGetGranularity(slaveInterp,
		TCL_LIMIT_COMMANDS)));

	if (Tcl_LimitTypeEnabled(slaveInterp, TCL_LIMIT_COMMANDS)) {
	    Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[2], -1),
		    Tcl_NewIntObj(Tcl_LimitGetCommands(slaveInterp)));
	} else {
	    Tcl_Obj *empty;

	    TclNewObj(empty);
	    Tcl_DictObjPut(NULL, dictPtr,
		    Tcl_NewStringObj(options[2], -1), empty);
	}
	Tcl_SetObjResult(interp, dictPtr);
	return TCL_OK;
    } else if (objc == consumedObjc+1) {
	if (Tcl_GetIndexFromObj(interp, objv[consumedObjc], options, "option",
		0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum Options) index) {
	case OPT_CMD:
	    key.interp = slaveInterp;
	    key.type = TCL_LIMIT_COMMANDS;
	    hPtr = Tcl_FindHashEntry(&iPtr->limit.callbacks, (char *) &key);
	    if (hPtr != NULL) {
		limitCBPtr = Tcl_GetHashValue(hPtr);
		if (limitCBPtr != NULL && limitCBPtr->scriptObj != NULL) {
		    Tcl_SetObjResult(interp, limitCBPtr->scriptObj);
		}
	    }
	    break;
	case OPT_GRAN:
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(
		    Tcl_LimitGetGranularity(slaveInterp, TCL_LIMIT_COMMANDS)));
	    break;
	case OPT_VAL:
	    if (Tcl_LimitTypeEnabled(slaveInterp, TCL_LIMIT_COMMANDS)) {
		Tcl_SetObjResult(interp,
			Tcl_NewIntObj(Tcl_LimitGetCommands(slaveInterp)));
	    }
	    break;
	}
	return TCL_OK;
    } else if ((objc-consumedObjc) & 1 /* isOdd(objc-consumedObjc) */) {
	Tcl_WrongNumArgs(interp, consumedObjc, objv, "?-option value ...?");
	return TCL_ERROR;
    } else {
	int i, scriptLen = 0, limitLen = 0;
	Tcl_Obj *scriptObj = NULL, *granObj = NULL, *limitObj = NULL;
	int gran = 0, limit = 0;

	for (i=consumedObjc ; i<objc ; i+=2) {
	    if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		    &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum Options) index) {
	    case OPT_CMD:
		scriptObj = objv[i+1];
		(void) Tcl_GetStringFromObj(objv[i+1], &scriptLen);
		break;
	    case OPT_GRAN:
		granObj = objv[i+1];
		if (TclGetIntFromObj(interp, objv[i+1], &gran) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (gran < 1) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "granularity must be at least 1", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADVALUE", NULL);
		    return TCL_ERROR;
		}
		break;
	    case OPT_VAL:
		limitObj = objv[i+1];
		(void) Tcl_GetStringFromObj(objv[i+1], &limitLen);
		if (limitLen == 0) {
		    break;
		}
		if (TclGetIntFromObj(interp, objv[i+1], &limit) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (limit < 0) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "command limit value must be at least 0", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADVALUE", NULL);
		    return TCL_ERROR;
		}
		break;
	    }
	}
	if (scriptObj != NULL) {
	    SetScriptLimitCallback(interp, TCL_LIMIT_COMMANDS, slaveInterp,
		    (scriptLen > 0 ? scriptObj : NULL));
	}
	if (granObj != NULL) {
	    Tcl_LimitSetGranularity(slaveInterp, TCL_LIMIT_COMMANDS, gran);
	}
	if (limitObj != NULL) {
	    if (limitLen > 0) {
		Tcl_LimitSetCommands(slaveInterp, limit);
		Tcl_LimitTypeSet(slaveInterp, TCL_LIMIT_COMMANDS);
	    } else {
		Tcl_LimitTypeReset(slaveInterp, TCL_LIMIT_COMMANDS);
	    }
	}
	return TCL_OK;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveTimeLimitCmd --
 *
 *	Implementation of the [interp limit $i time] and [$i limit time]
 *	subcommands. See the interp manual page for a full description.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Depends on the arguments.
 *
 *----------------------------------------------------------------------
 */

static int
SlaveTimeLimitCmd(
    Tcl_Interp *interp,			/* Current interpreter. */
    Tcl_Interp *slaveInterp,		/* Interpreter being adjusted. */
    int consumedObjc,			/* Number of args already parsed. */
    int objc,				/* Total number of arguments. */
    Tcl_Obj *const objv[])		/* Argument objects. */
{
    static const char *const options[] = {
	"-command", "-granularity", "-milliseconds", "-seconds", NULL
    };
    enum Options {
	OPT_CMD, OPT_GRAN, OPT_MILLI, OPT_SEC
    };
    Interp *iPtr = (Interp *) interp;
    int index;
    ScriptLimitCallbackKey key;
    ScriptLimitCallback *limitCBPtr;
    Tcl_HashEntry *hPtr;

    /*
     * First, ensure that we are not reading or writing the calling
     * interpreter's limits; it may only manipulate its children. Note that
     * the low level API enforces this with Tcl_Panic, which we want to
     * avoid. [Bug 3398794]
     */

    if (interp == slaveInterp) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"limits on current interpreter inaccessible", -1));
	Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP", "SELF", NULL);
	return TCL_ERROR;
    }

    if (objc == consumedObjc) {
	Tcl_Obj *dictPtr;

	TclNewObj(dictPtr);
	key.interp = slaveInterp;
	key.type = TCL_LIMIT_TIME;
	hPtr = Tcl_FindHashEntry(&iPtr->limit.callbacks, (char *) &key);
	if (hPtr != NULL) {
	    limitCBPtr = Tcl_GetHashValue(hPtr);
	    if (limitCBPtr != NULL && limitCBPtr->scriptObj != NULL) {
		Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[0], -1),
			limitCBPtr->scriptObj);
	    } else {
		goto putEmptyCommandInDict;
	    }
	} else {
	    Tcl_Obj *empty;
	putEmptyCommandInDict:
	    TclNewObj(empty);
	    Tcl_DictObjPut(NULL, dictPtr,
		    Tcl_NewStringObj(options[0], -1), empty);
	}
	Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[1], -1),
		Tcl_NewIntObj(Tcl_LimitGetGranularity(slaveInterp,
		TCL_LIMIT_TIME)));

	if (Tcl_LimitTypeEnabled(slaveInterp, TCL_LIMIT_TIME)) {
	    Tcl_Time limitMoment;

	    Tcl_LimitGetTime(slaveInterp, &limitMoment);
	    Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[2], -1),
		    Tcl_NewLongObj(limitMoment.usec/1000));
	    Tcl_DictObjPut(NULL, dictPtr, Tcl_NewStringObj(options[3], -1),
		    Tcl_NewLongObj(limitMoment.sec));
	} else {
	    Tcl_Obj *empty;

	    TclNewObj(empty);
	    Tcl_DictObjPut(NULL, dictPtr,
		    Tcl_NewStringObj(options[2], -1), empty);
	    Tcl_DictObjPut(NULL, dictPtr,
		    Tcl_NewStringObj(options[3], -1), empty);
	}
	Tcl_SetObjResult(interp, dictPtr);
	return TCL_OK;
    } else if (objc == consumedObjc+1) {
	if (Tcl_GetIndexFromObj(interp, objv[consumedObjc], options, "option",
		0, &index) != TCL_OK) {
	    return TCL_ERROR;
	}
	switch ((enum Options) index) {
	case OPT_CMD:
	    key.interp = slaveInterp;
	    key.type = TCL_LIMIT_TIME;
	    hPtr = Tcl_FindHashEntry(&iPtr->limit.callbacks, (char *) &key);
	    if (hPtr != NULL) {
		limitCBPtr = Tcl_GetHashValue(hPtr);
		if (limitCBPtr != NULL && limitCBPtr->scriptObj != NULL) {
		    Tcl_SetObjResult(interp, limitCBPtr->scriptObj);
		}
	    }
	    break;
	case OPT_GRAN:
	    Tcl_SetObjResult(interp, Tcl_NewIntObj(
		    Tcl_LimitGetGranularity(slaveInterp, TCL_LIMIT_TIME)));
	    break;
	case OPT_MILLI:
	    if (Tcl_LimitTypeEnabled(slaveInterp, TCL_LIMIT_TIME)) {
		Tcl_Time limitMoment;

		Tcl_LimitGetTime(slaveInterp, &limitMoment);
		Tcl_SetObjResult(interp,
			Tcl_NewLongObj(limitMoment.usec/1000));
	    }
	    break;
	case OPT_SEC:
	    if (Tcl_LimitTypeEnabled(slaveInterp, TCL_LIMIT_TIME)) {
		Tcl_Time limitMoment;

		Tcl_LimitGetTime(slaveInterp, &limitMoment);
		Tcl_SetObjResult(interp, Tcl_NewLongObj(limitMoment.sec));
	    }
	    break;
	}
	return TCL_OK;
    } else if ((objc-consumedObjc) & 1 /* isOdd(objc-consumedObjc) */) {
	Tcl_WrongNumArgs(interp, consumedObjc, objv, "?-option value ...?");
	return TCL_ERROR;
    } else {
	int i, scriptLen = 0, milliLen = 0, secLen = 0;
	Tcl_Obj *scriptObj = NULL, *granObj = NULL;
	Tcl_Obj *milliObj = NULL, *secObj = NULL;
	int gran = 0;
	Tcl_Time limitMoment;
	int tmp;

	Tcl_LimitGetTime(slaveInterp, &limitMoment);
	for (i=consumedObjc ; i<objc ; i+=2) {
	    if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0,
		    &index) != TCL_OK) {
		return TCL_ERROR;
	    }
	    switch ((enum Options) index) {
	    case OPT_CMD:
		scriptObj = objv[i+1];
		(void) Tcl_GetStringFromObj(objv[i+1], &scriptLen);
		break;
	    case OPT_GRAN:
		granObj = objv[i+1];
		if (TclGetIntFromObj(interp, objv[i+1], &gran) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (gran < 1) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "granularity must be at least 1", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADVALUE", NULL);
		    return TCL_ERROR;
		}
		break;
	    case OPT_MILLI:
		milliObj = objv[i+1];
		(void) Tcl_GetStringFromObj(objv[i+1], &milliLen);
		if (milliLen == 0) {
		    break;
		}
		if (TclGetIntFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tmp < 0) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "milliseconds must be at least 0", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADVALUE", NULL);
		    return TCL_ERROR;
		}
		limitMoment.usec = ((long) tmp)*1000;
		break;
	    case OPT_SEC:
		secObj = objv[i+1];
		(void) Tcl_GetStringFromObj(objv[i+1], &secLen);
		if (secLen == 0) {
		    break;
		}
		if (TclGetIntFromObj(interp, objv[i+1], &tmp) != TCL_OK) {
		    return TCL_ERROR;
		}
		if (tmp < 0) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "seconds must be at least 0", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADVALUE", NULL);
		    return TCL_ERROR;
		}
		limitMoment.sec = tmp;
		break;
	    }
	}
	if (milliObj != NULL || secObj != NULL) {
	    if (milliObj != NULL) {
		/*
		 * Setting -milliseconds but clearing -seconds, or resetting
		 * -milliseconds but not resetting -seconds? Bad voodoo!
		 */

		if (secObj != NULL && secLen == 0 && milliLen > 0) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "may only set -milliseconds if -seconds is not "
			    "also being reset", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADUSAGE", NULL);
		    return TCL_ERROR;
		}
		if (milliLen == 0 && (secObj == NULL || secLen > 0)) {
		    Tcl_SetObjResult(interp, Tcl_NewStringObj(
			    "may only reset -milliseconds if -seconds is "
			    "also being reset", -1));
		    Tcl_SetErrorCode(interp, "TCL", "OPERATION", "INTERP",
			    "BADUSAGE", NULL);
		    return TCL_ERROR;
		}
	    }

	    if (milliLen > 0 || secLen > 0) {
		/*
		 * Force usec to be in range [0..1000000), possibly
		 * incrementing sec in the process. This makes it much easier
		 * for people to write scripts that do small time increments.
		 */

		limitMoment.sec += limitMoment.usec / 1000000;
		limitMoment.usec %= 1000000;

		Tcl_LimitSetTime(slaveInterp, &limitMoment);
		Tcl_LimitTypeSet(slaveInterp, TCL_LIMIT_TIME);
	    } else {
		Tcl_LimitTypeReset(slaveInterp, TCL_LIMIT_TIME);
	    }
	}
	if (scriptObj != NULL) {
	    SetScriptLimitCallback(interp, TCL_LIMIT_TIME, slaveInterp,
		    (scriptLen > 0 ? scriptObj : NULL));
	}
	if (granObj != NULL) {
	    Tcl_LimitSetGranularity(slaveInterp, TCL_LIMIT_TIME, gran);
	}
	return TCL_OK;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
