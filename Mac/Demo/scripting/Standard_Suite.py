"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Moes:Programma's:Eudora:Eudora Light
AETE/AEUT resource version 2/16, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'CoRe'

class Standard_Suite:

	def close(self, _object, _attributes={}, **_arguments):
		"""close: Close an object
		Required argument: the object to close
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'clos'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_count = {
		'each' : 'kocl',
	}

	def count(self, _object, _attributes={}, **_arguments):
		"""count: Return the number of elements of a particular class within an object
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def exists(self, _object, _attributes={}, **_arguments):
		"""exists: Verify if an object exists
		Required argument: the object in question
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: true if it exists, false if not
		"""
		_code = 'core'
		_subcode = 'doex'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def get(self, _object, _attributes={}, **_arguments):
		"""get: Get the data for an object
		Required argument: the object whose data is to be returned
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the data from the object
		"""
		_code = 'core'
		_subcode = 'getd'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_make = {
		'new' : 'kocl',
		'at' : 'insh',
	}

	def make(self, _no_object=None, _attributes={}, **_arguments):
		"""make: Make a new element
		Keyword argument new: the class of the new element. Keyword 'new' is optional in AppleScript
		Keyword argument at: the location at which to insert the element
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the new object
		"""
		_code = 'core'
		_subcode = 'crel'

		aetools.keysubst(_arguments, self._argmap_make)
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_move = {
		'to' : 'insh',
	}

	def move(self, _object, _attributes={}, **_arguments):
		"""move: Move object to a new location
		Required argument: the object to move
		Keyword argument to: the new location for the object
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the object after they have been moved
		"""
		_code = 'core'
		_subcode = 'move'

		aetools.keysubst(_arguments, self._argmap_move)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_duplicate = {
		'to' : 'insh',
	}

	def duplicate(self, _object, _attributes={}, **_arguments):
		"""duplicate: Make a duplicate object
		Required argument: the object to move
		Keyword argument to: the new location for the object
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the object after they have been moved
		"""
		_code = 'core'
		_subcode = 'clon'

		aetools.keysubst(_arguments, self._argmap_duplicate)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open the specified object
		Required argument: list of objects to open
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'odoc'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _print(self, _object, _attributes={}, **_arguments):
		"""print: Print the specified message
		Required argument: the message to print
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'pdoc'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def save(self, _object, _attributes={}, **_arguments):
		"""save: Save an object
		Required argument: the composition message to save
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'save'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_set = {
		'to' : 'data',
	}

	def set(self, _object, _attributes={}, **_arguments):
		"""set: Set an object's data
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class 'application' ('capp') -- 'An application program'
#        property 'version' ('vers') 'itxt' -- 'the version number' []
#        property 'selected text' ('eStx') 'TEXT' -- 'the text of the user\325s current selection' []
#        element 'euMF' as ['indx', 'name']
#        element 'ePrf' as ['indx']
