resource_body = """
char *buf;
int len;
Handle h;

if (!PyArg_ParseTuple(_args, "s#", &buf, &len))
	return NULL;
h = NewHandle(len);
if ( h == NULL ) {
	PyErr_NoMemory();
	return NULL;
}
HLock(h);
memcpy(*h, buf, len);
HUnlock(h);
return (PyObject *)ResObj_New(h);
"""

f = ManualGenerator("Resource", resource_body)
f.docstring = lambda: """Convert a string to a resource object.

The created resource object is actually just a handle.
Apply AddResource() to write it to a resource file.
"""
functions.append(f)
