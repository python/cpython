/*
** Resources for the sys.path initialization, the Python options
** and the preference filename
*/
#include "Types.r"
#include "patchlevel.h"
#include "pythonresources.h"

/* A few resource type declarations */

type 'Popt' {
	literal byte version = POPT_VERSION_CURRENT;
	byte noInspect = 0, inspect = 1;
	byte noVerbose = 0, verbose = 1;
	byte noOptimize = 0, optimize = 1;
	byte noUnbuffered = 0, unbuffered = 1;
	byte noDebugParser = 0, debugParser = 1;
	byte closeOnNormalExit = 0, noCloseOnNormalExit = 1;
	byte closeOnErrorExit = 0, noCloseOnErrorExit = 1;
	byte interactiveOptions = 0, noInteractiveOptions = 1;
	byte argcArgv = 0, noArgcArgv = 1;
	byte newStandardExceptions = 0, oldStandardExceptions = 1;
	byte sitePython = 0, noSitePython = 1;
};

type 'TMPL' {
	wide array {
		pstring;
		literal longint;
	};
};

/* The resources themselves */

/* Popt template, for editing them in ResEdit */

resource 'TMPL' (PYTHONOPTIONS_ID, "Popt") {
	{
		"preference version",			'DBYT',
		"Interactive after script",		'DBYT',
		"Verbose import",				'DBYT',
		"Optimize",						'DBYT',
		"Unbuffered stdio",				'DBYT',
		"Debug parser",					'DBYT',
		"Keep window on normal exit",	'DBYT',
		"Keep window on error exit",	'DBYT',
		"No interactive option dialog",	'DBYT',
		"No argc/argv emulation",		'DBYT',
		"Old standard exceptions",		'DBYT',
		"No site-python support",		'DBYT',
	}
};

/* The default-default Python options */

resource 'Popt' (PYTHONOPTIONS_ID, "Options") {
	POPT_VERSION_CURRENT,
	noInspect,
	noVerbose,
	noOptimize,
	noUnbuffered,
	noDebugParser,
	closeOnNormalExit,
	noCloseOnErrorExit,
	interactiveOptions,
	argcArgv,
	newStandardExceptions,
	sitePython,
};

/* The sys.path initializer */

resource 'STR#' (PYTHONPATH_ID, "sys.path initialization") {
	{
		"$(PYTHON)",
		"$(PYTHON):Lib",
		"$(PYTHON):Mac:PlugIns",
		"$(PYTHON):Mac:Lib",
		"$(PYTHON):Mac:Lib:lib-toolbox",
		"$(PYTHON):Mac:Lib:lib-scripting",
		"$(PYTHON):Extensions:img:Mac",
		"$(PYTHON):Extensions:img:Lib",
		"$(PYTHON):Extensions:Numerical:Lib"
	}
};

/* The preferences filename */

resource 'STR ' (PREFFILENAME_ID, PREFFILENAME_PASCAL_NAME) {
	$$Format("Python %s Preferences", PY_VERSION)
};
