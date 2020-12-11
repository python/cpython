/*
 * tclResolve.c --
 *
 *	Contains hooks for customized command/variable name resolution
 *	schemes. These hooks allow extensions like [incr Tcl] to add their own
 *	name resolution rules to the Tcl language. Rules can be applied to a
 *	particular namespace, to the interpreter as a whole, or both.
 *
 * Copyright (c) 1998 Lucent Technologies, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Declarations for functions local to this file:
 */

static void		BumpCmdRefEpochs(Namespace *nsPtr);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AddInterpResolvers --
 *
 *	Adds a set of command/variable resolution functions to an interpreter.
 *	These functions are consulted when commands are resolved in
 *	Tcl_FindCommand, and when variables are resolved in TclLookupVar and
 *	LookupCompiledLocal. Each namespace may also have its own set of
 *	resolution functions which take precedence over those for the
 *	interpreter.
 *
 *	When a name is resolved, it is handled as follows. First, the name is
 *	passed to the resolution functions for the namespace. If not resolved,
 *	the name is passed to each of the resolution functions added to the
 *	interpreter. Finally, if still not resolved, the name is handled using
 *	the default Tcl rules for name resolution.
 *
 * Results:
 *	Returns pointers to the current name resolution functions in the
 *	cmdProcPtr, varProcPtr and compiledVarProcPtr arguments.
 *
 * Side effects:
 *	If a compiledVarProc is specified, this function bumps the
 *	compileEpoch for the interpreter, forcing all code to be recompiled.
 *	If a cmdProc is specified, this function bumps the cmdRefEpoch in all
 *	namespaces, forcing commands to be resolved again using the new rules.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AddInterpResolvers(
    Tcl_Interp *interp,		/* Interpreter whose name resolution rules are
				 * being modified. */
    const char *name,		/* Name of this resolution scheme. */
    Tcl_ResolveCmdProc *cmdProc,/* New function for command resolution. */
    Tcl_ResolveVarProc *varProc,/* Function for variable resolution at
				 * runtime. */
    Tcl_ResolveCompiledVarProc *compiledVarProc)
				/* Function for variable resolution at compile
				 * time. */
{
    Interp *iPtr = (Interp *) interp;
    ResolverScheme *resPtr;
    unsigned len;

    /*
     * Since we're adding a new name resolution scheme, we must force all code
     * to be recompiled to use the new scheme. If there are new compiled
     * variable resolution rules, bump the compiler epoch to invalidate
     * compiled code. If there are new command resolution rules, bump the
     * cmdRefEpoch in all namespaces.
     */

    if (compiledVarProc) {
	iPtr->compileEpoch++;
    }
    if (cmdProc) {
	BumpCmdRefEpochs(iPtr->globalNsPtr);
    }

    /*
     * Look for an existing scheme with the given name. If found, then replace
     * its rules.
     */

    for (resPtr=iPtr->resolverPtr ; resPtr!=NULL ; resPtr=resPtr->nextPtr) {
	if (*name == *resPtr->name && strcmp(name, resPtr->name) == 0) {
	    resPtr->cmdResProc = cmdProc;
	    resPtr->varResProc = varProc;
	    resPtr->compiledVarResProc = compiledVarProc;
	    return;
	}
    }

    /*
     * Otherwise, this is a new scheme. Add it to the FRONT of the linked
     * list, so that it overrides existing schemes.
     */

    resPtr = ckalloc(sizeof(ResolverScheme));
    len = strlen(name) + 1;
    resPtr->name = ckalloc(len);
    memcpy(resPtr->name, name, len);
    resPtr->cmdResProc = cmdProc;
    resPtr->varResProc = varProc;
    resPtr->compiledVarResProc = compiledVarProc;
    resPtr->nextPtr = iPtr->resolverPtr;
    iPtr->resolverPtr = resPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetInterpResolvers --
 *
 *	Looks for a set of command/variable resolution functions with the
 *	given name in an interpreter. These functions are registered by
 *	calling Tcl_AddInterpResolvers.
 *
 * Results:
 *	If the name is recognized, this function returns non-zero, along with
 *	pointers to the name resolution functions in the Tcl_ResolverInfo
 *	structure. If the name is not recognized, this function returns zero.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetInterpResolvers(
    Tcl_Interp *interp,		/* Interpreter whose name resolution rules are
				 * being queried. */
    const char *name,		/* Look for a scheme with this name. */
    Tcl_ResolverInfo *resInfoPtr)
				/* Returns pointers to the functions, if
				 * found */
{
    Interp *iPtr = (Interp *) interp;
    ResolverScheme *resPtr;

    /*
     * Look for an existing scheme with the given name. If found, then return
     * pointers to its functions.
     */

    for (resPtr=iPtr->resolverPtr ; resPtr!=NULL ; resPtr=resPtr->nextPtr) {
	if (*name == *resPtr->name && strcmp(name, resPtr->name) == 0) {
	    resInfoPtr->cmdResProc = resPtr->cmdResProc;
	    resInfoPtr->varResProc = resPtr->varResProc;
	    resInfoPtr->compiledVarResProc = resPtr->compiledVarResProc;
	    return 1;
	}
    }

    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_RemoveInterpResolvers --
 *
 *	Removes a set of command/variable resolution functions previously
 *	added by Tcl_AddInterpResolvers. The next time a command/variable name
 *	is resolved, these functions won't be consulted.
 *
 * Results:
 *	Returns non-zero if the name was recognized and the resolution scheme
 *	was deleted. Returns zero otherwise.
 *
 * Side effects:
 *	If a scheme with a compiledVarProc was deleted, this function bumps
 *	the compileEpoch for the interpreter, forcing all code to be
 *	recompiled. If a scheme with a cmdProc was deleted, this function
 *	bumps the cmdRefEpoch in all namespaces, forcing commands to be
 *	resolved again using the new rules.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_RemoveInterpResolvers(
    Tcl_Interp *interp,		/* Interpreter whose name resolution rules are
				 * being modified. */
    const char *name)		/* Name of the scheme to be removed. */
{
    Interp *iPtr = (Interp *) interp;
    ResolverScheme **prevPtrPtr, *resPtr;

    /*
     * Look for an existing scheme with the given name.
     */

    prevPtrPtr = &iPtr->resolverPtr;
    for (resPtr=iPtr->resolverPtr ; resPtr!=NULL ; resPtr=resPtr->nextPtr) {
	if (*name == *resPtr->name && strcmp(name, resPtr->name) == 0) {
	    break;
	}
	prevPtrPtr = &resPtr->nextPtr;
    }

    /*
     * If we found the scheme, delete it.
     */

    if (resPtr) {
	/*
	 * If we're deleting a scheme with compiled variable resolution rules,
	 * bump the compiler epoch to invalidate compiled code. If we're
	 * deleting a scheme with command resolution rules, bump the
	 * cmdRefEpoch in all namespaces.
	 */

	if (resPtr->compiledVarResProc) {
	    iPtr->compileEpoch++;
	}
	if (resPtr->cmdResProc) {
	    BumpCmdRefEpochs(iPtr->globalNsPtr);
	}

	*prevPtrPtr = resPtr->nextPtr;
	ckfree(resPtr->name);
	ckfree(resPtr);

	return 1;
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * BumpCmdRefEpochs --
 *
 *	This function is used to bump the cmdRefEpoch counters in the
 *	specified namespace and all of its child namespaces. It is used
 *	whenever name resolution schemes are added/removed from an
 *	interpreter, to invalidate all command references.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Bumps the cmdRefEpoch in the specified namespace and its children,
 *	recursively.
 *
 *----------------------------------------------------------------------
 */

static void
BumpCmdRefEpochs(
    Namespace *nsPtr)		/* Namespace being modified. */
{
    Tcl_HashEntry *entry;
    Tcl_HashSearch search;

    nsPtr->cmdRefEpoch++;

#ifndef BREAK_NAMESPACE_COMPAT
    for (entry = Tcl_FirstHashEntry(&nsPtr->childTable, &search);
	    entry != NULL; entry = Tcl_NextHashEntry(&search)) {
	Namespace *childNsPtr = Tcl_GetHashValue(entry);

	BumpCmdRefEpochs(childNsPtr);
    }
#else
    if (nsPtr->childTablePtr != NULL) {
	for (entry = Tcl_FirstHashEntry(nsPtr->childTablePtr, &search);
		entry != NULL; entry = Tcl_NextHashEntry(&search)) {
	    Namespace *childNsPtr = Tcl_GetHashValue(entry);

	    BumpCmdRefEpochs(childNsPtr);
	}
    }
#endif
    TclInvalidateNsPath(nsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetNamespaceResolvers --
 *
 *	Sets the command/variable resolution functions for a namespace,
 *	thereby changing the way that command/variable names are interpreted.
 *	This allows extension writers to support different name resolution
 *	schemes, such as those for object-oriented packages.
 *
 *	Command resolution is handled by a function of the following type:
 *
 *	  typedef int (Tcl_ResolveCmdProc)(Tcl_Interp *interp,
 *		  const char *name, Tcl_Namespace *context,
 *		  int flags, Tcl_Command *rPtr);
 *
 *	Whenever a command is executed or Tcl_FindCommand is invoked within
 *	the namespace, this function is called to resolve the command name. If
 *	this function is able to resolve the name, it should return the status
 *	code TCL_OK, along with the corresponding Tcl_Command in the rPtr
 *	argument. Otherwise, the function can return TCL_CONTINUE, and the
 *	command will be treated under the usual name resolution rules. Or, it
 *	can return TCL_ERROR, and the command will be considered invalid.
 *
 *	Variable resolution is handled by two functions. The first is called
 *	whenever a variable needs to be resolved at compile time:
 *
 *	  typedef int (Tcl_ResolveCompiledVarProc)(Tcl_Interp *interp,
 *		  const char *name, Tcl_Namespace *context,
 *		  Tcl_ResolvedVarInfo *rPtr);
 *
 *	If this function is able to resolve the name, it should return the
 *	status code TCL_OK, along with variable resolution info in the rPtr
 *	argument; this info will be used to set up compiled locals in the call
 *	frame at runtime. The function may also return TCL_CONTINUE, and the
 *	variable will be treated under the usual name resolution rules. Or, it
 *	can return TCL_ERROR, and the variable will be considered invalid.
 *
 *	Another function is used whenever a variable needs to be resolved at
 *	runtime but it is not recognized as a compiled local. (For example,
 *	the variable may be requested via Tcl_FindNamespaceVar.) This function
 *	has the following type:
 *
 *	  typedef int (Tcl_ResolveVarProc)(Tcl_Interp *interp,
 *		  const char *name, Tcl_Namespace *context,
 *		  int flags, Tcl_Var *rPtr);
 *
 *	This function is quite similar to the compile-time version. It returns
 *	the same status codes, but if variable resolution succeeds, this
 *	function returns a Tcl_Var directly via the rPtr argument.
 *
 * Results:
 *	Nothing.
 *
 * Side effects:
 *	Bumps the command epoch counter for the namespace, invalidating all
 *	command references in that namespace. Also bumps the resolver epoch
 *	counter for the namespace, forcing all code in the namespace to be
 *	recompiled.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetNamespaceResolvers(
    Tcl_Namespace *namespacePtr,/* Namespace whose resolution rules are being
				 * modified. */
    Tcl_ResolveCmdProc *cmdProc,/* Function for command resolution */
    Tcl_ResolveVarProc *varProc,/* Function for variable resolution at
				 * run-time */
    Tcl_ResolveCompiledVarProc *compiledVarProc)
				/* Function for variable resolution at compile
				 * time. */
{
    Namespace *nsPtr = (Namespace *) namespacePtr;

    /*
     * Plug in the new command resolver, and bump the epoch counters so that
     * all code will have to be recompiled and all commands will have to be
     * resolved again using the new policy.
     */

    nsPtr->cmdResProc = cmdProc;
    nsPtr->varResProc = varProc;
    nsPtr->compiledVarResProc = compiledVarProc;

    nsPtr->cmdRefEpoch++;
    nsPtr->resolverEpoch++;
    TclInvalidateNsPath(nsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetNamespaceResolvers --
 *
 *	Returns the current command/variable resolution functions for a
 *	namespace. By default, these functions are NULL. New functions can be
 *	installed by calling Tcl_SetNamespaceResolvers, to provide new name
 *	resolution rules.
 *
 * Results:
 *	Returns non-zero if any name resolution functions have been assigned
 *	to this namespace; also returns pointers to the functions in the
 *	Tcl_ResolverInfo structure. Returns zero otherwise.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetNamespaceResolvers(
    Tcl_Namespace *namespacePtr,/* Namespace whose resolution rules are being
				 * modified. */
    Tcl_ResolverInfo *resInfoPtr)
				/* Returns: pointers for all name resolution
				 * functions assigned to this namespace. */
{
    Namespace *nsPtr = (Namespace *) namespacePtr;

    resInfoPtr->cmdResProc = nsPtr->cmdResProc;
    resInfoPtr->varResProc = nsPtr->varResProc;
    resInfoPtr->compiledVarResProc = nsPtr->compiledVarResProc;

    if (nsPtr->cmdResProc != NULL || nsPtr->varResProc != NULL ||
	    nsPtr->compiledVarResProc != NULL) {
	return 1;
    }
    return 0;
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
