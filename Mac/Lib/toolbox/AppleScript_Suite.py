"""Suite AppleScript Suite: Goodies for Gustav
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

_code = 'ascr'

_Enum_cons = {
	'case' : 'case',	# case
	'diacriticals' : 'diac',	# diacriticals
	'white_space' : 'whit',	# white space
	'hyphens' : 'hyph',	# hyphens
	'expansion' : 'expa',	# expansion
	'punctuation' : 'punc',	# punctuation
	'application_responses' : 'rmte',	# remote event replies
}

_Enum_boov = {
	'true' : 'true',	# the true boolean value
	'false' : 'fals',	# the false boolean value
}

_Enum_misc = {
	'current_application' : 'cura',	# the current application
}

class AppleScript_Suite:

	def activate(self, _no_object=None, _attributes={}, **_arguments):
		"""activate: Bring targeted application program to the front.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'actv'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def log(self, _object, _attributes={}, **_arguments):
		"""log: Cause a comment to be logged.
		Required argument: anything
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'cmnt'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def stop_log(self, _no_object=None, _attributes={}, **_arguments):
		"""stop log: 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log0'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def start_log(self, _no_object=None, _attributes={}, **_arguments):
		"""start log: 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log1'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def copy(self, _no_object=None, _attributes={}, **_arguments):
		"""copy: Copy an object to the clipboard
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'copy'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def do_script(self, _object, _attributes={}, **_arguments):
		"""do script: Execute a script
		Required argument: the script to execute
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'misc'
		_subcode = 'dosc'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def idle(self, _no_object=None, _attributes={}, **_arguments):
		"""idle: Sent to a script application when it is idle
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: Number of seconds to wait for next idle event
		"""
		_code = 'misc'
		_subcode = 'idle'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def launch(self, _no_object=None, _attributes={}, **_arguments):
		"""launch: Start an application for scripting
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'noop'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def tell(self, _no_object=None, _attributes={}, **_arguments):
		"""tell: Magic tell event for event logging
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'tell'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def end_tell(self, _no_object=None, _attributes={}, **_arguments):
		"""end tell: Start an application for scripting
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'tend'

		if _no_object != None: raise TypeError, 'No direct arg expected'

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_error = {
		'number' : 'errn',
		'partial_result' : 'ptlr',
		'_from' : 'erob',
		'to' : 'errt',
	}

	def error(self, _object=None, _attributes={}, **_arguments):
		"""error: Raise an error
		Required argument: anything
		Keyword argument number: an error number
		Keyword argument partial_result: any partial result occurring before the error
		Keyword argument _from: the object that caused the error
		Keyword argument to: another parameter to the error
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'err '

		_arguments['----'] = _object

		aetools.keysubst(_arguments, self._argmap_error)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Call_a5_subroutine = {
		'at' : 'at  ',
		'_from' : 'from',
		'_for' : 'for ',
		'to' : 'to  ',
		'thru' : 'thru',
		'through' : 'thgh',
		'by' : 'by  ',
		'on' : 'on  ',
		'into' : 'into',
		'onto' : 'onto',
		'between' : 'btwn',
		'against' : 'agst',
		'out_of' : 'outo',
		'instead_of' : 'isto',
		'aside_from' : 'asdf',
		'around' : 'arnd',
		'beside' : 'bsid',
		'beneath' : 'bnth',
		'under' : 'undr',
		'over' : 'over',
		'above' : 'abve',
		'below' : 'belw',
		'apart_from' : 'aprt',
		'given' : 'givn',
		'with' : 'with',
		'without' : 'wout',
		'about' : 'abou',
		'since' : 'snce',
		'until' : 'till',
		'returning' : 'Krtn',
	}

	def Call_a5_subroutine(self, _object=None, _attributes={}, **_arguments):
		"""Call¥subroutine: A subroutine call
		Required argument: anything
		Keyword argument at: a preposition
		Keyword argument _from: a preposition
		Keyword argument _for: a preposition
		Keyword argument to: a preposition
		Keyword argument thru: a preposition
		Keyword argument through: a preposition
		Keyword argument by: a preposition
		Keyword argument on: a preposition
		Keyword argument into: a preposition
		Keyword argument onto: a preposition
		Keyword argument between: a preposition
		Keyword argument against: a preposition
		Keyword argument out_of: a preposition
		Keyword argument instead_of: a preposition
		Keyword argument aside_from: a preposition
		Keyword argument around: a preposition
		Keyword argument beside: a preposition
		Keyword argument beneath: a preposition
		Keyword argument under: a preposition
		Keyword argument over: a preposition
		Keyword argument above: a preposition
		Keyword argument below: a preposition
		Keyword argument apart_from: a preposition
		Keyword argument given: a preposition
		Keyword argument with: special preposition for setting event properties
		Keyword argument without: special preposition for clearing event properties
		Keyword argument about: a preposition
		Keyword argument since: a preposition
		Keyword argument until: a preposition
		Keyword argument returning: specifies a pattern to match results to
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'psbr'

		_arguments['----'] = _object

		aetools.keysubst(_arguments, self._argmap_Call_a5_subroutine)

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _3d_(self, _object, _attributes={}, **_arguments):
		"""=: Equality
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '=   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _ad_(self, _object, _attributes={}, **_arguments):
		"""­: Inequality
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '\255   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _2b_(self, _object, _attributes={}, **_arguments):
		"""+: Addition
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '+   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _2d_(self, _object, _attributes={}, **_arguments):
		"""-: Subtraction
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '-   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _2a_(self, _object, _attributes={}, **_arguments):
		"""*: Multiplication
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '*   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _d6_(self, _object, _attributes={}, **_arguments):
		"""Ö: Division
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '/   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def div(self, _object, _attributes={}, **_arguments):
		"""div: Quotient
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'div '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def mod(self, _object, _attributes={}, **_arguments):
		"""mod: Remainder
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'mod '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _5e_(self, _object, _attributes={}, **_arguments):
		"""^: Exponentiation
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '^   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _3e_(self, _object, _attributes={}, **_arguments):
		""">: Greater than
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '>   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _b3_(self, _object, _attributes={}, **_arguments):
		"""³: Greater than or equal to
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '>=  '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _3c_(self, _object, _attributes={}, **_arguments):
		"""<: Less than
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '<   '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _b2_(self, _object, _attributes={}, **_arguments):
		"""²: Less than or equal to
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '<=  '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _26_(self, _object, _attributes={}, **_arguments):
		"""&: Concatenation
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'ccat'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def starts_with(self, _object, _attributes={}, **_arguments):
		"""starts with: Starts with
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'bgwt'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def ends_with(self, _object, _attributes={}, **_arguments):
		"""ends with: Ends with
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'ends'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def contains(self, _object, _attributes={}, **_arguments):
		"""contains: Containment
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'cont'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _and(self, _object, _attributes={}, **_arguments):
		"""and: Logical conjunction
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'AND '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _or(self, _object, _attributes={}, **_arguments):
		"""or: Logical disjunction
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'OR  '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def as(self, _object, _attributes={}, **_arguments):
		"""as: Coercion
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'coer'

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _not(self, _object, _attributes={}, **_arguments):
		"""not: Logical negation
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'NOT '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def negate(self, _object, _attributes={}, **_arguments):
		"""negate: Numeric negation
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'neg '

		_arguments['----'] = _object

		if _arguments: raise TypeError, 'No optional args expected'

		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


#    Class '' ('undf') -- 'the undefined value'

#    Class 'upper case' ('case') -- 'Text with lower case converted to upper case'

#    Class 'machines' ('mach') -- 'every computer'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'machine' ('mach') -- 'A computer'

#    Class 'zones' ('zone') -- 'every AppleTalk zone'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'zone' ('zone') -- 'AppleTalk zone'

#    Class 'seconds' ('scnd') -- 'more than one second'

#    Class 'item' ('cobj') -- 'An item of any type'
#        property 'id' ('ID  ') 'long' -- 'the unique id number of this object' [mutable]

#    Class 'items' ('cobj') -- 'Every item'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'text items' ('citm') -- ''
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'text item' ('citm') -- ''

#    Class 'date' ('ldt ') -- 'Absolute date and time values'
#        property 'weekday' ('wkdy') 'wkdy' -- 'the day of a week of a date' []
#        property 'month' ('mnth') 'mnth' -- 'the month of a date' []
#        property 'day' ('day ') 'long' -- 'the day of the month of a date' []
#        property 'year' ('year') 'long' -- 'the year of a date' []
#        property 'time' ('time') 'long' -- 'the time since midnight of a date' []
#        property 'date string' ('dstr') 'TEXT' -- 'the date portion of a date-time value as a string' []
#        property 'time string' ('tstr') 'TEXT' -- 'the time portion of a date-time value as a string' []

#    Class 'dates' ('ldt ') -- 'every date'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'month' ('mnth') -- 'a month'

#    Class 'months' ('mnth') -- 'every month'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'January' ('jan ') -- "It's June in January..."

#    Class 'February' ('feb ') -- 'the month of February'

#    Class 'March' ('mar ') -- 'the month of March'

#    Class 'April' ('apr ') -- 'the month of April'

#    Class 'May' ('may ') -- 'the very merry month of May'

#    Class 'June' ('jun ') -- 'the month of June'

#    Class 'July' ('jul ') -- 'the month of July'

#    Class 'August' ('aug ') -- 'the month of August'

#    Class 'September' ('sep ') -- 'the month of September'

#    Class 'October' ('oct ') -- 'the month of October'

#    Class 'November' ('nov ') -- 'the month of November'

#    Class 'December' ('dec ') -- 'the month of December'

#    Class 'weekday' ('wkdy') -- 'a weekday'

#    Class 'weekdays' ('wkdy') -- 'every weekday'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'Sunday' ('sun ') -- 'Sunday Bloody Sunday'

#    Class 'Monday' ('mon ') -- 'Blue Monday'

#    Class 'Tuesday' ('tue ') -- 'Ruby Tuesday'

#    Class 'Wednesday' ('wed ') -- 'Wednesday Week'

#    Class 'Thursday' ('thu ') -- 'Thursday Afternoon'

#    Class 'Friday' ('fri ') -- 'Friday'

#    Class 'Saturday' ('sat ') -- "Saturday Night's Alright for Fighting"

#    Class 'RGB color' ('cRGB') -- 'Three numbers specifying red, green, blue color values'

#    Class 'RGB colors' ('cRGB') -- 'every RGB color'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'integer' ('long') -- 'An integral number'

#    Class 'integers' ('long') -- 'every integer'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'boolean' ('bool') -- 'A true or false value'

#    Class 'booleans' ('bool') -- 'every boolean'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'real' ('doub') -- 'A real number'

#    Class 'reals' ('doub') -- 'every real'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'list' ('list') -- 'An ordered collection of items'
#        property 'length' ('leng') 'long' -- 'the length of a list' []

#    Class 'lists' ('list') -- 'every list'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'linked list' ('llst') -- 'An ordered collection of items'
#        property 'length' ('leng') 'long' -- 'the length of a list' []

#    Class 'linked lists' ('llst') -- 'every linked list'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'vector' ('vect') -- 'An ordered collection of items'
#        property 'length' ('leng') 'long' -- 'the length of a list' []

#    Class 'vectors' ('vect') -- 'every vector'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'record' ('reco') -- 'A set of labeled items'

#    Class 'records' ('reco') -- 'every record'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'script' ('scpt') -- 'An AppleScript script'
#        property 'parent' ('pare') 'scpt' -- 'the parent of a script' []

#    Class 'scripts' ('scpt') -- 'every script'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'string' ('TEXT') -- 'a sequence of characters'

#    Class 'strings' ('TEXT') -- 'every string'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'styled text' ('STXT') -- 'a sequence of characters with style'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'number' ('nmbr') -- 'an integer or floating point number'

#    Class 'numbers' ('nmbr') -- 'every number'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'class' ('pcls') -- 'the type of a value'
#        property 'inherits' ('c@#^') 'pcls' -- 'classes to inherit properties from' []

#    Class 'classes' ('pcls') -- 'every class'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'event' ('evnt') -- 'an AppleEvents event'

#    Class 'events' ('evnt') -- 'every event'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'property' ('prop') -- 'an AppleEvents property'

#    Class 'properties' ('prop') -- 'every property'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'constant' ('enum') -- 'A constant value'

#    Class 'constants' ('enum') -- 'every constant'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'preposition' ('prep') -- 'an AppleEvents preposition'

#    Class 'prepositions' ('prep') -- 'every preposition'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'key' ('keyf') -- 'an AppleEvents key form'

#    Class 'keys' ('keyf') -- 'every key'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'picture' ('PICT') -- 'A picture'

#    Class 'pictures' ('PICT') -- 'every picture'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'reference' ('obj ') -- 'An AppleScript reference'

#    Class 'references' ('obj ') -- 'every reference'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'data' ('rdat') -- 'An AppleScript raw data object'

#    Class 'handler' ('hand') -- 'An AppleScript handler'

#    Class 'handlers' ('hand') -- 'every handler'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'list or record' ('lr  ') -- 'a list or record'

#    Class 'list or string' ('ls  ') -- 'a list or string'

#    Class 'list, record or text' ('lrs ') -- 'a list, record or text'

#    Class 'number or date' ('nd  ') -- 'a number or date'

#    Class 'number, date or text' ('nds ') -- 'a number, date or text'

#    Class 'alias' ('alis') -- 'a reference to an existing file'

#    Class 'aliases' ('alis') -- 'every alias'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'application' ('capp') -- 'specifies global properties of AppleScript'
#        property 'result' ('rslt') 'cobj' -- 'the last result of evaluation' []
#        property 'space' ('spac') 'TEXT' -- 'a space character' []
#        property 'return' ('ret ') 'TEXT' -- 'a return character' []
#        property 'tab' ('tab ') 'TEXT' -- 'a tab character' []
#        property 'minutes' ('min ') 'TEXT' -- 'the number of seconds in a minute' []
#        property 'hours' ('hour') 'TEXT' -- 'the number of seconds in an hour' []
#        property 'days' ('days') 'TEXT' -- 'the number of seconds in a day' []
#        property 'weeks' ('week') 'TEXT' -- 'the number of seconds in a week' []
#        property 'pi' ('pi  ') 'doub' -- 'the constant pi' []
#        property 'print length' ('prln') 'long' -- 'the maximum length to print' []
#        property 'print depth' ('prdp') 'long' -- 'the maximum depth to print' []
#        property 'reverse' ('rvse') 'list' -- 'the reverse of a list' []
#        property 'rest' ('rest') 'list' -- 'the rest of a list' []
#        property 'text item delimiters' ('txdl') 'list' -- 'the text item delimiters of a string' []
#        property 'AppleScript' ('ascr') 'scpt' -- 'the top-level script object' []

#    Class 'applications' ('capp') -- 'every application'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'app' ('capp') -- 'Short name for application'

#    Class 'version' ('vers') -- 'a version value'

#    Class 'writing code info' ('citl') -- 'Script code and language code of text run'
#        property 'script code' ('pscd') 'shor' -- 'the script code for the text' []
#        property 'language code' ('plcd') 'shor' -- 'the language code for the text' []

#    Class 'writing code infos' ('citl') -- 'every writing code info'
#        property '' ('c@#!') 'type' -- '' [0]
