as_resource_body = """
return ResObj_New((Handle)_self->ob_itself);
"""

f = ManualGenerator("as_Resource", as_resource_body)
f.docstring = lambda : "Return this Control as a Resource"

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
