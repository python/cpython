f = Function(void, 'SetPort',
	(WindowPtr, 'thePort', InMode),
)
functions.append(f)

f = Function(void, 'OpenDeskAcc',
	(Str255, 'name', InMode),
)
functions.append(f)
