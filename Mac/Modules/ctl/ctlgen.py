# Generated from 'Moes:CW5 GOLD \304:Metrowerks C/C++ \304:Headers \304:Universal Headers 2.0a3 \304:Controls.h'

f = Function(ControlRef, 'NewControl',
    (WindowRef, 'theWindow', InMode),
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

f = Method(void, 'SetControlTitle',
    (ControlRef, 'theControl', InMode),
    (ConstStr255Param, 'title', InMode),
)
methods.append(f)

f = Method(void, 'GetControlTitle',
    (ControlRef, 'theControl', InMode),
    (Str255, 'title', InMode),
)
methods.append(f)

f = Function(ControlRef, 'GetNewControl',
    (short, 'controlID', InMode),
    (WindowRef, 'owner', InMode),
)
functions.append(f)

f = Method(void, 'DisposeControl',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Function(void, 'KillControls',
    (WindowRef, 'theWindow', InMode),
)
functions.append(f)

f = Method(void, 'HideControl',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'ShowControl',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Function(void, 'DrawControls',
    (WindowRef, 'theWindow', InMode),
)
functions.append(f)

f = Method(void, 'Draw1Control',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'HiliteControl',
    (ControlRef, 'theControl', InMode),
    (short, 'hiliteState', InMode),
)
methods.append(f)

f = Function(void, 'UpdateControls',
    (WindowRef, 'theWindow', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
functions.append(f)

f = Method(void, 'MoveControl',
    (ControlRef, 'theControl', InMode),
    (short, 'h', InMode),
    (short, 'v', InMode),
)
methods.append(f)

f = Method(void, 'SizeControl',
    (ControlRef, 'theControl', InMode),
    (short, 'w', InMode),
    (short, 'h', InMode),
)
methods.append(f)

f = Method(void, 'SetControlValue',
    (ControlRef, 'theControl', InMode),
    (short, 'theValue', InMode),
)
methods.append(f)

f = Method(short, 'GetControlValue',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlMinimum',
    (ControlRef, 'theControl', InMode),
    (short, 'minValue', InMode),
)
methods.append(f)

f = Method(short, 'GetControlMinimum',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlMaximum',
    (ControlRef, 'theControl', InMode),
    (short, 'maxValue', InMode),
)
methods.append(f)

f = Method(short, 'GetControlMaximum',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlReference',
    (ControlRef, 'theControl', InMode),
    (long, 'data', InMode),
)
methods.append(f)

f = Method(long, 'GetControlReference',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlAction',
    (ControlRef, 'theControl', InMode),
    (FakeType('(ControlActionUPP)0'), 'actionProc', InMode),
)
methods.append(f)

f = Method(void, 'DragControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'limitRect', InMode),
    (Rect_ptr, 'slopRect', InMode),
    (short, 'axis', InMode),
)
methods.append(f)

f = Method(short, 'TestControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Function(short, 'FindControl',
    (Point, 'thePoint', InMode),
    (WindowRef, 'theWindow', InMode),
    (ExistingControlHandle, 'theControl', OutMode),
)
functions.append(f)

f = Method(short, 'GetControlVariant',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(short, 'TrackControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'thePoint', InMode),
    (FakeType('(ControlActionUPP)0'), 'actionProc', InMode),
)
methods.append(f)

