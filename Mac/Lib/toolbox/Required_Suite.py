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

	def quit(self, *arguments):
		"""quit: Quit application
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


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def run(self, *arguments):
		"""run: Sent to an application when it is double-clicked
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'aevt'
		_subcode = 'oapp'

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


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

