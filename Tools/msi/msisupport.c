#include "windows.h"
#include "msiquery.h"

/* Print a debug message to the installer log file.
 * To see the debug messages, install with
 * msiexec /i pythonxy.msi /l*v python.log
 */
static UINT debug(MSIHANDLE hInstall, LPCSTR msg)
{
    MSIHANDLE hRec = MsiCreateRecord(1);
    if (!hRec || MsiRecordSetStringA(hRec, 1, msg) != ERROR_SUCCESS) {
        return ERROR_INSTALL_FAILURE;
    }
    MsiProcessMessage(hInstall, INSTALLMESSAGE_INFO, hRec);
    MsiCloseHandle(hRec);
    return ERROR_SUCCESS;
}

/* Check whether the TARGETDIR exists and is a directory.
 * Set TargetExists appropriately.
 */
UINT __declspec(dllexport) __stdcall CheckDir(MSIHANDLE hInstall)
{
#define PSIZE 1024
    WCHAR wpath[PSIZE];
    char path[PSIZE];
    UINT result;
    DWORD size = PSIZE;
    DWORD attributes;


    result = MsiGetPropertyW(hInstall, L"TARGETDIR", wpath, &size);
    if (result != ERROR_SUCCESS)
        return result;
    wpath[size] = L'\0';
    path[size] = L'\0';

    attributes = GetFileAttributesW(wpath);
    if (attributes == INVALID_FILE_ATTRIBUTES ||
        !(attributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        return MsiSetPropertyA(hInstall, "TargetExists", "0");
    } else {
        return MsiSetPropertyA(hInstall, "TargetExists", "1");
    }
}

/* Update the state of the REGISTRY.tcl component according to the
 * Extension and TclTk features. REGISTRY.tcl must be installed
 * if both features are installed, and must be absent otherwise.
 */
UINT __declspec(dllexport) __stdcall UpdateEditIDLE(MSIHANDLE hInstall)
{
    INSTALLSTATE ext_old, ext_new, tcl_old, tcl_new, reg_new;
    UINT result;

    result = MsiGetFeatureStateA(hInstall, "Extensions", &ext_old, &ext_new);
    if (result != ERROR_SUCCESS)
        return result;
    result = MsiGetFeatureStateA(hInstall, "TclTk", &tcl_old, &tcl_new);
    if (result != ERROR_SUCCESS)
        return result;

    /* If the current state is Absent, and the user did not select
       the feature in the UI, Installer apparently sets the "selected"
       state to unknown. Update it to the current value, then. */
    if (ext_new == INSTALLSTATE_UNKNOWN)
        ext_new = ext_old;
    if (tcl_new == INSTALLSTATE_UNKNOWN)
        tcl_new = tcl_old;

    // XXX consider current state of REGISTRY.tcl?
    if (((tcl_new == INSTALLSTATE_LOCAL) ||
             (tcl_new == INSTALLSTATE_SOURCE) ||
             (tcl_new == INSTALLSTATE_DEFAULT)) &&
        ((ext_new == INSTALLSTATE_LOCAL) ||
         (ext_new == INSTALLSTATE_SOURCE) ||
         (ext_new == INSTALLSTATE_DEFAULT))) {
        reg_new = INSTALLSTATE_SOURCE;
    } else {
        reg_new = INSTALLSTATE_ABSENT;
    }
    result = MsiSetComponentStateA(hInstall, "REGISTRY.tcl", reg_new);
    return result;
}

BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    return TRUE;
}

