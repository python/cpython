# Generated from 'Sap:CodeWarrior6:Metrowerks C/C++:Headers:Universal Headers 2.0.1f:Dialogs.h'

f = Function(DialogRef, 'NewDialog',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowRef, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
    (Handle, 'itmLstHndl', InMode),
)
functions.append(f)

f = Function(DialogRef, 'GetNewDialog',
    (short, 'dialogID', InMode),
    (NullStorage, 'dStorage', InMode),
    (WindowRef, 'behind', InMode),
)
functions.append(f)

f = Function(void, 'ParamText',
    (ConstStr255Param, 'param0', InMode),
    (ConstStr255Param, 'param1', InMode),
    (ConstStr255Param, 'param2', InMode),
    (ConstStr255Param, 'param3', InMode),
)
functions.append(f)

f = Function(void, 'ModalDialog',
    (ModalFilterUPP, 'modalFilter', InMode),
    (short, 'itemHit', OutMode),
)
functions.append(f)

f = Function(Boolean, 'IsDialogEvent',
    (EventRecord_ptr, 'theEvent', InMode),
)
functions.append(f)

f = Function(Boolean, 'DialogSelect',
    (EventRecord_ptr, 'theEvent', InMode),
    (ExistingDialogPtr, 'theDialog', OutMode),
    (short, 'itemHit', OutMode),
)
functions.append(f)

f = Method(void, 'DrawDialog',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'UpdateDialog',
    (DialogRef, 'theDialog', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
methods.append(f)

f = Function(short, 'Alert',
    (short, 'alertID', InMode),
    (ModalFilterUPP, 'modalFilter', InMode),
)
functions.append(f)

f = Function(short, 'StopAlert',
    (short, 'alertID', InMode),
    (ModalFilterUPP, 'modalFilter', InMode),
)
functions.append(f)

f = Function(short, 'NoteAlert',
    (short, 'alertID', InMode),
    (ModalFilterUPP, 'modalFilter', InMode),
)
functions.append(f)

f = Function(short, 'CautionAlert',
    (short, 'alertID', InMode),
    (ModalFilterUPP, 'modalFilter', InMode),
)
functions.append(f)

f = Method(void, 'GetDialogItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'itemType', OutMode),
    (OptHandle, 'item', OutMode),
    (Rect, 'box', OutMode),
)
methods.append(f)

f = Method(void, 'SetDialogItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'itemType', InMode),
    (Handle, 'item', InMode),
    (Rect_ptr, 'box', InMode),
)
methods.append(f)

f = Method(void, 'HideDialogItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'itemNo', InMode),
)
methods.append(f)

f = Method(void, 'ShowDialogItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'itemNo', InMode),
)
methods.append(f)

f = Method(void, 'SelectDialogItemText',
    (DialogRef, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'strtSel', InMode),
    (short, 'endSel', InMode),
)
methods.append(f)

f = Function(void, 'GetDialogItemText',
    (Handle, 'item', InMode),
    (Str255, 'text', OutMode),
)
functions.append(f)

f = Function(void, 'SetDialogItemText',
    (Handle, 'item', InMode),
    (ConstStr255Param, 'text', InMode),
)
functions.append(f)

f = Method(short, 'FindDialogItem',
    (DialogRef, 'theDialog', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Function(DialogRef, 'NewColorDialog',
    (NullStorage, 'dStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowRef, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
    (Handle, 'items', InMode),
)
functions.append(f)

f = Function(short, 'GetAlertStage',
)
functions.append(f)

f = Function(void, 'ResetAlertStage',
)
functions.append(f)

f = Method(void, 'DialogCut',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DialogPaste',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DialogCopy',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DialogDelete',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Function(void, 'SetDialogFont',
    (short, 'value', InMode),
)
functions.append(f)

f = Method(void, 'AppendDITL',
    (DialogRef, 'theDialog', InMode),
    (Handle, 'theHandle', InMode),
    (DITLMethod, 'method', InMode),
)
methods.append(f)

f = Method(short, 'CountDITL',
    (DialogRef, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'ShortenDITL',
    (DialogRef, 'theDialog', InMode),
    (short, 'numberItems', InMode),
)
methods.append(f)

f = Method(Boolean, 'StdFilterProc',
    (DialogRef, 'theDialog', InMode),
    (EventRecord, 'event', OutMode),
    (short, 'itemHit', OutMode),
)
methods.append(f)

f = Method(OSErr, 'SetDialogDefaultItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'newItem', InMode),
)
methods.append(f)

f = Method(OSErr, 'SetDialogCancelItem',
    (DialogRef, 'theDialog', InMode),
    (short, 'newItem', InMode),
)
methods.append(f)

f = Method(OSErr, 'SetDialogTracksCursor',
    (DialogRef, 'theDialog', InMode),
    (Boolean, 'tracks', InMode),
)
methods.append(f)

