// Support back to Vista
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#include <sdkddkver.h>

// Use WRL to define a classic COM class
#define __WRL_CLASSIC_COM__
#include <wrl.h>

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <olectl.h>
#include <strsafe.h>

#include "pyshellext_h.h"

#define DDWM_UPDATEWINDOW (WM_USER+3)

static HINSTANCE hModule;
static CLIPFORMAT cfDropDescription;
static CLIPFORMAT cfDragWindow;

static const LPCWSTR CLASS_SUBKEY = L"Software\\Classes\\CLSID\\{BEA218D2-6950-497B-9434-61683EC065FE}";
static const LPCWSTR DRAG_MESSAGE = L"Open with %1";

using namespace Microsoft::WRL;

HRESULT FilenameListCchLengthA(LPCSTR pszSource, size_t cchMax, size_t *pcchLength, size_t *pcchCount) {
    HRESULT hr = S_OK;
    size_t count = 0;
    size_t length = 0;

    while (pszSource && pszSource[0]) {
        size_t oneLength;
        hr = StringCchLengthA(pszSource, cchMax - length, &oneLength);
        if (FAILED(hr)) {
            return hr;
        }
        count += 1;
        length += oneLength + (strchr(pszSource, ' ') ? 3 : 1);
        pszSource = &pszSource[oneLength + 1];
    }

    *pcchCount = count;
    *pcchLength = length;
    return hr;
}

HRESULT FilenameListCchLengthW(LPCWSTR pszSource, size_t cchMax, size_t *pcchLength, size_t *pcchCount) {
    HRESULT hr = S_OK;
    size_t count = 0;
    size_t length = 0;

    while (pszSource && pszSource[0]) {
        size_t oneLength;
        hr = StringCchLengthW(pszSource, cchMax - length, &oneLength);
        if (FAILED(hr)) {
            return hr;
        }
        count += 1;
        length += oneLength + (wcschr(pszSource, ' ') ? 3 : 1);
        pszSource = &pszSource[oneLength + 1];
    }

    *pcchCount = count;
    *pcchLength = length;
    return hr;
}

HRESULT FilenameListCchCopyA(STRSAFE_LPSTR pszDest, size_t cchDest, LPCSTR pszSource, LPCSTR pszSeparator) {
    HRESULT hr = S_OK;
    size_t count = 0;
    size_t length = 0;

    while (pszSource[0]) {
        STRSAFE_LPSTR newDest;

        hr = StringCchCopyExA(pszDest, cchDest, pszSource, &newDest, &cchDest, 0);
        if (FAILED(hr)) {
            return hr;
        }
        pszSource += (newDest - pszDest) + 1;
        pszDest = PathQuoteSpacesA(pszDest) ? newDest + 2 : newDest;

        if (pszSource[0]) {
            hr = StringCchCopyExA(pszDest, cchDest, pszSeparator, &newDest, &cchDest, 0);
            if (FAILED(hr)) {
                return hr;
            }
            pszDest = newDest;
        }
    }

    return hr;
}

HRESULT FilenameListCchCopyW(STRSAFE_LPWSTR pszDest, size_t cchDest, LPCWSTR pszSource, LPCWSTR pszSeparator) {
    HRESULT hr = S_OK;
    size_t count = 0;
    size_t length = 0;

    while (pszSource[0]) {
        STRSAFE_LPWSTR newDest;

        hr = StringCchCopyExW(pszDest, cchDest, pszSource, &newDest, &cchDest, 0);
        if (FAILED(hr)) {
            return hr;
        }
        pszSource += (newDest - pszDest) + 1;
        pszDest = PathQuoteSpacesW(pszDest) ? newDest + 2 : newDest;

        if (pszSource[0]) {
            hr = StringCchCopyExW(pszDest, cchDest, pszSeparator, &newDest, &cchDest, 0);
            if (FAILED(hr)) {
                return hr;
            }
            pszDest = newDest;
        }
    }

    return hr;
}


class PyShellExt : public RuntimeClass<
    RuntimeClassFlags<ClassicCom>,
    IDropTarget,
    IPersistFile
>
{
    LPOLESTR target, target_dir;
    DWORD target_mode;

    IDataObject *data_obj;

public:
    PyShellExt() : target(NULL), target_dir(NULL), target_mode(0), data_obj(NULL) {
        OutputDebugString(L"PyShellExt::PyShellExt");
    }

    ~PyShellExt() {
        if (target) {
            CoTaskMemFree(target);
        }
        if (target_dir) {
            CoTaskMemFree(target_dir);
        }
        if (data_obj) {
            data_obj->Release();
        }
    }

private:
    HRESULT UpdateDropDescription(IDataObject *pDataObj) {
        STGMEDIUM medium;
        FORMATETC fmt = {
            cfDropDescription,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        auto hr = pDataObj->GetData(&fmt, &medium);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::UpdateDropDescription - failed to get DROPDESCRIPTION format");
            return hr;
        }
        if (!medium.hGlobal) {
            OutputDebugString(L"PyShellExt::UpdateDropDescription - DROPDESCRIPTION format had NULL hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }
        auto dd = (DROPDESCRIPTION*)GlobalLock(medium.hGlobal);
        if (!dd) {
            OutputDebugString(L"PyShellExt::UpdateDropDescription - failed to lock DROPDESCRIPTION hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }
        StringCchCopy(dd->szMessage, sizeof(dd->szMessage) / sizeof(dd->szMessage[0]), DRAG_MESSAGE);
        StringCchCopy(dd->szInsert, sizeof(dd->szInsert) / sizeof(dd->szInsert[0]), PathFindFileNameW(target));
        dd->type = DROPIMAGE_MOVE;

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);

        return S_OK;
    }

    HRESULT GetDragWindow(IDataObject *pDataObj, HWND *phWnd) {
        HRESULT hr;
        HWND *pMem;
        STGMEDIUM medium;
        FORMATETC fmt = {
            cfDragWindow,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        hr = pDataObj->GetData(&fmt, &medium);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::GetDragWindow - failed to get DragWindow format");
            return hr;
        }
        if (!medium.hGlobal) {
            OutputDebugString(L"PyShellExt::GetDragWindow - DragWindow format had NULL hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        pMem = (HWND*)GlobalLock(medium.hGlobal);
        if (!pMem) {
            OutputDebugString(L"PyShellExt::GetDragWindow - failed to lock DragWindow hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        *phWnd = *pMem;

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);

        return S_OK;
    }

    HRESULT GetArguments(IDataObject *pDataObj, LPCWSTR *pArguments) {
        HRESULT hr;
        DROPFILES *pdropfiles;

        STGMEDIUM medium;
        FORMATETC fmt = {
            CF_HDROP,
            NULL,
            DVASPECT_CONTENT,
            -1,
            TYMED_HGLOBAL
        };

        hr = pDataObj->GetData(&fmt, &medium);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::GetArguments - failed to get CF_HDROP format");
            return hr;
        }
        if (!medium.hGlobal) {
            OutputDebugString(L"PyShellExt::GetArguments - CF_HDROP format had NULL hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        pdropfiles = (DROPFILES*)GlobalLock(medium.hGlobal);
        if (!pdropfiles) {
            OutputDebugString(L"PyShellExt::GetArguments - failed to lock CF_HDROP hGlobal");
            ReleaseStgMedium(&medium);
            return E_FAIL;
        }

        if (pdropfiles->fWide) {
            LPCWSTR files = (LPCWSTR)((char*)pdropfiles + pdropfiles->pFiles);
            size_t len, count;
            hr = FilenameListCchLengthW(files, 32767, &len, &count);
            if (SUCCEEDED(hr)) {
                LPWSTR args = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (len + 1));
                if (args) {
                    hr = FilenameListCchCopyW(args, 32767, files, L" ");
                    if (SUCCEEDED(hr)) {
                        *pArguments = args;
                    } else {
                        CoTaskMemFree(args);
                    }
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
        } else {
            LPCSTR files = (LPCSTR)((char*)pdropfiles + pdropfiles->pFiles);
            size_t len, count;
            hr = FilenameListCchLengthA(files, 32767, &len, &count);
            if (SUCCEEDED(hr)) {
                LPSTR temp = (LPSTR)CoTaskMemAlloc(sizeof(CHAR) * (len + 1));
                if (temp) {
                    hr = FilenameListCchCopyA(temp, 32767, files, " ");
                    if (SUCCEEDED(hr)) {
                        int wlen = MultiByteToWideChar(CP_ACP, 0, temp, (int)len, NULL, 0);
                        if (wlen) {
                            LPWSTR args = (LPWSTR)CoTaskMemAlloc(sizeof(WCHAR) * (wlen + 1));
                            if (MultiByteToWideChar(CP_ACP, 0, temp, (int)len, args, wlen + 1)) {
                                *pArguments = args;
                            } else {
                                OutputDebugString(L"PyShellExt::GetArguments - failed to convert multi-byte to wide-char path");
                                CoTaskMemFree(args);
                                hr = E_FAIL;
                            }
                        } else {
                            OutputDebugString(L"PyShellExt::GetArguments - failed to get length of wide-char path");
                            hr = E_FAIL;
                        }
                    }
                    CoTaskMemFree(temp);
                } else {
                    hr = E_OUTOFMEMORY;
                }
            }
        }

        GlobalUnlock(medium.hGlobal);
        ReleaseStgMedium(&medium);

        return hr;
    }

    HRESULT NotifyDragWindow(HWND hwnd) {
        LRESULT res;

        if (!hwnd) {
            return S_FALSE;
        }

        res = SendMessage(hwnd, DDWM_UPDATEWINDOW, 0, NULL);

        if (res) {
            OutputDebugString(L"PyShellExt::NotifyDragWindow - failed to post DDWM_UPDATEWINDOW");
            return E_FAIL;
        }

        return S_OK;
    }

public:
    // IDropTarget implementation

    STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
        HWND hwnd;

        OutputDebugString(L"PyShellExt::DragEnter");

        pDataObj->AddRef();
        data_obj = pDataObj;

        *pdwEffect = DROPEFFECT_MOVE;

        if (FAILED(UpdateDropDescription(data_obj))) {
            OutputDebugString(L"PyShellExt::DragEnter - failed to update drop description");
        }
        if (FAILED(GetDragWindow(data_obj, &hwnd))) {
            OutputDebugString(L"PyShellExt::DragEnter - failed to get drag window");
        }
        if (FAILED(NotifyDragWindow(hwnd))) {
            OutputDebugString(L"PyShellExt::DragEnter - failed to notify drag window");
        }

        return S_OK;
    }

    STDMETHODIMP DragLeave() {
        return S_OK;
    }

    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
        return S_OK;
    }

    STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) {
        LPCWSTR args;

        OutputDebugString(L"PyShellExt::Drop");
        *pdwEffect = DROPEFFECT_NONE;

        if (pDataObj != data_obj) {
            OutputDebugString(L"PyShellExt::Drop - unexpected data object");
            return E_FAIL;
        }

        data_obj->Release();
        data_obj = NULL;

        if (SUCCEEDED(GetArguments(pDataObj, &args))) {
            OutputDebugString(args);
            ShellExecute(NULL, NULL, target, args, target_dir, SW_NORMAL);

            CoTaskMemFree((LPVOID)args);
        } else {
            OutputDebugString(L"PyShellExt::Drop - failed to get launch arguments");
        }

        return S_OK;
    }

    // IPersistFile implementation

    STDMETHODIMP GetCurFile(LPOLESTR *ppszFileName) {
        HRESULT hr;
        size_t len;

        if (!ppszFileName) {
            return E_POINTER;
        }

        hr = StringCchLength(target, STRSAFE_MAX_CCH - 1, &len);
        if (FAILED(hr)) {
            return E_FAIL;
        }

        *ppszFileName = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * (len + 1));
        if (!*ppszFileName) {
            return E_OUTOFMEMORY;
        }

        hr = StringCchCopy(*ppszFileName, len + 1, target);
        if (FAILED(hr)) {
            CoTaskMemFree(*ppszFileName);
            *ppszFileName = NULL;
            return E_FAIL;
        }

        return S_OK;
    }

    STDMETHODIMP IsDirty() {
        return S_FALSE;
    }

    STDMETHODIMP Load(LPCOLESTR pszFileName, DWORD dwMode) {
        HRESULT hr;
        size_t len;

        OutputDebugString(L"PyShellExt::Load");
        OutputDebugString(pszFileName);

        hr = StringCchLength(pszFileName, STRSAFE_MAX_CCH - 1, &len);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::Load - failed to get string length");
            return hr;
        }

        if (target) {
            CoTaskMemFree(target);
        }
        if (target_dir) {
            CoTaskMemFree(target_dir);
        }

        target = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * (len + 1));
        if (!target) {
            OutputDebugString(L"PyShellExt::Load - E_OUTOFMEMORY");
            return E_OUTOFMEMORY;
        }
        target_dir = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR) * (len + 1));
        if (!target_dir) {
            OutputDebugString(L"PyShellExt::Load - E_OUTOFMEMORY");
            return E_OUTOFMEMORY;
        }

        hr = StringCchCopy(target, len + 1, pszFileName);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::Load - failed to copy string");
            return hr;
        }

        hr = StringCchCopy(target_dir, len + 1, pszFileName);
        if (FAILED(hr)) {
            OutputDebugString(L"PyShellExt::Load - failed to copy string");
            return hr;
        }
        if (!PathRemoveFileSpecW(target_dir)) {
            OutputDebugStringW(L"PyShellExt::Load - failed to remove filespec from target");
            return E_FAIL;
        }

        OutputDebugString(target);
        target_mode = dwMode;
        OutputDebugString(L"PyShellExt::Load - S_OK");
        return S_OK;
    }

    STDMETHODIMP Save(LPCOLESTR pszFileName, BOOL fRemember) {
        return E_NOTIMPL;
    }

    STDMETHODIMP SaveCompleted(LPCOLESTR pszFileName) {
        return E_NOTIMPL;
    }

    STDMETHODIMP GetClassID(CLSID *pClassID) {
        *pClassID = CLSID_PyShellExt;
        return S_OK;
    }
};

CoCreatableClass(PyShellExt);

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, _COM_Outptr_ void** ppv) {
    return Module<InProc>::GetModule().GetClassObject(rclsid, riid, ppv);
}

STDAPI DllCanUnloadNow() {
    return Module<InProc>::GetModule().Terminate() ? S_OK : S_FALSE;
}

STDAPI DllRegisterServer() {
    LONG res;
    SECURITY_ATTRIBUTES secattr = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    LPSECURITY_ATTRIBUTES psecattr = NULL;
    HKEY key, ipsKey;
    WCHAR modname[MAX_PATH];
    DWORD modname_len;

    OutputDebugString(L"PyShellExt::DllRegisterServer");
    if (!hModule) {
        OutputDebugString(L"PyShellExt::DllRegisterServer - module handle was not set");
        return SELFREG_E_CLASS;
    }
    modname_len = GetModuleFileName(hModule, modname, MAX_PATH);
    if (modname_len == 0 ||
        (modname_len == MAX_PATH && GetLastError() == ERROR_INSUFFICIENT_BUFFER)) {
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to get module file name");
        return SELFREG_E_CLASS;
    }

    DWORD disp;
    res = RegCreateKeyEx(HKEY_LOCAL_MACHINE, CLASS_SUBKEY, 0, NULL, 0,
        KEY_ALL_ACCESS, psecattr, &key, &disp);
    if (res == ERROR_ACCESS_DENIED) {
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to write per-machine registration. Attempting per-user instead.");
        res = RegCreateKeyEx(HKEY_CURRENT_USER, CLASS_SUBKEY, 0, NULL, 0,
            KEY_ALL_ACCESS, psecattr, &key, &disp);
    }
    if (res != ERROR_SUCCESS) {
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to create class key");
        return SELFREG_E_CLASS;
    }

    res = RegCreateKeyEx(key, L"InProcServer32", 0, NULL, 0,
        KEY_ALL_ACCESS, psecattr, &ipsKey, NULL);
    if (res != ERROR_SUCCESS) {
        RegCloseKey(key);
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to create InProcServer32 key");
        return SELFREG_E_CLASS;
    }

    res = RegSetValueEx(ipsKey, NULL, 0,
        REG_SZ, (LPBYTE)modname, modname_len * sizeof(modname[0]));

    if (res != ERROR_SUCCESS) {
        RegCloseKey(ipsKey);
        RegCloseKey(key);
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to set server path");
        return SELFREG_E_CLASS;
    }

    res = RegSetValueEx(ipsKey, L"ThreadingModel", 0,
        REG_SZ, (LPBYTE)(L"Apartment"), sizeof(L"Apartment"));

    RegCloseKey(ipsKey);
    RegCloseKey(key);
    if (res != ERROR_SUCCESS) {
        OutputDebugString(L"PyShellExt::DllRegisterServer - failed to set threading model");
        return SELFREG_E_CLASS;
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    OutputDebugString(L"PyShellExt::DllRegisterServer - S_OK");
    return S_OK;
}

STDAPI DllUnregisterServer() {
    LONG res_lm, res_cu;

    res_lm = RegDeleteTree(HKEY_LOCAL_MACHINE, CLASS_SUBKEY);
    if (res_lm != ERROR_SUCCESS && res_lm != ERROR_FILE_NOT_FOUND) {
        OutputDebugString(L"PyShellExt::DllUnregisterServer - failed to delete per-machine registration");
        return SELFREG_E_CLASS;
    }

    res_cu = RegDeleteTree(HKEY_CURRENT_USER, CLASS_SUBKEY);
    if (res_cu != ERROR_SUCCESS && res_cu != ERROR_FILE_NOT_FOUND) {
        OutputDebugString(L"PyShellExt::DllUnregisterServer - failed to delete per-user registration");
        return SELFREG_E_CLASS;
    }

    if (res_lm == ERROR_FILE_NOT_FOUND && res_cu == ERROR_FILE_NOT_FOUND) {
        OutputDebugString(L"PyShellExt::DllUnregisterServer - extension was not registered");
        return SELFREG_E_CLASS;
    }

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);

    OutputDebugString(L"PyShellExt::DllUnregisterServer - S_OK");
    return S_OK;
}

STDAPI_(BOOL) DllMain(_In_opt_ HINSTANCE hinst, DWORD reason, _In_opt_ void*) {
    if (reason == DLL_PROCESS_ATTACH) {
        hModule = hinst;

        cfDropDescription = RegisterClipboardFormat(CFSTR_DROPDESCRIPTION);
        if (!cfDropDescription) {
            OutputDebugString(L"PyShellExt::DllMain - failed to get CFSTR_DROPDESCRIPTION format");
        }
        cfDragWindow = RegisterClipboardFormat(L"DragWindow");
        if (!cfDragWindow) {
            OutputDebugString(L"PyShellExt::DllMain - failed to get DragWindow format");
        }

        DisableThreadLibraryCalls(hinst);
    }
    return TRUE;
}