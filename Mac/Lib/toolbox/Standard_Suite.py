"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Moes:Programma's:Speech Technology:Scriptable Text Editor
AETE/AEUT resource version 1/0, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'CoRe'

_Enum_savo = {
	'yes' : 'yes ',	# Save objects now
	'no' : 'no  ',	# Do not save objects
	'ask' : 'ask ',	# Ask the user whether to save
}

_Enum_kfrm = {
	'index' : 'indx',	# keyform designating indexed access
	'named' : 'name',	# keyform designating named access
	'id' : 'ID  ',	# keyform designating identifer access
}

class Standard_Suite:

	_argmap_close = {
		'saving' : 'savo',
		'saving_in' : 'kfil',
	}

	def close(self, object, *arguments):
		"""close: Close an object
		Required argument: the object to close
		Keyword argument saving: Specifies whether or not changes should be saved before closing
		Keyword argument saving_in: the file in which to save the object
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'clos'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_close)
		aetools.enumsubst(arguments, 'savo', _Enum_savo)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_count = {
		'each' : 'kocl',
	}

	def count(self, object, *arguments):
		"""count: Return the number of elements of a particular class within an object
		Required argument: the object whose elements are to be counted
		Keyword argument each: the class of the elements to be counted. Keyword 'each' is optional in AppleScript
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the number of elements
		"""
		_code = 'core'
		_subcode = 'cnte'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_count)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_data_size = {
		'as' : 'rtyp',
	}

	def data_size(self, object, *arguments):
		"""data size: Return the size in bytes of an object
		Required argument: the object whose data size is to be returned
		Keyword argument as: the data type for which the size is calculated
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the size of the object in bytes
		"""
		_code = 'core'
		_subcode = 'dsiz'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_data_size)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def delete(self, object, *arguments):
		"""delete: Delete an element from an object
		Required argument: the element to delete
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'delo'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_duplicate = {
		'to' : 'insh',
	}

	def duplicate(self, object, *arguments):
		"""duplicate: Duplicate object(s)
		Required argument: the object(s) to duplicate
		Keyword argument to: the new location for the object(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the duplicated object(s)
		"""
		_code = 'core'
		_subcode = 'clon'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_duplicate)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def exists(self, object, *arguments):
		"""exists: Verify if an object exists
		Required argument: the object in question
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: true if it exists, false if not
		"""
		_code = 'core'
		_subcode = 'doex'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_get = {
		'as' : 'rtyp',
	}

	def get(self, object, *arguments):
		"""get: Get the data for an object
		Required argument: the object whose data is to be returned
		Keyword argument as: the desired types for the data, in order of preference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: The data from the object
		"""
		_code = 'core'
		_subcode = 'getd'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_get)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_make = {
		'new' : 'kocl',
		'at' : 'insh',
		'with_data' : 'data',
		'with_properties' : 'prdt',
	}

	def make(self, *arguments):
		"""make: Make a new element
		Keyword argument new: the class of the new element. Keyword 'new' is optional in AppleScript
		Keyword argument at: the location at which to insert the element
		Keyword argument with_data: the initial data for the element
		Keyword argument with_properties: the initial values for the properties of the element
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the new object(s)
		"""
		_code = 'core'
		_subcode = 'crel'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_make)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_move = {
		'to' : 'insh',
	}

	def move(self, object, *arguments):
		"""move: Move object(s) to a new location
		Required argument: the object(s) to move
		Keyword argument to: the new location for the object(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the object(s) after they have been moved
		"""
		_code = 'core'
		_subcode = 'move'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_move)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def open(self, object, *arguments):
		"""open: Open the specified object(s)
		Required argument: list of objects to open
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'odoc'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def _print(self, object, *arguments):
		"""print: Print the specified object(s)
		Required argument: list of objects to print
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'pdoc'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_quit = {
		'saving' : 'savo',
	}

	def quit(self, *arguments):
		"""quit: Quit an application program
		Keyword argument saving: Specifies whether or not to save currently open documents
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'quit'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_quit)
		aetools.enumsubst(arguments, 'savo', _Enum_savo)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_save = {
		'_in' : 'kfil',
		'as' : 'fltp',
	}

	def save(self, object, *arguments):
		"""save: Save an object
		Required argument: the object to save
		Keyword argument _in: the file in which to save the object
		Keyword argument as: the file type of the document in which to save the data
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'save'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_save)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_set = {
		'to' : 'data',
	}

	def set(self, object, *arguments):
		"""set: Set an object's data
		Required argument: the object to change
		Keyword argument to: the new value
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'setd'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_set)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']


#    Class 'application' ('capp') -- 'An application program'
#        property 'clipboard' ('pcli') '****' -- 'contents of the clipboard' [mutable list]
#        property 'frontmost' ('pisf') 'bool' -- 'Is this the frontmost application?' []
#        property 'name' ('pnam') 'itxt' -- 'the name' []
#        property 'selection' ('sele') 'csel' -- 'the selection visible to the user' [mutable]
#        property 'version' ('vers') 'vers' -- 'the version of the application' []
#        property 'text item delimiters' ('txdl') 'list' -- 'a list of all the text item delimiters' [mutable]
#        element 'docu' as ['indx', 'name', 'rang', 'test']
#        element 'cwin' as ['indx', 'name', 'rang', 'test']

#    Class 'applications' ('capp') -- 'Every application'
#        property 'class attributes' ('c@#!') 'type' -- 'special class attributes' [0]

#    Class 'document' ('docu') -- 'A document'
#        property 'bounds' ('pbnd') 'qdrt' -- 'the boundary rectangle for the document' [mutable]
#        property 'closeable' ('hclb') 'bool' -- 'Does the document have a close box?' []
#        property 'titled' ('ptit') 'bool' -- 'Does the document have a title bar?' []
#        property 'index' ('pidx') 'long' -- 'the number of the document' [mutable]
#        property 'floating' ('isfl') 'bool' -- 'Does the document float?' []
#        property 'modal' ('pmod') 'bool' -- 'Is the document modal?' []
#        property 'resizable' ('prsz') 'bool' -- 'Is the document resizable?' []
#        property 'zoomable' ('iszm') 'bool' -- 'Is the document zoomable?' []
#        property 'zoomed' ('pzum') 'bool' -- 'Is the document zoomed?' [mutable]
#        property 'name' ('pnam') 'itxt' -- 'the title of the document' [mutable]
#        property 'selection' ('sele') 'csel' -- 'the selection visible to the user' [mutable]
#        property 'visible' ('pvis') 'bool' -- 'Is the document visible?' []
#        property 'modified' ('imod') 'bool' -- 'Has the document been modified since the last save?' []
#        property 'position' ('ppos') 'QDpt' -- 'upper left coordinates of the document' []
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cins' as ['rele']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'ctxt' as ['rang']
#        element 'citm' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

#    Class 'documents' ('docu') -- 'Every document'
#        property 'class attributes' ('c@#!') 'type' -- 'special class attributes' [0]

#    Class 'file' ('file') -- 'A file'

#    Class 'files' ('file') -- 'Every file'
#        property 'class attributes' ('c@#!') 'type' -- 'special class attributes' [0]

#    Class 'insertion point' ('cins') -- 'An insertion location between two objects'
#        property 'length' ('leng') 'long' -- 'length of text object (in characters)' []
#        property 'offset' ('ofse') 'long' -- 'offset of a text object from the beginning of the document (first char has offset 1)' []
#        property 'font' ('font') 'ctxt' -- 'the name of the font' [mutable]
#        property 'size' ('ptsz') 'long' -- 'the size in points' [mutable]
#        property 'style' ('txst') 'tsty' -- 'the text style' [mutable]
#        property 'uniform styles' ('ustl') 'tsty' -- 'the text style' []
#        property 'writing code' ('psct') 'intl' -- 'the script system and language' [mutable]

#    Class 'insertion points' ('cins') -- 'Every insertion location'
#        property 'class attributes' ('c@#!') 'type' -- 'special class attributes' [0]

#    Class 'selection-object' ('csel') -- 'the selection visible to the user'
#        property 'contents' ('pcnt') 'type' -- 'the contents of the selection' [mutable]
#        property 'length' ('leng') 'long' -- 'length of text object (in characters)' []
#        property 'offset' ('ofse') 'long' -- 'offset of a text object from the beginning of the document (first char has offset 1)' []
#        property 'font' ('font') 'ctxt' -- 'the name of the font' [mutable]
#        property 'size' ('ptsz') 'long' -- 'the size in points' [mutable]
#        property 'style' ('txst') 'tsty' -- 'the text style' [mutable]
#        property 'uniform styles' ('ustl') 'tsty' -- 'the text style' []
#        property 'writing code' ('psct') 'intl' -- 'the script system and language' [mutable]
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'ctxt' as ['rang']
#        element 'citm' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

#    Class 'window' ('cwin') -- 'A window'
#        property 'bounds' ('pbnd') 'qdrt' -- 'the boundary rectangle for the window' [mutable]
#        property 'closeable' ('hclb') 'bool' -- 'Does the window have a close box?' []
#        property 'titled' ('ptit') 'bool' -- 'Does the window have a title bar?' []
#        property 'index' ('pidx') 'long' -- 'the number of the window' [mutable]
#        property 'floating' ('isfl') 'bool' -- 'Does the window float?' []
#        property 'modal' ('pmod') 'bool' -- 'Is the window modal?' []
#        property 'resizable' ('prsz') 'bool' -- 'Is the window resizable?' []
#        property 'zoomable' ('iszm') 'bool' -- 'Is the window zoomable?' []
#        property 'zoomed' ('pzum') 'bool' -- 'Is the window zoomed?' [mutable]
#        property 'name' ('pnam') 'itxt' -- 'the title of the window' [mutable]
#        property 'selection' ('sele') 'csel' -- 'the selection visible to the user' [mutable]
#        property 'visible' ('pvis') 'bool' -- 'is the window visible?' []
#        property 'modified' ('imod') 'bool' -- 'has the window been modified since the last save?' []
#        property 'position' ('ppos') 'QDpt' -- 'upper left coordinates of window' []
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'cins' as ['rele']
#        element 'cpar' as ['indx', 'rele', 'rang', 'test']
#        element 'ctxt' as ['rang']
#        element 'citm' as ['indx', 'rele', 'rang', 'test']
#        element 'cwor' as ['indx', 'rele', 'rang', 'test']

#    Class 'windows' ('cwin') -- 'Every window'
#        property 'class attributes' ('c@#!') 'type' -- 'special class attributes' [0]
