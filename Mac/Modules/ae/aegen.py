# Generated from 'Moes:CW5 GOLD \304:Metrowerks C/C++ \304:Headers \304:Universal Headers 2.0a3 \304:AppleEvents.h'

f = AEFunction(OSErr, 'AECreateDesc',
    (DescType, 'typeCode', InMode),
    (InBuffer, 'dataPtr', InMode),
    (AEDesc, 'result', OutMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AECoercePtr',
    (DescType, 'typeCode', InMode),
    (InBuffer, 'dataPtr', InMode),
    (DescType, 'toType', InMode),
    (AEDesc, 'result', OutMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AECoerceDesc',
    (AEDesc_ptr, 'theAEDesc', InMode),
    (DescType, 'toType', InMode),
    (AEDesc, 'result', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEDuplicateDesc',
    (AEDesc_ptr, 'theAEDesc', InMode),
    (AEDesc, 'result', OutMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AECreateList',
    (InBuffer, 'factoringPtr', InMode),
    (Boolean, 'isRecord', InMode),
    (AEDescList, 'resultList', OutMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AECountItems',
    (AEDescList_ptr, 'theAEDescList', InMode),
    (long, 'theCount', OutMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AEPutPtr',
    (AEDescList, 'theAEDescList', OutMode),
    (long, 'index', InMode),
    (DescType, 'typeCode', InMode),
    (InBuffer, 'dataPtr', InMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AEPutDesc',
    (AEDescList, 'theAEDescList', OutMode),
    (long, 'index', InMode),
    (AEDesc_ptr, 'theAEDesc', InMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AEGetNthPtr',
    (AEDescList_ptr, 'theAEDescList', InMode),
    (long, 'index', InMode),
    (DescType, 'desiredType', InMode),
    (AEKeyword, 'theAEKeyword', OutMode),
    (DescType, 'typeCode', OutMode),
    (VarVarOutBuffer, 'dataPtr', InOutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetNthDesc',
    (AEDescList_ptr, 'theAEDescList', InMode),
    (long, 'index', InMode),
    (DescType, 'desiredType', InMode),
    (AEKeyword, 'theAEKeyword', OutMode),
    (AEDesc, 'result', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AESizeOfNthItem',
    (AEDescList_ptr, 'theAEDescList', InMode),
    (long, 'index', InMode),
    (DescType, 'typeCode', OutMode),
    (Size, 'dataSize', OutMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AEDeleteItem',
    (AEDescList, 'theAEDescList', OutMode),
    (long, 'index', InMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AEPutParamPtr',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'typeCode', InMode),
    (InBuffer, 'dataPtr', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEPutParamDesc',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (AEDesc_ptr, 'theAEDesc', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetParamPtr',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'desiredType', InMode),
    (DescType, 'typeCode', OutMode),
    (VarVarOutBuffer, 'dataPtr', InOutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetParamDesc',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'desiredType', InMode),
    (AEDesc, 'result', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AESizeOfParam',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'typeCode', OutMode),
    (Size, 'dataSize', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEDeleteParam',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetAttributePtr',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'desiredType', InMode),
    (DescType, 'typeCode', OutMode),
    (VarVarOutBuffer, 'dataPtr', InOutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetAttributeDesc',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'desiredType', InMode),
    (AEDesc, 'result', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AESizeOfAttribute',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'typeCode', OutMode),
    (Size, 'dataSize', OutMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEPutAttributePtr',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (DescType, 'typeCode', InMode),
    (InBuffer, 'dataPtr', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEPutAttributeDesc',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AEKeyword, 'theAEKeyword', InMode),
    (AEDesc_ptr, 'theAEDesc', InMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AECreateAppleEvent',
    (AEEventClass, 'theAEEventClass', InMode),
    (AEEventID, 'theAEEventID', InMode),
    (AEAddressDesc_ptr, 'target', InMode),
    (short, 'returnID', InMode),
    (long, 'transactionID', InMode),
    (AppleEvent, 'result', OutMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AESend',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AppleEvent, 'reply', OutMode),
    (AESendMode, 'sendMode', InMode),
    (AESendPriority, 'sendPriority', InMode),
    (long, 'timeOutInTicks', InMode),
    (AEIdleUPP, 'idleProc', InMode),
    (AEFilterUPP, 'filterProc', InMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AEProcessAppleEvent',
    (EventRecord_ptr, 'theEventRecord', InMode),
)
functions.append(f)

f = AEMethod(OSErr, 'AEResetTimer',
    (AppleEvent_ptr, 'reply', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AESuspendTheCurrentEvent',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEResumeTheCurrentEvent',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
    (AppleEvent_ptr, 'reply', InMode),
    (AEEventHandlerUPP, 'dispatcher', InMode),
    (long, 'handlerRefcon', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AEGetTheCurrentEvent',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
)
aedescmethods.append(f)

f = AEMethod(OSErr, 'AESetTheCurrentEvent',
    (AppleEvent_ptr, 'theAppleEvent', InMode),
)
aedescmethods.append(f)

f = AEFunction(OSErr, 'AEGetInteractionAllowed',
    (AEInteractAllowed, 'level', OutMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AESetInteractionAllowed',
    (AEInteractAllowed, 'level', InMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AEInteractWithUser',
    (long, 'timeOutInTicks', InMode),
    (NMRecPtr, 'nmReqPtr', InMode),
    (AEIdleUPP, 'idleProc', InMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AEInstallEventHandler',
    (AEEventClass, 'theAEEventClass', InMode),
    (AEEventID, 'theAEEventID', InMode),
    (AEEventHandlerUPP, 'handler', InMode),
    (long, 'handlerRefcon', InMode),
    (AlwaysFalse, 'isSysHandler', InMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AERemoveEventHandler',
    (AEEventClass, 'theAEEventClass', InMode),
    (AEEventID, 'theAEEventID', InMode),
    (AEEventHandlerUPP, 'handler', InMode),
    (AlwaysFalse, 'isSysHandler', InMode),
)
functions.append(f)

f = AEFunction(OSErr, 'AEManagerInfo',
    (AEKeyword, 'keyWord', InMode),
    (long, 'result', OutMode),
)
functions.append(f)

