/* FreezeDLLMain.cpp

This is a DLLMain suitable for frozen applications/DLLs on
a Windows platform.

The general problem is that many Python extension modules may define
DLL main functions, but when statically linked together to form
a frozen application, this DLLMain symbol exists multiple times.

The solution is:
* Each module checks for a frozen build, and if so, defines its DLLMain
  function as "__declspec(dllexport) DllMain%module%" 
  (eg, DllMainpythoncom, or DllMainpywintypes)

* The frozen .EXE/.DLL links against this module, which provides
  the single DllMain.

* This DllMain attempts to locate and call the DllMain for each
  of the extension modules.

* This code also has hooks to "simulate" DllMain when used from
  a frozen .EXE.

At this stage, there is a static table of "possibly embedded modules".
This should change to something better, but it will work OK for now.

Note that this scheme does not handle dependencies in the order
of DllMain calls - except it does call pywintypes first :-)

As an example of how an extension module with a DllMain should be
changed, here is a snippet from the pythoncom extension module.

  // end of example code from pythoncom's DllMain.cpp
  #ifndef BUILD_FREEZE
  #define DLLMAIN DllMain
  #define DLLMAIN_DECL
  #else
  #define DLLMAIN DllMainpythoncom
  #define DLLMAIN_DECL __declspec(dllexport)
  #endif

  extern "C" DLLMAIN_DECL
  BOOL WINAPI DLLMAIN(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
  // end of example code from pythoncom's DllMain.cpp

***************************************************************************/
#include "windows.h"

static char *possibleModules[] = {
	"pywintypes",
	"pythoncom",
	"win32ui",
	NULL,
};

BOOL CallModuleDllMain(char *modName, DWORD dwReason);


/*
  Called by a frozen .EXE only, so that built-in extension
  modules are initialized correctly
*/
void PyWinFreeze_ExeInit(void)
{
	char **modName;
	for (modName = possibleModules;*modName;*modName++) {
/*		printf("Initialising '%s'\n", *modName); */
		CallModuleDllMain(*modName, DLL_PROCESS_ATTACH);
	}
}

/*
  Called by a frozen .EXE only, so that built-in extension
  modules are cleaned up 
*/
void PyWinFreeze_ExeTerm(void)
{
	// Must go backwards
	char **modName;
	for (modName = possibleModules+(sizeof(possibleModules) / sizeof(char *))-2;
	     modName >= possibleModules;
	     *modName--) {
/*		printf("Terminating '%s'\n", *modName);*/
		CallModuleDllMain(*modName, DLL_PROCESS_DETACH);
	}
}

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	BOOL ret = TRUE;
	switch (dwReason) {
		case DLL_PROCESS_ATTACH: 
		{
			char **modName;
			for (modName = possibleModules;*modName;*modName++) {
				BOOL ok = CallModuleDllMain(*modName, dwReason);
				if (!ok)
					ret = FALSE;
			}
			break;
		}
		case DLL_PROCESS_DETACH: 
		{
			// Must go backwards
			char **modName;
			for (modName = possibleModules+(sizeof(possibleModules) / sizeof(char *))-2;
			     modName >= possibleModules;
			     *modName--)
				CallModuleDllMain(*modName, DLL_PROCESS_DETACH);
			break;
		}
	}
	return ret;
}

BOOL CallModuleDllMain(char *modName, DWORD dwReason)
{
	BOOL (WINAPI * pfndllmain)(HINSTANCE, DWORD, LPVOID);

	char funcName[255];
	HMODULE hmod = GetModuleHandle(NULL);
	strcpy(funcName, "_DllMain");
	strcat(funcName, modName);
	strcat(funcName, "@12"); // stdcall convention.
	pfndllmain = (BOOL (WINAPI *)(HINSTANCE, DWORD, LPVOID))GetProcAddress(hmod, funcName);
	if (pfndllmain==NULL) {
		/* No function by that name exported - then that module does
 		   not appear in our frozen program - return OK
                */
		return TRUE;
	}
	return (*pfndllmain)(hmod, dwReason, NULL);
}

