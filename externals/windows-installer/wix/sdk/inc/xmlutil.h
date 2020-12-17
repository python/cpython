#pragma once
// Copyright (c) .NET Foundation and contributors. All rights reserved. Licensed under the Microsoft Reciprocal License. See LICENSE.TXT file in the project root for full license information.


extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument   = {0x2933BF90, 0x7B36, 0x11d2, {0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument20  = {0xF6D90F11, 0x9C73, 0x11D3, {0xB3, 0x2E, 0x00, 0xC0, 0x4F, 0x99, 0x0B, 0xB4}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument26 = {0xf5078f1b, 0xc551, 0x11d3, {0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument30 = {0xf5078f32, 0xc551, 0x11d3, {0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument40 = {0x88d969c0, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument50 = {0x88d969e5, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_DOMDocument60 = {0x88d96a05, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}};
extern __declspec(selectany) const CLSID XmlUtil_CLSID_XMLSchemaCache = {0x88d969c2, 0xf192, 0x11d4, {0xa6, 0x5f, 0x00, 0x40, 0x96, 0x32, 0x51, 0xe5}};

extern __declspec(selectany) const IID XmlUtil_IID_IXMLDOMDocument =  {0x2933BF81, 0x7B36, 0x11D2, {0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60}};
extern __declspec(selectany) const IID XmlUtil_IID_IXMLDOMDocument2 = {0x2933BF95, 0x7B36, 0x11D2, {0xB2, 0x0E, 0x00, 0xC0, 0x4F, 0x98, 0x3E, 0x60}};
extern __declspec(selectany) const IID XmlUtil_IID_IXMLDOMSchemaCollection = {0x373984C8, 0xB845, 0x449B, {0x91, 0xE7, 0x45, 0xAC, 0x83, 0x03, 0x6A, 0xDE}};

typedef enum XML_LOAD_ATTRIBUTE
{
    XML_LOAD_PRESERVE_WHITESPACE = 1,
} XML_LOAD_ATTRIBUTE;


#ifdef __cplusplus
extern "C" {
#endif

HRESULT DAPI XmlInitialize();
void DAPI XmlUninitialize();

HRESULT DAPI XmlCreateElement(
    __in IXMLDOMDocument *pixdDocument,
    __in_z LPCWSTR wzElementName,
    __out IXMLDOMElement **ppixnElement
    );
HRESULT DAPI XmlCreateDocument(
    __in_opt LPCWSTR pwzElementName, 
    __out IXMLDOMDocument** ppixdDocument,
    __out_opt IXMLDOMElement** ppixeRootElement = NULL
    );
HRESULT DAPI XmlLoadDocument(
    __in_z LPCWSTR wzDocument,
    __out IXMLDOMDocument** ppixdDocument
    );
HRESULT DAPI XmlLoadDocumentEx(
    __in_z LPCWSTR wzDocument,
    __in DWORD dwAttributes,
    __out IXMLDOMDocument** ppixdDocument
    );
HRESULT DAPI XmlLoadDocumentFromFile(
    __in_z LPCWSTR wzPath,
    __out IXMLDOMDocument** ppixdDocument
    );
HRESULT DAPI XmlLoadDocumentFromBuffer(
    __in_bcount(cbSource) const BYTE* pbSource,
    __in DWORD cbSource,
    __out IXMLDOMDocument** ppixdDocument
    );
HRESULT DAPI XmlLoadDocumentFromFileEx(
    __in_z LPCWSTR wzPath,
    __in DWORD dwAttributes,
    __out IXMLDOMDocument** ppixdDocument
    );
HRESULT DAPI XmlSelectSingleNode(
    __in IXMLDOMNode* pixnParent,
    __in_z LPCWSTR wzXPath,
    __out IXMLDOMNode **ppixnChild
    );
HRESULT DAPI XmlSetAttribute(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute,
    __in_z LPCWSTR pwzAttributeValue
    );
HRESULT DAPI XmlCreateTextNode(
    __in IXMLDOMDocument *pixdDocument,
    __in_z LPCWSTR wzText,
    __out IXMLDOMText **ppixnTextNode
    );
HRESULT DAPI XmlGetText(
    __in IXMLDOMNode* pixnNode,
    __deref_out_z BSTR* pbstrText
    );
HRESULT DAPI XmlGetAttribute(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute,
    __deref_out_z BSTR* pbstrAttributeValue
    );
HRESULT DAPI XmlGetAttributeEx(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR wzAttribute,
    __deref_out_z LPWSTR* psczAttributeValue
    );
HRESULT DAPI XmlGetYesNoAttribute(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR wzAttribute,
    __out BOOL* pfYes
    );
HRESULT DAPI XmlGetAttributeNumber(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute,
    __out DWORD* pdwValue
    );
HRESULT DAPI XmlGetAttributeNumberBase(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute,
    __in int nBase,
    __out DWORD* pdwValue
    );
HRESULT DAPI XmlGetAttributeLargeNumber(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute,
    __out DWORD64* pdw64Value
    );
HRESULT DAPI XmlGetNamedItem(
    __in IXMLDOMNamedNodeMap *pixnmAttributes, 
    __in_opt LPCWSTR wzName, 
    __out IXMLDOMNode **ppixnNamedItem
    );
HRESULT DAPI XmlSetText(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzText
    );
HRESULT DAPI XmlSetTextNumber(
    __in IXMLDOMNode *pixnNode,
    __in DWORD dwValue
    );
HRESULT DAPI XmlCreateChild(
    __in IXMLDOMNode* pixnParent,
    __in_z LPCWSTR pwzElementType,
    __out IXMLDOMNode** ppixnChild
    );
HRESULT DAPI XmlRemoveAttribute(
    __in IXMLDOMNode* pixnNode,
    __in_z LPCWSTR pwzAttribute
    );
HRESULT DAPI XmlSelectNodes(
    __in IXMLDOMNode* pixnParent,
    __in_z LPCWSTR wzXPath,
    __out IXMLDOMNodeList **ppixnChild
    );
HRESULT DAPI XmlNextAttribute(
    __in IXMLDOMNamedNodeMap* pixnnm,
    __out IXMLDOMNode** pixnAttribute,
    __deref_opt_out_z_opt BSTR* pbstrAttribute
    );
HRESULT DAPI XmlNextElement(
    __in IXMLDOMNodeList* pixnl,
    __out IXMLDOMNode** pixnElement,
    __deref_opt_out_z_opt BSTR* pbstrElement
    );
HRESULT DAPI XmlRemoveChildren(
    __in IXMLDOMNode* pixnSource,
    __in_z LPCWSTR pwzXPath
    );
HRESULT DAPI XmlSaveDocument(
    __in IXMLDOMDocument* pixdDocument, 
    __inout LPCWSTR wzPath
    );
HRESULT DAPI XmlSaveDocumentToBuffer(
    __in IXMLDOMDocument* pixdDocument,
    __deref_out_bcount(*pcbDest) BYTE** ppbDest,
    __out DWORD* pcbDest
    );

#ifdef __cplusplus
}
#endif
