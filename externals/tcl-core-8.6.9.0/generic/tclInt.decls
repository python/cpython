# tclInt.decls --
#
#	This file contains the declarations for all unsupported
#	functions that are exported by the Tcl library.  This file
#	is used to generate the tclIntDecls.h, tclIntPlatDecls.h
#	and tclStubInit.c files
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2001 by Kevin B. Kenny.  All rights reserved.
# Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

library tcl

# Define the unsupported generic interfaces.

interface tclInt

# Declare each of the functions in the unsupported internal Tcl
# interface.  These interfaces are allowed to changed between versions.
# Use at your own risk.  Note that the position of functions should not
# be changed between versions to avoid gratuitous incompatibilities.

# Replaced by Tcl_FSAccess in 8.4:
#declare 0 {
#    int TclAccess(const char *path, int mode)
#}
#declare 1 {
#    int TclAccessDeleteProc(TclAccessProc_ *proc)
#}
#declare 2 {
#    int TclAccessInsertProc(TclAccessProc_ *proc)
#}
declare 3 {
    void TclAllocateFreeObjects(void)
}
# Replaced by TclpChdir in 8.1:
#  declare 4 {
#      int TclChdir(Tcl_Interp *interp, char *dirName)
#  }
declare 5 {
    int TclCleanupChildren(Tcl_Interp *interp, int numPids, Tcl_Pid *pidPtr,
	    Tcl_Channel errorChan)
}
declare 6 {
    void TclCleanupCommand(Command *cmdPtr)
}
declare 7 {
    int TclCopyAndCollapse(int count, const char *src, char *dst)
}
declare 8 {
    int TclCopyChannelOld(Tcl_Interp *interp, Tcl_Channel inChan,
	    Tcl_Channel outChan, int toRead, Tcl_Obj *cmdPtr)
}

# TclCreatePipeline unofficially exported for use by BLT.

declare 9 {
    int TclCreatePipeline(Tcl_Interp *interp, int argc, const char **argv,
	    Tcl_Pid **pidArrayPtr, TclFile *inPipePtr, TclFile *outPipePtr,
	    TclFile *errFilePtr)
}
declare 10 {
    int TclCreateProc(Tcl_Interp *interp, Namespace *nsPtr,
	    const char *procName,
	    Tcl_Obj *argsPtr, Tcl_Obj *bodyPtr, Proc **procPtrPtr)
}
declare 11 {
    void TclDeleteCompiledLocalVars(Interp *iPtr, CallFrame *framePtr)
}
declare 12 {
    void TclDeleteVars(Interp *iPtr, TclVarHashTable *tablePtr)
}
# Removed in 8.5
#declare 13 {
#    int TclDoGlob(Tcl_Interp *interp, char *separators,
#	    Tcl_DString *headPtr, char *tail, Tcl_GlobTypeData *types)
#}
declare 14 {
    int TclDumpMemoryInfo(ClientData clientData, int flags)
}
# Removed in 8.1:
#  declare 15 {
#      void TclExpandParseValue(ParseValue *pvPtr, int needed)
#  }
declare 16 {
    void TclExprFloatError(Tcl_Interp *interp, double value)
}
# Removed in 8.4
#declare 17 {
#    int TclFileAttrsCmd(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
#}
#declare 18 {
#    int TclFileCopyCmd(Tcl_Interp *interp, int argc, char **argv)
#}
#declare 19 {
#    int TclFileDeleteCmd(Tcl_Interp *interp, int argc, char **argv)
#}
#declare 20 {
#    int TclFileMakeDirsCmd(Tcl_Interp *interp, int argc, char **argv)
#}
#declare 21 {
#    int TclFileRenameCmd(Tcl_Interp *interp, int argc, char **argv)
#}
declare 22 {
    int TclFindElement(Tcl_Interp *interp, const char *listStr,
	    int listLength, const char **elementPtr, const char **nextPtr,
	    int *sizePtr, int *bracePtr)
}
declare 23 {
    Proc *TclFindProc(Interp *iPtr, const char *procName)
}
# Replaced with macro (see tclInt.h) in Tcl 8.5.0, restored in 8.5.10
declare 24 {
    int TclFormatInt(char *buffer, long n)
}
declare 25 {
    void TclFreePackageInfo(Interp *iPtr)
}
# Removed in 8.1:
#  declare 26 {
#      char *TclGetCwd(Tcl_Interp *interp)
#  }
# Removed in 8.5
#declare 27 {
#    int TclGetDate(char *p, unsigned long now, long zone,
#	    unsigned long *timePtr)
#}
declare 28 {
    Tcl_Channel TclpGetDefaultStdChannel(int type)
}
# Removed in 8.4b2:
#declare 29 {
#    Tcl_Obj *TclGetElementOfIndexedArray(Tcl_Interp *interp,
#	    int localIndex, Tcl_Obj *elemPtr, int flags)
#}
# Replaced by char *TclGetEnv(const char *name, Tcl_DString *valuePtr) in 8.1:
#  declare 30 {
#      char *TclGetEnv(const char *name)
#  }
declare 31 {
    const char *TclGetExtension(const char *name)
}
declare 32 {
    int TclGetFrame(Tcl_Interp *interp, const char *str,
	    CallFrame **framePtrPtr)
}
# Removed in Tcl 8.5
#declare 33 {
#    TclCmdProcType TclGetInterpProc(void)
#}
declare 34 {
    int TclGetIntForIndex(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    int endValue, int *indexPtr)
}
# Removed in 8.4b2:
#declare 35 {
#    Tcl_Obj *TclGetIndexedScalar(Tcl_Interp *interp, int localIndex,
#	    int flags)
#}
# Removed in 8.6a2
#declare 36 {
#    int TclGetLong(Tcl_Interp *interp, const char *str, long *longPtr)
#}
declare 37 {
    int TclGetLoadedPackages(Tcl_Interp *interp, const char *targetName)
}
declare 38 {
    int TclGetNamespaceForQualName(Tcl_Interp *interp, const char *qualName,
	    Namespace *cxtNsPtr, int flags, Namespace **nsPtrPtr,
	    Namespace **altNsPtrPtr, Namespace **actualCxtPtrPtr,
	    const char **simpleNamePtr)
}
declare 39 {
    TclObjCmdProcType TclGetObjInterpProc(void)
}
declare 40 {
    int TclGetOpenMode(Tcl_Interp *interp, const char *str, int *seekFlagPtr)
}
declare 41 {
    Tcl_Command TclGetOriginalCommand(Tcl_Command command)
}
declare 42 {
    CONST86 char *TclpGetUserHome(const char *name, Tcl_DString *bufferPtr)
}
# Removed in Tcl 8.5a2
#declare 43 {
#    int TclGlobalInvoke(Tcl_Interp *interp, int argc, CONST84 char **argv,
#	    int flags)
#}
declare 44 {
    int TclGuessPackageName(const char *fileName, Tcl_DString *bufPtr)
}
declare 45 {
    int TclHideUnsafeCommands(Tcl_Interp *interp)
}
declare 46 {
    int TclInExit(void)
}
# Removed in 8.4b2:
#declare 47 {
#    Tcl_Obj *TclIncrElementOfIndexedArray(Tcl_Interp *interp,
#	    int localIndex, Tcl_Obj *elemPtr, long incrAmount)
#}
# Removed in 8.4b2:
#declare 48 {
#    Tcl_Obj *TclIncrIndexedScalar(Tcl_Interp *interp, int localIndex,
#	    long incrAmount)
#}
#declare 49 {
#    Tcl_Obj *TclIncrVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
#	    Tcl_Obj *part2Ptr, long incrAmount, int part1NotParsed)
#}
declare 50 {
    void TclInitCompiledLocals(Tcl_Interp *interp, CallFrame *framePtr,
	    Namespace *nsPtr)
}
declare 51 {
    int TclInterpInit(Tcl_Interp *interp)
}
# Removed in Tcl 8.5a2
#declare 52 {
#    int TclInvoke(Tcl_Interp *interp, int argc, CONST84 char **argv,
#	    int flags)
#}
declare 53 {
    int TclInvokeObjectCommand(ClientData clientData, Tcl_Interp *interp,
	    int argc, CONST84 char **argv)
}
declare 54 {
    int TclInvokeStringCommand(ClientData clientData, Tcl_Interp *interp,
	    int objc, Tcl_Obj *const objv[])
}
declare 55 {
    Proc *TclIsProc(Command *cmdPtr)
}
# Replaced with TclpLoadFile in 8.1:
#  declare 56 {
#      int TclLoadFile(Tcl_Interp *interp, char *fileName, char *sym1,
#  	    char *sym2, Tcl_PackageInitProc **proc1Ptr,
#  	    Tcl_PackageInitProc **proc2Ptr)
#  }
# Signature changed to take a length in 8.1:
#  declare 57 {
#      int TclLooksLikeInt(char *p)
#  }
declare 58 {
    Var *TclLookupVar(Tcl_Interp *interp, const char *part1, const char *part2,
	    int flags, const char *msg, int createPart1, int createPart2,
	    Var **arrayPtrPtr)
}
# Replaced by Tcl_FSMatchInDirectory in 8.4
#declare 59 {
#    int TclpMatchFiles(Tcl_Interp *interp, char *separators,
#	    Tcl_DString *dirPtr, char *pattern, char *tail)
#}
declare 60 {
    int TclNeedSpace(const char *start, const char *end)
}
declare 61 {
    Tcl_Obj *TclNewProcBodyObj(Proc *procPtr)
}
declare 62 {
    int TclObjCommandComplete(Tcl_Obj *cmdPtr)
}
declare 63 {
    int TclObjInterpProc(ClientData clientData, Tcl_Interp *interp,
	    int objc, Tcl_Obj *const objv[])
}
declare 64 {
    int TclObjInvoke(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[],
	    int flags)
}
# Removed in Tcl 8.5a2
#declare 65 {
#    int TclObjInvokeGlobal(Tcl_Interp *interp, int objc,
#	    Tcl_Obj *const objv[], int flags)
#}
#declare 66 {
#    int TclOpenFileChannelDeleteProc(TclOpenFileChannelProc_ *proc)
#}
#declare 67 {
#    int TclOpenFileChannelInsertProc(TclOpenFileChannelProc_ *proc)
#}
# Replaced by Tcl_FSAccess in 8.4:
#declare 68 {
#    int TclpAccess(const char *path, int mode)
#}
declare 69 {
    char *TclpAlloc(unsigned int size)
}
#declare 70 {
#    int TclpCopyFile(const char *source, const char *dest)
#}
#declare 71 {
#    int TclpCopyDirectory(const char *source, const char *dest,
#	    Tcl_DString *errorPtr)
#}
#declare 72 {
#    int TclpCreateDirectory(const char *path)
#}
#declare 73 {
#    int TclpDeleteFile(const char *path)
#}
declare 74 {
    void TclpFree(char *ptr)
}
declare 75 {
    unsigned long TclpGetClicks(void)
}
declare 76 {
    unsigned long TclpGetSeconds(void)
}

# deprecated
declare 77 {
    void TclpGetTime(Tcl_Time *time)
}
# Removed in 8.6:
#declare 78 {
#    int TclpGetTimeZone(unsigned long time)
#}
# Replaced by Tcl_FSListVolumes in 8.4:
#declare 79 {
#    int TclpListVolumes(Tcl_Interp *interp)
#}
# Replaced by Tcl_FSOpenFileChannel in 8.4:
#declare 80 {
#    Tcl_Channel TclpOpenFileChannel(Tcl_Interp *interp, char *fileName,
#	    char *modeString, int permissions)
#}
declare 81 {
    char *TclpRealloc(char *ptr, unsigned int size)
}
#declare 82 {
#    int TclpRemoveDirectory(const char *path, int recursive,
#	    Tcl_DString *errorPtr)
#}
#declare 83 {
#    int TclpRenameFile(const char *source, const char *dest)
#}
# Removed in 8.1:
#  declare 84 {
#      int TclParseBraces(Tcl_Interp *interp, char *str, char **termPtr,
#  	    ParseValue *pvPtr)
#  }
#  declare 85 {
#      int TclParseNestedCmd(Tcl_Interp *interp, char *str, int flags,
#  	    char **termPtr, ParseValue *pvPtr)
#  }
#  declare 86 {
#      int TclParseQuotes(Tcl_Interp *interp, char *str, int termChar,
#  	    int flags, char **termPtr, ParseValue *pvPtr)
#  }
#  declare 87 {
#      void TclPlatformInit(Tcl_Interp *interp)
#  }
declare 88 {
    char *TclPrecTraceProc(ClientData clientData, Tcl_Interp *interp,
	    const char *name1, const char *name2, int flags)
}
declare 89 {
    int TclPreventAliasLoop(Tcl_Interp *interp, Tcl_Interp *cmdInterp,
	    Tcl_Command cmd)
}
# Removed in 8.1 (only available if compiled with TCL_COMPILE_DEBUG):
#  declare 90 {
#      void TclPrintByteCodeObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
#  }
declare 91 {
    void TclProcCleanupProc(Proc *procPtr)
}
declare 92 {
    int TclProcCompileProc(Tcl_Interp *interp, Proc *procPtr,
	    Tcl_Obj *bodyPtr, Namespace *nsPtr, const char *description,
	    const char *procName)
}
declare 93 {
    void TclProcDeleteProc(ClientData clientData)
}
# Removed in Tcl 8.5:
#declare 94 {
#    int TclProcInterpProc(ClientData clientData, Tcl_Interp *interp,
#	    int argc, const char **argv)
#}
# Replaced by Tcl_FSStat in 8.4:
#declare 95 {
#    int TclpStat(const char *path, Tcl_StatBuf *buf)
#}
declare 96 {
    int TclRenameCommand(Tcl_Interp *interp, const char *oldName,
            const char *newName)
}
declare 97 {
    void TclResetShadowedCmdRefs(Tcl_Interp *interp, Command *newCmdPtr)
}
declare 98 {
    int TclServiceIdle(void)
}
# Removed in 8.4b2:
#declare 99 {
#    Tcl_Obj *TclSetElementOfIndexedArray(Tcl_Interp *interp, int localIndex,
#	    Tcl_Obj *elemPtr, Tcl_Obj *objPtr, int flags)
#}
# Removed in 8.4b2:
#declare 100 {
#    Tcl_Obj *TclSetIndexedScalar(Tcl_Interp *interp, int localIndex,
#	    Tcl_Obj *objPtr, int flags)
#}
declare 101 {
    CONST86 char *TclSetPreInitScript(const char *string)
}
declare 102 {
    void TclSetupEnv(Tcl_Interp *interp)
}
declare 103 {
    int TclSockGetPort(Tcl_Interp *interp, const char *str, const char *proto,
	    int *portPtr)
}
declare 104 {
    int TclSockMinimumBuffersOld(int sock, int size)
}
# Replaced by Tcl_FSStat in 8.4:
#declare 105 {
#    int TclStat(const char *path, Tcl_StatBuf *buf)
#}
#declare 106 {
#    int TclStatDeleteProc(TclStatProc_ *proc)
#}
#declare 107 {
#    int TclStatInsertProc(TclStatProc_ *proc)
#}
declare 108 {
    void TclTeardownNamespace(Namespace *nsPtr)
}
declare 109 {
    int TclUpdateReturnInfo(Interp *iPtr)
}
declare 110 {
    int TclSockMinimumBuffers(void *sock, int size)
}
# Removed in 8.1:
#  declare 110 {
#      char *TclWordEnd(char *start, char *lastChar, int nested, int *semiPtr)
#  }

# Procedures used in conjunction with Tcl namespaces. They are
# defined here instead of in tcl.decls since they are not stable yet.

declare 111 {
    void Tcl_AddInterpResolvers(Tcl_Interp *interp, const char *name,
	    Tcl_ResolveCmdProc *cmdProc, Tcl_ResolveVarProc *varProc,
	    Tcl_ResolveCompiledVarProc *compiledVarProc)
}
declare 112 {
    int Tcl_AppendExportList(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    Tcl_Obj *objPtr)
}
declare 113 {
    Tcl_Namespace *Tcl_CreateNamespace(Tcl_Interp *interp, const char *name,
	    ClientData clientData, Tcl_NamespaceDeleteProc *deleteProc)
}
declare 114 {
    void Tcl_DeleteNamespace(Tcl_Namespace *nsPtr)
}
declare 115 {
    int Tcl_Export(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern, int resetListFirst)
}
declare 116 {
    Tcl_Command Tcl_FindCommand(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 117 {
    Tcl_Namespace *Tcl_FindNamespace(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 118 {
    int Tcl_GetInterpResolvers(Tcl_Interp *interp, const char *name,
	    Tcl_ResolverInfo *resInfo)
}
declare 119 {
    int Tcl_GetNamespaceResolvers(Tcl_Namespace *namespacePtr,
	    Tcl_ResolverInfo *resInfo)
}
declare 120 {
    Tcl_Var Tcl_FindNamespaceVar(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 121 {
    int Tcl_ForgetImport(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern)
}
declare 122 {
    Tcl_Command Tcl_GetCommandFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 123 {
    void Tcl_GetCommandFullName(Tcl_Interp *interp, Tcl_Command command,
	    Tcl_Obj *objPtr)
}
declare 124 {
    Tcl_Namespace *Tcl_GetCurrentNamespace(Tcl_Interp *interp)
}
declare 125 {
    Tcl_Namespace *Tcl_GetGlobalNamespace(Tcl_Interp *interp)
}
declare 126 {
    void Tcl_GetVariableFullName(Tcl_Interp *interp, Tcl_Var variable,
	    Tcl_Obj *objPtr)
}
declare 127 {
    int Tcl_Import(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern, int allowOverwrite)
}
declare 128 {
    void Tcl_PopCallFrame(Tcl_Interp *interp)
}
declare 129 {
    int Tcl_PushCallFrame(Tcl_Interp *interp, Tcl_CallFrame *framePtr,
	    Tcl_Namespace *nsPtr, int isProcCallFrame)
}
declare 130 {
    int Tcl_RemoveInterpResolvers(Tcl_Interp *interp, const char *name)
}
declare 131 {
    void Tcl_SetNamespaceResolvers(Tcl_Namespace *namespacePtr,
	    Tcl_ResolveCmdProc *cmdProc, Tcl_ResolveVarProc *varProc,
	    Tcl_ResolveCompiledVarProc *compiledVarProc)
}
declare 132 {
    int TclpHasSockets(Tcl_Interp *interp)
}
declare 133 {
    struct tm *TclpGetDate(const time_t *time, int useGMT)
}
# Removed in 8.5
#declare 134 {
#    size_t TclpStrftime(char *s, size_t maxsize, const char *format,
#	    const struct tm *t, int useGMT)
#}
#declare 135 {
#    int TclpCheckStackSpace(void)
#}

# Added in 8.1:

#declare 137 {
#   int TclpChdir(const char *dirName)
#}
declare 138 {
    CONST84_RETURN char *TclGetEnv(const char *name, Tcl_DString *valuePtr)
}
#declare 139 {
#    int TclpLoadFile(Tcl_Interp *interp, char *fileName, char *sym1,
#	    char *sym2, Tcl_PackageInitProc **proc1Ptr,
#	    Tcl_PackageInitProc **proc2Ptr, ClientData *clientDataPtr)
#}
#declare 140 {
#    int TclLooksLikeInt(const char *bytes, int length)
#}
# This is used by TclX, but should otherwise be considered private
declare 141 {
    CONST84_RETURN char *TclpGetCwd(Tcl_Interp *interp, Tcl_DString *cwdPtr)
}
declare 142 {
    int TclSetByteCodeFromAny(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    CompileHookProc *hookProc, ClientData clientData)
}
declare 143 {
    int TclAddLiteralObj(struct CompileEnv *envPtr, Tcl_Obj *objPtr,
	    LiteralEntry **litPtrPtr)
}
declare 144 {
    void TclHideLiteral(Tcl_Interp *interp, struct CompileEnv *envPtr,
	    int index)
}
declare 145 {
    const struct AuxDataType *TclGetAuxDataType(const char *typeName)
}
declare 146 {
    TclHandle TclHandleCreate(void *ptr)
}
declare 147 {
    void TclHandleFree(TclHandle handle)
}
declare 148 {
    TclHandle TclHandlePreserve(TclHandle handle)
}
declare 149 {
    void TclHandleRelease(TclHandle handle)
}

# Added for Tcl 8.2

declare 150 {
    int TclRegAbout(Tcl_Interp *interp, Tcl_RegExp re)
}
declare 151 {
    void TclRegExpRangeUniChar(Tcl_RegExp re, int index, int *startPtr,
	    int *endPtr)
}
declare 152 {
    void TclSetLibraryPath(Tcl_Obj *pathPtr)
}
declare 153 {
    Tcl_Obj *TclGetLibraryPath(void)
}

# moved to tclTest.c (static) in 8.3.2/8.4a2
#declare 154 {
#    int TclTestChannelCmd(ClientData clientData,
#    Tcl_Interp *interp, int argc, char **argv)
#}
#declare 155 {
#    int TclTestChannelEventCmd(ClientData clientData,
#	     Tcl_Interp *interp, int argc, char **argv)
#}

declare 156 {
    void TclRegError(Tcl_Interp *interp, const char *msg,
	    int status)
}
declare 157 {
    Var *TclVarTraceExists(Tcl_Interp *interp, const char *varName)
}
# REMOVED (except from stub table) - use public Tcl_SetStartupScript()
declare 158 {
    void TclSetStartupScriptFileName(const char *filename)
}
# REMOVED (except from stub table) - use public Tcl_GetStartupScript()
declare 159 {
    const char *TclGetStartupScriptFileName(void)
}
#declare 160 {
#    int TclpMatchFilesTypes(Tcl_Interp *interp, char *separators,
#	    Tcl_DString *dirPtr, char *pattern, char *tail,
#	    GlobTypeData *types)
#}

# new in 8.3.2/8.4a2
declare 161 {
    int TclChannelTransform(Tcl_Interp *interp, Tcl_Channel chan,
	    Tcl_Obj *cmdObjPtr)
}
declare 162 {
    void TclChannelEventScriptInvoker(ClientData clientData, int flags)
}

# ALERT: The result of 'TclGetInstructionTable' is actually a
# "const InstructionDesc*" but we do not want to describe this structure in
# "tclInt.h". It is described in "tclCompile.h". Use a cast to the
# correct type when calling this procedure.

declare 163 {
    const void *TclGetInstructionTable(void)
}

# ALERT: The argument of 'TclExpandCodeArray' is actually a
# "CompileEnv*" but we do not want to describe this structure in
# "tclInt.h". It is described in "tclCompile.h".

declare 164 {
    void TclExpandCodeArray(void *envPtr)
}

# These functions are vfs aware, but are generally only useful internally.
declare 165 {
    void TclpSetInitialEncodings(void)
}

# New function due to TIP #33
declare 166 {
    int TclListObjSetElement(Tcl_Interp *interp, Tcl_Obj *listPtr,
	    int index, Tcl_Obj *valuePtr)
}

# VFS-aware versions of Tcl*StartupScriptFileName (158 and 159 above)
# REMOVED (except from stub table) - use public Tcl_SetStartupScript()
declare 167 {
    void TclSetStartupScriptPath(Tcl_Obj *pathPtr)
}
# REMOVED (except from stub table) - use public Tcl_GetStartupScript()
declare 168 {
    Tcl_Obj *TclGetStartupScriptPath(void)
}
# variant of Tcl_UtfNCmp that takes n as bytes, not chars
declare 169 {
    int TclpUtfNcmp2(const char *s1, const char *s2, unsigned long n)
}
declare 170 {
    int TclCheckInterpTraces(Tcl_Interp *interp, const char *command,
	    int numChars, Command *cmdPtr, int result, int traceFlags,
	    int objc, Tcl_Obj *const objv[])
}
declare 171 {
    int TclCheckExecutionTraces(Tcl_Interp *interp, const char *command,
	    int numChars, Command *cmdPtr, int result, int traceFlags,
	    int objc, Tcl_Obj *const objv[])
}
declare 172 {
    int TclInThreadExit(void)
}

# added for 8.4.2

declare 173 {
    int TclUniCharMatch(const Tcl_UniChar *string, int strLen,
	    const Tcl_UniChar *pattern, int ptnLen, int flags)
}

# added for 8.4.3

#declare 174 {
#    Tcl_Obj *TclIncrWideVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
#	    Tcl_Obj *part2Ptr, Tcl_WideInt wideIncrAmount, int part1NotParsed)
#}

# Factoring out of trace code

declare 175 {
    int TclCallVarTraces(Interp *iPtr, Var *arrayPtr, Var *varPtr,
	    const char *part1, const char *part2, int flags, int leaveErrMsg)
}
declare 176 {
    void TclCleanupVar(Var *varPtr, Var *arrayPtr)
}
declare 177 {
    void TclVarErrMsg(Tcl_Interp *interp, const char *part1, const char *part2,
	    const char *operation, const char *reason)
}
# TIP 338 made these public - now declared in tcl.h too
declare 178 {
    void Tcl_SetStartupScript(Tcl_Obj *pathPtr, const char *encodingName)
}
declare 179 {
    Tcl_Obj *Tcl_GetStartupScript(const char **encodingNamePtr)
}

# REMOVED
# Allocate lists without copying arrays
# declare 180 {
#    Tcl_Obj *TclNewListObjDirect(int objc, Tcl_Obj **objv)
# }
#declare 181 {
#    Tcl_Obj *TclDbNewListObjDirect(int objc, Tcl_Obj **objv,
#	    const char *file, int line)
#}

# TclpGmtime and TclpLocaltime promoted to the generic interface from unix

declare 182 {
     struct tm *TclpLocaltime(const time_t *clock)
}
declare 183 {
     struct tm *TclpGmtime(const time_t *clock)
}

# For the new "Thread Storage" subsystem.

### REMOVED on grounds it should never have been exposed. All these
### functions are now either static in tclThreadStorage.c or
### MODULE_SCOPE.
# declare 184 {
#      void TclThreadStorageLockInit(void)
# }
# declare 185 {
#      void TclThreadStorageLock(void)
# }
# declare 186 {
#      void TclThreadStorageUnlock(void)
# }
# declare 187 {
#      void TclThreadStoragePrint(FILE *outFile, int flags)
# }
# declare 188 {
#      Tcl_HashTable *TclThreadStorageGetHashTable(Tcl_ThreadId id)
# }
# declare 189 {
#      Tcl_HashTable *TclThreadStorageInit(Tcl_ThreadId id, void *reserved)
# }
# declare 190 {
#      void TclThreadStorageDataKeyInit(Tcl_ThreadDataKey *keyPtr)
# }
# declare 191 {
#      void *TclThreadStorageDataKeyGet(Tcl_ThreadDataKey *keyPtr)
# }
# declare 192 {
#      void TclThreadStorageDataKeySet(Tcl_ThreadDataKey *keyPtr, void *data)
# }
# declare 193 {
#      void TclFinalizeThreadStorageThread(Tcl_ThreadId id)
# }
# declare 194 {
#      void TclFinalizeThreadStorage(void)
# }
# declare 195 {
#      void TclFinalizeThreadStorageData(Tcl_ThreadDataKey *keyPtr)
# }
# declare 196 {
#      void TclFinalizeThreadStorageDataKey(Tcl_ThreadDataKey *keyPtr)
# }

#
# Added in tcl8.5a5 for compiler/executor experimentation.
# Disabled in Tcl 8.5.1; experiments terminated. :/
#
#declare 197 {
#    int TclCompEvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
#		        const CmdFrame *invoker, int word)
#}
declare 198 {
    int TclObjGetFrame(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    CallFrame **framePtrPtr)
}

#declare 199 {
#    int TclMatchIsTrivial(const char *pattern)
#}

# 200-208 exported for use by the test suite [Bug 1054748]
declare 200 {
    int TclpObjRemoveDirectory(Tcl_Obj *pathPtr, int recursive,
	Tcl_Obj **errorPtr)
}
declare 201 {
    int TclpObjCopyDirectory(Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr,
	Tcl_Obj **errorPtr)
}
declare 202 {
    int TclpObjCreateDirectory(Tcl_Obj *pathPtr)
}
declare 203 {
    int TclpObjDeleteFile(Tcl_Obj *pathPtr)
}
declare 204 {
    int TclpObjCopyFile(Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr)
}
declare 205 {
    int TclpObjRenameFile(Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr)
}
declare 206 {
    int TclpObjStat(Tcl_Obj *pathPtr, Tcl_StatBuf *buf)
}
declare 207 {
    int TclpObjAccess(Tcl_Obj *pathPtr, int mode)
}
declare 208 {
    Tcl_Channel TclpOpenFileChannel(Tcl_Interp *interp,
	    Tcl_Obj *pathPtr, int mode, int permissions)
}
# Made public by TIP 258
#declare 209 {
#    Tcl_Obj *TclGetEncodingSearchPath(void)
#}
#declare 210 {
#    int TclSetEncodingSearchPath(Tcl_Obj *searchPath)
#}
#declare 211 {
#    const char *TclpGetEncodingNameFromEnvironment(Tcl_DString *bufPtr)
#}
declare 212 {
    void TclpFindExecutable(const char *argv0)
}
declare 213 {
    Tcl_Obj *TclGetObjNameOfExecutable(void)
}
declare 214 {
    void TclSetObjNameOfExecutable(Tcl_Obj *name, Tcl_Encoding encoding)
}
declare 215 {
    void *TclStackAlloc(Tcl_Interp *interp, int numBytes)
}
declare 216 {
    void TclStackFree(Tcl_Interp *interp, void *freePtr)
}
declare 217 {
    int TclPushStackFrame(Tcl_Interp *interp, Tcl_CallFrame **framePtrPtr,
            Tcl_Namespace *namespacePtr, int isProcCallFrame)
}
declare 218 {
    void TclPopStackFrame(Tcl_Interp *interp)
}

# for use in tclTest.c
declare 224 {
    TclPlatformType *TclGetPlatform(void)
}

#
declare 225 {
    Tcl_Obj *TclTraceDictPath(Tcl_Interp *interp, Tcl_Obj *rootPtr,
	    int keyc, Tcl_Obj *const keyv[], int flags)
}
declare 226 {
    int TclObjBeingDeleted(Tcl_Obj *objPtr)
}
declare 227 {
    void TclSetNsPath(Namespace *nsPtr, int pathLength,
            Tcl_Namespace *pathAry[])
}
#  Used to be needed for TclOO-extension; unneeded now that TclOO is in the
#  core and NRE-enabled
#  declare 228 {
#      int TclObjInterpProcCore(register Tcl_Interp *interp, Tcl_Obj *procNameObj,
#             int skip, ProcErrorProc *errorProc)
#  }
declare 229 {
    int	TclPtrMakeUpvar(Tcl_Interp *interp, Var *otherP1Ptr,
	    const char *myName, int myFlags, int index)
}
declare 230 {
    Var *TclObjLookupVar(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
	    const char *part2, int flags, const char *msg,
	    const int createPart1, const int createPart2, Var **arrayPtrPtr)
}
declare 231 {
    int	TclGetNamespaceFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    Tcl_Namespace **nsPtrPtr)
}

# Bits and pieces of TIP#280's guts
declare 232 {
    int TclEvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags,
	    const CmdFrame *invoker, int word)
}
declare 233 {
    void TclGetSrcInfoForPc(CmdFrame *contextPtr)
}

# Exports for VarReform compat: Itcl, XOTcl like to peek into our varTables :(
declare 234 {
    Var *TclVarHashCreateVar(TclVarHashTable *tablePtr, const char *key,
             int *newPtr)
}
declare 235 {
    void TclInitVarHashTable(TclVarHashTable *tablePtr, Namespace *nsPtr)
}


# TIP 337 made this one public
declare 236 {
    void TclBackgroundException(Tcl_Interp *interp, int code)
}

# TIP #285: Script cancellation support.
declare 237 {
    int TclResetCancellation(Tcl_Interp *interp, int force)
}

# NRE functions for "rogue" extensions to exploit NRE; they will need to
# include NRE.h too.
declare 238 {
    int TclNRInterpProc(ClientData clientData, Tcl_Interp *interp,
	    int objc, Tcl_Obj *const objv[])
}
declare 239 {
    int TclNRInterpProcCore(Tcl_Interp *interp, Tcl_Obj *procNameObj,
			    int skip, ProcErrorProc *errorProc)
}
declare 240 {
    int TclNRRunCallbacks(Tcl_Interp *interp, int result,
	      struct NRE_callback *rootPtr)
}
declare 241 {
    int TclNREvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags,
	    const CmdFrame *invoker, int word)
}
declare 242 {
    int TclNREvalObjv(Tcl_Interp *interp, int objc,
	      Tcl_Obj *const objv[], int flags, Command *cmdPtr)
}

# Tcl_Obj leak detection support.
declare 243 {
    void TclDbDumpActiveObjects(FILE *outFile)
}

# Functions to make things better for itcl
declare 244 {
    Tcl_HashTable *TclGetNamespaceChildTable(Tcl_Namespace *nsPtr)
}
declare 245 {
    Tcl_HashTable *TclGetNamespaceCommandTable(Tcl_Namespace *nsPtr)
}
declare 246 {
    int TclInitRewriteEnsemble(Tcl_Interp *interp, int numRemoved,
	    int numInserted, Tcl_Obj *const *objv)
}
declare 247 {
    void TclResetRewriteEnsemble(Tcl_Interp *interp, int isRootEnsemble)
}

declare 248 {
    int TclCopyChannel(Tcl_Interp *interp, Tcl_Channel inChan,
	    Tcl_Channel outChan, Tcl_WideInt toRead, Tcl_Obj *cmdPtr)
}

declare 249 {
    char *TclDoubleDigits(double dv, int ndigits, int flags,
			  int *decpt, int *signum, char **endPtr)
}
# TIP #285: Script cancellation support.
declare 250 {
    void TclSetSlaveCancelFlags(Tcl_Interp *interp, int flags, int force)
}

# Allow extensions for optimization
declare 251 {
    int TclRegisterLiteral(void *envPtr,
	    char *bytes, int length, int flags)
}

# Exporting of the internal API to variables.

declare 252 {
    Tcl_Obj *TclPtrGetVar(Tcl_Interp *interp, Tcl_Var varPtr,
	    Tcl_Var arrayPtr, Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr,
	    const int flags)
}
declare 253 {
    Tcl_Obj *TclPtrSetVar(Tcl_Interp *interp, Tcl_Var varPtr,
	    Tcl_Var arrayPtr, Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr,
	    Tcl_Obj *newValuePtr, const int flags)
}
declare 254 {
    Tcl_Obj *TclPtrIncrObjVar(Tcl_Interp *interp, Tcl_Var varPtr,
	    Tcl_Var arrayPtr, Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr,
	    Tcl_Obj *incrPtr, const int flags)
}
declare 255 {
    int	TclPtrObjMakeUpvar(Tcl_Interp *interp, Tcl_Var otherPtr,
	    Tcl_Obj *myNamePtr, int myFlags)
}
declare 256 {
    int	TclPtrUnsetVar(Tcl_Interp *interp, Tcl_Var varPtr, Tcl_Var arrayPtr,
	    Tcl_Obj *part1Ptr, Tcl_Obj *part2Ptr, const int flags)
}

##############################################################################

# Define the platform specific internal Tcl interface. These functions are
# only available on the designated platform.

interface tclIntPlat

################################
# Windows specific functions

declare 0 win {
    void TclWinConvertError(DWORD errCode)
}
declare 1 win {
    void TclWinConvertWSAError(DWORD errCode)
}
declare 2 win {
    struct servent *TclWinGetServByName(const char *nm,
	    const char *proto)
}
declare 3 win {
    int TclWinGetSockOpt(SOCKET s, int level, int optname,
	    char *optval, int *optlen)
}
declare 4 win {
    HINSTANCE TclWinGetTclInstance(void)
}
# new for 8.4.20+/8.5.12+ Cygwin only
declare 5 win {
    int TclUnixWaitForFile(int fd, int mask, int timeout)
}
# Removed in 8.1:
#  declare 5 win {
#      HINSTANCE TclWinLoadLibrary(char *name)
#  }
declare 6 win {
    unsigned short TclWinNToHS(unsigned short ns)
}
declare 7 win {
    int TclWinSetSockOpt(SOCKET s, int level, int optname,
	    const char *optval, int optlen)
}
declare 8 win {
    int TclpGetPid(Tcl_Pid pid)
}
declare 9 win {
    int TclWinGetPlatformId(void)
}
# new for 8.4.20+/8.5.12+ Cygwin only
declare 10 win {
    Tcl_DirEntry *TclpReaddir(TclDIR *dir)
}
# Removed in 8.3.1 (for Win32s only)
#declare 10 win {
#    int TclWinSynchSpawn(void *args, int type, void **trans, Tcl_Pid *pidPtr)
#}

# Pipe channel functions

declare 11 win {
    void TclGetAndDetachPids(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 12 win {
    int TclpCloseFile(TclFile file)
}
declare 13 win {
    Tcl_Channel TclpCreateCommandChannel(TclFile readFile,
	    TclFile writeFile, TclFile errorFile, int numPids, Tcl_Pid *pidPtr)
}
declare 14 win {
    int TclpCreatePipe(TclFile *readPipe, TclFile *writePipe)
}
declare 15 win {
    int TclpCreateProcess(Tcl_Interp *interp, int argc,
	    const char **argv, TclFile inputFile, TclFile outputFile,
	    TclFile errorFile, Tcl_Pid *pidPtr)
}
# new for 8.4.20+/8.5.12+ Cygwin only
declare 16 win {
    int TclpIsAtty(int fd)
}
# Signature changed in 8.1:
#  declare 16 win {
#      TclFile TclpCreateTempFile(char *contents, Tcl_DString *namePtr)
#  }
#  declare 17 win {
#      char *TclpGetTZName(void)
#  }
# new for 8.5.12+ Cygwin only
declare 17 win {
    int TclUnixCopyFile(const char *src, const char *dst,
	    const Tcl_StatBuf *statBufPtr, int dontCopyAtts)
}
declare 18 win {
    TclFile TclpMakeFile(Tcl_Channel channel, int direction)
}
declare 19 win {
    TclFile TclpOpenFile(const char *fname, int mode)
}
declare 20 win {
    void TclWinAddProcess(HANDLE hProcess, DWORD id)
}
# new for 8.4.20+/8.5.12+
declare 21 win {
    char *TclpInetNtoa(struct in_addr addr)
}
# removed permanently for 8.4
#declare 21 win {
#    void TclpAsyncMark(Tcl_AsyncHandler async)
#}

# Added in 8.1:
declare 22 win {
    TclFile TclpCreateTempFile(const char *contents)
}
# Removed in 8.6:
#declare 23 win {
#    char *TclpGetTZName(int isdst)
#}
declare 24 win {
    char *TclWinNoBackslash(char *path)
}
# replaced by generic TclGetPlatform
#declare 25 win {
#    TclPlatformType *TclWinGetPlatform(void)
#}
declare 26 win {
    void TclWinSetInterfaces(int wide)
}

# Added in Tcl 8.3.3 / 8.4

declare 27 win {
    void TclWinFlushDirtyChannels(void)
}

# Added in 8.4.2

declare 28 win {
    void TclWinResetInterfaces(void)
}

################################
# Unix specific functions

# Pipe channel functions

declare 0 unix {
    void TclGetAndDetachPids(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 1 unix {
    int TclpCloseFile(TclFile file)
}
declare 2 unix {
    Tcl_Channel TclpCreateCommandChannel(TclFile readFile,
	    TclFile writeFile, TclFile errorFile, int numPids, Tcl_Pid *pidPtr)
}
declare 3 unix {
    int TclpCreatePipe(TclFile *readPipe, TclFile *writePipe)
}
declare 4 unix {
    int TclpCreateProcess(Tcl_Interp *interp, int argc,
	    const char **argv, TclFile inputFile, TclFile outputFile,
	    TclFile errorFile, Tcl_Pid *pidPtr)
}
# Signature changed in 8.1:
#  declare 5 unix {
#      TclFile TclpCreateTempFile(char *contents, Tcl_DString *namePtr)
#  }
declare 6 unix {
    TclFile TclpMakeFile(Tcl_Channel channel, int direction)
}
declare 7 unix {
    TclFile TclpOpenFile(const char *fname, int mode)
}
declare 8 unix {
    int TclUnixWaitForFile(int fd, int mask, int timeout)
}

# Added in 8.1:

declare 9 unix {
    TclFile TclpCreateTempFile(const char *contents)
}

# Added in 8.4:

declare 10 unix {
    Tcl_DirEntry *TclpReaddir(TclDIR *dir)
}
# Slots 11 and 12 are forwarders for functions that were promoted to
# generic Stubs
declare 11 unix {
    struct tm *TclpLocaltime_unix(const time_t *clock)
}
declare 12 unix {
    struct tm *TclpGmtime_unix(const time_t *clock)
}
declare 13 unix {
    char *TclpInetNtoa(struct in_addr addr)
}

# Added in 8.5:

declare 14 unix {
    int TclUnixCopyFile(const char *src, const char *dst,
	    const Tcl_StatBuf *statBufPtr, int dontCopyAtts)
}

################################
# Mac OS X specific functions

declare 15 macosx {
    int TclMacOSXGetFileAttribute(Tcl_Interp *interp, int objIndex,
	    Tcl_Obj *fileName, Tcl_Obj **attributePtrPtr)
}
declare 16 macosx {
    int TclMacOSXSetFileAttribute(Tcl_Interp *interp, int objIndex,
	    Tcl_Obj *fileName, Tcl_Obj *attributePtr)
}
declare 17 macosx {
    int TclMacOSXCopyFileAttributes(const char *src, const char *dst,
	    const Tcl_StatBuf *statBufPtr)
}
declare 18 macosx {
    int TclMacOSXMatchType(Tcl_Interp *interp, const char *pathName,
	    const char *fileName, Tcl_StatBuf *statBufPtr,
	    Tcl_GlobTypeData *types)
}
declare 19 macosx {
    void TclMacOSXNotifierAddRunLoopMode(const void *runLoopMode)
}

declare 29 {win unix} {
    int TclWinCPUID(unsigned int index, unsigned int *regs)
}
# Added in 8.6; core of TclpOpenTemporaryFile
declare 30 {win unix} {
    int TclUnixOpenTemporaryFile(Tcl_Obj *dirObj, Tcl_Obj *basenameObj,
	    Tcl_Obj *extensionObj, Tcl_Obj *resultingNameObj)
}



# Local Variables:
# mode: tcl
# End:
