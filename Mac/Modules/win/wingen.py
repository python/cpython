# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Windows.h'

f = Function(void, 'InitWindows',
)
functions.append(f)

f = Function(WindowPtr, 'NewWindow',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'theProc', InMode),
    (WindowPtr, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
)
functions.append(f)

f = Function(WindowPtr, 'GetNewWindow',
    (short, 'windowID', InMode),
    (NullStorage, 'wStorage', InMode),
    (WindowPtr, 'behind', InMode),
)
functions.append(f)

f = Method(void, 'GetWTitle',
    (WindowPtr, 'theWindow', InMode),
    (Str255, 'title', OutMode),
)
methods.append(f)

f = Method(void, 'SelectWindow',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'HideWindow',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'ShowWindow',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'ShowHide',
    (WindowPtr, 'theWindow', InMode),
    (Boolean, 'showFlag', InMode),
)
methods.append(f)

f = Method(void, 'HiliteWindow',
    (WindowPtr, 'theWindow', InMode),
    (Boolean, 'fHilite', InMode),
)
methods.append(f)

f = Method(void, 'BringToFront',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SendBehind',
    (WindowPtr, 'theWindow', InMode),
    (WindowPtr, 'behindWindow', InMode),
)
methods.append(f)

f = Function(WindowPtr, 'FrontWindow',
)
functions.append(f)

f = Method(void, 'DrawGrowIcon',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'MoveWindow',
    (WindowPtr, 'theWindow', InMode),
    (short, 'hGlobal', InMode),
    (short, 'vGlobal', InMode),
    (Boolean, 'front', InMode),
)
methods.append(f)

f = Method(void, 'SizeWindow',
    (WindowPtr, 'theWindow', InMode),
    (short, 'w', InMode),
    (short, 'h', InMode),
    (Boolean, 'fUpdate', InMode),
)
methods.append(f)

f = Method(void, 'ZoomWindow',
    (WindowPtr, 'theWindow', InMode),
    (short, 'partCode', InMode),
    (Boolean, 'front', InMode),
)
methods.append(f)

f = Function(void, 'InvalRect',
    (Rect_ptr, 'badRect', InMode),
)
functions.append(f)

f = Function(void, 'ValidRect',
    (Rect_ptr, 'goodRect', InMode),
)
functions.append(f)

f = Method(void, 'BeginUpdate',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'EndUpdate',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWRefCon',
    (WindowPtr, 'theWindow', InMode),
    (long, 'data', InMode),
)
methods.append(f)

f = Method(long, 'GetWRefCon',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Function(Boolean, 'CheckUpdate',
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Method(void, 'ClipAbove',
    (WindowPeek, 'window', InMode),
)
methods.append(f)

f = Method(void, 'SaveOld',
    (WindowPeek, 'window', InMode),
)
methods.append(f)

f = Method(void, 'DrawNew',
    (WindowPeek, 'window', InMode),
    (Boolean, 'update', InMode),
)
methods.append(f)

f = Method(void, 'CalcVis',
    (WindowPeek, 'window', InMode),
)
methods.append(f)

f = Method(long, 'GrowWindow',
    (WindowPtr, 'theWindow', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'bBox', InMode),
)
methods.append(f)

f = Function(short, 'FindWindow',
    (Point, 'thePoint', InMode),
    (ExistingWindowPtr, 'theWindow', OutMode),
)
functions.append(f)

f = Function(long, 'PinRect',
    (Rect_ptr, 'theRect', InMode),
    (Point, 'thePt', InMode),
)
functions.append(f)

f = Method(Boolean, 'TrackBox',
    (WindowPtr, 'theWindow', InMode),
    (Point, 'thePt', InMode),
    (short, 'partCode', InMode),
)
methods.append(f)

f = Function(WindowPtr, 'NewCWindow',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowPtr, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
)
functions.append(f)

f = Function(WindowPtr, 'GetNewCWindow',
    (short, 'windowID', InMode),
    (NullStorage, 'wStorage', InMode),
    (WindowPtr, 'behind', InMode),
)
functions.append(f)

f = Method(short, 'GetWVariant',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWTitle',
    (WindowPtr, 'theWindow', InMode),
    (ConstStr255Param, 'title', InMode),
)
methods.append(f)

f = Method(Boolean, 'TrackGoAway',
    (WindowPtr, 'theWindow', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Method(void, 'DragWindow',
    (WindowPtr, 'theWindow', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'boundsRect', InMode),
)
methods.append(f)
