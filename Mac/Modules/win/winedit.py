# These are inline-routines/defines, so we do them "by hand"
#

f = Method(Boolean, 'IsWindowVisible',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'GetWindowStructureRgn',
	(WindowRef, 'theWindow', InMode),
	(RgnHandle, 'r', InMode),
)
methods.append(f)

f = Method(void, 'GetWindowContentRgn',
	(WindowRef, 'theWindow', InMode),
	(RgnHandle, 'r', InMode),
)
methods.append(f)

f = Method(void, 'GetWindowUpdateRgn',
	(WindowRef, 'theWindow', InMode),
	(RgnHandle, 'r', InMode),
)
methods.append(f)

f = Method(ExistingWindowPtr, 'GetNextWindow',
	(WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Function(short, 'FindWindow',
    (Point, 'thePoint', InMode),
    (ExistingWindowPtr, 'theWindow', OutMode),
)
functions.append(f)

f = Method(void, 'MoveWindow',
    (WindowPtr, 'theWindow', InMode),
    (short, 'hGlobal', InMode),
    (short, 'vGlobal', InMode),
    (Boolean, 'front', InMode),
)
methods.append(f)

f = Method(void, 'ShowWindow',
    (WindowPtr, 'theWindow', InMode),
)
methods.append(f)



