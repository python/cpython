# Generated from 'Sap:CodeWarrior7:Metrowerks CodeWarrior:MacOS Support:Headers:Universal Headers:Windows.h'

f = Function(RgnHandle, 'GetGrayRgn',
)
functions.append(f)

f = Function(void, 'InitWindows',
)
functions.append(f)

f = Function(void, 'GetWMgrPort',
    (GrafPtr, 'wPort', OutMode),
)
functions.append(f)

f = Function(WindowRef, 'NewWindow',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'theProc', InMode),
    (WindowRef, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
)
functions.append(f)

f = Function(WindowRef, 'GetNewWindow',
    (short, 'windowID', InMode),
    (NullStorage, 'wStorage', InMode),
    (WindowRef, 'behind', InMode),
)
functions.append(f)

f = Method(void, 'GetWTitle',
    (WindowRef, 'theWindow', InMode),
    (Str255, 'title', OutMode),
)
methods.append(f)

f = Method(void, 'SelectWindow',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'HideWindow',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'ShowWindow',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'ShowHide',
    (WindowRef, 'theWindow', InMode),
    (Boolean, 'showFlag', InMode),
)
methods.append(f)

f = Method(void, 'HiliteWindow',
    (WindowRef, 'theWindow', InMode),
    (Boolean, 'fHilite', InMode),
)
methods.append(f)

f = Method(void, 'BringToFront',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SendBehind',
    (WindowRef, 'theWindow', InMode),
    (WindowRef, 'behindWindow', InMode),
)
methods.append(f)

f = Function(ExistingWindowPtr, 'FrontWindow',
)
functions.append(f)

f = Method(void, 'DrawGrowIcon',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'MoveWindow',
    (WindowRef, 'theWindow', InMode),
    (short, 'hGlobal', InMode),
    (short, 'vGlobal', InMode),
    (Boolean, 'front', InMode),
)
methods.append(f)

f = Method(void, 'SizeWindow',
    (WindowRef, 'theWindow', InMode),
    (short, 'w', InMode),
    (short, 'h', InMode),
    (Boolean, 'fUpdate', InMode),
)
methods.append(f)

f = Method(void, 'ZoomWindow',
    (WindowRef, 'theWindow', InMode),
    (short, 'partCode', InMode),
    (Boolean, 'front', InMode),
)
methods.append(f)

f = Function(void, 'InvalRect',
    (Rect_ptr, 'badRect', InMode),
)
functions.append(f)

f = Function(void, 'InvalRgn',
    (RgnHandle, 'badRgn', InMode),
)
functions.append(f)

f = Function(void, 'ValidRect',
    (Rect_ptr, 'goodRect', InMode),
)
functions.append(f)

f = Function(void, 'ValidRgn',
    (RgnHandle, 'goodRgn', InMode),
)
functions.append(f)

f = Method(void, 'BeginUpdate',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'EndUpdate',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWRefCon',
    (WindowRef, 'theWindow', InMode),
    (long, 'data', InMode),
)
methods.append(f)

f = Method(long, 'GetWRefCon',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWindowPic',
    (WindowRef, 'theWindow', InMode),
    (PicHandle, 'pic', InMode),
)
methods.append(f)

f = Method(PicHandle, 'GetWindowPic',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Function(Boolean, 'CheckUpdate',
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Method(void, 'ClipAbove',
    (WindowRef, 'window', InMode),
)
methods.append(f)

f = Method(void, 'SaveOld',
    (WindowRef, 'window', InMode),
)
methods.append(f)

f = Method(void, 'DrawNew',
    (WindowRef, 'window', InMode),
    (Boolean, 'update', InMode),
)
methods.append(f)

f = Method(void, 'PaintOne',
    (WindowRef, 'window', InMode),
    (RgnHandle, 'clobberedRgn', InMode),
)
methods.append(f)

f = Method(void, 'PaintBehind',
    (WindowRef, 'startWindow', InMode),
    (RgnHandle, 'clobberedRgn', InMode),
)
methods.append(f)

f = Method(void, 'CalcVis',
    (WindowRef, 'window', InMode),
)
methods.append(f)

f = Method(void, 'CalcVisBehind',
    (WindowRef, 'startWindow', InMode),
    (RgnHandle, 'clobberedRgn', InMode),
)
methods.append(f)

f = Method(long, 'GrowWindow',
    (WindowRef, 'theWindow', InMode),
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
    (WindowRef, 'theWindow', InMode),
    (Point, 'thePt', InMode),
    (short, 'partCode', InMode),
)
methods.append(f)

f = Function(void, 'GetCWMgrPort',
    (CGrafPtr, 'wMgrCPort', OutMode),
)
functions.append(f)

f = Function(WindowRef, 'NewCWindow',
    (NullStorage, 'wStorage', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'procID', InMode),
    (WindowRef, 'behind', InMode),
    (Boolean, 'goAwayFlag', InMode),
    (long, 'refCon', InMode),
)
functions.append(f)

f = Function(WindowRef, 'GetNewCWindow',
    (short, 'windowID', InMode),
    (NullStorage, 'wStorage', InMode),
    (WindowRef, 'behind', InMode),
)
functions.append(f)

f = Method(short, 'GetWVariant',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWTitle',
    (WindowRef, 'theWindow', InMode),
    (ConstStr255Param, 'title', InMode),
)
methods.append(f)

f = Method(Boolean, 'TrackGoAway',
    (WindowRef, 'theWindow', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Method(void, 'DragWindow',
    (WindowRef, 'theWindow', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'boundsRect', InMode),
)
methods.append(f)

