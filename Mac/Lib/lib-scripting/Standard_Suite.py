"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from flap:System Folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'core'

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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
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
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - An application program"""
	want = 'capp'
class clipboard(aetools.NProperty):
	"""clipboard - the clipboard"""
	which = 'pcli'
	want = '****'
class frontmost(aetools.NProperty):
	"""frontmost - Is this the frontmost application?"""
	which = 'pisf'
	want = 'bool'
class name(aetools.NProperty):
	"""name - the name"""
	which = 'pnam'
	want = 'itxt'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user"""
	which = 'sele'
	want = 'csel'
class version(aetools.NProperty):
	"""version - the version of the application"""
	which = 'vers'
	want = 'vers'

applications = application

class character(aetools.ComponentItem):
	"""character - A character"""
	want = 'cha '
class color(aetools.NProperty):
	"""color - the color"""
	which = 'colr'
	want = 'cRGB'
class font(aetools.NProperty):
	"""font - the name of the font"""
	which = 'font'
	want = 'ctxt'
class size(aetools.NProperty):
	"""size - the size in points"""
	which = 'ptsz'
	want = 'fixd'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language"""
	which = 'psct'
	want = 'intl'
class style(aetools.NProperty):
	"""style - the text style"""
	which = 'txst'
	want = 'tsty'
class uniform_styles(aetools.NProperty):
	"""uniform styles - the text style"""
	which = 'ustl'
	want = 'tsty'

characters = character

class document(aetools.ComponentItem):
	"""document - A document"""
	want = 'docu'
class modified(aetools.NProperty):
	"""modified - Has the document been modified since the last save?"""
	which = 'imod'
	want = 'bool'

documents = document

class file(aetools.ComponentItem):
	"""file - A file"""
	want = 'file'
class stationery(aetools.NProperty):
	"""stationery - Is the file a stationery file?"""
	which = 'pspd'
	want = 'bool'

files = file

class selection_2d_object(aetools.ComponentItem):
	"""selection-object - the selection visible to the user"""
	want = 'csel'
class contents(aetools.NProperty):
	"""contents - the contents of the selection"""
	which = 'pcnt'
	want = 'type'

class text(aetools.ComponentItem):
	"""text - Text"""
	want = 'ctxt'
# repeated property font the name of the font of the first character

class text_style_info(aetools.ComponentItem):
	"""text style info - On and Off styles of text run"""
	want = 'tsty'
class on_styles(aetools.NProperty):
	"""on styles - the styles that are on for the text"""
	which = 'onst'
	want = 'styl'
class off_styles(aetools.NProperty):
	"""off styles - the styles that are off for the text"""
	which = 'ofst'
	want = 'styl'

text_style_infos = text_style_info

class window(aetools.ComponentItem):
	"""window - A window"""
	want = 'cwin'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window"""
	which = 'pbnd'
	want = 'qdrt'
class closeable(aetools.NProperty):
	"""closeable - Does the window have a close box?"""
	which = 'hclb'
	want = 'bool'
class titled(aetools.NProperty):
	"""titled - Does the window have a title bar?"""
	which = 'ptit'
	want = 'bool'
class index(aetools.NProperty):
	"""index - the number of the window"""
	which = 'pidx'
	want = 'long'
class floating(aetools.NProperty):
	"""floating - Does the window float?"""
	which = 'isfl'
	want = 'bool'
class modal(aetools.NProperty):
	"""modal - Is the window modal?"""
	which = 'pmod'
	want = 'bool'
class resizable(aetools.NProperty):
	"""resizable - Is the window resizable?"""
	which = 'prsz'
	want = 'bool'
class zoomable(aetools.NProperty):
	"""zoomable - Is the window zoomable?"""
	which = 'iszm'
	want = 'bool'
class zoomed(aetools.NProperty):
	"""zoomed - Is the window zoomed?"""
	which = 'pzum'
	want = 'bool'
class visible(aetools.NProperty):
	"""visible - Is the window visible?"""
	which = 'pvis'
	want = 'bool'

windows = window

class insertion_point(aetools.ComponentItem):
	"""insertion point - An insertion location between two objects"""
	want = 'cins'

insertion_points = insertion_point
application._propdict = {
	'clipboard' : clipboard,
	'frontmost' : frontmost,
	'name' : name,
	'selection' : selection,
	'version' : version,
}
application._elemdict = {
}
character._propdict = {
	'color' : color,
	'font' : font,
	'size' : size,
	'writing_code' : writing_code,
	'style' : style,
	'uniform_styles' : uniform_styles,
}
character._elemdict = {
}
document._propdict = {
	'modified' : modified,
}
document._elemdict = {
}
file._propdict = {
	'stationery' : stationery,
}
file._elemdict = {
}
selection_2d_object._propdict = {
	'contents' : contents,
}
selection_2d_object._elemdict = {
}
text._propdict = {
	'font' : font,
}
text._elemdict = {
}
text_style_info._propdict = {
	'on_styles' : on_styles,
	'off_styles' : off_styles,
}
text_style_info._elemdict = {
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
	'visible' : visible,
}
window._elemdict = {
}
insertion_point._propdict = {
}
insertion_point._elemdict = {
}
class starts_with(aetools.NComparison):
	"""starts with - Starts with"""
class contains(aetools.NComparison):
	"""contains - Contains"""
class ends_with(aetools.NComparison):
	"""ends with - Ends with"""
class _3d_(aetools.NComparison):
	"""= - Equal"""
class _3e_(aetools.NComparison):
	"""> - Greater than"""
class _b3_(aetools.NComparison):
	"""³ - Greater than or equal to"""
class _3c_(aetools.NComparison):
	"""< - Less than"""
class _b2_(aetools.NComparison):
	"""² - Less than or equal to"""
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


#
# Indices of types declared in this module
#
_classdeclarations = {
	'docu' : document,
	'tsty' : text_style_info,
	'ctxt' : text,
	'capp' : application,
	'csel' : selection_2d_object,
	'file' : file,
	'cwin' : window,
	'cha ' : character,
	'cins' : insertion_point,
}

_propdeclarations = {
	'ptit' : titled,
	'onst' : on_styles,
	'pnam' : name,
	'pcli' : clipboard,
	'ustl' : uniform_styles,
	'psct' : writing_code,
	'txst' : style,
	'pvis' : visible,
	'pspd' : stationery,
	'pisf' : frontmost,
	'sele' : selection,
	'pmod' : modal,
	'imod' : modified,
	'ofst' : off_styles,
	'ptsz' : size,
	'pzum' : zoomed,
	'hclb' : closeable,
	'font' : font,
	'pcnt' : contents,
	'isfl' : floating,
	'pidx' : index,
	'iszm' : zoomable,
	'colr' : color,
	'pbnd' : bounds,
	'vers' : version,
	'prsz' : resizable,
}

_compdeclarations = {
	'>   ' : _3e_,
	'bgwt' : starts_with,
	'>=  ' : _b3_,
	'=   ' : _3d_,
	'<=  ' : _b2_,
	'cont' : contains,
	'ends' : ends_with,
	'<   ' : _3c_,
}

_enumdeclarations = {
	'styl' : _Enum_styl,
	'savo' : _Enum_savo,
	'kfrm' : _Enum_kfrm,
}
