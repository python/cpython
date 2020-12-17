#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#include <sqlce_oledb.h>
#include <sqlce_sync.h>
#include <sqlce_err.h>

typedef void* SCE_DATABASE_HANDLE;
typedef void* SCE_ROW_HANDLE;
typedef void* SCE_QUERY_HANDLE;
typedef void* SCE_QUERY_RESULTS_HANDLE;

extern const int SCE_ROW_HANDLE_BYTES;
extern const int SCE_QUERY_HANDLE_BYTES;
extern const int SCE_QUERY_RESULTS_HANDLE_BYTES;

#define ReleaseSceRow(prrh) if (prrh) { SceFreeRow(prrh); }
#define ReleaseNullSceRow(prrh) if (prrh) { SceFreeRow(prrh); prrh = NULL; }
#define ReleaseSceQuery(pqh) if (pqh) { SceFreeQuery(pqh); }
#define ReleaseNullSceQuery(pqh) if (pqh) { SceFreeQuery(pqh); pqh = NULL; }
#define ReleaseSceQueryResults(pqh) if (pqh) { SceFreeQueryResults(pqh); }
#define ReleaseNullSceQueryResults(pqh) if (pqh) { SceFreeQueryResults(pqh); pqh = NULL; }

struct SCE_COLUMN_SCHEMA
{
    LPCWSTR wzName;
    DBTYPE dbtColumnType;
    DWORD dwLength;
    BOOL fPrimaryKey; // If this column is the primary key
    BOOL fNullable;
    BOOL fAutoIncrement;

    LPWSTR wzRelationName;
    DWORD dwForeignKeyTable;
    DWORD dwForeignKeyColumn;
};

struct SCE_INDEX_SCHEMA
{
    LPWSTR wzName;

    DWORD *rgColumns;
    DWORD cColumns;
};

struct SCE_TABLE_SCHEMA
{
    LPCWSTR wzName;
    DWORD cColumns;
    SCE_COLUMN_SCHEMA *rgColumns;

    DWORD cIndexes;
    SCE_INDEX_SCHEMA *rgIndexes;

    // Internal to SCEUtil - consumers shouldn't access or modify
    // TODO: enforce / hide in a handle of some sort?
    IRowset *pIRowset;
    IRowsetChange *pIRowsetChange;
};

struct SCE_DATABASE_SCHEMA
{
    DWORD cTables;
    SCE_TABLE_SCHEMA *rgTables;
};

struct SCE_DATABASE
{
    SCE_DATABASE_HANDLE sdbHandle;
    SCE_DATABASE_SCHEMA *pdsSchema;
};

HRESULT DAPI SceCreateDatabase(
    __in_z LPCWSTR sczFile,
    __deref_out SCE_DATABASE **ppDatabase
    );
HRESULT DAPI SceOpenDatabase(
    __in_z LPCWSTR sczFile,
    __in LPCWSTR wzSchemaType,
    __in DWORD dwExpectedVersion,
    __deref_out SCE_DATABASE **ppDatabase,
    __in BOOL fReadOnly
    );
HRESULT DAPI SceEnsureDatabase(
    __in_z LPCWSTR sczFile,
    __in LPCWSTR wzSchemaType,
    __in DWORD dwExpectedVersion,
    __in SCE_DATABASE_SCHEMA *pdsSchema,
    __deref_out SCE_DATABASE **ppDatabase
    );
HRESULT DAPI SceIsTableEmpty(
    __in SCE_DATABASE *pDatabase,
    __in DWORD dwTableIndex,
    __out BOOL *pfEmpty
    );
HRESULT DAPI SceGetFirstRow(
    __in SCE_DATABASE *pDatabase,
    __in DWORD dwTableIndex,
    __deref_out_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
HRESULT DAPI SceGetNextRow(
    __in SCE_DATABASE *pDatabase,
    __in DWORD dwTableIndex,
    __deref_out_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
HRESULT DAPI SceBeginTransaction(
    __in SCE_DATABASE *pDatabase
    );
HRESULT DAPI SceCommitTransaction(
    __in SCE_DATABASE *pDatabase
    );
HRESULT DAPI SceRollbackTransaction(
    __in SCE_DATABASE *pDatabase
    );
HRESULT DAPI SceDeleteRow(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
HRESULT DAPI ScePrepareInsert(
    __in SCE_DATABASE *pDatabase,
    __in DWORD dwTableIndex,
    __deref_out_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
HRESULT DAPI SceFinishUpdate(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle
    );
HRESULT DAPI SceSetColumnBinary(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in_bcount(cbBuffer) const BYTE* pbBuffer,
    __in SIZE_T cbBuffer
    );
HRESULT DAPI SceSetColumnDword(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in const DWORD dwValue
    );
HRESULT DAPI SceSetColumnQword(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in const DWORD64 qwValue
    );
HRESULT DAPI SceSetColumnBool(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in const BOOL fValue
    );
HRESULT DAPI SceSetColumnString(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in_z_opt LPCWSTR wzValue
    );
HRESULT DAPI SceSetColumnSystemTime(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex,
    __in const SYSTEMTIME *pst
    );
HRESULT DAPI SceSetColumnEmpty(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in DWORD dwColumnIndex
    );
HRESULT DAPI SceGetColumnBinary(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out_opt BYTE **ppbBuffer,
    __inout SIZE_T *pcbBuffer
    );
HRESULT DAPI SceGetColumnDword(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out DWORD *pdwValue
    );
HRESULT DAPI SceGetColumnQword(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out DWORD64 *pqwValue
    );
HRESULT DAPI SceGetColumnBool(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out BOOL *pfValue
    );
HRESULT DAPI SceGetColumnString(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out_z LPWSTR *psczValue
    );
HRESULT DAPI SceGetColumnSystemTime(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle,
    __in DWORD dwColumnIndex,
    __out SYSTEMTIME *pst
    );
HRESULT DAPI SceBeginQuery(
    __in SCE_DATABASE *pDatabase,
    __in DWORD dwTableIndex,
    __in DWORD dwIndex,
    __deref_out_bcount(SCE_QUERY_HANDLE_BYTES) SCE_QUERY_HANDLE *psqhHandle
    );
HRESULT DAPI SceSetQueryColumnBinary(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle,
    __in_bcount(cbBuffer) const BYTE* pbBuffer,
    __in SIZE_T cbBuffer
    );
HRESULT DAPI SceSetQueryColumnDword(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle,
    __in const DWORD dwValue
    );
HRESULT DAPI SceSetQueryColumnQword(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle,
    __in const DWORD64 qwValue
    );
HRESULT DAPI SceSetQueryColumnBool(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle,
    __in const BOOL fValue
    );
HRESULT DAPI SceSetQueryColumnString(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle,
    __in_z_opt LPCWSTR wzString
    );
HRESULT DAPI SceSetQueryColumnSystemTime(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowHandle,
    __in const SYSTEMTIME *pst
    );
HRESULT DAPI SceSetQueryColumnEmpty(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle
    );
HRESULT DAPI SceRunQueryExact(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE *psqhHandle,
    __deref_out_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
HRESULT DAPI SceRunQueryRange(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE *psqhHandle,
    __deref_out_bcount(SCE_QUERY_RESULTS_BYTES) SCE_QUERY_RESULTS_HANDLE *psqrhHandle
    );
HRESULT DAPI SceGetNextResultRow(
    __in_bcount(SCE_QUERY_RESULTS_BYTES) SCE_QUERY_RESULTS_HANDLE sqrhHandle,
    __deref_out_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE *pRowHandle
    );
void DAPI SceCloseTable(
    __in SCE_TABLE_SCHEMA *pTable
    );
HRESULT DAPI SceCloseDatabase(
    __in SCE_DATABASE *pDatabase
    );
void DAPI SceFreeRow(
    __in_bcount(SCE_ROW_HANDLE_BYTES) SCE_ROW_HANDLE rowReadHandle
    );
void DAPI SceFreeQuery(
    __in_bcount(SCE_QUERY_BYTES) SCE_QUERY_HANDLE sqhHandle
    );
void DAPI SceFreeQueryResults(
    __in_bcount(SCE_QUERY_RESULTS_BYTES) SCE_QUERY_RESULTS_HANDLE sqrhHandle
    );

#ifdef __cplusplus
}
#endif
