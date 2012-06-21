/*
 * Copyright (C) 2011-2012 Vinay Sajip.
 * Licensed to PSF under a contributor agreement.
 */
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include "associator.h"

#define PYTHON_EXECUTABLE L"python.exe"

#define MSGSIZE 1024
#define MAX_VERSION_SIZE    4

typedef struct {
    wchar_t version[MAX_VERSION_SIZE]; /* m.n */
    int bits;   /* 32 or 64 */
    wchar_t executable[MAX_PATH];
} INSTALLED_PYTHON;

/*
 * To avoid messing about with heap allocations, just assume we can allocate
 * statically and never have to deal with more versions than this.
 */
#define MAX_INSTALLED_PYTHONS   100

static INSTALLED_PYTHON installed_pythons[MAX_INSTALLED_PYTHONS];

static size_t num_installed_pythons = 0;

/* to hold SOFTWARE\Python\PythonCore\X.Y\InstallPath */
#define IP_BASE_SIZE 40
#define IP_SIZE (IP_BASE_SIZE + MAX_VERSION_SIZE)
#define CORE_PATH L"SOFTWARE\\Python\\PythonCore"

static wchar_t * location_checks[] = {
    L"\\",
/*
    L"\\PCBuild\\",
    L"\\PCBuild\\amd64\\",
 */
    NULL
};

static wchar_t *
skip_whitespace(wchar_t * p)
{
    while (*p && isspace(*p))
        ++p;
    return p;
}

/*
 * This function is here to minimise Visual Studio
 * warnings about security implications of getenv, and to
 * treat blank values as if they are absent.
 */
static wchar_t * get_env(wchar_t * key)
{
    wchar_t * result = _wgetenv(key);

    if (result) {
        result = skip_whitespace(result);
        if (*result == L'\0')
            result = NULL;
    }
    return result;
}

static FILE * log_fp = NULL;

static void
debug(wchar_t * format, ...)
{
    va_list va;

    if (log_fp != NULL) {
        va_start(va, format);
        vfwprintf_s(log_fp, format, va);
    }
}

static void winerror(int rc, wchar_t * message, int size)
{
    FormatMessageW(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, rc, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        message, size, NULL);
}

static INSTALLED_PYTHON *
find_existing_python(wchar_t * path)
{
    INSTALLED_PYTHON * result = NULL;
    size_t i;
    INSTALLED_PYTHON * ip;

    for (i = 0, ip = installed_pythons; i < num_installed_pythons; i++, ip++) {
        if (_wcsicmp(path, ip->executable) == 0) {
            result = ip;
            break;
        }
    }
    return result;
}

static void
locate_pythons_for_key(HKEY root, REGSAM flags)
{
    HKEY core_root, ip_key;
    LSTATUS status = RegOpenKeyExW(root, CORE_PATH, 0, flags, &core_root);
    wchar_t message[MSGSIZE];
    DWORD i;
    size_t n;
    BOOL ok;
    DWORD type, data_size, attrs;
    INSTALLED_PYTHON * ip, * pip;
    wchar_t ip_path[IP_SIZE];
    wchar_t * check;
    wchar_t ** checkp;
    wchar_t *key_name = (root == HKEY_LOCAL_MACHINE) ? L"HKLM" : L"HKCU";

    if (status != ERROR_SUCCESS)
        debug(L"locate_pythons_for_key: unable to open PythonCore key in %s\n",
              key_name);
    else {
        ip = &installed_pythons[num_installed_pythons];
        for (i = 0; num_installed_pythons < MAX_INSTALLED_PYTHONS; i++) {
            status = RegEnumKeyW(core_root, i, ip->version, MAX_VERSION_SIZE);
            if (status != ERROR_SUCCESS) {
                if (status != ERROR_NO_MORE_ITEMS) {
                    /* unexpected error */
                    winerror(status, message, MSGSIZE);
                    debug(L"Can't enumerate registry key for version %s: %s\n",
                          ip->version, message);
                }
                break;
            }
            else {
                _snwprintf_s(ip_path, IP_SIZE, _TRUNCATE,
                             L"%s\\%s\\InstallPath", CORE_PATH, ip->version);
                status = RegOpenKeyExW(root, ip_path, 0, flags, &ip_key);
                if (status != ERROR_SUCCESS) {
                    winerror(status, message, MSGSIZE);
                    // Note: 'message' already has a trailing \n
                    debug(L"%s\\%s: %s", key_name, ip_path, message);
                    continue;
                }
                data_size = sizeof(ip->executable) - 1;
                status = RegQueryValueEx(ip_key, NULL, NULL, &type,
                                         (LPBYTE) ip->executable, &data_size);
                RegCloseKey(ip_key);
                if (status != ERROR_SUCCESS) {
                    winerror(status, message, MSGSIZE);
                    debug(L"%s\\%s: %s\n", key_name, ip_path, message);
                    continue;
                }
                if (type == REG_SZ) {
                    data_size = data_size / sizeof(wchar_t) - 1;  /* for NUL */
                    if (ip->executable[data_size - 1] == L'\\')
                        --data_size; /* reg value ended in a backslash */
                    /* ip->executable is data_size long */
                    for (checkp = location_checks; *checkp; ++checkp) {
                        check = *checkp;
                        _snwprintf_s(&ip->executable[data_size],
                                     MAX_PATH - data_size,
                                     MAX_PATH - data_size,
                                     L"%s%s", check, PYTHON_EXECUTABLE);
                        attrs = GetFileAttributesW(ip->executable);
                        if (attrs == INVALID_FILE_ATTRIBUTES) {
                            winerror(GetLastError(), message, MSGSIZE);
                            debug(L"locate_pythons_for_key: %s: %s",
                                  ip->executable, message);
                        }
                        else if (attrs & FILE_ATTRIBUTE_DIRECTORY) {
                            debug(L"locate_pythons_for_key: '%s' is a \
directory\n",
                                  ip->executable, attrs);
                        }
                        else if (find_existing_python(ip->executable)) {
                            debug(L"locate_pythons_for_key: %s: already \
found: %s\n", ip->executable);
                        }
                        else {
                            /* check the executable type. */
                            ok = GetBinaryTypeW(ip->executable, &attrs);
                            if (!ok) {
                                debug(L"Failure getting binary type: %s\n",
                                      ip->executable);
                            }
                            else {
                                if (attrs == SCS_64BIT_BINARY)
                                    ip->bits = 64;
                                else if (attrs == SCS_32BIT_BINARY)
                                    ip->bits = 32;
                                else
                                    ip->bits = 0;
                                if (ip->bits == 0) {
                                    debug(L"locate_pythons_for_key: %s: \
invalid binary type: %X\n",
                                          ip->executable, attrs);
                                }
                                else {
                                    if (wcschr(ip->executable, L' ') != NULL) {
                                        /* has spaces, so quote */
                                        n = wcslen(ip->executable);
                                        memmove(&ip->executable[1],
                                                ip->executable, n * sizeof(wchar_t));
                                        ip->executable[0] = L'\"';
                                        ip->executable[n + 1] = L'\"';
                                        ip->executable[n + 2] = L'\0';
                                    }
                                    debug(L"locate_pythons_for_key: %s \
is a %dbit executable\n",
                                        ip->executable, ip->bits);
                                    ++num_installed_pythons;
                                    pip = ip++;
                                    if (num_installed_pythons >=
                                        MAX_INSTALLED_PYTHONS)
                                        break;
                                    /* Copy over the attributes for the next */
                                    *ip = *pip;
                                }
                            }
                        }
                    }
                }
            }
        }
        RegCloseKey(core_root);
    }
}

static int
compare_pythons(const void * p1, const void * p2)
{
    INSTALLED_PYTHON * ip1 = (INSTALLED_PYTHON *) p1;
    INSTALLED_PYTHON * ip2 = (INSTALLED_PYTHON *) p2;
    /* note reverse sorting on version */
    int result = wcscmp(ip2->version, ip1->version);

    if (result == 0)
        result = ip2->bits - ip1->bits; /* 64 before 32 */
    return result;
}

static void
locate_all_pythons()
{
#if defined(_M_X64)
    // If we are a 64bit process, first hit the 32bit keys.
    debug(L"locating Pythons in 32bit registry\n");
    locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ | KEY_WOW64_32KEY);
    locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_32KEY);
#else
    // If we are a 32bit process on a 64bit Windows, first hit the 64bit keys.
    BOOL f64 = FALSE;
    if (IsWow64Process(GetCurrentProcess(), &f64) && f64) {
        debug(L"locating Pythons in 64bit registry\n");
        locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ | KEY_WOW64_64KEY);
        locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ | KEY_WOW64_64KEY);
    }
#endif    
    // now hit the "native" key for this process bittedness.
    debug(L"locating Pythons in native registry\n");
    locate_pythons_for_key(HKEY_CURRENT_USER, KEY_READ);
    locate_pythons_for_key(HKEY_LOCAL_MACHINE, KEY_READ);
    qsort(installed_pythons, num_installed_pythons, sizeof(INSTALLED_PYTHON),
          compare_pythons);
}

typedef struct {
    wchar_t * path;
    wchar_t * key;
    wchar_t * value;
} REGISTRY_ENTRY;

static REGISTRY_ENTRY registry_entries[] = {
    { L".py", NULL, L"Python.File" },
    { L".pyc", NULL, L"Python.CompiledFile" },
    { L".pyo", NULL, L"Python.CompiledFile" },
    { L".pyw", NULL, L"Python.NoConFile" },

    { L"Python.CompiledFile", NULL, L"Compiled Python File" },
    { L"Python.CompiledFile\\DefaultIcon", NULL, L"pyc.ico" },
    { L"Python.CompiledFile\\shell\\open", NULL, L"Open" },
    { L"Python.CompiledFile\\shell\\open\\command", NULL, L"python.exe" },

    { L"Python.File", NULL, L"Python File" },
    { L"Python.File\\DefaultIcon", NULL, L"py.ico" },
    { L"Python.File\\shell\\open", NULL, L"Open" },
    { L"Python.File\\shell\\open\\command", NULL, L"python.exe" },

    { L"Python.NoConFile", NULL, L"Python File (no console)" },
    { L"Python.NoConFile\\DefaultIcon", NULL, L"py.ico" },
    { L"Python.NoConFile\\shell\\open", NULL, L"Open" },
    { L"Python.NoConFile\\shell\\open\\command", NULL, L"pythonw.exe" },

    { NULL }
};

static BOOL
do_association(INSTALLED_PYTHON * ip)
{
    LONG rc;
    BOOL result = TRUE;
    REGISTRY_ENTRY * rp = registry_entries;
    wchar_t value[MAX_PATH];
    wchar_t root[MAX_PATH];
    wchar_t message[MSGSIZE];
    wchar_t * pvalue;
    HKEY hKey;
    DWORD len;

    wcsncpy_s(root, MAX_PATH, ip->executable, _TRUNCATE);
    pvalue = wcsrchr(root, '\\');
    if (pvalue)
        *pvalue = L'\0';

    for (; rp->path; ++rp) {
        if (wcsstr(rp->path, L"DefaultIcon")) {
            pvalue = value;
            _snwprintf_s(value, MAX_PATH, _TRUNCATE,
                         L"%s\\DLLs\\%s", root, rp->value);
        }
        else if (wcsstr(rp->path, L"open\\command")) {
            pvalue = value;
            _snwprintf_s(value, MAX_PATH, _TRUNCATE,
                         L"%s\\%s \"%%1\" %%*", root, rp->value);
        }
        else {
            pvalue = rp->value;
        }
        /* use rp->path, rp->key, pvalue */
        /* NOTE: size is in bytes */
        len = (DWORD) ((1 + wcslen(pvalue)) * sizeof(wchar_t));
        rc = RegOpenKeyEx(HKEY_CLASSES_ROOT, rp->path, 0, KEY_SET_VALUE, &hKey);
        if (rc == ERROR_SUCCESS) {
            rc = RegSetValueExW(hKey, rp->key, 0, REG_SZ, (LPBYTE) pvalue, len);
            RegCloseKey(hKey);
        }
        if (rc != ERROR_SUCCESS) {
            winerror(rc, message, MSGSIZE);
            MessageBoxW(NULL, message, L"Unable to set file associations", MB_OK | MB_ICONSTOP);
            result = FALSE;
            break;
        }
    }
    return result;
}

static BOOL
associations_exist()
{
    BOOL result = FALSE;
    REGISTRY_ENTRY * rp = registry_entries;
    wchar_t buffer[MSGSIZE];
    LONG csize = MSGSIZE * sizeof(wchar_t);
    LONG rc;

    /* Currently, if any is found, we assume they're all there. */

    for (; rp->path; ++rp) {
        LONG size = csize;
        rc = RegQueryValueW(HKEY_CLASSES_ROOT, rp->path, buffer, &size); 
        if (rc == ERROR_SUCCESS) {
            result = TRUE;
            break;
        }
    }
    return result;
}

/* --------------------------------------------------------------------*/

static BOOL CALLBACK
find_by_title(HWND hwnd, LPARAM lParam)
{
    wchar_t buffer[MSGSIZE];
    BOOL not_found = TRUE;

    wchar_t * p = (wchar_t *) GetWindowTextW(hwnd, buffer, MSGSIZE);
    if (wcsstr(buffer, L"Python Launcher") == buffer) {
        not_found = FALSE;
        *((HWND *) lParam) = hwnd;
    }
    return not_found;
}

static HWND
find_installer_window()
{
    HWND result = NULL;
    BOOL found = EnumWindows(find_by_title, (LPARAM) &result);

    return result;
}

static void
centre_window_in_front(HWND hwnd)
{
    HWND hwndParent;
    RECT rect, rectP;
    int width, height;      
    int screenwidth, screenheight;
    int x, y;

    //make the window relative to its parent

    screenwidth  = GetSystemMetrics(SM_CXSCREEN);
    screenheight = GetSystemMetrics(SM_CYSCREEN);

    hwndParent = GetParent(hwnd);

    GetWindowRect(hwnd, &rect);
    if (hwndParent) {
        GetWindowRect(hwndParent, &rectP);
    }
    else {
        rectP.left = rectP.top = 0;
        rectP.right = screenwidth;
        rectP.bottom = screenheight;
    }

    width  = rect.right  - rect.left;
    height = rect.bottom - rect.top;

    x = ((rectP.right-rectP.left) -  width) / 2 + rectP.left;
    y = ((rectP.bottom-rectP.top) - height) / 2 + rectP.top;


    //make sure that the dialog box never moves outside of
    //the screen

    if (x < 0)
        x = 0;

    if (y < 0)
        y = 0;

    if (x + width  > screenwidth)
        x = screenwidth  - width;
    if (y + height > screenheight)
        y = screenheight - height;

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
}

static void
init_list(HWND hList)
{
    LVCOLUMNW column;
    LVITEMW item;
    int colno = 0;
    int width = 0;
    int row;
    size_t i;
    INSTALLED_PYTHON * ip;
    RECT r;
    LPARAM style;

    GetClientRect(hList, &r);

    style = SendMessage(hList, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    SendMessage(hList, LVM_SETEXTENDEDLISTVIEWSTYLE,
                0, style | LVS_EX_FULLROWSELECT);

    /* First set up the columns */
    memset(&column, 0, sizeof(column));
    column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
    column.pszText = L"Version";
    column.cx = 60;
    width += column.cx;
    SendMessage(hList, LVM_INSERTCOLUMN, colno++,(LPARAM) &column);
#if defined(_M_X64)
    column.pszText = L"Bits";
    column.cx = 40;
    column.iSubItem = colno;
    SendMessage(hList, LVM_INSERTCOLUMN, colno++,(LPARAM) &column);
    width += column.cx;
#endif
    column.pszText = L"Path";
    column.cx = r.right - r.top - width;
    column.iSubItem = colno;
    SendMessage(hList, LVM_INSERTCOLUMN, colno++,(LPARAM) &column);

    /* Then insert the rows */
    memset(&item, 0, sizeof(item));
    item.mask = LVIF_TEXT;
    for (i = 0, ip = installed_pythons;  i < num_installed_pythons; i++,ip++) {
        item.iItem = (int) i;
        item.iSubItem = 0;
        item.pszText = ip->version;
        colno = 0;
        row = (int) SendMessage(hList, LVM_INSERTITEM, 0, (LPARAM) &item);
#if defined(_M_X64)
        item.iSubItem = ++colno;
        item.pszText = (ip->bits == 64) ? L"64": L"32";
        SendMessage(hList, LVM_SETITEM, row, (LPARAM) &item);
#endif
        item.iSubItem = ++colno;
        item.pszText = ip->executable;
        SendMessage(hList, LVM_SETITEM, row, (LPARAM) &item);
    }
}

/* ----------------------------------------------------------------*/

typedef int (__stdcall *MSGBOXWAPI)(IN HWND hWnd, 
        IN LPCWSTR lpText, IN LPCWSTR lpCaption, 
        IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);

int MessageBoxTimeoutW(IN HWND hWnd, IN LPCWSTR lpText, 
    IN LPCWSTR lpCaption, IN UINT uType, 
    IN WORD wLanguageId, IN DWORD dwMilliseconds);

#define MB_TIMEDOUT 32000

int MessageBoxTimeoutW(HWND hWnd, LPCWSTR lpText, 
    LPCWSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
    static MSGBOXWAPI MsgBoxTOW = NULL;

    if (!MsgBoxTOW) {
        HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
        if (hUser32)
            MsgBoxTOW = (MSGBOXWAPI)GetProcAddress(hUser32, 
                                      "MessageBoxTimeoutW");
        else {
            //stuff happened, add code to handle it here 
            //(possibly just call MessageBox())
            return 0;
        }
    }

    if (MsgBoxTOW)
        return MsgBoxTOW(hWnd, lpText, lpCaption, uType, wLanguageId,
                         dwMilliseconds);

    return 0;
}
/* ----------------------------------------------------------------*/

static INT_PTR CALLBACK
DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    HWND hList;
    HWND hChild;
    static int selected_index = -1;
    WORD low = LOWORD(wParam);
    wchar_t confirmation[MSGSIZE];
    BOOL result = FALSE;

    debug(L"DialogProc entry: 0x%02X\n", message);
    switch (message) {
    case WM_INITDIALOG:
        hList = GetDlgItem(hDlg, IDC_LIST1);
        init_list(hList);
        SetFocus(hList);
        result = TRUE;
        break;
    case WM_COMMAND:
        if((low == IDOK) || (low == IDCANCEL)) {
            HMODULE hUser32 = LoadLibraryW(L"user32.dll");

            if (low == IDCANCEL)
                wcsncpy_s(confirmation, MSGSIZE, L"No association was \
performed.", _TRUNCATE);
            else {
                if (selected_index < 0) {
                /* should never happen */
                    wcsncpy_s(confirmation, MSGSIZE, L"The Python version to \
associate with couldn't be determined.", _TRUNCATE);
                }
                else {
                    INSTALLED_PYTHON * ip = &installed_pythons[selected_index];

                    /* Do the association and set the message. */
                    do_association(ip);
                    _snwprintf_s(confirmation, MSGSIZE, _TRUNCATE,
                                 L"Associated Python files with the Python %s \
found at '%s'", ip->version, ip->executable);
                }
            }

            if (hUser32) {
                MessageBoxTimeoutW(hDlg,
                                   confirmation, 
                                   L"Association Status",
                                   MB_OK | MB_SETFOREGROUND |
                                   MB_ICONINFORMATION,
                                   0, 2000);
                FreeLibrary(hUser32);
            }
            PostQuitMessage(0);
            EndDialog(hDlg, 0);
            result = TRUE;
        }
        break;
    case WM_NOTIFY:
        if (low == IDC_LIST1) {
            NMLISTVIEW * p = (NMLISTVIEW *) lParam;

            if ((p->hdr.code == LVN_ITEMCHANGED) &&
                (p->uNewState & LVIS_SELECTED)) {
                hChild = GetDlgItem(hDlg, IDOK);
                selected_index = p->iItem;
                EnableWindow(hChild, selected_index >= 0);
            }
            result = TRUE;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        result = TRUE;
        break;
    case WM_CLOSE:
        DestroyWindow(hDlg);
        result = TRUE;
        break;
    }
    debug(L"DialogProc exit: %d\n", result);
    return result;
}

int WINAPI wWinMain(HINSTANCE hInstance,
                   HINSTANCE hPrevInstance,
                   LPWSTR lpCmdLine, int nShow)
{
    MSG  msg;
    HWND hDialog = 0;
    HICON hIcon;
    HWND hParent;
    int status;
    DWORD dw;
    INITCOMMONCONTROLSEX icx;
    wchar_t * wp;

    wp = get_env(L"PYASSOC_DEBUG");
    if ((wp != NULL) && (*wp != L'\0')) {
        fopen_s(&log_fp, "c:\\temp\\associator.log", "w");
    }

    if (!lpCmdLine) {
        debug(L"No command line specified.\n");
        return 0;
    }
    if (!wcsstr(lpCmdLine, L"nocheck") &&
        associations_exist())   /* Could have been restored by uninstall. */
        return 0;

    locate_all_pythons();

    if (num_installed_pythons == 0)
        return 0;

    debug(L"%d pythons found.\n", num_installed_pythons);

    /*
     * OK, now there's something to do.
     *
     * We need to find the installer window to be the parent of
     * our dialog, otherwise our dialog will be behind it.
     *
     * First, initialize common controls. If you don't - on
     * some machines it works fine, on others the dialog never
     * appears!
     */

    icx.dwSize = sizeof(icx);
    icx.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icx);

    hParent = find_installer_window();
    debug(L"installer window: %X\n", hParent);
    hDialog = CreateDialogW(hInstance, MAKEINTRESOURCE(DLG_MAIN), hParent,
                            DialogProc);
    dw = GetLastError();
    debug(L"dialog created: %X: error: %X\n", hDialog, dw);

    if (!hDialog)
    {
        wchar_t buf [100];
        _snwprintf_s(buf, 100, _TRUNCATE, L"Error 0x%x", GetLastError());
        MessageBoxW(0, buf, L"CreateDialog", MB_ICONEXCLAMATION | MB_OK);
        return 1;
    }

    centre_window_in_front(hDialog);
    hIcon = LoadIcon( GetModuleHandle(NULL), MAKEINTRESOURCE(DLG_ICON));
    if( hIcon )
    {
       SendMessage(hDialog, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
       SendMessage(hDialog, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);
       DestroyIcon(hIcon);
    }

    while ((status = GetMessage (& msg, 0, 0, 0)) != 0)
    {
        if (status == -1)
            return -1;
        if (!IsDialogMessage(hDialog, & msg))
        {
            TranslateMessage( & msg );
            DispatchMessage( & msg );
        }
    }

    return (int) msg.wParam;
}
