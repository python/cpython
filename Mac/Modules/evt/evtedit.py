f = Function(void, 'SystemClick',
    (EventRecord_ptr, 'theEvent', InMode),
    (WindowPtr, 'theWindow', InMode),
)
functions.append(f)

f = Function(UInt32, 'TickCount',
)
functions.append(f)
