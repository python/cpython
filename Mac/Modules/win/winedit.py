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
        _self->ob_freeit = PyMac_AutoDisposeWindow;
else
        _self->ob_freeit = NULL;
_res = Py_BuildValue("i", old);
return _res;
"""
f = ManualGenerator("AutoDispose", AutoDispose_body)
f.docstring = lambda: "(int)->int. Automatically DisposeHandle the object on Python object cleanup"
methods.append(f)
