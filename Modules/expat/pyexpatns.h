/* Copyright (c) 2005-2006 ActiveState Software Inc.
 *
 * Namespace all expat exported symbols to avoid dynamic loading symbol
 * collisions when embedding Python.
 *
 * The Problem:
 * - you embed Python in some app
 * - the app dynamically loads libexpat of version X
 * - the embedded Python imports pyexpat (which was built against
 *   libexpat version X+n)
 * --> pyexpat gets the expat symbols from the already loaded and *older*
 *     libexpat: crash (Specifically the crash we observed was in
 *     getting an old XML_ErrorString (from xmlparse.c) and then calling
 *     it with newer values in the XML_Error enum:
 *
 *       // pyexpat.c, line 1970
 *       ...
 *       // Added in Expat 1.95.7.
 *       MYCONST(XML_ERROR_UNBOUND_PREFIX);
 *       ...
 *
 *
 * The Solution:
 * Prefix all a exported symbols with "PyExpat_". This is similar to
 * what Mozilla does for some common libs:
 * http://lxr.mozilla.org/seamonkey/source/modules/libimg/png/mozpngconf.h#115
 *
 * The list of relevant exported symbols can be had with this command:
 * 
       nm pyexpat.so \
           | grep -v " [a-zBUA] " \
           | grep -v "_fini\|_init\|initpyexpat"
 *
 * If any of those symbols are NOT prefixed with "PyExpat_" then
 * a #define should be added for it here.
 */

#ifndef PYEXPATNS_H
#define PYEXPATNS_H

#define XML_DefaultCurrent              PyExpat_XML_DefaultCurrent
#define XML_ErrorString                 PyExpat_XML_ErrorString
#define XML_ExpatVersion                PyExpat_XML_ExpatVersion
#define XML_ExpatVersionInfo            PyExpat_XML_ExpatVersionInfo
#define XML_ExternalEntityParserCreate  PyExpat_XML_ExternalEntityParserCreate
#define XML_FreeContentModel            PyExpat_XML_FreeContentModel
#define XML_GetBase                     PyExpat_XML_GetBase
#define XML_GetBuffer                   PyExpat_XML_GetBuffer
#define XML_GetCurrentByteCount         PyExpat_XML_GetCurrentByteCount
#define XML_GetCurrentByteIndex         PyExpat_XML_GetCurrentByteIndex
#define XML_GetCurrentColumnNumber      PyExpat_XML_GetCurrentColumnNumber
#define XML_GetCurrentLineNumber        PyExpat_XML_GetCurrentLineNumber
#define XML_GetErrorCode                PyExpat_XML_GetErrorCode
#define XML_GetFeatureList              PyExpat_XML_GetFeatureList
#define XML_GetIdAttributeIndex         PyExpat_XML_GetIdAttributeIndex
#define XML_GetInputContext             PyExpat_XML_GetInputContext
#define XML_GetParsingStatus            PyExpat_XML_GetParsingStatus
#define XML_GetSpecifiedAttributeCount  PyExpat_XML_GetSpecifiedAttributeCount
#define XmlGetUtf16InternalEncoding     PyExpat_XmlGetUtf16InternalEncoding
#define XmlGetUtf16InternalEncodingNS   PyExpat_XmlGetUtf16InternalEncodingNS
#define XmlGetUtf8InternalEncoding      PyExpat_XmlGetUtf8InternalEncoding
#define XmlGetUtf8InternalEncodingNS    PyExpat_XmlGetUtf8InternalEncodingNS
#define XmlInitEncoding                 PyExpat_XmlInitEncoding
#define XmlInitEncodingNS               PyExpat_XmlInitEncodingNS
#define XmlInitUnknownEncoding          PyExpat_XmlInitUnknownEncoding
#define XmlInitUnknownEncodingNS        PyExpat_XmlInitUnknownEncodingNS
#define XML_MemFree                     PyExpat_XML_MemFree
#define XML_MemMalloc                   PyExpat_XML_MemMalloc
#define XML_MemRealloc                  PyExpat_XML_MemRealloc
#define XML_Parse                       PyExpat_XML_Parse
#define XML_ParseBuffer                 PyExpat_XML_ParseBuffer
#define XML_ParserCreate                PyExpat_XML_ParserCreate
#define XML_ParserCreate_MM             PyExpat_XML_ParserCreate_MM
#define XML_ParserCreateNS              PyExpat_XML_ParserCreateNS
#define XML_ParserFree                  PyExpat_XML_ParserFree
#define XML_ParserReset                 PyExpat_XML_ParserReset
#define XmlParseXmlDecl                 PyExpat_XmlParseXmlDecl
#define XmlParseXmlDeclNS               PyExpat_XmlParseXmlDeclNS
#define XmlPrologStateInit              PyExpat_XmlPrologStateInit
#define XmlPrologStateInitExternalEntity    PyExpat_XmlPrologStateInitExternalEntity
#define XML_ResumeParser                PyExpat_XML_ResumeParser
#define XML_SetAttlistDeclHandler       PyExpat_XML_SetAttlistDeclHandler
#define XML_SetBase                     PyExpat_XML_SetBase
#define XML_SetCdataSectionHandler      PyExpat_XML_SetCdataSectionHandler
#define XML_SetCharacterDataHandler     PyExpat_XML_SetCharacterDataHandler
#define XML_SetCommentHandler           PyExpat_XML_SetCommentHandler
#define XML_SetDefaultHandler           PyExpat_XML_SetDefaultHandler
#define XML_SetDefaultHandlerExpand     PyExpat_XML_SetDefaultHandlerExpand
#define XML_SetDoctypeDeclHandler       PyExpat_XML_SetDoctypeDeclHandler
#define XML_SetElementDeclHandler       PyExpat_XML_SetElementDeclHandler
#define XML_SetElementHandler           PyExpat_XML_SetElementHandler
#define XML_SetEncoding                 PyExpat_XML_SetEncoding
#define XML_SetEndCdataSectionHandler   PyExpat_XML_SetEndCdataSectionHandler
#define XML_SetEndDoctypeDeclHandler    PyExpat_XML_SetEndDoctypeDeclHandler
#define XML_SetEndElementHandler        PyExpat_XML_SetEndElementHandler
#define XML_SetEndNamespaceDeclHandler  PyExpat_XML_SetEndNamespaceDeclHandler
#define XML_SetEntityDeclHandler        PyExpat_XML_SetEntityDeclHandler
#define XML_SetExternalEntityRefHandler PyExpat_XML_SetExternalEntityRefHandler
#define XML_SetExternalEntityRefHandlerArg  PyExpat_XML_SetExternalEntityRefHandlerArg
#define XML_SetHashSalt                 PyExpat_XML_SetHashSalt
#define XML_SetNamespaceDeclHandler     PyExpat_XML_SetNamespaceDeclHandler
#define XML_SetNotationDeclHandler      PyExpat_XML_SetNotationDeclHandler
#define XML_SetNotStandaloneHandler     PyExpat_XML_SetNotStandaloneHandler
#define XML_SetParamEntityParsing       PyExpat_XML_SetParamEntityParsing
#define XML_SetProcessingInstructionHandler PyExpat_XML_SetProcessingInstructionHandler
#define XML_SetReturnNSTriplet          PyExpat_XML_SetReturnNSTriplet
#define XML_SetSkippedEntityHandler     PyExpat_XML_SetSkippedEntityHandler
#define XML_SetStartCdataSectionHandler PyExpat_XML_SetStartCdataSectionHandler
#define XML_SetStartDoctypeDeclHandler  PyExpat_XML_SetStartDoctypeDeclHandler
#define XML_SetStartElementHandler      PyExpat_XML_SetStartElementHandler
#define XML_SetStartNamespaceDeclHandler    PyExpat_XML_SetStartNamespaceDeclHandler
#define XML_SetUnknownEncodingHandler   PyExpat_XML_SetUnknownEncodingHandler
#define XML_SetUnparsedEntityDeclHandler    PyExpat_XML_SetUnparsedEntityDeclHandler
#define XML_SetUserData                 PyExpat_XML_SetUserData
#define XML_SetXmlDeclHandler           PyExpat_XML_SetXmlDeclHandler
#define XmlSizeOfUnknownEncoding        PyExpat_XmlSizeOfUnknownEncoding
#define XML_StopParser                  PyExpat_XML_StopParser
#define XML_UseForeignDTD               PyExpat_XML_UseForeignDTD
#define XML_UseParserAsHandlerArg       PyExpat_XML_UseParserAsHandlerArg
#define XmlUtf16Encode                  PyExpat_XmlUtf16Encode
#define XmlUtf8Encode                   PyExpat_XmlUtf8Encode


#endif /* !PYEXPATNS_H */

