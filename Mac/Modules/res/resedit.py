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

# Convert resources to other things.

as_xxx_body = """
return %sObj_New((%sHandle)_self->ob_itself);
"""

def genresconverter(longname, shortname):

	f = ManualGenerator("as_%s"%longname, as_xxx_body%(shortname, longname))
	docstring =  "Return this resource/handle as a %s"%longname
	f.docstring = lambda docstring=docstring: docstring
	return f

resmethods.append(genresconverter("Control", "Ctl"))
resmethods.append(genresconverter("Menu", "Menu"))
