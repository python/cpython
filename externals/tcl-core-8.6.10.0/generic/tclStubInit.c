/*
 * tclStubInit.c --
 *
 *	This file contains the initializers for the Tcl stub vectors.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"
#include "tommath_private.h"

#ifdef __CYGWIN__
#   include <wchar.h>
#endif

#ifdef __GNUC__
#pragma GCC dependency "tcl.decls"
#pragma GCC dependency "tclInt.decls"
#pragma GCC dependency "tclTomMath.decls"
#endif

/*
 * Remove macros that will interfere with the definitions below.
 */

#undef Tcl_Alloc
#undef Tcl_Free
#undef Tcl_Realloc
#undef Tcl_NewBooleanObj
#undef Tcl_NewByteArrayObj
#undef Tcl_NewDoubleObj
#undef Tcl_NewIntObj
#undef Tcl_NewListObj
#undef Tcl_NewLongObj
#undef Tcl_NewObj
#undef Tcl_NewStringObj
#undef Tcl_DumpActiveMemory
#undef Tcl_ValidateAllMemory
#undef Tcl_FindHashEntry
#undef Tcl_CreateHashEntry
#undef Tcl_Panic
#undef Tcl_FindExecutable
#undef TclpGetPid
#undef TclSockMinimumBuffers
#define TclBackgroundException Tcl_BackgroundException
#undef Tcl_SetIntObj
#undef TclpInetNtoa
#undef TclWinGetServByName
#undef TclWinGetSockOpt
#undef TclWinSetSockOpt
#undef TclBN_mp_tc_and
#undef TclBN_mp_tc_or
#undef TclBN_mp_tc_xor
#define TclBN_mp_tc_and TclBN_mp_and
#define TclBN_mp_tc_or TclBN_mp_or
#define TclBN_mp_tc_xor TclBN_mp_xor
#define TclUnusedStubEntry NULL

/* See bug 510001: TclSockMinimumBuffers needs plat imp */
#ifdef _WIN64
#   define TclSockMinimumBuffersOld 0
#else
#define TclSockMinimumBuffersOld sockMinimumBuffersOld
static int TclSockMinimumBuffersOld(int sock, int size)
{
    return TclSockMinimumBuffers(INT2PTR(sock), size);
}
#endif

MP_SET_UNSIGNED(mp_set_ull, Tcl_WideUInt)


mp_err TclBN_mp_set_int(mp_int *a, unsigned long i)
{
	mp_set_ull(a, i);
	return MP_OKAY;
}

mp_err TclBN_mp_init_set_int(mp_int *a, unsigned long i)
{
    mp_err result = mp_init(a);
    if (result == MP_OKAY) {
	mp_set_ull(a, i);
    }
	return result;
}

int TclBN_mp_expt_d_ex(const mp_int *a, mp_digit b, mp_int *c, int fast)
{
	return mp_expt_u32(a, b, c);
}

#define TclSetStartupScriptPath setStartupScriptPath
static void TclSetStartupScriptPath(Tcl_Obj *path)
{
    Tcl_SetStartupScript(path, NULL);
}
#define TclGetStartupScriptPath getStartupScriptPath
static Tcl_Obj *TclGetStartupScriptPath(void)
{
    return Tcl_GetStartupScript(NULL);
}
#define TclSetStartupScriptFileName setStartupScriptFileName
static void TclSetStartupScriptFileName(
    const char *fileName)
{
    Tcl_SetStartupScript(Tcl_NewStringObj(fileName,-1), NULL);
}
#define TclGetStartupScriptFileName getStartupScriptFileName
static const char *TclGetStartupScriptFileName(void)
{
    Tcl_Obj *path = Tcl_GetStartupScript(NULL);
    if (path == NULL) {
	return NULL;
    }
    return Tcl_GetString(path);
}

#if defined(_WIN32) || defined(__CYGWIN__)
#undef TclWinNToHS
#define TclWinNToHS winNToHS
static unsigned short TclWinNToHS(unsigned short ns) {
	return ntohs(ns);
}
#endif

#ifdef _WIN32
#   define TclUnixWaitForFile 0
#   define TclUnixCopyFile 0
#   define TclUnixOpenTemporaryFile 0
#   define TclpReaddir 0
#   define TclpIsAtty 0
#elif defined(__CYGWIN__)
#   define TclpIsAtty TclPlatIsAtty
#   define TclWinSetInterfaces (void (*) (int)) doNothing
#   define TclWinAddProcess (void (*) (void *, unsigned int)) doNothing
#   define TclWinFlushDirtyChannels doNothing
#   define TclWinResetInterfaces doNothing

static int
TclpIsAtty(int fd)
{
    return isatty(fd);
}

#define TclWinGetPlatformId winGetPlatformId
static int
TclWinGetPlatformId()
{
    /* Don't bother to determine the real platform on cygwin,
     * because VER_PLATFORM_WIN32_NT is the only supported platform */
    return 2; /* VER_PLATFORM_WIN32_NT */;
}

void *TclWinGetTclInstance()
{
    void *hInstance = NULL;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
	    (const char *)&TclpIsAtty, &hInstance);
    return hInstance;
}

#define TclWinSetSockOpt winSetSockOpt
static int
TclWinSetSockOpt(SOCKET s, int level, int optname,
	    const char *optval, int optlen)
{
    return setsockopt((int) s, level, optname, optval, optlen);
}

#define TclWinGetSockOpt winGetSockOpt
static int
TclWinGetSockOpt(SOCKET s, int level, int optname,
	    char *optval, int *optlen)
{
    return getsockopt((int) s, level, optname, optval, optlen);
}

#define TclWinGetServByName winGetServByName
static struct servent *
TclWinGetServByName(const char *name, const char *proto)
{
    return getservbyname(name, proto);
}

#define TclWinNoBackslash winNoBackslash
static char *
TclWinNoBackslash(char *path)
{
    char *p;

    for (p = path; *p != '\0'; p++) {
	if (*p == '\\') {
	    *p = '/';
	}
    }
    return path;
}

int
TclpGetPid(Tcl_Pid pid)
{
    return (int) (size_t) pid;
}

static void
doNothing(void)
{
    /* dummy implementation, no need to do anything */
}

char *
Tcl_WinUtfToTChar(
    const char *string,
    int len,
    Tcl_DString *dsPtr)
{
#if TCL_UTF_MAX > 4
    Tcl_UniChar ch = 0;
    wchar_t *w, *wString;
    const char *p, *end;
    int oldLength;
#endif

    Tcl_DStringInit(dsPtr);
    if (!string) {
	return NULL;
    }
#if TCL_UTF_MAX > 4

    if (len < 0) {
	len = strlen(string);
    }

    /*
     * Unicode string length in Tcl_UniChars will be <= UTF-8 string length in
     * bytes.
     */

    oldLength = Tcl_DStringLength(dsPtr);

    Tcl_DStringSetLength(dsPtr,
	    oldLength + (int) ((len + 1) * sizeof(wchar_t)));
    wString = (wchar_t *) (Tcl_DStringValue(dsPtr) + oldLength);

    w = wString;
    p = string;
    end = string + len - 4;
    while (p < end) {
	p += TclUtfToUniChar(p, &ch);
	if (ch > 0xFFFF) {
	    *w++ = (wchar_t) (0xD800 + ((ch -= 0x10000) >> 10));
	    *w++ = (wchar_t) (0xDC00 | (ch & 0x3FF));
	} else {
	    *w++ = ch;
	}
    }
    end += 4;
    while (p < end) {
	if (Tcl_UtfCharComplete(p, end-p)) {
	    p += TclUtfToUniChar(p, &ch);
	} else {
	    ch = UCHAR(*p++);
	}
	if (ch > 0xFFFF) {
	    *w++ = (wchar_t) (0xD800 + ((ch -= 0x10000) >> 10));
	    *w++ = (wchar_t) (0xDC00 | (ch & 0x3FF));
	} else {
	    *w++ = ch;
	}
    }
    *w = '\0';
    Tcl_DStringSetLength(dsPtr,
	    oldLength + ((char *) w - (char *) wString));

    return (char *)wString;
#else
    return (char *)Tcl_UtfToUniCharDString(string, len, dsPtr);
#endif
}

char *
Tcl_WinTCharToUtf(
    const char *string,
    int len,
    Tcl_DString *dsPtr)
{
#if TCL_UTF_MAX > 4
    const wchar_t *w, *wEnd;
    char *p, *result;
    int oldLength, blen = 1;
#endif

    Tcl_DStringInit(dsPtr);
    if (!string) {
	return NULL;
    }
    if (len < 0) {
	len = wcslen((wchar_t *)string);
    } else {
	len /= 2;
    }
#if TCL_UTF_MAX > 4
    oldLength = Tcl_DStringLength(dsPtr);
    Tcl_DStringSetLength(dsPtr, oldLength + (len + 1) * 4);
    result = Tcl_DStringValue(dsPtr) + oldLength;

    p = result;
    wEnd = (wchar_t *)string + len;
    for (w = (wchar_t *)string; w < wEnd; ) {
	if (!blen && ((*w & 0xFC00) != 0xDC00)) {
	    /* Special case for handling high surrogates. */
	    p += Tcl_UniCharToUtf(-1, p);
	}
	blen = Tcl_UniCharToUtf(*w, p);
	p += blen;
	if ((*w >= 0xD800) && (blen < 3)) {
	    /* Indication that high surrogate is handled */
	    blen = 0;
	}
	w++;
    }
    if (!blen) {
	/* Special case for handling high surrogates. */
	p += Tcl_UniCharToUtf(-1, p);
    }
    Tcl_DStringSetLength(dsPtr, oldLength + (p - result));

    return result;
#else
    return Tcl_UniCharToUtfDString((Tcl_UniChar *)string, len, dsPtr);
#endif
}

#if defined(TCL_WIDE_INT_IS_LONG)
/* On Cygwin64, long is 64-bit while on Win64 long is 32-bit. Therefore
 * we have to make sure that all stub entries on Cygwin64 follow the Win64
 * signature. Tcl 9 must find a better solution, but that cannot be done
 * without introducing a binary incompatibility.
 */
#define Tcl_DbNewLongObj ((Tcl_Obj*(*)(long,const char*,int))dbNewLongObj)
static Tcl_Obj *dbNewLongObj(
    int intValue,
    const char *file,
    int line
) {
#ifdef TCL_MEM_DEBUG
    register Tcl_Obj *objPtr;

    TclDbNewObj(objPtr, file, line);
    objPtr->bytes = NULL;

    objPtr->internalRep.longValue = (long) intValue;
    objPtr->typePtr = &tclIntType;
    return objPtr;
#else
    return Tcl_NewIntObj(intValue);
#endif
}
#define Tcl_GetLongFromObj (int(*)(Tcl_Interp*,Tcl_Obj*,long*))Tcl_GetIntFromObj
#define Tcl_NewLongObj (Tcl_Obj*(*)(long))Tcl_NewIntObj
#define Tcl_SetLongObj (void(*)(Tcl_Obj*,long))Tcl_SetIntObj
static int exprInt(Tcl_Interp *interp, const char *expr, int *ptr){
    long longValue;
    int result = Tcl_ExprLong(interp, expr, &longValue);
    if (result == TCL_OK) {
	    if ((longValue >= -(long)(UINT_MAX))
		    && (longValue <= (long)(UINT_MAX))) {
	    *ptr = (int)longValue;
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "integer value too large to represent as non-long integer", -1));
	    result = TCL_ERROR;
	}
    }
    return result;
}
#define Tcl_ExprLong (int(*)(Tcl_Interp*,const char*,long*))exprInt
static int exprIntObj(Tcl_Interp *interp, Tcl_Obj*expr, int *ptr){
    long longValue;
    int result = Tcl_ExprLongObj(interp, expr, &longValue);
    if (result == TCL_OK) {
	    if ((longValue >= -(long)(UINT_MAX))
		    && (longValue <= (long)(UINT_MAX))) {
	    *ptr = (int)longValue;
	} else {
	    Tcl_SetObjResult(interp, Tcl_NewStringObj(
		    "integer value too large to represent as non-long integer", -1));
	    result = TCL_ERROR;
	}
    }
    return result;
}
#define Tcl_ExprLongObj (int(*)(Tcl_Interp*,Tcl_Obj*,long*))exprIntObj
static int uniCharNcmp(const Tcl_UniChar *ucs, const Tcl_UniChar *uct, unsigned int n){
   return Tcl_UniCharNcmp(ucs, uct, (unsigned long)n);
}
#define Tcl_UniCharNcmp (int(*)(const Tcl_UniChar*,const Tcl_UniChar*,unsigned long))uniCharNcmp
static int utfNcmp(const char *s1, const char *s2, unsigned int n){
   return Tcl_UtfNcmp(s1, s2, (unsigned long)n);
}
#define Tcl_UtfNcmp (int(*)(const char*,const char*,unsigned long))utfNcmp
static int utfNcasecmp(const char *s1, const char *s2, unsigned int n){
   return Tcl_UtfNcasecmp(s1, s2, (unsigned long)n);
}
#define Tcl_UtfNcasecmp (int(*)(const char*,const char*,unsigned long))utfNcasecmp
static int uniCharNcasecmp(const Tcl_UniChar *ucs, const Tcl_UniChar *uct, unsigned int n){
   return Tcl_UniCharNcasecmp(ucs, uct, (unsigned long)n);
}
#define Tcl_UniCharNcasecmp (int(*)(const Tcl_UniChar*,const Tcl_UniChar*,unsigned long))uniCharNcasecmp
static int formatInt(char *buffer, int n){
   return TclFormatInt(buffer, (long)n);
}
#define TclFormatInt (int(*)(char *, long))formatInt

#endif

#else /* UNIX and MAC */
#   define TclpLocaltime_unix TclpLocaltime
#   define TclpGmtime_unix TclpGmtime
#endif

mp_err mp_to_unsigned_bin(const mp_int *a, unsigned char *b)
{
   return mp_to_ubin(a, b, INT_MAX, NULL);
}

mp_err mp_to_unsigned_bin_n(const mp_int *a, unsigned char *b, unsigned long *outlen)
{
   size_t n = mp_ubin_size(a);
   if (*outlen < (unsigned long)n) {
      return MP_VAL;
   }
   *outlen = (unsigned long)n;
   return mp_to_ubin(a, b, n, NULL);
}

mp_err mp_toradix_n(const mp_int *a, char *str, int radix, int maxlen)
{
   if (maxlen < 0) {
      return MP_VAL;
   }
   return mp_to_radix(a, str, (size_t)maxlen, NULL, radix);
}

void bn_reverse(unsigned char *s, int len)
{
   if (len > 0) {
      s_mp_reverse(s, (size_t)len);
   }
}

/*
 * WARNING: The contents of this file is automatically generated by the
 * tools/genStubs.tcl script. Any modifications to the function declarations
 * below should be made in the generic/tcl.decls script.
 */

MODULE_SCOPE const TclStubs tclStubs;
MODULE_SCOPE const TclTomMathStubs tclTomMathStubs;

/* !BEGIN!: Do not edit below this line. */

static const TclIntStubs tclIntStubs = {
    TCL_STUB_MAGIC,
    0,
    0, /* 0 */
    0, /* 1 */
    0, /* 2 */
    TclAllocateFreeObjects, /* 3 */
    0, /* 4 */
    TclCleanupChildren, /* 5 */
    TclCleanupCommand, /* 6 */
    TclCopyAndCollapse, /* 7 */
    TclCopyChannelOld, /* 8 */
    TclCreatePipeline, /* 9 */
    TclCreateProc, /* 10 */
    TclDeleteCompiledLocalVars, /* 11 */
    TclDeleteVars, /* 12 */
    0, /* 13 */
    TclDumpMemoryInfo, /* 14 */
    0, /* 15 */
    TclExprFloatError, /* 16 */
    0, /* 17 */
    0, /* 18 */
    0, /* 19 */
    0, /* 20 */
    0, /* 21 */
    TclFindElement, /* 22 */
    TclFindProc, /* 23 */
    TclFormatInt, /* 24 */
    TclFreePackageInfo, /* 25 */
    0, /* 26 */
    0, /* 27 */
    TclpGetDefaultStdChannel, /* 28 */
    0, /* 29 */
    0, /* 30 */
    TclGetExtension, /* 31 */
    TclGetFrame, /* 32 */
    0, /* 33 */
    TclGetIntForIndex, /* 34 */
    0, /* 35 */
    0, /* 36 */
    TclGetLoadedPackages, /* 37 */
    TclGetNamespaceForQualName, /* 38 */
    TclGetObjInterpProc, /* 39 */
    TclGetOpenMode, /* 40 */
    TclGetOriginalCommand, /* 41 */
    TclpGetUserHome, /* 42 */
    0, /* 43 */
    TclGuessPackageName, /* 44 */
    TclHideUnsafeCommands, /* 45 */
    TclInExit, /* 46 */
    0, /* 47 */
    0, /* 48 */
    0, /* 49 */
    TclInitCompiledLocals, /* 50 */
    TclInterpInit, /* 51 */
    0, /* 52 */
    TclInvokeObjectCommand, /* 53 */
    TclInvokeStringCommand, /* 54 */
    TclIsProc, /* 55 */
    0, /* 56 */
    0, /* 57 */
    TclLookupVar, /* 58 */
    0, /* 59 */
    TclNeedSpace, /* 60 */
    TclNewProcBodyObj, /* 61 */
    TclObjCommandComplete, /* 62 */
    TclObjInterpProc, /* 63 */
    TclObjInvoke, /* 64 */
    0, /* 65 */
    0, /* 66 */
    0, /* 67 */
    0, /* 68 */
    TclpAlloc, /* 69 */
    0, /* 70 */
    0, /* 71 */
    0, /* 72 */
    0, /* 73 */
    TclpFree, /* 74 */
    TclpGetClicks, /* 75 */
    TclpGetSeconds, /* 76 */
    TclpGetTime, /* 77 */
    0, /* 78 */
    0, /* 79 */
    0, /* 80 */
    TclpRealloc, /* 81 */
    0, /* 82 */
    0, /* 83 */
    0, /* 84 */
    0, /* 85 */
    0, /* 86 */
    0, /* 87 */
    TclPrecTraceProc, /* 88 */
    TclPreventAliasLoop, /* 89 */
    0, /* 90 */
    TclProcCleanupProc, /* 91 */
    TclProcCompileProc, /* 92 */
    TclProcDeleteProc, /* 93 */
    0, /* 94 */
    0, /* 95 */
    TclRenameCommand, /* 96 */
    TclResetShadowedCmdRefs, /* 97 */
    TclServiceIdle, /* 98 */
    0, /* 99 */
    0, /* 100 */
    TclSetPreInitScript, /* 101 */
    TclSetupEnv, /* 102 */
    TclSockGetPort, /* 103 */
    TclSockMinimumBuffersOld, /* 104 */
    0, /* 105 */
    0, /* 106 */
    0, /* 107 */
    TclTeardownNamespace, /* 108 */
    TclUpdateReturnInfo, /* 109 */
    TclSockMinimumBuffers, /* 110 */
    Tcl_AddInterpResolvers, /* 111 */
    Tcl_AppendExportList, /* 112 */
    Tcl_CreateNamespace, /* 113 */
    Tcl_DeleteNamespace, /* 114 */
    Tcl_Export, /* 115 */
    Tcl_FindCommand, /* 116 */
    Tcl_FindNamespace, /* 117 */
    Tcl_GetInterpResolvers, /* 118 */
    Tcl_GetNamespaceResolvers, /* 119 */
    Tcl_FindNamespaceVar, /* 120 */
    Tcl_ForgetImport, /* 121 */
    Tcl_GetCommandFromObj, /* 122 */
    Tcl_GetCommandFullName, /* 123 */
    Tcl_GetCurrentNamespace, /* 124 */
    Tcl_GetGlobalNamespace, /* 125 */
    Tcl_GetVariableFullName, /* 126 */
    Tcl_Import, /* 127 */
    Tcl_PopCallFrame, /* 128 */
    Tcl_PushCallFrame, /* 129 */
    Tcl_RemoveInterpResolvers, /* 130 */
    Tcl_SetNamespaceResolvers, /* 131 */
    TclpHasSockets, /* 132 */
    TclpGetDate, /* 133 */
    0, /* 134 */
    0, /* 135 */
    0, /* 136 */
    0, /* 137 */
    TclGetEnv, /* 138 */
    0, /* 139 */
    0, /* 140 */
    TclpGetCwd, /* 141 */
    TclSetByteCodeFromAny, /* 142 */
    TclAddLiteralObj, /* 143 */
    TclHideLiteral, /* 144 */
    TclGetAuxDataType, /* 145 */
    TclHandleCreate, /* 146 */
    TclHandleFree, /* 147 */
    TclHandlePreserve, /* 148 */
    TclHandleRelease, /* 149 */
    TclRegAbout, /* 150 */
    TclRegExpRangeUniChar, /* 151 */
    TclSetLibraryPath, /* 152 */
    TclGetLibraryPath, /* 153 */
    0, /* 154 */
    0, /* 155 */
    TclRegError, /* 156 */
    TclVarTraceExists, /* 157 */
    TclSetStartupScriptFileName, /* 158 */
    TclGetStartupScriptFileName, /* 159 */
    0, /* 160 */
    TclChannelTransform, /* 161 */
    TclChannelEventScriptInvoker, /* 162 */
    TclGetInstructionTable, /* 163 */
    TclExpandCodeArray, /* 164 */
    TclpSetInitialEncodings, /* 165 */
    TclListObjSetElement, /* 166 */
    TclSetStartupScriptPath, /* 167 */
    TclGetStartupScriptPath, /* 168 */
    TclpUtfNcmp2, /* 169 */
    TclCheckInterpTraces, /* 170 */
    TclCheckExecutionTraces, /* 171 */
    TclInThreadExit, /* 172 */
    TclUniCharMatch, /* 173 */
    0, /* 174 */
    TclCallVarTraces, /* 175 */
    TclCleanupVar, /* 176 */
    TclVarErrMsg, /* 177 */
    Tcl_SetStartupScript, /* 178 */
    Tcl_GetStartupScript, /* 179 */
    0, /* 180 */
    0, /* 181 */
    TclpLocaltime, /* 182 */
    TclpGmtime, /* 183 */
    0, /* 184 */
    0, /* 185 */
    0, /* 186 */
    0, /* 187 */
    0, /* 188 */
    0, /* 189 */
    0, /* 190 */
    0, /* 191 */
    0, /* 192 */
    0, /* 193 */
    0, /* 194 */
    0, /* 195 */
    0, /* 196 */
    0, /* 197 */
    TclObjGetFrame, /* 198 */
    0, /* 199 */
    TclpObjRemoveDirectory, /* 200 */
    TclpObjCopyDirectory, /* 201 */
    TclpObjCreateDirectory, /* 202 */
    TclpObjDeleteFile, /* 203 */
    TclpObjCopyFile, /* 204 */
    TclpObjRenameFile, /* 205 */
    TclpObjStat, /* 206 */
    TclpObjAccess, /* 207 */
    TclpOpenFileChannel, /* 208 */
    0, /* 209 */
    0, /* 210 */
    0, /* 211 */
    TclpFindExecutable, /* 212 */
    TclGetObjNameOfExecutable, /* 213 */
    TclSetObjNameOfExecutable, /* 214 */
    TclStackAlloc, /* 215 */
    TclStackFree, /* 216 */
    TclPushStackFrame, /* 217 */
    TclPopStackFrame, /* 218 */
    0, /* 219 */
    0, /* 220 */
    0, /* 221 */
    0, /* 222 */
    0, /* 223 */
    TclGetPlatform, /* 224 */
    TclTraceDictPath, /* 225 */
    TclObjBeingDeleted, /* 226 */
    TclSetNsPath, /* 227 */
    0, /* 228 */
    TclPtrMakeUpvar, /* 229 */
    TclObjLookupVar, /* 230 */
    TclGetNamespaceFromObj, /* 231 */
    TclEvalObjEx, /* 232 */
    TclGetSrcInfoForPc, /* 233 */
    TclVarHashCreateVar, /* 234 */
    TclInitVarHashTable, /* 235 */
    TclBackgroundException, /* 236 */
    TclResetCancellation, /* 237 */
    TclNRInterpProc, /* 238 */
    TclNRInterpProcCore, /* 239 */
    TclNRRunCallbacks, /* 240 */
    TclNREvalObjEx, /* 241 */
    TclNREvalObjv, /* 242 */
    TclDbDumpActiveObjects, /* 243 */
    TclGetNamespaceChildTable, /* 244 */
    TclGetNamespaceCommandTable, /* 245 */
    TclInitRewriteEnsemble, /* 246 */
    TclResetRewriteEnsemble, /* 247 */
    TclCopyChannel, /* 248 */
    TclDoubleDigits, /* 249 */
    TclSetSlaveCancelFlags, /* 250 */
    TclRegisterLiteral, /* 251 */
    TclPtrGetVar, /* 252 */
    TclPtrSetVar, /* 253 */
    TclPtrIncrObjVar, /* 254 */
    TclPtrObjMakeUpvar, /* 255 */
    TclPtrUnsetVar, /* 256 */
    0, /* 257 */
    TclUnusedStubEntry, /* 258 */
};

static const TclIntPlatStubs tclIntPlatStubs = {
    TCL_STUB_MAGIC,
    0,
#if !defined(_WIN32) && !defined(__CYGWIN__) && !defined(MAC_OSX_TCL) /* UNIX */
    TclGetAndDetachPids, /* 0 */
    TclpCloseFile, /* 1 */
    TclpCreateCommandChannel, /* 2 */
    TclpCreatePipe, /* 3 */
    TclpCreateProcess, /* 4 */
    0, /* 5 */
    TclpMakeFile, /* 6 */
    TclpOpenFile, /* 7 */
    TclUnixWaitForFile, /* 8 */
    TclpCreateTempFile, /* 9 */
    TclpReaddir, /* 10 */
    TclpLocaltime_unix, /* 11 */
    TclpGmtime_unix, /* 12 */
    TclpInetNtoa, /* 13 */
    TclUnixCopyFile, /* 14 */
    0, /* 15 */
    0, /* 16 */
    0, /* 17 */
    0, /* 18 */
    0, /* 19 */
    0, /* 20 */
    0, /* 21 */
    0, /* 22 */
    0, /* 23 */
    0, /* 24 */
    0, /* 25 */
    0, /* 26 */
    0, /* 27 */
    0, /* 28 */
    TclWinCPUID, /* 29 */
    TclUnixOpenTemporaryFile, /* 30 */
#endif /* UNIX */
#if defined(_WIN32) || defined(__CYGWIN__) /* WIN */
    TclWinConvertError, /* 0 */
    TclWinConvertWSAError, /* 1 */
    TclWinGetServByName, /* 2 */
    TclWinGetSockOpt, /* 3 */
    TclWinGetTclInstance, /* 4 */
    TclUnixWaitForFile, /* 5 */
    TclWinNToHS, /* 6 */
    TclWinSetSockOpt, /* 7 */
    TclpGetPid, /* 8 */
    TclWinGetPlatformId, /* 9 */
    TclpReaddir, /* 10 */
    TclGetAndDetachPids, /* 11 */
    TclpCloseFile, /* 12 */
    TclpCreateCommandChannel, /* 13 */
    TclpCreatePipe, /* 14 */
    TclpCreateProcess, /* 15 */
    TclpIsAtty, /* 16 */
    TclUnixCopyFile, /* 17 */
    TclpMakeFile, /* 18 */
    TclpOpenFile, /* 19 */
    TclWinAddProcess, /* 20 */
    TclpInetNtoa, /* 21 */
    TclpCreateTempFile, /* 22 */
    0, /* 23 */
    TclWinNoBackslash, /* 24 */
    0, /* 25 */
    TclWinSetInterfaces, /* 26 */
    TclWinFlushDirtyChannels, /* 27 */
    TclWinResetInterfaces, /* 28 */
    TclWinCPUID, /* 29 */
    TclUnixOpenTemporaryFile, /* 30 */
#endif /* WIN */
#ifdef MAC_OSX_TCL /* MACOSX */
    TclGetAndDetachPids, /* 0 */
    TclpCloseFile, /* 1 */
    TclpCreateCommandChannel, /* 2 */
    TclpCreatePipe, /* 3 */
    TclpCreateProcess, /* 4 */
    0, /* 5 */
    TclpMakeFile, /* 6 */
    TclpOpenFile, /* 7 */
    TclUnixWaitForFile, /* 8 */
    TclpCreateTempFile, /* 9 */
    TclpReaddir, /* 10 */
    TclpLocaltime_unix, /* 11 */
    TclpGmtime_unix, /* 12 */
    TclpInetNtoa, /* 13 */
    TclUnixCopyFile, /* 14 */
    TclMacOSXGetFileAttribute, /* 15 */
    TclMacOSXSetFileAttribute, /* 16 */
    TclMacOSXCopyFileAttributes, /* 17 */
    TclMacOSXMatchType, /* 18 */
    TclMacOSXNotifierAddRunLoopMode, /* 19 */
    0, /* 20 */
    0, /* 21 */
    0, /* 22 */
    0, /* 23 */
    0, /* 24 */
    0, /* 25 */
    0, /* 26 */
    0, /* 27 */
    0, /* 28 */
    TclWinCPUID, /* 29 */
    TclUnixOpenTemporaryFile, /* 30 */
#endif /* MACOSX */
};

static const TclPlatStubs tclPlatStubs = {
    TCL_STUB_MAGIC,
    0,
#if defined(_WIN32) || defined(__CYGWIN__) /* WIN */
    Tcl_WinUtfToTChar, /* 0 */
    Tcl_WinTCharToUtf, /* 1 */
#endif /* WIN */
#ifdef MAC_OSX_TCL /* MACOSX */
    Tcl_MacOSXOpenBundleResources, /* 0 */
    Tcl_MacOSXOpenVersionedBundleResources, /* 1 */
#endif /* MACOSX */
};

const TclTomMathStubs tclTomMathStubs = {
    TCL_STUB_MAGIC,
    0,
    TclBN_epoch, /* 0 */
    TclBN_revision, /* 1 */
    TclBN_mp_add, /* 2 */
    TclBN_mp_add_d, /* 3 */
    TclBN_mp_and, /* 4 */
    TclBN_mp_clamp, /* 5 */
    TclBN_mp_clear, /* 6 */
    TclBN_mp_clear_multi, /* 7 */
    TclBN_mp_cmp, /* 8 */
    TclBN_mp_cmp_d, /* 9 */
    TclBN_mp_cmp_mag, /* 10 */
    TclBN_mp_copy, /* 11 */
    TclBN_mp_count_bits, /* 12 */
    TclBN_mp_div, /* 13 */
    TclBN_mp_div_d, /* 14 */
    TclBN_mp_div_2, /* 15 */
    TclBN_mp_div_2d, /* 16 */
    TclBN_mp_div_3, /* 17 */
    TclBN_mp_exch, /* 18 */
    TclBN_mp_expt_d, /* 19 */
    TclBN_mp_grow, /* 20 */
    TclBN_mp_init, /* 21 */
    TclBN_mp_init_copy, /* 22 */
    TclBN_mp_init_multi, /* 23 */
    TclBN_mp_init_set, /* 24 */
    TclBN_mp_init_size, /* 25 */
    TclBN_mp_lshd, /* 26 */
    TclBN_mp_mod, /* 27 */
    TclBN_mp_mod_2d, /* 28 */
    TclBN_mp_mul, /* 29 */
    TclBN_mp_mul_d, /* 30 */
    TclBN_mp_mul_2, /* 31 */
    TclBN_mp_mul_2d, /* 32 */
    TclBN_mp_neg, /* 33 */
    TclBN_mp_or, /* 34 */
    TclBN_mp_radix_size, /* 35 */
    TclBN_mp_read_radix, /* 36 */
    TclBN_mp_rshd, /* 37 */
    TclBN_mp_shrink, /* 38 */
    TclBN_mp_set, /* 39 */
    TclBN_mp_sqr, /* 40 */
    TclBN_mp_sqrt, /* 41 */
    TclBN_mp_sub, /* 42 */
    TclBN_mp_sub_d, /* 43 */
    TclBN_mp_to_unsigned_bin, /* 44 */
    TclBN_mp_to_unsigned_bin_n, /* 45 */
    TclBN_mp_toradix_n, /* 46 */
    TclBN_mp_unsigned_bin_size, /* 47 */
    TclBN_mp_xor, /* 48 */
    TclBN_mp_zero, /* 49 */
    TclBN_reverse, /* 50 */
    TclBN_fast_s_mp_mul_digs, /* 51 */
    TclBN_fast_s_mp_sqr, /* 52 */
    TclBN_mp_karatsuba_mul, /* 53 */
    TclBN_mp_karatsuba_sqr, /* 54 */
    TclBN_mp_toom_mul, /* 55 */
    TclBN_mp_toom_sqr, /* 56 */
    TclBN_s_mp_add, /* 57 */
    TclBN_s_mp_mul_digs, /* 58 */
    TclBN_s_mp_sqr, /* 59 */
    TclBN_s_mp_sub, /* 60 */
    TclBN_mp_init_set_int, /* 61 */
    TclBN_mp_set_int, /* 62 */
    TclBN_mp_cnt_lsb, /* 63 */
    TclBNInitBignumFromLong, /* 64 */
    TclBNInitBignumFromWideInt, /* 65 */
    TclBNInitBignumFromWideUInt, /* 66 */
    TclBN_mp_expt_d_ex, /* 67 */
    TclBN_mp_set_ull, /* 68 */
    0, /* 69 */
    0, /* 70 */
    0, /* 71 */
    0, /* 72 */
    TclBN_mp_tc_and, /* 73 */
    TclBN_mp_tc_or, /* 74 */
    TclBN_mp_tc_xor, /* 75 */
    TclBN_mp_signed_rsh, /* 76 */
    0, /* 77 */
    TclBN_mp_to_ubin, /* 78 */
    0, /* 79 */
    TclBN_mp_to_radix, /* 80 */
};

static const TclStubHooks tclStubHooks = {
    &tclPlatStubs,
    &tclIntStubs,
    &tclIntPlatStubs
};

const TclStubs tclStubs = {
    TCL_STUB_MAGIC,
    &tclStubHooks,
    Tcl_PkgProvideEx, /* 0 */
    Tcl_PkgRequireEx, /* 1 */
    Tcl_Panic, /* 2 */
    Tcl_Alloc, /* 3 */
    Tcl_Free, /* 4 */
    Tcl_Realloc, /* 5 */
    Tcl_DbCkalloc, /* 6 */
    Tcl_DbCkfree, /* 7 */
    Tcl_DbCkrealloc, /* 8 */
#if !defined(_WIN32) && !defined(MAC_OSX_TCL) /* UNIX */
    Tcl_CreateFileHandler, /* 9 */
#endif /* UNIX */
#if defined(_WIN32) /* WIN */
    0, /* 9 */
#endif /* WIN */
#ifdef MAC_OSX_TCL /* MACOSX */
    Tcl_CreateFileHandler, /* 9 */
#endif /* MACOSX */
#if !defined(_WIN32) && !defined(MAC_OSX_TCL) /* UNIX */
    Tcl_DeleteFileHandler, /* 10 */
#endif /* UNIX */
#if defined(_WIN32) /* WIN */
    0, /* 10 */
#endif /* WIN */
#ifdef MAC_OSX_TCL /* MACOSX */
    Tcl_DeleteFileHandler, /* 10 */
#endif /* MACOSX */
    Tcl_SetTimer, /* 11 */
    Tcl_Sleep, /* 12 */
    Tcl_WaitForEvent, /* 13 */
    Tcl_AppendAllObjTypes, /* 14 */
    Tcl_AppendStringsToObj, /* 15 */
    Tcl_AppendToObj, /* 16 */
    Tcl_ConcatObj, /* 17 */
    Tcl_ConvertToType, /* 18 */
    Tcl_DbDecrRefCount, /* 19 */
    Tcl_DbIncrRefCount, /* 20 */
    Tcl_DbIsShared, /* 21 */
    Tcl_DbNewBooleanObj, /* 22 */
    Tcl_DbNewByteArrayObj, /* 23 */
    Tcl_DbNewDoubleObj, /* 24 */
    Tcl_DbNewListObj, /* 25 */
    Tcl_DbNewLongObj, /* 26 */
    Tcl_DbNewObj, /* 27 */
    Tcl_DbNewStringObj, /* 28 */
    Tcl_DuplicateObj, /* 29 */
    TclFreeObj, /* 30 */
    Tcl_GetBoolean, /* 31 */
    Tcl_GetBooleanFromObj, /* 32 */
    Tcl_GetByteArrayFromObj, /* 33 */
    Tcl_GetDouble, /* 34 */
    Tcl_GetDoubleFromObj, /* 35 */
    Tcl_GetIndexFromObj, /* 36 */
    Tcl_GetInt, /* 37 */
    Tcl_GetIntFromObj, /* 38 */
    Tcl_GetLongFromObj, /* 39 */
    Tcl_GetObjType, /* 40 */
    Tcl_GetStringFromObj, /* 41 */
    Tcl_InvalidateStringRep, /* 42 */
    Tcl_ListObjAppendList, /* 43 */
    Tcl_ListObjAppendElement, /* 44 */
    Tcl_ListObjGetElements, /* 45 */
    Tcl_ListObjIndex, /* 46 */
    Tcl_ListObjLength, /* 47 */
    Tcl_ListObjReplace, /* 48 */
    Tcl_NewBooleanObj, /* 49 */
    Tcl_NewByteArrayObj, /* 50 */
    Tcl_NewDoubleObj, /* 51 */
    Tcl_NewIntObj, /* 52 */
    Tcl_NewListObj, /* 53 */
    Tcl_NewLongObj, /* 54 */
    Tcl_NewObj, /* 55 */
    Tcl_NewStringObj, /* 56 */
    Tcl_SetBooleanObj, /* 57 */
    Tcl_SetByteArrayLength, /* 58 */
    Tcl_SetByteArrayObj, /* 59 */
    Tcl_SetDoubleObj, /* 60 */
    Tcl_SetIntObj, /* 61 */
    Tcl_SetListObj, /* 62 */
    Tcl_SetLongObj, /* 63 */
    Tcl_SetObjLength, /* 64 */
    Tcl_SetStringObj, /* 65 */
    Tcl_AddErrorInfo, /* 66 */
    Tcl_AddObjErrorInfo, /* 67 */
    Tcl_AllowExceptions, /* 68 */
    Tcl_AppendElement, /* 69 */
    Tcl_AppendResult, /* 70 */
    Tcl_AsyncCreate, /* 71 */
    Tcl_AsyncDelete, /* 72 */
    Tcl_AsyncInvoke, /* 73 */
    Tcl_AsyncMark, /* 74 */
    Tcl_AsyncReady, /* 75 */
    Tcl_BackgroundError, /* 76 */
    Tcl_Backslash, /* 77 */
    Tcl_BadChannelOption, /* 78 */
    Tcl_CallWhenDeleted, /* 79 */
    Tcl_CancelIdleCall, /* 80 */
    Tcl_Close, /* 81 */
    Tcl_CommandComplete, /* 82 */
    Tcl_Concat, /* 83 */
    Tcl_ConvertElement, /* 84 */
    Tcl_ConvertCountedElement, /* 85 */
    Tcl_CreateAlias, /* 86 */
    Tcl_CreateAliasObj, /* 87 */
    Tcl_CreateChannel, /* 88 */
    Tcl_CreateChannelHandler, /* 89 */
    Tcl_CreateCloseHandler, /* 90 */
    Tcl_CreateCommand, /* 91 */
    Tcl_CreateEventSource, /* 92 */
    Tcl_CreateExitHandler, /* 93 */
    Tcl_CreateInterp, /* 94 */
    Tcl_CreateMathFunc, /* 95 */
    Tcl_CreateObjCommand, /* 96 */
    Tcl_CreateSlave, /* 97 */
    Tcl_CreateTimerHandler, /* 98 */
    Tcl_CreateTrace, /* 99 */
    Tcl_DeleteAssocData, /* 100 */
    Tcl_DeleteChannelHandler, /* 101 */
    Tcl_DeleteCloseHandler, /* 102 */
    Tcl_DeleteCommand, /* 103 */
    Tcl_DeleteCommandFromToken, /* 104 */
    Tcl_DeleteEvents, /* 105 */
    Tcl_DeleteEventSource, /* 106 */
    Tcl_DeleteExitHandler, /* 107 */
    Tcl_DeleteHashEntry, /* 108 */
    Tcl_DeleteHashTable, /* 109 */
    Tcl_DeleteInterp, /* 110 */
    Tcl_DetachPids, /* 111 */
    Tcl_DeleteTimerHandler, /* 112 */
    Tcl_DeleteTrace, /* 113 */
    Tcl_DontCallWhenDeleted, /* 114 */
    Tcl_DoOneEvent, /* 115 */
    Tcl_DoWhenIdle, /* 116 */
    Tcl_DStringAppend, /* 117 */
    Tcl_DStringAppendElement, /* 118 */
    Tcl_DStringEndSublist, /* 119 */
    Tcl_DStringFree, /* 120 */
    Tcl_DStringGetResult, /* 121 */
    Tcl_DStringInit, /* 122 */
    Tcl_DStringResult, /* 123 */
    Tcl_DStringSetLength, /* 124 */
    Tcl_DStringStartSublist, /* 125 */
    Tcl_Eof, /* 126 */
    Tcl_ErrnoId, /* 127 */
    Tcl_ErrnoMsg, /* 128 */
    Tcl_Eval, /* 129 */
    Tcl_EvalFile, /* 130 */
    Tcl_EvalObj, /* 131 */
    Tcl_EventuallyFree, /* 132 */
    Tcl_Exit, /* 133 */
    Tcl_ExposeCommand, /* 134 */
    Tcl_ExprBoolean, /* 135 */
    Tcl_ExprBooleanObj, /* 136 */
    Tcl_ExprDouble, /* 137 */
    Tcl_ExprDoubleObj, /* 138 */
    Tcl_ExprLong, /* 139 */
    Tcl_ExprLongObj, /* 140 */
    Tcl_ExprObj, /* 141 */
    Tcl_ExprString, /* 142 */
    Tcl_Finalize, /* 143 */
    Tcl_FindExecutable, /* 144 */
    Tcl_FirstHashEntry, /* 145 */
    Tcl_Flush, /* 146 */
    Tcl_FreeResult, /* 147 */
    Tcl_GetAlias, /* 148 */
    Tcl_GetAliasObj, /* 149 */
    Tcl_GetAssocData, /* 150 */
    Tcl_GetChannel, /* 151 */
    Tcl_GetChannelBufferSize, /* 152 */
    Tcl_GetChannelHandle, /* 153 */
    Tcl_GetChannelInstanceData, /* 154 */
    Tcl_GetChannelMode, /* 155 */
    Tcl_GetChannelName, /* 156 */
    Tcl_GetChannelOption, /* 157 */
    Tcl_GetChannelType, /* 158 */
    Tcl_GetCommandInfo, /* 159 */
    Tcl_GetCommandName, /* 160 */
    Tcl_GetErrno, /* 161 */
    Tcl_GetHostName, /* 162 */
    Tcl_GetInterpPath, /* 163 */
    Tcl_GetMaster, /* 164 */
    Tcl_GetNameOfExecutable, /* 165 */
    Tcl_GetObjResult, /* 166 */
#if !defined(_WIN32) && !defined(MAC_OSX_TCL) /* UNIX */
    Tcl_GetOpenFile, /* 167 */
#endif /* UNIX */
#if defined(_WIN32) /* WIN */
    0, /* 167 */
#endif /* WIN */
#ifdef MAC_OSX_TCL /* MACOSX */
    Tcl_GetOpenFile, /* 167 */
#endif /* MACOSX */
    Tcl_GetPathType, /* 168 */
    Tcl_Gets, /* 169 */
    Tcl_GetsObj, /* 170 */
    Tcl_GetServiceMode, /* 171 */
    Tcl_GetSlave, /* 172 */
    Tcl_GetStdChannel, /* 173 */
    Tcl_GetStringResult, /* 174 */
    Tcl_GetVar, /* 175 */
    Tcl_GetVar2, /* 176 */
    Tcl_GlobalEval, /* 177 */
    Tcl_GlobalEvalObj, /* 178 */
    Tcl_HideCommand, /* 179 */
    Tcl_Init, /* 180 */
    Tcl_InitHashTable, /* 181 */
    Tcl_InputBlocked, /* 182 */
    Tcl_InputBuffered, /* 183 */
    Tcl_InterpDeleted, /* 184 */
    Tcl_IsSafe, /* 185 */
    Tcl_JoinPath, /* 186 */
    Tcl_LinkVar, /* 187 */
    0, /* 188 */
    Tcl_MakeFileChannel, /* 189 */
    Tcl_MakeSafe, /* 190 */
    Tcl_MakeTcpClientChannel, /* 191 */
    Tcl_Merge, /* 192 */
    Tcl_NextHashEntry, /* 193 */
    Tcl_NotifyChannel, /* 194 */
    Tcl_ObjGetVar2, /* 195 */
    Tcl_ObjSetVar2, /* 196 */
    Tcl_OpenCommandChannel, /* 197 */
    Tcl_OpenFileChannel, /* 198 */
    Tcl_OpenTcpClient, /* 199 */
    Tcl_OpenTcpServer, /* 200 */
    Tcl_Preserve, /* 201 */
    Tcl_PrintDouble, /* 202 */
    Tcl_PutEnv, /* 203 */
    Tcl_PosixError, /* 204 */
    Tcl_QueueEvent, /* 205 */
    Tcl_Read, /* 206 */
    Tcl_ReapDetachedProcs, /* 207 */
    Tcl_RecordAndEval, /* 208 */
    Tcl_RecordAndEvalObj, /* 209 */
    Tcl_RegisterChannel, /* 210 */
    Tcl_RegisterObjType, /* 211 */
    Tcl_RegExpCompile, /* 212 */
    Tcl_RegExpExec, /* 213 */
    Tcl_RegExpMatch, /* 214 */
    Tcl_RegExpRange, /* 215 */
    Tcl_Release, /* 216 */
    Tcl_ResetResult, /* 217 */
    Tcl_ScanElement, /* 218 */
    Tcl_ScanCountedElement, /* 219 */
    Tcl_SeekOld, /* 220 */
    Tcl_ServiceAll, /* 221 */
    Tcl_ServiceEvent, /* 222 */
    Tcl_SetAssocData, /* 223 */
    Tcl_SetChannelBufferSize, /* 224 */
    Tcl_SetChannelOption, /* 225 */
    Tcl_SetCommandInfo, /* 226 */
    Tcl_SetErrno, /* 227 */
    Tcl_SetErrorCode, /* 228 */
    Tcl_SetMaxBlockTime, /* 229 */
    Tcl_SetPanicProc, /* 230 */
    Tcl_SetRecursionLimit, /* 231 */
    Tcl_SetResult, /* 232 */
    Tcl_SetServiceMode, /* 233 */
    Tcl_SetObjErrorCode, /* 234 */
    Tcl_SetObjResult, /* 235 */
    Tcl_SetStdChannel, /* 236 */
    Tcl_SetVar, /* 237 */
    Tcl_SetVar2, /* 238 */
    Tcl_SignalId, /* 239 */
    Tcl_SignalMsg, /* 240 */
    Tcl_SourceRCFile, /* 241 */
    Tcl_SplitList, /* 242 */
    Tcl_SplitPath, /* 243 */
    Tcl_StaticPackage, /* 244 */
    Tcl_StringMatch, /* 245 */
    Tcl_TellOld, /* 246 */
    Tcl_TraceVar, /* 247 */
    Tcl_TraceVar2, /* 248 */
    Tcl_TranslateFileName, /* 249 */
    Tcl_Ungets, /* 250 */
    Tcl_UnlinkVar, /* 251 */
    Tcl_UnregisterChannel, /* 252 */
    Tcl_UnsetVar, /* 253 */
    Tcl_UnsetVar2, /* 254 */
    Tcl_UntraceVar, /* 255 */
    Tcl_UntraceVar2, /* 256 */
    Tcl_UpdateLinkedVar, /* 257 */
    Tcl_UpVar, /* 258 */
    Tcl_UpVar2, /* 259 */
    Tcl_VarEval, /* 260 */
    Tcl_VarTraceInfo, /* 261 */
    Tcl_VarTraceInfo2, /* 262 */
    Tcl_Write, /* 263 */
    Tcl_WrongNumArgs, /* 264 */
    Tcl_DumpActiveMemory, /* 265 */
    Tcl_ValidateAllMemory, /* 266 */
    Tcl_AppendResultVA, /* 267 */
    Tcl_AppendStringsToObjVA, /* 268 */
    Tcl_HashStats, /* 269 */
    Tcl_ParseVar, /* 270 */
    Tcl_PkgPresent, /* 271 */
    Tcl_PkgPresentEx, /* 272 */
    Tcl_PkgProvide, /* 273 */
    Tcl_PkgRequire, /* 274 */
    Tcl_SetErrorCodeVA, /* 275 */
    Tcl_VarEvalVA, /* 276 */
    Tcl_WaitPid, /* 277 */
    Tcl_PanicVA, /* 278 */
    Tcl_GetVersion, /* 279 */
    Tcl_InitMemory, /* 280 */
    Tcl_StackChannel, /* 281 */
    Tcl_UnstackChannel, /* 282 */
    Tcl_GetStackedChannel, /* 283 */
    Tcl_SetMainLoop, /* 284 */
    0, /* 285 */
    Tcl_AppendObjToObj, /* 286 */
    Tcl_CreateEncoding, /* 287 */
    Tcl_CreateThreadExitHandler, /* 288 */
    Tcl_DeleteThreadExitHandler, /* 289 */
    Tcl_DiscardResult, /* 290 */
    Tcl_EvalEx, /* 291 */
    Tcl_EvalObjv, /* 292 */
    Tcl_EvalObjEx, /* 293 */
    Tcl_ExitThread, /* 294 */
    Tcl_ExternalToUtf, /* 295 */
    Tcl_ExternalToUtfDString, /* 296 */
    Tcl_FinalizeThread, /* 297 */
    Tcl_FinalizeNotifier, /* 298 */
    Tcl_FreeEncoding, /* 299 */
    Tcl_GetCurrentThread, /* 300 */
    Tcl_GetEncoding, /* 301 */
    Tcl_GetEncodingName, /* 302 */
    Tcl_GetEncodingNames, /* 303 */
    Tcl_GetIndexFromObjStruct, /* 304 */
    Tcl_GetThreadData, /* 305 */
    Tcl_GetVar2Ex, /* 306 */
    Tcl_InitNotifier, /* 307 */
    Tcl_MutexLock, /* 308 */
    Tcl_MutexUnlock, /* 309 */
    Tcl_ConditionNotify, /* 310 */
    Tcl_ConditionWait, /* 311 */
    Tcl_NumUtfChars, /* 312 */
    Tcl_ReadChars, /* 313 */
    Tcl_RestoreResult, /* 314 */
    Tcl_SaveResult, /* 315 */
    Tcl_SetSystemEncoding, /* 316 */
    Tcl_SetVar2Ex, /* 317 */
    Tcl_ThreadAlert, /* 318 */
    Tcl_ThreadQueueEvent, /* 319 */
    Tcl_UniCharAtIndex, /* 320 */
    Tcl_UniCharToLower, /* 321 */
    Tcl_UniCharToTitle, /* 322 */
    Tcl_UniCharToUpper, /* 323 */
    Tcl_UniCharToUtf, /* 324 */
    Tcl_UtfAtIndex, /* 325 */
    Tcl_UtfCharComplete, /* 326 */
    Tcl_UtfBackslash, /* 327 */
    Tcl_UtfFindFirst, /* 328 */
    Tcl_UtfFindLast, /* 329 */
    Tcl_UtfNext, /* 330 */
    Tcl_UtfPrev, /* 331 */
    Tcl_UtfToExternal, /* 332 */
    Tcl_UtfToExternalDString, /* 333 */
    Tcl_UtfToLower, /* 334 */
    Tcl_UtfToTitle, /* 335 */
    Tcl_UtfToUniChar, /* 336 */
    Tcl_UtfToUpper, /* 337 */
    Tcl_WriteChars, /* 338 */
    Tcl_WriteObj, /* 339 */
    Tcl_GetString, /* 340 */
    Tcl_GetDefaultEncodingDir, /* 341 */
    Tcl_SetDefaultEncodingDir, /* 342 */
    Tcl_AlertNotifier, /* 343 */
    Tcl_ServiceModeHook, /* 344 */
    Tcl_UniCharIsAlnum, /* 345 */
    Tcl_UniCharIsAlpha, /* 346 */
    Tcl_UniCharIsDigit, /* 347 */
    Tcl_UniCharIsLower, /* 348 */
    Tcl_UniCharIsSpace, /* 349 */
    Tcl_UniCharIsUpper, /* 350 */
    Tcl_UniCharIsWordChar, /* 351 */
    Tcl_UniCharLen, /* 352 */
    Tcl_UniCharNcmp, /* 353 */
    Tcl_UniCharToUtfDString, /* 354 */
    Tcl_UtfToUniCharDString, /* 355 */
    Tcl_GetRegExpFromObj, /* 356 */
    Tcl_EvalTokens, /* 357 */
    Tcl_FreeParse, /* 358 */
    Tcl_LogCommandInfo, /* 359 */
    Tcl_ParseBraces, /* 360 */
    Tcl_ParseCommand, /* 361 */
    Tcl_ParseExpr, /* 362 */
    Tcl_ParseQuotedString, /* 363 */
    Tcl_ParseVarName, /* 364 */
    Tcl_GetCwd, /* 365 */
    Tcl_Chdir, /* 366 */
    Tcl_Access, /* 367 */
    Tcl_Stat, /* 368 */
    Tcl_UtfNcmp, /* 369 */
    Tcl_UtfNcasecmp, /* 370 */
    Tcl_StringCaseMatch, /* 371 */
    Tcl_UniCharIsControl, /* 372 */
    Tcl_UniCharIsGraph, /* 373 */
    Tcl_UniCharIsPrint, /* 374 */
    Tcl_UniCharIsPunct, /* 375 */
    Tcl_RegExpExecObj, /* 376 */
    Tcl_RegExpGetInfo, /* 377 */
    Tcl_NewUnicodeObj, /* 378 */
    Tcl_SetUnicodeObj, /* 379 */
    Tcl_GetCharLength, /* 380 */
    Tcl_GetUniChar, /* 381 */
    Tcl_GetUnicode, /* 382 */
    Tcl_GetRange, /* 383 */
    Tcl_AppendUnicodeToObj, /* 384 */
    Tcl_RegExpMatchObj, /* 385 */
    Tcl_SetNotifier, /* 386 */
    Tcl_GetAllocMutex, /* 387 */
    Tcl_GetChannelNames, /* 388 */
    Tcl_GetChannelNamesEx, /* 389 */
    Tcl_ProcObjCmd, /* 390 */
    Tcl_ConditionFinalize, /* 391 */
    Tcl_MutexFinalize, /* 392 */
    Tcl_CreateThread, /* 393 */
    Tcl_ReadRaw, /* 394 */
    Tcl_WriteRaw, /* 395 */
    Tcl_GetTopChannel, /* 396 */
    Tcl_ChannelBuffered, /* 397 */
    Tcl_ChannelName, /* 398 */
    Tcl_ChannelVersion, /* 399 */
    Tcl_ChannelBlockModeProc, /* 400 */
    Tcl_ChannelCloseProc, /* 401 */
    Tcl_ChannelClose2Proc, /* 402 */
    Tcl_ChannelInputProc, /* 403 */
    Tcl_ChannelOutputProc, /* 404 */
    Tcl_ChannelSeekProc, /* 405 */
    Tcl_ChannelSetOptionProc, /* 406 */
    Tcl_ChannelGetOptionProc, /* 407 */
    Tcl_ChannelWatchProc, /* 408 */
    Tcl_ChannelGetHandleProc, /* 409 */
    Tcl_ChannelFlushProc, /* 410 */
    Tcl_ChannelHandlerProc, /* 411 */
    Tcl_JoinThread, /* 412 */
    Tcl_IsChannelShared, /* 413 */
    Tcl_IsChannelRegistered, /* 414 */
    Tcl_CutChannel, /* 415 */
    Tcl_SpliceChannel, /* 416 */
    Tcl_ClearChannelHandlers, /* 417 */
    Tcl_IsChannelExisting, /* 418 */
    Tcl_UniCharNcasecmp, /* 419 */
    Tcl_UniCharCaseMatch, /* 420 */
    Tcl_FindHashEntry, /* 421 */
    Tcl_CreateHashEntry, /* 422 */
    Tcl_InitCustomHashTable, /* 423 */
    Tcl_InitObjHashTable, /* 424 */
    Tcl_CommandTraceInfo, /* 425 */
    Tcl_TraceCommand, /* 426 */
    Tcl_UntraceCommand, /* 427 */
    Tcl_AttemptAlloc, /* 428 */
    Tcl_AttemptDbCkalloc, /* 429 */
    Tcl_AttemptRealloc, /* 430 */
    Tcl_AttemptDbCkrealloc, /* 431 */
    Tcl_AttemptSetObjLength, /* 432 */
    Tcl_GetChannelThread, /* 433 */
    Tcl_GetUnicodeFromObj, /* 434 */
    Tcl_GetMathFuncInfo, /* 435 */
    Tcl_ListMathFuncs, /* 436 */
    Tcl_SubstObj, /* 437 */
    Tcl_DetachChannel, /* 438 */
    Tcl_IsStandardChannel, /* 439 */
    Tcl_FSCopyFile, /* 440 */
    Tcl_FSCopyDirectory, /* 441 */
    Tcl_FSCreateDirectory, /* 442 */
    Tcl_FSDeleteFile, /* 443 */
    Tcl_FSLoadFile, /* 444 */
    Tcl_FSMatchInDirectory, /* 445 */
    Tcl_FSLink, /* 446 */
    Tcl_FSRemoveDirectory, /* 447 */
    Tcl_FSRenameFile, /* 448 */
    Tcl_FSLstat, /* 449 */
    Tcl_FSUtime, /* 450 */
    Tcl_FSFileAttrsGet, /* 451 */
    Tcl_FSFileAttrsSet, /* 452 */
    Tcl_FSFileAttrStrings, /* 453 */
    Tcl_FSStat, /* 454 */
    Tcl_FSAccess, /* 455 */
    Tcl_FSOpenFileChannel, /* 456 */
    Tcl_FSGetCwd, /* 457 */
    Tcl_FSChdir, /* 458 */
    Tcl_FSConvertToPathType, /* 459 */
    Tcl_FSJoinPath, /* 460 */
    Tcl_FSSplitPath, /* 461 */
    Tcl_FSEqualPaths, /* 462 */
    Tcl_FSGetNormalizedPath, /* 463 */
    Tcl_FSJoinToPath, /* 464 */
    Tcl_FSGetInternalRep, /* 465 */
    Tcl_FSGetTranslatedPath, /* 466 */
    Tcl_FSEvalFile, /* 467 */
    Tcl_FSNewNativePath, /* 468 */
    Tcl_FSGetNativePath, /* 469 */
    Tcl_FSFileSystemInfo, /* 470 */
    Tcl_FSPathSeparator, /* 471 */
    Tcl_FSListVolumes, /* 472 */
    Tcl_FSRegister, /* 473 */
    Tcl_FSUnregister, /* 474 */
    Tcl_FSData, /* 475 */
    Tcl_FSGetTranslatedStringPath, /* 476 */
    Tcl_FSGetFileSystemForPath, /* 477 */
    Tcl_FSGetPathType, /* 478 */
    Tcl_OutputBuffered, /* 479 */
    Tcl_FSMountsChanged, /* 480 */
    Tcl_EvalTokensStandard, /* 481 */
    Tcl_GetTime, /* 482 */
    Tcl_CreateObjTrace, /* 483 */
    Tcl_GetCommandInfoFromToken, /* 484 */
    Tcl_SetCommandInfoFromToken, /* 485 */
    Tcl_DbNewWideIntObj, /* 486 */
    Tcl_GetWideIntFromObj, /* 487 */
    Tcl_NewWideIntObj, /* 488 */
    Tcl_SetWideIntObj, /* 489 */
    Tcl_AllocStatBuf, /* 490 */
    Tcl_Seek, /* 491 */
    Tcl_Tell, /* 492 */
    Tcl_ChannelWideSeekProc, /* 493 */
    Tcl_DictObjPut, /* 494 */
    Tcl_DictObjGet, /* 495 */
    Tcl_DictObjRemove, /* 496 */
    Tcl_DictObjSize, /* 497 */
    Tcl_DictObjFirst, /* 498 */
    Tcl_DictObjNext, /* 499 */
    Tcl_DictObjDone, /* 500 */
    Tcl_DictObjPutKeyList, /* 501 */
    Tcl_DictObjRemoveKeyList, /* 502 */
    Tcl_NewDictObj, /* 503 */
    Tcl_DbNewDictObj, /* 504 */
    Tcl_RegisterConfig, /* 505 */
    Tcl_CreateNamespace, /* 506 */
    Tcl_DeleteNamespace, /* 507 */
    Tcl_AppendExportList, /* 508 */
    Tcl_Export, /* 509 */
    Tcl_Import, /* 510 */
    Tcl_ForgetImport, /* 511 */
    Tcl_GetCurrentNamespace, /* 512 */
    Tcl_GetGlobalNamespace, /* 513 */
    Tcl_FindNamespace, /* 514 */
    Tcl_FindCommand, /* 515 */
    Tcl_GetCommandFromObj, /* 516 */
    Tcl_GetCommandFullName, /* 517 */
    Tcl_FSEvalFileEx, /* 518 */
    Tcl_SetExitProc, /* 519 */
    Tcl_LimitAddHandler, /* 520 */
    Tcl_LimitRemoveHandler, /* 521 */
    Tcl_LimitReady, /* 522 */
    Tcl_LimitCheck, /* 523 */
    Tcl_LimitExceeded, /* 524 */
    Tcl_LimitSetCommands, /* 525 */
    Tcl_LimitSetTime, /* 526 */
    Tcl_LimitSetGranularity, /* 527 */
    Tcl_LimitTypeEnabled, /* 528 */
    Tcl_LimitTypeExceeded, /* 529 */
    Tcl_LimitTypeSet, /* 530 */
    Tcl_LimitTypeReset, /* 531 */
    Tcl_LimitGetCommands, /* 532 */
    Tcl_LimitGetTime, /* 533 */
    Tcl_LimitGetGranularity, /* 534 */
    Tcl_SaveInterpState, /* 535 */
    Tcl_RestoreInterpState, /* 536 */
    Tcl_DiscardInterpState, /* 537 */
    Tcl_SetReturnOptions, /* 538 */
    Tcl_GetReturnOptions, /* 539 */
    Tcl_IsEnsemble, /* 540 */
    Tcl_CreateEnsemble, /* 541 */
    Tcl_FindEnsemble, /* 542 */
    Tcl_SetEnsembleSubcommandList, /* 543 */
    Tcl_SetEnsembleMappingDict, /* 544 */
    Tcl_SetEnsembleUnknownHandler, /* 545 */
    Tcl_SetEnsembleFlags, /* 546 */
    Tcl_GetEnsembleSubcommandList, /* 547 */
    Tcl_GetEnsembleMappingDict, /* 548 */
    Tcl_GetEnsembleUnknownHandler, /* 549 */
    Tcl_GetEnsembleFlags, /* 550 */
    Tcl_GetEnsembleNamespace, /* 551 */
    Tcl_SetTimeProc, /* 552 */
    Tcl_QueryTimeProc, /* 553 */
    Tcl_ChannelThreadActionProc, /* 554 */
    Tcl_NewBignumObj, /* 555 */
    Tcl_DbNewBignumObj, /* 556 */
    Tcl_SetBignumObj, /* 557 */
    Tcl_GetBignumFromObj, /* 558 */
    Tcl_TakeBignumFromObj, /* 559 */
    Tcl_TruncateChannel, /* 560 */
    Tcl_ChannelTruncateProc, /* 561 */
    Tcl_SetChannelErrorInterp, /* 562 */
    Tcl_GetChannelErrorInterp, /* 563 */
    Tcl_SetChannelError, /* 564 */
    Tcl_GetChannelError, /* 565 */
    Tcl_InitBignumFromDouble, /* 566 */
    Tcl_GetNamespaceUnknownHandler, /* 567 */
    Tcl_SetNamespaceUnknownHandler, /* 568 */
    Tcl_GetEncodingFromObj, /* 569 */
    Tcl_GetEncodingSearchPath, /* 570 */
    Tcl_SetEncodingSearchPath, /* 571 */
    Tcl_GetEncodingNameFromEnvironment, /* 572 */
    Tcl_PkgRequireProc, /* 573 */
    Tcl_AppendObjToErrorInfo, /* 574 */
    Tcl_AppendLimitedToObj, /* 575 */
    Tcl_Format, /* 576 */
    Tcl_AppendFormatToObj, /* 577 */
    Tcl_ObjPrintf, /* 578 */
    Tcl_AppendPrintfToObj, /* 579 */
    Tcl_CancelEval, /* 580 */
    Tcl_Canceled, /* 581 */
    Tcl_CreatePipe, /* 582 */
    Tcl_NRCreateCommand, /* 583 */
    Tcl_NREvalObj, /* 584 */
    Tcl_NREvalObjv, /* 585 */
    Tcl_NRCmdSwap, /* 586 */
    Tcl_NRAddCallback, /* 587 */
    Tcl_NRCallObjProc, /* 588 */
    Tcl_GetFSDeviceFromStat, /* 589 */
    Tcl_GetFSInodeFromStat, /* 590 */
    Tcl_GetModeFromStat, /* 591 */
    Tcl_GetLinkCountFromStat, /* 592 */
    Tcl_GetUserIdFromStat, /* 593 */
    Tcl_GetGroupIdFromStat, /* 594 */
    Tcl_GetDeviceTypeFromStat, /* 595 */
    Tcl_GetAccessTimeFromStat, /* 596 */
    Tcl_GetModificationTimeFromStat, /* 597 */
    Tcl_GetChangeTimeFromStat, /* 598 */
    Tcl_GetSizeFromStat, /* 599 */
    Tcl_GetBlocksFromStat, /* 600 */
    Tcl_GetBlockSizeFromStat, /* 601 */
    Tcl_SetEnsembleParameterList, /* 602 */
    Tcl_GetEnsembleParameterList, /* 603 */
    Tcl_ParseArgsObjv, /* 604 */
    Tcl_GetErrorLine, /* 605 */
    Tcl_SetErrorLine, /* 606 */
    Tcl_TransferResult, /* 607 */
    Tcl_InterpActive, /* 608 */
    Tcl_BackgroundException, /* 609 */
    Tcl_ZlibDeflate, /* 610 */
    Tcl_ZlibInflate, /* 611 */
    Tcl_ZlibCRC32, /* 612 */
    Tcl_ZlibAdler32, /* 613 */
    Tcl_ZlibStreamInit, /* 614 */
    Tcl_ZlibStreamGetCommandName, /* 615 */
    Tcl_ZlibStreamEof, /* 616 */
    Tcl_ZlibStreamChecksum, /* 617 */
    Tcl_ZlibStreamPut, /* 618 */
    Tcl_ZlibStreamGet, /* 619 */
    Tcl_ZlibStreamClose, /* 620 */
    Tcl_ZlibStreamReset, /* 621 */
    Tcl_SetStartupScript, /* 622 */
    Tcl_GetStartupScript, /* 623 */
    Tcl_CloseEx, /* 624 */
    Tcl_NRExprObj, /* 625 */
    Tcl_NRSubstObj, /* 626 */
    Tcl_LoadFile, /* 627 */
    Tcl_FindSymbol, /* 628 */
    Tcl_FSUnloadFile, /* 629 */
    Tcl_ZlibStreamSetCompressionDictionary, /* 630 */
    0, /* 631 */
    0, /* 632 */
    0, /* 633 */
    0, /* 634 */
    0, /* 635 */
    0, /* 636 */
    0, /* 637 */
    0, /* 638 */
    0, /* 639 */
    0, /* 640 */
    0, /* 641 */
    0, /* 642 */
    0, /* 643 */
    0, /* 644 */
    0, /* 645 */
    0, /* 646 */
    0, /* 647 */
    TclUnusedStubEntry, /* 648 */
};

/* !END!: Do not edit above this line. */
