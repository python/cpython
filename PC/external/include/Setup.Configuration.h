// The MIT License(MIT)
// Copyright(C) Microsoft Corporation.All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#pragma once

// Constants
//
#ifndef E_NOTFOUND
#define E_NOTFOUND HRESULT_FROM_WIN32(ERROR_NOT_FOUND)
#endif

#ifndef E_FILENOTFOUND
#define E_FILENOTFOUND HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)
#endif

#ifndef E_NOTSUPPORTED
#define E_NOTSUPPORTED HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)
#endif

// Enumerations
//
/// <summary>
/// The state of an instance.
/// </summary>
enum InstanceState
{
    /// <summary>
    /// The instance state has not been determined.
    /// </summary>
    eNone = 0,

    /// <summary>
    /// The instance installation path exists.
    /// </summary>
    eLocal = 1,

    /// <summary>
    /// A product is registered to the instance.
    /// </summary>
    eRegistered = 2,

    /// <summary>
    /// No reboot is required for the instance.
    /// </summary>
    eNoRebootRequired = 4,

    /// <summary>
    /// No errors were reported for the instance.
    /// </summary>
    eNoErrors = 8,

    /// <summary>
    /// The instance represents a complete install.
    /// </summary>
    eComplete = MAXUINT,
};

// Forward interface declarations
//
#ifndef __ISetupInstance_FWD_DEFINED__
#define __ISetupInstance_FWD_DEFINED__
typedef struct ISetupInstance ISetupInstance;
#endif

#ifndef __ISetupInstance2_FWD_DEFINED__
#define __ISetupInstance2_FWD_DEFINED__
typedef struct ISetupInstance2 ISetupInstance2;
#endif

#ifndef __ISetupLocalizedProperties_FWD_DEFINED__
#define __ISetupLocalizedProperties_FWD_DEFINED__
typedef struct ISetupLocalizedProperties ISetupLocalizedProperties;
#endif

#ifndef __IEnumSetupInstances_FWD_DEFINED__
#define __IEnumSetupInstances_FWD_DEFINED__
typedef struct IEnumSetupInstances IEnumSetupInstances;
#endif

#ifndef __ISetupConfiguration_FWD_DEFINED__
#define __ISetupConfiguration_FWD_DEFINED__
typedef struct ISetupConfiguration ISetupConfiguration;
#endif

#ifndef __ISetupConfiguration2_FWD_DEFINED__
#define __ISetupConfiguration2_FWD_DEFINED__
typedef struct ISetupConfiguration2 ISetupConfiguration2;
#endif

#ifndef __ISetupPackageReference_FWD_DEFINED__
#define __ISetupPackageReference_FWD_DEFINED__
typedef struct ISetupPackageReference ISetupPackageReference;
#endif

#ifndef __ISetupHelper_FWD_DEFINED__
#define __ISetupHelper_FWD_DEFINED__
typedef struct ISetupHelper ISetupHelper;
#endif

#ifndef __ISetupErrorState_FWD_DEFINED__
#define __ISetupErrorState_FWD_DEFINED__
typedef struct ISetupErrorState ISetupErrorState;
#endif

#ifndef __ISetupErrorState2_FWD_DEFINED__
#define __ISetupErrorState2_FWD_DEFINED__
typedef struct ISetupErrorState2 ISetupErrorState2;
#endif

#ifndef __ISetupFailedPackageReference_FWD_DEFINED__
#define __ISetupFailedPackageReference_FWD_DEFINED__
typedef struct ISetupFailedPackageReference ISetupFailedPackageReference;
#endif

#ifndef __ISetupFailedPackageReference2_FWD_DEFINED__
#define __ISetupFailedPackageReference2_FWD_DEFINED__
typedef struct ISetupFailedPackageReference2 ISetupFailedPackageReference2;
#endif

#ifndef __ISetupPropertyStore_FWD_DEFINED__
#define __ISetupPropertyStore_FWD_DEFINED__
typedef struct ISetupPropertyStore ISetupPropertyStore;
#endif

#ifndef __ISetupLocalizedPropertyStore_FWD_DEFINED__
#define __ISetupLocalizedPropertyStore_FWD_DEFINED__
typedef struct ISetupLocalizedPropertyStore ISetupLocalizedPropertyStore;
#endif

// Forward class declarations
//
#ifndef __SetupConfiguration_FWD_DEFINED__
#define __SetupConfiguration_FWD_DEFINED__

#ifdef __cplusplus
typedef class SetupConfiguration SetupConfiguration;
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

// Interface definitions
//
EXTERN_C const IID IID_ISetupInstance;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Information about an instance of a product.
/// </summary>
struct DECLSPEC_UUID("B41463C3-8866-43B5-BC33-2B0676F7F42E") DECLSPEC_NOVTABLE ISetupInstance : public IUnknown
{
    /// <summary>
    /// Gets the instance identifier (should match the name of the parent instance directory).
    /// </summary>
    /// <param name="pbstrInstanceId">The instance identifier.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetInstanceId)(
        _Out_ BSTR* pbstrInstanceId
        ) = 0;

    /// <summary>
    /// Gets the local date and time when the installation was originally installed.
    /// </summary>
    /// <param name="pInstallDate">The local date and time when the installation was originally installed.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetInstallDate)(
        _Out_ LPFILETIME pInstallDate
        ) = 0;

    /// <summary>
    /// Gets the unique name of the installation, often indicating the branch and other information used for telemetry.
    /// </summary>
    /// <param name="pbstrInstallationName">The unique name of the installation, often indicating the branch and other information used for telemetry.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetInstallationName)(
        _Out_ BSTR* pbstrInstallationName
        ) = 0;

    /// <summary>
    /// Gets the path to the installation root of the product.
    /// </summary>
    /// <param name="pbstrInstallationPath">The path to the installation root of the product.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetInstallationPath)(
        _Out_ BSTR* pbstrInstallationPath
        ) = 0;

    /// <summary>
    /// Gets the version of the product installed in this instance.
    /// </summary>
    /// <param name="pbstrInstallationVersion">The version of the product installed in this instance.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetInstallationVersion)(
        _Out_ BSTR* pbstrInstallationVersion
        ) = 0;

    /// <summary>
    /// Gets the display name (title) of the product installed in this instance.
    /// </summary>
    /// <param name="lcid">The LCID for the display name.</param>
    /// <param name="pbstrDisplayName">The display name (title) of the product installed in this instance.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetDisplayName)(
        _In_ LCID lcid,
        _Out_ BSTR* pbstrDisplayName
        ) = 0;

    /// <summary>
    /// Gets the description of the product installed in this instance.
    /// </summary>
    /// <param name="lcid">The LCID for the description.</param>
    /// <param name="pbstrDescription">The description of the product installed in this instance.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(GetDescription)(
        _In_ LCID lcid,
        _Out_ BSTR* pbstrDescription
        ) = 0;

    /// <summary>
    /// Resolves the optional relative path to the root path of the instance.
    /// </summary>
    /// <param name="pwszRelativePath">A relative path within the instance to resolve, or NULL to get the root path.</param>
    /// <param name="pbstrAbsolutePath">The full path to the optional relative path within the instance. If the relative path is NULL, the root path will always terminate in a backslash.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the property is not defined.</returns>
    STDMETHOD(ResolvePath)(
        _In_opt_z_ LPCOLESTR pwszRelativePath,
        _Out_ BSTR* pbstrAbsolutePath
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupInstance2;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Information about an instance of a product.
/// </summary>
struct DECLSPEC_UUID("89143C9A-05AF-49B0-B717-72E218A2185C") DECLSPEC_NOVTABLE ISetupInstance2 : public ISetupInstance
{
    /// <summary>
    /// Gets the state of the instance.
    /// </summary>
    /// <param name="pState">The state of the instance.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetState)(
        _Out_ InstanceState* pState
        ) = 0;

    /// <summary>
    /// Gets an array of package references registered to the instance.
    /// </summary>
    /// <param name="ppsaPackages">Pointer to an array of <see cref="ISetupPackageReference"/>.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the packages property is not defined.</returns>
    STDMETHOD(GetPackages)(
        _Out_ LPSAFEARRAY* ppsaPackages
        ) = 0;

    /// <summary>
    /// Gets a pointer to the <see cref="ISetupPackageReference"/> that represents the registered product.
    /// </summary>
    /// <param name="ppPackage">Pointer to an instance of <see cref="ISetupPackageReference"/>. This may be NULL if <see cref="GetState"/> does not return <see cref="eComplete"/>.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist and E_NOTFOUND if the packages property is not defined.</returns>
    STDMETHOD(GetProduct)(
        _Outptr_result_maybenull_ ISetupPackageReference** ppPackage
        ) = 0;

    /// <summary>
    /// Gets the relative path to the product application, if available.
    /// </summary>
    /// <param name="pbstrProductPath">The relative path to the product application, if available.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetProductPath)(
        _Outptr_result_maybenull_ BSTR* pbstrProductPath
        ) = 0;

    /// <summary>
    /// Gets the error state of the instance, if available.
    /// </summary>
    /// <param name="pErrorState">The error state of the instance, if available.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetErrors)(
        _Outptr_result_maybenull_ ISetupErrorState** ppErrorState
        ) = 0;

    /// <summary>
    /// Gets a value indicating whether the instance can be launched.
    /// </summary>
    /// <param name="pfIsLaunchable">Whether the instance can be launched.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    /// <remarks>
    /// An instance could have had errors during install but still be launched. Some features may not work correctly, but others will.
    /// </remarks>
    STDMETHOD(IsLaunchable)(
        _Out_ VARIANT_BOOL* pfIsLaunchable
        ) = 0;

    /// <summary>
    /// Gets a value indicating whether the instance is complete.
    /// </summary>
    /// <param name="pfIsLaunchable">Whether the instance is complete.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    /// <remarks>
    /// An instance is complete if it had no errors during install, resume, or repair.
    /// </remarks>
    STDMETHOD(IsComplete)(
        _Out_ VARIANT_BOOL* pfIsComplete
        ) = 0;

    /// <summary>
    /// Gets product-specific properties.
    /// </summary>
    /// <param name="ppPropeties">A pointer to an instance of <see cref="ISetupPropertyStore"/>. This may be NULL if no properties are defined.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetProperties)(
        _Outptr_result_maybenull_ ISetupPropertyStore** ppProperties
        ) = 0;

    /// <summary>
    /// Gets the directory path to the setup engine that installed the instance.
    /// </summary>
    /// <param name="pbstrEnginePath">The directory path to the setup engine that installed the instance.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_FILENOTFOUND if the instance state does not exist.</returns>
    STDMETHOD(GetEnginePath)(
        _Outptr_result_maybenull_ BSTR* pbstrEnginePath
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupLocalizedProperties;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Provides localized properties of an instance of a product.
/// </summary>
struct DECLSPEC_UUID("F4BD7382-FE27-4AB4-B974-9905B2A148B0") DECLSPEC_NOVTABLE ISetupLocalizedProperties : public IUnknown
{
    /// <summary>
    /// Gets localized product-specific properties.
    /// </summary>
    /// <param name="ppLocalizedProperties">A pointer to an instance of <see cref="ISetupLocalizedPropertyStore"/>. This may be NULL if no properties are defined.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetLocalizedProperties)(
        _Outptr_result_maybenull_ ISetupLocalizedPropertyStore** ppLocalizedProperties
    ) = 0;

    /// <summary>
    /// Gets localized channel-specific properties.
    /// </summary>
    /// <param name="ppLocalizedChannelProperties">A pointer to an instance of <see cref="ISetupLocalizedPropertyStore"/>. This may be NULL if no channel properties are defined.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetLocalizedChannelProperties)(
        _Outptr_result_maybenull_ ISetupLocalizedPropertyStore** ppLocalizedChannelProperties
    ) = 0;
};
#endif

EXTERN_C const IID IID_IEnumSetupInstances;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// An enumerator of installed <see cref="ISetupInstance"/> objects.
/// </summary>
struct DECLSPEC_UUID("6380BCFF-41D3-4B2E-8B2E-BF8A6810C848") DECLSPEC_NOVTABLE IEnumSetupInstances : public IUnknown
{
    /// <summary>
    /// Retrieves the next set of product instances in the enumeration sequence.
    /// </summary>
    /// <param name="celt">The number of product instances to retrieve.</param>
    /// <param name="rgelt">A pointer to an array of <see cref="ISetupInstance"/>.</param>
    /// <param name="pceltFetched">A pointer to the number of product instances retrieved. If <paramref name="celt"/> is 1 this parameter may be NULL.</param>
    /// <returns>S_OK if the number of elements were fetched, S_FALSE if nothing was fetched (at end of enumeration), E_INVALIDARG if <paramref name="celt"/> is greater than 1 and pceltFetched is NULL, or E_OUTOFMEMORY if an <see cref="ISetupInstance"/> could not be allocated.</returns>
    STDMETHOD(Next)(
        _In_ ULONG celt,
        _Out_writes_to_(celt, *pceltFetched) ISetupInstance** rgelt,
        _Out_opt_ _Deref_out_range_(0, celt) ULONG* pceltFetched
        ) = 0;

    /// <summary>
    /// Skips the next set of product instances in the enumeration sequence.
    /// </summary>
    /// <param name="celt">The number of product instances to skip.</param>
    /// <returns>S_OK if the number of elements could be skipped; otherwise, S_FALSE;</returns>
    STDMETHOD(Skip)(
        _In_ ULONG celt
        ) = 0;

    /// <summary>
    /// Resets the enumeration sequence to the beginning.
    /// </summary>
    /// <returns>Always returns S_OK;</returns>
    STDMETHOD(Reset)(void) = 0;

    /// <summary>
    /// Creates a new enumeration object in the same state as the current enumeration object: the new object points to the same place in the enumeration sequence.
    /// </summary>
    /// <param name="ppenum">A pointer to a pointer to a new <see cref="IEnumSetupInstances"/> interface. If the method fails, this parameter is undefined.</param>
    /// <returns>S_OK if a clone was returned; otherwise, E_OUTOFMEMORY.</returns>
    STDMETHOD(Clone)(
        _Deref_out_opt_ IEnumSetupInstances** ppenum
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupConfiguration;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Gets information about product instances installed on the machine.
/// </summary>
struct DECLSPEC_UUID("42843719-DB4C-46C2-8E7C-64F1816EFD5B") DECLSPEC_NOVTABLE ISetupConfiguration : public IUnknown
{
    /// <summary>
    /// Enumerates all launchable product instances installed.
    /// </summary>
    /// <param name="ppEnumInstances">An enumeration of completed, installed product instances.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(EnumInstances)(
        _Out_ IEnumSetupInstances** ppEnumInstances
        ) = 0;

    /// <summary>
    /// Gets the instance for the current process path.
    /// </summary>
    /// <param name="ppInstance">The instance for the current process path.</param>
    /// <returns>
    /// The instance for the current process path, or E_NOTFOUND if not found.
    /// The <see cref="ISetupInstance::GetState"/> may indicate the instance is invalid.
    /// </returns>
    /// <remarks>
    /// The returned instance may not be launchable.
    /// </remarks>
STDMETHOD(GetInstanceForCurrentProcess)(
        _Out_ ISetupInstance** ppInstance
        ) = 0;

    /// <summary>
    /// Gets the instance for the given path.
    /// </summary>
    /// <param name="ppInstance">The instance for the given path.</param>
    /// <returns>
    /// The instance for the given path, or E_NOTFOUND if not found.
    /// The <see cref="ISetupInstance::GetState"/> may indicate the instance is invalid.
    /// </returns>
    /// <remarks>
    /// The returned instance may not be launchable.
    /// </remarks>
STDMETHOD(GetInstanceForPath)(
        _In_z_ LPCWSTR wzPath,
        _Out_ ISetupInstance** ppInstance
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupConfiguration2;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Gets information about product instances.
/// </summary>
struct DECLSPEC_UUID("26AAB78C-4A60-49D6-AF3B-3C35BC93365D") DECLSPEC_NOVTABLE ISetupConfiguration2 : public ISetupConfiguration
{
    /// <summary>
    /// Enumerates all product instances.
    /// </summary>
    /// <param name="ppEnumInstances">An enumeration of all product instances.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(EnumAllInstances)(
        _Out_ IEnumSetupInstances** ppEnumInstances
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupPackageReference;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// A reference to a package.
/// </summary>
struct DECLSPEC_UUID("da8d8a16-b2b6-4487-a2f1-594ccccd6bf5") DECLSPEC_NOVTABLE ISetupPackageReference : public IUnknown
{
    /// <summary>
    /// Gets the general package identifier.
    /// </summary>
    /// <param name="pbstrId">The general package identifier.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetId)(
        _Out_ BSTR* pbstrId
        ) = 0;

    /// <summary>
    /// Gets the version of the package.
    /// </summary>
    /// <param name="pbstrVersion">The version of the package.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetVersion)(
        _Out_ BSTR* pbstrVersion
        ) = 0;

    /// <summary>
    /// Gets the target process architecture of the package.
    /// </summary>
    /// <param name="pbstrChip">The target process architecture of the package.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetChip)(
        _Out_ BSTR* pbstrChip
        ) = 0;

    /// <summary>
    /// Gets the language and optional region identifier.
    /// </summary>
    /// <param name="pbstrLanguage">The language and optional region identifier.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetLanguage)(
        _Out_ BSTR* pbstrLanguage
        ) = 0;

    /// <summary>
    /// Gets the build branch of the package.
    /// </summary>
    /// <param name="pbstrBranch">The build branch of the package.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetBranch)(
        _Out_ BSTR* pbstrBranch
        ) = 0;

    /// <summary>
    /// Gets the type of the package.
    /// </summary>
    /// <param name="pbstrType">The type of the package.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetType)(
        _Out_ BSTR* pbstrType
        ) = 0;

    /// <summary>
    /// Gets the unique identifier consisting of all defined tokens.
    /// </summary>
    /// <param name="pbstrUniqueId">The unique identifier consisting of all defined tokens.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_UNEXPECTED if no Id was defined (required).</returns>
    STDMETHOD(GetUniqueId)(
        _Out_ BSTR* pbstrUniqueId
        ) = 0;

    /// <summary>
    /// Gets a value indicating whether the package refers to an external extension.
    /// </summary>
    /// <param name="pfIsExtension">A value indicating whether the package refers to an external extension.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_UNEXPECTED if no Id was defined (required).</returns>
    STDMETHOD(GetIsExtension)(
        _Out_ VARIANT_BOOL* pfIsExtension
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupHelper;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Helper functions.
/// </summary>
/// <remarks>
/// You can query for this interface from the <see cref="SetupConfiguration"/> class.
/// </remarks>
struct DECLSPEC_UUID("42b21b78-6192-463e-87bf-d577838f1d5c") DECLSPEC_NOVTABLE ISetupHelper : public IUnknown
{
    /// <summary>
    /// Parses a dotted quad version string into a 64-bit unsigned integer.
    /// </summary>
    /// <param name="pwszVersion">The dotted quad version string to parse, e.g. 1.2.3.4.</param>
    /// <param name="pullVersion">A 64-bit unsigned integer representing the version. You can compare this to other versions.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_INVALIDARG if the version is not valid.</returns>
    STDMETHOD(ParseVersion)(
        _In_ LPCOLESTR pwszVersion,
        _Out_ PULONGLONG pullVersion
        ) = 0;

    /// <summary>
    /// Parses a dotted quad version string into a 64-bit unsigned integer.
    /// </summary>
    /// <param name="pwszVersionRange">The string containing 1 or 2 dotted quad version strings to parse, e.g. [1.0,) that means 1.0.0.0 or newer.</param>
    /// <param name="pullMinVersion">A 64-bit unsigned integer representing the minimum version, which may be 0. You can compare this to other versions.</param>
    /// <param name="pullMaxVersion">A 64-bit unsigned integer representing the maximum version, which may be MAXULONGLONG. You can compare this to other versions.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_INVALIDARG if the version range is not valid.</returns>
    STDMETHOD(ParseVersionRange)(
        _In_ LPCOLESTR pwszVersionRange,
        _Out_ PULONGLONG pullMinVersion,
        _Out_ PULONGLONG pullMaxVersion
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupErrorState;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Information about the error state of an instance.
/// </summary>
struct DECLSPEC_UUID("46DCCD94-A287-476A-851E-DFBC2FFDBC20") DECLSPEC_NOVTABLE ISetupErrorState : public IUnknown
{
    /// <summary>
    /// Gets an array of failed package references.
    /// </summary>
    /// <param name="ppsaPackages">Pointer to an array of <see cref="ISetupFailedPackageReference"/>, if packages have failed.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetFailedPackages)(
        _Outptr_result_maybenull_ LPSAFEARRAY* ppsaFailedPackages
        ) = 0;

    /// <summary>
    /// Gets an array of skipped package references.
    /// </summary>
    /// <param name="ppsaPackages">Pointer to an array of <see cref="ISetupPackageReference"/>, if packages have been skipped.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetSkippedPackages)(
        _Outptr_result_maybenull_ LPSAFEARRAY* ppsaSkippedPackages
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupErrorState2;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Information about the error state of an instance.
/// </summary>
struct DECLSPEC_UUID("9871385B-CA69-48F2-BC1F-7A37CBF0B1EF") DECLSPEC_NOVTABLE ISetupErrorState2 : public ISetupErrorState
{
    /// <summary>
    /// Gets the path to the error log.
    /// </summary>
    /// <param name="pbstrChip">The path to the error log.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetErrorLogFilePath)(
        _Outptr_result_maybenull_ BSTR* pbstrErrorLogFilePath
        ) = 0;
};
#endif

EXTERN_C const IID IID_ISetupFailedPackageReference;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// A reference to a failed package.
/// </summary>
struct DECLSPEC_UUID("E73559CD-7003-4022-B134-27DC650B280F") DECLSPEC_NOVTABLE ISetupFailedPackageReference : public ISetupPackageReference
{
};

#endif

EXTERN_C const IID IID_ISetupFailedPackageReference2;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// A reference to a failed package.
/// </summary>
struct DECLSPEC_UUID("0FAD873E-E874-42E3-B268-4FE2F096B9CA") DECLSPEC_NOVTABLE ISetupFailedPackageReference2 : public ISetupFailedPackageReference
{
    /// <summary>
    /// Gets the path to the optional package log.
    /// </summary>
    /// <param name="pbstrId">The path to the optional package log.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetLogFilePath)(
        _Outptr_result_maybenull_ BSTR* pbstrLogFilePath
        ) = 0;

    /// <summary>
    /// Gets the description of the package failure.
    /// </summary>
    /// <param name="pbstrId">The description of the package failure.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetDescription)(
        _Outptr_result_maybenull_ BSTR* pbstrDescription
        ) = 0;

    /// <summary>
    /// Gets the signature to use for feedback reporting.
    /// </summary>
    /// <param name="pbstrId">The signature to use for feedback reporting.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetSignature)(
        _Outptr_result_maybenull_ BSTR* pbstrSignature
        ) = 0;

    /// <summary>
    ///  Gets the array of details for this package failure.
    /// </summary>
    /// <param name="ppsaDetails">Pointer to an array of details as BSTRs.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetDetails)(
        _Out_ LPSAFEARRAY* ppsaDetails
        ) = 0;

    /// <summary>
    /// Gets an array of packages affected by this package failure.
    /// </summary>
    /// <param name="ppsaPackages">Pointer to an array of <see cref="ISetupPackageReference"/> for packages affected by this package failure. This may be NULL.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetAffectedPackages)(
        _Out_ LPSAFEARRAY* ppsaAffectedPackages
        ) = 0;
};

#endif

EXTERN_C const IID IID_ISetupPropertyStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Provides named properties.
/// </summary>
/// <remarks>
/// You can get this from an <see cref="ISetupInstance"/>, <see cref="ISetupPackageReference"/>, or derivative.
/// </remarks>
struct DECLSPEC_UUID("C601C175-A3BE-44BC-91F6-4568D230FC83") DECLSPEC_NOVTABLE ISetupPropertyStore : public IUnknown
{
    /// <summary>
    /// Gets an array of property names in this property store.
    /// </summary>
    /// <param name="ppsaNames">Pointer to an array of property names as BSTRs.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetNames)(
        _Out_ LPSAFEARRAY* ppsaNames
        ) = 0;

    /// <summary>
    /// Gets the value of a named property in this property store.
    /// </summary>
    /// <param name="pwszName">The name of the property to get.</param>
    /// <param name="pvtValue">The value of the property.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_NOTFOUND if the property is not defined or E_NOTSUPPORTED if the property type is not supported.</returns>
    STDMETHOD(GetValue)(
        _In_ LPCOLESTR pwszName,
        _Out_ LPVARIANT pvtValue
        ) = 0;
};

#endif

EXTERN_C const IID IID_ISetupLocalizedPropertyStore;

#if defined(__cplusplus) && !defined(CINTERFACE)
/// <summary>
/// Provides localized named properties.
/// </summary>
/// <remarks>
/// You can get this from an <see cref="ISetupLocalizedProperties"/>.
/// </remarks>
struct DECLSPEC_UUID("5BB53126-E0D5-43DF-80F1-6B161E5C6F6C") DECLSPEC_NOVTABLE ISetupLocalizedPropertyStore : public IUnknown
{
    /// <summary>
    /// Gets an array of property names in this property store.
    /// </summary>
    /// <param name="lcid">The LCID for the property names.</param>
    /// <param name="ppsaNames">Pointer to an array of property names as BSTRs.</param>
    /// <returns>Standard HRESULT indicating success or failure.</returns>
    STDMETHOD(GetNames)(
        _In_ LCID lcid,
        _Out_ LPSAFEARRAY* ppsaNames
        ) = 0;

    /// <summary>
    /// Gets the value of a named property in this property store.
    /// </summary>
    /// <param name="pwszName">The name of the property to get.</param>
    /// <param name="lcid">The LCID for the property.</param>
    /// <param name="pvtValue">The value of the property.</param>
    /// <returns>Standard HRESULT indicating success or failure, including E_NOTFOUND if the property is not defined or E_NOTSUPPORTED if the property type is not supported.</returns>
    STDMETHOD(GetValue)(
        _In_ LPCOLESTR pwszName,
        _In_ LCID lcid,
        _Out_ LPVARIANT pvtValue
        ) = 0;
};

#endif

// Class declarations
//
EXTERN_C const CLSID CLSID_SetupConfiguration;

#ifdef __cplusplus
/// <summary>
/// This class implements <see cref="ISetupConfiguration"/>, <see cref="ISetupConfiguration2"/>, and <see cref="ISetupHelper"/>.
/// </summary>
class DECLSPEC_UUID("177F0C4A-1CD3-4DE7-A32C-71DBBB9FA36D") SetupConfiguration;
#endif

// Function declarations
//
/// <summary>
/// Gets an <see cref="ISetupConfiguration"/> that provides information about product instances installed on the machine.
/// </summary>
/// <param name="ppConfiguration">The <see cref="ISetupConfiguration"/> that provides information about product instances installed on the machine.</param>
/// <param name="pReserved">Reserved for future use.</param>
/// <returns>Standard HRESULT indicating success or failure.</returns>
STDMETHODIMP GetSetupConfiguration(
    _Out_ ISetupConfiguration** ppConfiguration,
    _Reserved_ LPVOID pReserved
);

#ifdef __cplusplus
}
#endif
