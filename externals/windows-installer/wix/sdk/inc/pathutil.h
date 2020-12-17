#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

typedef enum PATH_EXPAND
{
    PATH_EXPAND_ENVIRONMENT = 0x0001,
    PATH_EXPAND_FULLPATH    = 0x0002,
} PATH_EXPAND;


/*******************************************************************
 PathCommandLineAppend - appends a command line argument on to a
    string such that ::CommandLineToArgv() will shred them correctly
    (i.e. quote arguments with spaces in them).
********************************************************************/
DAPI_(HRESULT) PathCommandLineAppend(
    __deref_out_z LPWSTR* psczCommandLine,
    __in_z LPCWSTR wzArgument
    );

/*******************************************************************
 PathFile -  returns a pointer to the file part of the path.
********************************************************************/
DAPI_(LPWSTR) PathFile(
    __in_z LPCWSTR wzPath
    );

/*******************************************************************
 PathExtension -  returns a pointer to the extension part of the path
                  (including the dot).
********************************************************************/
DAPI_(LPCWSTR) PathExtension(
    __in_z LPCWSTR wzPath
    );

/*******************************************************************
 PathGetDirectory - extracts the directory from a path.
********************************************************************/
DAPI_(HRESULT) PathGetDirectory(
    __in_z LPCWSTR wzPath,
    __out LPWSTR *psczDirectory
    );

/*******************************************************************
PathGetParentPath - extracts the parent directory from a full path.
********************************************************************/
DAPI_(HRESULT) PathGetParentPath(
    __in_z LPCWSTR wzPath,
    __out LPWSTR *psczDirectory
    );

/*******************************************************************
 PathExpand - gets the full path to a file resolving environment
              variables along the way.
********************************************************************/
DAPI_(HRESULT) PathExpand(
    __out LPWSTR *psczFullPath,
    __in_z LPCWSTR wzRelativePath,
    __in DWORD dwResolveFlags
    );

/*******************************************************************
 PathPrefix - prefixes a full path with \\?\ or \\?\UNC as
              appropriate.
********************************************************************/
DAPI_(HRESULT) PathPrefix(
    __inout LPWSTR *psczFullPath
    );

/*******************************************************************
 PathFixedBackslashTerminate - appends a \ if path does not have it
                                 already, but fails if the buffer is
                                 insufficient.
********************************************************************/
DAPI_(HRESULT) PathFixedBackslashTerminate(
    __inout_ecount_z(cchPath) LPWSTR wzPath,
    __in DWORD_PTR cchPath
    );

/*******************************************************************
 PathBackslashTerminate - appends a \ if path does not have it
                                 already.
********************************************************************/
DAPI_(HRESULT) PathBackslashTerminate(
    __inout LPWSTR* psczPath
    );

/*******************************************************************
 PathForCurrentProcess - gets the full path to the currently executing
                         process or (optionally) a module inside the process.
********************************************************************/
DAPI_(HRESULT) PathForCurrentProcess(
    __inout LPWSTR *psczFullPath,
    __in_opt HMODULE hModule
    );

/*******************************************************************
 PathRelativeToModule - gets the name of a file in the same 
    directory as the current process or (optionally) a module inside 
    the process
********************************************************************/
DAPI_(HRESULT) PathRelativeToModule(
    __inout LPWSTR *psczFullPath,
    __in_opt LPCWSTR wzFileName,
    __in_opt HMODULE hModule
    );

/*******************************************************************
 PathCreateTempFile

 Note: if wzDirectory is null, ::GetTempPath() will be used instead.
       if wzFileNameTemplate is null, GetTempFileName() will be used instead.
*******************************************************************/
DAPI_(HRESULT) PathCreateTempFile(
    __in_opt LPCWSTR wzDirectory,
    __in_opt __format_string LPCWSTR wzFileNameTemplate,
    __in DWORD dwUniqueCount,
    __in DWORD dwFileAttributes,
    __out_opt LPWSTR* psczTempFile,
    __out_opt HANDLE* phTempFile
    );

/*******************************************************************
 PathCreateTimeBasedTempFile - creates an empty temp file based on current
                           system time
********************************************************************/
DAPI_(HRESULT) PathCreateTimeBasedTempFile(
    __in_z_opt LPCWSTR wzDirectory,
    __in_z LPCWSTR wzPrefix,
    __in_z_opt LPCWSTR wzPostfix,
    __in_z LPCWSTR wzExtension,
    __deref_opt_out_z LPWSTR* psczTempFile,
    __out_opt HANDLE* phTempFile
    );

/*******************************************************************
 PathCreateTempDirectory

 Note: if wzDirectory is null, ::GetTempPath() will be used instead.
*******************************************************************/
DAPI_(HRESULT) PathCreateTempDirectory(
    __in_opt LPCWSTR wzDirectory,
    __in __format_string LPCWSTR wzDirectoryNameTemplate,
    __in DWORD dwUniqueCount,
    __out LPWSTR* psczTempDirectory
    );

/*******************************************************************
 PathGetKnownFolder - returns the path to a well-known shell folder

*******************************************************************/
DAPI_(HRESULT) PathGetKnownFolder(
    __in int csidl,
    __out LPWSTR* psczKnownFolder
    );

/*******************************************************************
 PathIsAbsolute - returns true if the path is absolute; false 
    otherwise.
*******************************************************************/
DAPI_(BOOL) PathIsAbsolute(
    __in_z LPCWSTR wzPath
    );

/*******************************************************************
 PathConcat - like .NET's Path.Combine, lets you build up a path
    one piece -- file or directory -- at a time.
*******************************************************************/
DAPI_(HRESULT) PathConcat(
    __in_opt LPCWSTR wzPath1,
    __in_opt LPCWSTR wzPath2,
    __deref_out_z LPWSTR* psczCombined
    );

/*******************************************************************
 PathEnsureQuoted - ensures that a path is quoted; optionally,
     this function also terminates a directory with a backslash
     if it is not already.
*******************************************************************/
DAPI_(HRESULT) PathEnsureQuoted(
    __inout LPWSTR* ppszPath,
    __in BOOL fDirectory
    );

/*******************************************************************
 PathCompare - compares the fully expanded path of the two paths using
               ::CompareStringW().
*******************************************************************/
DAPI_(HRESULT) PathCompare(
    __in_z LPCWSTR wzPath1,
    __in_z LPCWSTR wzPath2,
    __out int* pnResult
    );

/*******************************************************************
 PathCompress - sets the compression state on an existing file or 
                directory. A no-op on file systems that don't 
                support compression.
*******************************************************************/
DAPI_(HRESULT) PathCompress(
    __in_z LPCWSTR wzPath
    );

/*******************************************************************
PathCanonicalizePath - wrapper around PathCanonicalizeW.
*******************************************************************/
DAPI_(HRESULT) PathCanonicalizePath(
    __in_z LPCWSTR wzPath,
    __deref_out_z LPWSTR* psczCanonicalized
    );

/*******************************************************************
PathDirectoryContainsPath - checks if wzPath is located inside
                            wzDirectory.
*******************************************************************/
DAPI_(HRESULT) PathDirectoryContainsPath(
    __in_z LPCWSTR wzDirectory,
    __in_z LPCWSTR wzPath
    );

#ifdef __cplusplus
}
#endif
