"""Suite 7.0 Finder Suite: This is the original Finder suite. These events will be supported in the future, but the syntax may be changed in a future Finder release.
Level 1, version 1

Generated from flap:System Folder:Finder
AETE/AEUT resource version 0/149, language 0, script 0
"""
# XXXX Hand edited to change the classname

import aetools
import MacOS

_code = 'FNDR'

class Finder_7_0_Suite:

	def open_about_box(self, _no_object=None, _attributes={}, **_arguments):
		"""open about box: Open the 'About This Mac' window
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'abou'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_copy_to = {
		'_from' : 'fsel',
	}

	def copy_to(self, _object, _attributes={}, **_arguments):
		"""copy to: Copies one or more items into a folder
		Required argument: Alias for folder into which the items are copied
		Keyword argument _from: List of aliases for items to be copied
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'drag'

		aetools.keysubst(_arguments, self._argmap_copy_to)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_duplicate = {
		'items' : 'fsel',
	}

	def duplicate(self, _object, _attributes={}, **_arguments):
		"""duplicate: Duplicate a set of items in a folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases for items in the folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'sdup'

		aetools.keysubst(_arguments, self._argmap_duplicate)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def empty_trash(self, _no_object=None, _attributes={}, **_arguments):
		"""empty trash: Empties the trash
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'empt'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_make_aliases_for = {
		'items' : 'fsel',
	}

	def make_aliases_for(self, _object, _attributes={}, **_arguments):
		"""make aliases for: Make aliases to items from a single folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases for items in folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'sali'

		aetools.keysubst(_arguments, self._argmap_make_aliases_for)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_move_to = {
		'_from' : 'fsel',
	}

	def move_to(self, _object, _attributes={}, **_arguments):
		"""move to: Move one or more items into a folder
		Required argument: Alias for destination folder
		Keyword argument _from: List of aliases for items to be moved 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'move'

		aetools.keysubst(_arguments, self._argmap_move_to)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def sleep(self, _no_object=None, _attributes={}, **_arguments):
		"""sleep: Put portable into sleep mode
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'slep'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def shut_down(self, _no_object=None, _attributes={}, **_arguments):
		"""shut down: Shuts down the Macintosh if all applications can quit
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'shut'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_open = {
		'items' : 'fsel',
	}

	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open folders, files, or applications from a given folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases for items in the folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'sope'

		aetools.keysubst(_arguments, self._argmap_open)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap__print = {
		'items' : 'fsel',
	}

	def _print(self, _object, _attributes={}, **_arguments):
		"""print: Print items from a given folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases for items in folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'spri'

		aetools.keysubst(_arguments, self._argmap__print)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_put_away = {
		'items' : 'fsel',
	}

	def put_away(self, _object, _attributes={}, **_arguments):
		"""put away: Put away items from a given folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases to items in folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: undocumented, typecode 'alis'
		"""
		_code = 'FNDR'
		_subcode = 'sput'

		aetools.keysubst(_arguments, self._argmap_put_away)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def restart(self, _no_object=None, _attributes={}, **_arguments):
		"""restart: Restart the Macintosh
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'rest'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_select = {
		'items' : 'fsel',
	}

	def select(self, _object, _attributes={}, **_arguments):
		"""select: Select items in a folder
		Required argument: Alias for folder containing the items
		Keyword argument items: List of aliases for items in folder
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'FNDR'
		_subcode = 'srev'

		aetools.keysubst(_arguments, self._argmap_select)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#
# Indices of types declared in this module
#
_classdeclarations = {
}

_propdeclarations = {
}

_compdeclarations = {
}

_enumdeclarations = {
}
