/*
 * tixClass.c --
 *
 *	Implements the basic OOP class mechanism for the Tix Intrinsics.
 *
 * Copyright (c) 1993-1999 Ioi Kim Lam.
 * Copyright (c) 2000-2001 Tix Project Group.
 *
 * See the file "license.terms" for information on usage and
 * redistribution of this file, and for a DISCLAIMER OF ALL
 * WARRANTIES.
 *
 * $Id: tixClass.c,v 1.7 2008/02/28 04:29:47 hobbs Exp $
 */

/*
 *
 * Todo:
 *
 * (1) 	Problems: now a class shares some configspecs with the parent class.
 *    	If an option is declared as -static in the child class but not
 *	in the parent class, the parent class will still see this
 *	option as static.
 *
 */


#include <tk.h>
#include <tixPort.h>
#include <tixInt.h>

/*
 * Access control is not enabled yet.
 */
#define USE_ACCESS_CONTROL 0


static void		ClassTableDeleteProc _ANSI_ARGS_((
			    ClientData clientData, Tcl_Interp *interp));
static TixConfigSpec *	CopySpec _ANSI_ARGS_((TixConfigSpec *spec));
static TixClassRecord * CreateClassByName _ANSI_ARGS_((Tcl_Interp * interp,
			    CONST84 char * classRec));
static TixClassRecord * CreateClassRecord _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST84 char * classRec, Tk_Window mainWindow,
			    int isWidget));
static void		FreeClassRecord _ANSI_ARGS_((
			    TixClassRecord *cPtr));
static void		FreeParseOptions _ANSI_ARGS_((
			    TixClassParseStruct * parsePtr));
static void		FreeSpec _ANSI_ARGS_((TixConfigSpec *spec));
static TixClassRecord * GetClassByName _ANSI_ARGS_((Tcl_Interp * interp,
			    CONST84 char * classRec));
static TixConfigSpec *	InitAlias _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s));
static int		InitHashEntries _ANSI_ARGS_((
			    Tcl_Interp *interp,TixClassRecord * cPtr));
static int		InitClass _ANSI_ARGS_((Tcl_Interp * interp,
			    CONST84 char * classRec, TixClassRecord * cPtr,
			    TixClassRecord * scPtr,
			    TixClassParseStruct * parsePtr));
static TixConfigSpec *	InitSpec _ANSI_ARGS_((Tcl_Interp * interp,
			    CONST84 char * s, int isWidget));
static int 		ParseClassOptions _ANSI_ARGS_((
			    Tcl_Interp * interp, CONST84 char * opts,
			    TixClassParseStruct * rec));
static int		ParseInstanceOptions _ANSI_ARGS_((
			    Tcl_Interp * interp,TixClassRecord * cPtr,
			    CONST84 char *widRec, int argc, CONST84 char** argv));
static int 		SetupAlias _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s));
static int		SetupAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s,
			    int which));
static int 		SetupMethod _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s));
static int 		SetupDefault _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s));
#if USE_ACCESS_CONTROL
static int 		SetupSubWidget _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s));
#endif
static int 		SetupSpec _ANSI_ARGS_((Tcl_Interp *interp,
			    TixClassRecord * cPtr, CONST84 char *s,
			    int isWidget));

TIX_DECLARE_CMD(Tix_CreateWidgetCmd);
TIX_DECLARE_CMD(Tix_CreateInstanceCmd);
TIX_DECLARE_CMD(Tix_InstanceCmd);
TIX_DECLARE_CMD(Tix_UninitializedClassCmd);

/*
 * Per-interpreter hashtables used to store the classes and class specs. 
 */

#define GetClassTable(interp) \
    (TixGetHashTable(interp, "tixClassTab",  ClassTableDeleteProc, \
            TCL_STRING_KEYS))
#define GetSpecTable(interp)  \
    (TixGetHashTable(interp, "tixSpecTab", NULL, TCL_STRING_KEYS))

static char * TIX_EMPTY_STRING = "";


/*----------------------------------------------------------------------
 * GetClassByName --
 *
 *	Return a class struct if it has been created.
 *
 *----------------------------------------------------------------------
 */

static TixClassRecord *
GetClassByName(interp, classRec)
    Tcl_Interp * interp;
    CONST84 char * classRec;
{
    Tcl_HashEntry *hashPtr;

    hashPtr = Tcl_FindHashEntry(GetClassTable(interp), classRec);
    if (hashPtr) {
	return (TixClassRecord *)Tcl_GetHashValue(hashPtr);
    } else {
	return NULL;
    }
}

static TixClassRecord *
CreateClassByName(interp, classRec)
    Tcl_Interp * interp;
    CONST84 char * classRec;
{
    TixClassRecord * cPtr;
    Tcl_SavedResult state;

    cPtr = GetClassByName(interp, classRec);
    if (cPtr == NULL) {
	Tcl_SaveResult(interp, &state);
	if (Tix_GlobalVarEval(interp, classRec, ":AutoLoad", (char*)NULL)
	        == TCL_ERROR){
	    cPtr = NULL;
	} else {
	    cPtr = GetClassByName(interp, classRec);
	}
	Tcl_RestoreResult(interp, &state);
    }

    return cPtr;
}

/*----------------------------------------------------------------------
 * CreateClassRecord --
 *
 *	Create a class record for the definiton of a new class, or return
 *	error if the class already exists.
 *
 *----------------------------------------------------------------------
 */

static TixClassRecord *
CreateClassRecord(interp, classRec, mainWindow, isWidget)
    Tcl_Interp * interp;
    CONST84 char * classRec;
    Tk_Window mainWindow;
    int isWidget;
{
    Tcl_HashEntry *hashPtr;
    int isNew;
    TixClassRecord * cPtr;

    hashPtr = Tcl_CreateHashEntry(GetClassTable(interp), classRec, &isNew);

    if (isNew) {
	cPtr = (TixClassRecord *)Tix_ZAlloc(sizeof(TixClassRecord));
#if USE_ACCESS_CONTROL
	cPtr->next       = NULL;
#endif
	cPtr->superClass = NULL;
	cPtr->isWidget   = isWidget;
	cPtr->className  = tixStrDup(classRec);
	cPtr->ClassName  = NULL;
	cPtr->nSpecs     = 0;
	cPtr->specs      = 0;
	cPtr->nMethods   = 0;
	cPtr->methods    = 0;
	cPtr->mainWindow = mainWindow;
	cPtr->parsePtr	 = NULL;
	cPtr->initialized= 0;
	Tix_SimpleListInit(&cPtr->unInitSubCls);
	Tix_SimpleListInit(&cPtr->subWDefs);

#if USE_ACCESS_CONTROL
	Tix_SimpleListInit(&cPtr->subWidgets);
#endif

	Tcl_SetHashValue(hashPtr, (char*)cPtr);
	return cPtr;
    } else {

	/*
	 * We don't allow redefinition of classes
	 */

	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "Class \"", classRec, "\" redefined", NULL);
	return NULL;
    }
}

/*----------------------------------------------------------------------
 * Tix_ClassCmd
 *
 * 	Create a class record for a Tix class.
 *
 * argv[0] = "tixClass" or "tixWidgetClass"
 * argv[1] = class
 * argv[2] = arglist
 *----------------------------------------------------------------------
 */

TIX_DEFINE_CMD(Tix_ClassCmd)
{
    int	isWidget, code = TCL_OK;
    TixClassParseStruct * parsePtr;
    TixClassRecord * cPtr, * scPtr;
    CONST84 char * classRec = argv[1];
    Tk_Window mainWindow = (Tk_Window)clientData;

    if (strcmp(argv[0], "tixClass")==0) {
	isWidget = 0;
    } else {
	isWidget = 1;
    }

    if (argc != 3) {
	return Tix_ArgcError(interp, argc, argv, 1, "className {...}");
    }

    if (strstr(argv[1], "::") != NULL) {
        /*
         * Cannot contain :: in instance name, otherwise all hell will
         * rise w.r.t. namespace
         */

        Tcl_AppendResult(interp, "invalid class name \"", argv[1],
		"\": may not contain substring \"::\"", NULL);
        return TCL_ERROR;
    }


    parsePtr = (TixClassParseStruct *)Tix_ZAlloc(sizeof(TixClassParseStruct));
    if (ParseClassOptions(interp, argv[2], parsePtr) != TCL_OK) {
	ckfree((char*)parsePtr);
	parsePtr = NULL;
	code = TCL_ERROR;
	goto done;
    }

    cPtr = GetClassByName(interp, classRec);
    if (cPtr == NULL) {
	cPtr = CreateClassRecord(interp, classRec, mainWindow, isWidget);
	if (cPtr == NULL) {
	    code = TCL_ERROR;
	    goto done;
	}
    }
    if (cPtr->initialized) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "Class \"", classRec, "\" redefined", NULL);
	code = TCL_ERROR;
	goto done;
    }

    /*
     * (2) Set up the superclass
     */

    if (!parsePtr->superClass || strlen(parsePtr->superClass) == 0) {
	scPtr = NULL;
    }
    else {
	/*
	 * Create the superclass's record if it does not exist yet.
	 */
	scPtr = GetClassByName(interp, parsePtr->superClass);
	if (scPtr == NULL) {
	    scPtr = CreateClassByName(interp, parsePtr->superClass);
	    if (scPtr == NULL) {
		/*
		 * The superclass cannot be autoloaded. We create a
		 * empty class record. This record may later be filled
		 * by a tixClass call (which may be initiated by a
		 * "source" call by the the application, or by SAM).
		 */
		scPtr = CreateClassRecord(interp, parsePtr->superClass,
			mainWindow, isWidget);
		if (scPtr == NULL) {
		    code = TCL_ERROR;
		    goto done;
		}
	    }
	}
    }
    cPtr->superClass = scPtr;

    if (scPtr == NULL || scPtr->initialized == 1) {
	/*
	 * It is safe to initialized the class now.
	 */

	code = InitClass(interp, classRec, cPtr, scPtr, parsePtr);
	FreeParseOptions(parsePtr);
        parsePtr = NULL;
	cPtr->parsePtr = NULL;
    } else {
	/*
	 * This class has an uninitialized superclass. We wait until the
	 * superclass is initialized before we initialize this class.
         *
         * Because there is no :: inside the cPtr->className, the command is
         * created in the global namespace.
	 */

	Tix_SimpleListAppend(&scPtr->unInitSubCls, (char*)cPtr, 0);
	Tcl_CreateCommand(interp, cPtr->className,
		Tix_UninitializedClassCmd, (ClientData)cPtr, NULL);
	cPtr->parsePtr = parsePtr;
    }

done:
    if (code == TCL_ERROR) {
	if (parsePtr != NULL) {
	    FreeParseOptions(parsePtr);
	}
    }
    return code;
}

static int
ParseClassOptions(interp, opts, parsePtr)
    Tcl_Interp * interp;
    CONST84 char * opts;
    TixClassParseStruct * parsePtr;
{
    int	   i;
    int code = TCL_OK;

    parsePtr->alias		= TIX_EMPTY_STRING;
    parsePtr->configSpec	= TIX_EMPTY_STRING;
    parsePtr->ClassName		= TIX_EMPTY_STRING;
    parsePtr->def		= TIX_EMPTY_STRING;
    parsePtr->flag		= TIX_EMPTY_STRING;
    parsePtr->forceCall		= TIX_EMPTY_STRING;
    parsePtr->isStatic		= TIX_EMPTY_STRING;
    parsePtr->method		= TIX_EMPTY_STRING;
    parsePtr->readOnly		= TIX_EMPTY_STRING;
    parsePtr->subWidget		= TIX_EMPTY_STRING;
    parsePtr->superClass	= TIX_EMPTY_STRING;
    parsePtr->isVirtual		= TIX_EMPTY_STRING;
    parsePtr->optArgv		= NULL;

    if (Tcl_SplitList(interp, opts, &parsePtr->optArgc, &parsePtr->optArgv)
	    != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    if ((parsePtr->optArgc %2) == 1) {
	Tcl_AppendResult(interp, "value for \"", 
		parsePtr->optArgv[parsePtr->optArgc-1],
	        "\" missing", (char*)NULL);
	code = TCL_ERROR;
	goto done;
    }
    for (i=0; i<parsePtr->optArgc; i+=2) {
	if (strcmp(parsePtr->optArgv[i], "-alias") == 0) {
	    parsePtr->alias = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-configspec") == 0) {
	    parsePtr->configSpec = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-classname") == 0) {
	    parsePtr->ClassName = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-default") == 0) {
	    parsePtr->def = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-flag") == 0) {
	    parsePtr->flag = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-forcecall") == 0) {
	    parsePtr->forceCall = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-method") == 0) {
	    parsePtr->method = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-readonly") == 0) {
	    parsePtr->readOnly = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-static") == 0) {
	    parsePtr->isStatic = parsePtr->optArgv[i+1];
	}
#if USE_ACCESS_CONTROL
	else if (strcmp(parsePtr->optArgv[i], "-subwidget") == 0) {
	    parsePtr->subWidget = parsePtr->optArgv[i+1];
	}
#endif
	else if (strcmp(parsePtr->optArgv[i], "-superclass") == 0) {
	    parsePtr->superClass = parsePtr->optArgv[i+1];
	}
	else if (strcmp(parsePtr->optArgv[i], "-virtual") == 0) {
	    parsePtr->isVirtual = parsePtr->optArgv[i+1];
	}
	else {
	    Tcl_AppendResult(interp, "unknown parsePtr->option \"",
		parsePtr->optArgv[i], "\"", (char*)NULL);
	    code = TCL_ERROR;
	    goto done;
	}
    }

  done:
    if (code != TCL_OK) {
	if (parsePtr->optArgv != NULL) {
	    ckfree((char*)parsePtr->optArgv);
	    parsePtr->optArgv = NULL;
	}
    }
    return code;
}

static void
FreeParseOptions(parsePtr)
    TixClassParseStruct * parsePtr;
{
    if (parsePtr->optArgv) {
	ckfree((char*)parsePtr->optArgv);
    }
    ckfree((char*)parsePtr);
}

/*----------------------------------------------------------------------
 * InitClass --
 *
 * 	Initialize the class record using the arguments supplied by the
 *	tixClass and tixWidgetClass commands.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	The given class is initialized.
 *----------------------------------------------------------------------
 */

static int
InitClass(interp, classRec, cPtr, scPtr, parsePtr)
    Tcl_Interp * interp;
    CONST84 char * classRec;
    TixClassRecord * cPtr;
    TixClassRecord * scPtr;
    TixClassParseStruct * parsePtr;
{
    int code = TCL_OK;
    int i, flag;
    int isWidget = cPtr->isWidget;
    Tix_ListIterator li;
    TixClassRecord * subPtr;

    cPtr->ClassName = tixStrDup(parsePtr->ClassName);

    /*
     * (3) Set up the methods.
     */
    if (SetupMethod(interp, cPtr, parsePtr->method) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /* (4) Set up the major configspecs */
    if (SetupSpec(interp, cPtr, parsePtr->configSpec, isWidget) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /*
     * (5) Set up the aliases
     */

    /* (5.1)Create the alias configSpec's */
    if (parsePtr->alias && *parsePtr->alias) {
	if (SetupAlias(interp, cPtr, parsePtr->alias) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }

    /*
     * We are done with the class record. Now let's put the flags into
     * a hash table so then they can be retrived quickly whenever we call
     * the "$widget config" method
     */

    if (InitHashEntries(interp, cPtr)!=TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /*
     * (5.2) Initialize the alias configSpec's
     */
    for (i=0; i<cPtr->nSpecs; i++) {
	if (cPtr->specs[i]->isAlias) {
	    cPtr->specs[i]->realPtr = 
	      Tix_FindConfigSpecByName(interp, cPtr, cPtr->specs[i]->dbName);
	} 
    }

    /*
     * (6) Set up the attributes of the specs
     */
    if (parsePtr->isStatic  && *parsePtr->isStatic) {
	if (SetupAttribute(interp, cPtr, parsePtr->isStatic, FLAG_STATIC)
		!= TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }
    if (parsePtr->readOnly  && *parsePtr->readOnly) {
	if (SetupAttribute(interp,cPtr,parsePtr->readOnly, FLAG_READONLY)
		!=TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }
    if (parsePtr->forceCall  && *parsePtr->forceCall) {
	if (SetupAttribute(interp,cPtr,parsePtr->forceCall,FLAG_FORCECALL)
		!=TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }

    /* (7) Record the default options */
    if (SetupDefault(interp, cPtr, parsePtr->def) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

#if USE_ACCESS_CONTROL
    /* (8) Set up the SubWidget specs */
    if (isWidget) {
	if (SetupSubWidget(interp, cPtr, parsePtr->subWidget) != TCL_OK) {
	    code = TCL_ERROR;
	    goto done;
	}
    }
#endif

    /*
     * Set up the TCL array variable to store some information about the
     * class. This is compatible with the old Tix and it also speeds up
     * some operations because the look-up of these variables are done
     * by hash tables.
     */

    flag = TCL_GLOBAL_ONLY;
    if (parsePtr->superClass) {
	Tcl_SetVar2(interp, classRec, "superClass", parsePtr->superClass,flag);
    } else {
	Tcl_SetVar2(interp, classRec, "superClass", "", flag);
    }

    Tcl_SetVar2(interp, classRec, "className",     classRec,            flag);
    Tcl_SetVar2(interp, classRec, "ClassName",     parsePtr->ClassName, flag);
    Tcl_SetVar2(interp, classRec, "options",       parsePtr->flag,      flag);
    Tcl_SetVar2(interp, classRec, "forceCall",     parsePtr->forceCall, flag);
    Tcl_SetVar2(interp, classRec, "defaults",      parsePtr->def   ,    flag);
    Tcl_SetVar2(interp, classRec, "methods",       parsePtr->method,    flag);
    Tcl_SetVar2(interp, classRec, "staticOptions", parsePtr->isStatic,  flag);

    if (parsePtr->isVirtual) {
	Tcl_SetVar2(interp, classRec, "virtual",   "1",        flag);
    } else {
	Tcl_SetVar2(interp, classRec, "virtual",   "0",        flag);
    }

    if (isWidget) {
	Tcl_SetVar2(interp, classRec, "isWidget",  "1",        flag);
    } else {
	Tcl_SetVar2(interp, classRec, "isWidget",  "0",        flag);
    }

    /*
     * Now create the instantiation command. Because there is no ::
     * inside the cPtr->className, the command is created in the
     * global namespace.
     */

    if (isWidget) {
	Tcl_CreateCommand(interp, cPtr->className, Tix_CreateWidgetCmd,
		(ClientData)cPtr, NULL);
    } else {
	Tcl_CreateCommand(interp, cPtr->className, Tix_CreateInstanceCmd,
		(ClientData)cPtr, NULL);
    }

    /*
     * Create an "AutoLoad" command. This is needed so that class
     * definitions can be auto-loaded properly
     */

    if (Tix_GlobalVarEval(interp, "proc ", cPtr->className, ":AutoLoad {} {}",
	    (char *) NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    cPtr->initialized = 1;

    /*
     * Complete the initialization of all the partially initialized
     * sub-classes.
     */

    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&cPtr->unInitSubCls, &li);
	     !Tix_SimpleListDone(&li);
	     Tix_SimpleListNext(&cPtr->unInitSubCls, &li)) {

	subPtr = (TixClassRecord*)li.curr;
	code = InitClass(interp, subPtr->className, subPtr, cPtr,
		subPtr->parsePtr);

	if (code == TCL_OK) {
	    if (subPtr->parsePtr) {
		FreeParseOptions(subPtr->parsePtr);
	    }
	    subPtr->parsePtr = NULL;
	    Tix_SimpleListDelete(&cPtr->unInitSubCls, &li);
	} else {
	    /*
	     * (ToDo) Tix is not in a stable state. Some variables
	     * have not been freed.
	     */
	    goto done;
	}
    }

  done:
    return code;
}

/*
 *----------------------------------------------------------------------
 * FreeClassRecord --
 *
 *	Frees the data associated with a class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Tix_InstanceCmd cannot be called afterwards.
 *----------------------------------------------------------------------
 */

static void
FreeClassRecord(cPtr)
    TixClassRecord *cPtr;
{
    int i;
    Tix_ListIterator li;

    if (cPtr->className) {
	ckfree(cPtr->className);
    }
    if (cPtr->ClassName) {
	ckfree(cPtr->ClassName);
    }
    for (i=0; i<cPtr->nSpecs; i++) {
        if (cPtr->specs[i] != NULL) {
            FreeSpec(cPtr->specs[i]);
        }
    }
    if (cPtr->specs) {
	ckfree((char*)cPtr->specs);
    }
    for (i=0; i<cPtr->nMethods; i++) {
	ckfree(cPtr->methods[i]);
    }
    if (cPtr->methods) {
	ckfree((char*)cPtr->methods);
    }

    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&cPtr->unInitSubCls, &li);
	     !Tix_SimpleListDone(&li);
	     Tix_SimpleListNext(&cPtr->unInitSubCls, &li)) {
	Tix_SimpleListDelete(&cPtr->unInitSubCls, &li);
    }
    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&cPtr->subWDefs, &li);
	     !Tix_SimpleListDone(&li);
	     Tix_SimpleListNext(&cPtr->subWDefs, &li)) {

	Tix_SubwidgetDef * defPtr = (Tix_SubwidgetDef*)li.curr;
	Tix_SimpleListDelete(&cPtr->subWDefs, &li);

	ckfree((char*)defPtr->spec);
	ckfree((char*)defPtr->value);
	ckfree((char*)defPtr);
    }

    if (cPtr->parsePtr) {
	FreeParseOptions(cPtr->parsePtr);
    }

    ckfree((char*)cPtr);
}

TIX_DEFINE_CMD(Tix_UninitializedClassCmd)
{
    TixClassRecord * cPtr, *scPtr;

    cPtr = (TixClassRecord *)clientData;
    for (scPtr = cPtr->superClass; scPtr != NULL && scPtr->superClass != NULL;
	    scPtr = scPtr->superClass) {
	;
    }
    if (scPtr != NULL) {
	Tcl_AppendResult(interp, "Superclass \"", scPtr->className,
	        "\" not defined", NULL);
    } else {
	Tcl_AppendResult(interp, "Unknown Tix internal error", NULL);
    }

    return TCL_ERROR;
}


/*----------------------------------------------------------------------
 * Tix_CreateInstanceCmd --
 *
 * 	Create an instance object of a normal Tix class.
 *
 * argv[0]  = object name.
 * argv[1+] = args 
 *----------------------------------------------------------------------
 */

TIX_DEFINE_CMD(Tix_CreateInstanceCmd)
{
    TixClassRecord * cPtr;
    CONST84 char * widRec;
    CONST84 char * value;
    int i, code = TCL_OK;
    TixConfigSpec * spec;

    if (argc <= 1) {
	return Tix_ArgcError(interp, argc, argv, 1, "name ?arg? ...");
    }

    if (strstr(argv[1], "::") != NULL) {
        /*
         * Cannot contain :: in instance name, otherwise all hell will
         * rise w.r.t. namespace
         */

        Tcl_AppendResult(interp, "invalid instance name \"", argv[1],
		"\": may not contain substring \"::\"", NULL);
        return TCL_ERROR;
    }

    cPtr = (TixClassRecord *)clientData;
    widRec = argv[1];

    Tcl_SetVar2(interp, widRec, "className", cPtr->className, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "ClassName", cPtr->ClassName, TCL_GLOBAL_ONLY);
    Tcl_SetVar2(interp, widRec, "context",   cPtr->className, TCL_GLOBAL_ONLY);

    /*
     * This is the command that access the class instace. Because there
     * is no :: inside widRec, it's created in the global namespace
     */

    Tcl_CreateCommand(interp, widRec, Tix_InstanceCmd,
	(ClientData)cPtr, NULL);

    /*
     * Set up the widget record according to defaults and arguments
     */

    ParseInstanceOptions(interp, cPtr, widRec, argc-2, argv+2);

    /*
     * Call the constructor method
     */
    if (Tix_CallMethod(interp, cPtr->className, widRec, "Constructor",
		0, 0, NULL) != TCL_OK) {
	code = TCL_ERROR;
	goto done;
    }

    /*
     * %% warning. configuration methods for -forcecall options must
     * *not* assume that the value in the widget record has been
     * validated!
     *
     * todo: please explain the above in the programming guide.
     */

    for (i=0; i<cPtr->nSpecs; i++) {
	spec = cPtr->specs[i];
	if (spec->forceCall) {
            value = Tcl_GetVar2(interp, widRec, spec->argvName,
	            TCL_GLOBAL_ONLY);
            if (Tix_CallConfigMethod(interp, cPtr, widRec, spec, value)
                    !=TCL_OK){
                code = TCL_ERROR;
                goto done;
            }
	}
    }

    Tcl_SetResult(interp, (char *) widRec, TCL_VOLATILE);

  done:
    return code;
}

/*----------------------------------------------------------------------
 * Tix_InstanceCmd
 *
 * 	Redirect the method calls to the class methods
 *
 * argv[0]  = widget name
 * argv[1]  = method name
 * argv[2+] = arglist
 */
TIX_DEFINE_CMD(Tix_InstanceCmd)
{
    TixClassRecord * cPtr;
    CONST84 char * widRec = argv[0];
    CONST84 char * method = argv[1];
    CONST84 char * classRec;
    CONST84 char * methodName;	/* full name of the method -- method may be
				 * abbreviated */
    unsigned int len;
    int code;
    int foundMethod;

    cPtr = (TixClassRecord *)clientData;
    classRec = cPtr->className;
    
    if (argc <= 1) {
	return Tix_ArgcError(interp, argc, argv, 1, "option ...");
    }

    Tk_Preserve((ClientData) cPtr);

    len = strlen(method);

    if ((methodName = Tix_FindPublicMethod(interp, cPtr, method)) == NULL) {
	code = Tix_UnknownPublicMethodError(interp, cPtr, widRec, method);
	goto done;
    }

    if ((code = Tix_CallMethod(interp, classRec, widRec, methodName,
	    argc-2, argv+2, &foundMethod)) == TCL_OK) {
	goto done;
    }

    if (foundMethod) {
        /*
         * The method is found, but its execution caused error. Return now.
         * Don't try to run it again with the code below.
         */
        goto done;
    }

    /*
     * The method was not defined in Tcl code. See if it's one of
     * the built-in methods: configure, cget or subwidget.
     *
     * Note that the "subwidgets" method is implemented in Tcl code
     * (Primitiv.tcl) so we don't need to worry about it here.
     */

    if (strncmp(method, "configure", len) == 0) {
	Tcl_ResetResult(interp);

	if (argc==2) {
	    code = Tix_QueryAllOptions(interp, cPtr, widRec);
	    goto done;
	}
	else if (argc == 3) {
	    code = Tix_QueryOneOption(interp, cPtr, widRec, argv[2]);
	    goto done;
	} else {
	    code = Tix_ChangeOptions(interp, cPtr, widRec, argc-2, argv+2);
	    goto done;
	}
    }
    else if (strncmp(method, "cget", len) == 0) {
	Tcl_ResetResult(interp);

	if (argc == 3) {
	    code = Tix_GetVar(interp, cPtr, widRec, argv[2]);
	    goto done;
	} else {
	    code = Tix_ArgcError(interp, argc, argv, 2, "-flag");
	    goto done;
	}
    }
#if 0
    else if (cPtr->isWidget && strcmp(method, "subwidgets") == 0) {
	Tcl_ResetResult(interp);

	code = Tix_CallMethod(interp, classRec, widRec, "subwidgets",
	        argc-2, argv+2, NULL);
	goto done;
    }
#endif
    else if (cPtr->isWidget && strncmp(method, "subwidget", len) == 0) {
        /*
         * TODO: Subwidget protection is not yet implemented
         */

	Tcl_ResetResult(interp);
	if (argc >= 3) {
#define STATIC_SPACE_SIZE 60
	    char buff[STATIC_SPACE_SIZE];
	    char *index = buff;
            CONST84 char *swName;

            if ((strlen(argv[2]) + 3) > STATIC_SPACE_SIZE) {
                index = (char*)ckalloc(strlen(argv[2]) + 3);
            }

	    sprintf(index, "w:%s", argv[2]);
	    swName = Tcl_GetVar2(interp, widRec, index, TCL_GLOBAL_ONLY);
            if (index != buff) {
                ckfree((char*)index);
            }

	    if (swName) {
		if (argc == 3) {
		    Tcl_SetResult(interp, (char *) swName, TCL_VOLATILE);
		    code = TCL_OK;
		    goto done;
		} else {
		    argv[2] = swName;
		    code = Tix_EvalArgv(interp, argc-2, argv+2);
		    goto done;
		}
	    }
	    Tcl_AppendResult(interp, "unknown subwidget \"", argv[2],
		"\"", NULL);
	    code = TCL_ERROR;
	    goto done;
#undef STATIC_SPACE_SIZE
	} else {
	    code = Tix_ArgcError(interp, argc, argv, 2, "name ?args ...?");
	    goto done;
	}
    } else {
	/*
	 * error message already append by Tix_CallMethod()
	 */
	code = TCL_ERROR;
	goto done;
    }

  done:
    Tk_Release((ClientData) cPtr);
    return code;
}

/*----------------------------------------------------------------------
 * Subroutines for Class definition
 *
 *
 *----------------------------------------------------------------------
 */
static int SetupMethod(interp, cPtr, s)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
{
    TixClassRecord * scPtr = cPtr->superClass;
    CONST84 char ** listArgv;
    int listArgc, i;
    int nMethods;


    if (s && *s) {
	if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	listArgc = 0;
	listArgv = 0;
    }

    nMethods = listArgc;

    if (scPtr) {
	nMethods += scPtr->nMethods;
    }
    cPtr->nMethods = nMethods;
    cPtr->methods  = (char**)Tix_ZAlloc(nMethods*sizeof(char*));
    /* Copy the methods of this class */
    for (i=0; i<listArgc; i++) {
	cPtr->methods[i] = tixStrDup(listArgv[i]);
    }
    /* Copy the methods of the super class */
    for (; i<nMethods; i++) {
	cPtr->methods[i] = tixStrDup(scPtr->methods[i-listArgc]);
    }

    if (listArgv) {
	ckfree((char*)listArgv);
    }

    return TCL_OK;
}

static int
SetupDefault(interp, cPtr, s)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
{
    CONST84 char ** listArgv;
    int listArgc, i;
    TixClassRecord * scPtr = cPtr->superClass;
    Tix_ListIterator li;
    Tix_SubwidgetDef *defPtr;
    Tcl_Obj *objv[5];

    if (s && *s) {
	if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	return TCL_OK;
    }

    if (scPtr) {
	/*
	 * Copy the subwidget default specs from the super-class
	 */
	Tix_SimpleListIteratorInit(&li);
	for (Tix_SimpleListStart(&scPtr->subWDefs, &li);
	     !Tix_SimpleListDone(&li);
	     Tix_SimpleListNext (&scPtr->subWDefs, &li)) {

	    Tix_SubwidgetDef * p = (Tix_SubwidgetDef*)li.curr;

	    defPtr = (Tix_SubwidgetDef*)Tix_ZAlloc(sizeof(Tix_SubwidgetDef));
	    defPtr->spec  = tixStrDup(p->spec);
	    defPtr->value = tixStrDup(p->value);

	    Tix_SimpleListAppend(&cPtr->subWDefs, (char*)defPtr, 0);
	}
    }

    /*
     * Merge with the new default specs
     */
    for (i=0; i<listArgc; i++) {
	CONST84 char **list;
	int n;

	if (Tcl_SplitList(interp, listArgv[i], &n, &list) != TCL_OK) {
	    goto error;
	}
	if (n != 2) {
	    Tcl_AppendResult(interp, "bad subwidget default format \"",
		listArgv[i], "\"", NULL);
	    ckfree((char*)list);
	    goto error;
	}
	
	Tix_SimpleListIteratorInit(&li);
	for (Tix_SimpleListStart(&cPtr->subWDefs, &li);
	     !Tix_SimpleListDone(&li);
	     Tix_SimpleListNext (&cPtr->subWDefs, &li)) {

	    Tix_SubwidgetDef * p = (Tix_SubwidgetDef*)li.curr;

	    if (strcmp(list[0], p->spec) == 0) {
		Tix_SimpleListDelete(&cPtr->subWDefs, &li);
		ckfree((char*)p->value);
		ckfree((char*)p->spec);
		ckfree((char*)p);
		break;
	    }
	}
	/* Append this spec to the end
	 */
	defPtr = (Tix_SubwidgetDef*)Tix_ZAlloc(sizeof(Tix_SubwidgetDef));
	defPtr->spec  = tixStrDup(list[0]);
	defPtr->value = tixStrDup(list[1]);

	Tix_SimpleListAppend(&cPtr->subWDefs, (char*)defPtr, 0);

	ckfree((char*)list);
    }

    /*
     * Add the defaults into the options database.
     * Be efficient with objects, reusing the middle 2.
     */

    objv[0] = Tcl_NewStringObj("option", -1); Tcl_IncrRefCount(objv[0]);
    objv[1] = Tcl_NewStringObj("add", -1); Tcl_IncrRefCount(objv[1]);
    objv[4] = Tcl_NewStringObj("widgetDefault", -1); Tcl_IncrRefCount(objv[4]);
    Tix_SimpleListIteratorInit(&li);
    for (Tix_SimpleListStart(&cPtr->subWDefs, &li);
	 !Tix_SimpleListDone(&li);
	 Tix_SimpleListNext (&cPtr->subWDefs, &li)) {
	Tix_SubwidgetDef * p = (Tix_SubwidgetDef*)li.curr;

	objv[2] = Tcl_NewStringObj("*", -1);
	Tcl_AppendStringsToObj(objv[2], cPtr->ClassName, p->spec, NULL);
	objv[3] = Tcl_NewStringObj(p->value, -1);
	Tcl_IncrRefCount(objv[2]);
	Tcl_IncrRefCount(objv[3]);

	if (Tcl_EvalObjv(interp, 5, objv, TCL_EVAL_GLOBAL) != TCL_OK) {
	    for (i = 0; i < 5; i++) { Tcl_DecrRefCount(objv[i]); }
	    goto error;
	}
	Tcl_DecrRefCount(objv[2]);
	Tcl_DecrRefCount(objv[3]);
    }
    Tcl_DecrRefCount(objv[0]);
    Tcl_DecrRefCount(objv[1]);
    Tcl_DecrRefCount(objv[4]);

    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_OK;

  error:
   if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_ERROR;
}

static int
SetupSpec(interp, cPtr, s, isWidget)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
    int isWidget;
{
    TixClassRecord * scPtr = cPtr->superClass;
    CONST84 char ** listArgv;
    int listArgc, i;
    TixConfigSpec * dupSpec;
    int nSpecs;
    int j;
    int nAlloc;
    int code = TCL_OK;

    if (s && *s) {
	if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	listArgc = 0;
	listArgv = 0;
    }

    nSpecs = listArgc;

    if (scPtr != NULL) {
	nAlloc = nSpecs+scPtr->nSpecs;
    } else {
	nAlloc = nSpecs;
    }

    cPtr->nSpecs = nSpecs;
    cPtr->specs  = (TixConfigSpec**)Tix_ZAlloc(nAlloc*sizeof(TixConfigSpec*));

    /*
     * Initialize the specs of this class
     */
    for (i=0; i<listArgc; i++) {
	if ((cPtr->specs[i] = InitSpec(interp, listArgv[i], isWidget))==NULL){
	    code = TCL_ERROR;
	    goto done;
	}
    }
    /*
     * Copy the specs of the super class
     */
    if (scPtr != NULL) {
	for (i=0; i<scPtr->nSpecs; i++) {
	    /* See if we have re-defined this configspec */
	    for (dupSpec = 0, j=0; j<listArgc; j++) {
		char * pName = scPtr->specs[i]->argvName;
		if (strcmp(cPtr->specs[j]->argvName, pName)==0) {
		    dupSpec = cPtr->specs[j];
		    break;
		}
	    }

	    if (dupSpec) {
		/*
		 * If we have not redefined the dbclass or dbname of
		 * this duplicated configSpec, then simply
		 * copy the parent's attributes to the new configSpec
		 *
		 * Otherwise we don't copy the parent's attributes (do nothing)
		 */
		if ((strcmp(dupSpec->dbClass, scPtr->specs[i]->dbClass) == 0)
		    &&(strcmp(dupSpec->dbName, scPtr->specs[i]->dbName) == 0)){
		    dupSpec->readOnly  = scPtr->specs[i]->readOnly;
		    dupSpec->isStatic  = scPtr->specs[i]->isStatic;
		    dupSpec->forceCall = scPtr->specs[i]->forceCall;
		}
	    } else {
		/* 
		 *Let's copy the parent's configSpec
		 */
		cPtr->specs[cPtr->nSpecs] = CopySpec(scPtr->specs[i]);
		cPtr->nSpecs ++;
	    }
	}
    }

    if (cPtr->nSpecs != nAlloc) {
	cPtr->specs = (TixConfigSpec**)
	  ckrealloc((char*)cPtr->specs, cPtr->nSpecs*sizeof(TixConfigSpec*));
    }

  done:
    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return code;
}

static TixConfigSpec *
InitSpec(interp, specList, isWidget)
    Tcl_Interp * interp;
    CONST84 char * specList;
    int isWidget;
{
    CONST84 char ** listArgv = NULL;
    int listArgc;
    TixConfigSpec * sPtr = NULL;
    
    if (Tcl_SplitList(interp, specList, &listArgc, &listArgv)!= TCL_OK) {
	goto done;
    }
    if (( isWidget && (listArgc < 4 || listArgc > 5)) ||
	(!isWidget && (listArgc < 2 || listArgc > 3))) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "Wrong number of elements in ",
	    "config spec list \"", specList, "\"", NULL);
	goto done;
    }

    sPtr = (TixConfigSpec * )Tix_ZAlloc(sizeof(TixConfigSpec));

    sPtr->isAlias   = 0;
    sPtr->readOnly  = 0;
    sPtr->isStatic  = 0;
    sPtr->forceCall = 0;
    sPtr->realPtr   = NULL;

    if (isWidget) {
	sPtr->argvName = tixStrDup(listArgv[0]);
	sPtr->dbName   = tixStrDup(listArgv[1]);
	sPtr->dbClass  = tixStrDup(listArgv[2]);
	sPtr->defValue = tixStrDup(listArgv[3]);
    }
    else {
	sPtr->argvName = tixStrDup(listArgv[0]);
	sPtr->dbClass  = TIX_EMPTY_STRING;
	sPtr->dbName   = TIX_EMPTY_STRING;
	sPtr->defValue = tixStrDup(listArgv[1]);
    }

    /* Set up the verifyCmd */
    if ((isWidget && listArgc == 5) || (!isWidget && listArgc == 3)) {
	int n;

	if (isWidget) {
	    n = 4;
	} else {
	    n = 2;
	}

	sPtr->verifyCmd = tixStrDup(listArgv[n]);
    } else {
	sPtr->verifyCmd = NULL;
    }

  done:
    if (listArgv) {
	ckfree((char *) listArgv);
    }
    return sPtr;
}

static TixConfigSpec *
CopySpec(sPtr)
    TixConfigSpec *sPtr;	/* The spec record from the super class. */
{
    TixConfigSpec *nPtr = (TixConfigSpec *)Tix_ZAlloc(sizeof(TixConfigSpec));

    nPtr->isAlias   = sPtr->isAlias;
    nPtr->readOnly  = sPtr->readOnly;
    nPtr->isStatic  = sPtr->isStatic;
    nPtr->forceCall = sPtr->forceCall;

    if (sPtr->argvName != NULL &&  sPtr->argvName != TIX_EMPTY_STRING) {
	nPtr->argvName = tixStrDup(sPtr->argvName);
    } else {
	nPtr->argvName = TIX_EMPTY_STRING;
    }
    if (sPtr->defValue != NULL &&  sPtr->defValue != TIX_EMPTY_STRING) {
	nPtr->defValue = tixStrDup(sPtr->defValue);
    } else {
	nPtr->defValue = TIX_EMPTY_STRING;
    }
    if (sPtr->dbName != NULL &&  sPtr->dbName != TIX_EMPTY_STRING) {
	nPtr->dbName = tixStrDup(sPtr->dbName);
    } else {
	nPtr->dbName = TIX_EMPTY_STRING;
    }
    if (sPtr->dbClass != NULL &&  sPtr->dbClass != TIX_EMPTY_STRING) {
	nPtr->dbClass = tixStrDup(sPtr->dbClass);
    } else {
	nPtr->dbClass = TIX_EMPTY_STRING;
    }
    if (sPtr->verifyCmd != NULL) {
	nPtr->verifyCmd = tixStrDup(sPtr->verifyCmd);
    } else {
	nPtr->verifyCmd = NULL;
    }

    nPtr->realPtr = NULL;

    return nPtr;
}

static void
FreeSpec(sPtr)
    TixConfigSpec *sPtr;	/* The spec record to free. */
{
    if (sPtr->argvName != NULL && sPtr->argvName != TIX_EMPTY_STRING) {
	ckfree(sPtr->argvName);
    }
    if (sPtr->defValue != NULL && sPtr->defValue != TIX_EMPTY_STRING) {
	ckfree(sPtr->defValue);
    }
    if (sPtr->dbName   != NULL && sPtr->dbName   != TIX_EMPTY_STRING) {
	ckfree(sPtr->dbName);
    }
    if (sPtr->dbClass  != NULL && sPtr->dbClass  != TIX_EMPTY_STRING) {
	ckfree(sPtr->dbClass);
    }
    if (sPtr->verifyCmd != NULL) {
	ckfree(sPtr->verifyCmd);
    }
    ckfree((char*)sPtr);
}

/*
 *----------------------------------------------------------------------
 * SetupAttribute --
 *
 *	Marks the spec's with the given attribute (-readonly, -forcecall,
 *	and -static).
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	The attributes of the specs are updated.
 *----------------------------------------------------------------------
 */

static int
SetupAttribute(interp, cPtr, s, which)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
    int which;
{
    CONST84 char ** listArgv;
    int listArgc, i;
    TixConfigSpec  * spec;

    if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    } else {
	for (i=0; i<listArgc; i++) {
	    spec = Tix_FindConfigSpecByName(interp, cPtr, listArgv[i]);
	    if (spec == NULL) {
		ckfree((char*)listArgv);
		return TCL_ERROR;
	    }
	    switch(which) {
	      case FLAG_READONLY:
		spec->readOnly = 1;
		break;
	      case FLAG_STATIC:
		spec->isStatic = 1;
		break;
	      case FLAG_FORCECALL:
		spec->forceCall = 1;
		break;
	    }
	}
    }

    ckfree((char*)listArgv);
    return TCL_OK;
}

static int
SetupAlias(interp, cPtr, s)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
{
    CONST84 char ** listArgv;
    int listArgc, i;

    if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    } else {
	int nAliases = listArgc;
	int nAlloc = cPtr->nSpecs + nAliases;

	cPtr->specs = (TixConfigSpec**)
	    ckrealloc((char*)cPtr->specs, nAlloc*sizeof(TixConfigSpec*));

	/* Initialize the aliases of this class */
	for (i=cPtr->nSpecs; i<nAlloc; i++) {
	    cPtr->specs[i] = InitAlias(interp, cPtr, listArgv[i-cPtr->nSpecs]);
	    if (cPtr->specs[i] == NULL) {
		ckfree((char*)listArgv);
		return TCL_ERROR;
	    }
	}

	cPtr->nSpecs = nAlloc;
    }
    ckfree((char*)listArgv);
    return TCL_OK;
}

static TixConfigSpec *
InitAlias(interp, cPtr, s)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
{
    CONST84 char ** listArgv;
    int listArgc;
    TixConfigSpec  * sPtr;

    if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK
	    || 2 != listArgc) {
	return NULL;
    } else {
	sPtr = (TixConfigSpec*) Tix_ZAlloc(sizeof(TixConfigSpec));
	sPtr->isAlias    = 1;
	sPtr->isStatic   = 0;
	sPtr->forceCall  = 0;
	sPtr->readOnly   = 0;
	sPtr->argvName   = tixStrDup(listArgv[0]);
	sPtr->dbName     = tixStrDup(listArgv[1]);
	sPtr->dbClass    = TIX_EMPTY_STRING;
	sPtr->defValue   = TIX_EMPTY_STRING;
	sPtr->verifyCmd  = NULL;
	sPtr->realPtr    = NULL;

	ckfree((char*)listArgv);
	return sPtr;
    }
}

static int
InitHashEntries(interp, cPtr)
    Tcl_Interp * interp;
    TixClassRecord *cPtr;
{
    Tcl_HashEntry * hashPtr;
    int    	    isNew;
    CONST84 char  * key;
    int		    i;
    TixConfigSpec * sPtr;

    for (i=0; i<cPtr->nSpecs; i++) {
	sPtr = cPtr->specs[i];
	key = Tix_GetConfigSpecFullName(cPtr->className, sPtr->argvName);

	hashPtr = Tcl_CreateHashEntry(GetSpecTable(interp), key, &isNew);
	Tcl_SetHashValue(hashPtr, (char*)sPtr);

	ckfree((char *) key);
    }

    return TCL_OK;
}
/*----------------------------------------------------------------------
 * Subroutines for object instantiation.
 *
 *
 *----------------------------------------------------------------------
 */
static int
ParseInstanceOptions(interp, cPtr, widRec, argc, argv)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char *widRec;
    int argc;
    CONST84 char** argv;
{
    int i;
    TixConfigSpec *spec;

    if ((argc %2) != 0) {
	Tcl_AppendResult(interp, "missing argument for \"", argv[argc-1],
	    "\"", NULL);
	return TCL_ERROR;
    }

    /* Set all specs by their default values */
    for (i=0; i<cPtr->nSpecs; i++) {
	spec = cPtr->specs[i];
	if (!spec->isAlias) {
	    if (Tix_ChangeOneOption(interp, cPtr, widRec, spec,
		spec->defValue, 1, 0)!=TCL_OK) {
		return TCL_ERROR;
	    }
	}
    }

    /* Set specs according to argument line values */
    for (i=0; i<argc; i+=2) {
	spec = Tix_FindConfigSpecByName(interp, cPtr, argv[i]);

	if (spec == NULL) {	/* this is an invalid flag */
	    return TCL_ERROR;
	}

	if (Tix_ChangeOneOption(interp, cPtr, widRec, spec,
		argv[i+1], 0, 1)!=TCL_OK) {
	    return TCL_ERROR;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 * ClassTableDeleteProc --
 *
 *	This procedure is called when the interp is about to be
 *	deleted. It cleans up the hash entries and destroys the hash
 *	table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All class definitions are deleted.
 *----------------------------------------------------------------------
 */

static void
ClassTableDeleteProc(clientData, interp)
    ClientData clientData;
    Tcl_Interp *interp;
{
    Tcl_HashTable * classTablePtr = (Tcl_HashTable*)clientData;
    Tcl_HashSearch hashSearch;
    Tcl_HashEntry * hashPtr;
    TixClassRecord * cPtr;

    for (hashPtr = Tcl_FirstHashEntry(classTablePtr, &hashSearch);
	    hashPtr;
	    hashPtr = Tcl_NextHashEntry(&hashSearch)) {
	cPtr = (TixClassRecord*)Tcl_GetHashValue(hashPtr);
	FreeClassRecord(cPtr);
	Tcl_DeleteHashEntry(hashPtr);
    }
    Tcl_DeleteHashTable(classTablePtr);
    ckfree((char*)classTablePtr);
}

#if USE_ACCESS_CONTROL

/*
 * Everything after this line are not used at this moment.
 *
 */

/*----------------------------------------------------------------------
 *
 *
 *			ACCESS CONTROL
 *
 *
 *----------------------------------------------------------------------
 */
static void InitExportSpec(exPtr)
    Tix_ExportSpec * exPtr;
{
    Tix_SimpleListInit(&exPtr->exportCmds);
    Tix_SimpleListInit(&exPtr->restrictCmds);
    Tix_SimpleListInit(&exPtr->exportOpts);
    Tix_SimpleListInit(&exPtr->restrictOpts);
}

static Tix_LinkList * CopyStringList(list, newList)
    Tix_LinkList * list;
    Tix_LinkList * newList;
{
    Tix_StringLink * ptr, * newLink;

    for (ptr=(Tix_StringLink*)list->head; ptr; ptr=ptr->next) {
	newLink = (Tix_StringLink*)Tix_ZAlloc(sizeof(Tix_StringLink));

	newLink->string = tixStrDup(ptr->string);
	Tix_SimpleListAppend(newList, (char*)newLink, 0);
    }
}

static void
CopyExportSpec(src, dst)
    Tix_ExportSpec * src;
    Tix_ExportSpec * dst;
{
    CopyStringList(&src->exportCmds,   &dst->exportCmds);
    CopyStringList(&src->restrictCmds, &dst->restrictCmds);

    CopyStringList(&src->exportOpts,   &dst->exportOpts);
    CopyStringList(&src->restrictOpts, &dst->restrictOpts);
}

/*
 * (1) All items that appear in list1 must not appear in list 2
 * (2) If either list have the item "all" -- an item whose string pointer is
 *     NULL -- the other list must be empty.
 */
static int CheckMutualExclusion(list1, list2)
    Tix_LinkList * list1;
    Tix_LinkList * list2;
{
    Tix_StringLink * ptr, *ptr2;

    if (list1->numItems == 0) {
	return TCL_OK;
    }
    if (list2->numItems == 0) {
	return TCL_OK;
    }

    for (ptr=(Tix_StringLink *)(list1->head); ptr; ptr=ptr->next) {
	if (ptr->string == NULL) {
	    goto error;
	}

	for (ptr2=(Tix_StringLink *)(list2->head); ptr2; ptr2=ptr2->next) {
	    if (strcmp(ptr->string, ptr2->string) == 0) {

		/* Violates requirement (1) above :
		 * Some items in list 1 also appear in list 2.
		 */
		goto error;
	    }
	}
    }

    return TCL_OK;

  error:

    return TCL_ERROR;
}

static int AppendStrings(interp, list, string)
    Tcl_Interp * interp;
    Tix_LinkList * list;
    char * string;
{
    char ** listArgv = NULL;
    int listArgc, i;
    Tix_StringLink * ptr;

    if (string && *string) {
	if (Tcl_SplitList(interp, string, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	/* Nothing to be done */
	return TCL_OK;
    }

    for (i=0; i<listArgc; i++) {
	ptr = (Tix_StringLink *)Tix_ZAlloc(sizeof(Tix_StringLink));

	if (strcmp(listArgv[i], "all")==0) {
	    ptr->string = NULL;
	} else {
	    ptr->string = tixStrDup(listArgv[i]);
	}
	Tix_SimpleListAppend(list, (char*)ptr, 0);
    }

    if (listArgv) {
	ckfree((char*)listArgv);
    }

    return TCL_OK;
}


static int ConflictingSpec(interp, which, eList, rList)
    Tcl_Interp * interp;
    char * which;
    Tix_LinkList * eList;
    Tix_LinkList * rList;
{
    Tix_LinkList *lists[2];
    Tix_StringLink * ptr;
    int i;
    char * specs[2] = {"export :", "restrict :"};

    lists[0] = eList;
    lists[1] = rList;

    Tcl_ResetResult(interp);
    Tcl_AppendResult(interp, "conflicting export and restrictions ",
	"for ", which, "\n", NULL);

    for (i=0; i<2; i++) {
	Tcl_AppendResult(interp, specs[i], NULL);

	for (ptr=(Tix_StringLink *)(lists[i]->head); ptr; ptr=ptr->next) {
	    if (ptr->string == 0) {
		Tcl_AppendResult(interp, "all ", NULL);
	    } else {
		Tcl_AppendResult(interp, ptr->string, " ", NULL);
	    }
	}
	Tcl_AppendResult(interp, "\n", NULL);
    }

    return TCL_ERROR;
}

/*
 * Define or redefine the export control
 */
static int DefineExport(interp, exPtr, name, spec)
    Tcl_Interp * interp;
    Tix_ExportSpec * exPtr;
    char * name;
    char * spec;
{
    char ** listArgv = NULL;
    int listArgc, i;

    if (spec && *spec) {
	if (Tcl_SplitList(interp, spec, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	/* Nothing to be done */
	return TCL_OK;
    }

    if (listArgc %2 != 0) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "wrong # of argument in subwidget spec: \"",
	    spec, "\"", NULL);
	goto error;
    }

    for (i=0; i<listArgc; i+=2) {
	if (strcmp(listArgv[i], "-exportcmd") == 0) {
	    if (AppendStrings(interp, &exPtr->exportCmds, 
		listArgv[i+1])==TCL_ERROR) {
		goto error;
	    }
	}
	else if (strcmp(listArgv[i], "-restrictcmd") == 0) {
	    if (AppendStrings(interp, &exPtr->restrictCmds, 
		listArgv[i+1])==TCL_ERROR){
		goto error;
	    }
	}
	else if (strcmp(listArgv[i], "-exportopt") == 0) {
	    if (AppendStrings(interp, &exPtr->exportOpts,  
		listArgv[i+1])==TCL_ERROR) {
		goto error;
	    }
	}
	else if (strcmp(listArgv[i], "-restrictopt") == 0) {
	    if (AppendStrings(interp, &exPtr->restrictOpts, 
		listArgv[i+1])==TCL_ERROR){
		goto error;
	    }
	}
    }

    if (CheckMutualExclusion(&exPtr->exportCmds, &exPtr->restrictCmds)
	==TCL_ERROR) {
	ConflictingSpec(interp, "commands",
	    &exPtr->exportCmds, &exPtr->restrictCmds);
	goto error;
    }
    if (CheckMutualExclusion(&exPtr->restrictCmds, &exPtr->exportCmds)
	==TCL_ERROR) {
	ConflictingSpec(interp, "commands",
	     &exPtr->exportCmds, &exPtr->restrictCmds);
	goto error;
    }
    if (CheckMutualExclusion(&exPtr->exportOpts, &exPtr->restrictOpts)
	==TCL_ERROR) {
	ConflictingSpec(interp, "options",
	     &exPtr->exportOpts, &exPtr->restrictOpts);
	goto error;
    }
    if (CheckMutualExclusion(&exPtr->restrictOpts, &exPtr->exportOpts)
	==TCL_ERROR) {
	ConflictingSpec(interp, "options",
	     &exPtr->exportOpts, &exPtr->restrictOpts);
	goto error;
    }

  done:
    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_OK;

  error:
    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_ERROR;
}
/*----------------------------------------------------------------------
 *
 *
 *			SUBWIDGET SETUP
 *
 *
 *----------------------------------------------------------------------
 */
static Tix_SubWidgetSpec *
GetSubWidgetSpec(cPtr, name)
    TixClassRecord * cPtr;
    CONST84 char * name;
{
    Tix_SubWidgetSpec *ptr;

    for (ptr=(Tix_SubWidgetSpec *)cPtr->subWidgets.head; ptr; ptr=ptr->next) {
	if (strcmp(ptr->name, name) == 0) {
	    return ptr;
	}
    }

    return NULL;
}

static Tix_SubWidgetSpec *
AllocSubWidgetSpec()
{
    Tix_SubWidgetSpec * newPtr = 
            (Tix_SubWidgetSpec *)Tix_ZAlloc(sizeof(Tix_SubWidgetSpec));

    newPtr->next 	= NULL;
    newPtr->name 	= NULL;
    InitExportSpec(&newPtr->exportSpec);
    return newPtr;
}

static void
CopySubWidgetSpecs(scPtr, cPtr)
    TixClassRecord * scPtr;
    TixClassRecord * cPtr;
{
    Tix_SubWidgetSpec *ssPtr;

    for (ssPtr=(Tix_SubWidgetSpec *)scPtr->subWidgets.head;
	ssPtr;
	ssPtr=ssPtr->next) {

	Tix_SubWidgetSpec *newPtr;
	newPtr = AllocSubWidgetSpec();

	newPtr->name = tixStrDup(ssPtr->name);
	CopyExportSpec(&ssPtr->exportSpec, & newPtr->exportSpec);

	Tix_SimpleListAppend(&cPtr->subWidgets, (char*)newPtr, 0);
    }
}

static int
SetupSubWidget(interp, cPtr, s)
    Tcl_Interp * interp;
    TixClassRecord * cPtr;
    CONST84 char * s;
{
    char ** listArgv;
    TixClassRecord * scPtr = cPtr->superClass;
    int listArgc, i;

    if (s && *s) {
	if (Tcl_SplitList(interp, s, &listArgc, &listArgv) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
	return TCL_OK;
    }

    if (listArgc %2 != 0) {
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, "wrong # of argument in subwidget spec: \"",
	    s, "\"", NULL);
	goto error;
    }

    /* Copy all the subwidgets of the superclass to this class */
    if (scPtr) {
	CopySubWidgetSpecs(scPtr, cPtr);
    }

    /* Iterate over all the newly defined or re-defined subwidgets */
    for (i=0; i<listArgc; i+=2) {
	char * name, *spec;
	Tix_SubWidgetSpec * oldSpec;
	Tix_SubWidgetSpec * newSpec;

	name = listArgv[i];
	spec = listArgv[i+1];

	if (scPtr && ((oldSpec = GetSubWidgetSpec(cPtr, name)) != NULL)) {
	    if (DefineExport(interp, &oldSpec->exportSpec, name, spec)
		    != TCL_OK) {
		goto error;
	    }
	}
	else {
	    newSpec = AllocSubWidgetSpec();
	    newSpec->name = tixStrDup(name);

	    Tix_SimpleListAppend(&cPtr->subWidgets, (char*)newSpec, 0);

	    if (DefineExport(interp, &newSpec->exportSpec, name, spec) 
		    != TCL_OK) {
		goto error;
	    }
	}
    }

    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_OK;


  error:
    if (listArgv) {
	ckfree((char*)listArgv);
    }
    return TCL_ERROR;
}

#endif

