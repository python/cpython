# Generated from 'Moes:CodeWarrior6:Metrowerks C/C++:Headers:Universal Headers 2.0.1f:Events.h'

f = Function(UInt32, 'GetCaretTime',
)
functions.append(f)

f = Function(void, 'SetEventMask',
    (MacOSEventMask, 'value', InMode),
)
functions.append(f)

f = Function(UInt32, 'GetDblTime',
)
functions.append(f)

f = Function(Boolean, 'GetNextEvent',
    (MacOSEventMask, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'WaitNextEvent',
    (MacOSEventMask, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
    (UInt32, 'sleep', InMode),
    (RgnHandle, 'mouseRgn', InMode),
)
functions.append(f)

f = Function(Boolean, 'EventAvail',
    (MacOSEventMask, 'eventMask', InMode),
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
    (MacOSEventKind, 'eventNum', InMode),
    (UInt32, 'eventMsg', InMode),
)
functions.append(f)

f = Function(Boolean, 'OSEventAvail',
    (MacOSEventMask, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'GetOSEvent',
    (MacOSEventMask, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(void, 'FlushEvents',
    (MacOSEventMask, 'whichMask', InMode),
    (MacOSEventMask, 'stopMask', InMode),
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

