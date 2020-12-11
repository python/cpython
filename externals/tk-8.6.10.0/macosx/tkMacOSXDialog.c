/*
 * tkMacOSXDialog.c --
 *
 *	Contains the Mac implementation of the common dialog boxes.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2006-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2017 Christian Gollwitzer.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"
#include "tkFileFilter.h"
#include "tkMacOSXConstants.h"

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1090
#define modalOK     NSOKButton
#define modalCancel NSCancelButton
#else
#define modalOK     NSModalResponseOK
#define modalCancel NSModalResponseCancel
#endif // MAC_OS_X_VERSION_MIN_REQUIRED < 1090
#define modalOther  -1
#define modalError  -2

/*
 * Vars for filtering in "open file" and "save file" dialogs.
 */

typedef struct {
    bool doFileTypes;			/* Show the accessory view which
					 * displays the filter menu */
    bool preselectFilter;		/* A filter was selected by the
					 * typevariable. */
    bool userHasSelectedFilter;		/* The user has changed the filter in
					 * the accessory view. */
    NSMutableArray *fileTypeNames;	/* Array of names, e.g. "Text
					 * document". */
    NSMutableArray *fileTypeExtensions;	/* Array of allowed extensions per
					 * name, e.g. "txt", "doc". */
    NSMutableArray *fileTypeLabels;	/* Displayed string, e.g. "Text
					 * document (.txt, .doc)". */
    NSMutableArray *fileTypeAllowsAll;	/* Boolean if the all pattern (*.*) is
					 * included. */
    NSMutableArray *allowedExtensions;	/* Set of all allowed extensions. */
    bool allowedExtensionsAllowAll;	/* Set of all allowed extensions
					 * includes *.* */
    NSUInteger fileTypeIndex;		/* Index of currently selected
					 * filter. */
} filepanelFilterInfo;

static filepanelFilterInfo filterInfo;
static NSOpenPanel *openpanel;
static NSSavePanel *savepanel;

static const char *const colorOptionStrings[] = {
    "-initialcolor", "-parent", "-title", NULL
};
enum colorOptions {
    COLOR_INITIAL, COLOR_PARENT, COLOR_TITLE
};

static const char *const openOptionStrings[] = {
    "-defaultextension", "-filetypes", "-initialdir", "-initialfile",
    "-message", "-multiple", "-parent", "-title", "-typevariable",
    "-command", NULL
};
enum openOptions {
    OPEN_DEFAULT, OPEN_FILETYPES, OPEN_INITDIR, OPEN_INITFILE,
    OPEN_MESSAGE, OPEN_MULTIPLE, OPEN_PARENT, OPEN_TITLE,
    OPEN_TYPEVARIABLE, OPEN_COMMAND,
};
static const char *const saveOptionStrings[] = {
    "-defaultextension", "-filetypes", "-initialdir", "-initialfile",
    "-message", "-parent", "-title", "-typevariable", "-command",
    "-confirmoverwrite", NULL
};
enum saveOptions {
    SAVE_DEFAULT, SAVE_FILETYPES, SAVE_INITDIR, SAVE_INITFILE,
    SAVE_MESSAGE, SAVE_PARENT, SAVE_TITLE, SAVE_TYPEVARIABLE, SAVE_COMMAND,
    SAVE_CONFIRMOW
};
static const char *const chooseOptionStrings[] = {
    "-initialdir", "-message", "-mustexist", "-parent", "-title", "-command",
    NULL
};
enum chooseOptions {
    CHOOSE_INITDIR, CHOOSE_MESSAGE, CHOOSE_MUSTEXIST, CHOOSE_PARENT,
    CHOOSE_TITLE, CHOOSE_COMMAND,
};
typedef struct {
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int multiple;
} FilePanelCallbackInfo;

static const char *const alertOptionStrings[] = {
    "-default", "-detail", "-icon", "-message", "-parent", "-title",
    "-type", "-command", NULL
};
enum alertOptions {
    ALERT_DEFAULT, ALERT_DETAIL, ALERT_ICON, ALERT_MESSAGE, ALERT_PARENT,
    ALERT_TITLE, ALERT_TYPE, ALERT_COMMAND,
};
typedef struct {
    Tcl_Interp *interp;
    Tcl_Obj *cmdObj;
    int typeIndex;
} AlertCallbackInfo;
static const char *const alertTypeStrings[] = {
    "abortretryignore", "ok", "okcancel", "retrycancel", "yesno",
    "yesnocancel", NULL
};
enum alertTypeOptions {
    TYPE_ABORTRETRYIGNORE, TYPE_OK, TYPE_OKCANCEL, TYPE_RETRYCANCEL,
    TYPE_YESNO, TYPE_YESNOCANCEL
};
static const char *const alertIconStrings[] = {
    "error", "info", "question", "warning", NULL
};
enum alertIconOptions {
    ICON_ERROR, ICON_INFO, ICON_QUESTION, ICON_WARNING
};
static const char *const alertButtonStrings[] = {
    "abort", "retry", "ignore", "ok", "cancel", "no", "yes", NULL
};

static const NSString *const alertButtonNames[][3] = {
    [TYPE_ABORTRETRYIGNORE] =   {@"Abort", @"Retry", @"Ignore"},
    [TYPE_OK] =			{@"OK"},
    [TYPE_OKCANCEL] =		{@"OK", @"Cancel"},
    [TYPE_RETRYCANCEL] =	{@"Retry", @"Cancel"},
    [TYPE_YESNO] =		{@"Yes", @"No"},
    [TYPE_YESNOCANCEL] =	{@"Yes", @"No", @"Cancel"},
};
static const NSAlertStyle alertStyles[] = {
    [ICON_ERROR] =		NSWarningAlertStyle,
    [ICON_INFO] =		NSInformationalAlertStyle,
    [ICON_QUESTION] =		NSWarningAlertStyle,
    [ICON_WARNING] =		NSCriticalAlertStyle,
};

/*
 * Need to map from 'alertButtonStrings' and its corresponding integer, index
 * to the native button index, which is 1, 2, 3, from right to left. This is
 * necessary to do for each separate '-type' of button sets.
 */

static const short alertButtonIndexAndTypeToNativeButtonIndex[][7] = {
			    /*  abort retry ignore ok   cancel yes   no */
    [TYPE_ABORTRETRYIGNORE] =   {1,    2,    3,    0,    0,    0,    0},
    [TYPE_OK] =			{0,    0,    0,    1,    0,    0,    0},
    [TYPE_OKCANCEL] =		{0,    0,    0,    1,    2,    0,    0},
    [TYPE_RETRYCANCEL] =	{0,    1,    0,    0,    2,    0,    0},
    [TYPE_YESNO] =		{0,    0,    0,    0,    0,    2,    1},
    [TYPE_YESNOCANCEL] =	{0,    0,    0,    0,    3,    2,    1},
};

/*
 * Need also the inverse mapping, from NSAlertFirstButtonReturn etc to the
 * descriptive button text string index.
 */

static const short alertNativeButtonIndexAndTypeToButtonIndex[][3] = {
    [TYPE_ABORTRETRYIGNORE] =   {0, 1, 2},
    [TYPE_OK] =			{3, 0, 0},
    [TYPE_OKCANCEL] =		{3, 4, 0},
    [TYPE_RETRYCANCEL] =	{1, 4, 0},
    [TYPE_YESNO] =		{6, 5, 0},
    [TYPE_YESNOCANCEL] =	{6, 5, 4},
};

/*
 * Construct a file URL from directory and filename. Either may be nil. If both
 * are nil, returns nil.
 */

static NSURL *
getFileURL(
    NSString *directory,
    NSString *filename)
{
    NSURL *url = nil;
    if (directory) {
	url = [NSURL fileURLWithPath:directory isDirectory:YES];
    }
    if (filename) {
	url = [NSURL URLWithString:filename relativeToURL:url];
    }
    return url;
}

#pragma mark TKApplication(TKDialog)

@implementation TKApplication(TKDialog)

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    return YES;
}

- (void)panel:(id)sender didChangeToDirectoryURL:(NSURL *)url {
}

- (BOOL)panel:(id)sender validateURL:(NSURL *)url error:(NSError **)outError {
    *outError = nil;
    return YES;
}

- (void) tkFilePanelDidEnd: (NSSavePanel *) panel
	returnCode: (NSInteger) returnCode contextInfo: (void *) contextInfo
{
    FilePanelCallbackInfo *callbackInfo = contextInfo;

    if (returnCode == modalOK) {
	Tcl_Obj *resultObj;

	if (callbackInfo->multiple) {
	    resultObj = Tcl_NewListObj(0, NULL);
	    for (NSURL *url in [(NSOpenPanel*)panel URLs]) {
		Tcl_ListObjAppendElement(callbackInfo->interp, resultObj,
			Tcl_NewStringObj([[url path] UTF8String], -1));
	    }
	} else {
	    resultObj = Tcl_NewStringObj([[[panel URL]path] UTF8String], -1);
	}
	if (callbackInfo->cmdObj) {
	    Tcl_Obj **objv, **tmpv;
	    int objc, result = Tcl_ListObjGetElements(callbackInfo->interp,
		    callbackInfo->cmdObj, &objc, &objv);

	    if (result == TCL_OK && objc) {
		tmpv = ckalloc(sizeof(Tcl_Obj *) * (objc + 2));
		memcpy(tmpv, objv, sizeof(Tcl_Obj *) * objc);
		tmpv[objc] = resultObj;
		TkBackgroundEvalObjv(callbackInfo->interp, objc + 1, tmpv,
			TCL_EVAL_GLOBAL);
		ckfree(tmpv);
	    }
	} else {
	    Tcl_SetObjResult(callbackInfo->interp, resultObj);
	}
    } else if (returnCode == modalCancel) {
	Tcl_ResetResult(callbackInfo->interp);
    }
    if (panel == [NSApp modalWindow]) {
	[NSApp stopModalWithCode:returnCode];
    }
    if (callbackInfo->cmdObj) {
	Tcl_DecrRefCount(callbackInfo->cmdObj);
	ckfree(callbackInfo);
    }
}

- (void) tkAlertDidEnd: (NSAlert *) alert returnCode: (NSInteger) returnCode
	contextInfo: (void *) contextInfo
{
    AlertCallbackInfo *callbackInfo = contextInfo;

    if (returnCode >= NSAlertFirstButtonReturn) {
	Tcl_Obj *resultObj = Tcl_NewStringObj(alertButtonStrings[
		alertNativeButtonIndexAndTypeToButtonIndex[callbackInfo->
		typeIndex][returnCode - NSAlertFirstButtonReturn]], -1);

	if (callbackInfo->cmdObj) {
	    Tcl_Obj **objv, **tmpv;
	    int objc, result = Tcl_ListObjGetElements(callbackInfo->interp,
		    callbackInfo->cmdObj, &objc, &objv);

	    if (result == TCL_OK && objc) {
		tmpv = ckalloc(sizeof(Tcl_Obj *) * (objc + 2));
		memcpy(tmpv, objv, sizeof(Tcl_Obj *) * objc);
		tmpv[objc] = resultObj;
		TkBackgroundEvalObjv(callbackInfo->interp, objc + 1, tmpv,
			TCL_EVAL_GLOBAL);
		ckfree(tmpv);
	    }
	} else {
	    Tcl_SetObjResult(callbackInfo->interp, resultObj);
	}
    }
    if ([alert window] == [NSApp modalWindow]) {
	[NSApp stopModalWithCode:returnCode];
    }
    if (callbackInfo->cmdObj) {
	Tcl_DecrRefCount(callbackInfo->cmdObj);
	ckfree(callbackInfo);
    }
}

- (void)selectFormat:(id)sender  {
    NSPopUpButton *button      = (NSPopUpButton *)sender;
    filterInfo.fileTypeIndex   = [button indexOfSelectedItem];

    if ([[filterInfo.fileTypeAllowsAll objectAtIndex:filterInfo.fileTypeIndex] boolValue]) {
	[openpanel setAllowsOtherFileTypes:YES];

	/*
	 * setAllowsOtherFileTypes might have no effect; it's inherited from
	 * the NSSavePanel, where it has the effect that it does not append an
	 * extension. Setting the allowed file types to nil allows selecting
	 * any file.
	 */

	[openpanel setAllowedFileTypes:nil];
    } else {
	NSMutableArray *allowedtypes =
		[filterInfo.fileTypeExtensions objectAtIndex:filterInfo.fileTypeIndex];
	[openpanel setAllowedFileTypes:allowedtypes];
	[openpanel setAllowsOtherFileTypes:NO];
    }

    filterInfo.userHasSelectedFilter = true;
}

- (void)saveFormat:(id)sender  {
    NSPopUpButton *button     = (NSPopUpButton *)sender;
    filterInfo.fileTypeIndex  = [button indexOfSelectedItem];

    if ([[filterInfo.fileTypeAllowsAll objectAtIndex:filterInfo.fileTypeIndex] boolValue]) {
	[savepanel setAllowsOtherFileTypes:YES];
	[savepanel setAllowedFileTypes:nil];
    } else {
	NSMutableArray *allowedtypes =
		[filterInfo.fileTypeExtensions objectAtIndex:filterInfo.fileTypeIndex];
	[savepanel setAllowedFileTypes:allowedtypes];
	[savepanel setAllowsOtherFileTypes:NO];
    }

    filterInfo.userHasSelectedFilter = true;
}

@end

#pragma mark -

static NSInteger showOpenSavePanel(
    NSSavePanel *panel,
    NSWindow *parent,
    FilePanelCallbackInfo *callbackInfo)
{
    NSInteger modalReturnCode;

    if (parent && ![parent attachedSheet] && [NSApp macMinorVersion] < 15) {
	[panel beginSheetModalForWindow:parent
	       completionHandler:^(NSInteger returnCode) {
	    [NSApp tkFilePanelDidEnd:panel
		       returnCode:returnCode
		       contextInfo:callbackInfo ];
	    }];
	modalReturnCode = callbackInfo->cmdObj ? modalOther :
	    [NSApp runModalForWindow:panel];
    } else {
	modalReturnCode = [panel runModal];
	[NSApp tkFilePanelDidEnd:panel returnCode:modalReturnCode
		     contextInfo:callbackInfo];
    }
    return modalReturnCode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseColorObjCmd --
 *
 *	This procedure implements the color dialog box for the Mac platform.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseColorObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    int result = TCL_ERROR;
    Tk_Window parent, tkwin = clientData;
    const char *title = NULL;
    int i;
    NSColor *color = nil, *initialColor = nil;
    NSColorPanel *colorPanel;
    NSInteger returnCode, numberOfComponents = 0;

    for (i = 1; i < objc; i += 2) {
	int index;
	const char *value;

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], colorOptionStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "COLORDIALOG", "VALUE", NULL);
	    goto end;
	}
	value = Tcl_GetString(objv[i + 1]);

	switch (index) {
	case COLOR_INITIAL: {
	    XColor *colorPtr;

	    colorPtr = Tk_GetColor(interp, tkwin, value);
	    if (colorPtr == NULL) {
		goto end;
	    }
	    initialColor = TkMacOSXGetNSColor(NULL, colorPtr->pixel);
	    Tk_FreeColor(colorPtr);
	    break;
	}
	case COLOR_PARENT:
	    parent = Tk_NameToWindow(interp, value, tkwin);
	    if (parent == NULL) {
		goto end;
	    }
	    break;
	case COLOR_TITLE:
	    title = value;
	    break;
	}
    }
    colorPanel = [NSColorPanel sharedColorPanel];
    [colorPanel orderOut:NSApp];
    [colorPanel setContinuous:NO];
    [colorPanel setBecomesKeyOnlyIfNeeded:NO];
    [colorPanel setShowsAlpha: NO];
    [colorPanel _setUseModalAppearance:YES];
    if (title) {
	NSString *s = [[NSString alloc] initWithUTF8String:title];

	[colorPanel setTitle:s];
	[s release];
    }
    if (initialColor) {
	[colorPanel setColor:initialColor];
    }
    returnCode = [NSApp runModalForWindow:colorPanel];
    if (returnCode == modalOK) {
	color = [[colorPanel color] colorUsingColorSpace:
		[NSColorSpace deviceRGBColorSpace]];
	numberOfComponents = [color numberOfComponents];
    }
    if (color && numberOfComponents >= 3 && numberOfComponents <= 4) {
	CGFloat components[4];
	char colorstr[8];

	[color getComponents:components];
	snprintf(colorstr, 8, "#%02x%02x%02x",
		(short)(components[0] * 255),
		(short)(components[1] * 255),
		(short)(components[2] * 255));
	Tcl_SetObjResult(interp, Tcl_NewStringObj(colorstr, 7));
    } else {
	Tcl_ResetResult(interp);
    }
    result = TCL_OK;

end:
    return result;
}

/*
 * Dissect the -filetype nested lists and store the information in the
 * filterInfo structure.
 */

static int
parseFileFilters(
    Tcl_Interp *interp,
    Tcl_Obj *fileTypesPtr,
    Tcl_Obj *typeVariablePtr)
{

    if (!fileTypesPtr) {
	filterInfo.doFileTypes = false;
	return TCL_OK;
    }

    FileFilterList fl;

    TkInitFileFilters(&fl);
    if (TkGetFileFilters(interp, &fl, fileTypesPtr, 0) != TCL_OK) {
	TkFreeFileFilters(&fl);
	return TCL_ERROR;
    }

    filterInfo.doFileTypes = (fl.filters != NULL);

    filterInfo.fileTypeIndex = 0;
    filterInfo.fileTypeExtensions = [NSMutableArray array];
    filterInfo.fileTypeNames = [NSMutableArray array];
    filterInfo.fileTypeLabels = [NSMutableArray array];
    filterInfo.fileTypeAllowsAll = [NSMutableArray array];

    filterInfo.allowedExtensions = [NSMutableArray array];
    filterInfo.allowedExtensionsAllowAll = NO;

    if (filterInfo.doFileTypes) {
	for (FileFilter *filterPtr = fl.filters; filterPtr;
		filterPtr = filterPtr->next) {
	    NSString *name = [[NSString alloc] initWithUTF8String: filterPtr->name];

	    [filterInfo.fileTypeNames addObject:name];
	    [name release];
	    NSMutableArray *clauseextensions = [NSMutableArray array];
	    NSMutableArray *displayextensions = [NSMutableArray array];
	    bool allowsAll = NO;

	    for (FileFilterClause *clausePtr = filterPtr->clauses; clausePtr;
		    clausePtr = clausePtr->next) {

		for (GlobPattern *globPtr = clausePtr->patterns; globPtr;
			globPtr = globPtr->next) {
		    const char *str = globPtr->pattern;
		    while (*str && (*str == '*' || *str == '.')) {
		    	str++;
		    }
		    if (*str) {
			NSString *extension = [[NSString alloc] initWithUTF8String:str];
			if (![filterInfo.allowedExtensions containsObject:extension]) {
			    [filterInfo.allowedExtensions addObject:extension];
			}

			[clauseextensions addObject:extension];
			[displayextensions addObject:[@"." stringByAppendingString:extension]];

			[extension release];
		    } else {
			/*
			 * It is the all pattern (*, .* or *.*)
			 */

			allowsAll = YES;
			filterInfo.allowedExtensionsAllowAll = YES;
			[displayextensions addObject:@"*"];
		    }
		}
	    }
	    [filterInfo.fileTypeExtensions addObject:clauseextensions];
	    [filterInfo.fileTypeAllowsAll addObject:[NSNumber numberWithBool:allowsAll]];

	    NSMutableString *label = [[NSMutableString alloc] initWithString:name];
	    [label appendString:@" ("];
	    [label appendString:[displayextensions componentsJoinedByString:@", "]];
	    [label appendString:@")"];
	    [filterInfo.fileTypeLabels addObject:label];
	    [label release];
	}

	/*
	 * Check if the typevariable exists and matches one of the names.
	 */

	filterInfo.preselectFilter = false;
	filterInfo.userHasSelectedFilter = false;
	if (typeVariablePtr) {
	    /*
	     * Extract the variable content as a NSString.
	     */

	    Tcl_Obj *selectedFileTypeObj = Tcl_ObjGetVar2(interp,
		    typeVariablePtr, NULL, TCL_GLOBAL_ONLY);

	    /*
	     * Check that the typevariable exists.
	     */

	    if (selectedFileTypeObj != NULL) {
		const char *selectedFileType =
			Tcl_GetString(selectedFileTypeObj);
		NSString *selectedFileTypeStr =
			[[NSString alloc] initWithUTF8String:selectedFileType];
		NSUInteger index =
			[filterInfo.fileTypeNames indexOfObject:selectedFileTypeStr];

		if (index != NSNotFound) {
		    filterInfo.fileTypeIndex = index;
		    filterInfo.preselectFilter = true;
		}
	    }
	}

    }

    TkFreeFileFilters(&fl);
    return TCL_OK;
}

static bool
filterCompatible(
    NSString *extension,
    int filterIndex)
{
    NSMutableArray *allowedExtensions =
	    [filterInfo.fileTypeExtensions objectAtIndex: filterIndex];

    /*
     * If this contains the all pattern, accept any extension.
     */

    if ([[filterInfo.fileTypeAllowsAll objectAtIndex:filterIndex] boolValue]) {
	return true;
    }

    return [allowedExtensions containsObject: extension];
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetOpenFileObjCmd --
 *
 *	This procedure implements the "open file" dialog box for the Mac
 *	platform. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See user documentation.
 *----------------------------------------------------------------------
 */

int
Tk_GetOpenFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    char *str;
    int i, result = TCL_ERROR, haveParentOption = 0;
    int index, len, multiple = 0;
    Tcl_Obj *cmdObj = NULL, *typeVariablePtr = NULL, *fileTypesPtr = NULL;
    FilePanelCallbackInfo callbackInfoStruct;
    FilePanelCallbackInfo *callbackInfo = &callbackInfoStruct;
    NSString *directory = nil, *filename = nil;
    NSString *message = nil, *title = nil;
    NSWindow *parent;
    openpanel =  [NSOpenPanel openPanel];
    NSInteger modalReturnCode = modalError;
    BOOL parentIsKey = NO;

    [openpanel setDelegate:NSApp];

    for (i = 1; i < objc; i += 2) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], openOptionStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "FILEDIALOG", "VALUE", NULL);
	    goto end;
	}
	switch (index) {
	case OPEN_DEFAULT:
	    break;
	case OPEN_FILETYPES:
	    fileTypesPtr = objv[i + 1];
	    break;
	case OPEN_INITDIR:
	    str = Tcl_GetStringFromObj(objv[i + 1], &len);
	    if (len) {
		directory = [[[NSString alloc] initWithUTF8String:str]
			autorelease];
	    }
	    break;
	case OPEN_INITFILE:
	    str = Tcl_GetStringFromObj(objv[i + 1], &len);
	    if (len) {
		filename = [[[NSString alloc] initWithUTF8String:str]
			autorelease];
	    }
	    break;
	case OPEN_MESSAGE:
	    message = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    break;
	case OPEN_MULTIPLE:
	    if (Tcl_GetBooleanFromObj(interp, objv[i + 1],
		    &multiple) != TCL_OK) {
		goto end;
	    }
	    break;
	case OPEN_PARENT:
	    str = Tcl_GetStringFromObj(objv[i + 1], &len);
	    tkwin = Tk_NameToWindow(interp, str, tkwin);
	    if (!tkwin) {
		goto end;
	    }
	    haveParentOption = 1;
	    break;
	case OPEN_TITLE:
	    title = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    break;
	case OPEN_TYPEVARIABLE:
	    typeVariablePtr = objv[i + 1];
	    break;
	case OPEN_COMMAND:
	    cmdObj = objv[i+1];
	    break;
	}
    }
    if (title) {
	[openpanel setTitle:title];

	/*
	 * From OSX 10.11, the title string is silently ignored in the open
	 * panel.  Prepend the title to the message in this case.
	 */

	if ([NSApp macMinorVersion] > 10) {
	    if (message) {
		NSString *fullmessage =
		    [[NSString alloc] initWithFormat:@"%@\n%@", title, message];
		[message release];
		[title release];
		message = fullmessage;
	    } else {
		message = title;
	    }
	}
    }

    if (message) {
	[openpanel setMessage:message];
	[message release];
    }

    [openpanel setAllowsMultipleSelection:multiple];

    if (parseFileFilters(interp, fileTypesPtr, typeVariablePtr) != TCL_OK) {
	goto end;
    }

    if (filterInfo.doFileTypes) {
	NSTextField *label = [[NSTextField alloc]
		initWithFrame:NSMakeRect(0, 0, 60, 22)];
	NSPopUpButton *popupButton = [[NSPopUpButton alloc]
		initWithFrame:NSMakeRect(50.0, 2, 240, 22.0) pullsDown:NO];
	NSView *accessoryView = [[NSView alloc]
		initWithFrame:NSMakeRect(0.0, 0.0, 300, 32.0)];

	[label setEditable:NO];
	[label setStringValue:@"Filter:"];
	[label setBordered:NO];
	[label setBezeled:NO];
	[label setDrawsBackground:NO];
	[popupButton addItemsWithTitles:filterInfo.fileTypeLabels];
	[popupButton setAction:@selector(selectFormat:)];
	[accessoryView addSubview:label];
	[accessoryView addSubview:popupButton];
	if (filterInfo.preselectFilter) {

	    /*
	     * A specific filter was selected from the typevariable. Select it
	     * and open the accessory view.
	     */

	    [popupButton selectItemAtIndex:filterInfo.fileTypeIndex];

	    /*
	     * On OSX > 10.11, the options are not visible by default. Ergo
	     * allow all file types
	    [openpanel setAllowedFileTypes:filterInfo.fileTypeExtensions[filterInfo.fileTypeIndex]];
	    */

	    [openpanel setAllowedFileTypes:filterInfo.allowedExtensions];
	} else {
	    [openpanel setAllowedFileTypes:filterInfo.allowedExtensions];
	}
	if (filterInfo.allowedExtensionsAllowAll) {
	    [openpanel setAllowsOtherFileTypes:YES];
	} else {
	    [openpanel setAllowsOtherFileTypes:NO];
	}
	[openpanel setAccessoryView:accessoryView];
    } else {
	/*
	 * No filters are given. Allow picking all files.
	 */

	[openpanel setAllowsOtherFileTypes:YES];
    }
    if (cmdObj) {
	if (Tcl_IsShared(cmdObj)) {
	    cmdObj = Tcl_DuplicateObj(cmdObj);
	}
	Tcl_IncrRefCount(cmdObj);
    }
    callbackInfo = ckalloc(sizeof(FilePanelCallbackInfo));
    callbackInfo->cmdObj = cmdObj;
    callbackInfo->interp = interp;
    callbackInfo->multiple = multiple;
    if (directory || filename) {
	NSURL *fileURL = getFileURL(directory, filename);

	[openpanel setDirectoryURL:fileURL];
    }
    if (haveParentOption) {
	parent = TkMacOSXDrawableWindow(((TkWindow *) tkwin)->window);
	parentIsKey = parent && [parent isKeyWindow];
    } else {
	parent = nil;
	parentIsKey = False;
    }
    modalReturnCode = showOpenSavePanel(openpanel, parent, callbackInfo);
    result = (modalReturnCode != modalError) ? TCL_OK : TCL_ERROR;
    if (parentIsKey) {
	[parent makeKeyWindow];
    }
    if ((typeVariablePtr && (modalReturnCode == NSOKButton))
	    && filterInfo.doFileTypes) {
	/*
	 * The -typevariable must be set to the selected file type, if the
	 * dialog was not cancelled.
	 */

	NSUInteger selectedFilterIndex = filterInfo.fileTypeIndex;
	NSString *selectedFilter = NULL;

	if (filterInfo.userHasSelectedFilter) {
	    selectedFilterIndex = filterInfo.fileTypeIndex;
	    selectedFilter = [filterInfo.fileTypeNames objectAtIndex:selectedFilterIndex];
	} else {
	    /*
	     * Difficult case: the user has not touched the filter settings,
	     * but we must return something in the typevariable. First check if
	     * the preselected type is compatible with the selected file,
	     * otherwise choose the first compatible type from the list,
	     * finally fall back to the empty string.
	     */

	    NSURL *selectedFile;
	    NSString *extension;
	    if (multiple) {
		/*
		 * Use the first file in the case of multiple selection.
		 * Anyway it is not overly useful here.
		 */
		selectedFile = [[openpanel URLs] objectAtIndex:0];
	    } else {
		selectedFile = [openpanel URL];
	    }

	    extension = [selectedFile pathExtension];

	    if (filterInfo.preselectFilter &&
		    filterCompatible(extension, filterInfo.fileTypeIndex)) {
		selectedFilterIndex = filterInfo.fileTypeIndex;  // The preselection from the typevariable
		selectedFilter = [filterInfo.fileTypeNames objectAtIndex:selectedFilterIndex];
	    } else {
		NSUInteger i;

		for (i = 0; i < [filterInfo.fileTypeNames count]; i++) {
		    if (filterCompatible(extension, i)) {
			selectedFilterIndex = i;
			break;
		    }
		}
		if (i == selectedFilterIndex) {
		    selectedFilter = [filterInfo.fileTypeNames objectAtIndex:selectedFilterIndex];
		} else {
		    selectedFilter = @"";
		}
	    }
	}
	Tcl_ObjSetVar2(interp, typeVariablePtr, NULL,
		Tcl_NewStringObj([selectedFilter UTF8String], -1),
		TCL_GLOBAL_ONLY);
    }
 end:
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetSaveFileObjCmd --
 *
 *	This procedure implements the "save file" dialog box for the Mac
 *	platform. See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See user documentation.
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetSaveFileObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    char *str;
    int i, result = TCL_ERROR, haveParentOption = 0;
    int confirmOverwrite = 1;
    int index, len;
    Tcl_Obj *cmdObj = NULL, *typeVariablePtr = NULL, *fileTypesPtr = NULL;
    FilePanelCallbackInfo callbackInfoStruct;
    FilePanelCallbackInfo *callbackInfo = &callbackInfoStruct;
    NSString *directory = nil, *filename = nil, *defaultType = nil;
    NSString *message = nil, *title = nil;
    NSWindow *parent;
    savepanel = [NSSavePanel savePanel];
    NSInteger modalReturnCode = modalError;
    BOOL parentIsKey = NO;

    [savepanel setDelegate:NSApp];

    for (i = 1; i < objc; i += 2) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], saveOptionStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "FILEDIALOG", "VALUE", NULL);
	    goto end;
	}
	switch (index) {
	    case SAVE_DEFAULT:
		str = Tcl_GetStringFromObj(objv[i + 1], &len);
		while (*str && (*str == '*' || *str == '.')) {
		    str++;
		}
		if (*str) {
		    defaultType = [[[NSString alloc] initWithUTF8String:str]
			    autorelease];
		}
		break;
	    case SAVE_FILETYPES:
		fileTypesPtr = objv[i + 1];
		break;
	    case SAVE_INITDIR:
		str = Tcl_GetStringFromObj(objv[i + 1], &len);
		if (len) {
		    directory = [[[NSString alloc] initWithUTF8String:str]
			    autorelease];
		}
		break;
	    case SAVE_INITFILE:
		str = Tcl_GetStringFromObj(objv[i + 1], &len);
		if (len) {
		    filename = [[[NSString alloc] initWithUTF8String:str]
			    autorelease];
		    [savepanel setNameFieldStringValue:filename];
		}
		break;
	    case SAVE_MESSAGE:
		message = [[NSString alloc] initWithUTF8String:
			Tcl_GetString(objv[i + 1])];
		break;
	    case SAVE_PARENT:
		str = Tcl_GetStringFromObj(objv[i + 1], &len);
		tkwin = Tk_NameToWindow(interp, str, tkwin);
		if (!tkwin) {
		    goto end;
		}
		haveParentOption = 1;
		break;
	    case SAVE_TITLE:
		title = [[NSString alloc] initWithUTF8String:
			Tcl_GetString(objv[i + 1])];
		break;
	    case SAVE_TYPEVARIABLE:
		typeVariablePtr = objv[i + 1];
		break;
	    case SAVE_COMMAND:
		cmdObj = objv[i+1];
		break;
	    case SAVE_CONFIRMOW:
		if (Tcl_GetBooleanFromObj(interp, objv[i + 1],
			&confirmOverwrite) != TCL_OK) {
		    goto end;
		}
		break;
	}
    }

    if (title) {
	[savepanel setTitle:title];

	/*
	 * From OSX 10.11, the title string is silently ignored, if the save
	 * panel is a sheet.  Prepend the title to the message in this case.
	 * NOTE: should be conditional on OSX version, but -mmacosx-version-min
	 * does not revert this behaviour.
	 */

	if (haveParentOption) {
	    if (message) {
		NSString *fullmessage =
		    [[NSString alloc] initWithFormat:@"%@\n%@",title,message];
		[message release];
		[title release];
		message = fullmessage;
	    } else {
		message = title;
	    }
	}
    }

    if (message) {
	[savepanel setMessage:message];
	[message release];
    }

    if (parseFileFilters(interp, fileTypesPtr, typeVariablePtr) != TCL_OK) {
	goto end;
    }

    if (filterInfo.doFileTypes) {
	NSView *accessoryView = [[NSView alloc]
		initWithFrame:NSMakeRect(0.0, 0.0, 300, 32.0)];
	NSTextField *label = [[NSTextField alloc]
		initWithFrame:NSMakeRect(0, 0, 60, 22)];

	[label setEditable:NO];
	[label setStringValue:NSLocalizedString(@"Format:", nil)];
	[label setBordered:NO];
	[label setBezeled:NO];
	[label setDrawsBackground:NO];

	NSPopUpButton *popupButton = [[NSPopUpButton alloc]
		initWithFrame:NSMakeRect(50.0, 2, 340, 22.0) pullsDown:NO];

	[popupButton addItemsWithTitles:filterInfo.fileTypeLabels];
	[popupButton selectItemAtIndex:filterInfo.fileTypeIndex];
	[popupButton setAction:@selector(saveFormat:)];

	[accessoryView addSubview:label];
	[accessoryView addSubview:popupButton];

	[savepanel setAccessoryView:accessoryView];

	[savepanel setAllowedFileTypes:[filterInfo.fileTypeExtensions objectAtIndex:filterInfo.fileTypeIndex]];
	[savepanel setAllowsOtherFileTypes:filterInfo.allowedExtensionsAllowAll];
    } else if (defaultType) {
	/*
	 * If no filetypes are given, defaultextension is an alternative way to
	 * specify the attached extension. Just propose this extension, but
	 * don't display an accessory view.
	 */

	NSMutableArray *AllowedFileTypes = [NSMutableArray array];

	[AllowedFileTypes addObject:defaultType];
	[savepanel setAllowedFileTypes:AllowedFileTypes];
	[savepanel setAllowsOtherFileTypes:YES];
    }

    [savepanel setCanSelectHiddenExtension:YES];
    [savepanel setExtensionHidden:NO];

    if (cmdObj) {
	if (Tcl_IsShared(cmdObj)) {
	    cmdObj = Tcl_DuplicateObj(cmdObj);
	}
	Tcl_IncrRefCount(cmdObj);
    }
    callbackInfo = ckalloc(sizeof(FilePanelCallbackInfo));
    callbackInfo->cmdObj = cmdObj;
    callbackInfo->interp = interp;
    callbackInfo->multiple = 0;

    if (directory) {
	[savepanel setDirectoryURL:[NSURL fileURLWithPath:directory isDirectory:YES]];
    }

    /*
     * Check for file name and set to the empty string if nil. This prevents a crash
     * with an uncaught exception.
     */

    if (filename) {
	[savepanel setNameFieldStringValue:filename];
    } else {
	[savepanel setNameFieldStringValue:@""];
    }
    if (haveParentOption) {
	parent = TkMacOSXDrawableWindow(((TkWindow *) tkwin)->window);
	parentIsKey = parent && [parent isKeyWindow];
    } else {
	parent = nil;
	parentIsKey = False;
    }
    modalReturnCode = showOpenSavePanel(savepanel, parent, callbackInfo);
    result = (modalReturnCode != modalError) ? TCL_OK : TCL_ERROR;
    if (parentIsKey) {
	[parent makeKeyWindow];
    }

    if (typeVariablePtr && (modalReturnCode == NSOKButton)
	    && filterInfo.doFileTypes) {
	/*
	 * The -typevariable must be set to the selected file type, if the
	 * dialog was not cancelled.
	 */

	NSString *selectedFilter =
	    [filterInfo.fileTypeNames objectAtIndex:filterInfo.fileTypeIndex];
	Tcl_ObjSetVar2(interp, typeVariablePtr, NULL,
		Tcl_NewStringObj([selectedFilter UTF8String], -1),
		TCL_GLOBAL_ONLY);
    }

  end:
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_ChooseDirectoryObjCmd --
 *
 *	This procedure implements the "tk_chooseDirectory" dialog box for the
 *	MacOS X platform. See the user documentation for details on what it
 *	does.
 *
 * Results:
 *	See user documentation.
 *
 * Side effects:
 *	A modal dialog window is created.
 *
 *----------------------------------------------------------------------
 */

int
Tk_ChooseDirectoryObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    char *str;
    int i, result = TCL_ERROR, haveParentOption = 0;
    int index, len, mustexist = 0;
    Tcl_Obj *cmdObj = NULL;
    FilePanelCallbackInfo callbackInfoStruct;
    FilePanelCallbackInfo *callbackInfo = &callbackInfoStruct;
    NSString *directory = nil;
    NSString *message, *title;
    NSWindow *parent;
    NSOpenPanel *panel = [NSOpenPanel openPanel];
    NSInteger modalReturnCode = modalError;
    BOOL parentIsKey = NO;

    [panel setDelegate:NSApp];

    for (i = 1; i < objc; i += 2) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], chooseOptionStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "DIRDIALOG", "VALUE", NULL);
	    goto end;
	}
	switch (index) {
	case CHOOSE_INITDIR:
	    str = Tcl_GetStringFromObj(objv[i + 1], &len);
	    if (len) {
		directory = [[[NSString alloc] initWithUTF8String:str]
			autorelease];
	    }
	    break;
	case CHOOSE_MESSAGE:
	    message = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    [panel setMessage:message];
	    [message release];
	    break;
	case CHOOSE_MUSTEXIST:
	    if (Tcl_GetBooleanFromObj(interp, objv[i + 1],
		    &mustexist) != TCL_OK) {
		goto end;
	    }
	    break;
	case CHOOSE_PARENT:
	    str = Tcl_GetStringFromObj(objv[i + 1], &len);
	    tkwin = Tk_NameToWindow(interp, str, tkwin);
	    if (!tkwin) {
		goto end;
	    }
	    haveParentOption = 1;
	    break;
	case CHOOSE_TITLE:
	    title = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    [panel setTitle:title];
	    [title release];
	    break;
	case CHOOSE_COMMAND:
	    cmdObj = objv[i+1];
	    break;
	}
    }
    [panel setPrompt:@"Choose"];
    [panel setCanChooseFiles:NO];
    [panel setCanChooseDirectories:YES];
    [panel setCanCreateDirectories:!mustexist];
    if (cmdObj) {
	if (Tcl_IsShared(cmdObj)) {
	    cmdObj = Tcl_DuplicateObj(cmdObj);
	}
	Tcl_IncrRefCount(cmdObj);
    }
    callbackInfo = ckalloc(sizeof(FilePanelCallbackInfo));
    callbackInfo->cmdObj = cmdObj;
    callbackInfo->interp = interp;
    callbackInfo->multiple = 0;

    /*
     * Check for directory value, set to root if not specified; otherwise
     * crashes with exception because of nil string parameter.
     */

    if (!directory) {
	directory = @"/";
    }
    parent = TkMacOSXDrawableWindow(((TkWindow *) tkwin)->window);
    [panel setDirectoryURL:[NSURL fileURLWithPath:directory isDirectory:YES]];
    if (haveParentOption) {
	parent = TkMacOSXDrawableWindow(((TkWindow *) tkwin)->window);
	parentIsKey = parent && [parent isKeyWindow];
    } else {
	parent = nil;
	parentIsKey = False;
    }
    modalReturnCode = showOpenSavePanel(panel, parent, callbackInfo);
    result = (modalReturnCode != modalError) ? TCL_OK : TCL_ERROR;
    if (parentIsKey) {
	[parent makeKeyWindow];
    }
  end:
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkAboutDlg --
 *
 *	Displays the default Tk About box.
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
TkAboutDlg(void)
{
    NSImage *image;
    NSString *path = [NSApp tkFrameworkImagePath: @"Tk.tiff"];

    if (path) {
	image = [[[NSImage alloc] initWithContentsOfFile:path] autorelease];
    } else {
	image = [NSApp applicationIconImage];
    }

    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];

    [dateFormatter setFormatterBehavior:NSDateFormatterBehavior10_4];
    [dateFormatter setDateFormat:@"Y"];

    NSString *year = [dateFormatter stringFromDate:[NSDate date]];

    [dateFormatter release];

    /*
     * This replaces the old about dialog with a standard alert that displays
     * correctly on 10.14.
     */

    NSString *version =  @"Tcl " TCL_PATCH_LEVEL " & Tk " TCL_PATCH_LEVEL;
    NSString *url = @"www.tcl-lang.org";
    NSTextView *credits = [[NSTextView alloc] initWithFrame:NSMakeRect(0,0,300,300)];
    NSFont *font = [NSFont systemFontOfSize:[NSFont systemFontSize]];
    NSDictionary *textAttributes = [NSDictionary dictionaryWithObject:font
					        forKey:NSFontAttributeName];

    [credits insertText: [[NSAttributedString alloc]
		 initWithString:[NSString stringWithFormat: @"\n"
		"Tcl and Tk are distributed under a modified BSD license: "
		"www.tcl.tk/software/tcltk/license.html\n\n"
		"%1$C 1987-%2$@ Tcl Core Team and Contributers.\n\n"
		"%1$C 2011-%2$@ Kevin Walzer/WordTech Communications LLC.\n\n"
		"%1$C 2014-%2$@ Marc Culler.\n\n"
		"%1$C 2002-2012 Daniel A. Steffen.\n\n"
		"%1$C 2001-2009 Apple Inc.\n\n"
		"%1$C 2001-2002 Jim Ingham & Ian Reid\n\n"
		"%1$C 1998-2000 Jim Ingham & Ray Johnson\n\n"
		"%1$C 1998-2000 Scriptics Inc.\n\n"
		"%1$C 1996-1997 Sun Microsystems Inc.", 0xA9, year]
	    attributes:textAttributes]
            replacementRange:NSMakeRange(0,0)];
    [credits setDrawsBackground:NO];
    [credits setEditable:NO];

    NSAlert *about = [[NSAlert alloc] init];

    [[about window] setTitle:@"About Tcl & Tk"];
    [about setMessageText: version];
    [about setInformativeText:url];
    about.accessoryView = credits;
    [about runModal];
    [about release];
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXStandardAboutPanelObjCmd --
 *
 *	Implements the ::tk::mac::standardAboutPanel command.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXStandardAboutPanelObjCmd(
    ClientData clientData,	/* Unused. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    if (objc > 1) {
	Tcl_WrongNumArgs(interp, 1, objv, NULL);
	return TCL_ERROR;
    }
    TkAboutDlg();
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_MessageBoxObjCmd --
 *
 *	Implements the tk_messageBox in native Mac OS X style.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */

int
Tk_MessageBoxObjCmd(
    ClientData clientData,	/* Main window associated with interpreter. */
    Tcl_Interp *interp,		/* Current interpreter. */
    int objc,			/* Number of arguments. */
    Tcl_Obj *const objv[])	/* Argument objects. */
{
    Tk_Window tkwin = clientData;
    char *str;
    int i, result = TCL_ERROR, haveParentOption = 0;
    int index, typeIndex, iconIndex, indexDefaultOption = 0;
    int defaultNativeButtonIndex = 1; /* 1, 2, 3: right to left */
    Tcl_Obj *cmdObj = NULL;
    AlertCallbackInfo callbackInfoStruct, *callbackInfo = &callbackInfoStruct;
    NSString *message, *title;
    NSWindow *parent;
    NSArray *buttons;
    NSAlert *alert = [NSAlert new];
    NSInteger modalReturnCode = 1;
    BOOL parentIsKey = NO;

    iconIndex = ICON_INFO;
    typeIndex = TYPE_OK;
    for (i = 1; i < objc; i += 2) {
	if (Tcl_GetIndexFromObjStruct(interp, objv[i], alertOptionStrings,
		sizeof(char *), "option", TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "MSGBOX", "VALUE", NULL);
	    goto end;
	}
	switch (index) {
	case ALERT_DEFAULT:
	    /*
	     * Need to postpone processing of this option until we are sure to
	     * know the '-type' as well.
	     */

	    indexDefaultOption = i;
	    break;

	case ALERT_DETAIL:
	    message = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    [alert setInformativeText:message];
	    [message release];
	    break;

	case ALERT_ICON:
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i + 1], alertIconStrings,
		    sizeof(char *), "-icon value", TCL_EXACT, &iconIndex) != TCL_OK) {
		goto end;
	    }
	    break;

	case ALERT_MESSAGE:
	    message = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    [alert setMessageText:message];
	    [message release];
	    break;

	case ALERT_PARENT:
	    str = Tcl_GetString(objv[i + 1]);
	    tkwin = Tk_NameToWindow(interp, str, tkwin);
	    if (!tkwin) {
		goto end;
	    }
	    haveParentOption = 1;
	    break;

	case ALERT_TITLE:
	    title = [[NSString alloc] initWithUTF8String:
		    Tcl_GetString(objv[i + 1])];
	    [[alert window] setTitle:title];
	    [title release];
	    break;

	case ALERT_TYPE:
	    if (Tcl_GetIndexFromObjStruct(interp, objv[i + 1], alertTypeStrings,
		    sizeof(char *), "-type value", TCL_EXACT, &typeIndex) != TCL_OK) {
		goto end;
	    }
	    break;
	case ALERT_COMMAND:
	    cmdObj = objv[i+1];
	    break;
	}
    }
    if (indexDefaultOption) {
	/*
	 * Any '-default' option needs to know the '-type' option, which is
	 * why we do this here.
	 */

	if (Tcl_GetIndexFromObjStruct(interp, objv[indexDefaultOption + 1],
		alertButtonStrings, sizeof(char *), "-default value",
		TCL_EXACT, &index) != TCL_OK) {
	    goto end;
	}

	/*
	 * Need to map from "ok" etc. to 1, 2, 3, right to left.
	 */

	defaultNativeButtonIndex =
		alertButtonIndexAndTypeToNativeButtonIndex[typeIndex][index];
	if (!defaultNativeButtonIndex) {
	    Tcl_SetObjResult(interp,
		    Tcl_NewStringObj("Illegal default option", -1));
	    Tcl_SetErrorCode(interp, "TK", "MSGBOX", "DEFAULT", NULL);
	    goto end;
	}
    }
    [alert setIcon:[NSApp applicationIconImage]];
    [alert setAlertStyle:alertStyles[iconIndex]];
    i = 0;
    while (i < 3 && alertButtonNames[typeIndex][i]) {
	[alert addButtonWithTitle:(NSString*) alertButtonNames[typeIndex][i++]];
    }
    buttons = [alert buttons];
    for (NSButton *b in buttons) {
	NSString *ke = [b keyEquivalent];

	if (([ke isEqualToString:@"\r"] || [ke isEqualToString:@"\033"]) &&
		![b keyEquivalentModifierMask]) {
	    [b setKeyEquivalent:@""];
	}
    }
    [[buttons objectAtIndex: [buttons count]-1] setKeyEquivalent: @"\033"];
    [[buttons objectAtIndex: defaultNativeButtonIndex-1]
	    setKeyEquivalent: @"\r"];
    if (cmdObj) {
	if (Tcl_IsShared(cmdObj)) {
	    cmdObj = Tcl_DuplicateObj(cmdObj);
	}
	Tcl_IncrRefCount(cmdObj);
    }
    callbackInfo = ckalloc(sizeof(AlertCallbackInfo));
    callbackInfo->cmdObj = cmdObj;
    callbackInfo->interp = interp;
    callbackInfo->typeIndex = typeIndex;
    parent = TkMacOSXDrawableWindow(((TkWindow *) tkwin)->window);
    if (haveParentOption && parent && ![parent attachedSheet]) {
	parentIsKey = [parent isKeyWindow];
#if MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
 	[alert beginSheetModalForWindow:parent
	       completionHandler:^(NSModalResponse returnCode) {
	    [NSApp tkAlertDidEnd:alert
		    returnCode:returnCode
		    contextInfo:callbackInfo];
	}];
#else
	[alert beginSheetModalForWindow:parent
	       modalDelegate:NSApp
	       didEndSelector:@selector(tkAlertDidEnd:returnCode:contextInfo:)
	       contextInfo:callbackInfo];
#endif
	modalReturnCode = cmdObj ? 0 :
	    [alert runModal];
    } else {
	modalReturnCode = [alert runModal];
	[NSApp tkAlertDidEnd:alert returnCode:modalReturnCode
		contextInfo:callbackInfo];
    }
    result = (modalReturnCode >= NSAlertFirstButtonReturn) ? TCL_OK : TCL_ERROR;
  end:
    [alert release];
    if (parentIsKey) {
	[parent makeKeyWindow];
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 */
#pragma mark [tk fontchooser] implementation (TIP 324)
/*
 *----------------------------------------------------------------------
 */

#include "tkMacOSXEvent.h"
#include "tkMacOSXFont.h"

typedef struct FontchooserData {
    Tcl_Obj *titleObj;
    Tcl_Obj *cmdObj;
    Tk_Window parent;
} FontchooserData;

enum FontchooserEvent {
    FontchooserClosed,
    FontchooserSelection
};

static void		FontchooserEvent(int kind);
static Tcl_Obj *	FontchooserCget(FontchooserData *fcdPtr,
			    int optionIndex);
static int		FontchooserConfigureCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		FontchooserShowCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static int		FontchooserHideCmd(ClientData clientData,
			    Tcl_Interp *interp, int objc,
			    Tcl_Obj *const objv[]);
static void		FontchooserParentEventHandler(ClientData clientData,
			    XEvent *eventPtr);
static void		DeleteFontchooserData(ClientData clientData,
			    Tcl_Interp *interp);

MODULE_SCOPE const TkEnsemble tkFontchooserEnsemble[];
const TkEnsemble tkFontchooserEnsemble[] = {
    { "configure", FontchooserConfigureCmd, NULL },
    { "show", FontchooserShowCmd, NULL },
    { "hide", FontchooserHideCmd, NULL },
    { NULL, NULL, NULL }
};

static Tcl_Interp *fontchooserInterp = NULL;
static NSFont *fontPanelFont = nil;
static NSMutableDictionary *fontPanelFontAttributes = nil;

static const char *const fontchooserOptionStrings[] = {
    "-parent", "-title", "-font", "-command",
    "-visible", NULL
};
enum FontchooserOption {
    FontchooserParent, FontchooserTitle, FontchooserFont, FontchooserCmd,
    FontchooserVisible
};

@implementation TKApplication(TKFontPanel)

- (void) changeFont: (id) sender
{
    NSFontManager *fm = [NSFontManager sharedFontManager];

    if ([fm currentFontAction] == NSViaPanelFontAction) {
	NSFont *font = [fm convertFont:fontPanelFont];

	if (![fontPanelFont isEqual:font]) {
	    [fontPanelFont release];
	    fontPanelFont = [font retain];
	    FontchooserEvent(FontchooserSelection);
	}
    }
}

- (void) changeAttributes: (id) sender
{
    NSDictionary *attributes = [sender convertAttributes:
	    fontPanelFontAttributes];

    if (![fontPanelFontAttributes isEqual:attributes]) {
	[fontPanelFontAttributes setDictionary:attributes];
	FontchooserEvent(FontchooserSelection);
    }
}

- (NSUInteger) validModesForFontPanel: (NSFontPanel *) fontPanel
{
    return (NSFontPanelStandardModesMask & ~NSFontPanelAllEffectsModeMask) |
	    NSFontPanelUnderlineEffectModeMask |
	    NSFontPanelStrikethroughEffectModeMask;
}

- (void) windowDidOrderOffScreen: (NSNotification *) notification
{
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
#endif
    if ([[notification object] isEqual:[[NSFontManager sharedFontManager]
	    fontPanel:NO]]) {
	FontchooserEvent(FontchooserClosed);
    }
}
@end

/*
 *----------------------------------------------------------------------
 *
 * FontchooserEvent --
 *
 *	This processes events generated by user interaction with the font
 *	panel.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Additional events may be place on the Tk event queue.
 *
 *----------------------------------------------------------------------
 */

static void
FontchooserEvent(
    int kind)
{
    FontchooserData *fcdPtr;
    Tcl_Obj *fontObj;

    if (!fontchooserInterp) {
	return;
    }
    fcdPtr = Tcl_GetAssocData(fontchooserInterp, "::tk::fontchooser", NULL);
    switch (kind) {
    case FontchooserClosed:
	if (fcdPtr->parent != None) {
	    TkSendVirtualEvent(fcdPtr->parent, "TkFontchooserVisibility", NULL);
	    fontchooserInterp = NULL;
	}
	break;
    case FontchooserSelection:
	fontObj = TkMacOSXFontDescriptionForNSFontAndNSFontAttributes(
		fontPanelFont, fontPanelFontAttributes);
	if (fontObj) {
	    if (fcdPtr->cmdObj) {
		int objc, result;
		Tcl_Obj **objv, **tmpv;

		result = Tcl_ListObjGetElements(fontchooserInterp,
			fcdPtr->cmdObj, &objc, &objv);
		if (result == TCL_OK) {
		    tmpv = ckalloc(sizeof(Tcl_Obj *) * (objc + 2));
		    memcpy(tmpv, objv, sizeof(Tcl_Obj *) * objc);
		    tmpv[objc] = fontObj;
		    TkBackgroundEvalObjv(fontchooserInterp, objc + 1, tmpv,
			    TCL_EVAL_GLOBAL);
		    ckfree(tmpv);
		}
	    }
	    TkSendVirtualEvent(fcdPtr->parent, "TkFontchooserFontChanged", NULL);
	}
	break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FontchooserCget --
 *
 *	Helper for the FontchooserConfigure command to return the current value
 *	of any of the options (which may be NULL in the structure).
 *
 * Results:
 *	Tcl object of option value.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
FontchooserCget(
    FontchooserData *fcdPtr,
    int optionIndex)
{
    Tcl_Obj *resObj = NULL;

    switch(optionIndex) {
    case FontchooserParent:
	if (fcdPtr->parent != None) {
	    resObj = Tcl_NewStringObj(
		    ((TkWindow *) fcdPtr->parent)->pathName, -1);
	} else {
	    resObj = Tcl_NewStringObj(".", 1);
	}
	break;
    case FontchooserTitle:
	if (fcdPtr->titleObj) {
	    resObj = fcdPtr->titleObj;
	} else {
	    resObj = Tcl_NewObj();
	}
	break;
    case FontchooserFont:
	resObj = TkMacOSXFontDescriptionForNSFontAndNSFontAttributes(
		fontPanelFont, fontPanelFontAttributes);
	if (!resObj) {
	    resObj = Tcl_NewObj();
	}
	break;
    case FontchooserCmd:
	if (fcdPtr->cmdObj) {
	    resObj = fcdPtr->cmdObj;
	} else {
	    resObj = Tcl_NewObj();
	}
	break;
    case FontchooserVisible:
	resObj = Tcl_NewBooleanObj([[[NSFontManager sharedFontManager]
		fontPanel:NO] isVisible]);
	break;
    default:
	resObj = Tcl_NewObj();
    }
    return resObj;
}

/*
 * ----------------------------------------------------------------------
 *
 * FontchooserConfigureCmd --
 *
 *	Implementation of the 'tk fontchooser configure' ensemble command.  See
 *	the user documentation for what it does.
 *
 * Results:
 *	See the user documentation.
 *
 * Side effects:
 *	Per-interp data structure may be modified
 *
 * ----------------------------------------------------------------------
 */

static int
FontchooserConfigureCmd(
    ClientData clientData,	/* Main window */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    Tk_Window tkwin = (Tk_Window)clientData;
    FontchooserData *fcdPtr = Tcl_GetAssocData(interp, "::tk::fontchooser",
	    NULL);
    int i, r = TCL_OK;

    /*
     * With no arguments we return all the options in a dict
     */

    if (objc == 1) {
	Tcl_Obj *keyObj, *valueObj;
	Tcl_Obj *dictObj = Tcl_NewDictObj();

	for (i = 0; r == TCL_OK && fontchooserOptionStrings[i] != NULL; ++i) {
	    keyObj = Tcl_NewStringObj(fontchooserOptionStrings[i], -1);
	    valueObj = FontchooserCget(fcdPtr, i);
	    r = Tcl_DictObjPut(interp, dictObj, keyObj, valueObj);
	}
	if (r == TCL_OK) {
	    Tcl_SetObjResult(interp, dictObj);
	}
	return r;
    }

    for (i = 1; i < objc; i += 2) {
	int optionIndex, len;

	if (Tcl_GetIndexFromObjStruct(interp, objv[i], fontchooserOptionStrings,
		sizeof(char *), "option", 0, &optionIndex) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (objc == 2) {
	    /*
	     * With one option and no arg, return the current value.
	     */

	    Tcl_SetObjResult(interp, FontchooserCget(fcdPtr, optionIndex));
	    return TCL_OK;
	}
	if (i + 1 == objc) {
	    Tcl_SetObjResult(interp, Tcl_ObjPrintf(
		    "value for \"%s\" missing", Tcl_GetString(objv[i])));
	    Tcl_SetErrorCode(interp, "TK", "FONTDIALOG", "VALUE", NULL);
	    return TCL_ERROR;
	}
	switch (optionIndex) {
	case FontchooserVisible: {
	    const char *msg = "cannot change read-only option "
		    "\"-visible\": use the show or hide command";

	    Tcl_SetObjResult(interp, Tcl_NewStringObj(msg, -1));
	    Tcl_SetErrorCode(interp, "TK", "FONTDIALOG", "READONLY", NULL);
	    return TCL_ERROR;
	}
	case FontchooserParent: {
	    Tk_Window parent = Tk_NameToWindow(interp,
		    Tcl_GetString(objv[i+1]), tkwin);

	    if (parent == None) {
		return TCL_ERROR;
	    }
	    if (fcdPtr->parent) {
		Tk_DeleteEventHandler(fcdPtr->parent, StructureNotifyMask,
			FontchooserParentEventHandler, fcdPtr);
	    }
	    fcdPtr->parent = parent;
	    Tk_CreateEventHandler(fcdPtr->parent, StructureNotifyMask,
		    FontchooserParentEventHandler, fcdPtr);
	    break;
	}
	case FontchooserTitle:
	    if (fcdPtr->titleObj) {
		Tcl_DecrRefCount(fcdPtr->titleObj);
	    }
	    Tcl_GetStringFromObj(objv[i+1], &len);
	    if (len) {
		fcdPtr->titleObj = objv[i+1];
		if (Tcl_IsShared(fcdPtr->titleObj)) {
		    fcdPtr->titleObj = Tcl_DuplicateObj(fcdPtr->titleObj);
		}
		Tcl_IncrRefCount(fcdPtr->titleObj);
	    } else {
		fcdPtr->titleObj = NULL;
	    }
	    break;
	case FontchooserFont:
	    Tcl_GetStringFromObj(objv[i+1], &len);
	    if (len) {
		Tk_Font f = Tk_AllocFontFromObj(interp, tkwin, objv[i+1]);

		if (!f) {
		    return TCL_ERROR;
		}
		[fontPanelFont autorelease];
		fontPanelFont = [TkMacOSXNSFontForFont(f) retain];
		[fontPanelFontAttributes setDictionary:
			TkMacOSXNSFontAttributesForFont(f)];
		[fontPanelFontAttributes removeObjectsForKeys:[NSArray
			arrayWithObjects:NSFontAttributeName,
			NSLigatureAttributeName, NSKernAttributeName, nil]];
		Tk_FreeFont(f);
	    } else {
		[fontPanelFont release];
		fontPanelFont = nil;
		[fontPanelFontAttributes removeAllObjects];
	    }

	    NSFontManager *fm = [NSFontManager sharedFontManager];
	    NSFontPanel *fp = [fm fontPanel:NO];

	    [fp setPanelFont:fontPanelFont isMultiple:NO];
	    [fm setSelectedFont:fontPanelFont isMultiple:NO];
	    [fm setSelectedAttributes:fontPanelFontAttributes
		    isMultiple:NO];
	    if ([fp isVisible]) {
		TkSendVirtualEvent(fcdPtr->parent,
			"TkFontchooserFontChanged", NULL);
	    }
	    break;
	case FontchooserCmd:
	    if (fcdPtr->cmdObj) {
		Tcl_DecrRefCount(fcdPtr->cmdObj);
	    }
	    Tcl_GetStringFromObj(objv[i+1], &len);
	    if (len) {
		fcdPtr->cmdObj = objv[i+1];
		if (Tcl_IsShared(fcdPtr->cmdObj)) {
		    fcdPtr->cmdObj = Tcl_DuplicateObj(fcdPtr->cmdObj);
		}
		Tcl_IncrRefCount(fcdPtr->cmdObj);
	    } else {
		fcdPtr->cmdObj = NULL;
	    }
	    break;
	}
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * FontchooserShowCmd --
 *
 *	Implements the 'tk fontchooser show' ensemble command. The per-interp
 *	configuration data for the dialog is held in an interp associated
 *	structure.
 *
 * Results:
 *	See the user documentation.
 *
 * Side effects:
 *	Font Panel may be shown.
 *
 * ----------------------------------------------------------------------
 */

static int
FontchooserShowCmd(
    ClientData clientData,	/* Main window */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    FontchooserData *fcdPtr = Tcl_GetAssocData(interp, "::tk::fontchooser",
	    NULL);

    if (fcdPtr->parent == None) {
	fcdPtr->parent = (Tk_Window) clientData;
	Tk_CreateEventHandler(fcdPtr->parent, StructureNotifyMask,
		FontchooserParentEventHandler, fcdPtr);
    }

    NSFontManager *fm = [NSFontManager sharedFontManager];
    NSFontPanel *fp = [fm fontPanel:YES];

    if ([fp delegate] != NSApp) {
	[fp setDelegate:NSApp];
    }
    if (![fp isVisible]) {
	[fm orderFrontFontPanel:NSApp];
	TkSendVirtualEvent(fcdPtr->parent, "TkFontchooserVisibility", NULL);
    }
    fontchooserInterp = interp;

    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * FontchooserHideCmd --
 *
 *	Implementation of the 'tk fontchooser hide' ensemble. See the user
 *	documentation for details.
 *
 * Results:
 *	See the user documentation.
 *
 * Side effects:
 *	Font Panel may be hidden.
 *
 * ----------------------------------------------------------------------
 */

static int
FontchooserHideCmd(
    ClientData clientData,	/* Main window */
    Tcl_Interp *interp,
    int objc,
    Tcl_Obj *const objv[])
{
    NSFontPanel *fp = [[NSFontManager sharedFontManager] fontPanel:NO];

    if ([fp isVisible]) {
	[fp orderOut:NSApp];
    }
    return TCL_OK;
}

/*
 * ----------------------------------------------------------------------
 *
 * FontchooserParentEventHandler --
 *
 *	Event handler for StructureNotify events on the font chooser's parent
 *	window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Font chooser parent info is cleared and font panel is hidden.
 *
 * ----------------------------------------------------------------------
 */

static void
FontchooserParentEventHandler(
    ClientData clientData,
    XEvent *eventPtr)
{
    FontchooserData *fcdPtr = clientData;

    if (eventPtr->type == DestroyNotify) {
	Tk_DeleteEventHandler(fcdPtr->parent, StructureNotifyMask,
		FontchooserParentEventHandler, fcdPtr);
	fcdPtr->parent = NULL;
	FontchooserHideCmd(NULL, NULL, 0, NULL);
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * DeleteFontchooserData --
 *
 *	Clean up the font chooser configuration data when the interp is
 *	destroyed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	per-interp configuration data is destroyed.
 *
 * ----------------------------------------------------------------------
 */

static void
DeleteFontchooserData(
    ClientData clientData,
    Tcl_Interp *interp)
{
    FontchooserData *fcdPtr = clientData;

    if (fcdPtr->titleObj) {
	Tcl_DecrRefCount(fcdPtr->titleObj);
    }
    if (fcdPtr->cmdObj) {
	Tcl_DecrRefCount(fcdPtr->cmdObj);
    }
    ckfree(fcdPtr);

    if (fontchooserInterp == interp) {
	fontchooserInterp = NULL;
    }
}

/*
 * ----------------------------------------------------------------------
 *
 * TkInitFontchooser --
 *
 *	Associate the font chooser configuration data with the Tcl interpreter.
 *	There is one font chooser per interp.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	per-interp configuration data is destroyed.
 *
 * ----------------------------------------------------------------------
 */

MODULE_SCOPE int
TkInitFontchooser(
    Tcl_Interp *interp,
    ClientData clientData)
{
    FontchooserData *fcdPtr = ckalloc(sizeof(FontchooserData));

    bzero(fcdPtr, sizeof(FontchooserData));
    Tcl_SetAssocData(interp, "::tk::fontchooser", DeleteFontchooserData,
	    fcdPtr);
    if (!fontPanelFontAttributes) {
	fontPanelFontAttributes = [NSMutableDictionary new];
    }
    return TCL_OK;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
