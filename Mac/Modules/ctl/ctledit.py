f = Function(ControlHandle, 'as_Control',
	(Handle, 'h', InMode))
functions.append(f)

f = Method(Handle, 'as_Resource', (ControlHandle, 'ctl', InMode))
methods.append(f)

f = Method(void, 'GetControlRect', (ControlHandle, 'ctl', InMode), (Rect, 'rect', OutMode))
methods.append(f)

DisposeControl_body = """
	if (!PyArg_ParseTuple(_args, ""))
		return NULL;
	if ( _self->ob_itself ) {
		SetControlReference(_self->ob_itself, (long)0); /* Make it forget about us */
		DisposeControl(_self->ob_itself);
		_self->ob_itself = NULL;
	}
	Py_INCREF(Py_None);
	_res = Py_None;
	return _res;
"""

f = ManualGenerator("DisposeControl", DisposeControl_body)
f.docstring = lambda : "() -> None"

methods.append(f)
