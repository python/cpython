# Accessor functions for control properties

from Controls import *
import struct

_codingdict = {
	kControlPushButtonDefaultTag : ("b", None, None),
	kControlEditTextTextTag: (None, None, None),
	kControlEditTextPasswordTag: (None, None, None),
}

def SetControlData(control, part, selector, data):
	if not _codingdict.has_key(selector):
		raise KeyError, ('Unknown control selector', selector)
	structfmt, coder, decoder = _codingdict[selector]
	if coder:
		data = coder(data)
	if structfmt:
		data = struct.pack(structfmt, data)
	control.SetControlData(part, selector, data)
	
def GetControlData(control, part, selector):
	if not _codingdict.has_key(selector):
		raise KeyError, ('Unknown control selector', selector)
	structfmt, coder, decoder = _codingdict[selector]
	data = control.GetControlData(part, selector)
	if structfmt:
		data = struct.unpack(structfmt, data)
	if decoder:
		data = decoder(data)
	return data
	
