# Generated from 'Sap:CW8 Gold:Metrowerks CodeWarrior:MacOS Support:Headers:Universal Headers:Events.h'

f = Function(UInt32, 'GetCaretTime',
)
functions.append(f)

f = Function(void, 'SetEventMask',
    (EventMask, 'value', InMode),
)
functions.append(f)

f = Function(UInt32, 'GetDblTime',
)
functions.append(f)

f = Function(Boolean, 'GetNextEvent',
    (EventMask, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'WaitNextEvent',
    (EventMask, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
    (UInt32, 'sleep', InMode),
    (RgnHandle, 'mouseRgn', InMode),
)
functions.append(f)

f = Function(Boolean, 'EventAvail',
    (EventMask, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(void, 'GetMouse',
    (Point, 'mouseLoc', OutMode),
)
functions.append(f)

f = Function(Boolean, 'Button',
)
functions.append(f)

f = Function(Boolean, 'StillDown',
)
functions.append(f)

f = Function(Boolean, 'WaitMouseUp',
)
functions.append(f)

f = Function(void, 'GetKeys',
    (KeyMap, 'theKeys', OutMode),
)
functions.append(f)

f = Function(UInt32, 'TickCount',
)
functions.append(f)

f = Function(OSErr, 'PostEvent',
    (EventKind, 'eventNum', InMode),
    (UInt32, 'eventMsg', InMode),
)
functions.append(f)

f = Function(Boolean, 'OSEventAvail',
    (EventMask, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'GetOSEvent',
    (EventMask, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(void, 'FlushEvents',
    (EventMask, 'whichMask', InMode),
    (EventMask, 'stopMask', InMode),
)
functions.append(f)

f = Function(void, 'SystemClick',
    (EventRecord_ptr, 'theEvent', InMode),
    (WindowPtr, 'theWindow', InMode),
)
functions.append(f)

f = Function(void, 'SystemTask',
)
functions.append(f)

f = Function(Boolean, 'SystemEvent',
    (EventRecord_ptr, 'theEvent', InMode),
)
functions.append(f)

