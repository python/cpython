#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define ReleaseEseQuery(pqh) if (pqh) { EseFinishQuery(pqh); }
#define ReleaseNullEseQuery(pqh) if (pqh) { EseFinishQuery(pqh); pqh = NULL; }

struct ESE_COLUMN_SCHEMA
{
    JET_COLUMNID jcColumn;
    LPCWSTR pszName;
    JET_COLTYP jcColumnType;
    BOOL fKey; // If this column is part of the key of the table
    BOOL fFixed;
    BOOL fNullable;
    BOOL fAutoIncrement;
};

struct ESE_TABLE_SCHEMA
{
    JET_TABLEID jtTable;
    LPCWSTR pszName;
    DWORD dwColumns;
    ESE_COLUMN_SCHEMA *pcsColumns;
};

struct ESE_DATABASE_SCHEMA
{
    DWORD dwTables;
    ESE_TABLE_SCHEMA *ptsTables;
};

typedef enum ESE_QUERY_TYPE
{
    ESE_QUERY_EXACT,
    ESE_QUERY_FROM_TOP,
    ESE_QUERY_FROM_BOTTOM
} ESE_QUERY_TYPE;

typedef void* ESE_QUERY_HANDLE;

HRESULT DAPI EseBeginSession(
    __out JET_INSTANCE *pjiInstance,
    __out JET_SESID *pjsSession,
    __in_z LPCWSTR pszInstance,
    __in_z LPCWSTR pszPath
    );
HRESULT DAPI EseEndSession(
    __in JET_INSTANCE jiInstance,
    __in JET_SESID jsSession
    );
HRESULT DAPI EseEnsureDatabase(
    __in JET_SESID jsSession,
    __in_z LPCWSTR pszFile,
    __in ESE_DATABASE_SCHEMA *pdsSchema,
    __out JET_DBID* pjdbDb,
    __in BOOL fExclusive,
    __in BOOL fReadonly
    );
HRESULT DAPI EseCloseDatabase(
    __in JET_SESID jsSession,
    __in JET_DBID jdbDb
    );
HRESULT DAPI EseCreateTable(
    __in JET_SESID jsSession,
    __in JET_DBID jdbDb,
    __in_z LPCWSTR pszTable,
    __out JET_TABLEID *pjtTable
    );
HRESULT DAPI EseOpenTable(
    __in JET_SESID jsSession,
    __in JET_DBID jdbDb,
    __in_z LPCWSTR pszTable,
    __out JET_TABLEID *pjtTable
    );
HRESULT DAPI EseCloseTable(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable
    );
HRESULT DAPI EseEnsureColumn(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in_z LPCWSTR pszColumnName,
    __in JET_COLTYP jcColumnType,
    __in ULONG ulColumnSize,
    __in BOOL fFixed,
    __in BOOL fNullable,
    __out_opt JET_COLUMNID *pjcColumn
    );
HRESULT DAPI EseGetColumn(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in_z LPCWSTR pszColumnName,
    __out JET_COLUMNID *pjcColumn
    );
HRESULT DAPI EseMoveCursor(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in LONG lRow
    );
HRESULT DAPI EseDeleteRow(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable
    );
HRESULT DAPI EseBeginTransaction(
    __in JET_SESID jsSession
    );
HRESULT DAPI EseRollbackTransaction(
    __in JET_SESID jsSession,
    __in BOOL fAll
    );
HRESULT DAPI EseCommitTransaction(
    __in JET_SESID jsSession
    );
HRESULT DAPI EsePrepareUpdate(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in ULONG ulPrep
    );
HRESULT DAPI EseFinishUpdate(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in BOOL fSeekToInsertedRecord
    );
HRESULT DAPI EseSetColumnBinary(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __in_bcount(cbBuffer) const BYTE* pbBuffer,
    __in SIZE_T cbBuffer
    );
HRESULT DAPI EseSetColumnDword(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __in DWORD dwValue
    );
HRESULT DAPI EseSetColumnBool(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __in BOOL fValue
    );
HRESULT DAPI EseSetColumnString(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __in_z LPCWSTR pszValue
    );
HRESULT DAPI EseSetColumnEmpty(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn
    );
HRESULT DAPI EseGetColumnBinary(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __deref_out_bcount(*piBuffer) BYTE** ppbBuffer,
    __inout SIZE_T* piBuffer
    );
HRESULT DAPI EseGetColumnDword(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __out DWORD *pdwValue
    );
HRESULT DAPI EseGetColumnBool(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __out BOOL *pfValue
    );
HRESULT DAPI EseGetColumnString(
    __in JET_SESID jsSession,
    __in ESE_TABLE_SCHEMA tsTable,
    __in DWORD dwColumn,
    __out LPWSTR *ppszValue
    );

// Call this once for each key column in the table
HRESULT DAPI EseBeginQuery(
    __in JET_SESID jsSession,
    __in JET_TABLEID jtTable,
    __in ESE_QUERY_TYPE qtQueryType,
    __out ESE_QUERY_HANDLE *peqhHandle
    );
HRESULT DAPI EseSetQueryColumnBinary(
    __in ESE_QUERY_HANDLE eqhHandle,
    __in_bcount(cbBuffer) const BYTE* pbBuffer,
    __in SIZE_T cbBuffer,
    __in BOOL fFinal // If this is true, all other key columns in the query will be set to "*"
    );
HRESULT DAPI EseSetQueryColumnDword(
    __in ESE_QUERY_HANDLE eqhHandle,
    __in DWORD dwData,
    __in BOOL fFinal // If this is true, all other key columns in the query will be set to "*"
    );
HRESULT DAPI EseSetQueryColumnBool(
    __in ESE_QUERY_HANDLE eqhHandle,
    __in BOOL fValue,
    __in BOOL fFinal // If this is true, all other key columns in the query will be set to "*"
    );
HRESULT DAPI EseSetQueryColumnString(
    __in ESE_QUERY_HANDLE eqhHandle,
    __in_z LPCWSTR pszString,
    __in BOOL fFinal // If this is true, all other key columns in the query will be set to "*"
    );
HRESULT DAPI EseFinishQuery(
    __in ESE_QUERY_HANDLE eqhHandle
    );
// Once all columns have been set up, call this and read the result
HRESULT DAPI EseRunQuery(
    __in ESE_QUERY_HANDLE eqhHandle
    );

#ifdef __cplusplus
}
#endif
