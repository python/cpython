# tcl.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Tcl library via the stubs table.
#	This file is used to generate the tclDecls.h, tclPlatDecls.h
#	and tclStubInit.c files.
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# Copyright (c) 2001, 2002 by Kevin B. Kenny.  All rights reserved.
# Copyright (c) 2007 Daniel A. Steffen <das@users.sourceforge.net>
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

library tcl

# Define the tcl interface with several sub interfaces:
#     tclPlat	 - platform specific public
#     tclInt	 - generic private
#     tclPlatInt - platform specific private

interface tcl
hooks {tclPlat tclInt tclIntPlat}
scspec EXTERN

# Declare each of the functions in the public Tcl interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 {
    int Tcl_PkgProvideEx(Tcl_Interp *interp, const char *name,
	    const char *version, const void *clientData)
}
declare 1 {
    CONST84_RETURN char *Tcl_PkgRequireEx(Tcl_Interp *interp,
	    const char *name, const char *version, int exact,
	    void *clientDataPtr)
}
declare 2 {
    TCL_NORETURN void Tcl_Panic(const char *format, ...)
}
declare 3 {
    char *Tcl_Alloc(unsigned int size)
}
declare 4 {
    void Tcl_Free(char *ptr)
}
declare 5 {
    char *Tcl_Realloc(char *ptr, unsigned int size)
}
declare 6 {
    char *Tcl_DbCkalloc(unsigned int size, const char *file, int line)
}
declare 7 {
    void Tcl_DbCkfree(char *ptr, const char *file, int line)
}
declare 8 {
    char *Tcl_DbCkrealloc(char *ptr, unsigned int size,
	    const char *file, int line)
}

# Tcl_CreateFileHandler and Tcl_DeleteFileHandler are only available on unix,
# but they are part of the old generic interface, so we include them here for
# compatibility reasons.

declare 9 unix {
    void Tcl_CreateFileHandler(int fd, int mask, Tcl_FileProc *proc,
	    ClientData clientData)
}
declare 10 unix {
    void Tcl_DeleteFileHandler(int fd)
}
declare 11 {
    void Tcl_SetTimer(const Tcl_Time *timePtr)
}
declare 12 {
    void Tcl_Sleep(int ms)
}
declare 13 {
    int Tcl_WaitForEvent(const Tcl_Time *timePtr)
}
declare 14 {
    int Tcl_AppendAllObjTypes(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 15 {
    void Tcl_AppendStringsToObj(Tcl_Obj *objPtr, ...)
}
declare 16 {
    void Tcl_AppendToObj(Tcl_Obj *objPtr, const char *bytes, int length)
}
declare 17 {
    Tcl_Obj *Tcl_ConcatObj(int objc, Tcl_Obj *const objv[])
}
declare 18 {
    int Tcl_ConvertToType(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    const Tcl_ObjType *typePtr)
}
declare 19 {
    void Tcl_DbDecrRefCount(Tcl_Obj *objPtr, const char *file, int line)
}
declare 20 {
    void Tcl_DbIncrRefCount(Tcl_Obj *objPtr, const char *file, int line)
}
declare 21 {
    int Tcl_DbIsShared(Tcl_Obj *objPtr, const char *file, int line)
}
declare 22 {
    Tcl_Obj *Tcl_DbNewBooleanObj(int boolValue, const char *file, int line)
}
declare 23 {
    Tcl_Obj *Tcl_DbNewByteArrayObj(const unsigned char *bytes, int length,
	    const char *file, int line)
}
declare 24 {
    Tcl_Obj *Tcl_DbNewDoubleObj(double doubleValue, const char *file,
	    int line)
}
declare 25 {
    Tcl_Obj *Tcl_DbNewListObj(int objc, Tcl_Obj *const *objv,
	    const char *file, int line)
}
declare 26 {
    Tcl_Obj *Tcl_DbNewLongObj(long longValue, const char *file, int line)
}
declare 27 {
    Tcl_Obj *Tcl_DbNewObj(const char *file, int line)
}
declare 28 {
    Tcl_Obj *Tcl_DbNewStringObj(const char *bytes, int length,
	    const char *file, int line)
}
declare 29 {
    Tcl_Obj *Tcl_DuplicateObj(Tcl_Obj *objPtr)
}
declare 30 {
    void TclFreeObj(Tcl_Obj *objPtr)
}
declare 31 {
    int Tcl_GetBoolean(Tcl_Interp *interp, const char *src, int *boolPtr)
}
declare 32 {
    int Tcl_GetBooleanFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    int *boolPtr)
}
declare 33 {
    unsigned char *Tcl_GetByteArrayFromObj(Tcl_Obj *objPtr, int *lengthPtr)
}
declare 34 {
    int Tcl_GetDouble(Tcl_Interp *interp, const char *src, double *doublePtr)
}
declare 35 {
    int Tcl_GetDoubleFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    double *doublePtr)
}
declare 36 {
    int Tcl_GetIndexFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    CONST84 char *const *tablePtr, const char *msg, int flags, int *indexPtr)
}
declare 37 {
    int Tcl_GetInt(Tcl_Interp *interp, const char *src, int *intPtr)
}
declare 38 {
    int Tcl_GetIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *intPtr)
}
declare 39 {
    int Tcl_GetLongFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *longPtr)
}
declare 40 {
    CONST86 Tcl_ObjType *Tcl_GetObjType(const char *typeName)
}
declare 41 {
    char *Tcl_GetStringFromObj(Tcl_Obj *objPtr, int *lengthPtr)
}
declare 42 {
    void Tcl_InvalidateStringRep(Tcl_Obj *objPtr)
}
declare 43 {
    int Tcl_ListObjAppendList(Tcl_Interp *interp, Tcl_Obj *listPtr,
	    Tcl_Obj *elemListPtr)
}
declare 44 {
    int Tcl_ListObjAppendElement(Tcl_Interp *interp, Tcl_Obj *listPtr,
	    Tcl_Obj *objPtr)
}
declare 45 {
    int Tcl_ListObjGetElements(Tcl_Interp *interp, Tcl_Obj *listPtr,
	    int *objcPtr, Tcl_Obj ***objvPtr)
}
declare 46 {
    int Tcl_ListObjIndex(Tcl_Interp *interp, Tcl_Obj *listPtr, int index,
	    Tcl_Obj **objPtrPtr)
}
declare 47 {
    int Tcl_ListObjLength(Tcl_Interp *interp, Tcl_Obj *listPtr,
	    int *lengthPtr)
}
declare 48 {
    int Tcl_ListObjReplace(Tcl_Interp *interp, Tcl_Obj *listPtr, int first,
	    int count, int objc, Tcl_Obj *const objv[])
}
declare 49 {
    Tcl_Obj *Tcl_NewBooleanObj(int boolValue)
}
declare 50 {
    Tcl_Obj *Tcl_NewByteArrayObj(const unsigned char *bytes, int length)
}
declare 51 {
    Tcl_Obj *Tcl_NewDoubleObj(double doubleValue)
}
declare 52 {
    Tcl_Obj *Tcl_NewIntObj(int intValue)
}
declare 53 {
    Tcl_Obj *Tcl_NewListObj(int objc, Tcl_Obj *const objv[])
}
declare 54 {
    Tcl_Obj *Tcl_NewLongObj(long longValue)
}
declare 55 {
    Tcl_Obj *Tcl_NewObj(void)
}
declare 56 {
    Tcl_Obj *Tcl_NewStringObj(const char *bytes, int length)
}
declare 57 {
    void Tcl_SetBooleanObj(Tcl_Obj *objPtr, int boolValue)
}
declare 58 {
    unsigned char *Tcl_SetByteArrayLength(Tcl_Obj *objPtr, int length)
}
declare 59 {
    void Tcl_SetByteArrayObj(Tcl_Obj *objPtr, const unsigned char *bytes,
	    int length)
}
declare 60 {
    void Tcl_SetDoubleObj(Tcl_Obj *objPtr, double doubleValue)
}
declare 61 {
    void Tcl_SetIntObj(Tcl_Obj *objPtr, int intValue)
}
declare 62 {
    void Tcl_SetListObj(Tcl_Obj *objPtr, int objc, Tcl_Obj *const objv[])
}
declare 63 {
    void Tcl_SetLongObj(Tcl_Obj *objPtr, long longValue)
}
declare 64 {
    void Tcl_SetObjLength(Tcl_Obj *objPtr, int length)
}
declare 65 {
    void Tcl_SetStringObj(Tcl_Obj *objPtr, const char *bytes, int length)
}
declare 66 {
    void Tcl_AddErrorInfo(Tcl_Interp *interp, const char *message)
}
declare 67 {
    void Tcl_AddObjErrorInfo(Tcl_Interp *interp, const char *message,
	    int length)
}
declare 68 {
    void Tcl_AllowExceptions(Tcl_Interp *interp)
}
declare 69 {
    void Tcl_AppendElement(Tcl_Interp *interp, const char *element)
}
declare 70 {
    void Tcl_AppendResult(Tcl_Interp *interp, ...)
}
declare 71 {
    Tcl_AsyncHandler Tcl_AsyncCreate(Tcl_AsyncProc *proc,
	    ClientData clientData)
}
declare 72 {
    void Tcl_AsyncDelete(Tcl_AsyncHandler async)
}
declare 73 {
    int Tcl_AsyncInvoke(Tcl_Interp *interp, int code)
}
declare 74 {
    void Tcl_AsyncMark(Tcl_AsyncHandler async)
}
declare 75 {
    int Tcl_AsyncReady(void)
}
declare 76 {
    void Tcl_BackgroundError(Tcl_Interp *interp)
}
declare 77 {
    char Tcl_Backslash(const char *src, int *readPtr)
}
declare 78 {
    int Tcl_BadChannelOption(Tcl_Interp *interp, const char *optionName,
	    const char *optionList)
}
declare 79 {
    void Tcl_CallWhenDeleted(Tcl_Interp *interp, Tcl_InterpDeleteProc *proc,
	    ClientData clientData)
}
declare 80 {
    void Tcl_CancelIdleCall(Tcl_IdleProc *idleProc, ClientData clientData)
}
declare 81 {
    int Tcl_Close(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 82 {
    int Tcl_CommandComplete(const char *cmd)
}
declare 83 {
    char *Tcl_Concat(int argc, CONST84 char *const *argv)
}
declare 84 {
    int Tcl_ConvertElement(const char *src, char *dst, int flags)
}
declare 85 {
    int Tcl_ConvertCountedElement(const char *src, int length, char *dst,
	    int flags)
}
declare 86 {
    int Tcl_CreateAlias(Tcl_Interp *slave, const char *slaveCmd,
	    Tcl_Interp *target, const char *targetCmd, int argc,
	    CONST84 char *const *argv)
}
declare 87 {
    int Tcl_CreateAliasObj(Tcl_Interp *slave, const char *slaveCmd,
	    Tcl_Interp *target, const char *targetCmd, int objc,
	    Tcl_Obj *const objv[])
}
declare 88 {
    Tcl_Channel Tcl_CreateChannel(const Tcl_ChannelType *typePtr,
	    const char *chanName, ClientData instanceData, int mask)
}
declare 89 {
    void Tcl_CreateChannelHandler(Tcl_Channel chan, int mask,
	    Tcl_ChannelProc *proc, ClientData clientData)
}
declare 90 {
    void Tcl_CreateCloseHandler(Tcl_Channel chan, Tcl_CloseProc *proc,
	    ClientData clientData)
}
declare 91 {
    Tcl_Command Tcl_CreateCommand(Tcl_Interp *interp, const char *cmdName,
	    Tcl_CmdProc *proc, ClientData clientData,
	    Tcl_CmdDeleteProc *deleteProc)
}
declare 92 {
    void Tcl_CreateEventSource(Tcl_EventSetupProc *setupProc,
	    Tcl_EventCheckProc *checkProc, ClientData clientData)
}
declare 93 {
    void Tcl_CreateExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 94 {
    Tcl_Interp *Tcl_CreateInterp(void)
}
declare 95 {
    void Tcl_CreateMathFunc(Tcl_Interp *interp, const char *name,
	    int numArgs, Tcl_ValueType *argTypes,
	    Tcl_MathProc *proc, ClientData clientData)
}
declare 96 {
    Tcl_Command Tcl_CreateObjCommand(Tcl_Interp *interp,
	    const char *cmdName,
	    Tcl_ObjCmdProc *proc, ClientData clientData,
	    Tcl_CmdDeleteProc *deleteProc)
}
declare 97 {
    Tcl_Interp *Tcl_CreateSlave(Tcl_Interp *interp, const char *slaveName,
	    int isSafe)
}
declare 98 {
    Tcl_TimerToken Tcl_CreateTimerHandler(int milliseconds,
	    Tcl_TimerProc *proc, ClientData clientData)
}
declare 99 {
    Tcl_Trace Tcl_CreateTrace(Tcl_Interp *interp, int level,
	    Tcl_CmdTraceProc *proc, ClientData clientData)
}
declare 100 {
    void Tcl_DeleteAssocData(Tcl_Interp *interp, const char *name)
}
declare 101 {
    void Tcl_DeleteChannelHandler(Tcl_Channel chan, Tcl_ChannelProc *proc,
	    ClientData clientData)
}
declare 102 {
    void Tcl_DeleteCloseHandler(Tcl_Channel chan, Tcl_CloseProc *proc,
	    ClientData clientData)
}
declare 103 {
    int Tcl_DeleteCommand(Tcl_Interp *interp, const char *cmdName)
}
declare 104 {
    int Tcl_DeleteCommandFromToken(Tcl_Interp *interp, Tcl_Command command)
}
declare 105 {
    void Tcl_DeleteEvents(Tcl_EventDeleteProc *proc, ClientData clientData)
}
declare 106 {
    void Tcl_DeleteEventSource(Tcl_EventSetupProc *setupProc,
	    Tcl_EventCheckProc *checkProc, ClientData clientData)
}
declare 107 {
    void Tcl_DeleteExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 108 {
    void Tcl_DeleteHashEntry(Tcl_HashEntry *entryPtr)
}
declare 109 {
    void Tcl_DeleteHashTable(Tcl_HashTable *tablePtr)
}
declare 110 {
    void Tcl_DeleteInterp(Tcl_Interp *interp)
}
declare 111 {
    void Tcl_DetachPids(int numPids, Tcl_Pid *pidPtr)
}
declare 112 {
    void Tcl_DeleteTimerHandler(Tcl_TimerToken token)
}
declare 113 {
    void Tcl_DeleteTrace(Tcl_Interp *interp, Tcl_Trace trace)
}
declare 114 {
    void Tcl_DontCallWhenDeleted(Tcl_Interp *interp,
	    Tcl_InterpDeleteProc *proc, ClientData clientData)
}
declare 115 {
    int Tcl_DoOneEvent(int flags)
}
declare 116 {
    void Tcl_DoWhenIdle(Tcl_IdleProc *proc, ClientData clientData)
}
declare 117 {
    char *Tcl_DStringAppend(Tcl_DString *dsPtr, const char *bytes, int length)
}
declare 118 {
    char *Tcl_DStringAppendElement(Tcl_DString *dsPtr, const char *element)
}
declare 119 {
    void Tcl_DStringEndSublist(Tcl_DString *dsPtr)
}
declare 120 {
    void Tcl_DStringFree(Tcl_DString *dsPtr)
}
declare 121 {
    void Tcl_DStringGetResult(Tcl_Interp *interp, Tcl_DString *dsPtr)
}
declare 122 {
    void Tcl_DStringInit(Tcl_DString *dsPtr)
}
declare 123 {
    void Tcl_DStringResult(Tcl_Interp *interp, Tcl_DString *dsPtr)
}
declare 124 {
    void Tcl_DStringSetLength(Tcl_DString *dsPtr, int length)
}
declare 125 {
    void Tcl_DStringStartSublist(Tcl_DString *dsPtr)
}
declare 126 {
    int Tcl_Eof(Tcl_Channel chan)
}
declare 127 {
    CONST84_RETURN char *Tcl_ErrnoId(void)
}
declare 128 {
    CONST84_RETURN char *Tcl_ErrnoMsg(int err)
}
declare 129 {
    int Tcl_Eval(Tcl_Interp *interp, const char *script)
}
declare 130 {
    int Tcl_EvalFile(Tcl_Interp *interp, const char *fileName)
}
declare 131 {
    int Tcl_EvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 132 {
    void Tcl_EventuallyFree(ClientData clientData, Tcl_FreeProc *freeProc)
}
declare 133 {
    TCL_NORETURN void Tcl_Exit(int status)
}
declare 134 {
    int Tcl_ExposeCommand(Tcl_Interp *interp, const char *hiddenCmdToken,
	    const char *cmdName)
}
declare 135 {
    int Tcl_ExprBoolean(Tcl_Interp *interp, const char *expr, int *ptr)
}
declare 136 {
    int Tcl_ExprBooleanObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int *ptr)
}
declare 137 {
    int Tcl_ExprDouble(Tcl_Interp *interp, const char *expr, double *ptr)
}
declare 138 {
    int Tcl_ExprDoubleObj(Tcl_Interp *interp, Tcl_Obj *objPtr, double *ptr)
}
declare 139 {
    int Tcl_ExprLong(Tcl_Interp *interp, const char *expr, long *ptr)
}
declare 140 {
    int Tcl_ExprLongObj(Tcl_Interp *interp, Tcl_Obj *objPtr, long *ptr)
}
declare 141 {
    int Tcl_ExprObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    Tcl_Obj **resultPtrPtr)
}
declare 142 {
    int Tcl_ExprString(Tcl_Interp *interp, const char *expr)
}
declare 143 {
    void Tcl_Finalize(void)
}
declare 144 {
    void Tcl_FindExecutable(const char *argv0)
}
declare 145 {
    Tcl_HashEntry *Tcl_FirstHashEntry(Tcl_HashTable *tablePtr,
	    Tcl_HashSearch *searchPtr)
}
declare 146 {
    int Tcl_Flush(Tcl_Channel chan)
}
declare 147 {
    void Tcl_FreeResult(Tcl_Interp *interp)
}
declare 148 {
    int Tcl_GetAlias(Tcl_Interp *interp, const char *slaveCmd,
	    Tcl_Interp **targetInterpPtr, CONST84 char **targetCmdPtr,
	    int *argcPtr, CONST84 char ***argvPtr)
}
declare 149 {
    int Tcl_GetAliasObj(Tcl_Interp *interp, const char *slaveCmd,
	    Tcl_Interp **targetInterpPtr, CONST84 char **targetCmdPtr,
	    int *objcPtr, Tcl_Obj ***objv)
}
declare 150 {
    ClientData Tcl_GetAssocData(Tcl_Interp *interp, const char *name,
	    Tcl_InterpDeleteProc **procPtr)
}
declare 151 {
    Tcl_Channel Tcl_GetChannel(Tcl_Interp *interp, const char *chanName,
	    int *modePtr)
}
declare 152 {
    int Tcl_GetChannelBufferSize(Tcl_Channel chan)
}
declare 153 {
    int Tcl_GetChannelHandle(Tcl_Channel chan, int direction,
	    ClientData *handlePtr)
}
declare 154 {
    ClientData Tcl_GetChannelInstanceData(Tcl_Channel chan)
}
declare 155 {
    int Tcl_GetChannelMode(Tcl_Channel chan)
}
declare 156 {
    CONST84_RETURN char *Tcl_GetChannelName(Tcl_Channel chan)
}
declare 157 {
    int Tcl_GetChannelOption(Tcl_Interp *interp, Tcl_Channel chan,
	    const char *optionName, Tcl_DString *dsPtr)
}
declare 158 {
    CONST86 Tcl_ChannelType *Tcl_GetChannelType(Tcl_Channel chan)
}
declare 159 {
    int Tcl_GetCommandInfo(Tcl_Interp *interp, const char *cmdName,
	    Tcl_CmdInfo *infoPtr)
}
declare 160 {
    CONST84_RETURN char *Tcl_GetCommandName(Tcl_Interp *interp,
	    Tcl_Command command)
}
declare 161 {
    int Tcl_GetErrno(void)
}
declare 162 {
    CONST84_RETURN char *Tcl_GetHostName(void)
}
declare 163 {
    int Tcl_GetInterpPath(Tcl_Interp *askInterp, Tcl_Interp *slaveInterp)
}
declare 164 {
    Tcl_Interp *Tcl_GetMaster(Tcl_Interp *interp)
}
declare 165 {
    const char *Tcl_GetNameOfExecutable(void)
}
declare 166 {
    Tcl_Obj *Tcl_GetObjResult(Tcl_Interp *interp)
}

# Tcl_GetOpenFile is only available on unix, but it is a part of the old
# generic interface, so we inlcude it here for compatibility reasons.

declare 167 unix {
    int Tcl_GetOpenFile(Tcl_Interp *interp, const char *chanID, int forWriting,
	    int checkUsage, ClientData *filePtr)
}
# Obsolete.  Should now use Tcl_FSGetPathType which is objectified
# and therefore usually faster.
declare 168 {
    Tcl_PathType Tcl_GetPathType(const char *path)
}
declare 169 {
    int Tcl_Gets(Tcl_Channel chan, Tcl_DString *dsPtr)
}
declare 170 {
    int Tcl_GetsObj(Tcl_Channel chan, Tcl_Obj *objPtr)
}
declare 171 {
    int Tcl_GetServiceMode(void)
}
declare 172 {
    Tcl_Interp *Tcl_GetSlave(Tcl_Interp *interp, const char *slaveName)
}
declare 173 {
    Tcl_Channel Tcl_GetStdChannel(int type)
}
declare 174 {
    CONST84_RETURN char *Tcl_GetStringResult(Tcl_Interp *interp)
}
declare 175 {
    CONST84_RETURN char *Tcl_GetVar(Tcl_Interp *interp, const char *varName,
	    int flags)
}
declare 176 {
    CONST84_RETURN char *Tcl_GetVar2(Tcl_Interp *interp, const char *part1,
	    const char *part2, int flags)
}
declare 177 {
    int Tcl_GlobalEval(Tcl_Interp *interp, const char *command)
}
declare 178 {
    int Tcl_GlobalEvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 179 {
    int Tcl_HideCommand(Tcl_Interp *interp, const char *cmdName,
	    const char *hiddenCmdToken)
}
declare 180 {
    int Tcl_Init(Tcl_Interp *interp)
}
declare 181 {
    void Tcl_InitHashTable(Tcl_HashTable *tablePtr, int keyType)
}
declare 182 {
    int Tcl_InputBlocked(Tcl_Channel chan)
}
declare 183 {
    int Tcl_InputBuffered(Tcl_Channel chan)
}
declare 184 {
    int Tcl_InterpDeleted(Tcl_Interp *interp)
}
declare 185 {
    int Tcl_IsSafe(Tcl_Interp *interp)
}
# Obsolete, use Tcl_FSJoinPath
declare 186 {
    char *Tcl_JoinPath(int argc, CONST84 char *const *argv,
	    Tcl_DString *resultPtr)
}
declare 187 {
    int Tcl_LinkVar(Tcl_Interp *interp, const char *varName, char *addr,
	    int type)
}

# This slot is reserved for use by the plus patch:
#  declare 188 {
#	Tcl_MainLoop
#  }

declare 189 {
    Tcl_Channel Tcl_MakeFileChannel(ClientData handle, int mode)
}
declare 190 {
    int Tcl_MakeSafe(Tcl_Interp *interp)
}
declare 191 {
    Tcl_Channel Tcl_MakeTcpClientChannel(ClientData tcpSocket)
}
declare 192 {
    char *Tcl_Merge(int argc, CONST84 char *const *argv)
}
declare 193 {
    Tcl_HashEntry *Tcl_NextHashEntry(Tcl_HashSearch *searchPtr)
}
declare 194 {
    void Tcl_NotifyChannel(Tcl_Channel channel, int mask)
}
declare 195 {
    Tcl_Obj *Tcl_ObjGetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
	    Tcl_Obj *part2Ptr, int flags)
}
declare 196 {
    Tcl_Obj *Tcl_ObjSetVar2(Tcl_Interp *interp, Tcl_Obj *part1Ptr,
	    Tcl_Obj *part2Ptr, Tcl_Obj *newValuePtr, int flags)
}
declare 197 {
    Tcl_Channel Tcl_OpenCommandChannel(Tcl_Interp *interp, int argc,
	    CONST84 char **argv, int flags)
}
# This is obsolete, use Tcl_FSOpenFileChannel
declare 198 {
    Tcl_Channel Tcl_OpenFileChannel(Tcl_Interp *interp, const char *fileName,
	    const char *modeString, int permissions)
}
declare 199 {
    Tcl_Channel Tcl_OpenTcpClient(Tcl_Interp *interp, int port,
	    const char *address, const char *myaddr, int myport, int async)
}
declare 200 {
    Tcl_Channel Tcl_OpenTcpServer(Tcl_Interp *interp, int port,
	    const char *host, Tcl_TcpAcceptProc *acceptProc,
	    ClientData callbackData)
}
declare 201 {
    void Tcl_Preserve(ClientData data)
}
declare 202 {
    void Tcl_PrintDouble(Tcl_Interp *interp, double value, char *dst)
}
declare 203 {
    int Tcl_PutEnv(const char *assignment)
}
declare 204 {
    CONST84_RETURN char *Tcl_PosixError(Tcl_Interp *interp)
}
declare 205 {
    void Tcl_QueueEvent(Tcl_Event *evPtr, Tcl_QueuePosition position)
}
declare 206 {
    int Tcl_Read(Tcl_Channel chan, char *bufPtr, int toRead)
}
declare 207 {
    void Tcl_ReapDetachedProcs(void)
}
declare 208 {
    int Tcl_RecordAndEval(Tcl_Interp *interp, const char *cmd, int flags)
}
declare 209 {
    int Tcl_RecordAndEvalObj(Tcl_Interp *interp, Tcl_Obj *cmdPtr, int flags)
}
declare 210 {
    void Tcl_RegisterChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 211 {
    void Tcl_RegisterObjType(const Tcl_ObjType *typePtr)
}
declare 212 {
    Tcl_RegExp Tcl_RegExpCompile(Tcl_Interp *interp, const char *pattern)
}
declare 213 {
    int Tcl_RegExpExec(Tcl_Interp *interp, Tcl_RegExp regexp,
	    const char *text, const char *start)
}
declare 214 {
    int Tcl_RegExpMatch(Tcl_Interp *interp, const char *text,
	    const char *pattern)
}
declare 215 {
    void Tcl_RegExpRange(Tcl_RegExp regexp, int index,
	    CONST84 char **startPtr, CONST84 char **endPtr)
}
declare 216 {
    void Tcl_Release(ClientData clientData)
}
declare 217 {
    void Tcl_ResetResult(Tcl_Interp *interp)
}
declare 218 {
    int Tcl_ScanElement(const char *src, int *flagPtr)
}
declare 219 {
    int Tcl_ScanCountedElement(const char *src, int length, int *flagPtr)
}
# Obsolete
declare 220 {
    int Tcl_SeekOld(Tcl_Channel chan, int offset, int mode)
}
declare 221 {
    int Tcl_ServiceAll(void)
}
declare 222 {
    int Tcl_ServiceEvent(int flags)
}
declare 223 {
    void Tcl_SetAssocData(Tcl_Interp *interp, const char *name,
	    Tcl_InterpDeleteProc *proc, ClientData clientData)
}
declare 224 {
    void Tcl_SetChannelBufferSize(Tcl_Channel chan, int sz)
}
declare 225 {
    int Tcl_SetChannelOption(Tcl_Interp *interp, Tcl_Channel chan,
	    const char *optionName, const char *newValue)
}
declare 226 {
    int Tcl_SetCommandInfo(Tcl_Interp *interp, const char *cmdName,
	    const Tcl_CmdInfo *infoPtr)
}
declare 227 {
    void Tcl_SetErrno(int err)
}
declare 228 {
    void Tcl_SetErrorCode(Tcl_Interp *interp, ...)
}
declare 229 {
    void Tcl_SetMaxBlockTime(const Tcl_Time *timePtr)
}
declare 230 {
    void Tcl_SetPanicProc(TCL_NORETURN1 Tcl_PanicProc *panicProc)
}
declare 231 {
    int Tcl_SetRecursionLimit(Tcl_Interp *interp, int depth)
}
declare 232 {
    void Tcl_SetResult(Tcl_Interp *interp, char *result,
	    Tcl_FreeProc *freeProc)
}
declare 233 {
    int Tcl_SetServiceMode(int mode)
}
declare 234 {
    void Tcl_SetObjErrorCode(Tcl_Interp *interp, Tcl_Obj *errorObjPtr)
}
declare 235 {
    void Tcl_SetObjResult(Tcl_Interp *interp, Tcl_Obj *resultObjPtr)
}
declare 236 {
    void Tcl_SetStdChannel(Tcl_Channel channel, int type)
}
declare 237 {
    CONST84_RETURN char *Tcl_SetVar(Tcl_Interp *interp, const char *varName,
	    const char *newValue, int flags)
}
declare 238 {
    CONST84_RETURN char *Tcl_SetVar2(Tcl_Interp *interp, const char *part1,
	    const char *part2, const char *newValue, int flags)
}
declare 239 {
    CONST84_RETURN char *Tcl_SignalId(int sig)
}
declare 240 {
    CONST84_RETURN char *Tcl_SignalMsg(int sig)
}
declare 241 {
    void Tcl_SourceRCFile(Tcl_Interp *interp)
}
declare 242 {
    int Tcl_SplitList(Tcl_Interp *interp, const char *listStr, int *argcPtr,
	    CONST84 char ***argvPtr)
}
# Obsolete, use Tcl_FSSplitPath
declare 243 {
    void Tcl_SplitPath(const char *path, int *argcPtr, CONST84 char ***argvPtr)
}
declare 244 {
    void Tcl_StaticPackage(Tcl_Interp *interp, const char *pkgName,
	    Tcl_PackageInitProc *initProc, Tcl_PackageInitProc *safeInitProc)
}
declare 245 {
    int Tcl_StringMatch(const char *str, const char *pattern)
}
# Obsolete
declare 246 {
    int Tcl_TellOld(Tcl_Channel chan)
}
declare 247 {
    int Tcl_TraceVar(Tcl_Interp *interp, const char *varName, int flags,
	    Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 248 {
    int Tcl_TraceVar2(Tcl_Interp *interp, const char *part1, const char *part2,
	    int flags, Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 249 {
    char *Tcl_TranslateFileName(Tcl_Interp *interp, const char *name,
	    Tcl_DString *bufferPtr)
}
declare 250 {
    int Tcl_Ungets(Tcl_Channel chan, const char *str, int len, int atHead)
}
declare 251 {
    void Tcl_UnlinkVar(Tcl_Interp *interp, const char *varName)
}
declare 252 {
    int Tcl_UnregisterChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 253 {
    int Tcl_UnsetVar(Tcl_Interp *interp, const char *varName, int flags)
}
declare 254 {
    int Tcl_UnsetVar2(Tcl_Interp *interp, const char *part1, const char *part2,
	    int flags)
}
declare 255 {
    void Tcl_UntraceVar(Tcl_Interp *interp, const char *varName, int flags,
	    Tcl_VarTraceProc *proc, ClientData clientData)
}
declare 256 {
    void Tcl_UntraceVar2(Tcl_Interp *interp, const char *part1,
	    const char *part2, int flags, Tcl_VarTraceProc *proc,
	    ClientData clientData)
}
declare 257 {
    void Tcl_UpdateLinkedVar(Tcl_Interp *interp, const char *varName)
}
declare 258 {
    int Tcl_UpVar(Tcl_Interp *interp, const char *frameName,
	    const char *varName, const char *localName, int flags)
}
declare 259 {
    int Tcl_UpVar2(Tcl_Interp *interp, const char *frameName, const char *part1,
	    const char *part2, const char *localName, int flags)
}
declare 260 {
    int Tcl_VarEval(Tcl_Interp *interp, ...)
}
declare 261 {
    ClientData Tcl_VarTraceInfo(Tcl_Interp *interp, const char *varName,
	    int flags, Tcl_VarTraceProc *procPtr, ClientData prevClientData)
}
declare 262 {
    ClientData Tcl_VarTraceInfo2(Tcl_Interp *interp, const char *part1,
	    const char *part2, int flags, Tcl_VarTraceProc *procPtr,
	    ClientData prevClientData)
}
declare 263 {
    int Tcl_Write(Tcl_Channel chan, const char *s, int slen)
}
declare 264 {
    void Tcl_WrongNumArgs(Tcl_Interp *interp, int objc,
	    Tcl_Obj *const objv[], const char *message)
}
declare 265 {
    int Tcl_DumpActiveMemory(const char *fileName)
}
declare 266 {
    void Tcl_ValidateAllMemory(const char *file, int line)
}
declare 267 {
    void Tcl_AppendResultVA(Tcl_Interp *interp, va_list argList)
}
declare 268 {
    void Tcl_AppendStringsToObjVA(Tcl_Obj *objPtr, va_list argList)
}
declare 269 {
    char *Tcl_HashStats(Tcl_HashTable *tablePtr)
}
declare 270 {
    CONST84_RETURN char *Tcl_ParseVar(Tcl_Interp *interp, const char *start,
	    CONST84 char **termPtr)
}
declare 271 {
    CONST84_RETURN char *Tcl_PkgPresent(Tcl_Interp *interp, const char *name,
	    const char *version, int exact)
}
declare 272 {
    CONST84_RETURN char *Tcl_PkgPresentEx(Tcl_Interp *interp,
	    const char *name, const char *version, int exact,
	    void *clientDataPtr)
}
declare 273 {
    int Tcl_PkgProvide(Tcl_Interp *interp, const char *name,
	    const char *version)
}
# TIP #268: The internally used new Require function is in slot 573.
declare 274 {
    CONST84_RETURN char *Tcl_PkgRequire(Tcl_Interp *interp, const char *name,
	    const char *version, int exact)
}
declare 275 {
    void Tcl_SetErrorCodeVA(Tcl_Interp *interp, va_list argList)
}
declare 276 {
    int  Tcl_VarEvalVA(Tcl_Interp *interp, va_list argList)
}
declare 277 {
    Tcl_Pid Tcl_WaitPid(Tcl_Pid pid, int *statPtr, int options)
}
declare 278 {
    TCL_NORETURN void Tcl_PanicVA(const char *format, va_list argList)
}
declare 279 {
    void Tcl_GetVersion(int *major, int *minor, int *patchLevel, int *type)
}
declare 280 {
    void Tcl_InitMemory(Tcl_Interp *interp)
}

# Andreas Kupries <a.kupries@westend.com>, 03/21/1999
# "Trf-Patch for filtering channels"
#
# C-Level API for (un)stacking of channels. This allows the introduction
# of filtering channels with relatively little changes to the core.
# This patch was created in cooperation with Jan Nijtmans j.nijtmans@chello.nl
# and is therefore part of his plus-patches too.
#
# It would have been possible to place the following definitions according
# to the alphabetical order used elsewhere in this file, but I decided
# against that to ease the maintenance of the patch across new tcl versions
# (patch usually has no problems to integrate the patch file for the last
# version into the new one).

declare 281 {
    Tcl_Channel Tcl_StackChannel(Tcl_Interp *interp,
	    const Tcl_ChannelType *typePtr, ClientData instanceData,
	    int mask, Tcl_Channel prevChan)
}
declare 282 {
    int Tcl_UnstackChannel(Tcl_Interp *interp, Tcl_Channel chan)
}
declare 283 {
    Tcl_Channel Tcl_GetStackedChannel(Tcl_Channel chan)
}

# 284 was reserved, but added in 8.4a2
declare 284 {
    void Tcl_SetMainLoop(Tcl_MainLoopProc *proc)
}

# Reserved for future use (8.0.x vs. 8.1)
#  declare 285 {
#  }

# Added in 8.1:

declare 286 {
    void Tcl_AppendObjToObj(Tcl_Obj *objPtr, Tcl_Obj *appendObjPtr)
}
declare 287 {
    Tcl_Encoding Tcl_CreateEncoding(const Tcl_EncodingType *typePtr)
}
declare 288 {
    void Tcl_CreateThreadExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 289 {
    void Tcl_DeleteThreadExitHandler(Tcl_ExitProc *proc, ClientData clientData)
}
declare 290 {
    void Tcl_DiscardResult(Tcl_SavedResult *statePtr)
}
declare 291 {
    int Tcl_EvalEx(Tcl_Interp *interp, const char *script, int numBytes,
	    int flags)
}
declare 292 {
    int Tcl_EvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[],
	    int flags)
}
declare 293 {
    int Tcl_EvalObjEx(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags)
}
declare 294 {
    void Tcl_ExitThread(int status)
}
declare 295 {
    int Tcl_ExternalToUtf(Tcl_Interp *interp, Tcl_Encoding encoding,
	    const char *src, int srcLen, int flags,
	    Tcl_EncodingState *statePtr, char *dst, int dstLen,
	    int *srcReadPtr, int *dstWrotePtr, int *dstCharsPtr)
}
declare 296 {
    char *Tcl_ExternalToUtfDString(Tcl_Encoding encoding,
	    const char *src, int srcLen, Tcl_DString *dsPtr)
}
declare 297 {
    void Tcl_FinalizeThread(void)
}
declare 298 {
    void Tcl_FinalizeNotifier(ClientData clientData)
}
declare 299 {
    void Tcl_FreeEncoding(Tcl_Encoding encoding)
}
declare 300 {
    Tcl_ThreadId Tcl_GetCurrentThread(void)
}
declare 301 {
    Tcl_Encoding Tcl_GetEncoding(Tcl_Interp *interp, const char *name)
}
declare 302 {
    CONST84_RETURN char *Tcl_GetEncodingName(Tcl_Encoding encoding)
}
declare 303 {
    void Tcl_GetEncodingNames(Tcl_Interp *interp)
}
declare 304 {
    int Tcl_GetIndexFromObjStruct(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    const void *tablePtr, int offset, const char *msg, int flags,
	    int *indexPtr)
}
declare 305 {
    void *Tcl_GetThreadData(Tcl_ThreadDataKey *keyPtr, int size)
}
declare 306 {
    Tcl_Obj *Tcl_GetVar2Ex(Tcl_Interp *interp, const char *part1,
	    const char *part2, int flags)
}
declare 307 {
    ClientData Tcl_InitNotifier(void)
}
declare 308 {
    void Tcl_MutexLock(Tcl_Mutex *mutexPtr)
}
declare 309 {
    void Tcl_MutexUnlock(Tcl_Mutex *mutexPtr)
}
declare 310 {
    void Tcl_ConditionNotify(Tcl_Condition *condPtr)
}
declare 311 {
    void Tcl_ConditionWait(Tcl_Condition *condPtr, Tcl_Mutex *mutexPtr,
	    const Tcl_Time *timePtr)
}
declare 312 {
    int Tcl_NumUtfChars(const char *src, int length)
}
declare 313 {
    int Tcl_ReadChars(Tcl_Channel channel, Tcl_Obj *objPtr, int charsToRead,
	    int appendFlag)
}
declare 314 {
    void Tcl_RestoreResult(Tcl_Interp *interp, Tcl_SavedResult *statePtr)
}
declare 315 {
    void Tcl_SaveResult(Tcl_Interp *interp, Tcl_SavedResult *statePtr)
}
declare 316 {
    int Tcl_SetSystemEncoding(Tcl_Interp *interp, const char *name)
}
declare 317 {
    Tcl_Obj *Tcl_SetVar2Ex(Tcl_Interp *interp, const char *part1,
	    const char *part2, Tcl_Obj *newValuePtr, int flags)
}
declare 318 {
    void Tcl_ThreadAlert(Tcl_ThreadId threadId)
}
declare 319 {
    void Tcl_ThreadQueueEvent(Tcl_ThreadId threadId, Tcl_Event *evPtr,
	    Tcl_QueuePosition position)
}
declare 320 {
    Tcl_UniChar Tcl_UniCharAtIndex(const char *src, int index)
}
declare 321 {
    Tcl_UniChar Tcl_UniCharToLower(int ch)
}
declare 322 {
    Tcl_UniChar Tcl_UniCharToTitle(int ch)
}
declare 323 {
    Tcl_UniChar Tcl_UniCharToUpper(int ch)
}
declare 324 {
    int Tcl_UniCharToUtf(int ch, char *buf)
}
declare 325 {
    CONST84_RETURN char *Tcl_UtfAtIndex(const char *src, int index)
}
declare 326 {
    int Tcl_UtfCharComplete(const char *src, int length)
}
declare 327 {
    int Tcl_UtfBackslash(const char *src, int *readPtr, char *dst)
}
declare 328 {
    CONST84_RETURN char *Tcl_UtfFindFirst(const char *src, int ch)
}
declare 329 {
    CONST84_RETURN char *Tcl_UtfFindLast(const char *src, int ch)
}
declare 330 {
    CONST84_RETURN char *Tcl_UtfNext(const char *src)
}
declare 331 {
    CONST84_RETURN char *Tcl_UtfPrev(const char *src, const char *start)
}
declare 332 {
    int Tcl_UtfToExternal(Tcl_Interp *interp, Tcl_Encoding encoding,
	    const char *src, int srcLen, int flags,
	    Tcl_EncodingState *statePtr, char *dst, int dstLen,
	    int *srcReadPtr, int *dstWrotePtr, int *dstCharsPtr)
}
declare 333 {
    char *Tcl_UtfToExternalDString(Tcl_Encoding encoding,
	    const char *src, int srcLen, Tcl_DString *dsPtr)
}
declare 334 {
    int Tcl_UtfToLower(char *src)
}
declare 335 {
    int Tcl_UtfToTitle(char *src)
}
declare 336 {
    int Tcl_UtfToUniChar(const char *src, Tcl_UniChar *chPtr)
}
declare 337 {
    int Tcl_UtfToUpper(char *src)
}
declare 338 {
    int Tcl_WriteChars(Tcl_Channel chan, const char *src, int srcLen)
}
declare 339 {
    int Tcl_WriteObj(Tcl_Channel chan, Tcl_Obj *objPtr)
}
declare 340 {
    char *Tcl_GetString(Tcl_Obj *objPtr)
}
declare 341 {
    CONST84_RETURN char *Tcl_GetDefaultEncodingDir(void)
}
declare 342 {
    void Tcl_SetDefaultEncodingDir(const char *path)
}
declare 343 {
    void Tcl_AlertNotifier(ClientData clientData)
}
declare 344 {
    void Tcl_ServiceModeHook(int mode)
}
declare 345 {
    int Tcl_UniCharIsAlnum(int ch)
}
declare 346 {
    int Tcl_UniCharIsAlpha(int ch)
}
declare 347 {
    int Tcl_UniCharIsDigit(int ch)
}
declare 348 {
    int Tcl_UniCharIsLower(int ch)
}
declare 349 {
    int Tcl_UniCharIsSpace(int ch)
}
declare 350 {
    int Tcl_UniCharIsUpper(int ch)
}
declare 351 {
    int Tcl_UniCharIsWordChar(int ch)
}
declare 352 {
    int Tcl_UniCharLen(const Tcl_UniChar *uniStr)
}
declare 353 {
    int Tcl_UniCharNcmp(const Tcl_UniChar *ucs, const Tcl_UniChar *uct,
	    unsigned long numChars)
}
declare 354 {
    char *Tcl_UniCharToUtfDString(const Tcl_UniChar *uniStr,
	    int uniLength, Tcl_DString *dsPtr)
}
declare 355 {
    Tcl_UniChar *Tcl_UtfToUniCharDString(const char *src,
	    int length, Tcl_DString *dsPtr)
}
declare 356 {
    Tcl_RegExp Tcl_GetRegExpFromObj(Tcl_Interp *interp, Tcl_Obj *patObj,
	    int flags)
}
declare 357 {
    Tcl_Obj *Tcl_EvalTokens(Tcl_Interp *interp, Tcl_Token *tokenPtr,
	    int count)
}
declare 358 {
    void Tcl_FreeParse(Tcl_Parse *parsePtr)
}
declare 359 {
    void Tcl_LogCommandInfo(Tcl_Interp *interp, const char *script,
	    const char *command, int length)
}
declare 360 {
    int Tcl_ParseBraces(Tcl_Interp *interp, const char *start, int numBytes,
	    Tcl_Parse *parsePtr, int append, CONST84 char **termPtr)
}
declare 361 {
    int Tcl_ParseCommand(Tcl_Interp *interp, const char *start, int numBytes,
	    int nested, Tcl_Parse *parsePtr)
}
declare 362 {
    int Tcl_ParseExpr(Tcl_Interp *interp, const char *start, int numBytes,
	    Tcl_Parse *parsePtr)
}
declare 363 {
    int Tcl_ParseQuotedString(Tcl_Interp *interp, const char *start,
	    int numBytes, Tcl_Parse *parsePtr, int append,
	    CONST84 char **termPtr)
}
declare 364 {
    int Tcl_ParseVarName(Tcl_Interp *interp, const char *start, int numBytes,
	    Tcl_Parse *parsePtr, int append)
}
# These 4 functions are obsolete, use Tcl_FSGetCwd, Tcl_FSChdir,
# Tcl_FSAccess and Tcl_FSStat
declare 365 {
    char *Tcl_GetCwd(Tcl_Interp *interp, Tcl_DString *cwdPtr)
}
declare 366 {
   int Tcl_Chdir(const char *dirName)
}
declare 367 {
   int Tcl_Access(const char *path, int mode)
}
declare 368 {
    int Tcl_Stat(const char *path, struct stat *bufPtr)
}
declare 369 {
    int Tcl_UtfNcmp(const char *s1, const char *s2, unsigned long n)
}
declare 370 {
    int Tcl_UtfNcasecmp(const char *s1, const char *s2, unsigned long n)
}
declare 371 {
    int Tcl_StringCaseMatch(const char *str, const char *pattern, int nocase)
}
declare 372 {
    int Tcl_UniCharIsControl(int ch)
}
declare 373 {
    int Tcl_UniCharIsGraph(int ch)
}
declare 374 {
    int Tcl_UniCharIsPrint(int ch)
}
declare 375 {
    int Tcl_UniCharIsPunct(int ch)
}
declare 376 {
    int Tcl_RegExpExecObj(Tcl_Interp *interp, Tcl_RegExp regexp,
	    Tcl_Obj *textObj, int offset, int nmatches, int flags)
}
declare 377 {
    void Tcl_RegExpGetInfo(Tcl_RegExp regexp, Tcl_RegExpInfo *infoPtr)
}
declare 378 {
    Tcl_Obj *Tcl_NewUnicodeObj(const Tcl_UniChar *unicode, int numChars)
}
declare 379 {
    void Tcl_SetUnicodeObj(Tcl_Obj *objPtr, const Tcl_UniChar *unicode,
	    int numChars)
}
declare 380 {
    int Tcl_GetCharLength(Tcl_Obj *objPtr)
}
declare 381 {
    Tcl_UniChar Tcl_GetUniChar(Tcl_Obj *objPtr, int index)
}
declare 382 {
    Tcl_UniChar *Tcl_GetUnicode(Tcl_Obj *objPtr)
}
declare 383 {
    Tcl_Obj *Tcl_GetRange(Tcl_Obj *objPtr, int first, int last)
}
declare 384 {
    void Tcl_AppendUnicodeToObj(Tcl_Obj *objPtr, const Tcl_UniChar *unicode,
	    int length)
}
declare 385 {
    int Tcl_RegExpMatchObj(Tcl_Interp *interp, Tcl_Obj *textObj,
	    Tcl_Obj *patternObj)
}
declare 386 {
    void Tcl_SetNotifier(Tcl_NotifierProcs *notifierProcPtr)
}
declare 387 {
    Tcl_Mutex *Tcl_GetAllocMutex(void)
}
declare 388 {
    int Tcl_GetChannelNames(Tcl_Interp *interp)
}
declare 389 {
    int Tcl_GetChannelNamesEx(Tcl_Interp *interp, const char *pattern)
}
declare 390 {
    int Tcl_ProcObjCmd(ClientData clientData, Tcl_Interp *interp,
	    int objc, Tcl_Obj *const objv[])
}
declare 391 {
    void Tcl_ConditionFinalize(Tcl_Condition *condPtr)
}
declare 392 {
    void Tcl_MutexFinalize(Tcl_Mutex *mutex)
}
declare 393 {
    int Tcl_CreateThread(Tcl_ThreadId *idPtr, Tcl_ThreadCreateProc *proc,
	    ClientData clientData, int stackSize, int flags)
}

# Introduced in 8.3.2
declare 394 {
    int Tcl_ReadRaw(Tcl_Channel chan, char *dst, int bytesToRead)
}
declare 395 {
    int Tcl_WriteRaw(Tcl_Channel chan, const char *src, int srcLen)
}
declare 396 {
    Tcl_Channel Tcl_GetTopChannel(Tcl_Channel chan)
}
declare 397 {
    int Tcl_ChannelBuffered(Tcl_Channel chan)
}
declare 398 {
    CONST84_RETURN char *Tcl_ChannelName(const Tcl_ChannelType *chanTypePtr)
}
declare 399 {
    Tcl_ChannelTypeVersion Tcl_ChannelVersion(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 400 {
    Tcl_DriverBlockModeProc *Tcl_ChannelBlockModeProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 401 {
    Tcl_DriverCloseProc *Tcl_ChannelCloseProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 402 {
    Tcl_DriverClose2Proc *Tcl_ChannelClose2Proc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 403 {
    Tcl_DriverInputProc *Tcl_ChannelInputProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 404 {
    Tcl_DriverOutputProc *Tcl_ChannelOutputProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 405 {
    Tcl_DriverSeekProc *Tcl_ChannelSeekProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 406 {
    Tcl_DriverSetOptionProc *Tcl_ChannelSetOptionProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 407 {
    Tcl_DriverGetOptionProc *Tcl_ChannelGetOptionProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 408 {
    Tcl_DriverWatchProc *Tcl_ChannelWatchProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 409 {
    Tcl_DriverGetHandleProc *Tcl_ChannelGetHandleProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 410 {
    Tcl_DriverFlushProc *Tcl_ChannelFlushProc(
	    const Tcl_ChannelType *chanTypePtr)
}
declare 411 {
    Tcl_DriverHandlerProc *Tcl_ChannelHandlerProc(
	    const Tcl_ChannelType *chanTypePtr)
}

# Introduced in 8.4a2
declare 412 {
    int Tcl_JoinThread(Tcl_ThreadId threadId, int *result)
}
declare 413 {
    int Tcl_IsChannelShared(Tcl_Channel channel)
}
declare 414 {
    int Tcl_IsChannelRegistered(Tcl_Interp *interp, Tcl_Channel channel)
}
declare 415 {
    void Tcl_CutChannel(Tcl_Channel channel)
}
declare 416 {
    void Tcl_SpliceChannel(Tcl_Channel channel)
}
declare 417 {
    void Tcl_ClearChannelHandlers(Tcl_Channel channel)
}
declare 418 {
    int Tcl_IsChannelExisting(const char *channelName)
}
declare 419 {
    int Tcl_UniCharNcasecmp(const Tcl_UniChar *ucs, const Tcl_UniChar *uct,
	    unsigned long numChars)
}
declare 420 {
    int Tcl_UniCharCaseMatch(const Tcl_UniChar *uniStr,
	    const Tcl_UniChar *uniPattern, int nocase)
}
declare 421 {
    Tcl_HashEntry *Tcl_FindHashEntry(Tcl_HashTable *tablePtr, const void *key)
}
declare 422 {
    Tcl_HashEntry *Tcl_CreateHashEntry(Tcl_HashTable *tablePtr,
	    const void *key, int *newPtr)
}
declare 423 {
    void Tcl_InitCustomHashTable(Tcl_HashTable *tablePtr, int keyType,
	    const Tcl_HashKeyType *typePtr)
}
declare 424 {
    void Tcl_InitObjHashTable(Tcl_HashTable *tablePtr)
}
declare 425 {
    ClientData Tcl_CommandTraceInfo(Tcl_Interp *interp, const char *varName,
	    int flags, Tcl_CommandTraceProc *procPtr,
	    ClientData prevClientData)
}
declare 426 {
    int Tcl_TraceCommand(Tcl_Interp *interp, const char *varName, int flags,
	    Tcl_CommandTraceProc *proc, ClientData clientData)
}
declare 427 {
    void Tcl_UntraceCommand(Tcl_Interp *interp, const char *varName,
	    int flags, Tcl_CommandTraceProc *proc, ClientData clientData)
}
declare 428 {
    char *Tcl_AttemptAlloc(unsigned int size)
}
declare 429 {
    char *Tcl_AttemptDbCkalloc(unsigned int size, const char *file, int line)
}
declare 430 {
    char *Tcl_AttemptRealloc(char *ptr, unsigned int size)
}
declare 431 {
    char *Tcl_AttemptDbCkrealloc(char *ptr, unsigned int size,
	    const char *file, int line)
}
declare 432 {
    int Tcl_AttemptSetObjLength(Tcl_Obj *objPtr, int length)
}

# TIP#10 (thread-aware channels) akupries
declare 433 {
    Tcl_ThreadId Tcl_GetChannelThread(Tcl_Channel channel)
}

# introduced in 8.4a3
declare 434 {
    Tcl_UniChar *Tcl_GetUnicodeFromObj(Tcl_Obj *objPtr, int *lengthPtr)
}

# TIP#15 (math function introspection) dkf
declare 435 {
    int Tcl_GetMathFuncInfo(Tcl_Interp *interp, const char *name,
	    int *numArgsPtr, Tcl_ValueType **argTypesPtr,
	    Tcl_MathProc **procPtr, ClientData *clientDataPtr)
}
declare 436 {
    Tcl_Obj *Tcl_ListMathFuncs(Tcl_Interp *interp, const char *pattern)
}

# TIP#36 (better access to 'subst') dkf
declare 437 {
    Tcl_Obj *Tcl_SubstObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags)
}

# TIP#17 (virtual filesystem layer) vdarley
declare 438 {
    int Tcl_DetachChannel(Tcl_Interp *interp, Tcl_Channel channel)
}
declare 439 {
    int Tcl_IsStandardChannel(Tcl_Channel channel)
}
declare 440 {
    int	Tcl_FSCopyFile(Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr)
}
declare 441 {
    int	Tcl_FSCopyDirectory(Tcl_Obj *srcPathPtr,
	    Tcl_Obj *destPathPtr, Tcl_Obj **errorPtr)
}
declare 442 {
    int	Tcl_FSCreateDirectory(Tcl_Obj *pathPtr)
}
declare 443 {
    int	Tcl_FSDeleteFile(Tcl_Obj *pathPtr)
}
declare 444 {
    int	Tcl_FSLoadFile(Tcl_Interp *interp, Tcl_Obj *pathPtr, const char *sym1,
	    const char *sym2, Tcl_PackageInitProc **proc1Ptr,
	    Tcl_PackageInitProc **proc2Ptr, Tcl_LoadHandle *handlePtr,
	    Tcl_FSUnloadFileProc **unloadProcPtr)
}
declare 445 {
    int	Tcl_FSMatchInDirectory(Tcl_Interp *interp, Tcl_Obj *result,
	    Tcl_Obj *pathPtr, const char *pattern, Tcl_GlobTypeData *types)
}
declare 446 {
    Tcl_Obj *Tcl_FSLink(Tcl_Obj *pathPtr, Tcl_Obj *toPtr, int linkAction)
}
declare 447 {
    int Tcl_FSRemoveDirectory(Tcl_Obj *pathPtr,
	    int recursive, Tcl_Obj **errorPtr)
}
declare 448 {
    int	Tcl_FSRenameFile(Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr)
}
declare 449 {
    int	Tcl_FSLstat(Tcl_Obj *pathPtr, Tcl_StatBuf *buf)
}
declare 450 {
    int Tcl_FSUtime(Tcl_Obj *pathPtr, struct utimbuf *tval)
}
declare 451 {
    int Tcl_FSFileAttrsGet(Tcl_Interp *interp,
	    int index, Tcl_Obj *pathPtr, Tcl_Obj **objPtrRef)
}
declare 452 {
    int Tcl_FSFileAttrsSet(Tcl_Interp *interp,
	    int index, Tcl_Obj *pathPtr, Tcl_Obj *objPtr)
}
declare 453 {
    const char *CONST86 *Tcl_FSFileAttrStrings(Tcl_Obj *pathPtr,
	    Tcl_Obj **objPtrRef)
}
declare 454 {
    int Tcl_FSStat(Tcl_Obj *pathPtr, Tcl_StatBuf *buf)
}
declare 455 {
    int Tcl_FSAccess(Tcl_Obj *pathPtr, int mode)
}
declare 456 {
    Tcl_Channel Tcl_FSOpenFileChannel(Tcl_Interp *interp, Tcl_Obj *pathPtr,
	    const char *modeString, int permissions)
}
declare 457 {
    Tcl_Obj *Tcl_FSGetCwd(Tcl_Interp *interp)
}
declare 458 {
    int Tcl_FSChdir(Tcl_Obj *pathPtr)
}
declare 459 {
    int Tcl_FSConvertToPathType(Tcl_Interp *interp, Tcl_Obj *pathPtr)
}
declare 460 {
    Tcl_Obj *Tcl_FSJoinPath(Tcl_Obj *listObj, int elements)
}
declare 461 {
    Tcl_Obj *Tcl_FSSplitPath(Tcl_Obj *pathPtr, int *lenPtr)
}
declare 462 {
    int Tcl_FSEqualPaths(Tcl_Obj *firstPtr, Tcl_Obj *secondPtr)
}
declare 463 {
    Tcl_Obj *Tcl_FSGetNormalizedPath(Tcl_Interp *interp, Tcl_Obj *pathPtr)
}
declare 464 {
    Tcl_Obj *Tcl_FSJoinToPath(Tcl_Obj *pathPtr, int objc,
	    Tcl_Obj *const objv[])
}
declare 465 {
    ClientData Tcl_FSGetInternalRep(Tcl_Obj *pathPtr,
	    const Tcl_Filesystem *fsPtr)
}
declare 466 {
    Tcl_Obj *Tcl_FSGetTranslatedPath(Tcl_Interp *interp, Tcl_Obj *pathPtr)
}
declare 467 {
    int Tcl_FSEvalFile(Tcl_Interp *interp, Tcl_Obj *fileName)
}
declare 468 {
    Tcl_Obj *Tcl_FSNewNativePath(const Tcl_Filesystem *fromFilesystem,
	    ClientData clientData)
}
declare 469 {
    const void *Tcl_FSGetNativePath(Tcl_Obj *pathPtr)
}
declare 470 {
    Tcl_Obj *Tcl_FSFileSystemInfo(Tcl_Obj *pathPtr)
}
declare 471 {
    Tcl_Obj *Tcl_FSPathSeparator(Tcl_Obj *pathPtr)
}
declare 472 {
    Tcl_Obj *Tcl_FSListVolumes(void)
}
declare 473 {
    int Tcl_FSRegister(ClientData clientData, const Tcl_Filesystem *fsPtr)
}
declare 474 {
    int Tcl_FSUnregister(const Tcl_Filesystem *fsPtr)
}
declare 475 {
    ClientData Tcl_FSData(const Tcl_Filesystem *fsPtr)
}
declare 476 {
    const char *Tcl_FSGetTranslatedStringPath(Tcl_Interp *interp,
	    Tcl_Obj *pathPtr)
}
declare 477 {
    CONST86 Tcl_Filesystem *Tcl_FSGetFileSystemForPath(Tcl_Obj *pathPtr)
}
declare 478 {
    Tcl_PathType Tcl_FSGetPathType(Tcl_Obj *pathPtr)
}

# TIP#49 (detection of output buffering) akupries
declare 479 {
    int Tcl_OutputBuffered(Tcl_Channel chan)
}
declare 480 {
    void Tcl_FSMountsChanged(const Tcl_Filesystem *fsPtr)
}

# TIP#56 (evaluate a parsed script) msofer
declare 481 {
    int Tcl_EvalTokensStandard(Tcl_Interp *interp, Tcl_Token *tokenPtr,
	    int count)
}

# TIP#73 (access to current time) kbk
declare 482 {
    void Tcl_GetTime(Tcl_Time *timeBuf)
}

# TIP#32 (object-enabled traces) kbk
declare 483 {
    Tcl_Trace Tcl_CreateObjTrace(Tcl_Interp *interp, int level, int flags,
	    Tcl_CmdObjTraceProc *objProc, ClientData clientData,
	    Tcl_CmdObjTraceDeleteProc *delProc)
}
declare 484 {
    int Tcl_GetCommandInfoFromToken(Tcl_Command token, Tcl_CmdInfo *infoPtr)
}
declare 485 {
    int Tcl_SetCommandInfoFromToken(Tcl_Command token,
	    const Tcl_CmdInfo *infoPtr)
}

### New functions on 64-bit dev branch ###
# TIP#72 (64-bit values) dkf
declare 486 {
    Tcl_Obj *Tcl_DbNewWideIntObj(Tcl_WideInt wideValue,
	    const char *file, int line)
}
declare 487 {
    int Tcl_GetWideIntFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    Tcl_WideInt *widePtr)
}
declare 488 {
    Tcl_Obj *Tcl_NewWideIntObj(Tcl_WideInt wideValue)
}
declare 489 {
    void Tcl_SetWideIntObj(Tcl_Obj *objPtr, Tcl_WideInt wideValue)
}
declare 490 {
    Tcl_StatBuf *Tcl_AllocStatBuf(void)
}
declare 491 {
    Tcl_WideInt Tcl_Seek(Tcl_Channel chan, Tcl_WideInt offset, int mode)
}
declare 492 {
    Tcl_WideInt Tcl_Tell(Tcl_Channel chan)
}

# TIP#91 (back-compat enhancements for channels) dkf
declare 493 {
    Tcl_DriverWideSeekProc *Tcl_ChannelWideSeekProc(
	    const Tcl_ChannelType *chanTypePtr)
}

# ----- BASELINE -- FOR -- 8.4.0 ----- #

# TIP#111 (dictionaries) dkf
declare 494 {
    int Tcl_DictObjPut(Tcl_Interp *interp, Tcl_Obj *dictPtr,
	    Tcl_Obj *keyPtr, Tcl_Obj *valuePtr)
}
declare 495 {
    int Tcl_DictObjGet(Tcl_Interp *interp, Tcl_Obj *dictPtr, Tcl_Obj *keyPtr,
	    Tcl_Obj **valuePtrPtr)
}
declare 496 {
    int Tcl_DictObjRemove(Tcl_Interp *interp, Tcl_Obj *dictPtr,
	    Tcl_Obj *keyPtr)
}
declare 497 {
    int Tcl_DictObjSize(Tcl_Interp *interp, Tcl_Obj *dictPtr, int *sizePtr)
}
declare 498 {
    int Tcl_DictObjFirst(Tcl_Interp *interp, Tcl_Obj *dictPtr,
	    Tcl_DictSearch *searchPtr,
	    Tcl_Obj **keyPtrPtr, Tcl_Obj **valuePtrPtr, int *donePtr)
}
declare 499 {
    void Tcl_DictObjNext(Tcl_DictSearch *searchPtr,
	    Tcl_Obj **keyPtrPtr, Tcl_Obj **valuePtrPtr, int *donePtr)
}
declare 500 {
    void Tcl_DictObjDone(Tcl_DictSearch *searchPtr)
}
declare 501 {
    int Tcl_DictObjPutKeyList(Tcl_Interp *interp, Tcl_Obj *dictPtr,
	    int keyc, Tcl_Obj *const *keyv, Tcl_Obj *valuePtr)
}
declare 502 {
    int Tcl_DictObjRemoveKeyList(Tcl_Interp *interp, Tcl_Obj *dictPtr,
	    int keyc, Tcl_Obj *const *keyv)
}
declare 503 {
    Tcl_Obj *Tcl_NewDictObj(void)
}
declare 504 {
    Tcl_Obj *Tcl_DbNewDictObj(const char *file, int line)
}

# TIP#59 (configuration reporting) akupries
declare 505 {
    void Tcl_RegisterConfig(Tcl_Interp *interp, const char *pkgName,
	    const Tcl_Config *configuration, const char *valEncoding)
}

# TIP #139 (partial exposure of namespace API - transferred from tclInt.decls)
# dkf, API by Brent Welch?
declare 506 {
    Tcl_Namespace *Tcl_CreateNamespace(Tcl_Interp *interp, const char *name,
	    ClientData clientData, Tcl_NamespaceDeleteProc *deleteProc)
}
declare 507 {
    void Tcl_DeleteNamespace(Tcl_Namespace *nsPtr)
}
declare 508 {
    int Tcl_AppendExportList(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    Tcl_Obj *objPtr)
}
declare 509 {
    int Tcl_Export(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern, int resetListFirst)
}
declare 510 {
    int Tcl_Import(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern, int allowOverwrite)
}
declare 511 {
    int Tcl_ForgetImport(Tcl_Interp *interp, Tcl_Namespace *nsPtr,
	    const char *pattern)
}
declare 512 {
    Tcl_Namespace *Tcl_GetCurrentNamespace(Tcl_Interp *interp)
}
declare 513 {
    Tcl_Namespace *Tcl_GetGlobalNamespace(Tcl_Interp *interp)
}
declare 514 {
    Tcl_Namespace *Tcl_FindNamespace(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 515 {
    Tcl_Command Tcl_FindCommand(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *contextNsPtr, int flags)
}
declare 516 {
    Tcl_Command Tcl_GetCommandFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 517 {
    void Tcl_GetCommandFullName(Tcl_Interp *interp, Tcl_Command command,
	    Tcl_Obj *objPtr)
}

# TIP#137 (encoding-aware source command) dgp for Anton Kovalenko
declare 518 {
    int Tcl_FSEvalFileEx(Tcl_Interp *interp, Tcl_Obj *fileName,
	    const char *encodingName)
}

# TIP#121 (exit handler) dkf for Joe Mistachkin
declare 519 {
    Tcl_ExitProc *Tcl_SetExitProc(TCL_NORETURN1 Tcl_ExitProc *proc)
}

# TIP#143 (resource limits) dkf
declare 520 {
    void Tcl_LimitAddHandler(Tcl_Interp *interp, int type,
	    Tcl_LimitHandlerProc *handlerProc, ClientData clientData,
	    Tcl_LimitHandlerDeleteProc *deleteProc)
}
declare 521 {
    void Tcl_LimitRemoveHandler(Tcl_Interp *interp, int type,
	    Tcl_LimitHandlerProc *handlerProc, ClientData clientData)
}
declare 522 {
    int Tcl_LimitReady(Tcl_Interp *interp)
}
declare 523 {
    int Tcl_LimitCheck(Tcl_Interp *interp)
}
declare 524 {
    int Tcl_LimitExceeded(Tcl_Interp *interp)
}
declare 525 {
    void Tcl_LimitSetCommands(Tcl_Interp *interp, int commandLimit)
}
declare 526 {
    void Tcl_LimitSetTime(Tcl_Interp *interp, Tcl_Time *timeLimitPtr)
}
declare 527 {
    void Tcl_LimitSetGranularity(Tcl_Interp *interp, int type, int granularity)
}
declare 528 {
    int Tcl_LimitTypeEnabled(Tcl_Interp *interp, int type)
}
declare 529 {
    int Tcl_LimitTypeExceeded(Tcl_Interp *interp, int type)
}
declare 530 {
    void Tcl_LimitTypeSet(Tcl_Interp *interp, int type)
}
declare 531 {
    void Tcl_LimitTypeReset(Tcl_Interp *interp, int type)
}
declare 532 {
    int Tcl_LimitGetCommands(Tcl_Interp *interp)
}
declare 533 {
    void Tcl_LimitGetTime(Tcl_Interp *interp, Tcl_Time *timeLimitPtr)
}
declare 534 {
    int Tcl_LimitGetGranularity(Tcl_Interp *interp, int type)
}

# TIP#226 (interpreter result state management) dgp
declare 535 {
    Tcl_InterpState Tcl_SaveInterpState(Tcl_Interp *interp, int status)
}
declare 536 {
    int Tcl_RestoreInterpState(Tcl_Interp *interp, Tcl_InterpState state)
}
declare 537 {
    void Tcl_DiscardInterpState(Tcl_InterpState state)
}

# TIP#227 (return options interface) dgp
declare 538 {
    int Tcl_SetReturnOptions(Tcl_Interp *interp, Tcl_Obj *options)
}
declare 539 {
    Tcl_Obj *Tcl_GetReturnOptions(Tcl_Interp *interp, int result)
}

# TIP#235 (ensembles) dkf
declare 540 {
    int Tcl_IsEnsemble(Tcl_Command token)
}
declare 541 {
    Tcl_Command Tcl_CreateEnsemble(Tcl_Interp *interp, const char *name,
	    Tcl_Namespace *namespacePtr, int flags)
}
declare 542 {
    Tcl_Command Tcl_FindEnsemble(Tcl_Interp *interp, Tcl_Obj *cmdNameObj,
	    int flags)
}
declare 543 {
    int Tcl_SetEnsembleSubcommandList(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj *subcmdList)
}
declare 544 {
    int Tcl_SetEnsembleMappingDict(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj *mapDict)
}
declare 545 {
    int Tcl_SetEnsembleUnknownHandler(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj *unknownList)
}
declare 546 {
    int Tcl_SetEnsembleFlags(Tcl_Interp *interp, Tcl_Command token, int flags)
}
declare 547 {
    int Tcl_GetEnsembleSubcommandList(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj **subcmdListPtr)
}
declare 548 {
    int Tcl_GetEnsembleMappingDict(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj **mapDictPtr)
}
declare 549 {
    int Tcl_GetEnsembleUnknownHandler(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj **unknownListPtr)
}
declare 550 {
    int Tcl_GetEnsembleFlags(Tcl_Interp *interp, Tcl_Command token,
	    int *flagsPtr)
}
declare 551 {
    int Tcl_GetEnsembleNamespace(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Namespace **namespacePtrPtr)
}

# TIP#233 (virtualized time) akupries
declare 552 {
    void Tcl_SetTimeProc(Tcl_GetTimeProc *getProc,
	    Tcl_ScaleTimeProc *scaleProc,
	    ClientData clientData)
}
declare 553 {
    void Tcl_QueryTimeProc(Tcl_GetTimeProc **getProc,
	    Tcl_ScaleTimeProc **scaleProc,
	    ClientData *clientData)
}

# TIP#218 (driver thread actions) davygrvy/akupries ChannelType ver 4
declare 554 {
    Tcl_DriverThreadActionProc *Tcl_ChannelThreadActionProc(
	    const Tcl_ChannelType *chanTypePtr)
}

# TIP#237 (arbitrary-precision integers) kbk
declare 555 {
    Tcl_Obj *Tcl_NewBignumObj(mp_int *value)
}
declare 556 {
    Tcl_Obj *Tcl_DbNewBignumObj(mp_int *value, const char *file, int line)
}
declare 557 {
    void Tcl_SetBignumObj(Tcl_Obj *obj, mp_int *value)
}
declare 558 {
    int Tcl_GetBignumFromObj(Tcl_Interp *interp, Tcl_Obj *obj, mp_int *value)
}
declare 559 {
    int Tcl_TakeBignumFromObj(Tcl_Interp *interp, Tcl_Obj *obj, mp_int *value)
}

# TIP #208 ('chan' command) jeffh
declare 560 {
    int Tcl_TruncateChannel(Tcl_Channel chan, Tcl_WideInt length)
}
declare 561 {
    Tcl_DriverTruncateProc *Tcl_ChannelTruncateProc(
	    const Tcl_ChannelType *chanTypePtr)
}

# TIP#219 (channel reflection api) akupries
declare 562 {
    void Tcl_SetChannelErrorInterp(Tcl_Interp *interp, Tcl_Obj *msg)
}
declare 563 {
    void Tcl_GetChannelErrorInterp(Tcl_Interp *interp, Tcl_Obj **msg)
}
declare 564 {
    void Tcl_SetChannelError(Tcl_Channel chan, Tcl_Obj *msg)
}
declare 565 {
    void Tcl_GetChannelError(Tcl_Channel chan, Tcl_Obj **msg)
}

# TIP #237 (additional conversion functions for bignum support) kbk/dgp
declare 566 {
    int Tcl_InitBignumFromDouble(Tcl_Interp *interp, double initval,
	    mp_int *toInit)
}

# TIP#181 (namespace unknown command) dgp for Neil Madden
declare 567 {
    Tcl_Obj *Tcl_GetNamespaceUnknownHandler(Tcl_Interp *interp,
	    Tcl_Namespace *nsPtr)
}
declare 568 {
    int Tcl_SetNamespaceUnknownHandler(Tcl_Interp *interp,
	    Tcl_Namespace *nsPtr, Tcl_Obj *handlerPtr)
}

# TIP#258 (enhanced interface for encodings) dgp
declare 569 {
    int Tcl_GetEncodingFromObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    Tcl_Encoding *encodingPtr)
}
declare 570 {
    Tcl_Obj *Tcl_GetEncodingSearchPath(void)
}
declare 571 {
    int Tcl_SetEncodingSearchPath(Tcl_Obj *searchPath)
}
declare 572 {
    const char *Tcl_GetEncodingNameFromEnvironment(Tcl_DString *bufPtr)
}

# TIP#268 (extended version numbers and requirements) akupries
declare 573 {
    int Tcl_PkgRequireProc(Tcl_Interp *interp, const char *name,
	    int objc, Tcl_Obj *const objv[], void *clientDataPtr)
}

# TIP#270 (utility C routines for string formatting) dgp
declare 574 {
    void Tcl_AppendObjToErrorInfo(Tcl_Interp *interp, Tcl_Obj *objPtr)
}
declare 575 {
    void Tcl_AppendLimitedToObj(Tcl_Obj *objPtr, const char *bytes, int length,
	    int limit, const char *ellipsis)
}
declare 576 {
    Tcl_Obj *Tcl_Format(Tcl_Interp *interp, const char *format, int objc,
	    Tcl_Obj *const objv[])
}
declare 577 {
    int Tcl_AppendFormatToObj(Tcl_Interp *interp, Tcl_Obj *objPtr,
	    const char *format, int objc, Tcl_Obj *const objv[])
}
declare 578 {
    Tcl_Obj *Tcl_ObjPrintf(const char *format, ...)
}
declare 579 {
    void Tcl_AppendPrintfToObj(Tcl_Obj *objPtr, const char *format, ...)
}

# ----- BASELINE -- FOR -- 8.5.0 ----- #

# TIP #285 (script cancellation support) jmistachkin
declare 580 {
    int Tcl_CancelEval(Tcl_Interp *interp, Tcl_Obj *resultObjPtr,
	    ClientData clientData, int flags)
}
declare 581 {
    int Tcl_Canceled(Tcl_Interp *interp, int flags)
}

# TIP#304 (chan pipe) aferrieux
declare 582 {
    int Tcl_CreatePipe(Tcl_Interp  *interp, Tcl_Channel *rchan,
	    Tcl_Channel *wchan, int flags)
}

# TIP #322 (NRE public interface) msofer
declare 583 {
    Tcl_Command Tcl_NRCreateCommand(Tcl_Interp *interp,
	    const char *cmdName, Tcl_ObjCmdProc *proc,
	    Tcl_ObjCmdProc *nreProc, ClientData clientData,
	    Tcl_CmdDeleteProc *deleteProc)
}
declare 584 {
    int Tcl_NREvalObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags)
}
declare 585 {
    int Tcl_NREvalObjv(Tcl_Interp *interp, int objc, Tcl_Obj *const objv[],
	    int flags)
}
declare 586 {
    int Tcl_NRCmdSwap(Tcl_Interp *interp, Tcl_Command cmd, int objc,
	    Tcl_Obj *const objv[], int flags)
}
declare 587 {
    void Tcl_NRAddCallback(Tcl_Interp *interp, Tcl_NRPostProc *postProcPtr,
	    ClientData data0, ClientData data1, ClientData data2,
	    ClientData data3)
}
# For use by NR extenders, to have a simple way to also provide a (required!)
# classic objProc
declare 588 {
    int Tcl_NRCallObjProc(Tcl_Interp *interp, Tcl_ObjCmdProc *objProc,
	    ClientData clientData, int objc, Tcl_Obj *const objv[])
}

# TIP#316 (Tcl_StatBuf reader functions) dkf
declare 589 {
    unsigned Tcl_GetFSDeviceFromStat(const Tcl_StatBuf *statPtr)
}
declare 590 {
    unsigned Tcl_GetFSInodeFromStat(const Tcl_StatBuf *statPtr)
}
declare 591 {
    unsigned Tcl_GetModeFromStat(const Tcl_StatBuf *statPtr)
}
declare 592 {
    int Tcl_GetLinkCountFromStat(const Tcl_StatBuf *statPtr)
}
declare 593 {
    int Tcl_GetUserIdFromStat(const Tcl_StatBuf *statPtr)
}
declare 594 {
    int Tcl_GetGroupIdFromStat(const Tcl_StatBuf *statPtr)
}
declare 595 {
    int Tcl_GetDeviceTypeFromStat(const Tcl_StatBuf *statPtr)
}
declare 596 {
    Tcl_WideInt Tcl_GetAccessTimeFromStat(const Tcl_StatBuf *statPtr)
}
declare 597 {
    Tcl_WideInt Tcl_GetModificationTimeFromStat(const Tcl_StatBuf *statPtr)
}
declare 598 {
    Tcl_WideInt Tcl_GetChangeTimeFromStat(const Tcl_StatBuf *statPtr)
}
declare 599 {
    Tcl_WideUInt Tcl_GetSizeFromStat(const Tcl_StatBuf *statPtr)
}
declare 600 {
    Tcl_WideUInt Tcl_GetBlocksFromStat(const Tcl_StatBuf *statPtr)
}
declare 601 {
    unsigned Tcl_GetBlockSizeFromStat(const Tcl_StatBuf *statPtr)
}

# TIP#314 (ensembles with parameters) dkf for Lars Hellstr"om
declare 602 {
    int Tcl_SetEnsembleParameterList(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj *paramList)
}
declare 603 {
    int Tcl_GetEnsembleParameterList(Tcl_Interp *interp, Tcl_Command token,
	    Tcl_Obj **paramListPtr)
}

# TIP#265 (option parser) dkf for Sam Bromley
declare 604 {
    int Tcl_ParseArgsObjv(Tcl_Interp *interp, const Tcl_ArgvInfo *argTable,
	    int *objcPtr, Tcl_Obj *const *objv, Tcl_Obj ***remObjv)
}

# TIP#336 (manipulate the error line) dgp
declare 605 {
    int Tcl_GetErrorLine(Tcl_Interp *interp)
}
declare 606 {
    void Tcl_SetErrorLine(Tcl_Interp *interp, int lineNum)
}

# TIP#307 (move results between interpreters) dkf
declare 607 {
    void Tcl_TransferResult(Tcl_Interp *sourceInterp, int result,
	    Tcl_Interp *targetInterp)
}

# TIP#335 (detect if interpreter in use) jmistachkin
declare 608 {
    int Tcl_InterpActive(Tcl_Interp *interp)
}

# TIP#337 (log exception for background processing) dgp
declare 609 {
    void Tcl_BackgroundException(Tcl_Interp *interp, int code)
}

# TIP#234 (zlib interface) dkf/Pascal Scheffers
declare 610 {
    int Tcl_ZlibDeflate(Tcl_Interp *interp, int format, Tcl_Obj *data,
	    int level, Tcl_Obj *gzipHeaderDictObj)
}
declare 611 {
    int Tcl_ZlibInflate(Tcl_Interp *interp, int format, Tcl_Obj *data,
	    int buffersize, Tcl_Obj *gzipHeaderDictObj)
}
declare 612 {
    unsigned int Tcl_ZlibCRC32(unsigned int crc, const unsigned char *buf,
	    int len)
}
declare 613 {
    unsigned int Tcl_ZlibAdler32(unsigned int adler, const unsigned char *buf,
	    int len)
}
declare 614 {
    int Tcl_ZlibStreamInit(Tcl_Interp *interp, int mode, int format,
	    int level, Tcl_Obj *dictObj, Tcl_ZlibStream *zshandle)
}
declare 615 {
    Tcl_Obj *Tcl_ZlibStreamGetCommandName(Tcl_ZlibStream zshandle)
}
declare 616 {
    int Tcl_ZlibStreamEof(Tcl_ZlibStream zshandle)
}
declare 617 {
    int Tcl_ZlibStreamChecksum(Tcl_ZlibStream zshandle)
}
declare 618 {
    int Tcl_ZlibStreamPut(Tcl_ZlibStream zshandle, Tcl_Obj *data, int flush)
}
declare 619 {
    int Tcl_ZlibStreamGet(Tcl_ZlibStream zshandle, Tcl_Obj *data, int count)
}
declare 620 {
    int Tcl_ZlibStreamClose(Tcl_ZlibStream zshandle)
}
declare 621 {
    int Tcl_ZlibStreamReset(Tcl_ZlibStream zshandle)
}

# TIP 338 (control over startup script) dgp
declare 622 {
    void Tcl_SetStartupScript(Tcl_Obj *path, const char *encoding)
}
declare 623 {
    Tcl_Obj *Tcl_GetStartupScript(const char **encodingPtr)
}

# TIP#332 (half-close made public) aferrieux
declare 624 {
    int Tcl_CloseEx(Tcl_Interp *interp, Tcl_Channel chan, int flags)
}

# TIP #353 (NR-enabled expressions) dgp
declare 625 {
    int Tcl_NRExprObj(Tcl_Interp *interp, Tcl_Obj *objPtr, Tcl_Obj *resultPtr)
}

# TIP #356 (NR-enabled substitution) dgp
declare 626 {
    int Tcl_NRSubstObj(Tcl_Interp *interp, Tcl_Obj *objPtr, int flags)
}

# TIP #357 (Export TclLoadFile and TclpFindSymbol) kbk
declare 627 {
    int Tcl_LoadFile(Tcl_Interp *interp, Tcl_Obj *pathPtr,
		     const char *const symv[], int flags, void *procPtrs,
		     Tcl_LoadHandle *handlePtr)
}
declare 628 {
    void *Tcl_FindSymbol(Tcl_Interp *interp, Tcl_LoadHandle handle,
			 const char *symbol)
}
declare 629 {
    int Tcl_FSUnloadFile(Tcl_Interp *interp, Tcl_LoadHandle handlePtr)
}

# TIP #400
declare 630 {
    void Tcl_ZlibStreamSetCompressionDictionary(Tcl_ZlibStream zhandle,
	    Tcl_Obj *compressionDictionaryObj)
}

declare 648 {
    void TclUnusedStubEntry(void)
}

# ----- BASELINE -- FOR -- 8.6.0 ----- #

##############################################################################

# Define the platform specific public Tcl interface. These functions are only
# available on the designated platform.

interface tclPlat

################################
# Unix specific functions
#   (none)

################################
# Windows specific functions

# Added in Tcl 8.1

declare 0 win {
    TCHAR *Tcl_WinUtfToTChar(const char *str, int len, Tcl_DString *dsPtr)
}
declare 1 win {
    char *Tcl_WinTCharToUtf(const TCHAR *str, int len, Tcl_DString *dsPtr)
}

################################
# Mac OS X specific functions

declare 0 macosx {
    int Tcl_MacOSXOpenBundleResources(Tcl_Interp *interp,
	    const char *bundleName, int hasResourceFile,
	    int maxPathLen, char *libraryPath)
}
declare 1 macosx {
    int Tcl_MacOSXOpenVersionedBundleResources(Tcl_Interp *interp,
	    const char *bundleName, const char *bundleVersion,
	    int hasResourceFile, int maxPathLen, char *libraryPath)
}

##############################################################################

# Public functions that are not accessible via the stubs table.

export {
    void Tcl_Main(int argc, char **argv, Tcl_AppInitProc *appInitProc)
}
export {
    void Tcl_MainEx(int argc, char **argv, Tcl_AppInitProc *appInitProc,
    Tcl_Interp *interp)
}
export {
    const char *Tcl_InitStubs(Tcl_Interp *interp, const char *version,
	int exact)
}
export {
    const char *TclTomMathInitializeStubs(Tcl_Interp* interp,
	const char* version, int epoch, int revision)
}
export {
    const char *Tcl_PkgInitStubsCheck(Tcl_Interp *interp, const char *version,
	int exact)
}
export {
    void Tcl_GetMemoryInfo(Tcl_DString *dsPtr)
}

# Local Variables:
# mode: tcl
# End:
