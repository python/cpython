# Accessor functions for control properties

from Controls import *
import struct

# These needn't go through this module, but are here for completeness
def SetControlDataHandle(control, part, selector, data):
	control.SetControlDataHandle(part, selector, data)
	
def GetControlDataHandle(control, part, selector):
	return control.GetControlDataHandle(part, selector)
	
_accessdict = {
	kControlPopupButtonMenuHandleTag: (SetControlDataHandle, GetControlDataHandle),
}

_codingdict = {
	kControlPushButtonDefaultTag : ("b", None, None),
	
	kControlEditTextTextTag: (None, None, None),
	kControlEditTextPasswordTag: (None, None, None),
	
	kControlPopupButtonMenuIDTag: ("h", None, None),
}

def SetControlData(control, part, selector, data):
	if _accessdict.has_key(selector):
		setfunc, getfunc = _accessdict[selector]
		setfunc(control, part, selector, data)
		return
	if not _codingdict.has_key(selector):
		raise KeyError, ('Unknown control selector', selector)
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
		raise KeyError, ('Unknown control selector', selector)
	structfmt, coder, decoder = _codingdict[selector]
	data = control.GetControlData(part, selector)
	if structfmt:
		data = struct.unpack(structfmt, data)
	if decoder:
		data = decoder(data)
	return data
	
