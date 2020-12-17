#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#include <aclapi.h>
#include <sddl.h>

#define ReleaseSid(x) if (x) { AclFreeSid(x); }
#define ReleaseNullSid(x) if (x) { AclFreeSid(x); x = NULL; }

#ifdef __cplusplus
extern "C" {
#endif

// structs
struct ACL_ACCESS
{
    BOOL fDenyAccess;
    DWORD dwAccessMask;

    // TODO: consider using a union
    LPCWSTR pwzAccountName;   // NOTE: the last three items in this structure are ignored if this is not NULL

    SID_IDENTIFIER_AUTHORITY sia;  // used if pwzAccountName is NULL
    BYTE nSubAuthorityCount;
    DWORD nSubAuthority[8];
};

struct ACL_ACE
{
    DWORD dwFlags;
    DWORD dwMask;
    PSID psid;
};


// functions
HRESULT DAPI AclCheckAccess(
    __in HANDLE hToken, 
    __in ACL_ACCESS* paa
    );
HRESULT DAPI AclCheckAdministratorAccess(
    __in HANDLE hToken
    );
HRESULT DAPI AclCheckLocalSystemAccess(
    __in HANDLE hToken
    );

HRESULT DAPI AclGetWellKnownSid(
    __in WELL_KNOWN_SID_TYPE wkst,
    __deref_out PSID* ppsid
    );
HRESULT DAPI AclGetAccountSid(
    __in_opt LPCWSTR wzSystem,
    __in_z LPCWSTR wzAccount,
    __deref_out PSID* ppsid
    );
HRESULT DAPI AclGetAccountSidString(
    __in_z LPCWSTR wzSystem,
    __in_z LPCWSTR wzAccount,
    __deref_out_z LPWSTR* ppwzSid
    );

HRESULT DAPI AclCreateDacl(
    __in_ecount(cDeny) ACL_ACE rgaaDeny[],
    __in DWORD cDeny,
    __in_ecount(cAllow) ACL_ACE rgaaAllow[],
    __in DWORD cAllow,
    __deref_out ACL** ppAcl
    );
HRESULT DAPI AclAddToDacl(
    __in ACL* pAcl,
    __in_ecount_opt(cDeny) const ACL_ACE rgaaDeny[],
    __in DWORD cDeny,
    __in_ecount_opt(cAllow) const ACL_ACE rgaaAllow[],
    __in DWORD cAllow,
    __deref_out ACL** ppAclNew
    );
HRESULT DAPI AclMergeDacls(
    __in const ACL* pAcl1,
    __in const ACL* pAcl2,
    __deref_out ACL** ppAclNew
    );
HRESULT DAPI AclCreateDaclOld(
    __in_ecount(cAclAccesses) ACL_ACCESS* paa,
    __in DWORD cAclAccesses,
    __deref_out ACL** ppAcl
    );
HRESULT DAPI AclCreateSecurityDescriptor(
    __in_ecount(cAclAccesses) ACL_ACCESS* paa,
    __in DWORD cAclAccesses,
    __deref_out SECURITY_DESCRIPTOR** ppsd
    );
HRESULT DAPI AclCreateSecurityDescriptorFromDacl(
    __in ACL* pACL,
    __deref_out SECURITY_DESCRIPTOR** ppsd
    );
HRESULT __cdecl AclCreateSecurityDescriptorFromString(
    __deref_out SECURITY_DESCRIPTOR** ppsd,
    __in_z __format_string LPCWSTR wzSddlFormat,
    ...
    );
HRESULT DAPI AclDuplicateSecurityDescriptor(
    __in SECURITY_DESCRIPTOR* psd,
    __deref_out SECURITY_DESCRIPTOR** ppsd
    );
HRESULT DAPI AclGetSecurityDescriptor(
    __in_z LPCWSTR wzObject,
    __in SE_OBJECT_TYPE sot,
    __in SECURITY_INFORMATION securityInformation,
    __deref_out SECURITY_DESCRIPTOR** ppsd
    );
HRESULT DAPI AclSetSecurityWithRetry(
    __in_z LPCWSTR wzObject,
    __in SE_OBJECT_TYPE sot,
    __in SECURITY_INFORMATION securityInformation,
    __in_opt PSID psidOwner,
    __in_opt PSID psidGroup,
    __in_opt PACL pDacl,
    __in_opt PACL pSacl,
    __in DWORD cRetry,
    __in DWORD dwWaitMilliseconds
    );

HRESULT DAPI AclFreeSid(
    __in PSID psid
    );
HRESULT DAPI AclFreeDacl(
    __in ACL* pACL
    );
HRESULT DAPI AclFreeSecurityDescriptor(
    __in SECURITY_DESCRIPTOR* psd
    );

HRESULT DAPI AclAddAdminToSecurityDescriptor(
    __in SECURITY_DESCRIPTOR* pSecurity,
    __deref_out SECURITY_DESCRIPTOR** ppSecurityNew
    );

// Following code in acl2util.cpp due to dependency on crypt32.dll.
HRESULT DAPI AclCalculateServiceSidString(
    __in LPCWSTR wzServiceName,
    __in int cchServiceName,
    __deref_out_z LPWSTR* psczSid
    );
HRESULT DAPI AclGetAccountSidStringEx(
    __in_z LPCWSTR wzSystem,
    __in_z LPCWSTR wzAccount,
    __deref_out_z LPWSTR* psczSid
    );
#ifdef __cplusplus
}
#endif
