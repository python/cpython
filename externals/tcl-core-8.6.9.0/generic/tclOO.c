/*
 * tclOO.c --
 *
 *	This file contains the object-system core (NB: not Tcl_Obj, but ::oo)
 *
 * Copyright (c) 2005-2012 by Donal K. Fellows
 * Copyright (c) 2017 by Nathan Coulter
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "tclInt.h"
#include "tclOOInt.h"

/*
 * Commands in oo::define.
 */

static const struct {
    const char *name;
    Tcl_ObjCmdProc *objProc;
    int flag;
} defineCmds[] = {
    {"constructor", TclOODefineConstructorObjCmd, 0},
    {"deletemethod", TclOODefineDeleteMethodObjCmd, 0},
    {"destructor", TclOODefineDestructorObjCmd, 0},
    {"export", TclOODefineExportObjCmd, 0},
    {"forward", TclOODefineForwardObjCmd, 0},
    {"method", TclOODefineMethodObjCmd, 0},
    {"renamemethod", TclOODefineRenameMethodObjCmd, 0},
    {"self", TclOODefineSelfObjCmd, 0},
    {"unexport", TclOODefineUnexportObjCmd, 0},
    {NULL, NULL, 0}
}, objdefCmds[] = {
    {"class", TclOODefineClassObjCmd, 1},
    {"deletemethod", TclOODefineDeleteMethodObjCmd, 1},
    {"export", TclOODefineExportObjCmd, 1},
    {"forward", TclOODefineForwardObjCmd, 1},
    {"method", TclOODefineMethodObjCmd, 1},
    {"renamemethod", TclOODefineRenameMethodObjCmd, 1},
    {"unexport", TclOODefineUnexportObjCmd, 1},
    {NULL, NULL, 0}
};

/*
 * What sort of size of things we like to allocate.
 */

#define ALLOC_CHUNK 8

/*
 * Function declarations for things defined in this file.
 */

static Object *		AllocObject(Tcl_Interp *interp, const char *nameStr,
			    Namespace *nsPtr, const char *nsNameStr);
static int		CloneClassMethod(Tcl_Interp *interp, Class *clsPtr,
			    Method *mPtr, Tcl_Obj *namePtr,
			    Method **newMPtrPtr);
static int		CloneObjectMethod(Tcl_Interp *interp, Object *oPtr,
			    Method *mPtr, Tcl_Obj *namePtr);
static void		DeletedDefineNamespace(ClientData clientData);
static void		DeletedObjdefNamespace(ClientData clientData);
static void		DeletedHelpersNamespace(ClientData clientData);
static Tcl_NRPostProc	FinalizeAlloc;
static Tcl_NRPostProc	FinalizeNext;
static Tcl_NRPostProc	FinalizeObjectCall;
static void		initClassPath(Tcl_Interp * interp, Class *clsPtr);
static int		InitFoundation(Tcl_Interp *interp);
static void		KillFoundation(ClientData clientData,
			    Tcl_Interp *interp);
static void		MyDeleted(ClientData clientData);
static void		ObjectNamespaceDeleted(ClientData clientData);
static void		ObjectRenamedTrace(ClientData clientData,
			    Tcl_Interp *interp, const char *oldName,
			    const char *newName, int flags);
static inline void	SquelchCachedName(Object *oPtr);

static int		PublicObjectCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);
static int		PublicNRObjectCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);
static int		PrivateObjectCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);
static int		PrivateNRObjectCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const *objv);
static void		RemoveClass(Class ** list, int num, int idx);
static void		RemoveObject(Object ** list, int num, int idx);

/*
 * Methods in the oo::object and oo::class classes. First, we define a helper
 * macro that makes building the method type declaration structure a lot
 * easier. No point in making life harder than it has to be!
 *
 * Note that the core methods don't need clone or free proc callbacks.
 */

#define DCM(name,visibility,proc) \
    {name,visibility,\
	{TCL_OO_METHOD_VERSION_CURRENT,"core method: "#name,proc,NULL,NULL}}

static const DeclaredClassMethod objMethods[] = {
    DCM("destroy", 1,	TclOO_Object_Destroy),
    DCM("eval", 0,	TclOO_Object_Eval),
    DCM("unknown", 0,	TclOO_Object_Unknown),
    DCM("variable", 0,	TclOO_Object_LinkVar),
    DCM("varname", 0,	TclOO_Object_VarName),
    {NULL, 0, {0, NULL, NULL, NULL, NULL}}
}, clsMethods[] = {
    DCM("create", 1,	TclOO_Class_Create),
    DCM("new", 1,	TclOO_Class_New),
    DCM("createWithNamespace", 0, TclOO_Class_CreateNs),
    {NULL, 0, {0, NULL, NULL, NULL, NULL}}
};

/*
 * And for the oo::class constructor...
 */

static const Tcl_MethodType classConstructor = {
    TCL_OO_METHOD_VERSION_CURRENT,
    "oo::class constructor",
    TclOO_Class_Constructor, NULL, NULL
};

/*
 * Scripted parts of TclOO. First, the master script (cannot be outside this
 * file).
 */

static const char *initScript =
"package ifneeded TclOO " TCLOO_PATCHLEVEL " {# Already present, OK?};"
"namespace eval ::oo { variable version " TCLOO_VERSION " };"
"namespace eval ::oo { variable patchlevel " TCLOO_PATCHLEVEL " };";
/* "tcl_findLibrary tcloo $oo::version $oo::version" */
/* " tcloo.tcl OO_LIBRARY oo::library;"; */

/*
 * The scripted part of the definitions of slots.
 */

static const char *slotScript =
"::oo::define ::oo::Slot {\n"
"    method Get {} {error unimplemented}\n"
"    method Set list {error unimplemented}\n"
"    method -set args {\n"
"        uplevel 1 [list [namespace which my] Set $args]\n"
"    }\n"
"    method -append args {\n"
"        uplevel 1 [list [namespace which my] Set [list"
"                {*}[uplevel 1 [list [namespace which my] Get]] {*}$args]]\n"
"    }\n"
"    method -clear {} {uplevel 1 [list [namespace which my] Set {}]}\n"
"    forward --default-operation my -append\n"
"    method unknown {args} {\n"
"        set def --default-operation\n"
"        if {[llength $args] == 0} {\n"
"            return [uplevel 1 [list [namespace which my] $def]]\n"
"        } elseif {![string match -* [lindex $args 0]]} {\n"
"            return [uplevel 1 [list [namespace which my] $def {*}$args]]\n"
"        }\n"
"        next {*}$args\n"
"    }\n"
"    export -set -append -clear\n"
"    unexport unknown destroy\n"
"}\n"
"::oo::objdefine ::oo::define::superclass forward --default-operation my -set\n"
"::oo::objdefine ::oo::define::mixin forward --default-operation my -set\n"
"::oo::objdefine ::oo::objdefine::mixin forward --default-operation my -set\n";

/*
 * The body of the <cloned> method of oo::object.
 */

static const char *clonedBody =
"foreach p [info procs [info object namespace $originObject]::*] {"
"    set args [info args $p];"
"    set idx -1;"
"    foreach a $args {"
"        lset args [incr idx] "
"            [if {[info default $p $a d]} {list $a $d} {list $a}]"
"    };"
"    set b [info body $p];"
"    set p [namespace tail $p];"
"    proc $p $args $b;"
"};"
"foreach v [info vars [info object namespace $originObject]::*] {"
"    upvar 0 $v vOrigin;"
"    namespace upvar [namespace current] [namespace tail $v] vNew;"
"    if {[info exists vOrigin]} {"
"        if {[array exists vOrigin]} {"
"            array set vNew [array get vOrigin];"
"        } else {"
"            set vNew $vOrigin;"
"        }"
"    }"
"}";

/*
 * The actual definition of the variable holding the TclOO stub table.
 */

MODULE_SCOPE const TclOOStubs tclOOStubs;

/*
 * Convenience macro for getting the foundation from an interpreter.
 */

#define GetFoundation(interp) \
	((Foundation *)((Interp *)(interp))->objectFoundation)

/*
 * Macros to make inspecting into the guts of an object cleaner.
 *
 * The ocPtr parameter (only in these macros) is assumed to work fine with
 * either an oPtr or a classPtr. Note that the roots oo::object and oo::class
 * have _both_ their object and class flags tagged with ROOT_OBJECT and
 * ROOT_CLASS respectively.
 */

#define Deleted(oPtr)		((oPtr)->flags & OBJECT_DELETED)
#define IsRootObject(ocPtr)	((ocPtr)->flags & ROOT_OBJECT)
#define IsRootClass(ocPtr)	((ocPtr)->flags & ROOT_CLASS)
#define IsRoot(ocPtr)		((ocPtr)->flags & (ROOT_OBJECT|ROOT_CLASS))

#define RemoveItem(type, lst, i) \
    do { \
	Remove ## type ((lst).list, (lst).num, i); \
	(lst).num--; \
    } while (0)

/*
 * ----------------------------------------------------------------------
 *
 * TclOOInit --
 *
 *	Called to initialise the OO system within an interpreter.
 *
 * Result:
 *	TCL_OK if the setup succeeded. Currently assumed to always work.
 *
 * Side effects:
 *	Creates namespaces, commands, several classes and a number of
 *	callbacks. Upon return, the OO system is ready for use.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOInit(
    Tcl_Interp *interp)		/* The interpreter to install into. */
{
    /*
     * Build the core of the OO system.
     */

    if (InitFoundation(interp) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Run our initialization script and, if that works, declare the package
     * to be fully provided.
     */

    if (Tcl_Eval(interp, initScript) != TCL_OK) {
	return TCL_ERROR;
    }

    return Tcl_PkgProvideEx(interp, "TclOO", TCLOO_PATCHLEVEL,
	    (ClientData) &tclOOStubs);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOGetFoundation --
 *
 *	Get a reference to the OO core class system.
 *
 * ----------------------------------------------------------------------
 */

Foundation *
TclOOGetFoundation(
    Tcl_Interp *interp)
{
    return GetFoundation(interp);
}

/*
 * ----------------------------------------------------------------------
 *
 * InitFoundation --
 *
 *	Set up the core of the OO core class system. This is a structure
 *	holding references to the magical bits that need to be known about in
 *	other places, plus the oo::object and oo::class classes.
 *
 * ----------------------------------------------------------------------
 */

static int
InitFoundation(
    Tcl_Interp *interp)
{
    static Tcl_ThreadDataKey tsdKey;
    ThreadLocalData *tsdPtr =
	    Tcl_GetThreadData(&tsdKey, sizeof(ThreadLocalData));
    Foundation *fPtr = ckalloc(sizeof(Foundation));
    Tcl_Obj *namePtr, *argsPtr, *bodyPtr;

    Class fakeCls;
    Object fakeObject;

    Tcl_DString buffer;
    Command *cmdPtr;
    int i;

    /*
     * Initialize the structure that holds the OO system core. This is
     * attached to the interpreter via an assocData entry; not very efficient,
     * but the best we can do without hacking the core more.
     */

    memset(fPtr, 0, sizeof(Foundation));
    ((Interp *) interp)->objectFoundation = fPtr;
    fPtr->interp = interp;
    fPtr->ooNs = Tcl_CreateNamespace(interp, "::oo", fPtr, NULL);
    Tcl_Export(interp, fPtr->ooNs, "[a-z]*", 1);
    fPtr->defineNs = Tcl_CreateNamespace(interp, "::oo::define", fPtr,
	    DeletedDefineNamespace);
    fPtr->objdefNs = Tcl_CreateNamespace(interp, "::oo::objdefine", fPtr,
	    DeletedObjdefNamespace);
    fPtr->helpersNs = Tcl_CreateNamespace(interp, "::oo::Helpers", fPtr,
	    DeletedHelpersNamespace);
    fPtr->epoch = 0;
    fPtr->tsdPtr = tsdPtr;
    TclNewLiteralStringObj(fPtr->unknownMethodNameObj, "unknown");
    TclNewLiteralStringObj(fPtr->constructorName, "<constructor>");
    TclNewLiteralStringObj(fPtr->destructorName, "<destructor>");
    TclNewLiteralStringObj(fPtr->clonedName, "<cloned>");
    TclNewLiteralStringObj(fPtr->defineName, "::oo::define");
    Tcl_IncrRefCount(fPtr->unknownMethodNameObj);
    Tcl_IncrRefCount(fPtr->constructorName);
    Tcl_IncrRefCount(fPtr->destructorName);
    Tcl_IncrRefCount(fPtr->clonedName);
    Tcl_IncrRefCount(fPtr->defineName);
    Tcl_CreateObjCommand(interp, "::oo::UnknownDefinition",
	    TclOOUnknownDefinition, NULL, NULL);
    TclNewLiteralStringObj(namePtr, "::oo::UnknownDefinition");
    Tcl_SetNamespaceUnknownHandler(interp, fPtr->defineNs, namePtr);
    Tcl_SetNamespaceUnknownHandler(interp, fPtr->objdefNs, namePtr);

    /*
     * Create the subcommands in the oo::define and oo::objdefine spaces.
     */

    Tcl_DStringInit(&buffer);
    for (i=0 ; defineCmds[i].name ; i++) {
	TclDStringAppendLiteral(&buffer, "::oo::define::");
	Tcl_DStringAppend(&buffer, defineCmds[i].name, -1);
	Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
		defineCmds[i].objProc, INT2PTR(defineCmds[i].flag), NULL);
	Tcl_DStringFree(&buffer);
    }
    for (i=0 ; objdefCmds[i].name ; i++) {
	TclDStringAppendLiteral(&buffer, "::oo::objdefine::");
	Tcl_DStringAppend(&buffer, objdefCmds[i].name, -1);
	Tcl_CreateObjCommand(interp, Tcl_DStringValue(&buffer),
		objdefCmds[i].objProc, INT2PTR(objdefCmds[i].flag), NULL);
	Tcl_DStringFree(&buffer);
    }

    Tcl_CallWhenDeleted(interp, KillFoundation, NULL);

    /*
     * Create the objects at the core of the object system. These need to be
     * spliced manually.
     */

    /* Stand up a phony class for bootstrapping. */
    fPtr->objectCls = &fakeCls;
    /* referenced in TclOOAllocClass to increment the refCount. */
    fakeCls.thisPtr = &fakeObject;

    fPtr->objectCls = TclOOAllocClass(interp,
	    AllocObject(interp, "object", (Namespace *)fPtr->ooNs, NULL));
    /* Corresponding TclOODecrRefCount in KillFoudation */
    AddRef(fPtr->objectCls->thisPtr);

    /* This is why it is unnecessary in this routine to replace the
     * incremented reference count of fPtr->objectCls that was swallowed by
     * fakeObject. */
    fPtr->objectCls->superclasses.num = 0;
    ckfree(fPtr->objectCls->superclasses.list);
    fPtr->objectCls->superclasses.list = NULL;

    /* special initialization for the primordial objects */
    fPtr->objectCls->thisPtr->flags |= ROOT_OBJECT;
    fPtr->objectCls->flags |= ROOT_OBJECT;

    fPtr->classCls = TclOOAllocClass(interp,
	    AllocObject(interp, "class", (Namespace *)fPtr->ooNs, NULL));
    /* Corresponding TclOODecrRefCount in KillFoudation */
    AddRef(fPtr->classCls->thisPtr);

    /*
     * Increment reference counts for each reference because these
     * relationships can be dynamically changed.
     *
     * Corresponding TclOODecrRefCount for all incremented refcounts is in
     * KillFoundation.
     */

    /* Rewire bootstrapped objects. */
    fPtr->objectCls->thisPtr->selfCls = fPtr->classCls;
    AddRef(fPtr->classCls->thisPtr);
    TclOOAddToInstances(fPtr->objectCls->thisPtr, fPtr->classCls);

    fPtr->classCls->thisPtr->selfCls = fPtr->classCls;
    AddRef(fPtr->classCls->thisPtr);
    TclOOAddToInstances(fPtr->classCls->thisPtr, fPtr->classCls);

    fPtr->classCls->thisPtr->flags |= ROOT_CLASS;
    fPtr->classCls->flags |= ROOT_CLASS;

    /* Standard initialization for new Objects */
    TclOOAddToSubclasses(fPtr->classCls, fPtr->objectCls);

    /*
     * Basic method declarations for the core classes.
     */

    for (i=0 ; objMethods[i].name ; i++) {
	TclOONewBasicMethod(interp, fPtr->objectCls, &objMethods[i]);
    }
    for (i=0 ; clsMethods[i].name ; i++) {
	TclOONewBasicMethod(interp, fPtr->classCls, &clsMethods[i]);
    }

    /*
     * Create the default <cloned> method implementation, used when 'oo::copy'
     * is called to finish the copying of one object to another.
     */

    TclNewLiteralStringObj(argsPtr, "originObject");
    Tcl_IncrRefCount(argsPtr);
    bodyPtr = Tcl_NewStringObj(clonedBody, -1);
    TclOONewProcMethod(interp, fPtr->objectCls, 0, fPtr->clonedName, argsPtr,
	    bodyPtr, NULL);
    TclDecrRefCount(argsPtr);

    /*
     * Finish setting up the class of classes by marking the 'new' method as
     * private; classes, unlike general objects, must have explicit names. We
     * also need to create the constructor for classes.
     */

    TclNewLiteralStringObj(namePtr, "new");
    Tcl_NewInstanceMethod(interp, (Tcl_Object) fPtr->classCls->thisPtr,
	    namePtr /* keeps ref */, 0 /* ==private */, NULL, NULL);
    fPtr->classCls->constructorPtr = (Method *) Tcl_NewMethod(interp,
	    (Tcl_Class) fPtr->classCls, NULL, 0, &classConstructor, NULL);

    /*
     * Create non-object commands and plug ourselves into the Tcl [info]
     * ensemble.
     */

    cmdPtr = (Command *) Tcl_NRCreateCommand(interp, "::oo::Helpers::next",
	    NULL, TclOONextObjCmd, NULL, NULL);
    cmdPtr->compileProc = TclCompileObjectNextCmd;
    cmdPtr = (Command *) Tcl_NRCreateCommand(interp, "::oo::Helpers::nextto",
	    NULL, TclOONextToObjCmd, NULL, NULL);
    cmdPtr->compileProc = TclCompileObjectNextToCmd;
    cmdPtr = (Command *) Tcl_CreateObjCommand(interp, "::oo::Helpers::self",
	    TclOOSelfObjCmd, NULL, NULL);
    cmdPtr->compileProc = TclCompileObjectSelfCmd;
    Tcl_CreateObjCommand(interp, "::oo::define", TclOODefineObjCmd, NULL,
	    NULL);
    Tcl_CreateObjCommand(interp, "::oo::objdefine", TclOOObjDefObjCmd, NULL,
	    NULL);
    Tcl_CreateObjCommand(interp, "::oo::copy", TclOOCopyObjectCmd, NULL,NULL);
    TclOOInitInfo(interp);

    /*
     * Now make the class of slots.
     */

    if (TclOODefineSlots(fPtr) != TCL_OK) {
	return TCL_ERROR;
    }
    return Tcl_Eval(interp, slotScript);
}

/*
 * ----------------------------------------------------------------------
 *
 * DeletedDefineNamespace, DeletedObjdefNamespace, DeletedHelpersNamespace --
 *
 *	Simple helpers used to clear fields of the foundation when they no
 *	longer hold useful information.
 *
 * ----------------------------------------------------------------------
 */

static void
DeletedDefineNamespace(
    ClientData clientData)
{
    Foundation *fPtr = clientData;

    fPtr->defineNs = NULL;
}

static void
DeletedObjdefNamespace(
    ClientData clientData)
{
    Foundation *fPtr = clientData;

    fPtr->objdefNs = NULL;
}

static void
DeletedHelpersNamespace(
    ClientData clientData)
{
    Foundation *fPtr = clientData;

    fPtr->helpersNs = NULL;
}

/*
 * ----------------------------------------------------------------------
 *
 * KillFoundation --
 *
 *	Delete those parts of the OO core that are not deleted automatically
 *	when the objects and classes themselves are destroyed.
 *
 * ----------------------------------------------------------------------
 */

static void
KillFoundation(
    ClientData clientData,	/* Pointer to the OO system foundation
				 * structure. */
    Tcl_Interp *interp)		/* The interpreter containing the OO system
				 * foundation. */
{
    Foundation *fPtr = GetFoundation(interp);

    TclDecrRefCount(fPtr->unknownMethodNameObj);
    TclDecrRefCount(fPtr->constructorName);
    TclDecrRefCount(fPtr->destructorName);
    TclDecrRefCount(fPtr->clonedName);
    TclDecrRefCount(fPtr->defineName);
    TclOODecrRefCount(fPtr->objectCls->thisPtr);
    TclOODecrRefCount(fPtr->classCls->thisPtr);

    ckfree(fPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * AllocObject --
 *
 *	Allocate an object of basic type. Does not splice the object into its
 *	class's instance list.  The caller must set the classPtr on the object
 *	to either a class or NULL, call TclOOAddToInstances to add the object
 *	to the class's instance list, and if the object itself is a class, use
 *	call TclOOAddToSubclasses() to add it to the right class's list of
 *	subclasses.
 *
 * ----------------------------------------------------------------------
 */

static Object *
AllocObject(
    Tcl_Interp *interp,		/* Interpreter within which to create the
				 * object. */
    const char *nameStr,	/* The name of the object to create, or NULL
				 * if the OO system should pick the object
				 * name itself (equal to the namespace
				 * name). */
    Namespace *nsPtr,		/* The namespace to create the object in,
				   or NULL if *nameStr is NULL */
    const char *nsNameStr)	/* The name of the namespace to create, or
				 * NULL if the OO system should pick a unique
				 * name itself. If this is non-NULL but names
				 * a namespace that already exists, the effect
				 * will be the same as if this was NULL. */
{
    Foundation *fPtr = GetFoundation(interp);
    Object *oPtr;
    Command *cmdPtr;
    CommandTrace *tracePtr;
    int creationEpoch;

    oPtr = ckalloc(sizeof(Object));
    memset(oPtr, 0, sizeof(Object));

    /*
     * Every object has a namespace; make one. Note that this also normally
     * computes the creation epoch value for the object, a sequence number
     * that is unique to the object (and which allows us to manage method
     * caching without comparing pointers).
     *
     * When creating a namespace, we first check to see if the caller
     * specified the name for the namespace. If not, we generate namespace
     * names using the epoch until such time as a new namespace is actually
     * created.
     */

    if (nsNameStr != NULL) {
	oPtr->namespacePtr = Tcl_CreateNamespace(interp, nsNameStr, oPtr, NULL);
	if (oPtr->namespacePtr != NULL) {
	    creationEpoch = ++fPtr->tsdPtr->nsCount;
	    goto configNamespace;
	}
	Tcl_ResetResult(interp);
    }

    while (1) {
	char objName[10 + TCL_INTEGER_SPACE];

	sprintf(objName, "::oo::Obj%d", ++fPtr->tsdPtr->nsCount);
	oPtr->namespacePtr = Tcl_CreateNamespace(interp, objName, oPtr, NULL);
	if (oPtr->namespacePtr != NULL) {
	    creationEpoch = fPtr->tsdPtr->nsCount;
	    break;
	}

	/*
	 * Could not make that namespace, so we make another. But first we
	 * have to get rid of the error message from Tcl_CreateNamespace,
	 * since that's something that should not be exposed to the user.
	 */

	Tcl_ResetResult(interp);
    }


  configNamespace:

    ((Namespace *)oPtr->namespacePtr)->refCount++;

    /*
     * Make the namespace know about the helper commands. This grants access
     * to the [self] and [next] commands.
     */

    if (fPtr->helpersNs != NULL) {
	TclSetNsPath((Namespace *) oPtr->namespacePtr, 1, &fPtr->helpersNs);
    }
    TclOOSetupVariableResolver(oPtr->namespacePtr);

    /*
     * Suppress use of compiled versions of the commands in this object's
     * namespace and its children; causes wrong behaviour without expensive
     * recompilation. [Bug 2037727]
     */

    ((Namespace *) oPtr->namespacePtr)->flags |= NS_SUPPRESS_COMPILATION;

    /*
     * Set up a callback to get notification of the deletion of a namespace
     * when enough of the namespace still remains to execute commands and
     * access variables in it. [Bug 2950259]
     */

    ((Namespace *) oPtr->namespacePtr)->earlyDeleteProc = ObjectNamespaceDeleted;

    /*
     * Fill in the rest of the non-zero/NULL parts of the structure.
     */

    oPtr->fPtr = fPtr;
    oPtr->creationEpoch = creationEpoch;

    /*
     * An object starts life with a refCount of 2 to mark the two stages of
     * destruction it occur:  A call to ObjectRenamedTrace(), and a call to
     * ObjectNamespaceDeleted(). 
     */
    oPtr->refCount = 2;

    oPtr->flags = USE_CLASS_CACHE;

    /*
     * Finally, create the object commands and initialize the trace on the
     * public command (so that the object structures are deleted when the
     * command is deleted).
     */

    if (!nameStr) {
	nameStr = oPtr->namespacePtr->name;
	nsPtr = (Namespace *)oPtr->namespacePtr;
	if (nsPtr->parentPtr != NULL) {
	    nsPtr = nsPtr->parentPtr;
	}

    }
    oPtr->command = TclCreateObjCommandInNs(interp, nameStr,
	(Tcl_Namespace *)nsPtr, PublicObjectCmd, oPtr, NULL);

    /*
     * Add the NRE command and trace directly. While this breaks a number of
     * abstractions, it is faster and we're inside Tcl here so we're allowed.
     */

    cmdPtr = (Command *) oPtr->command;
    cmdPtr->nreProc = PublicNRObjectCmd;
    cmdPtr->tracePtr = tracePtr = ckalloc(sizeof(CommandTrace));
    tracePtr->traceProc = ObjectRenamedTrace;
    tracePtr->clientData = oPtr;
    tracePtr->flags = TCL_TRACE_RENAME|TCL_TRACE_DELETE;
    tracePtr->nextPtr = NULL;
    tracePtr->refCount = 1;

    oPtr->myCommand = TclNRCreateCommandInNs(interp, "my", oPtr->namespacePtr,
	PrivateObjectCmd, PrivateNRObjectCmd, oPtr, MyDeleted);
    return oPtr;
}

/*
 * ----------------------------------------------------------------------
 *
 * SquelchCachedName --
 *
 *	Encapsulates how to throw away a cached object name. Called from
 *	object rename traces and at object destruction.
 *
 * ----------------------------------------------------------------------
 */

static inline void
SquelchCachedName(
    Object *oPtr)
{
    if (oPtr->cachedNameObj) {
	Tcl_DecrRefCount(oPtr->cachedNameObj);
	oPtr->cachedNameObj = NULL;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * MyDeleted --
 *
 *	This callback is triggered when the object's [my] command is deleted
 *	by any mechanism. It just marks the object as not having a [my]
 *	command, and so prevents cleanup of that when the object itself is
 *	deleted.
 *
 * ----------------------------------------------------------------------
 */

static void
MyDeleted(
    ClientData clientData)	/* Reference to the object whose [my] has been
				 * squelched. */
{
    register Object *oPtr = clientData;

    oPtr->myCommand = NULL;
}

/*
 * ----------------------------------------------------------------------
 *
 * ObjectRenamedTrace --
 *
 *	This callback is triggered when the object is deleted by any
 *	mechanism. It runs the destructors and arranges for the actual cleanup
 *	of the object's namespace, which in turn triggers cleansing of the
 *	object data structures.
 *
 * ----------------------------------------------------------------------
 */

static void
ObjectRenamedTrace(
    ClientData clientData,	/* The object being deleted. */
    Tcl_Interp *interp,		/* The interpreter containing the object. */
    const char *oldName,	/* What the object was (last) called. */
    const char *newName,	/* What it's getting renamed to. (unused) */
    int flags)			/* Why was the object deleted? */
{
    Object *oPtr = clientData;
    /*
     * If this is a rename and not a delete of the object, we just flush the
     * cache of the object name.
     */

    if (flags & TCL_TRACE_RENAME) {
	SquelchCachedName(oPtr);
	return;
    }

    /*
     * The namespace is only deleted if it hasn't already been deleted. [Bug
     * 2950259].
     */

    if (!Deleted(oPtr)) {
	Tcl_DeleteNamespace(oPtr->namespacePtr);
    }
    oPtr->command = NULL;
    TclOODecrRefCount(oPtr);
    return;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODeleteDescendants --
 *
 *	Delete all descendants of a particular class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOODeleteDescendants(
    Tcl_Interp *interp,		/* The interpreter containing the class. */
    Object *oPtr)		/* The object representing the class. */
{
    Class *clsPtr = oPtr->classPtr, *subclassPtr, *mixinSubclassPtr;
    Object *instancePtr;

    /*
     * Squelch classes that this class has been mixed into.
     */

    if (clsPtr->mixinSubs.num > 0) {
	while (clsPtr->mixinSubs.num > 0) {
	    mixinSubclassPtr = clsPtr->mixinSubs.list[clsPtr->mixinSubs.num-1];
	    /* This condition also covers the case where mixinSubclassPtr ==
	     * clsPtr
	     */
	    if (!Deleted(mixinSubclassPtr->thisPtr)
		    && !(mixinSubclassPtr->thisPtr->flags & DONT_DELETE)) {
		Tcl_DeleteCommandFromToken(interp,
			mixinSubclassPtr->thisPtr->command);
	    }
	    TclOORemoveFromMixinSubs(mixinSubclassPtr, clsPtr);
	}
    }
    if (clsPtr->mixinSubs.size > 0) {
	ckfree(clsPtr->mixinSubs.list);
	clsPtr->mixinSubs.size = 0;
    }
    /*
     * Squelch subclasses of this class.
     */

    if (clsPtr->subclasses.num > 0) {
	while (clsPtr->subclasses.num > 0) {
	    subclassPtr = clsPtr->subclasses.list[clsPtr->subclasses.num-1];
	    if (!Deleted(subclassPtr->thisPtr) && !IsRoot(subclassPtr)
		    && !(subclassPtr->thisPtr->flags & DONT_DELETE)) {
		Tcl_DeleteCommandFromToken(interp,
			subclassPtr->thisPtr->command);
	    }
	    TclOORemoveFromSubclasses(subclassPtr, clsPtr);
	}
    }
    if (clsPtr->subclasses.size > 0) {
	ckfree(clsPtr->subclasses.list);
	clsPtr->subclasses.list = NULL;
	clsPtr->subclasses.size = 0;
    }

    /*
     * Squelch instances of this class (includes objects we're mixed into).
     */

    if (clsPtr->instances.num > 0) {
	while (clsPtr->instances.num > 0) {
	    instancePtr = clsPtr->instances.list[clsPtr->instances.num-1];
	    /* This condition also covers the case where instancePtr == oPtr */
	    if (!Deleted(instancePtr) && !IsRoot(instancePtr) &&
		    !(instancePtr->flags & DONT_DELETE)) {
		Tcl_DeleteCommandFromToken(interp, instancePtr->command);
	    }
	    TclOORemoveFromInstances(instancePtr, clsPtr);
	}
    }
    if (clsPtr->instances.size > 0) {
	ckfree(clsPtr->instances.list);
	clsPtr->instances.list = NULL;
	clsPtr->instances.size = 0;
    }
}
	

/*
 * ----------------------------------------------------------------------
 *
 * TclOOReleaseClassContents --
 *
 *	Tear down the special class data structure, including deleting all
 *	dependent classes and objects.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOReleaseClassContents(
    Tcl_Interp *interp,		/* The interpreter containing the class. */
    Object *oPtr)		/* The object representing the class. */
{
    FOREACH_HASH_DECLS;
    int i; 
    Class *clsPtr = oPtr->classPtr, *tmpClsPtr;
    Method *mPtr;
    Foundation *fPtr = oPtr->fPtr;
    Tcl_Obj *variableObj;

    /*
     * Sanity check!
     */

    if (!Deleted(oPtr)) {
	if (IsRootClass(oPtr)) {
	    Tcl_Panic("deleting class structure for non-deleted %s",
		    "::oo::class");
	} else if (IsRootObject(oPtr)) {
	    Tcl_Panic("deleting class structure for non-deleted %s",
		    "::oo::object");
	}
    }

    /*
     * Squelch method implementation chain caches.
     */

    if (clsPtr->constructorChainPtr) {
	TclOODeleteChain(clsPtr->constructorChainPtr);
	clsPtr->constructorChainPtr = NULL;
    }
    if (clsPtr->destructorChainPtr) {
	TclOODeleteChain(clsPtr->destructorChainPtr);
	clsPtr->destructorChainPtr = NULL;
    }
    if (clsPtr->classChainCache) {
	CallChain *callPtr;

	FOREACH_HASH_VALUE(callPtr, clsPtr->classChainCache) {
	    TclOODeleteChain(callPtr);
	}
	Tcl_DeleteHashTable(clsPtr->classChainCache);
	ckfree(clsPtr->classChainCache);
	clsPtr->classChainCache = NULL;
    }

    /*
     * Squelch our filter list.
     */

    if (clsPtr->filters.num) {
	Tcl_Obj *filterObj;

	FOREACH(filterObj, clsPtr->filters) {
	    TclDecrRefCount(filterObj);
	}
	ckfree(clsPtr->filters.list);
	clsPtr->filters.list = NULL;
	clsPtr->filters.num = 0;
    }

    /*
     * Squelch our metadata.
     */

    if (clsPtr->metadataPtr != NULL) {
	Tcl_ObjectMetadataType *metadataTypePtr;
	ClientData value;

	FOREACH_HASH(metadataTypePtr, value, clsPtr->metadataPtr) {
	    metadataTypePtr->deleteProc(value);
	}
	Tcl_DeleteHashTable(clsPtr->metadataPtr);
	ckfree(clsPtr->metadataPtr);
	clsPtr->metadataPtr = NULL;
    }

    if (clsPtr->mixins.num) {
	FOREACH(tmpClsPtr, clsPtr->mixins) {
	    TclOORemoveFromMixinSubs(clsPtr, tmpClsPtr);
	    TclOODecrRefCount(tmpClsPtr->thisPtr);
	}
	ckfree(clsPtr->mixins.list);
	clsPtr->mixins.list = NULL;
	clsPtr->mixins.num = 0;
    }

    if (clsPtr->superclasses.num > 0) {
	FOREACH(tmpClsPtr, clsPtr->superclasses) {
	    TclOORemoveFromSubclasses(clsPtr, tmpClsPtr);
	    TclOODecrRefCount(tmpClsPtr->thisPtr);
	}
	ckfree(clsPtr->superclasses.list);
	clsPtr->superclasses.num = 0;
	clsPtr->superclasses.list = NULL;
    }

    FOREACH_HASH_VALUE(mPtr, &clsPtr->classMethods) {
	TclOODelMethodRef(mPtr);
    }
    Tcl_DeleteHashTable(&clsPtr->classMethods);
    TclOODelMethodRef(clsPtr->constructorPtr);
    TclOODelMethodRef(clsPtr->destructorPtr);

    FOREACH(variableObj, clsPtr->variables) {
	TclDecrRefCount(variableObj);
    }
    if (i) {
	ckfree(clsPtr->variables.list);
    }

    if (IsRootClass(oPtr) && !Deleted(fPtr->objectCls->thisPtr)) {
	Tcl_DeleteCommandFromToken(interp, fPtr->objectCls->thisPtr->command);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * ObjectNamespaceDeleted --
 *
 *	Callback when the object's namespace is deleted. Used to clean up the
 *	data structures associated with the object. The complicated bit is
 *	that this can sometimes happen before the object's command is deleted
 *	(interpreter teardown is complex!)
 *
 * ----------------------------------------------------------------------
 */

static void
ObjectNamespaceDeleted(
    ClientData clientData)	/* Pointer to the class whose namespace is
				 * being deleted. */
{
    Object *oPtr = clientData;
    Foundation *fPtr = oPtr->fPtr;
    FOREACH_HASH_DECLS;
    Class *mixinPtr;
    Method *mPtr;
    Tcl_Obj *filterObj, *variableObj;
    Tcl_Interp *interp = oPtr->fPtr->interp;
    int i;

    if (Deleted(oPtr)) {
	/* To do:  Can ObjectNamespaceDeleted ever be called twice?  If not,
	 * this guard could be removed.
	 */
	return;
    }

    /*
     * One rule for the teardown routines is that if an object is in the
     * process of being deleted, nothing else may modify its bookeeping
     * records.  This is the flag that
     */
    oPtr->flags |= OBJECT_DELETED;

    /* Let the dominoes fall */
    if (oPtr->classPtr) {
	TclOODeleteDescendants(interp, oPtr);
    }

    /*
     * We do not run destructors on the core class objects when the
     * interpreter is being deleted; their incestuous nature causes problems
     * in that case when the destructor is partially deleted before the uses
     * of it have gone. [Bug 2949397]
     */
    if (!Tcl_InterpDeleted(interp) && !(oPtr->flags & DESTRUCTOR_CALLED)) {
	CallContext *contextPtr =
		TclOOGetCallContext(oPtr, NULL, DESTRUCTOR, NULL);
	int result;

	Tcl_InterpState state;
	oPtr->flags |= DESTRUCTOR_CALLED;

	if (contextPtr != NULL) {
	    contextPtr->callPtr->flags |= DESTRUCTOR;
	    contextPtr->skip = 0;
	    state = Tcl_SaveInterpState(interp, TCL_OK);
	    result = Tcl_NRCallObjProc(interp, TclOOInvokeContext,
		    contextPtr, 0, NULL);
	    if (result != TCL_OK) {
		Tcl_BackgroundException(interp, result);
	    }
	    Tcl_RestoreInterpState(interp, state);
	    TclOODeleteContext(contextPtr);
	}
    }

    /*
     * Instruct everyone to no longer use any allocated fields of the object.
     * Also delete the command that refers to the object at this point (if
     * it still exists) because otherwise its pointer to the object
     * points into freed memory.
     */

    if (((Command *)oPtr->command)->flags && CMD_IS_DELETED) {
	/*
	 * Something has already started the command deletion process. We can
	 * go ahead and clean up the the namespace,
	 */
    } else {
	/*
	 * The namespace must have been deleted directly.  Delete the command
	 * as well.
	 */
	Tcl_DeleteCommandFromToken(oPtr->fPtr->interp, oPtr->command);
    }

    if (oPtr->myCommand) {
	Tcl_DeleteCommandFromToken(oPtr->fPtr->interp, oPtr->myCommand);
    }

    /*
     * Splice the object out of its context. After this, we must *not* call
     * methods on the object.
     */

    /* To do: Should this be protected with a * !IsRoot() condition?  */ 
    TclOORemoveFromInstances(oPtr, oPtr->selfCls);

    if (oPtr->mixins.num > 0) {
	FOREACH(mixinPtr, oPtr->mixins) {
	    TclOORemoveFromInstances(oPtr, mixinPtr);
	    TclOODecrRefCount(mixinPtr->thisPtr);
	}
	if (oPtr->mixins.list != NULL) {
	    ckfree(oPtr->mixins.list);
	}
    }

    FOREACH(filterObj, oPtr->filters) {
	TclDecrRefCount(filterObj);
    }
    if (i) {
	ckfree(oPtr->filters.list);
    }

    if (oPtr->methodsPtr) {
	FOREACH_HASH_VALUE(mPtr, oPtr->methodsPtr) {
	    TclOODelMethodRef(mPtr);
	}
	Tcl_DeleteHashTable(oPtr->methodsPtr);
	ckfree(oPtr->methodsPtr);
    }

    FOREACH(variableObj, oPtr->variables) {
	TclDecrRefCount(variableObj);
    }
    if (i) {
	ckfree(oPtr->variables.list);
    }

    if (oPtr->chainCache) {
	TclOODeleteChainCache(oPtr->chainCache);
    }

    SquelchCachedName(oPtr);

    if (oPtr->metadataPtr != NULL) {
	Tcl_ObjectMetadataType *metadataTypePtr;
	ClientData value;

	FOREACH_HASH(metadataTypePtr, value, oPtr->metadataPtr) {
	    metadataTypePtr->deleteProc(value);
	}
	Tcl_DeleteHashTable(oPtr->metadataPtr);
	ckfree(oPtr->metadataPtr);
	oPtr->metadataPtr = NULL;
    }

    /*
     * Because an object can be a class that is an instance of itself, the
     * class object's class structure should only be cleaned after most of
     * the cleanup on the object is done. 
     *
     * The class of objects needs some special care; if it is deleted (and
     * we're not killing the whole interpreter) we force the delete of the
     * class of classes now as well. Due to the incestuous nature of those two
     * classes, if one goes the other must too and yet the tangle can
     * sometimes not go away automatically; we force it here. [Bug 2962664]
     */

    if (IsRootObject(oPtr) && !Deleted(fPtr->classCls->thisPtr)
	    && !Tcl_InterpDeleted(interp)) {

	Tcl_DeleteCommandFromToken(interp, fPtr->classCls->thisPtr->command);
    }

    if (oPtr->classPtr != NULL) {
	TclOOReleaseClassContents(interp, oPtr);
    }

    /*
     * Delete the object structure itself.
     */

    TclNsDecrRefCount((Namespace *)oPtr->namespacePtr);
    oPtr->namespacePtr = NULL;
    TclOODecrRefCount(oPtr->selfCls->thisPtr);
    oPtr->selfCls = NULL;
    TclOODecrRefCount(oPtr);
    return;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOODecrRef --
 *
 *	Decrement the refcount of an object and deallocate storage then object
 *	is no longer referenced.  Returns 1 if storage was deallocated, and 0
 *	otherwise.
 *
 * ----------------------------------------------------------------------
 */
int TclOODecrRefCount(Object *oPtr) {
    if (oPtr->refCount-- <= 1) {
	if (oPtr->classPtr != NULL) {
	    ckfree(oPtr->classPtr);
	}
	ckfree(oPtr);
	return 1;
    }
    return 0;
}

/* setting the "empty" location to NULL makes debugging a little easier */
#define REMOVEBODY { \
    for (; idx < num - 1; idx++) { \
	list[idx] = list[idx+1]; \
    } \
    list[idx] = NULL;  \
    return; \
}
void RemoveClass(Class **list, int num, int idx) REMOVEBODY

void RemoveObject(Object **list, int num, int idx) REMOVEBODY

/*
 * ----------------------------------------------------------------------
 *
 * TclOORemoveFromInstances --
 *
 *	Utility function to remove an object from the list of instances within
 *	a class.
 *
 * ----------------------------------------------------------------------
 */

int
TclOORemoveFromInstances(
    Object *oPtr,		/* The instance to remove. */
    Class *clsPtr)		/* The class (possibly) containing the
				 * reference to the instance. */
{
    int i, res = 0;
    Object *instPtr;

    FOREACH(instPtr, clsPtr->instances) {
	if (oPtr == instPtr) {
	    RemoveItem(Object, clsPtr->instances, i);
	    TclOODecrRefCount(oPtr);
	    res++;
	    break;
	}
    }
    return res;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOAddToInstances --
 *
 *	Utility function to add an object to the list of instances within a
 *	class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOAddToInstances(
    Object *oPtr,		/* The instance to add. */
    Class *clsPtr)		/* The class to add the instance to. It is
				 * assumed that the class is not already
				 * present as an instance in the class. */
{
    if (clsPtr->instances.num >= clsPtr->instances.size) {
	clsPtr->instances.size += ALLOC_CHUNK;
	if (clsPtr->instances.size == ALLOC_CHUNK) {
	    clsPtr->instances.list = ckalloc(sizeof(Object *) * ALLOC_CHUNK);
	} else {
	    clsPtr->instances.list = ckrealloc(clsPtr->instances.list,
		    sizeof(Object *) * clsPtr->instances.size);
	}
    }
    clsPtr->instances.list[clsPtr->instances.num++] = oPtr;
    AddRef(oPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOORemoveFromMixins --
 *
 *	Utility function to remove a class from the list of mixins within an
 *	object.
 *
 * ----------------------------------------------------------------------
 */

int
TclOORemoveFromMixins(
    Class *mixinPtr,		/* The mixin to remove. */
    Object *oPtr)		/* The object (possibly) containing the
				 * reference to the mixin. */
{
    int i, res = 0;
    Class *mixPtr;

    FOREACH(mixPtr, oPtr->mixins) {
	if (mixinPtr == mixPtr) {
	    RemoveItem(Class, oPtr->mixins, i);
	    TclOODecrRefCount(mixPtr->thisPtr);
	    res++;
	    break;
	}
    }
    if (oPtr->mixins.num == 0) {
	ckfree(oPtr->mixins.list);
	oPtr->mixins.list = NULL;
    }
    return res;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOORemoveFromSubclasses --
 *
 *	Utility function to remove a class from the list of subclasses within
 *	another class. Returns the number of removals performed.
 *
 * ----------------------------------------------------------------------
 */

int
TclOORemoveFromSubclasses(
    Class *subPtr,		/* The subclass to remove. */
    Class *superPtr)		/* The superclass to possibly remove the
				 * subclass reference from. */
{
    int i, res = 0;
    Class *subclsPtr;

    FOREACH(subclsPtr, superPtr->subclasses) {
	if (subPtr == subclsPtr) {
	    RemoveItem(Class, superPtr->subclasses, i);
	    TclOODecrRefCount(subPtr->thisPtr);
	    res++;
	}
    }
    return res;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOAddToSubclasses --
 *
 *	Utility function to add a class to the list of subclasses within
 *	another class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOAddToSubclasses(
    Class *subPtr,		/* The subclass to add. */
    Class *superPtr)		/* The superclass to add the subclass to. It
				 * is assumed that the class is not already
				 * present as a subclass in the superclass. */
{
    if (Deleted(superPtr->thisPtr)) {
	return;
    }
    if (superPtr->subclasses.num >= superPtr->subclasses.size) {
	superPtr->subclasses.size += ALLOC_CHUNK;
	if (superPtr->subclasses.size == ALLOC_CHUNK) {
	    superPtr->subclasses.list = ckalloc(sizeof(Class *) * ALLOC_CHUNK);
	} else {
	    superPtr->subclasses.list = ckrealloc(superPtr->subclasses.list,
		    sizeof(Class *) * superPtr->subclasses.size);
	}
    }
    superPtr->subclasses.list[superPtr->subclasses.num++] = subPtr;
    AddRef(subPtr->thisPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOORemoveFromMixinSubs --
 *
 *	Utility function to remove a class from the list of mixinSubs within
 *	another class.
 *
 * ----------------------------------------------------------------------
 */

int
TclOORemoveFromMixinSubs(
    Class *subPtr,		/* The subclass to remove. */
    Class *superPtr)		/* The superclass to possibly remove the
				 * subclass reference from. */
{
    int i, res = 0;
    Class *subclsPtr;

    FOREACH(subclsPtr, superPtr->mixinSubs) {
	if (subPtr == subclsPtr) {
	    RemoveItem(Class, superPtr->mixinSubs, i);
	    TclOODecrRefCount(subPtr->thisPtr);
	    res++;
	    break;
	}
    }
    return res;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOAddToMixinSubs --
 *
 *	Utility function to add a class to the list of mixinSubs within
 *	another class.
 *
 * ----------------------------------------------------------------------
 */

void
TclOOAddToMixinSubs(
    Class *subPtr,		/* The subclass to add. */
    Class *superPtr)		/* The superclass to add the subclass to. It
				 * is assumed that the class is not already
				 * present as a subclass in the superclass. */
{
    if (Deleted(superPtr->thisPtr)) {
	return;
    }
    if (superPtr->mixinSubs.num >= superPtr->mixinSubs.size) {
	superPtr->mixinSubs.size += ALLOC_CHUNK;
	if (superPtr->mixinSubs.size == ALLOC_CHUNK) {
	    superPtr->mixinSubs.list = ckalloc(sizeof(Class *) * ALLOC_CHUNK);
	} else {
	    superPtr->mixinSubs.list = ckrealloc(superPtr->mixinSubs.list,
		    sizeof(Class *) * superPtr->mixinSubs.size);
	}
    }
    superPtr->mixinSubs.list[superPtr->mixinSubs.num++] = subPtr;
    AddRef(subPtr->thisPtr);
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOAllocClass --
 *
 *	Allocate a basic class. Does not add class to its class's instance
 *	list.
 *
 * ----------------------------------------------------------------------
 */

Class *
TclOOAllocClass(
    Tcl_Interp *interp,		/* Interpreter within which to allocate the
				 * class. */
    Object *useThisObj)		/* Object that is to act as the class
				 * representation. */
{
    Foundation *fPtr = GetFoundation(interp);
    Class *clsPtr = ckalloc(sizeof(Class));

    memset(clsPtr, 0, sizeof(Class));
    clsPtr->thisPtr = useThisObj;

    /*
     * Configure the namespace path for the class's object.
     */
    initClassPath(interp, clsPtr);

    /*
     * Classes are subclasses of oo::object, i.e. the objects they create are
     * objects.
     */

    clsPtr->superclasses.num = 1;
    clsPtr->superclasses.list = ckalloc(sizeof(Class *));
    clsPtr->superclasses.list[0] = fPtr->objectCls;
    AddRef(fPtr->objectCls->thisPtr);

    /*
     * Finish connecting the class structure to the object structure.
     */

    clsPtr->thisPtr->classPtr = clsPtr;

    /*
     * That's the complicated bit. Now fill in the rest of the non-zero/NULL
     * fields.
     */

    Tcl_InitObjHashTable(&clsPtr->classMethods);
    return clsPtr;
}
static void
initClassPath(Tcl_Interp *interp, Class *clsPtr) {
    Foundation *fPtr = GetFoundation(interp);
    if (fPtr->helpersNs != NULL) {
	Tcl_Namespace *path[2];
	path[0] = fPtr->helpersNs;
	path[1] = fPtr->ooNs;
	TclSetNsPath((Namespace *) clsPtr->thisPtr->namespacePtr, 2, path);
    } else {
	TclSetNsPath((Namespace *) clsPtr->thisPtr->namespacePtr, 1,
		&fPtr->ooNs);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_NewObjectInstance --
 *
 *	Allocate a new instance of an object.
 *
 * ----------------------------------------------------------------------
 */
Tcl_Object
Tcl_NewObjectInstance(
    Tcl_Interp *interp,		/* Interpreter context. */
    Tcl_Class cls,		/* Class to create an instance of. */
    const char *nameStr,	/* Name of object to create, or NULL to ask
				 * the code to pick its own unique name. */
    const char *nsNameStr,	/* Name of namespace to create inside object,
				 * or NULL to ask the code to pick its own
				 * unique name. */
    int objc,			/* Number of arguments. Negative value means
				 * do not call constructor. */
    Tcl_Obj *const *objv,	/* Argument list. */
    int skip)			/* Number of arguments to _not_ pass to the
				 * constructor. */
{
    register Class *classPtr = (Class *) cls;
    Object *oPtr;
    ClientData clientData[4];

    oPtr = TclNewObjectInstanceCommon(interp, classPtr, nameStr, nsNameStr);
    if (oPtr == NULL) {return NULL;}

    /*
     * Run constructors, except when objc < 0, which is a special flag case
     * used for object cloning only.
     */

    if (objc >= 0) {
	CallContext *contextPtr =
		TclOOGetCallContext(oPtr, NULL, CONSTRUCTOR, NULL);

	if (contextPtr != NULL) {
	    int isRoot, result;
	    Tcl_InterpState state;

	    state = Tcl_SaveInterpState(interp, TCL_OK);
	    contextPtr->callPtr->flags |= CONSTRUCTOR;
	    contextPtr->skip = skip;

	    /*
	     * Adjust the ensemble tracking record if necessary. [Bug 3514761]
	     */

	    isRoot = TclInitRewriteEnsemble(interp, skip, skip, objv);
	    result = Tcl_NRCallObjProc(interp, TclOOInvokeContext, contextPtr,
		    objc, objv);

	    if (isRoot) {
		TclResetRewriteEnsemble(interp, 1);
	    }

	    clientData[0] = contextPtr;
	    clientData[1] = oPtr;
	    clientData[2] = state;
	    clientData[3] = &oPtr;

	    result = FinalizeAlloc(clientData, interp, result);
	    if (result != TCL_OK) {
		return NULL;
	    }
	}
    }

    return (Tcl_Object) oPtr;
}

int
TclNRNewObjectInstance(
    Tcl_Interp *interp,		/* Interpreter context. */
    Tcl_Class cls,		/* Class to create an instance of. */
    const char *nameStr,	/* Name of object to create, or NULL to ask
				 * the code to pick its own unique name. */
    const char *nsNameStr,	/* Name of namespace to create inside object,
				 * or NULL to ask the code to pick its own
				 * unique name. */
    int objc,			/* Number of arguments. Negative value means
				 * do not call constructor. */
    Tcl_Obj *const *objv,	/* Argument list. */
    int skip,			/* Number of arguments to _not_ pass to the
				 * constructor. */
    Tcl_Object *objectPtr)	/* Place to write the object reference upon
				 * successful allocation. */
{
    register Class *classPtr = (Class *) cls;
    CallContext *contextPtr;
    Tcl_InterpState state;
    Object *oPtr;

    oPtr = TclNewObjectInstanceCommon(interp, classPtr, nameStr, nsNameStr);
    if (oPtr == NULL) {return TCL_ERROR;}

    /*
     * Run constructors, except when objc < 0 (a special flag case used for
     * object cloning only). If there aren't any constructors, we do nothing.
     */

    if (objc < 0) {
	*objectPtr = (Tcl_Object) oPtr;
	return TCL_OK;
    }
    contextPtr = TclOOGetCallContext(oPtr, NULL, CONSTRUCTOR, NULL);
    if (contextPtr == NULL) {
	*objectPtr = (Tcl_Object) oPtr;
	return TCL_OK;
    }

    state = Tcl_SaveInterpState(interp, TCL_OK);
    contextPtr->callPtr->flags |= CONSTRUCTOR;
    contextPtr->skip = skip;

    /*
     * Adjust the ensemble tracking record if necessary. [Bug 3514761]
     */

    if (TclInitRewriteEnsemble(interp, skip, skip, objv)) {
	TclNRAddCallback(interp, TclClearRootEnsemble, NULL, NULL, NULL, NULL);
    }

    /*
     * Fire off the constructors non-recursively.
     */

    TclNRAddCallback(interp, FinalizeAlloc, contextPtr, oPtr, state,
	    objectPtr);
    TclPushTailcallPoint(interp);
    return TclOOInvokeContext(contextPtr, interp, objc, objv);
}


Object *
TclNewObjectInstanceCommon(
    Tcl_Interp *interp,
    Class *classPtr,
    const char *nameStr,
    const char *nsNameStr)
{
    Tcl_HashEntry *hPtr;
    Foundation *fPtr = GetFoundation(interp);
    Object *oPtr;
    const char *simpleName = NULL;
    Namespace *nsPtr = NULL, *dummy,
	*inNsPtr = (Namespace *)TclGetCurrentNamespace(interp);
    int isNew;

    if (nameStr) {
	TclGetNamespaceForQualName(interp, nameStr, inNsPtr, TCL_CREATE_NS_IF_UNKNOWN,
		&nsPtr, &dummy, &dummy, &simpleName);

	/*
	 * Disallow creation of an object over an existing command.
	 */

	hPtr = Tcl_CreateHashEntry(&nsPtr->cmdTable, simpleName, &isNew);
	if (isNew) {
	    /* Just kidding */
	    Tcl_DeleteHashEntry(hPtr);
	} else {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "can't create object \"%s\": command already exists with"
		    " that name", nameStr));
	    Tcl_SetErrorCode(interp, "TCL", "OO", "OVERWRITE_OBJECT", NULL);
	    return NULL;
	}
    }

    /*
     * Create the object.
     */

    oPtr = AllocObject(interp, simpleName, nsPtr, nsNameStr);
    oPtr->selfCls = classPtr;
    AddRef(classPtr->thisPtr);
    TclOOAddToInstances(oPtr, classPtr);
    /*
     * Check to see if we're really creating a class. If so, allocate the
     * class structure as well.
     */

    if (TclOOIsReachable(fPtr->classCls, classPtr)) {
	/*
	 * Is a class, so attach a class structure. Note that the
	 * TclOOAllocClass function splices the structure into the object, so
	 * we don't have to. Once that's done, we need to repatch the object
	 * to have the right class since TclOOAllocClass interferes with that.
	 */

	TclOOAllocClass(interp, oPtr);
	TclOOAddToSubclasses(oPtr->classPtr, fPtr->objectCls);
    } else {
	oPtr->classPtr = NULL;
    }
    return oPtr;
}



static int
FinalizeAlloc(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CallContext *contextPtr = data[0];
    Object *oPtr = data[1];
    Tcl_InterpState state = data[2];
    Tcl_Object *objectPtr = data[3];

    /*
     * Ensure an error if the object was deleted in the constructor.
     * Don't want to lose errors by accident. [Bug 2903011]
     */

    if (result != TCL_ERROR && Deleted(oPtr)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"object deleted in constructor", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "STILLBORN", NULL);
	result = TCL_ERROR;
    }
    if (result != TCL_OK) {
	Tcl_DiscardInterpState(state);

	/*
	 * Take care to not delete a deleted object; that would be bad. [Bug
	 * 2903011] Also take care to make sure that we have the name of the
	 * command before we delete it. [Bug 9dd1bd7a74]
	 */

	if (!Deleted(oPtr)) {
	    (void) TclOOObjectName(interp, oPtr);
	    Tcl_DeleteCommandFromToken(interp, oPtr->command);
	}
	/* This decrements the refcount of oPtr */
	TclOODeleteContext(contextPtr);
	return TCL_ERROR;
    }
    Tcl_RestoreInterpState(interp, state);
    *objectPtr = (Tcl_Object) oPtr;
    /* This decrements the refcount of oPtr */
    TclOODeleteContext(contextPtr);
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_CopyObjectInstance --
 *
 *	Creates a copy of an object. Does not copy the backing namespace,
 *	since the correct way to do that (e.g., shallow/deep) depends on the
 *	object/class's own policies.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Object
Tcl_CopyObjectInstance(
    Tcl_Interp *interp,
    Tcl_Object sourceObject,
    const char *targetName,
    const char *targetNamespaceName)
{
    Object *oPtr = (Object *) sourceObject, *o2Ptr;
    FOREACH_HASH_DECLS;
    Method *mPtr;
    Class *mixinPtr;
    CallContext *contextPtr;
    Tcl_Obj *keyPtr, *filterObj, *variableObj, *args[3];
    int i, result;

    /*
     * Sanity check.
     */

    if (IsRootClass(oPtr)) {
	Tcl_SetObjResult(interp, Tcl_NewStringObj(
		"may not clone the class of classes", -1));
	Tcl_SetErrorCode(interp, "TCL", "OO", "CLONING_CLASS", NULL);
	return NULL;
    }

    /*
     * Build the instance. Note that this does not run any constructors.
     */

    o2Ptr = (Object *) Tcl_NewObjectInstance(interp,
	    (Tcl_Class) oPtr->selfCls, targetName, targetNamespaceName, -1,
	    NULL, -1);
    if (o2Ptr == NULL) {
	return NULL;
    }

    /*
     * Copy the object-local methods to the new object.
     */

    if (oPtr->methodsPtr) {
	FOREACH_HASH(keyPtr, mPtr, oPtr->methodsPtr) {
	    if (CloneObjectMethod(interp, o2Ptr, mPtr, keyPtr) != TCL_OK) {
		Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
		return NULL;
	    }
	}
    }

    /*
     * Copy the object's mixin references to the new object.
     */

    if (o2Ptr->mixins.num != 0) {
	FOREACH(mixinPtr, o2Ptr->mixins) {
	    if (mixinPtr && mixinPtr != o2Ptr->selfCls) {
		TclOORemoveFromInstances(o2Ptr, mixinPtr);
	    }
	    TclOODecrRefCount(mixinPtr->thisPtr);
	}
	ckfree(o2Ptr->mixins.list);
    }
    DUPLICATE(o2Ptr->mixins, oPtr->mixins, Class *);
    FOREACH(mixinPtr, o2Ptr->mixins) {
	if (mixinPtr && mixinPtr != o2Ptr->selfCls) {
	    TclOOAddToInstances(o2Ptr, mixinPtr);
	}
	/* For the reference just created in DUPLICATE */
	AddRef(mixinPtr->thisPtr);
    }

    /*
     * Copy the object's filter list to the new object.
     */

    DUPLICATE(o2Ptr->filters, oPtr->filters, Tcl_Obj *);
    FOREACH(filterObj, o2Ptr->filters) {
	Tcl_IncrRefCount(filterObj);
    }

    /*
     * Copy the object's variable resolution list to the new object.
     */

    DUPLICATE(o2Ptr->variables, oPtr->variables, Tcl_Obj *);
    FOREACH(variableObj, o2Ptr->variables) {
	Tcl_IncrRefCount(variableObj);
    }

    /*
     * Copy the object's flags to the new object, clearing those that must be
     * kept object-local. The duplicate is never deleted at this point, nor is
     * it the root of the object system or in the midst of processing a filter
     * call.
     */

    o2Ptr->flags = oPtr->flags & ~(
	    OBJECT_DELETED | ROOT_OBJECT | ROOT_CLASS | FILTER_HANDLING); 
    /*
     * Copy the object's metadata.
     */

    if (oPtr->metadataPtr != NULL) {
	Tcl_ObjectMetadataType *metadataTypePtr;
	ClientData value, duplicate;

	FOREACH_HASH(metadataTypePtr, value, oPtr->metadataPtr) {
	    if (metadataTypePtr->cloneProc == NULL) {
		duplicate = value;
	    } else {
		if (metadataTypePtr->cloneProc(interp, value,
			&duplicate) != TCL_OK) {
		    Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
		    return NULL;
		}
	    }
	    if (duplicate != NULL) {
		Tcl_ObjectSetMetadata((Tcl_Object) o2Ptr, metadataTypePtr,
			duplicate);
	    }
	}
    }

    /*
     * Copy the class, if present. Note that if there is a class present in
     * the source object, there must also be one in the copy.
     */

    if (oPtr->classPtr != NULL) {
	Class *clsPtr = oPtr->classPtr;
	Class *cls2Ptr = o2Ptr->classPtr;
	Class *superPtr;

	/*
	 * Copy the class flags across.
	 */

	cls2Ptr->flags = clsPtr->flags;

	/*
	 * Ensure that the new class's superclass structure is the same as the
	 * old class's.
	 */

	FOREACH(superPtr, cls2Ptr->superclasses) {
	    TclOORemoveFromSubclasses(cls2Ptr, superPtr);
	    TclOODecrRefCount(superPtr->thisPtr);
	}
	if (cls2Ptr->superclasses.num) {
	    cls2Ptr->superclasses.list = ckrealloc(cls2Ptr->superclasses.list,
		    sizeof(Class *) * clsPtr->superclasses.num);
	} else {
	    cls2Ptr->superclasses.list =
		    ckalloc(sizeof(Class *) * clsPtr->superclasses.num);
	}
	memcpy(cls2Ptr->superclasses.list, clsPtr->superclasses.list,
		sizeof(Class *) * clsPtr->superclasses.num);
	cls2Ptr->superclasses.num = clsPtr->superclasses.num;
	FOREACH(superPtr, cls2Ptr->superclasses) {
	    TclOOAddToSubclasses(cls2Ptr, superPtr);

	    /* For the new item in cls2Ptr->superclasses that memcpy just
	     * created
	     */
	    AddRef(superPtr->thisPtr);
	}

	/*
	 * Duplicate the source class's filters.
	 */

	DUPLICATE(cls2Ptr->filters, clsPtr->filters, Tcl_Obj *);
	FOREACH(filterObj, cls2Ptr->filters) {
	    Tcl_IncrRefCount(filterObj);
	}

	/*
	 * Copy the source class's variable resolution list.
	 */

	DUPLICATE(cls2Ptr->variables, clsPtr->variables, Tcl_Obj *);
	FOREACH(variableObj, cls2Ptr->variables) {
	    Tcl_IncrRefCount(variableObj);
	}

	/*
	 * Duplicate the source class's mixins (which cannot be circular
	 * references to the duplicate).
	 */

	if (cls2Ptr->mixins.num != 0) {
	    FOREACH(mixinPtr, cls2Ptr->mixins) {
		TclOORemoveFromMixinSubs(cls2Ptr, mixinPtr);
		TclOODecrRefCount(mixinPtr->thisPtr);
	    }
	    ckfree(clsPtr->mixins.list);
	}
	DUPLICATE(cls2Ptr->mixins, clsPtr->mixins, Class *);
	FOREACH(mixinPtr, cls2Ptr->mixins) {
	    TclOOAddToMixinSubs(cls2Ptr, mixinPtr);
	    /* For the copy just created in DUPLICATE */
	    AddRef(mixinPtr->thisPtr);
	}

	/*
	 * Duplicate the source class's methods, constructor and destructor.
	 */

	FOREACH_HASH(keyPtr, mPtr, &clsPtr->classMethods) {
	    if (CloneClassMethod(interp, cls2Ptr, mPtr, keyPtr,
		    NULL) != TCL_OK) {
		Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
		return NULL;
	    }
	}
	if (clsPtr->constructorPtr) {
	    if (CloneClassMethod(interp, cls2Ptr, clsPtr->constructorPtr,
		    NULL, &cls2Ptr->constructorPtr) != TCL_OK) {
		Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
		return NULL;
	    }
	}
	if (clsPtr->destructorPtr) {
	    if (CloneClassMethod(interp, cls2Ptr, clsPtr->destructorPtr, NULL,
		    &cls2Ptr->destructorPtr) != TCL_OK) {
		Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
		return NULL;
	    }
	}

	/*
	 * Duplicate the class's metadata.
	 */

	if (clsPtr->metadataPtr != NULL) {
	    Tcl_ObjectMetadataType *metadataTypePtr;
	    ClientData value, duplicate;

	    FOREACH_HASH(metadataTypePtr, value, clsPtr->metadataPtr) {
		if (metadataTypePtr->cloneProc == NULL) {
		    duplicate = value;
		} else {
		    if (metadataTypePtr->cloneProc(interp, value,
			    &duplicate) != TCL_OK) {
			Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
			return NULL;
		    }
		}
		if (duplicate != NULL) {
		    Tcl_ClassSetMetadata((Tcl_Class) cls2Ptr, metadataTypePtr,
			    duplicate);
		}
	    }
	}
    }

    TclResetRewriteEnsemble(interp, 1);
    contextPtr = TclOOGetCallContext(o2Ptr, oPtr->fPtr->clonedName, 0, NULL);
    if (contextPtr) {
	args[0] = TclOOObjectName(interp, o2Ptr);
	args[1] = oPtr->fPtr->clonedName;
	args[2] = TclOOObjectName(interp, oPtr);
	Tcl_IncrRefCount(args[0]);
	Tcl_IncrRefCount(args[1]);
	Tcl_IncrRefCount(args[2]);
	result = Tcl_NRCallObjProc(interp, TclOOInvokeContext, contextPtr, 3,
		args);
	TclDecrRefCount(args[0]);
	TclDecrRefCount(args[1]);
	TclDecrRefCount(args[2]);
	TclOODeleteContext(contextPtr);
	if (result == TCL_ERROR) {
	    Tcl_AddErrorInfo(interp,
		    "\n    (while performing post-copy callback)");
	}
	if (result != TCL_OK) {
	    Tcl_DeleteCommandFromToken(interp, o2Ptr->command);
	    return NULL;
	}
    }

    return (Tcl_Object) o2Ptr;
}

/*
 * ----------------------------------------------------------------------
 *
 * CloneObjectMethod, CloneClassMethod --
 *
 *	Helper functions used for cloning methods. They work identically to
 *	each other, except for the difference between them in how they
 *	register the cloned method on a successful clone.
 *
 * ----------------------------------------------------------------------
 */

static int
CloneObjectMethod(
    Tcl_Interp *interp,
    Object *oPtr,
    Method *mPtr,
    Tcl_Obj *namePtr)
{
    if (mPtr->typePtr == NULL) {
	Tcl_NewInstanceMethod(interp, (Tcl_Object) oPtr, namePtr,
		mPtr->flags & PUBLIC_METHOD, NULL, NULL);
    } else if (mPtr->typePtr->cloneProc) {
	ClientData newClientData;

	if (mPtr->typePtr->cloneProc(interp, mPtr->clientData,
		&newClientData) != TCL_OK) {
	    return TCL_ERROR;
	}
	Tcl_NewInstanceMethod(interp, (Tcl_Object) oPtr, namePtr,
		mPtr->flags & PUBLIC_METHOD, mPtr->typePtr, newClientData);
    } else {
	Tcl_NewInstanceMethod(interp, (Tcl_Object) oPtr, namePtr,
		mPtr->flags & PUBLIC_METHOD, mPtr->typePtr, mPtr->clientData);
    }
    return TCL_OK;
}

static int
CloneClassMethod(
    Tcl_Interp *interp,
    Class *clsPtr,
    Method *mPtr,
    Tcl_Obj *namePtr,
    Method **m2PtrPtr)
{
    Method *m2Ptr;

    if (mPtr->typePtr == NULL) {
	m2Ptr = (Method *) Tcl_NewMethod(interp, (Tcl_Class) clsPtr,
		namePtr, mPtr->flags & PUBLIC_METHOD, NULL, NULL);
    } else if (mPtr->typePtr->cloneProc) {
	ClientData newClientData;

	if (mPtr->typePtr->cloneProc(interp, mPtr->clientData,
		&newClientData) != TCL_OK) {
	    return TCL_ERROR;
	}
	m2Ptr = (Method *) Tcl_NewMethod(interp, (Tcl_Class) clsPtr,
		namePtr, mPtr->flags & PUBLIC_METHOD, mPtr->typePtr,
		newClientData);
    } else {
	m2Ptr = (Method *) Tcl_NewMethod(interp, (Tcl_Class) clsPtr,
		namePtr, mPtr->flags & PUBLIC_METHOD, mPtr->typePtr,
		mPtr->clientData);
    }
    if (m2PtrPtr != NULL) {
	*m2PtrPtr = m2Ptr;
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_ClassGetMetadata, Tcl_ClassSetMetadata, Tcl_ObjectGetMetadata,
 * Tcl_ObjectSetMetadata --
 *
 *	Metadata management API. The metadata system allows code in extensions
 *	to attach arbitrary non-NULL pointers to objects and classes without
 *	the different things that might be interested being able to interfere
 *	with each other. Apart from non-NULL-ness, these routines attach no
 *	interpretation to the meaning of the metadata pointers.
 *
 *	The Tcl_*GetMetadata routines get the metadata pointer attached that
 *	has been related with a particular type, or NULL if no metadata
 *	associated with the given type has been attached.
 *
 *	The Tcl_*SetMetadata routines set or delete the metadata pointer that
 *	is related to a particular type. The value associated with the type is
 *	deleted (if present; no-op otherwise) if the value is NULL, and
 *	attached (replacing the previous value, which is deleted if present)
 *	otherwise. This means it is impossible to attach a NULL value for any
 *	metadata type.
 *
 * ----------------------------------------------------------------------
 */

ClientData
Tcl_ClassGetMetadata(
    Tcl_Class clazz,
    const Tcl_ObjectMetadataType *typePtr)
{
    Class *clsPtr = (Class *) clazz;
    Tcl_HashEntry *hPtr;

    /*
     * If there's no metadata store attached, the type in question has
     * definitely not been attached either!
     */

    if (clsPtr->metadataPtr == NULL) {
	return NULL;
    }

    /*
     * There is a metadata store, so look in it for the given type.
     */

    hPtr = Tcl_FindHashEntry(clsPtr->metadataPtr, (char *) typePtr);

    /*
     * Return the metadata value if we found it, otherwise NULL.
     */

    if (hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

void
Tcl_ClassSetMetadata(
    Tcl_Class clazz,
    const Tcl_ObjectMetadataType *typePtr,
    ClientData metadata)
{
    Class *clsPtr = (Class *) clazz;
    Tcl_HashEntry *hPtr;
    int isNew;

    /*
     * Attach the metadata store if not done already.
     */

    if (clsPtr->metadataPtr == NULL) {
	if (metadata == NULL) {
	    return;
	}
	clsPtr->metadataPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(clsPtr->metadataPtr, TCL_ONE_WORD_KEYS);
    }

    /*
     * If the metadata is NULL, we're deleting the metadata for the type.
     */

    if (metadata == NULL) {
	hPtr = Tcl_FindHashEntry(clsPtr->metadataPtr, (char *) typePtr);
	if (hPtr != NULL) {
	    typePtr->deleteProc(Tcl_GetHashValue(hPtr));
	    Tcl_DeleteHashEntry(hPtr);
	}
	return;
    }

    /*
     * Otherwise we're attaching the metadata. Note that if there was already
     * some metadata attached of this type, we delete that first.
     */

    hPtr = Tcl_CreateHashEntry(clsPtr->metadataPtr, (char *) typePtr, &isNew);
    if (!isNew) {
	typePtr->deleteProc(Tcl_GetHashValue(hPtr));
    }
    Tcl_SetHashValue(hPtr, metadata);
}

ClientData
Tcl_ObjectGetMetadata(
    Tcl_Object object,
    const Tcl_ObjectMetadataType *typePtr)
{
    Object *oPtr = (Object *) object;
    Tcl_HashEntry *hPtr;

    /*
     * If there's no metadata store attached, the type in question has
     * definitely not been attached either!
     */

    if (oPtr->metadataPtr == NULL) {
	return NULL;
    }

    /*
     * There is a metadata store, so look in it for the given type.
     */

    hPtr = Tcl_FindHashEntry(oPtr->metadataPtr, (char *) typePtr);

    /*
     * Return the metadata value if we found it, otherwise NULL.
     */

    if (hPtr == NULL) {
	return NULL;
    }
    return Tcl_GetHashValue(hPtr);
}

void
Tcl_ObjectSetMetadata(
    Tcl_Object object,
    const Tcl_ObjectMetadataType *typePtr,
    ClientData metadata)
{
    Object *oPtr = (Object *) object;
    Tcl_HashEntry *hPtr;
    int isNew;

    /*
     * Attach the metadata store if not done already.
     */

    if (oPtr->metadataPtr == NULL) {
	if (metadata == NULL) {
	    return;
	}
	oPtr->metadataPtr = ckalloc(sizeof(Tcl_HashTable));
	Tcl_InitHashTable(oPtr->metadataPtr, TCL_ONE_WORD_KEYS);
    }

    /*
     * If the metadata is NULL, we're deleting the metadata for the type.
     */

    if (metadata == NULL) {
	hPtr = Tcl_FindHashEntry(oPtr->metadataPtr, (char *) typePtr);
	if (hPtr != NULL) {
	    typePtr->deleteProc(Tcl_GetHashValue(hPtr));
	    Tcl_DeleteHashEntry(hPtr);
	}
	return;
    }

    /*
     * Otherwise we're attaching the metadata. Note that if there was already
     * some metadata attached of this type, we delete that first.
     */

    hPtr = Tcl_CreateHashEntry(oPtr->metadataPtr, (char *) typePtr, &isNew);
    if (!isNew) {
	typePtr->deleteProc(Tcl_GetHashValue(hPtr));
    }
    Tcl_SetHashValue(hPtr, metadata);
}

/*
 * ----------------------------------------------------------------------
 *
 * PublicObjectCmd, PrivateObjectCmd, TclOOInvokeObject --
 *
 *	Main entry point for object invocations. The Public* and Private*
 *	wrapper functions (implementations of both object instance commands
 *	and [my]) are just thin wrappers round the main TclOOObjectCmdCore
 *	function. Note that the core is function is NRE-aware.
 *
 * ----------------------------------------------------------------------
 */

static int
PublicObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return Tcl_NRCallObjProc(interp, PublicNRObjectCmd, clientData,objc,objv);
}

static int
PublicNRObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return TclOOObjectCmdCore(clientData, interp, objc, objv, PUBLIC_METHOD,
	    NULL);
}

static int
PrivateObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return Tcl_NRCallObjProc(interp, PrivateNRObjectCmd,clientData,objc,objv);
}

static int
PrivateNRObjectCmd(
    ClientData clientData,
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const *objv)
{
    return TclOOObjectCmdCore(clientData, interp, objc, objv, 0, NULL);
}

int
TclOOInvokeObject(
    Tcl_Interp *interp,		/* Interpreter for commands, variables,
				 * results, error reporting, etc. */
    Tcl_Object object,		/* The object to invoke. */
    Tcl_Class startCls,		/* Where in the class chain to start the
				 * invoke from, or NULL to traverse the whole
				 * chain including filters. */
    int publicPrivate,		/* Whether this is an invoke from a public
				 * context (PUBLIC_METHOD), a private context
				 * (PRIVATE_METHOD), or a *really* private
				 * context (any other value; conventionally
				 * 0). */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const *objv)	/* Array of argument objects. It is assumed
				 * that the name of the method to invoke will
				 * be at index 1. */
{
    switch (publicPrivate) {
    case PUBLIC_METHOD:
	return TclOOObjectCmdCore((Object *) object, interp, objc, objv,
		PUBLIC_METHOD, (Class *) startCls);
    case PRIVATE_METHOD:
	return TclOOObjectCmdCore((Object *) object, interp, objc, objv,
		PRIVATE_METHOD, (Class *) startCls);
    default:
	return TclOOObjectCmdCore((Object *) object, interp, objc, objv, 0,
		(Class *) startCls);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOObjectCmdCore, FinalizeObjectCall --
 *
 *	Main function for object invocations. Does call chain creation,
 *	management and invocation. The function FinalizeObjectCall exists to
 *	clean up after the non-recursive processing of TclOOObjectCmdCore.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOObjectCmdCore(
    Object *oPtr,		/* The object being invoked. */
    Tcl_Interp *interp,		/* The interpreter containing the object. */
    int objc,			/* How many arguments are being passed in. */
    Tcl_Obj *const *objv,	/* The array of arguments. */
    int flags,			/* Whether this is an invocation through the
				 * public or the private command interface. */
    Class *startCls)		/* Where to start in the call chain, or NULL
				 * if we are to start at the front with
				 * filters and the object's methods (which is
				 * the normal case). */
{
    CallContext *contextPtr;
    Tcl_Obj *methodNamePtr;
    int result;

    /*
     * If we've no method name, throw this directly into the unknown
     * processing.
     */

    if (objc < 2) {
	flags |= FORCE_UNKNOWN;
	methodNamePtr = NULL;
	goto noMapping;
    }

    /*
     * Give plugged in code a chance to remap the method name.
     */

    methodNamePtr = objv[1];
    if (oPtr->mapMethodNameProc != NULL) {
	register Class **startClsPtr = &startCls;
	Tcl_Obj *mappedMethodName = Tcl_DuplicateObj(methodNamePtr);

	result = oPtr->mapMethodNameProc(interp, (Tcl_Object) oPtr,
		(Tcl_Class *) startClsPtr, mappedMethodName);
	if (result != TCL_OK) {
	    TclDecrRefCount(mappedMethodName);
	    if (result == TCL_BREAK) {
		goto noMapping;
	    } else if (result == TCL_ERROR) {
		Tcl_AddErrorInfo(interp, "\n    (while mapping method name)");
	    }
	    return result;
	}

	/*
	 * Get the call chain for the remapped name.
	 */

	Tcl_IncrRefCount(mappedMethodName);
	contextPtr = TclOOGetCallContext(oPtr, mappedMethodName,
		flags | (oPtr->flags & FILTER_HANDLING), methodNamePtr);
	TclDecrRefCount(mappedMethodName);
	if (contextPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "impossible to invoke method \"%s\": no defined method or"
		    " unknown method", TclGetString(methodNamePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD_MAPPED",
		    TclGetString(methodNamePtr), NULL);
	    return TCL_ERROR;
	}
    } else {
	/*
	 * Get the call chain.
	 */

    noMapping:
	contextPtr = TclOOGetCallContext(oPtr, methodNamePtr,
		flags | (oPtr->flags & FILTER_HANDLING), NULL);
	if (contextPtr == NULL) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "impossible to invoke method \"%s\": no defined method or"
		    " unknown method", TclGetString(methodNamePtr)));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		    TclGetString(methodNamePtr), NULL);
	    return TCL_ERROR;
	}
    }

    /*
     * Check to see if we need to apply magical tricks to start part way
     * through the call chain.
     */

    if (startCls != NULL) {
	for (; contextPtr->index < contextPtr->callPtr->numChain;
		contextPtr->index++) {
	    register struct MInvoke *miPtr =
		    &contextPtr->callPtr->chain[contextPtr->index];

	    if (miPtr->isFilter) {
		continue;
	    }
	    if (miPtr->mPtr->declaringClassPtr == startCls) {
		break;
	    }
	}
	if (contextPtr->index >= contextPtr->callPtr->numChain) {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "no valid method implementation", -1));
	    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "METHOD",
		    TclGetString(methodNamePtr), NULL);
	    TclOODeleteContext(contextPtr);
	    return TCL_ERROR;
	}
    }

    /*
     * Invoke the call chain, locking the object structure against deletion
     * for the duration.
     */

    TclNRAddCallback(interp, FinalizeObjectCall, contextPtr, NULL,NULL,NULL);
    return TclOOInvokeContext(contextPtr, interp, objc, objv);
}

static int
FinalizeObjectCall(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    /*
     * Dispose of the call chain, which drops the lock on the object's
     * structure.
     */

    TclOODeleteContext(data[0]);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_ObjectContextInvokeNext, TclNRObjectContextInvokeNext, FinalizeNext --
 *
 *	Invokes the next stage of the call chain described in an object
 *	context. This is the core of the implementation of the [next] command.
 *	Does not do management of the call-frame stack. Available in public
 *	(standard API) and private (NRE-aware) forms. FinalizeNext is a
 *	private function used to clean up in the NRE case.
 *
 * ----------------------------------------------------------------------
 */

int
Tcl_ObjectContextInvokeNext(
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv,
    int skip)
{
    CallContext *contextPtr = (CallContext *) context;
    int savedIndex = contextPtr->index;
    int savedSkip = contextPtr->skip;
    int result;

    if (contextPtr->index+1 >= contextPtr->callPtr->numChain) {
	/*
	 * We're at the end of the chain; generate an error message unless the
	 * interpreter is being torn down, in which case we might be getting
	 * here because of methods/destructors doing a [next] (or equivalent)
	 * unexpectedly.
	 */

	const char *methodType;

	if (Tcl_InterpDeleted(interp)) {
	    return TCL_OK;
	}

	if (contextPtr->callPtr->flags & CONSTRUCTOR) {
	    methodType = "constructor";
	} else if (contextPtr->callPtr->flags & DESTRUCTOR) {
	    methodType = "destructor";
	} else {
	    methodType = "method";
	}

	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"no next %s implementation", methodType));
	Tcl_SetErrorCode(interp, "TCL", "OO", "NOTHING_NEXT", NULL);
	return TCL_ERROR;
    }

    /*
     * Advance to the next method implementation in the chain in the method
     * call context while we process the body. However, need to adjust the
     * argument-skip control because we're guaranteed to have a single prefix
     * arg (i.e., 'next') and not the variable amount that can happen because
     * method invocations (i.e., '$obj meth' and 'my meth'), constructors
     * (i.e., '$cls new' and '$cls create obj') and destructors (no args at
     * all) come through the same code.
     */

    contextPtr->index++;
    contextPtr->skip = skip;

    /*
     * Invoke the (advanced) method call context in the caller context.
     */

    result = Tcl_NRCallObjProc(interp, TclOOInvokeContext, contextPtr, objc,
	    objv);

    /*
     * Restore the call chain context index as we've finished the inner invoke
     * and want to operate in the outer context again.
     */

    contextPtr->index = savedIndex;
    contextPtr->skip = savedSkip;

    return result;
}

int
TclNRObjectContextInvokeNext(
    Tcl_Interp *interp,
    Tcl_ObjectContext context,
    int objc,
    Tcl_Obj *const *objv,
    int skip)
{
    register CallContext *contextPtr = (CallContext *) context;

    if (contextPtr->index+1 >= contextPtr->callPtr->numChain) {
	/*
	 * We're at the end of the chain; generate an error message unless the
	 * interpreter is being torn down, in which case we might be getting
	 * here because of methods/destructors doing a [next] (or equivalent)
	 * unexpectedly.
	 */

	const char *methodType;

	if (Tcl_InterpDeleted(interp)) {
	    return TCL_OK;
	}

	if (contextPtr->callPtr->flags & CONSTRUCTOR) {
	    methodType = "constructor";
	} else if (contextPtr->callPtr->flags & DESTRUCTOR) {
	    methodType = "destructor";
	} else {
	    methodType = "method";
	}

	Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		"no next %s implementation", methodType));
	Tcl_SetErrorCode(interp, "TCL", "OO", "NOTHING_NEXT", NULL);
	return TCL_ERROR;
    }

    /*
     * Advance to the next method implementation in the chain in the method
     * call context while we process the body. However, need to adjust the
     * argument-skip control because we're guaranteed to have a single prefix
     * arg (i.e., 'next') and not the variable amount that can happen because
     * method invocations (i.e., '$obj meth' and 'my meth'), constructors
     * (i.e., '$cls new' and '$cls create obj') and destructors (no args at
     * all) come through the same code.
     */

    TclNRAddCallback(interp, FinalizeNext, contextPtr,
	    INT2PTR(contextPtr->index), INT2PTR(contextPtr->skip), NULL);
    contextPtr->index++;
    contextPtr->skip = skip;

    /*
     * Invoke the (advanced) method call context in the caller context.
     */

    return TclOOInvokeContext(contextPtr, interp, objc, objv);
}

static int
FinalizeNext(
    ClientData data[],
    Tcl_Interp *interp,
    int result)
{
    CallContext *contextPtr = data[0];

    /*
     * Restore the call chain context index as we've finished the inner invoke
     * and want to operate in the outer context again.
     */

    contextPtr->index = PTR2INT(data[1]);
    contextPtr->skip = PTR2INT(data[2]);
    return result;
}

/*
 * ----------------------------------------------------------------------
 *
 * Tcl_GetObjectFromObj --
 *
 *	Utility function to get an object from a Tcl_Obj containing its name.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Object
Tcl_GetObjectFromObj(
    Tcl_Interp *interp,		/* Interpreter in which to locate the object.
				 * Will have an error message placed in it if
				 * the name does not refer to an object. */
    Tcl_Obj *objPtr)		/* The name of the object to look up, which is
				 * exactly the name of its public command. */
{
    Command *cmdPtr = (Command *) Tcl_GetCommandFromObj(interp, objPtr);

    if (cmdPtr == NULL) {
	goto notAnObject;
    }
    if (cmdPtr->objProc != PublicObjectCmd) {
	cmdPtr = (Command *) TclGetOriginalCommand((Tcl_Command) cmdPtr);
	if (cmdPtr == NULL || cmdPtr->objProc != PublicObjectCmd) {
	    goto notAnObject;
	}
    }
    return cmdPtr->objClientData;

  notAnObject:
    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
	    "%s does not refer to an object", TclGetString(objPtr)));
    Tcl_SetErrorCode(interp, "TCL", "LOOKUP", "OBJECT", TclGetString(objPtr),
	    NULL);
    return NULL;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOIsReachable --
 *
 *	Utility function that tests whether a class is a subclass (whether
 *	directly or indirectly) of another class.
 *
 * ----------------------------------------------------------------------
 */

int
TclOOIsReachable(
    Class *targetPtr,
    Class *startPtr)
{
    int i;
    Class *superPtr;

  tailRecurse:
    if (startPtr == targetPtr) {
	return 1;
    }
    if (startPtr->superclasses.num == 1 && startPtr->mixins.num == 0) {
	startPtr = startPtr->superclasses.list[0];
	goto tailRecurse;
    }
    FOREACH(superPtr, startPtr->superclasses) {
	if (TclOOIsReachable(targetPtr, superPtr)) {
	    return 1;
	}
    }
    FOREACH(superPtr, startPtr->mixins) {
	if (TclOOIsReachable(targetPtr, superPtr)) {
	    return 1;
	}
    }
    return 0;
}

/*
 * ----------------------------------------------------------------------
 *
 * TclOOObjectName, Tcl_GetObjectName --
 *
 *	Utility function that returns the name of the object. Note that this
 *	simplifies cache management by keeping the code to do it in one place
 *	and not sprayed all over. The value returned always has a reference
 *	count of at least one.
 *
 * ----------------------------------------------------------------------
 */

Tcl_Obj *
TclOOObjectName(
    Tcl_Interp *interp,
    Object *oPtr)
{
    Tcl_Obj *namePtr;

    if (oPtr->cachedNameObj) {
	return oPtr->cachedNameObj;
    }
    namePtr = Tcl_NewObj();
    Tcl_GetCommandFullName(interp, oPtr->command, namePtr);
    Tcl_IncrRefCount(namePtr);
    oPtr->cachedNameObj = namePtr;
    return namePtr;
}

Tcl_Obj *
Tcl_GetObjectName(
    Tcl_Interp *interp,
    Tcl_Object object)
{
    return TclOOObjectName(interp, (Object *) object);
}

/*
 * ----------------------------------------------------------------------
 *
 * assorted trivial 'getter' functions
 *
 * ----------------------------------------------------------------------
 */

Tcl_Method
Tcl_ObjectContextMethod(
    Tcl_ObjectContext context)
{
    CallContext *contextPtr = (CallContext *) context;
    return (Tcl_Method) contextPtr->callPtr->chain[contextPtr->index].mPtr;
}

int
Tcl_ObjectContextIsFiltering(
    Tcl_ObjectContext context)
{
    CallContext *contextPtr = (CallContext *) context;
    return contextPtr->callPtr->chain[contextPtr->index].isFilter;
}

Tcl_Object
Tcl_ObjectContextObject(
    Tcl_ObjectContext context)
{
    return (Tcl_Object) ((CallContext *)context)->oPtr;
}

int
Tcl_ObjectContextSkippedArgs(
    Tcl_ObjectContext context)
{
    return ((CallContext *)context)->skip;
}

Tcl_Namespace *
Tcl_GetObjectNamespace(
    Tcl_Object object)
{
    return ((Object *)object)->namespacePtr;
}

Tcl_Command
Tcl_GetObjectCommand(
    Tcl_Object object)
{
    return ((Object *)object)->command;
}

Tcl_Class
Tcl_GetObjectAsClass(
    Tcl_Object object)
{
    return (Tcl_Class) ((Object *)object)->classPtr;
}

int
Tcl_ObjectDeleted(
    Tcl_Object object)
{
    return ((Object *)object)->command == NULL;
}

Tcl_Object
Tcl_GetClassAsObject(
    Tcl_Class clazz)
{
    return (Tcl_Object) ((Class *)clazz)->thisPtr;
}

Tcl_ObjectMapMethodNameProc *
Tcl_ObjectGetMethodNameMapper(
    Tcl_Object object)
{
    return ((Object *) object)->mapMethodNameProc;
}

void
Tcl_ObjectSetMethodNameMapper(
    Tcl_Object object,
    Tcl_ObjectMapMethodNameProc *mapMethodNameProc)
{
    ((Object *) object)->mapMethodNameProc = mapMethodNameProc;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
