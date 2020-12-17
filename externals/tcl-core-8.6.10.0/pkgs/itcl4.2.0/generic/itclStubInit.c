/*
 * This file is (mostly) automatically generated from itcl.decls.
 * It is compiled and linked in with the itcl package proper.
 */

#include "itclInt.h"

MODULE_SCOPE const ItclStubs itclStubs;
/* !BEGIN!: Do not edit below this line. */

static const ItclIntStubs itclIntStubs = {
    TCL_STUB_MAGIC,
    ITCLINT_STUBS_EPOCH,
    ITCLINT_STUBS_REVISION,
    0,
    Itcl_IsClassNamespace, /* 0 */
    Itcl_IsClass, /* 1 */
    Itcl_FindClass, /* 2 */
    Itcl_FindObject, /* 3 */
    Itcl_IsObject, /* 4 */
    Itcl_ObjectIsa, /* 5 */
    Itcl_Protection, /* 6 */
    Itcl_ProtectionStr, /* 7 */
    Itcl_CanAccess, /* 8 */
    Itcl_CanAccessFunc, /* 9 */
    0, /* 10 */
    Itcl_ParseNamespPath, /* 11 */
    Itcl_DecodeScopedCommand, /* 12 */
    Itcl_EvalArgs, /* 13 */
    Itcl_CreateArgs, /* 14 */
    0, /* 15 */
    0, /* 16 */
    Itcl_GetContext, /* 17 */
    Itcl_InitHierIter, /* 18 */
    Itcl_DeleteHierIter, /* 19 */
    Itcl_AdvanceHierIter, /* 20 */
    Itcl_FindClassesCmd, /* 21 */
    Itcl_FindObjectsCmd, /* 22 */
    0, /* 23 */
    Itcl_DelClassCmd, /* 24 */
    Itcl_DelObjectCmd, /* 25 */
    Itcl_ScopeCmd, /* 26 */
    Itcl_CodeCmd, /* 27 */
    Itcl_StubCreateCmd, /* 28 */
    Itcl_StubExistsCmd, /* 29 */
    Itcl_IsStub, /* 30 */
    Itcl_CreateClass, /* 31 */
    Itcl_DeleteClass, /* 32 */
    Itcl_FindClassNamespace, /* 33 */
    Itcl_HandleClass, /* 34 */
    0, /* 35 */
    0, /* 36 */
    0, /* 37 */
    Itcl_BuildVirtualTables, /* 38 */
    Itcl_CreateVariable, /* 39 */
    Itcl_DeleteVariable, /* 40 */
    Itcl_GetCommonVar, /* 41 */
    0, /* 42 */
    0, /* 43 */
    Itcl_CreateObject, /* 44 */
    Itcl_DeleteObject, /* 45 */
    Itcl_DestructObject, /* 46 */
    0, /* 47 */
    Itcl_GetInstanceVar, /* 48 */
    0, /* 49 */
    Itcl_BodyCmd, /* 50 */
    Itcl_ConfigBodyCmd, /* 51 */
    Itcl_CreateMethod, /* 52 */
    Itcl_CreateProc, /* 53 */
    Itcl_CreateMemberFunc, /* 54 */
    Itcl_ChangeMemberFunc, /* 55 */
    Itcl_DeleteMemberFunc, /* 56 */
    Itcl_CreateMemberCode, /* 57 */
    Itcl_DeleteMemberCode, /* 58 */
    Itcl_GetMemberCode, /* 59 */
    0, /* 60 */
    Itcl_EvalMemberCode, /* 61 */
    0, /* 62 */
    0, /* 63 */
    0, /* 64 */
    0, /* 65 */
    0, /* 66 */
    Itcl_GetMemberFuncUsage, /* 67 */
    Itcl_ExecMethod, /* 68 */
    Itcl_ExecProc, /* 69 */
    0, /* 70 */
    Itcl_ConstructBase, /* 71 */
    Itcl_InvokeMethodIfExists, /* 72 */
    0, /* 73 */
    Itcl_ReportFuncErrors, /* 74 */
    Itcl_ParseInit, /* 75 */
    Itcl_ClassCmd, /* 76 */
    Itcl_ClassInheritCmd, /* 77 */
    Itcl_ClassProtectionCmd, /* 78 */
    Itcl_ClassConstructorCmd, /* 79 */
    Itcl_ClassDestructorCmd, /* 80 */
    Itcl_ClassMethodCmd, /* 81 */
    Itcl_ClassProcCmd, /* 82 */
    Itcl_ClassVariableCmd, /* 83 */
    Itcl_ClassCommonCmd, /* 84 */
    Itcl_ParseVarResolver, /* 85 */
    Itcl_BiInit, /* 86 */
    Itcl_InstallBiMethods, /* 87 */
    Itcl_BiIsaCmd, /* 88 */
    Itcl_BiConfigureCmd, /* 89 */
    Itcl_BiCgetCmd, /* 90 */
    Itcl_BiChainCmd, /* 91 */
    Itcl_BiInfoClassCmd, /* 92 */
    Itcl_BiInfoInheritCmd, /* 93 */
    Itcl_BiInfoHeritageCmd, /* 94 */
    Itcl_BiInfoFunctionCmd, /* 95 */
    Itcl_BiInfoVariableCmd, /* 96 */
    Itcl_BiInfoBodyCmd, /* 97 */
    Itcl_BiInfoArgsCmd, /* 98 */
    0, /* 99 */
    Itcl_EnsembleInit, /* 100 */
    Itcl_CreateEnsemble, /* 101 */
    Itcl_AddEnsemblePart, /* 102 */
    Itcl_GetEnsemblePart, /* 103 */
    Itcl_IsEnsemble, /* 104 */
    Itcl_GetEnsembleUsage, /* 105 */
    Itcl_GetEnsembleUsageForObj, /* 106 */
    Itcl_EnsembleCmd, /* 107 */
    Itcl_EnsPartCmd, /* 108 */
    Itcl_EnsembleErrorCmd, /* 109 */
    0, /* 110 */
    0, /* 111 */
    0, /* 112 */
    0, /* 113 */
    0, /* 114 */
    Itcl_Assert, /* 115 */
    Itcl_IsObjectCmd, /* 116 */
    Itcl_IsClassCmd, /* 117 */
    0, /* 118 */
    0, /* 119 */
    0, /* 120 */
    0, /* 121 */
    0, /* 122 */
    0, /* 123 */
    0, /* 124 */
    0, /* 125 */
    0, /* 126 */
    0, /* 127 */
    0, /* 128 */
    0, /* 129 */
    0, /* 130 */
    0, /* 131 */
    0, /* 132 */
    0, /* 133 */
    0, /* 134 */
    0, /* 135 */
    0, /* 136 */
    0, /* 137 */
    0, /* 138 */
    0, /* 139 */
    Itcl_FilterAddCmd, /* 140 */
    Itcl_FilterDeleteCmd, /* 141 */
    Itcl_ForwardAddCmd, /* 142 */
    Itcl_ForwardDeleteCmd, /* 143 */
    Itcl_MixinAddCmd, /* 144 */
    Itcl_MixinDeleteCmd, /* 145 */
    0, /* 146 */
    0, /* 147 */
    0, /* 148 */
    0, /* 149 */
    0, /* 150 */
    Itcl_BiInfoUnknownCmd, /* 151 */
    Itcl_BiInfoVarsCmd, /* 152 */
    Itcl_CanAccess2, /* 153 */
    0, /* 154 */
    0, /* 155 */
    0, /* 156 */
    0, /* 157 */
    0, /* 158 */
    0, /* 159 */
    Itcl_SetCallFrameResolver, /* 160 */
    ItclEnsembleSubCmd, /* 161 */
    Itcl_GetUplevelNamespace, /* 162 */
    Itcl_GetCallFrameClientData, /* 163 */
    0, /* 164 */
    Itcl_SetCallFrameNamespace, /* 165 */
    Itcl_GetCallFrameObjc, /* 166 */
    Itcl_GetCallFrameObjv, /* 167 */
    Itcl_NWidgetCmd, /* 168 */
    Itcl_AddOptionCmd, /* 169 */
    Itcl_AddComponentCmd, /* 170 */
    Itcl_BiInfoOptionCmd, /* 171 */
    Itcl_BiInfoComponentCmd, /* 172 */
    Itcl_RenameCommand, /* 173 */
    Itcl_PushCallFrame, /* 174 */
    Itcl_PopCallFrame, /* 175 */
    Itcl_GetUplevelCallFrame, /* 176 */
    Itcl_ActivateCallFrame, /* 177 */
    ItclSetInstanceVar, /* 178 */
    ItclCapitalize, /* 179 */
    ItclClassBaseCmd, /* 180 */
    ItclCreateComponent, /* 181 */
    Itcl_SetContext, /* 182 */
    Itcl_UnsetContext, /* 183 */
    ItclGetInstanceVar, /* 184 */
};

static const ItclStubHooks itclStubHooks = {
    &itclIntStubs
};

const ItclStubs itclStubs = {
    TCL_STUB_MAGIC,
    ITCL_STUBS_EPOCH,
    ITCL_STUBS_REVISION,
    &itclStubHooks,
    0, /* 0 */
    0, /* 1 */
    Itcl_RegisterC, /* 2 */
    Itcl_RegisterObjC, /* 3 */
    Itcl_FindC, /* 4 */
    Itcl_InitStack, /* 5 */
    Itcl_DeleteStack, /* 6 */
    Itcl_PushStack, /* 7 */
    Itcl_PopStack, /* 8 */
    Itcl_PeekStack, /* 9 */
    Itcl_GetStackValue, /* 10 */
    Itcl_InitList, /* 11 */
    Itcl_DeleteList, /* 12 */
    Itcl_CreateListElem, /* 13 */
    Itcl_DeleteListElem, /* 14 */
    Itcl_InsertList, /* 15 */
    Itcl_InsertListElem, /* 16 */
    Itcl_AppendList, /* 17 */
    Itcl_AppendListElem, /* 18 */
    Itcl_SetListValue, /* 19 */
    Itcl_EventuallyFree, /* 20 */
    Itcl_PreserveData, /* 21 */
    Itcl_ReleaseData, /* 22 */
    Itcl_SaveInterpState, /* 23 */
    Itcl_RestoreInterpState, /* 24 */
    Itcl_DiscardInterpState, /* 25 */
    Itcl_Alloc, /* 26 */
    Itcl_Free, /* 27 */
};

/* !END!: Do not edit above this line. */
