#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include <cguid.h>
#include <oledberr.h>
#include <sqloledb.h>


#ifdef __cplusplus
extern "C" {
#endif

// Adding this until the SQL annotations are published to specstrings.h
#ifndef __sql_command
#define __sql_command
#endif

// structs
struct SQL_FILESPEC
{
    WCHAR wzName[MAX_PATH];
    WCHAR wzFilename[MAX_PATH];
    WCHAR wzSize[MAX_PATH];
    WCHAR wzMaxSize[MAX_PATH];
    WCHAR wzGrow[MAX_PATH];
};


// functions
HRESULT DAPI SqlConnectDatabase(
    __in_z LPCWSTR wzServer, 
    __in_z LPCWSTR wzInstance,
    __in_z LPCWSTR wzDatabase, 
    __in BOOL fIntegratedAuth, 
    __in_z LPCWSTR wzUser, 
    __in_z LPCWSTR wzPassword, 
    __out IDBCreateSession** ppidbSession
    );
HRESULT DAPI SqlStartTransaction(
    __in IDBCreateSession* pidbSession,
    __out IDBCreateCommand** ppidbCommand,
    __out ITransaction** ppit
    );
HRESULT DAPI SqlEndTransaction(
    __in ITransaction* pit,
    __in BOOL fCommit
    );
HRESULT DAPI SqlDatabaseExists(
    __in_z LPCWSTR wzServer,
    __in_z LPCWSTR wzInstance,
    __in_z LPCWSTR wzDatabase,
    __in BOOL fIntegratedAuth,
    __in_z LPCWSTR wzUser,
    __in_z LPCWSTR wzPassword,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlSessionDatabaseExists(
    __in IDBCreateSession* pidbSession,
    __in_z LPCWSTR wzDatabase,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlDatabaseEnsureExists(
    __in_z LPCWSTR wzServer,
    __in_z LPCWSTR wzInstance,
    __in_z LPCWSTR wzDatabase,
    __in BOOL fIntegratedAuth,
    __in_z LPCWSTR wzUser,
    __in_z LPCWSTR wzPassword,
    __in_opt const SQL_FILESPEC* psfDatabase,
    __in_opt const SQL_FILESPEC* psfLog,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlSessionDatabaseEnsureExists(
    __in IDBCreateSession* pidbSession,
    __in_z LPCWSTR wzDatabase,
    __in_opt const SQL_FILESPEC* psfDatabase,
    __in_opt const SQL_FILESPEC* psfLog,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlCreateDatabase(
    __in_z LPCWSTR wzServer,
    __in_z LPCWSTR wzInstance,
    __in_z LPCWSTR wzDatabase,
    __in BOOL fIntegratedAuth,
    __in_z LPCWSTR wzUser,
    __in_z LPCWSTR wzPassword,
    __in_opt const SQL_FILESPEC* psfDatabase,
    __in_opt const SQL_FILESPEC* psfLog,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlSessionCreateDatabase(
    __in IDBCreateSession* pidbSession,
    __in_z LPCWSTR wzDatabase,
    __in_opt const SQL_FILESPEC* psfDatabase,
    __in_opt const SQL_FILESPEC* psfLog,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlDropDatabase(
    __in_z LPCWSTR wzServer,
    __in_z LPCWSTR wzInstance,
    __in_z LPCWSTR wzDatabase,
    __in BOOL fIntegratedAuth,
    __in_z LPCWSTR wzUser,
    __in_z LPCWSTR wzPassword,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlSessionDropDatabase(
    __in IDBCreateSession* pidbSession,
    __in_z LPCWSTR wzDatabase,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlSessionExecuteQuery(
    __in IDBCreateSession* pidbSession, 
    __in __sql_command LPCWSTR wzSql, 
    __out_opt IRowset** ppirs,
    __out_opt DBROWCOUNT* pcRows,
    __out_opt BSTR* pbstrErrorDescription
    );
HRESULT DAPI SqlCommandExecuteQuery(
    __in IDBCreateCommand* pidbCommand, 
    __in __sql_command LPCWSTR wzSql, 
    __out IRowset** ppirs,
    __out DBROWCOUNT* pcRows
    );
HRESULT DAPI SqlGetErrorInfo(
    __in IUnknown* pObjectWithError,
    __in REFIID IID_InterfaceWithError,
    __in DWORD dwLocaleId,
    __out_opt BSTR* pbstrErrorSource,
    __out_opt BSTR* pbstrErrorDescription
    );

#ifdef __cplusplus
}
#endif
