"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Macintosh HD:Internet:Internet-programma's:Netscape Communicatoré-map:Netscape Communicatoré
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'CoRe'

class Standard_Suite_Events:

	def close(self, _object, _attributes={}, **_arguments):
		"""close: Close an object
		Required argument: the objects to close
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'clos'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def data_size(self, _object, _attributes={}, **_arguments):
		"""data size: Return the size in bytes of an object
		Required argument: the object whose data size is to be returned
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the size of the object in bytes
		"""
		_code = 'core'
		_subcode = 'dsiz'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def get(self, _object, _attributes={}, **_arguments):
		"""get: Get the data for an object
		Required argument: the object whose data is to be returned
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The data from the object
		"""
		_code = 'core'
		_subcode = 'getd'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_set = {
		'to' : 'data',
	}

	def set(self, _object, _attributes={}, **_arguments):
		"""set: Set an object’s data
		Required argument: the object to change
		Keyword argument to: the new value
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'setd'

		aetools.keysubst(_arguments, self._argmap_set)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - An application program """
	want = 'capp'
class alert_application(aetools.NProperty):
	"""alert application - Most of the alerts will be sent to this application using yet unspecified AE interface. We need a few alert boxes: alert, confirm and notify. Any ideas on how to design this event? mailto:atotic@netscape.com. I’d like to conform to the standard. """
	which = 'ALAP'
	want = 'type'
class kiosk_mode(aetools.NProperty):
	"""kiosk mode - Kiosk mode leaves very few menus enabled """
	which = 'KOSK'
	want = 'long'
#        element 'cwin' as ['indx', 'name', 'ID  ']

class window(aetools.ComponentItem):
	"""window - A Window """
	want = 'cwin'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window """
	which = 'pbnd'
	want = 'qdrt'
class closeable(aetools.NProperty):
	"""closeable - Does the window have a close box? """
	which = 'hclb'
	want = 'bool'
class titled(aetools.NProperty):
	"""titled - Does the window have a title bar? """
	which = 'ptit'
	want = 'bool'
class index(aetools.NProperty):
	"""index - the number of the window """
	which = 'pidx'
	want = 'long'
class floating(aetools.NProperty):
	"""floating - Does the window float? """
	which = 'isfl'
	want = 'bool'
class modal(aetools.NProperty):
	"""modal - Is the window modal? """
	which = 'pmod'
	want = 'bool'
class resizable(aetools.NProperty):
	"""resizable - Is the window resizable? """
	which = 'prsz'
	want = 'bool'
class zoomable(aetools.NProperty):
	"""zoomable - Is the window zoomable? """
	which = 'iszm'
	want = 'bool'
class zoomed(aetools.NProperty):
	"""zoomed - Is the window zoomed? """
	which = 'pzum'
	want = 'bool'
class name(aetools.NProperty):
	"""name - the title of the window """
	which = 'pnam'
	want = 'itxt'
class visible(aetools.NProperty):
	"""visible - is the window visible? """
	which = 'pvis'
	want = 'bool'
class position(aetools.NProperty):
	"""position - upper left coordinates of window """
	which = 'ppos'
	want = 'QDpt'
class URL(aetools.NProperty):
	"""URL - Current URL """
	which = 'curl'
	want = 'TEXT'
class unique_ID(aetools.NProperty):
	"""unique ID - Window’s unique ID (a bridge between WWW! suite window id’s and standard AE windows) """
	which = 'wiid'
	want = 'long'
class busy(aetools.NProperty):
	"""busy - Is window loading something right now. 2, window is busy and will reject load requests. 1, window is busy, but will interrupt outstanding loads """
	which = 'busy'
	want = 'long'
application._propdict = {
	'alert_application' : alert_application,
	'kiosk_mode' : kiosk_mode,
}
application._elemdict = {
	'window' : window,
}
window._propdict = {
	'bounds' : bounds,
	'closeable' : closeable,
	'titled' : titled,
	'index' : index,
	'floating' : floating,
	'modal' : modal,
	'resizable' : resizable,
	'zoomable' : zoomable,
	'zoomed' : zoomed,
	'name' : name,
	'visible' : visible,
	'position' : position,
	'URL' : URL,
	'unique_ID' : unique_ID,
	'busy' : busy,
}
window._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cwin' : window,
	'capp' : application,
}

_propdeclarations = {
	'ptit' : titled,
	'pidx' : index,
	'ppos' : position,
	'curl' : URL,
	'pnam' : name,
	'pbnd' : bounds,
	'isfl' : floating,
	'hclb' : closeable,
	'ALAP' : alert_application,
	'iszm' : zoomable,
	'pmod' : modal,
	'pzum' : zoomed,
	'pvis' : visible,
	'KOSK' : kiosk_mode,
	'busy' : busy,
	'prsz' : resizable,
	'wiid' : unique_ID,
}

_compdeclarations = {
}

_enumdeclarations = {
}
