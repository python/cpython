##resource_body = """
##char *buf;
##int len;
##Handle h;
##
##if (!PyArg_ParseTuple(_args, "s#", &buf, &len))
##      return NULL;
##h = NewHandle(len);
##if ( h == NULL ) {
##      PyErr_NoMemory();
##      return NULL;
##}
##HLock(h);
##memcpy(*h, buf, len);
##HUnlock(h);
##_res = ResObj_New(h);
##return _res;
##"""
##
##f = ManualGenerator("Resource", resource_body)
##f.docstring = lambda: """Convert a string to a resource object.
##
##The created resource object is actually just a handle,
##apply AddResource() to write it to a resource file.
##See also the Handle() docstring.
##"""
##functions.append(f)

handle_body = """
char *buf;
int len;
Handle h;
ResourceObject *rv;

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
rv = (ResourceObject *)ResObj_New(h);
rv->ob_freeit = PyMac_AutoDisposeHandle;
_res = (PyObject *)rv;
return _res;
"""

f = ManualGenerator("Handle", handle_body)
f.docstring = lambda: """Convert a string to a Handle object.

Resource() and Handle() are very similar, but objects created with Handle() are
by default automatically DisposeHandle()d upon object cleanup. Use AutoDispose()
to change this.
"""
functions.append(f)

# Convert resources to other things.

as_xxx_body = """
_res = %sObj_New((%sHandle)_self->ob_itself);
return _res;
"""

def genresconverter(longname, shortname):

    f = ManualGenerator("as_%s"%longname, as_xxx_body%(shortname, longname))
    docstring =  "Return this resource/handle as a %s"%longname
    f.docstring = lambda docstring=docstring: docstring
    return f

resmethods.append(genresconverter("Control", "Ctl"))
resmethods.append(genresconverter("Menu", "Menu"))

# The definition of this one is MacLoadResource, so we do it by hand...

f = ResMethod(void, 'LoadResource',
    (Handle, 'theResource', InMode),
)
resmethods.append(f)

#
# A method to set the auto-dispose flag
#
AutoDispose_body = """
int onoff, old = 0;
if (!PyArg_ParseTuple(_args, "i", &onoff))
        return NULL;
if ( _self->ob_freeit )
        old = 1;
if ( onoff )
        _self->ob_freeit = PyMac_AutoDisposeHandle;
else
        _self->ob_freeit = NULL;
_res = Py_BuildValue("i", old);
return _res;
"""
f = ManualGenerator("AutoDispose", AutoDispose_body)
f.docstring = lambda: "(int)->int. Automatically DisposeHandle the object on Python object cleanup"
resmethods.append(f)
