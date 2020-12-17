#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


#ifdef __cplusplus
extern "C" {
#endif

#define ReleaseAtomFeed(p) if (p) { AtomFreeFeed(p); }
#define ReleaseNullAtomFeed(p) if (p) { AtomFreeFeed(p); p = NULL; }


struct ATOM_UNKNOWN_ATTRIBUTE
{
    LPWSTR wzNamespace;
    LPWSTR wzAttribute;
    LPWSTR wzValue;

    ATOM_UNKNOWN_ATTRIBUTE* pNext;
};

struct ATOM_UNKNOWN_ELEMENT
{
    LPWSTR wzNamespace;
    LPWSTR wzElement;
    LPWSTR wzValue;

    ATOM_UNKNOWN_ATTRIBUTE* pAttributes;
    ATOM_UNKNOWN_ELEMENT* pNext;
};

struct ATOM_LINK
{
    LPWSTR wzRel;
    LPWSTR wzTitle;
    LPWSTR wzType;
    LPWSTR wzUrl;
    LPWSTR wzValue;
    DWORD64 dw64Length;

    ATOM_UNKNOWN_ATTRIBUTE* pUnknownAttributes;
    ATOM_UNKNOWN_ELEMENT* pUnknownElements;
};

struct ATOM_CONTENT
{
    LPWSTR wzType;
    LPWSTR wzUrl;
    LPWSTR wzValue;

    ATOM_UNKNOWN_ELEMENT* pUnknownElements;
};

struct ATOM_AUTHOR
{
    LPWSTR wzName;
    LPWSTR wzEmail;
    LPWSTR wzUrl;
};

struct ATOM_CATEGORY
{
    LPWSTR wzLabel;
    LPWSTR wzScheme;
    LPWSTR wzTerm;

    ATOM_UNKNOWN_ELEMENT* pUnknownElements;
};

struct ATOM_ENTRY
{
    LPWSTR wzId;
    LPWSTR wzSummary;
    LPWSTR wzTitle;
    FILETIME ftPublished;
    FILETIME ftUpdated;

    ATOM_CONTENT* pContent;

    DWORD cAuthors;
    ATOM_AUTHOR* rgAuthors;

    DWORD cCategories;
    ATOM_CATEGORY* rgCategories;

    DWORD cLinks;
    ATOM_LINK* rgLinks;

    IXMLDOMNode* pixn;
    ATOM_UNKNOWN_ELEMENT* pUnknownElements;
};

struct ATOM_FEED
{
    LPWSTR wzGenerator;
    LPWSTR wzIcon;
    LPWSTR wzId;
    LPWSTR wzLogo;
    LPWSTR wzSubtitle;
    LPWSTR wzTitle;
    FILETIME ftUpdated;

    DWORD cAuthors;
    ATOM_AUTHOR* rgAuthors;

    DWORD cCategories;
    ATOM_CATEGORY* rgCategories;

    DWORD cEntries;
    ATOM_ENTRY* rgEntries;

    DWORD cLinks;
    ATOM_LINK* rgLinks;

    IXMLDOMNode* pixn;
    ATOM_UNKNOWN_ELEMENT* pUnknownElements;
};

HRESULT DAPI AtomInitialize(
    );

void DAPI AtomUninitialize(
    );

HRESULT DAPI AtomParseFromString(
    __in_z LPCWSTR wzAtomString,
    __out ATOM_FEED **ppFeed
    );

HRESULT DAPI AtomParseFromFile(
    __in_z LPCWSTR wzAtomFile,
    __out ATOM_FEED **ppFeed
    );

HRESULT DAPI AtomParseFromDocument(
    __in IXMLDOMDocument* pixdDocument,
    __out ATOM_FEED **ppFeed
    );

void DAPI AtomFreeFeed(
    __in_xcount(pFeed->cItems) ATOM_FEED *pFEED
    );

#ifdef __cplusplus
}
#endif
