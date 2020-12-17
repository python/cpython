#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

// constants

#define IDNOACTION 0
#define WIU_MB_OKIGNORECANCELRETRY 0xE

#define MAX_DARWIN_KEY 73
#define MAX_DARWIN_COLUMN 255

#define WIU_LOG_DEFAULT INSTALLLOGMODE_FATALEXIT | INSTALLLOGMODE_ERROR | INSTALLLOGMODE_WARNING | \
                        INSTALLLOGMODE_USER | INSTALLLOGMODE_INFO | INSTALLLOGMODE_RESOLVESOURCE | \
                        INSTALLLOGMODE_OUTOFDISKSPACE | INSTALLLOGMODE_ACTIONSTART | \
                        INSTALLLOGMODE_ACTIONDATA | INSTALLLOGMODE_COMMONDATA | INSTALLLOGMODE_PROPERTYDUMP

#define ReleaseMsi(h) if (h) { ::MsiCloseHandle(h); }
#define ReleaseNullMsi(h) if (h) { ::MsiCloseHandle(h); h = NULL; }


typedef enum WIU_RESTART
{
    WIU_RESTART_NONE,
    WIU_RESTART_REQUIRED,
    WIU_RESTART_INITIATED,
} WIU_RESTART;

typedef enum WIU_MSI_EXECUTE_MESSAGE_TYPE
{
    WIU_MSI_EXECUTE_MESSAGE_NONE,
    WIU_MSI_EXECUTE_MESSAGE_PROGRESS,
    WIU_MSI_EXECUTE_MESSAGE_ERROR,
    WIU_MSI_EXECUTE_MESSAGE_MSI_MESSAGE,
    WIU_MSI_EXECUTE_MESSAGE_MSI_FILES_IN_USE,
} WIU_MSI_EXECUTE_MESSAGE_TYPE;


// structures

typedef struct _WIU_MSI_EXECUTE_MESSAGE
{
    WIU_MSI_EXECUTE_MESSAGE_TYPE type;
    DWORD dwAllowedResults;

    DWORD cData;
    LPCWSTR* rgwzData;

    INT nResultRecommendation; // recommended return result for this message based on analysis of real world installs.

    union
    {
        struct
        {
            DWORD dwPercentage;
        } progress;
        struct
        {
            DWORD dwErrorCode;
            LPCWSTR wzMessage;
        } error;
        struct
        {
            INSTALLMESSAGE mt;
            LPCWSTR wzMessage;
        } msiMessage;
        struct
        {
            DWORD cFiles;
            LPCWSTR* rgwzFiles;
        } msiFilesInUse;
    };
} WIU_MSI_EXECUTE_MESSAGE;

typedef struct _WIU_MSI_PROGRESS
{
    DWORD dwTotal;
    DWORD dwCompleted;
    DWORD dwStep;
    BOOL fMoveForward;
    BOOL fEnableActionData;
    BOOL fScriptInProgress;
} WIU_MSI_PROGRESS;


typedef int (*PFN_MSIEXECUTEMESSAGEHANDLER)(
    __in WIU_MSI_EXECUTE_MESSAGE* pMessage,
    __in_opt LPVOID pvContext
    );

typedef struct _WIU_MSI_EXECUTE_CONTEXT
{
    BOOL fRollback;
    PFN_MSIEXECUTEMESSAGEHANDLER pfnMessageHandler;
    LPVOID pvContext;
    WIU_MSI_PROGRESS rgMsiProgress[64];
    DWORD dwCurrentProgressIndex;

    INSTALLUILEVEL previousInstallUILevel;
    HWND hwndPreviousParentWindow;
    INSTALLUI_HANDLERW pfnPreviousExternalUI;
    INSTALLUI_HANDLER_RECORD pfnPreviousExternalUIRecord;

    BOOL fSetPreviousExternalUIRecord;
    BOOL fSetPreviousExternalUI;
} WIU_MSI_EXECUTE_CONTEXT;


// typedefs
typedef UINT (WINAPI *PFN_MSIENABLELOGW)(
    __in DWORD dwLogMode,
    __in_z LPCWSTR szLogFile,
    __in DWORD dwLogAttributes
    );
typedef UINT (WINAPI *PFN_MSIGETPRODUCTINFOW)(
    __in LPCWSTR szProductCode,
    __in LPCWSTR szProperty,
    __out_ecount_opt(*pcchValue) LPWSTR szValue,
    __inout LPDWORD pcchValue
    );
typedef INSTALLSTATE (WINAPI *PFN_MSIGETCOMPONENTPATHW)(
    __in LPCWSTR szProduct,
    __in LPCWSTR szComponent,
    __out_ecount_opt(*pcchBuf) LPWSTR lpPathBuf,
    __inout_opt LPDWORD pcchBuf
    );
typedef INSTALLSTATE (WINAPI *PFN_MSILOCATECOMPONENTW)(
    __in LPCWSTR szComponent,
    __out_ecount_opt(*pcchBuf) LPWSTR lpPathBuf,
    __inout_opt LPDWORD pcchBuf
    );
typedef UINT (WINAPI *PFN_MSIGETPRODUCTINFOEXW)(
    __in LPCWSTR szProductCode,
    __in_opt LPCWSTR szUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in LPCWSTR szProperty,
    __out_ecount_opt(*pcchValue) LPWSTR szValue,
    __inout_opt LPDWORD pcchValue
    );
typedef INSTALLSTATE (WINAPI *PFN_MSIQUERYFEATURESTATEW)(
    __in LPCWSTR szProduct,
    __in LPCWSTR szFeature
    );
typedef UINT (WINAPI *PFN_MSIGETPATCHINFOEXW)(
    __in_z LPCWSTR wzPatchCode,
    __in_z LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in_z LPCWSTR wzProperty,
    __out_opt LPWSTR wzValue,
    __inout DWORD* pcchValue
    );
typedef UINT (WINAPI *PFN_MSIDETERMINEPATCHSEQUENCEW)(
    __in_z LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT context,
    __in DWORD cPatchInfo,
    __in PMSIPATCHSEQUENCEINFOW pPatchInfo
    );
typedef UINT (WINAPI *PFN_MSIDETERMINEAPPLICABLEPATCHESW)(
    __in_z LPCWSTR wzProductPackagePath,
    __in DWORD cPatchInfo,
    __in PMSIPATCHSEQUENCEINFOW pPatchInfo
    );
typedef UINT (WINAPI *PFN_MSIINSTALLPRODUCTW)(
    __in LPCWSTR szPackagePath,
    __in_opt LPCWSTR szCommandLine
    );
typedef UINT (WINAPI *PFN_MSICONFIGUREPRODUCTEXW)(
    __in LPCWSTR szProduct,
    __in int iInstallLevel,
    __in INSTALLSTATE eInstallState,
    __in_opt LPCWSTR szCommandLine
    );
typedef UINT (WINAPI *PFN_MSIREMOVEPATCHESW)(
    __in_z LPCWSTR wzPatchList,
    __in_z LPCWSTR wzProductCode,
    __in INSTALLTYPE eUninstallType,
    __in_z_opt LPCWSTR szPropertyList
    );
typedef INSTALLUILEVEL (WINAPI *PFN_MSISETINTERNALUI)(
    __in INSTALLUILEVEL dwUILevel,
    __inout_opt HWND *phWnd
    );
typedef UINT (WINAPI *PFN_MSISETEXTERNALUIRECORD)(
    __in_opt INSTALLUI_HANDLER_RECORD puiHandler,
    __in DWORD dwMessageFilter,
    __in_opt LPVOID pvContext,
    __out_opt PINSTALLUI_HANDLER_RECORD ppuiPrevHandler
    );
typedef INSTALLUI_HANDLERW (WINAPI *PFN_MSISETEXTERNALUIW)(
    __in_opt INSTALLUI_HANDLERW puiHandler,
    __in DWORD dwMessageFilter,
    __in_opt LPVOID pvContext
    );
typedef UINT (WINAPI *PFN_MSIENUMPRODUCTSW)(
    __in DWORD iProductIndex,
    __out_ecount(MAX_GUID_CHARS + 1) LPWSTR lpProductBuf
    );
typedef UINT (WINAPI *PFN_MSIENUMPRODUCTSEXW)(
    __in_z_opt LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in DWORD dwContext,
    __in DWORD dwIndex,
    __out_opt WCHAR wzInstalledProductCode[39],
    __out_opt MSIINSTALLCONTEXT *pdwInstalledContext,
    __out_opt LPWSTR wzSid,
    __inout_opt LPDWORD pcchSid
    );

typedef UINT (WINAPI *PFN_MSIENUMRELATEDPRODUCTSW)(
    __in LPCWSTR lpUpgradeCode,
    __reserved DWORD dwReserved,
    __in DWORD iProductIndex,
    __out_ecount(MAX_GUID_CHARS + 1) LPWSTR lpProductBuf
    );
typedef UINT (WINAPI *PFN_MSISOURCELISTADDSOURCEEXW)(
    __in LPCWSTR szProductCodeOrPatchCode,
    __in_opt LPCWSTR szUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in DWORD dwOptions,
    __in LPCWSTR szSource,
    __in_opt DWORD dwIndex
    );


HRESULT DAPI WiuInitialize(
    );
void DAPI WiuUninitialize(
    );
void DAPI WiuFunctionOverride(
    __in_opt PFN_MSIENABLELOGW pfnMsiEnableLogW,
    __in_opt PFN_MSIGETCOMPONENTPATHW pfnMsiGetComponentPathW,
    __in_opt PFN_MSILOCATECOMPONENTW pfnMsiLocateComponentW,
    __in_opt PFN_MSIQUERYFEATURESTATEW pfnMsiQueryFeatureStateW,
    __in_opt PFN_MSIGETPRODUCTINFOW pfnMsiGetProductInfoW,
    __in_opt PFN_MSIGETPRODUCTINFOEXW pfnMsiGetProductInfoExW,
    __in_opt PFN_MSIINSTALLPRODUCTW pfnMsiInstallProductW,
    __in_opt PFN_MSICONFIGUREPRODUCTEXW pfnMsiConfigureProductExW,
    __in_opt PFN_MSISETINTERNALUI pfnMsiSetInternalUI,
    __in_opt PFN_MSISETEXTERNALUIW pfnMsiSetExternalUIW,
    __in_opt PFN_MSIENUMRELATEDPRODUCTSW pfnMsiEnumRelatedProductsW,
    __in_opt PFN_MSISETEXTERNALUIRECORD pfnMsiSetExternalUIRecord,
    __in_opt PFN_MSISOURCELISTADDSOURCEEXW pfnMsiSourceListAddSourceExW
    );
HRESULT DAPI WiuGetComponentPath(
    __in_z LPCWSTR wzProductCode,
    __in_z LPCWSTR wzComponentId,
    __out INSTALLSTATE* pInstallState,
    __out_z LPWSTR* psczValue
    );
HRESULT DAPI WiuLocateComponent(
    __in_z LPCWSTR wzComponentId,
    __out INSTALLSTATE* pInstallState,
    __out_z LPWSTR* psczValue
    );
HRESULT DAPI WiuQueryFeatureState(
    __in_z LPCWSTR wzProduct,
    __in_z LPCWSTR wzFeature,
    __out INSTALLSTATE* pInstallState
    );
HRESULT DAPI WiuGetProductInfo(
    __in_z LPCWSTR wzProductCode,
    __in_z LPCWSTR wzProperty,
    __out LPWSTR* psczValue
    );
HRESULT DAPI WiuGetProductInfoEx(
    __in_z LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in_z LPCWSTR wzProperty,
    __out LPWSTR* psczValue
    );
HRESULT DAPI WiuGetProductProperty(
    __in MSIHANDLE hProduct,
    __in_z LPCWSTR wzProperty,
    __out LPWSTR* psczValue
    );
HRESULT DAPI WiuGetPatchInfoEx(
    __in_z LPCWSTR wzPatchCode,
    __in_z LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in_z LPCWSTR wzProperty,
    __out LPWSTR* psczValue
    );
HRESULT DAPI WiuDeterminePatchSequence(
    __in_z LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT context,
    __in PMSIPATCHSEQUENCEINFOW pPatchInfo,
    __in DWORD cPatchInfo
    );
HRESULT DAPI WiuDetermineApplicablePatches(
    __in_z LPCWSTR wzProductPackagePath,
    __in PMSIPATCHSEQUENCEINFOW pPatchInfo,
    __in DWORD cPatchInfo
    );
HRESULT DAPI WiuEnumProducts(
    __in DWORD iProductIndex,
    __out_ecount(MAX_GUID_CHARS + 1) LPWSTR wzProductCode
    );
HRESULT DAPI WiuEnumProductsEx(
    __in_z_opt LPCWSTR wzProductCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in DWORD dwContext,
    __in DWORD dwIndex,
    __out_opt WCHAR wzInstalledProductCode[39],
    __out_opt MSIINSTALLCONTEXT *pdwInstalledContext,
    __out_opt LPWSTR wzSid,
    __inout_opt LPDWORD pcchSid
    );
HRESULT DAPI WiuEnumRelatedProducts(
    __in_z LPCWSTR wzUpgradeCode,
    __in DWORD iProductIndex,
    __out_ecount(MAX_GUID_CHARS + 1) LPWSTR wzProductCode
    );
HRESULT DAPI WiuEnumRelatedProductCodes(
    __in_z LPCWSTR wzUpgradeCode,
    __deref_out_ecount_opt(pcRelatedProducts) LPWSTR** prgsczProductCodes,
    __out DWORD* pcRelatedProducts,
    __in BOOL fReturnHighestVersionOnly
    );
HRESULT DAPI WiuEnableLog(
    __in DWORD dwLogMode,
    __in_z LPCWSTR wzLogFile,
    __in DWORD dwLogAttributes
    );
HRESULT DAPI WiuInitializeExternalUI(
    __in PFN_MSIEXECUTEMESSAGEHANDLER pfnMessageHandler,
    __in INSTALLUILEVEL internalUILevel,
    __in HWND hwndParent,
    __in LPVOID pvContext,
    __in BOOL fRollback,
    __in WIU_MSI_EXECUTE_CONTEXT* pExecuteContext
    );
void DAPI WiuUninitializeExternalUI(
    __in WIU_MSI_EXECUTE_CONTEXT* pExecuteContext
    );
HRESULT DAPI WiuConfigureProductEx(
    __in_z LPCWSTR wzProduct,
    __in int iInstallLevel,
    __in INSTALLSTATE eInstallState,
    __in_z LPCWSTR wzCommandLine,
    __out WIU_RESTART* pRestart
    );
HRESULT DAPI WiuInstallProduct(
    __in_z LPCWSTR wzPackagPath,
    __in_z LPCWSTR wzCommandLine,
    __out WIU_RESTART* pRestart
    );
HRESULT DAPI WiuRemovePatches(
    __in_z LPCWSTR wzPatchList,
    __in_z LPCWSTR wzProductCode,
    __in_z LPCWSTR wzPropertyList,
    __out WIU_RESTART* pRestart
    );
HRESULT DAPI WiuSourceListAddSourceEx(
    __in_z LPCWSTR wzProductCodeOrPatchCode,
    __in_z_opt LPCWSTR wzUserSid,
    __in MSIINSTALLCONTEXT dwContext,
    __in DWORD dwCode,
    __in_z LPCWSTR wzSource,
    __in_opt DWORD dwIndex
    );

#ifdef __cplusplus
}
#endif
