
/*	$Id: tixOption.c,v 1.4 2008/02/28 04:29:03 hobbs Exp $	*/

/*
 * tixOption.c --
 *
 *	Handle the "$widget config" commands
 *
 * Copyright (c) 1996, Expert Interface Technologies
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* ToDo:
 *
 * 1) ConfigSpec structures shouldn't be shared between parent and child
 *    classes.
 * 2) Specs array should be compacted (no duplication) and sorted according
 *    to argvName during class declaration.
 * 3) Merge aliases with specs
 *
 */
#include <tk.h>
#include <tixPort.h>
#include <tixInt.h>

static char *	FormatConfigInfo(Tcl_Interp *interp,
	TixClassRecord *cPtr, CONST84 char * widRec, TixConfigSpec * spec);
/*
 * This is a lightweight interface for querying the value of a
 * variable in the object. This is simpler compared to
 * [lindex [$w config -flag] 4]
 */
int Tix_GetVar (interp, cPtr, widRec, flag)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char * widRec;
    CONST84 char * flag;
{
    TixConfigSpec * spec;
    CONST84 char * value;

    spec = Tix_FindConfigSpecByName(interp, cPtr, flag);
    if (spec != NULL) {
	if (spec->isAlias) {
	    flag = spec->realPtr->argvName;
	} else {
	    /* The user may have specified a shorthand like -backgro */
	    flag = spec->argvName;
	}
	value = Tcl_GetVar2(interp, widRec, flag, TCL_GLOBAL_ONLY);

	Tcl_AppendResult(interp, value, (char*)NULL);
	return TCL_OK;
    }
    else {
	return TCL_ERROR;
    }
}

int
Tix_QueryOneOption(interp, cPtr, widRec, flag)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char *widRec;
    CONST84 char *flag;
{
    TixConfigSpec * spec;
    char * list;

    spec = Tix_FindConfigSpecByName(interp, cPtr, flag);
    if (spec != NULL) {
	list = FormatConfigInfo(interp, cPtr, widRec, spec);
	Tcl_SetResult(interp, list, TCL_VOLATILE);

	ckfree((char *) list);
	return TCL_OK;
    }
    else {
	return TCL_ERROR;
    }
}

/*
 * Tix_QueryAllOptions --
 *
 *	Note: This function does not call Tix_FindConfigSpec() because
 * it just needs to print out the list of all configSpecs from the
 * class structure.
 *
 */
int
Tix_QueryAllOptions (interp, cPtr, widRec)
    Tcl_Interp *interp;
    TixClassRecord * cPtr;
    CONST84 char *widRec;
{
    int		i, code = TCL_OK;
    char *list;
    CONST84 char *lead = "{";

    /* Iterate over all the options of class */
    for (i=0; i<cPtr->nSpecs; i++) {
	if (cPtr->specs[i] && cPtr->specs[i]->argvName) {
	    list = FormatConfigInfo(interp, cPtr, widRec, cPtr->specs[i]);
	    Tcl_AppendResult(interp, lead, list, "}", (char *) NULL);

	    ckfree((char *) list);
	    lead = " {";
	}
    }

    return code;
}

TixConfigSpec *
Tix_FindConfigSpecByName(interp, cPtr, flag)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char *flag;
{
    CONST84 char *classRec;
    CONST84 char *key;
    int nMatch, i;
    size_t len;
    Tcl_HashEntry *hashPtr;
    TixConfigSpec *configSpec;

    classRec = cPtr->className;

    /*
     * First try to look up the confifspec in a hash table, 
     * it should be faster.
     */

    key = Tix_GetConfigSpecFullName(classRec, flag);
    hashPtr = Tcl_FindHashEntry(TixGetHashTable(interp, "tixSpecTab", NULL,
            TCL_STRING_KEYS), key);
    ckfree((char *) key);

    if (hashPtr) {
	return (TixConfigSpec *) Tcl_GetHashValue(hashPtr);
    }

    /*
     * The user may specified a partial name. Try to match, but will
     * return error if we don't get exactly one match.
     */
    len = strlen(flag);
    for (configSpec=NULL,nMatch=0,i=0; i<cPtr->nSpecs; i++) {
	if (strncmp(flag, cPtr->specs[i]->argvName, len) == 0) {
	    if (nMatch > 0) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "ambiguous option \"", flag, "\"",
		    (char*)NULL);
		return NULL;
	    } else {
		nMatch ++;
		configSpec = cPtr->specs[i];
	    }
	}
    }

    if (configSpec == NULL) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "unknown option \"", flag, "\"", (char*)NULL);
	return NULL;
    } else {
	return configSpec;
    }
}

CONST84 char *
Tix_GetConfigSpecFullName(classRec, flag)
    CONST84 char * classRec;
    CONST84 char * flag;
{
    char * buff;
    int    max;
    int    conLen;

    conLen = strlen(classRec);
    max = conLen + strlen(flag) + 1;
    buff = (char*)ckalloc(max * sizeof(char));

    strcpy(buff, classRec);
    strcpy(buff+conLen, flag);

    return buff;
}

int Tix_ChangeOptions(interp, cPtr, widRec, argc, argv)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char * widRec;
    int argc;
    CONST84 char ** argv;
{
    int i, code = TCL_OK;
    TixConfigSpec * spec;

    if (argc == 0) {
	goto done;
    }

    if ((argc %2) != 0) {
	if (Tix_FindConfigSpecByName(interp, cPtr, argv[argc-1])) {
	    Tcl_AppendResult(interp, "value for \"", argv[argc-1],
		"\" missing", (char*)NULL);
	} else {
	    /* The error message is already appended by 
	     * Tix_FindConfigSpecByName()
	     */
	}
	code = TCL_ERROR;
	goto done;
    }

    for (i=0; i<argc; i+=2) {
	spec = Tix_FindConfigSpecByName(interp, cPtr, argv[i]);

	if (spec == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}
	if (Tix_ChangeOneOption(interp, cPtr, widRec, spec,
		argv[i+1], 0, 0)!=TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }

  done:
    return code;
}

int Tix_CallConfigMethod(interp, cPtr, widRec, spec, value)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char * widRec;
    TixConfigSpec *spec;
    CONST84 char * value;
{
#define STATIC_SPACE_SIZE 60
    CONST84 char * argv[2];
    char buff[STATIC_SPACE_SIZE];
    char * method = buff;
    CONST84 char * context = Tix_GetContext(interp, widRec);
    CONST84 char * c;
    unsigned int bufsize = strlen(spec->argvName) + 7;
    int code;

    if (bufsize > STATIC_SPACE_SIZE) {
        method = ckalloc(bufsize);
    }

    sprintf(method, "config%s", spec->argvName);

    c = Tix_FindMethod(interp, context, method);
    if (c != NULL) {
	argv[0] = value;
	code = Tix_CallMethod(interp, c, widRec, method, 1, argv, NULL);
        goto done;
    }

    c = Tix_FindMethod(interp, context, "config");
    if (c != NULL) {
	argv[0] = spec->argvName;
	argv[1] = value;
	code = Tix_CallMethod(interp, c, widRec, "config", 2, argv, NULL);
        goto done;
    }

    code = TCL_OK;

done:
    if (method != buff) {
        ckfree((char *) method);
    }
    return code;
#undef STATIC_SPACE_SIZE
}

int Tix_ChangeOneOption(interp, cPtr, widRec, spec, value, isDefault, isInit)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char * widRec;
    TixConfigSpec *spec;
    CONST84 char * value;
    int isDefault;		/* Set to be true when Tix tries to set
				 * the options according to their default
				 * values */
    int isInit;			/* Set to true only if the option was
				 * specified at the widget creation command
				 */
{
    int code = TCL_OK;
    const char *newValue = NULL;

    if (spec->isAlias) {
	spec = spec->realPtr;
    }

    /* -- STEP 1 --
     * Check if these variables are protected.
     *	readonly means the variable can never be assigned to.
     *	static means ths variable can only ne assigned during initialization.
     */

    if (!isDefault && spec->readOnly) {
	Tcl_AppendResult(interp, "cannot assigned to readonly variable \"",
		spec->argvName, "\"", (char*) NULL);
	code = TCL_ERROR;
	goto done;
    }
    if (!(isInit || isDefault) && spec->isStatic) {
	Tcl_AppendResult(interp, "cannot assigned to static variable \"",
		spec->argvName, "\"", (char*) NULL);
	code = TCL_ERROR;
	goto done;
    }

    /* -- STEP 2 --
     * Call the verify command to check whether the value is acceptable
     *
     */
    if (spec->verifyCmd != NULL) {
	CONST84 char * cmdArgv[2];
	cmdArgv[0] = spec->verifyCmd;
	cmdArgv[1] = value;

	if (Tix_EvalArgv(interp, 2, cmdArgv) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	} else {
	    newValue = value = tixStrDup(Tcl_GetStringResult(interp));
	}
    }

    /* -- STEP 3 --
     * Call the configuration method of the widget. This method may
     * override the user-supplied value. The configuration method should
     * not be called during initialization.
     */
    if (isInit || isDefault) {
	/* No need to call the configuration method */
	Tcl_SetVar2(interp, widRec, spec->argvName, value,TCL_GLOBAL_ONLY);
    } else {
	const char *str;

	if (Tix_CallConfigMethod(interp, cPtr, widRec, spec, value)!=TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
	/* -- STEP 4 --
	 * If the configuration method does not override the value, set
	 * it in the widget record.
	 *
	 * If the configuration method has override the value, it will
	 * return a non-empty string. In this case, it is the configuration
	 * method's responsibility to set the value in the widget record.
	 */

	str = Tcl_GetStringResult(interp);
	if (str && *str) {
	    /* value was overrided. Don't do anything */
	    Tcl_ResetResult(interp);
	} else {
	    Tcl_SetVar2(interp, widRec, spec->argvName, value,TCL_GLOBAL_ONLY);
	}
    }

  done:
    if (newValue) {
	ckfree((char *) newValue);
    }
    return code;
}

static char *
FormatConfigInfo(interp, cPtr, widRec, sPtr)
    Tcl_Interp *interp;
    TixClassRecord *cPtr;
    CONST84 char * widRec;
    TixConfigSpec * sPtr;
{
    CONST84 char *argv[6];

    if (sPtr->isAlias) {
	if (cPtr->isWidget) {
	    argv[0] = sPtr->argvName;
	    argv[1] = sPtr->realPtr->dbName;
	} else {
	    argv[0] = sPtr->argvName;
	    argv[1] = sPtr->realPtr->argvName;
	}
	return Tcl_Merge(2, argv);
    } else {
	argv[0] = sPtr->argvName;
	argv[1] = sPtr->dbName;
	argv[2] = sPtr->dbClass;
	argv[3] = sPtr->defValue;
	argv[4] = Tcl_GetVar2(interp, widRec, argv[0], TCL_GLOBAL_ONLY);

	return Tcl_Merge(5, argv);
    }
}
