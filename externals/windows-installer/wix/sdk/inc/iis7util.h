#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

// IIS Config schema names
#define IIS_CONFIG_ADD                      L"add"
#define IIS_CONFIG_ALLOWED                  L"allowed"
#define IIS_CONFIG_APPHOST_ROOT             L"MACHINE/WEBROOT/APPHOST"
#define IIS_CONFIG_APPLICATION              L"application"
#define IIS_CONFIG_APPPOOL                  L"applicationPool"
#define IIS_CONFIG_APPPOOL_AUTO             L"autoStart"
#define IIS_CONFIG_APPPOOL_SECTION          L"system.applicationHost/applicationPools"
#define IIS_CONFIG_AUTOSTART                L"serverAutoStart"
#define IIS_CONFIG_BINDING                  L"binding"
#define IIS_CONFIG_BINDINGINFO              L"bindingInformation"
#define IIS_CONFIG_BINDINGS                 L"bindings"
#define IIS_CONFIG_DESC                     L"description"
#define IIS_CONFIG_EXECUTABLE               L"scriptProcessor"
#define IIS_CONFIG_ENABLED                  L"enabled"
#define IIS_CONFIG_ENABLE32                 L"enable32BitAppOnWin64"
#define IIS_CONFIG_FILEEXT                  L"fileExtension"
#define IIS_CONFIG_FILTER                   L"filter"
#define IIS_CONFIG_GROUPID                  L"groupId"
#define IIS_CONFIG_HEADERS                  L"customHeaders"
#define IIS_CONFIG_HTTPERRORS_SECTION       L"system.webServer/httpErrors"
#define IIS_CONFIG_ID                       L"id"
#define IIS_CONFIG_ISAPI_SECTION            L"system.webServer/isapiFilters"
#define IIS_CONFIG_HTTPPROTO_SECTION        L"system.webServer/httpProtocol"
#define IIS_CONFIG_LOG_SECTION              L"system.applicationHost/log"
#define IIS_CONFIG_LOG_UTF8                 L"logInUTF8"
#define IIS_CONFIG_LIMITS                   L"limits"
#define IIS_CONFIG_PIPELINEMODE             L"managedPipelineMode"
#define IIS_CONFIG_MANAGEDRUNTIMEVERSION    L"managedRuntimeVersion"
#define IIS_CONFIG_WEBLOG                   L"logFile"
#define IIS_CONFIG_LOGFORMAT                L"logFormat"
#define IIS_CONFIG_MIMEMAP                  L"mimeMap"
#define IIS_CONFIG_MIMETYPE                 L"mimeType"
#define IIS_CONFIG_MODULES                  L"modules"
#define IIS_CONFIG_NAME                     L"name"
#define IIS_CONFIG_PATH                     L"path"
#define IIS_CONFIG_PHYSPATH                 L"physicalPath"
#define IIS_CONFIG_PROTOCOL                 L"protocol"
#define IIS_CONFIG_RESTRICTION_SECTION      L"system.webServer/security/isapiCgiRestriction"
#define IIS_CONFIG_SITE                     L"site"
#define IIS_CONFIG_SITE_ID                  L"id"
#define IIS_CONFIG_SITES_SECTION            L"system.applicationHost/sites"
#define IIS_CONFIG_CONNECTTIMEOUT           L"connectionTimeout"
#define IIS_CONFIG_VDIR                     L"virtualDirectory"
#define IIS_CONFIG_VALUE                    L"value"
#define IIS_CONFIG_VERBS                    L"verb"
#define IIS_CONFIG_WEBLIMITS_SECTION        L"system.applicationHost/webLimits"
#define IIS_CONFIG_WEBLIMITS_MAXBAND        L"maxGlobalBandwidth"
#define IIS_CONFIG_TRUE                     L"true"
#define IIS_CONFIG_FALSE                    L"false"
#define IIS_CONFIG_ERROR                    L"error"
#define IIS_CONFIG_STATUSCODE               L"statusCode"
#define IIS_CONFIG_SUBSTATUS                L"subStatusCode"
#define IIS_CONFIG_LANGPATH                 L"prefixLanguageFilePath"
#define IIS_CONFIG_RESPMODE                 L"responseMode"
#define IIS_CONFIG_CLEAR                    L"clear"
#define IIS_CONFIG_RECYCLING                L"recycling"
#define IIS_CONFIG_PEROIDRESTART            L"periodicRestart"
#define IIS_CONFIG_TIME                     L"time"
#define IIS_CONFIG_REQUESTS                 L"requests"
#define IIS_CONFIG_SCHEDULE                 L"schedule"
#define IIS_CONFIG_MEMORY                   L"memory"
#define IIS_CONFIG_PRIVMEMORY               L"privateMemory"
#define IIS_CONFIG_PROCESSMODEL             L"processModel"
#define IIS_CONFIG_IDLETIMEOUT              L"idleTimeout"
#define IIS_CONFIG_QUEUELENGTH              L"queueLength"
#define IIS_CONFIG_IDENITITYTYPE            L"identityType"
#define IIS_CONFIG_LOCALSYSTEM              L"LocalSystem"
#define IIS_CONFIG_LOCALSERVICE             L"LocalService"
#define IIS_CONFIG_NETWORKSERVICE           L"NetworkService"
#define IIS_CONFIG_SPECIFICUSER             L"SpecificUser"
#define IIS_CONFIG_APPLICATIONPOOLIDENTITY  L"ApplicationPoolIdentity"
#define IIS_CONFIG_USERNAME                 L"userName"
#define IIS_CONFIG_PASSWORD                 L"password"
#define IIS_CONFIG_CPU                      L"cpu"
#define IIS_CONFIG_LIMIT                    L"limit"
#define IIS_CONFIG_CPU_ACTION               L"action"
#define IIS_CONFIG_KILLW3WP                 L"KillW3wp"
#define IIS_CONFIG_NOACTION                 L"NoAction"
#define IIS_CONFIG_RESETINTERVAL            L"resetInterval"
#define IIS_CONFIG_MAXWRKPROCESSES          L"maxProcesses"
#define IIS_CONFIG_HANDLERS_SECTION         L"system.webServer/handlers"
#define IIS_CONFIG_DEFAULTDOC_SECTION       L"system.webServer/defaultDocument"
#define IIS_CONFIG_ASP_SECTION              L"system.webServer/asp"
#define IIS_CONFIG_SCRIPTERROR              L"scriptErrorSentToBrowser"
#define IIS_CONFIG_STATICCONTENT_SECTION    L"system.webServer/staticContent"
#define IIS_CONFIG_HTTPEXPIRES              L"httpExpires"
#define IIS_CONFIG_MAXAGE                   L"cacheControlMaxAge"
#define IIS_CONFIG_CLIENTCACHE              L"clientCache"
#define IIS_CONFIG_CACHECONTROLMODE         L"cacheControlMode"
#define IIS_CONFIG_USEMAXAGE                L"UseMaxAge"
#define IIS_CONFIG_USEEXPIRES               L"UseExpires"
#define IIS_CONFIG_CACHECUST                L"cacheControlCustom"
#define IIS_CONFIG_ASP_SECTION              L"system.webServer/asp"
#define IIS_CONFIG_SESSION                  L"session"
#define IIS_CONFIG_ALLOWSTATE               L"allowSessionState"
#define IIS_CONFIG_TIMEOUT                  L"timeout"
#define IIS_CONFIG_BUFFERING                L"bufferingOn"
#define IIS_CONFIG_PARENTPATHS              L"enableParentPaths"
#define IIS_CONFIG_SCRIPTLANG               L"scriptLanguage"
#define IIS_CONFIG_SCRIPTTIMEOUT            L"scriptTimeout"
#define IIS_CONFIG_LIMITS                   L"limits"
#define IIS_CONFIG_ALLOWDEBUG               L"appAllowDebugging"
#define IIS_CONFIG_ALLOWCLIENTDEBUG         L"appAllowClientDebug"
#define IIS_CONFIG_CERTIFICATEHASH          L"certificateHash"
#define IIS_CONFIG_CERTIFICATESTORENAME     L"certificateStoreName"
#define IIS_CONFIG_HTTPLOGGING_SECTION      L"system.webServer/httpLogging"
#define IIS_CONFIG_DONTLOG                  L"dontLog"

typedef BOOL (CALLBACK* ENUMAPHOSTELEMENTPROC)(IAppHostElement*, LPVOID);
typedef BOOL (CALLBACK* VARIANTCOMPARATORPROC)(VARIANT*, VARIANT*);

HRESULT DAPI Iis7PutPropertyVariant(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in VARIANT vtPut
    );

HRESULT DAPI Iis7PutPropertyInteger(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in DWORD dValue
    );

HRESULT DAPI Iis7PutPropertyString(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in LPCWSTR wzString
    );

HRESULT DAPI Iis7PutPropertyBool(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in BOOL fValue);

HRESULT DAPI Iis7GetPropertyVariant(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in VARIANT* vtGet
    );

HRESULT DAPI Iis7GetPropertyString(
    __in IAppHostElement *pElement,
    __in LPCWSTR wzPropName,
    __in LPWSTR* psczGet
    );

struct IIS7_APPHOSTELEMENTCOMPARISON
{
    LPCWSTR sczElementName;
    LPCWSTR sczAttributeName;
    VARIANT* pvAttributeValue;
    VARIANTCOMPARATORPROC pComparator;
};

BOOL DAPI Iis7IsMatchingAppHostElement( 
    __in IAppHostElement *pElement,
    __in IIS7_APPHOSTELEMENTCOMPARISON* pComparison
    );

HRESULT DAPI Iis7FindAppHostElementString( 
    __in IAppHostElementCollection *pCollection,
    __in LPCWSTR wzElementName,
    __in LPCWSTR wzAttributeName,
    __in LPCWSTR wzAttributeValue,
    __out IAppHostElement** ppElement,
    __out DWORD* pdwIndex
    );

HRESULT DAPI Iis7FindAppHostElementPath( 
    __in IAppHostElementCollection *pCollection,
    __in LPCWSTR wzElementName,
    __in LPCWSTR wzAttributeName,
    __in LPCWSTR wzAttributeValue,
    __out IAppHostElement** ppElement,
    __out DWORD* pdwIndex
    );

HRESULT DAPI Iis7FindAppHostElementInteger( 
    __in IAppHostElementCollection *pCollection,
    __in LPCWSTR wzElementName,
    __in LPCWSTR wzAttributeName,
    __in DWORD dwAttributeValue,
    __out IAppHostElement** ppElement,
    __out DWORD* pdwIndex
    );

HRESULT DAPI Iis7FindAppHostElementVariant( 
    __in IAppHostElementCollection *pCollection,
    __in LPCWSTR wzElementName,
    __in LPCWSTR wzAttributeName,
    __in VARIANT* pvAttributeValue,
    __out IAppHostElement** ppElement,
    __out DWORD* pdwIndex
    );

HRESULT DAPI Iis7EnumAppHostElements( 
    __in IAppHostElementCollection *pCollection,
    __in ENUMAPHOSTELEMENTPROC pCallback,
    __in LPVOID pContext,
    __out IAppHostElement** ppElement,
    __out DWORD* pdwIndex
    );

HRESULT DAPI Iis7FindAppHostMethod(
    __in IAppHostMethodCollection *pCollection,
    __in LPCWSTR wzMethodName,
    __out IAppHostMethod** ppMethod,
    __out DWORD* pdwIndex
    );

#ifdef __cplusplus
}
#endif
