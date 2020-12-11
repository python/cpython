/*
 * tkMacOSXInit.c --
 *
 *	This file contains Mac OS X -specific interpreter initialization
 *	functions.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright 2001-2009, Apple Inc.
 * Copyright (c) 2005-2009 Daniel A. Steffen <das@users.sourceforge.net>
 * Copyright (c) 2017 Marc Culler
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tkMacOSXPrivate.h"

#include <sys/stat.h>
#include <dlfcn.h>
#include <objc/objc-auto.h>

static char tkLibPath[PATH_MAX + 1] = "";

/*
 * If the App is in an App package, then we want to add the Scripts directory
 * to the auto_path.
 */

static char scriptPath[PATH_MAX + 1] = "";

#pragma mark TKApplication(TKInit)

@implementation TKApplication
@synthesize poolLock = _poolLock;
@synthesize macMinorVersion = _macMinorVersion;
@synthesize isDrawing = _isDrawing;
@end

/*
 * #define this to see a message on stderr whenever _resetAutoreleasePool is
 * called while the pool is locked.
 */
#undef DEBUG_LOCK

@implementation TKApplication(TKInit)
- (void) _resetAutoreleasePool
{
    if ([self poolLock] == 0) {
	[_mainPool drain];
	_mainPool = [NSAutoreleasePool new];
    } else {
#ifdef DEBUG_LOCK
	fprintf(stderr, "Pool is locked with count %d!!!!\n", [self poolLock]);
#endif
    }
}
- (void) _lockAutoreleasePool
{
    [self setPoolLock:[self poolLock] + 1];
}
- (void) _unlockAutoreleasePool
{
    [self setPoolLock:[self poolLock] - 1];
}
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
- (void) _postedNotification: (NSNotification *) notification
{
    TKLog(@"-[%@(%p) %s] %@", [self class], self, _cmd, notification);
}
#endif

- (void) _setupApplicationNotifications
{
    NSNotificationCenter *nc = [NSNotificationCenter defaultCenter];
#define observe(n, s) \
	[nc addObserver:self selector:@selector(s) name:(n) object:nil]
    observe(NSApplicationDidBecomeActiveNotification, applicationActivate:);
    observe(NSApplicationDidResignActiveNotification, applicationDeactivate:);
    observe(NSApplicationDidUnhideNotification, applicationShowHide:);
    observe(NSApplicationDidHideNotification, applicationShowHide:);
    observe(NSApplicationDidChangeScreenParametersNotification, displayChanged:);
    observe(NSTextInputContextKeyboardSelectionDidChangeNotification, keyboardChanged:);
#undef observe
}

-(void)applicationWillFinishLaunching:(NSNotification *)aNotification
{

    /*
     * Initialize notifications.
     */
#ifdef TK_MAC_DEBUG_NOTIFICATIONS
    [[NSNotificationCenter defaultCenter] addObserver:self
	    selector:@selector(_postedNotification:) name:nil object:nil];
#endif
    [self _setupWindowNotifications];
    [self _setupApplicationNotifications];

    /*
     * Construct the menu bar.
     */
    _defaultMainMenu = nil;
    [self _setupMenus];

    /*
     * Initialize event processing.
     */
    TkMacOSXInitAppleEvents(_eventInterp);

    /*
     * Initialize the graphics context.
     */
    TkMacOSXUseAntialiasedText(_eventInterp, -1);
    TkMacOSXInitCGDrawing(_eventInterp, TRUE, 0);
}

-(void)applicationDidFinishLaunching:(NSNotification *)notification
{
    /*
     * It is not safe to force activation of the NSApp until this method is
     * called. Activating too early can cause the menu bar to be unresponsive.
     * The call to activateIgnoringOtherApps was moved here to avoid this.
     * However, with the release of macOS 10.15 (Catalina) this was no longer
     * sufficient.  (See ticket bf93d098d7.)  Apparently apps were being
     * activated automatically, and this was sometimes being done too early.
     * As a workaround we deactivate and then reactivate the app, even though
     * Apple says that "Normally, you shouldn’t invoke this method".
     */

    [NSApp deactivate];
    [NSApp activateIgnoringOtherApps: YES];

    /*
     * Process events to ensure that the root window is fully initialized. See
     * ticket 56a1823c73.
     */

    [NSApp _lockAutoreleasePool];
    while (Tcl_DoOneEvent(TCL_WINDOW_EVENTS| TCL_DONT_WAIT)) {}
    [NSApp _unlockAutoreleasePool];
}

- (void) _setup: (Tcl_Interp *) interp
{
    /*
     * Remember our interpreter.
     */
    _eventInterp = interp;

    /*
     * Install the global autoreleasePool.
     */
    _mainPool = [NSAutoreleasePool new];
    [NSApp setPoolLock:0];

    /*
     * Record the OS version we are running on.
     */
    int minorVersion;
#if MAC_OS_X_VERSION_MAX_ALLOWED < 101000
    Gestalt(gestaltSystemVersionMinor, (SInt32*)&minorVersion);
#else
    NSOperatingSystemVersion systemVersion;
    systemVersion = [[NSProcessInfo processInfo] operatingSystemVersion];
    minorVersion = systemVersion.minorVersion;
#endif
    [NSApp setMacMinorVersion: minorVersion];

    /*
     * We are not drawing right now.
     */

    [NSApp setIsDrawing:NO];

    /*
     * Be our own delegate.
     */

    [self setDelegate:self];

    /*
     * Make sure we are allowed to open windows.
     */

    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    /*
     * If no icon has been set from an Info.plist file, use the Wish icon from
     * the Tk framework.
     */

    NSString *iconFile = [[NSBundle mainBundle] objectForInfoDictionaryKey:
						    @"CFBundleIconFile"];
    if (!iconFile) {
	NSString *path = [NSApp tkFrameworkImagePath:@"Tk.icns"];
	if (path) {
	    NSImage *image = [[NSImage alloc] initWithContentsOfFile:path];
	    if (image) {
		[NSApp setApplicationIconImage:image];
		[image release];
	    }
	}
    }
}

- (NSString *) tkFrameworkImagePath: (NSString *) image
{
    NSString *path = nil;
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    if (tkLibPath[0] != '\0') {
	path = [[NSBundle bundleWithPath:[[NSString stringWithUTF8String:
		tkLibPath] stringByAppendingString:@"/../.."]]
		pathForImageResource:image];
    }
    if (!path) {
	const char *tk_library = Tcl_GetVar2(_eventInterp, "tk_library", NULL,
		TCL_GLOBAL_ONLY);

	if (tk_library) {
	    NSFileManager *fm = [NSFileManager defaultManager];

	    path = [[NSString stringWithUTF8String:tk_library]
		    stringByAppendingFormat:@"/%@", image];
	    if (![fm isReadableFileAtPath:path]) {
		path = [[NSString stringWithUTF8String:tk_library]
			stringByAppendingFormat:@"/../macosx/%@", image];
		if (![fm isReadableFileAtPath:path]) {
		    path = nil;
		}
	    }
	}
    }
#ifdef TK_MAC_DEBUG
    if (!path && getenv("TK_SRCROOT")) {
	path = [[NSString stringWithUTF8String:getenv("TK_SRCROOT")]
		stringByAppendingFormat:@"/macosx/%@", image];
	if (![[NSFileManager defaultManager] isReadableFileAtPath:path]) {
	    path = nil;
	}
    }
#endif
    [path retain];
    [pool drain];
    return path;
}
@end

#pragma mark -

/*
 *----------------------------------------------------------------------
 *
 * TkpInit --
 *
 *	Performs Mac-specific interpreter initialization related to the
 *	tk_library variable.
 *
 * Results:
 *	Returns a standard Tcl result. Leaves an error message or result in
 *	the interp's result.
 *
 * Side effects:
 *	Sets "tk_library" Tcl variable, runs "tk.tcl" script.
 *
 *----------------------------------------------------------------------
 */

int
TkpInit(
    Tcl_Interp *interp)
{
    static int initialized = 0;

    /*
     * Since it is possible for TkInit to be called multiple times and we
     * don't want to do the following initialization multiple times we protect
     * against doing it more than once.
     */

    if (!initialized) {
	struct stat st;

	initialized = 1;

	/*
	 * Initialize/check OS version variable for runtime checks.
	 */

#if MAC_OS_X_VERSION_MIN_REQUIRED < 1060
#   error Mac OS X 10.6 required
#endif

#ifdef TK_FRAMEWORK
	/*
	 * When Tk is in a framework, force tcl_findLibrary to look in the
	 * framework scripts directory.
	 * FIXME: Should we come up with a more generic way of doing this?
	 */

	if (Tcl_MacOSXOpenVersionedBundleResources(interp,
		"com.tcltk.tklibrary", TK_FRAMEWORK_VERSION, 0, PATH_MAX,
		tkLibPath) != TCL_OK) {
            # if 0 /* This is not really an error.  Wish still runs fine. */
	    TkMacOSXDbgMsg("Tcl_MacOSXOpenVersionedBundleResources failed");
	    # endif
	}
#endif

	/*
	 * FIXME: Close stdin & stdout for remote debugging otherwise we will
	 * fight with gdb for stdin & stdout
	 */

	if (getenv("XCNOSTDIN") != NULL) {
	    close(0);
	    close(1);
	}

	/*
	 * Instantiate our NSApplication object. This needs to be done before
	 * we check whether to open a console window.
	 */

	NSAutoreleasePool *pool = [NSAutoreleasePool new];
	[[NSUserDefaults standardUserDefaults] registerDefaults:
		[NSDictionary dictionaryWithObjectsAndKeys:
				  [NSNumber numberWithBool:YES],
			      @"_NSCanWrapButtonTitles",
				   [NSNumber numberWithInt:-1],
			      @"NSStringDrawingTypesetterBehavior",
			      nil]];
	[TKApplication sharedApplication];
	[pool drain];
	[NSApp _setup:interp];

        /*
         * WARNING: The finishLaunching method runs asynchronously, apparently
         * in a separate thread.  This creates a race between the
         * initialization of the NSApplication and the initialization of Tk.
         * If Tk wins the race bad things happen with the root window (see
         * below).  If the NSApplication wins then an AppleEvent created during
         * launch, e.g. by dropping a file icon on the application icon, will
         * be delivered before the procedure meant to to handle the AppleEvent
         * has been defined.  This is now handled by processing the AppleEvent
         * as an idle task.  See tkMacOSXHLEvents.c.
         */

	[NSApp finishLaunching];

        /*
         * Create a Tk event source based on the Appkit event queue.
         */

	Tk_MacOSXSetupTkNotifier();

	/*
	 * If Tk initialization wins the race, the root window is mapped before
         * the NSApplication is initialized.  This can cause bad things to
         * happen.  The root window can open off screen with no way to make it
         * appear on screen until the app icon is clicked.  This will happen if
         * a Tk application opens a modal window in its startup script (see
         * ticket 56a1823c73).  In other cases, an empty root window can open
         * on screen and remain visible for a noticeable amount of time while
         * the Tk initialization finishes (see ticket d1989fb7cf).  The call
         * below forces Tk to block until the Appkit event queue has been
         * created.  This seems to be sufficient to ensure that the
         * NSApplication initialization wins the race, avoiding these bad
         * window behaviors.
	 */

	Tcl_DoOneEvent(TCL_WINDOW_EVENTS | TCL_DONT_WAIT);

	/*
	 * If we don't have a TTY and stdin is a special character file of
	 * length 0, (e.g. /dev/null, which is what Finder sets when double
	 * clicking Wish) then use the Tk based console interpreter.
	 */

	if (getenv("TK_CONSOLE") ||
		(!isatty(0) && (fstat(0, &st) ||
		(S_ISCHR(st.st_mode) && st.st_blocks == 0)))) {
	    Tk_InitConsoleChannels(interp);
	    Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDIN));
	    Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDOUT));
	    Tcl_RegisterChannel(interp, Tcl_GetStdChannel(TCL_STDERR));

	    /*
	     * Only show the console if we don't have a startup script and
	     * tcl_interactive hasn't been set already.
	     */

	    if (Tcl_GetStartupScript(NULL) == NULL) {
		const char *intvar = Tcl_GetVar2(interp,
			"tcl_interactive", NULL, TCL_GLOBAL_ONLY);

		if (intvar == NULL) {
		    Tcl_SetVar2(interp, "tcl_interactive", NULL, "1",
			    TCL_GLOBAL_ONLY);
		}
	    }
	    if (Tk_CreateConsoleWindow(interp) == TCL_ERROR) {
		return TCL_ERROR;
	    }
	}

	/*
	 * Initialize the NSServices object here. Apple's docs say to do this
	 * in applicationDidFinishLaunching, but the Tcl interpreter is not
	 * initialized until this function call.
	 */

	TkMacOSXServices_Init(interp);
    }

    if (tkLibPath[0] != '\0') {
	Tcl_SetVar2(interp, "tk_library", NULL, tkLibPath, TCL_GLOBAL_ONLY);
    }

    if (scriptPath[0] != '\0') {
	Tcl_SetVar2(interp, "auto_path", NULL, scriptPath,
		TCL_GLOBAL_ONLY|TCL_LIST_ELEMENT|TCL_APPEND_VALUE);
    }

    Tcl_CreateObjCommand(interp, "::tk::mac::standardAboutPanel",
	    TkMacOSXStandardAboutPanelObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tk::mac::iconBitmap",
	    TkMacOSXIconBitmapObjCmd, NULL, NULL);
    Tcl_CreateObjCommand(interp, "::tk::mac::GetAppPath", TkMacOSXGetAppPath, NULL, NULL);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetAppName --
 *
 *	Retrieves the name of the current application from a platform specific
 *	location. For Unix, the application name is the tail of the path
 *	contained in the tcl variable argv0.
 *
 * Results:
 *	Returns the application name in the given Tcl_DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpGetAppName(
    Tcl_Interp *interp,
    Tcl_DString *namePtr)	/* A previously initialized Tcl_DString. */
{
    const char *p, *name;

    name = Tcl_GetVar2(interp, "argv0", NULL, TCL_GLOBAL_ONLY);
    if ((name == NULL) || (*name == 0)) {
	name = "tk";
    } else {
	p = strrchr(name, '/');
	if (p != NULL) {
	    name = p+1;
	}
    }
    Tcl_DStringAppend(namePtr, name, -1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetAppPath --
 *
 *	Returns the path of the Wish application bundle.
 *
 * Results:
 *	Returns the application path.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
int TkMacOSXGetAppPath(
		       ClientData cd,
		       Tcl_Interp *ip,
		       int objc,
		       Tcl_Obj *const objv[])
{

  CFURLRef mainBundleURL = CFBundleCopyBundleURL(CFBundleGetMainBundle());


  /*
   * Convert the URL reference into a string reference.
   */

  CFStringRef appPath = CFURLCopyFileSystemPath(mainBundleURL, kCFURLPOSIXPathStyle);

  /*
   * Get the system encoding method.
   */

  CFStringEncoding encodingMethod = CFStringGetSystemEncoding();

  /*
   * Convert the string reference into a C string.
   */

  char *path = (char *) CFStringGetCStringPtr(appPath, encodingMethod);

  Tcl_SetResult(ip, path, NULL);

  CFRelease(mainBundleURL);
  CFRelease(appPath);
  return TCL_OK;

}

/*
 *----------------------------------------------------------------------
 *
 * TkpDisplayWarning --
 *
 *	This routines is called from Tk_Main to display warning messages that
 *	occur during startup.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates messages on stdout.
 *
 *----------------------------------------------------------------------
 */

void
TkpDisplayWarning(
    const char *msg,		/* Message to be displayed. */
    const char *title)		/* Title of warning. */
{
    Tcl_Channel errChannel = Tcl_GetStdChannel(TCL_STDERR);

    if (errChannel) {
	Tcl_WriteChars(errChannel, title, -1);
	Tcl_WriteChars(errChannel, ": ", 2);
	Tcl_WriteChars(errChannel, msg, -1);
	Tcl_WriteChars(errChannel, "\n", 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXDefaultStartupScript --
 *
 *	On MacOS X, we look for a file in the Resources/Scripts directory
 *	called AppMain.tcl and if found, we set argv[1] to that, so that the
 *	rest of the code will find it, and add the Scripts folder to the
 *	auto_path. If we don't find the startup script, we just bag it,
 *	assuming the user is starting up some other way.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Tcl_SetStartupScript() called when AppMain.tcl found.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE void
TkMacOSXDefaultStartupScript(void)
{
    NSAutoreleasePool *pool = [NSAutoreleasePool new];
    CFBundleRef bundleRef = CFBundleGetMainBundle();

    if (bundleRef != NULL) {
	CFURLRef appMainURL = CFBundleCopyResourceURL(bundleRef,
		CFSTR("AppMain"), CFSTR("tcl"), CFSTR("Scripts"));

	if (appMainURL != NULL) {
	    CFURLRef scriptFldrURL;
	    char startupScript[PATH_MAX + 1];

	    if (CFURLGetFileSystemRepresentation (appMainURL, true,
		    (unsigned char *) startupScript, PATH_MAX)) {
		Tcl_SetStartupScript(Tcl_NewStringObj(startupScript,-1), NULL);
		scriptFldrURL = CFURLCreateCopyDeletingLastPathComponent(NULL,
			appMainURL);
		if (scriptFldrURL != NULL) {
		    CFURLGetFileSystemRepresentation(scriptFldrURL, true,
			    (unsigned char *) scriptPath, PATH_MAX);
		    CFRelease(scriptFldrURL);
		}
	    }
	    CFRelease(appMainURL);
	}
    }
    [pool drain];
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetNamedSymbol --
 *
 *	Dynamically acquire address of a named symbol from a loaded dynamic
 *	library, so that we can use API that may not be available on all OS
 *	versions.
 *
 * Results:
 *	Address of given symbol or NULL if unavailable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE void*
TkMacOSXGetNamedSymbol(
    const char* module,
    const char* symbol)
{
    void *addr = dlsym(RTLD_NEXT, symbol);
    if (!addr) {
	(void) dlerror(); /* Clear dlfcn error state */
    }
    return addr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXGetStringObjFromCFString --
 *
 *	Get a string object from a CFString as efficiently as possible.
 *
 * Results:
 *	New string object or NULL if conversion failed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

MODULE_SCOPE Tcl_Obj*
TkMacOSXGetStringObjFromCFString(
    CFStringRef str)
{
    Tcl_Obj *obj = NULL;
    const char *c = CFStringGetCStringPtr(str, kCFStringEncodingUTF8);

    if (c) {
	obj = Tcl_NewStringObj(c, -1);
    } else {
	CFRange all = CFRangeMake(0, CFStringGetLength(str));
	CFIndex len;

	if (CFStringGetBytes(str, all, kCFStringEncodingUTF8, 0, false, NULL,
		0, &len) > 0 && len < INT_MAX) {
	    obj = Tcl_NewObj();
	    Tcl_SetObjLength(obj, len);
	    CFStringGetBytes(str, all, kCFStringEncodingUTF8, 0, false,
		    (UInt8*) obj->bytes, len, NULL);
	}
    }
    return obj;
}

/*
 * Local Variables:
 * mode: objc
 * c-basic-offset: 4
 * fill-column: 79
 * coding: utf-8
 * End:
 */
