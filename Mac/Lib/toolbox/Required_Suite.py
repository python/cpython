"""Suite Required Suite: Terms that every application should support
Level 1, version 1

Generated from Moes:System folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'reqd'

class Required_Suite:

	def open(self, _object, _attributes={}, **_arguments):
		"""open: Open the specified object(s)
		Required argument: list of objects to open
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'odoc'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

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

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def quit(self, _no_object=None, _attributes={}, **_arguments):
		"""quit: Quit application
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'quit'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def run(self, _no_object=None, _attributes={}, **_arguments):
		"""run: Sent to an application when it is double-clicked
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'oapp'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

