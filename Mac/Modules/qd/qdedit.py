f = Function(void, 'GlobalToLocal',
	(Point, 'thePoint', InOutMode),
)
functions.append(f)

f = Function(void, 'LocalToGlobal',
	(Point, 'thePoint', InOutMode),
)
functions.append(f)

f = Function(void, 'SetPort',
	(WindowPtr, 'thePort', InMode),
)
functions.append(f)

f = Function(void, 'ClipRect',
	(Rect, 'r', InMode),
)
functions.append(f)

f = Function(void, 'EraseRect',
	(Rect, 'r', InMode),
)
functions.append(f)

f = Function(void, 'OpenDeskAcc',
	(Str255, 'name', InMode),
)
functions.append(f)
