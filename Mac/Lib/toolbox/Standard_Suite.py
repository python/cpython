"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from Moes:System folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'core'

_Enum_savo = {
	'yes' : 'yes ',	# Save objects now
	'no' : 'no  ',	# Do not save objects
	'ask' : 'ask ',	# Ask the user whether to save
}

_Enum_kfrm = {
	'index' : 'indx',	# keyform designating indexed access
	'named' : 'name',	# keyform designating named access
	'id' : 'ID  ',	# keyform designating access by unique identifier
}

_Enum_styl = {
	'plain' : 'plan',	# Plain
	'bold' : 'bold',	# Bold
	'italic' : 'ital',	# Italic
	'outline' : 'outl',	# Outline
	'shadow' : 'shad',	# Shadow
	'underline' : 'undl',	# Underline
	'superscript' : 'spsc',	# Superscript
	'subscript' : 'sbsc',	# Subscript
	'strikethrough' : 'strk',	# Strikethrough
	'small_caps' : 'smcp',	# Small caps
	'all_caps' : 'alcp',	# All capital letters
	'all_lowercase' : 'lowc',	# Lowercase
	'condensed' : 'cond',	# Condensed
	'expanded' : 'pexp',	# Expanded
	'hidden' : 'hidn',	# Hidden
}

class Standard_Suite:

	_argmap_class_info = {
		'_in' : 'wrcd',
	}

	def class_info(self, _object=None, _attributes={}, **_arguments):
		"""class info: Get information about an object class
		Required argument: the object class about which information is requested
		Keyword argument _in: the human language and script system in which to return information
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: a record containing the objectÕs properties and elements
		"""
		_code = 'core'
		_subcode = 'qobj'

		aetools.keysubst(_arguments, self._argmap_class_info)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_close = {
		'saving' : 'savo',
		'saving_in' : 'kfil',
	}

	def close(self, _object, _attributes={}, **_arguments):
		"""close: Close an object
		Required argument: the object to close
		Keyword argument saving: specifies whether changes should be saved before closing
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
		Keyword argument each: the class of the elements to be counted.
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

	_argmap_data_size = {
		'as' : 'rtyp',
	}

	def data_size(self, _object, _attributes={}, **_arguments):
		"""data size: Return the size in bytes of an object
		Required argument: the object whose data size is to be returned
		Keyword argument as: the data type for which the size is calculated
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the size of the object in bytes
		"""
		_code = 'core'
		_subcode = 'dsiz'

		aetools.keysubst(_arguments, self._argmap_data_size)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def delete(self, _object, _attributes={}, **_arguments):
		"""delete: Delete an element from an object
		Required argument: the element to delete
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'delo'

		if _arguments: raise TypeError, 'No optional args expected'
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
		"""duplicate: Duplicate object(s)
		Required argument: the object(s) to duplicate
		Keyword argument to: the new location for the object(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the duplicated object(s)
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

	_argmap_event_info = {
		'_in' : 'wrcd',
	}

	def event_info(self, _object, _attributes={}, **_arguments):
		"""event info: Get information about the Apple events in a suite
		Required argument: the event class of the Apple events for which to return information
		Keyword argument _in: the human language and script system in which to return information
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: a record containing the events and their parameters
		"""
		_code = 'core'
		_subcode = 'gtei'

		aetools.keysubst(_arguments, self._argmap_event_info)
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

	_argmap_make = {
		'new' : 'kocl',
		'at' : 'insh',
		'with_data' : 'data',
		'with_properties' : 'prdt',
	}

	def make(self, _no_object=None, _attributes={}, **_arguments):
		"""make: Make a new element
		Keyword argument new: the class of the new element.
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
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_move = {
		'to' : 'insh',
	}

	def move(self, _object, _attributes={}, **_arguments):
		"""move: Move object(s) to a new location
		Required argument: the object(s) to move
		Keyword argument to: the new location for the object(s)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: to the object(s) after they have been moved
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

	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open the specified object(s)
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
		"""print: Print the specified object(s)
		Required argument: list of objects to print
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

	_argmap_quit = {
		'saving' : 'savo',
	}

	def quit(self, _no_object=None, _attributes={}, **_arguments):
		"""quit: Quit an application program
		Keyword argument saving: specifies whether to save currently open documents
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'quit'

		aetools.keysubst(_arguments, self._argmap_quit)
		if _no_object != None: raise TypeError, 'No direct arg expected'

		aetools.enumsubst(_arguments, 'savo', _Enum_savo)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_save = {
		'_in' : 'kfil',
		'as' : 'fltp',
	}

	def save(self, _object, _attributes={}, **_arguments):
		"""save: Save an object
		Required argument: the object to save
		Keyword argument _in: the file in which to save the object
		Keyword argument as: the file type of the document in which to save the data
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'core'
		_subcode = 'save'

		aetools.keysubst(_arguments, self._argmap_save)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_suite_info = {
		'_in' : 'wrcd',
	}

	def suite_info(self, _object, _attributes={}, **_arguments):
		"""suite info: Get information about event suite(s)
		Required argument: the suite for which to return information
		Keyword argument _in: the human language and script system in which to return information
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: a record containing the suites and their versions
		"""
		_code = 'core'
		_subcode = 'gtsi'

		aetools.keysubst(_arguments, self._argmap_suite_info)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class 'application' ('capp') -- 'An application program'
#        property 'clipboard' ('pcli') '****' -- 'the clipboard' [mutable list]
#        property 'frontmost' ('pisf') 'bool' -- 'Is this the frontmost application?' []
#        property 'name' ('pnam') 'itxt' -- 'the name' []
#        property 'selection' ('sele') 'csel' -- 'the selection visible to the user' [mutable]
#        property 'version' ('vers') 'vers' -- 'the version of the application' []

#    Class 'applications' ('capp') -- 'Every application'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'character' ('cha ') -- 'A character'
#        property 'color' ('colr') 'cRGB' -- 'the color' [mutable]
#        property 'font' ('font') 'ctxt' -- 'the name of the font' [mutable]
#        property 'size' ('ptsz') 'fixd' -- 'the size in points' [mutable]
#        property 'writing code' ('psct') 'intl' -- 'the script system and language' []
#        property 'style' ('txst') 'tsty' -- 'the text style' [mutable]
#        property 'uniform styles' ('ustl') 'tsty' -- 'the text style' []

#    Class 'characters' ('cha ') -- 'Every character'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'document' ('docu') -- 'A document'
#        property 'modified' ('imod') 'bool' -- 'Has the document been modified since the last save?' []

#    Class 'documents' ('docu') -- 'Every document'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'file' ('file') -- 'A file'
#        property 'stationery' ('pspd') 'bool' -- 'Is the file a stationery file?' [mutable]

#    Class 'files' ('file') -- 'Every file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'selection-object' ('csel') -- 'the selection visible to the user'
#        property 'contents' ('pcnt') 'type' -- 'the contents of the selection' []

#    Class 'text' ('ctxt') -- 'Text'
#        property '' ('c@#!') 'type' -- '' [0]
#        property 'font' ('font') 'ctxt' -- 'the name of the font of the first character' [mutable]

#    Class 'text style info' ('tsty') -- 'On and Off styles of text run'
#        property 'on styles' ('onst') 'styl' -- 'the styles that are on for the text' [enum list]
#        property 'off styles' ('ofst') 'styl' -- 'the styles that are off for the text' [enum list]

#    Class 'text style infos' ('tsty') -- 'every text style info'
#        property '' ('c@#!') 'type' -- '' [0]

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
#        property 'visible' ('pvis') 'bool' -- 'Is the window visible?' [mutable]

#    Class 'windows' ('cwin') -- 'Every window'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'insertion point' ('cins') -- 'An insertion location between two objects'

#    Class 'insertion points' ('cins') -- 'Every insertion location'
#        property '' ('c@#!') 'type' -- '' [0]
#    comparison  'starts with' ('bgwt') -- Starts with
#    comparison  'contains' ('cont') -- Contains
#    comparison  'ends with' ('ends') -- Ends with
#    comparison  '=' ('=   ') -- Equal
#    comparison  '>' ('>   ') -- Greater than
#    comparison  '\263' ('>=  ') -- Greater than or equal to
#    comparison  '<' ('<   ') -- Less than
#    comparison  '\262' ('<=  ') -- Less than or equal to
