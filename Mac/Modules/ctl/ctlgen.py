# Generated from 'Sap:CodeWarrior6:Metrowerks C/C++:Headers:Universal Headers 2.0.1f:Controls.h'

f = Function(ControlRef, 'NewControl',
    (WindowRef, 'theWindow', InMode),
    (Rect_ptr, 'boundsRect', InMode),
    (ConstStr255Param, 'title', InMode),
    (Boolean, 'visible', InMode),
    (SInt16, 'value', InMode),
    (SInt16, 'min', InMode),
    (SInt16, 'max', InMode),
    (SInt16, 'procID', InMode),
    (SInt32, 'refCon', InMode),
)
functions.append(f)

f = Function(ControlRef, 'GetNewControl',
    (SInt16, 'controlID', InMode),
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

f = Method(void, 'ShowControl',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'HideControl',
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

f = Function(void, 'UpdateControls',
    (WindowRef, 'theWindow', InMode),
    (RgnHandle, 'updateRgn', InMode),
)
functions.append(f)

f = Method(void, 'HiliteControl',
    (ControlRef, 'theControl', InMode),
    (ControlPartCode, 'hiliteState', InMode),
)
methods.append(f)

f = Method(ControlPartCode, 'TrackControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'thePoint', InMode),
    (FakeType('(ControlActionUPP)0'), 'actionProc', InMode),
)
methods.append(f)

f = Method(void, 'DragControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'startPt', InMode),
    (Rect_ptr, 'limitRect', InMode),
    (Rect_ptr, 'slopRect', InMode),
    (DragConstraint, 'axis', InMode),
)
methods.append(f)

f = Method(ControlPartCode, 'TestControl',
    (ControlRef, 'theControl', InMode),
    (Point, 'thePt', InMode),
)
methods.append(f)

f = Function(ControlPartCode, 'FindControl',
    (Point, 'thePoint', InMode),
    (WindowRef, 'theWindow', InMode),
    (ExistingControlHandle, 'theControl', OutMode),
)
functions.append(f)

f = Method(void, 'MoveControl',
    (ControlRef, 'theControl', InMode),
    (SInt16, 'h', InMode),
    (SInt16, 'v', InMode),
)
methods.append(f)

f = Method(void, 'SizeControl',
    (ControlRef, 'theControl', InMode),
    (SInt16, 'w', InMode),
    (SInt16, 'h', InMode),
)
methods.append(f)

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

f = Method(SInt16, 'GetControlValue',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlValue',
    (ControlRef, 'theControl', InMode),
    (SInt16, 'newValue', InMode),
)
methods.append(f)

f = Method(SInt16, 'GetControlMinimum',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlMinimum',
    (ControlRef, 'theControl', InMode),
    (SInt16, 'newMinimum', InMode),
)
methods.append(f)

f = Method(SInt16, 'GetControlMaximum',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlMaximum',
    (ControlRef, 'theControl', InMode),
    (SInt16, 'newMaximum', InMode),
)
methods.append(f)

f = Method(SInt16, 'GetControlVariant',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

f = Method(void, 'SetControlAction',
    (ControlRef, 'theControl', InMode),
    (FakeType('(ControlActionUPP)0'), 'actionProc', InMode),
)
methods.append(f)

f = Method(void, 'SetControlReference',
    (ControlRef, 'theControl', InMode),
    (SInt32, 'data', InMode),
)
methods.append(f)

f = Method(SInt32, 'GetControlReference',
    (ControlRef, 'theControl', InMode),
)
methods.append(f)

