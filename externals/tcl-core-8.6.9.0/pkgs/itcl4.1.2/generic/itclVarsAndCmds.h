/*
 * ------------------------------------------------------------------------
 *      PACKAGE:  [incr Tcl]
 *  DESCRIPTION:  Object-Oriented Extensions to Tcl
 *
 *  These procedures handle command and variable resolution
 *
 * ========================================================================
 *  AUTHOR:  Arnulf Wiedemann
 *
 * ========================================================================
 *           Copyright (c) Arnulf Wiedemann
 * ------------------------------------------------------------------------
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

typedef int (ItclCheckClassProtection)(Tcl_Interp *interp,
         Tcl_Namespace *nsPtr, const char *varName, ClientData clientData);

ClientData Itcl_RegisterClassVariable(Tcl_Interp *interp,
        Tcl_Namespace *nsPtr, const char *varName, ClientData clientData);

Tcl_Var Itcl_RegisterObjectVariable( Tcl_Interp *interp, ItclObject *ioPtr,
	const char *varName, ClientData clientData, Tcl_Var varPtr,
	Tcl_Namespace *nsPtr);

ClientData Itcl_RegisterClassCommand(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
        const char *cmdName, ClientData clientData);

Tcl_Command Itcl_RegisterObjectCommand( Tcl_Interp *interp, ItclObject *ioPtr,
	const char *cmdName, ClientData clientData, Tcl_Command cmdPtr,
	Tcl_Namespace *nsPtr);

int Itcl_CheckClassVariableProtection(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
        const char *varName, ClientData clientData);

int Itcl_CheckClassCommandProtection(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
        const char *cmdName, ClientData clientData);

int Itcl_SetClassVariableProtectionCallback(Tcl_Interp *interp,
         Tcl_Namespace *nsPtr, ItclCheckClassProtection *fcnPtr);

int Itcl_SetClassCommandProtectionCallback(Tcl_Interp *interp,
         Tcl_Namespace *nsPtr, ItclCheckClassProtection *fcnPtr);

