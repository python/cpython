# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Controls.h'

f = Function(ControlHandle, 'NewControl',
    (WindowPtr, 'theWindow', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (short, 'value', InMode),
    (short, 'min', InMode),
    (short, 'max', InMode),
    (short, 'procID', InMode),
    (long, 'refCon', InMode),
)
functions.append(f)

f = Method(void, 'SetCTitle',
    (ControlHandle, 'theControl', InMode),
    (ConstStr255Param, 'title', InMode),
)
methods.append(f)

f = Method(void, 'GetCTitle',
    (ControlHandle, 'theControl', InMode),
    (Str255, 'title', InMode),
)
methods.append(f)

f = Function(ControlHandle, 'GetNewControl',
    (short, 'controlID', InMode),
    (WindowPtr, 'owner', InMode),
)
functions.append(f)

f = Method(void, 'DisposeControl',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Function(void, 'KillControls',
    (WindowPtr, 'theWindow', InMode),
)
functions.append(f)

f = Method(void, 'HideControl',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'ShowControl',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Function(void, 'DrawControls',
    (WindowPtr, 'theWindow', InMode),
)
functions.append(f)

f = Method(void, 'Draw1Control',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'HiliteControl',
    (ControlHandle, 'theControl', InMode),
    (short, 'hiliteState', InMode),
)
methods.append(f)

f = Function(void, 'UpdtControl',
    (WindowPtr, 'theWindow', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
functions.append(f)

f = Function(void, 'UpdateControls',
    (WindowPtr, 'theWindow', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
functions.append(f)

f = Method(void, 'MoveControl',
    (ControlHandle, 'theControl', InMode),
    (short, 'h', InMode),
    (short, 'v', InMode),
)
methods.append(f)

f = Method(void, 'SizeControl',
    (ControlHandle, 'theControl', InMode),
    (short, 'w', InMode),
    (short, 'h', InMode),
)
methods.append(f)

f = Method(void, 'SetCtlValue',
    (ControlHandle, 'theControl', InMode),
    (short, 'theValue', InMode),
)
methods.append(f)

f = Method(short, 'GetCtlValue',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetCtlMin',
    (ControlHandle, 'theControl', InMode),
    (short, 'minValue', InMode),
)
methods.append(f)

f = Method(short, 'GetCtlMin',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetCtlMax',
    (ControlHandle, 'theControl', InMode),
    (short, 'maxValue', InMode),
)
methods.append(f)

f = Method(short, 'GetCtlMax',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetCRefCon',
    (ControlHandle, 'theControl', InMode),
    (long, 'data', InMode),
)
methods.append(f)

f = Method(long, 'GetCRefCon',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'DragControl',
    (ControlHandle, 'theControl', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'limitRect', InMode),
    (Rect_ptr, 'slopRect', InMode),
    (short, 'axis', InMode),
)
methods.append(f)

f = Method(short, 'TestControl',
    (ControlHandle, 'theControl', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Method(short, 'TrackControl',
    (ControlHandle, 'theControl', InMode),
    (Point, 'thePoint', InMode),
    (FakeType('(ControlActionUPP)0'), 'actionProc', InMode),
)
methods.append(f)

f = Function(short, 'FindControl',
    (Point, 'thePoint', InMode),
    (WindowPtr, 'theWindow', InMode),
    (ExistingControlHandle, 'theControl', OutMode),
)
functions.append(f)

f = Method(short, 'GetCVariant',
    (ControlHandle, 'theControl', InMode),
)
methods.append(f)

