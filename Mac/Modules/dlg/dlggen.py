# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Dialogs.h'

f = Function(DialogPtr, 'NewDialog',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowPtr, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
    (Handle, 'itmLstHndl', InMode),
)
functions.append(f)

f = Function(DialogPtr, 'GetNewDialog',
    (short, 'dialogID', InMode),
    (NullStorage, 'dStorage', InMode),
    (WindowPtr, 'behind', InMode),
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
    (ModalFilterProcPtr, 'filterProc', InMode),
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
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'UpdateDialog',
    (DialogPtr, 'theDialog', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
methods.append(f)

f = Function(short, 'Alert',
    (short, 'alertID', InMode),
    (ModalFilterProcPtr, 'filterProc', InMode),
)
functions.append(f)

f = Function(short, 'StopAlert',
    (short, 'alertID', InMode),
    (ModalFilterProcPtr, 'filterProc', InMode),
)
functions.append(f)

f = Function(short, 'NoteAlert',
    (short, 'alertID', InMode),
    (ModalFilterProcPtr, 'filterProc', InMode),
)
functions.append(f)

f = Function(short, 'CautionAlert',
    (short, 'alertID', InMode),
    (ModalFilterProcPtr, 'filterProc', InMode),
)
functions.append(f)

f = Method(void, 'GetDItem',
    (DialogPtr, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'itemType', OutMode),
    (Handle, 'item', OutMode),
    (Rect, 'box', OutMode),
)
methods.append(f)

f = Method(void, 'SetDItem',
    (DialogPtr, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'itemType', InMode),
    (Handle, 'item', InMode),
    (Rect_ptr, 'box', InMode),
)
methods.append(f)

f = Method(void, 'HideDItem',
    (DialogPtr, 'theDialog', InMode),
    (short, 'itemNo', InMode),
)
methods.append(f)

f = Method(void, 'ShowDItem',
    (DialogPtr, 'theDialog', InMode),
    (short, 'itemNo', InMode),
)
methods.append(f)

f = Method(void, 'SelIText',
    (DialogPtr, 'theDialog', InMode),
    (short, 'itemNo', InMode),
    (short, 'strtSel', InMode),
    (short, 'endSel', InMode),
)
methods.append(f)

f = Function(void, 'GetIText',
    (Handle, 'item', InMode),
    (Str255, 'text', OutMode),
)
functions.append(f)

f = Function(void, 'SetIText',
    (Handle, 'item', InMode),
    (ConstStr255Param, 'text', InMode),
)
functions.append(f)

f = Method(short, 'FindDItem',
    (DialogPtr, 'theDialog', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Function(DialogPtr, 'NewCDialog',
    (NullStorage, 'dStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowPtr, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
    (Handle, 'items', InMode),
)
functions.append(f)

f = Function(void, 'ResetAlrtStage',
)
functions.append(f)

f = Method(void, 'DlgCut',
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DlgPaste',
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DlgCopy',
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'DlgDelete',
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Function(void, 'SetDAFont',
    (short, 'fontNum', InMode),
)
functions.append(f)

f = Method(void, 'AppendDITL',
    (DialogPtr, 'theDialog', InMode),
    (Handle, 'theHandle', InMode),
    (DITLMethod, 'method', InMode),
)
methods.append(f)

f = Method(short, 'CountDITL',
    (DialogPtr, 'theDialog', InMode),
)
methods.append(f)

f = Method(void, 'ShortenDITL',
    (DialogPtr, 'theDialog', InMode),
    (short, 'numberItems', InMode),
)
methods.append(f)

