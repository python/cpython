# Generated from 'D:Development:THINK C:Mac #includes:Apple #includes:Events.h'

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

