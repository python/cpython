"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Moes:Applications (Mac OS 9):Metrowerks CodeWarrior 7.0:Metrowerks CodeWarrior:CodeWarrior IDE 4.2.6
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'CoRe'

from StdSuites.Standard_Suite import *
class Standard_Suite_Events(Standard_Suite_Events):

	_argmap_close = {
		'saving' : 'savo',
		'saving_in' : 'kfil',
	}

	def close(self, _object, _attributes={}, **_arguments):
		"""close: close an object
		Required argument: the object to close
		Keyword argument saving: specifies whether or not changes should be saved before closing
		Keyword argument saving_in: the file in which to save the object
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'clos'

		aetools.keysubst(_arguments, self._argmap_close)
		_arguments['----'] = _object

		aetools.enumsubst(_arguments, 'savo', _Enum_savo)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_count = {
		'each' : 'kocl',
	}

	def count(self, _object, _attributes={}, **_arguments):
		"""count: return the number of elements of a particular class within an object
		Required argument: the object whose elements are to be counted
		Keyword argument each: the class of the elements to be counted. Keyword 'each' is optional in AppleScript
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the number of elements
		"""
		_code = 'core'
		_subcode = 'cnte'

		aetools.keysubst(_arguments, self._argmap_count)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_get = {
		'as' : 'rtyp',
	}

	def get(self, _object, _attributes={}, **_arguments):
		"""get: get the data for an object
		Required argument: the object whose data is to be returned
		Keyword argument as: the desired types for the data, in order of preference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The data from the object
		"""
		_code = 'core'
		_subcode = 'getd'

		aetools.keysubst(_arguments, self._argmap_get)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_make = {
		'new' : 'kocl',
		'as' : 'rtyp',
		'at' : 'insh',
		'with_data' : 'data',
		'with_properties' : 'prdt',
	}

	def make(self, _no_object=None, _attributes={}, **_arguments):
		"""make: make a new element
		Keyword argument new: the class of the new element„keyword 'new' is optional in AppleScript
		Keyword argument as: the desired types for the data, in order of preference
		Keyword argument at: the location at which to insert the element
		Keyword argument with_data: the initial data for the element
		Keyword argument with_properties: the initial values for the properties of the element
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the new object(s)
		"""
		_code = 'core'
		_subcode = 'crel'

		aetools.keysubst(_arguments, self._argmap_make)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def select(self, _object=None, _attributes={}, **_arguments):
		"""select: select the specified object
		Required argument: the object to select
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'slct'

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
		"""set: set an object's data
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
	"""application - an application program """
	want = 'capp'
class user_interaction(aetools.NProperty):
	"""user interaction - user interaction level """
	which = 'inte'
	want = 'Inte'
#        element 'docu' as ['indx', 'name', 'rang']
#        element 'cwin' as ['indx', 'name', 'rang']

class character(aetools.ComponentItem):
	"""character - a character """
	want = 'cha '
class offset(aetools.NProperty):
	"""offset - offset of a text object from the beginning of the document (first char has offset 1) """
	which = 'pOff'
	want = 'long'
class length(aetools.NProperty):
	"""length - length in characters of this object """
	which = 'pLen'
	want = 'long'

class document(aetools.ComponentItem):
	"""document - a document """
	want = 'docu'
class name(aetools.NProperty):
	"""name - the title of the document """
	which = 'pnam'
	want = 'itxt'
class kind(aetools.NProperty):
	"""kind - the kind of document """
	which = 'DKND'
	want = 'DKND'
class index(aetools.NProperty):
	"""index - the number of the document """
	which = 'pidx'
	want = 'long'
class location(aetools.NProperty):
	"""location - the file of the document """
	which = 'FILE'
	want = 'fss '
class file_permissions(aetools.NProperty):
	"""file permissions - the file permissions for the document """
	which = 'PERM'
	want = 'PERM'
class window(aetools.NProperty):
	"""window - the window of the document. """
	which = 'cwin'
	want = 'cwin'

documents = document

class file(aetools.ComponentItem):
	"""file - A file """
	want = 'file'

files = file

class insertion_point(aetools.ComponentItem):
	"""insertion point - An insertion location between two objects """
	want = 'cins'

class line(aetools.ComponentItem):
	"""line - lines of text """
	want = 'clin'
#        element 'cha ' as ['indx', 'rang', 'rele']

lines = line

class selection_2d_object(aetools.ComponentItem):
	"""selection-object - the selection visible to the user """
	want = 'csel'
class contents(aetools.NProperty):
	"""contents - the contents of the selection """
	which = 'pcnt'
	want = 'type'
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'clin' as ['indx', 'rang', 'rele']
#        element 'ctxt' as ['rang']

class text(aetools.ComponentItem):
	"""text - Text """
	want = 'ctxt'
#        element 'cha ' as ['indx', 'rele', 'rang']
#        element 'cins' as ['rele']
#        element 'clin' as ['indx', 'rang', 'rele']
#        element 'ctxt' as ['rang']

class window(aetools.ComponentItem):
	"""window - A window """
	want = 'cwin'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window """
	which = 'pbnd'
	want = 'qdrt'
class document(aetools.NProperty):
	"""document - the document that owns this window """
	which = 'docu'
	want = 'docu'
class position(aetools.NProperty):
	"""position - upper left coordinates of window """
	which = 'ppos'
	want = 'QDpt'
class visible(aetools.NProperty):
	"""visible - is the window visible? """
	which = 'pvis'
	want = 'bool'
class zoomed(aetools.NProperty):
	"""zoomed - Is the window zoomed? """
	which = 'pzum'
	want = 'bool'

windows = window
application._propdict = {
	'user_interaction' : user_interaction,
}
application._elemdict = {
	'document' : document,
	'window' : window,
}
character._propdict = {
	'offset' : offset,
	'length' : length,
}
character._elemdict = {
}
document._propdict = {
	'name' : name,
	'kind' : kind,
	'index' : index,
	'location' : location,
	'file_permissions' : file_permissions,
	'window' : window,
}
document._elemdict = {
}
file._propdict = {
}
file._elemdict = {
}
insertion_point._propdict = {
	'length' : length,
	'offset' : offset,
}
insertion_point._elemdict = {
}
line._propdict = {
	'index' : index,
	'offset' : offset,
	'length' : length,
}
line._elemdict = {
	'character' : character,
}
selection_2d_object._propdict = {
	'contents' : contents,
	'length' : length,
	'offset' : offset,
}
selection_2d_object._elemdict = {
	'character' : character,
	'line' : line,
	'text' : text,
}
text._propdict = {
	'length' : length,
	'offset' : offset,
}
text._elemdict = {
	'character' : character,
	'insertion_point' : insertion_point,
	'line' : line,
	'text' : text,
}
window._propdict = {
	'name' : name,
	'index' : index,
	'bounds' : bounds,
	'document' : document,
	'position' : position,
	'visible' : visible,
	'zoomed' : zoomed,
}
window._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cha ' : character,
	'ctxt' : text,
	'cwin' : window,
	'file' : file,
	'clin' : line,
	'csel' : selection_2d_object,
	'capp' : application,
	'cins' : insertion_point,
	'docu' : document,
}

_propdeclarations = {
	'inte' : user_interaction,
	'pvis' : visible,
	'DKND' : kind,
	'pbnd' : bounds,
	'PERM' : file_permissions,
	'docu' : document,
	'pidx' : index,
	'pOff' : offset,
	'cwin' : window,
	'FILE' : location,
	'pnam' : name,
	'pLen' : length,
	'ppos' : position,
	'pzum' : zoomed,
	'pcnt' : contents,
}

_compdeclarations = {
}

_enumdeclarations = {
}
