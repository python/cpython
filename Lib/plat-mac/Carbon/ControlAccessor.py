# Accessor functions for control properties

from Controls import *
import struct

# These needn't go through this module, but are here for completeness
def SetControlData_Handle(control, part, selector, data):
    control.SetControlData_Handle(part, selector, data)

def GetControlData_Handle(control, part, selector):
    return control.GetControlData_Handle(part, selector)

_accessdict = {
    kControlPopupButtonMenuHandleTag: (SetControlData_Handle, GetControlData_Handle),
}

_codingdict = {
    kControlPushButtonDefaultTag : ("b", None, None),

    kControlEditTextTextTag: (None, None, None),
    kControlEditTextPasswordTag: (None, None, None),

    kControlPopupButtonMenuIDTag: ("h", None, None),

    kControlListBoxDoubleClickTag: ("b", None, None),
}

def SetControlData(control, part, selector, data):
    if _accessdict.has_key(selector):
        setfunc, getfunc = _accessdict[selector]
        setfunc(control, part, selector, data)
        return
    if not _codingdict.has_key(selector):
        raise KeyError('Unknown control selector', selector)
    structfmt, coder, decoder = _codingdict[selector]
    if coder:
        data = coder(data)
    if structfmt:
        data = struct.pack(structfmt, data)
    control.SetControlData(part, selector, data)

def GetControlData(control, part, selector):
    if _accessdict.has_key(selector):
        setfunc, getfunc = _accessdict[selector]
        return getfunc(control, part, selector, data)
    if not _codingdict.has_key(selector):
        raise KeyError('Unknown control selector', selector)
    structfmt, coder, decoder = _codingdict[selector]
    data = control.GetControlData(part, selector)
    if structfmt:
        data = struct.unpack(structfmt, data)
    if decoder:
        data = decoder(data)
    if type(data) == type(()) and len(data) == 1:
        data = data[0]
    return data
