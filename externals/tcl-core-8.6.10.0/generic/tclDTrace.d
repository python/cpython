/*
 * tclDTrace.d --
 *
 *	Tcl DTrace provider.
 *
 * Copyright (c) 2007-2008 Daniel A. Steffen <das@users.sourceforge.net>
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

typedef struct Tcl_Obj Tcl_Obj;

/*
 * Tcl DTrace probes
 */

provider tcl {
    /***************************** proc probes *****************************/
    /*
     *	tcl*:::proc-entry probe
     *	    triggered immediately before proc bytecode execution
     *		arg0: proc name				(string)
     *		arg1: number of arguments		(int)
     *		arg2: array of proc argument objects	(Tcl_Obj**)
     */
    probe proc__entry(const char *name, int objc, struct Tcl_Obj **objv);
    /*
     *	tcl*:::proc-return probe
     *	    triggered immediately after proc bytecode execution
     *		arg0: proc name				(string)
     *		arg1: return code			(int)
     */
    probe proc__return(const char *name, int code);
    /*
     *	tcl*:::proc-result probe
     *	    triggered after proc-return probe and result processing
     *		arg0: proc name				(string)
     *		arg1: return code			(int)
     *		arg2: proc result			(string)
     *		arg3: proc result object		(Tcl_Obj*)
     */
    probe proc__result(const char *name, int code, const char *result,
	    struct Tcl_Obj *resultobj);
    /*
     *	tcl*:::proc-args probe
     *	    triggered before proc-entry probe, gives access to string
     *	    representation of proc arguments
     *		arg0: proc name				(string)
     *		arg1-arg9: proc arguments or NULL	(strings)
     */
    probe proc__args(const char *name, const char *arg1, const char *arg2,
	    const char *arg3, const char *arg4, const char *arg5,
	    const char *arg6, const char *arg7, const char *arg8,
	    const char *arg9);
    /*
     *	tcl*:::proc-info probe
     *	    triggered before proc-entry probe, gives access to TIP 280
     *	    information for the proc invocation (i.e. [info frame 0])
     *		arg0: TIP 280 cmd			(string)
     *		arg1: TIP 280 type			(string)
     *		arg2: TIP 280 proc			(string)
     *		arg3: TIP 280 file			(string)
     *		arg4: TIP 280 line			(int)
     *		arg5: TIP 280 level			(int)
     *		arg6: TclOO method			(string)
     *		arg7: TclOO class/object		(string)
     */
    probe proc__info(const char *cmd, const char *type, const char *proc,
	    const char *file, int line, int level, const char *method,
	    const char *class);

    /***************************** cmd probes ******************************/
    /*
     *	tcl*:::cmd-entry probe
     *	    triggered immediately before commmand execution
     *		arg0: command name			(string)
     *		arg1: number of arguments		(int)
     *		arg2: array of command argument objects	(Tcl_Obj**)
     */
    probe cmd__entry(const char *name, int objc, struct Tcl_Obj **objv);
    /*
     *	tcl*:::cmd-return probe
     *	    triggered immediately after commmand execution
     *		arg0: command name			(string)
     *		arg1: return code			(int)
     */
    probe cmd__return(const char *name, int code);
    /*
     *	tcl*:::cmd-result probe
     *	    triggered after cmd-return probe and result processing
     *		arg0: command name			(string)
     *		arg1: return code			(int)
     *		arg2: command result			(string)
     *		arg3: command result object		(Tcl_Obj*)
     */
    probe cmd__result(const char *name, int code, const char *result,
	    struct Tcl_Obj *resultobj);
    /*
     *	tcl*:::cmd-args probe
     *	    triggered before cmd-entry probe, gives access to string
     *	    representation of command arguments
     *		arg0: command name			(string)
     *		arg1-arg9: command arguments or NULL	(strings)
     */
    probe cmd__args(const char *name, const char *arg1, const char *arg2,
	    const char *arg3, const char *arg4, const char *arg5,
	    const char *arg6, const char *arg7, const char *arg8,
	    const char *arg9);
    /*
     *	tcl*:::cmd-info probe
     *	    triggered before cmd-entry probe, gives access to TIP 280
     *	    information for the command invocation (i.e. [info frame 0])
     *		arg0: TIP 280 cmd			(string)
     *		arg1: TIP 280 type			(string)
     *		arg2: TIP 280 proc			(string)
     *		arg3: TIP 280 file			(string)
     *		arg4: TIP 280 line			(int)
     *		arg5: TIP 280 level			(int)
     *		arg6: TclOO method			(string)
     *		arg7: TclOO class/object		(string)
     */
    probe cmd__info(const char *cmd, const char *type, const char *proc,
	    const char *file, int line, int level, const char *method,
	    const char *class);

    /***************************** inst probes *****************************/
    /*
     *	tcl*:::inst-start probe
     *	    triggered immediately before execution of a bytecode
     *		arg0: bytecode name			(string)
     *		arg1: depth of stack			(int)
     *		arg2: top of stack			(Tcl_Obj**)
     */
    probe inst__start(const char *name, int depth, struct Tcl_Obj **stack);
    /*
     *	tcl*:::inst-done probe
     *	    triggered immediately after execution of a bytecode
     *		arg0: bytecode name			(string)
     *		arg1: depth of stack			(int)
     *		arg2: top of stack			(Tcl_Obj**)
     */
    probe inst__done(const char *name, int depth, struct Tcl_Obj **stack);

    /***************************** obj probes ******************************/
    /*
     *	tcl*:::obj-create probe
     *	    triggered immediately after a new Tcl_Obj has been created
     *		arg0: object created			(Tcl_Obj*)
     */
    probe obj__create(struct Tcl_Obj* obj);
    /*
     *	tcl*:::obj-free probe
     *	    triggered immediately before a Tcl_Obj is freed
     *		arg0: object to be freed		(Tcl_Obj*)
     */
    probe obj__free(struct Tcl_Obj* obj);

    /***************************** tcl probes ******************************/
    /*
     *	tcl*:::tcl-probe probe
     *	    triggered when the ::tcl::dtrace command is called
     *		arg0-arg9: command arguments		(strings)
     */
    probe tcl__probe(const char *arg0, const char *arg1, const char *arg2,
	    const char *arg3, const char *arg4, const char *arg5,
	    const char *arg6, const char *arg7, const char *arg8,
	    const char *arg9);
};

/*
 * Tcl types and constants for use in DTrace scripts
 */

typedef struct Tcl_ObjType {
    const char *name;
    void *freeIntRepProc;
    void *dupIntRepProc;
    void *updateStringProc;
    void *setFromAnyProc;
} Tcl_ObjType;

struct Tcl_Obj {
    int refCount;
    char *bytes;
    int length;
    const Tcl_ObjType *typePtr;
    union {
	long longValue;
	double doubleValue;
	void *otherValuePtr;
	int64_t wideValue;
	struct {
	    void *ptr1;
	    void *ptr2;
	} twoPtrValue;
	struct {
	    void *ptr;
	    unsigned long value;
	} ptrAndLongRep;
    } internalRep;
};

enum return_codes {
    TCL_OK = 0,
    TCL_ERROR,
    TCL_RETURN,
    TCL_BREAK,
    TCL_CONTINUE
};

#pragma D attributes Evolving/Evolving/Common provider tcl provider
#pragma D attributes Private/Private/Common provider tcl module
#pragma D attributes Private/Private/Common provider tcl function
#pragma D attributes Evolving/Evolving/Common provider tcl name
#pragma D attributes Evolving/Evolving/Common provider tcl args

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
