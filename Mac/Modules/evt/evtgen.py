# Generated from 'Moes:CW5 GOLD \304:Metrowerks C/C++ \304:Headers \304:Universal Headers 2.0a3 \304:Events.h'

f = Function(long, 'GetDblTime',
)
functions.append(f)

f = Function(long, 'GetCaretTime',
)
functions.append(f)

f = Function(void, 'SetEventMask',
    (short, 'value', InMode),
)
functions.append(f)

f = Function(Boolean, 'GetNextEvent',
    (short, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'WaitNextEvent',
    (short, 'eventMask', InMode),
    (EventRecord, 'theEvent', OutMode),
    (unsigned_long, 'sleep', InMode),
    (RgnHandle, 'mouseRgn', InMode),
)
functions.append(f)

f = Function(Boolean, 'EventAvail',
    (short, 'eventMask', InMode),
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

f = Function(unsigned_long, 'TickCount',
)
functions.append(f)

f = Function(OSErr, 'PostEvent',
    (short, 'eventNum', InMode),
    (long, 'eventMsg', InMode),
)
functions.append(f)

f = Function(Boolean, 'OSEventAvail',
    (short, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(Boolean, 'GetOSEvent',
    (short, 'mask', InMode),
    (EventRecord, 'theEvent', OutMode),
)
functions.append(f)

f = Function(void, 'FlushEvents',
    (short, 'whichMask', InMode),
    (short, 'stopMask', InMode),
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

