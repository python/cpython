# These are inline-routines/defines, so we do them "by hand"
#

f = Method(CGrafPtr, 'GetWindowPort',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetPortWindowPort',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(short, 'GetWindowKind',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'SetWindowKind',
    (WindowRef, 'theWindow', InMode),
    (short, 'wKind', InMode),
)
methods.append(f)


f = Method(Boolean, 'IsWindowVisible',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(Boolean, 'IsWindowHilited',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(Boolean, 'GetWindowGoAwayFlag',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(Boolean, 'GetWindowZoomFlag',
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

f = Method(short, 'GetWindowTitleWidth',
    (WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(ExistingWindowPtr, 'GetNextWindow',
	(WindowRef, 'theWindow', InMode),
)
methods.append(f)

f = Method(void, 'GetWindowStandardState',
	(WindowRef, 'theWindow', InMode),
	(Rect, 'r', OutMode),
)
methods.append(f)

f = Method(void, 'GetWindowUserState',
	(WindowRef, 'theWindow', InMode),
	(Rect, 'r', OutMode),
)
methods.append(f)


f = Method(void, 'SetWindowStandardState',
	(WindowRef, 'theWindow', InMode),
	(Rect, 'r', InMode),
)
methods.append(f)

f = Method(void, 'SetWindowUserState',
	(WindowRef, 'theWindow', InMode),
	(Rect, 'r', InMode),
)
methods.append(f)

# These have Mac prefixed to their name in the 3.1 universal headers,
# so we add the old/real names by hand.
f = Method(void, 'CloseWindow',
    (WindowPtr, 'theWindow', InMode),
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



