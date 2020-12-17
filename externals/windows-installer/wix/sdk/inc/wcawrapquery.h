#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include "wcautil.h"

// Enumerations
typedef enum eWrapQueryAction
{
    wqaTableBegin = 1,
    wqaTableFinish,
    wqaRowBegin,
    wqaRowFinish
} eWrapQueryAction;

typedef enum eColumnDataType
{
    cdtString = 1,
    cdtInt,
    cdtStream,
    cdtUnknown
} eColumnDataType;

typedef enum eFormatMaskColumn
{
    efmcColumn1 = 1,
    efmcColumn2 = 1 << 1,
    efmcColumn3 = 1 << 2,
    efmcColumn4 = 1 << 3,
    efmcColumn5 = 1 << 4,
    efmcColumn6 = 1 << 5,
    efmcColumn7 = 1 << 6,
    efmcColumn8 = 1 << 7,
    efmcColumn9 = 1 << 8,
    efmcColumn10 = 1 << 9,
    efmcColumn11 = 1 << 10,
    efmcColumn12 = 1 << 11,
    efmcColumn13 = 1 << 12,
    efmcColumn14 = 1 << 13,
    efmcColumn15 = 1 << 14,
    efmcColumn16 = 1 << 15,
    efmcColumn17 = 1 << 16,
    efmcColumn18 = 1 << 17,
    efmcColumn19 = 1 << 18,
    efmcColumn20 = 1 << 19,
    efmcColumn21 = 1 << 20,
    efmcColumn22 = 1 << 21,
    efmcColumn23 = 1 << 22,
    efmcColumn24 = 1 << 23,
    efmcColumn25 = 1 << 24,
    efmcColumn26 = 1 << 25,
    efmcColumn27 = 1 << 26,
    efmcColumn28 = 1 << 27,
    efmcColumn29 = 1 << 28,
    efmcColumn30 = 1 << 29,
    efmcColumn31 = 1 << 30,
    efmcColumn32 = 1 << 31,
} eFormatMaskColumn;

// Keeps track of the query instance for the reading CA (deferred CA)
typedef struct WCA_WRAPQUERY_STRUCT
{
    // These are used to size our dynamic arrays below
    DWORD dwColumns, dwRows, dwNextIndex;

    // Dynamic arrays of column schema information
    eColumnDataType *pcdtColumnType;
    LPWSTR *ppwzColumnNames;

    // Dynamic array of raw record data
    MSIHANDLE *phRecords;
} *WCA_WRAPQUERY_HANDLE;

// Wrap a query
// Setting the pfFormatMask enables control over which fields will be formatted, and which will be left unchanged
// Setting dwComponentColumn to something other than 0xFFFFFFFF tells WcaWrapQuery to add two additional columns to the right side of the table
//      - ISInstalled and ISAction - which map to the ComponentState of the component (the component is found in the column specified)
//      Note that if a component is NULL, the component state columns will also be left null, and it will be up to the deferred CA to fail or ignore the case appropriately
// Setting dwDirectoryColumn to something other than 0xFFFFFFFF tells WcaWrapQuery to add two more additional columns to the right side of the table
//      - SourcePath and TargetPath - which map to the Directory's Source and Target Path (the directory is found in the column specified)
//      Note that if a directory is NULL, the directory source/target path columns will also be left null, and it will be up to the deferred CA to fail or ignore the case appropriately
HRESULT WIXAPI WcaWrapQuery(
    __in_z LPCWSTR pwzQuery,
    __inout LPWSTR * ppwzCustomActionData,
    __in_opt DWORD dwFormatMask,
    __in_opt DWORD dwComponentColumn,
    __in_opt DWORD dwDirectoryColumn
    );
// This wraps an empty table query into the custom action data - this is a way to indicate to the deferred custom action that a necessary table doesn't exist, or its query returned no results
HRESULT WIXAPI WcaWrapEmptyQuery(
    __inout LPWSTR * ppwzCustomActionData
    );

// Open a new unwrap query operation, with data from the ppwzCustomActionData string
HRESULT WIXAPI WcaBeginUnwrapQuery(
    __out WCA_WRAPQUERY_HANDLE * phWrapQuery,
    __inout LPWSTR * ppwzCustomActionData
    );

// Get the number of records in a query being unwrapped
DWORD WIXAPI WcaGetQueryRecords(
    __in const WCA_WRAPQUERY_HANDLE hWrapQuery
    );

// This function resets a query back to its first row, so that the next fetch returns the first record
void WIXAPI WcaFetchWrappedReset(
    __in WCA_WRAPQUERY_HANDLE hWrapQuery
    );
// Fetch the next record in this query
// NOTE: the MSIHANDLE returned by this function should not be released, as it is the same handle used by the query object to maintain the item.
//       so, don't use this function with PMSIHANDLE objects!
HRESULT WIXAPI WcaFetchWrappedRecord(
    __in WCA_WRAPQUERY_HANDLE hWrapQuery,
    __out MSIHANDLE* phRec
    );

// Fetch the next record in the query where the string value in column dwComparisonColumn equals the value pwzExpectedValue
// NOTE: the MSIHANDLE returned by this function should not be released, as it is the same handle used by the query object to maintain the item.
//       so, don't use this function with PMSIHANDLE objects!
HRESULT WIXAPI WcaFetchWrappedRecordWhereString(
    __in WCA_WRAPQUERY_HANDLE hWrapQuery,
    __in DWORD dwComparisonColumn,
    __in_z LPCWSTR pwzExpectedValue,
    __out MSIHANDLE* phRec
    );

// Release a query ID (frees memory, and frees the ID for a new query)
void WIXAPI WcaFinishUnwrapQuery(
    __in_opt WCA_WRAPQUERY_HANDLE hWrapQuery
    );
