"""Suite AppleScript Suite: Goodies for Gustav
Level 1, version 1

Generated from flap:System Folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'ascr'

class AppleScript_Suite:

	def activate(self, _no_object=None, _attributes={}, **_arguments):
		"""activate: Bring targeted application program to the front.
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'actv'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def stop_log(self, _no_object=None, _attributes={}, **_arguments):
		"""stop log: 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log0'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def start_log(self, _no_object=None, _attributes={}, **_arguments):
		"""start log: 
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log1'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def copy(self, _no_object=None, _attributes={}, **_arguments):
		"""copy: Copy an object to the clipboard
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'copy'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		aetools.keysubst(_arguments, self._argmap_error)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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
		"""Call‚subroutine: A subroutine call
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

		aetools.keysubst(_arguments, self._argmap_Call_a5_subroutine)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _d6_(self, _object, _attributes={}, **_arguments):
		"""÷: Division
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '/   '

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
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

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class _empty_ae_name(aetools.ComponentItem):
	""" - the undefined value"""
	want = 'undf'

class upper_case(aetools.ComponentItem):
	"""upper case - Text with lower case converted to upper case"""
	want = 'case'

class machines(aetools.ComponentItem):
	"""machines - every computer"""
	want = 'mach'

machine = machines

class zones(aetools.ComponentItem):
	"""zones - every AppleTalk zone"""
	want = 'zone'

zone = zones

class seconds(aetools.ComponentItem):
	"""seconds - more than one second"""
	want = 'scnd'

class item(aetools.ComponentItem):
	"""item - An item of any type"""
	want = 'cobj'
class id(aetools.NProperty):
	"""id - the unique id number of this object"""
	which = 'ID  '
	want = 'long'

items = item

class text_items(aetools.ComponentItem):
	"""text items - """
	want = 'citm'

text_item = text_items

class date(aetools.ComponentItem):
	"""date - Absolute date and time values"""
	want = 'ldt '
class weekday(aetools.NProperty):
	"""weekday - the day of a week of a date"""
	which = 'wkdy'
	want = 'wkdy'
class month(aetools.NProperty):
	"""month - the month of a date"""
	which = 'mnth'
	want = 'mnth'
class day(aetools.NProperty):
	"""day - the day of the month of a date"""
	which = 'day '
	want = 'long'
class year(aetools.NProperty):
	"""year - the year of a date"""
	which = 'year'
	want = 'long'
class time(aetools.NProperty):
	"""time - the time since midnight of a date"""
	which = 'time'
	want = 'long'
class date_string(aetools.NProperty):
	"""date string - the date portion of a date-time value as a string"""
	which = 'dstr'
	want = 'TEXT'
class time_string(aetools.NProperty):
	"""time string - the time portion of a date-time value as a string"""
	which = 'tstr'
	want = 'TEXT'

dates = date

class month(aetools.ComponentItem):
	"""month - a month"""
	want = 'mnth'

months = month

class January(aetools.ComponentItem):
	"""January - It's June in January..."""
	want = 'jan '

class February(aetools.ComponentItem):
	"""February - the month of February"""
	want = 'feb '

class March(aetools.ComponentItem):
	"""March - the month of March"""
	want = 'mar '

class April(aetools.ComponentItem):
	"""April - the month of April"""
	want = 'apr '

class May(aetools.ComponentItem):
	"""May - the very merry month of May"""
	want = 'may '

class June(aetools.ComponentItem):
	"""June - the month of June"""
	want = 'jun '

class July(aetools.ComponentItem):
	"""July - the month of July"""
	want = 'jul '

class August(aetools.ComponentItem):
	"""August - the month of August"""
	want = 'aug '

class September(aetools.ComponentItem):
	"""September - the month of September"""
	want = 'sep '

class October(aetools.ComponentItem):
	"""October - the month of October"""
	want = 'oct '

class November(aetools.ComponentItem):
	"""November - the month of November"""
	want = 'nov '

class December(aetools.ComponentItem):
	"""December - the month of December"""
	want = 'dec '

class weekday(aetools.ComponentItem):
	"""weekday - a weekday"""
	want = 'wkdy'

weekdays = weekday

class Sunday(aetools.ComponentItem):
	"""Sunday - Sunday Bloody Sunday"""
	want = 'sun '

class Monday(aetools.ComponentItem):
	"""Monday - Blue Monday"""
	want = 'mon '

class Tuesday(aetools.ComponentItem):
	"""Tuesday - Ruby Tuesday"""
	want = 'tue '

class Wednesday(aetools.ComponentItem):
	"""Wednesday - Wednesday Week"""
	want = 'wed '

class Thursday(aetools.ComponentItem):
	"""Thursday - Thursday Afternoon"""
	want = 'thu '

class Friday(aetools.ComponentItem):
	"""Friday - Friday"""
	want = 'fri '

class Saturday(aetools.ComponentItem):
	"""Saturday - Saturday Night's Alright for Fighting"""
	want = 'sat '

class RGB_color(aetools.ComponentItem):
	"""RGB color - Three numbers specifying red, green, blue color values"""
	want = 'cRGB'

RGB_colors = RGB_color

class integer(aetools.ComponentItem):
	"""integer - An integral number"""
	want = 'long'

integers = integer

class boolean(aetools.ComponentItem):
	"""boolean - A true or false value"""
	want = 'bool'

booleans = boolean

class real(aetools.ComponentItem):
	"""real - A real number"""
	want = 'doub'

reals = real

class list(aetools.ComponentItem):
	"""list - An ordered collection of items"""
	want = 'list'
class length(aetools.NProperty):
	"""length - the length of a list"""
	which = 'leng'
	want = 'long'

lists = list

class linked_list(aetools.ComponentItem):
	"""linked list - An ordered collection of items"""
	want = 'llst'
# repeated property length the length of a list

linked_lists = linked_list

class vector(aetools.ComponentItem):
	"""vector - An ordered collection of items"""
	want = 'vect'
# repeated property length the length of a list

vectors = vector

class record(aetools.ComponentItem):
	"""record - A set of labeled items"""
	want = 'reco'

records = record

class script(aetools.ComponentItem):
	"""script - An AppleScript script"""
	want = 'scpt'
class parent(aetools.NProperty):
	"""parent - the parent of a script"""
	which = 'pare'
	want = 'scpt'

scripts = script

class string(aetools.ComponentItem):
	"""string - a sequence of characters"""
	want = 'TEXT'

strings = string

class styled_text(aetools.ComponentItem):
	"""styled text - a sequence of characters with style"""
	want = 'STXT'

class number(aetools.ComponentItem):
	"""number - an integer or floating point number"""
	want = 'nmbr'

numbers = number

class _class(aetools.ComponentItem):
	"""class - the type of a value"""
	want = 'pcls'
class inherits(aetools.NProperty):
	"""inherits - classes to inherit properties from"""
	which = 'c@#^'
	want = 'pcls'

classes = _class

class event(aetools.ComponentItem):
	"""event - an AppleEvents event"""
	want = 'evnt'

events = event

class property(aetools.ComponentItem):
	"""property - an AppleEvents property"""
	want = 'prop'

properties = property

class constant(aetools.ComponentItem):
	"""constant - A constant value"""
	want = 'enum'

constants = constant

class preposition(aetools.ComponentItem):
	"""preposition - an AppleEvents preposition"""
	want = 'prep'

prepositions = preposition

class key(aetools.ComponentItem):
	"""key - an AppleEvents key form"""
	want = 'keyf'

keys = key

class picture(aetools.ComponentItem):
	"""picture - A picture"""
	want = 'PICT'

pictures = picture

class reference(aetools.ComponentItem):
	"""reference - An AppleScript reference"""
	want = 'obj '

references = reference

class data(aetools.ComponentItem):
	"""data - An AppleScript raw data object"""
	want = 'rdat'

class handler(aetools.ComponentItem):
	"""handler - An AppleScript handler"""
	want = 'hand'

handlers = handler

class list_or_record(aetools.ComponentItem):
	"""list or record - a list or record"""
	want = 'lr  '

class list_or_string(aetools.ComponentItem):
	"""list or string - a list or string"""
	want = 'ls  '

class list_2c__record_or_text(aetools.ComponentItem):
	"""list, record or text - a list, record or text"""
	want = 'lrs '

class number_or_date(aetools.ComponentItem):
	"""number or date - a number or date"""
	want = 'nd  '

class number_2c__date_or_text(aetools.ComponentItem):
	"""number, date or text - a number, date or text"""
	want = 'nds '

class alias(aetools.ComponentItem):
	"""alias - a reference to an existing file"""
	want = 'alis'

aliases = alias

class application(aetools.ComponentItem):
	"""application - specifies global properties of AppleScript"""
	want = 'capp'
class result(aetools.NProperty):
	"""result - the last result of evaluation"""
	which = 'rslt'
	want = 'cobj'
class space(aetools.NProperty):
	"""space - a space character"""
	which = 'spac'
	want = 'TEXT'
class _return(aetools.NProperty):
	"""return - a return character"""
	which = 'ret '
	want = 'TEXT'
class tab(aetools.NProperty):
	"""tab - a tab character"""
	which = 'tab '
	want = 'TEXT'
class minutes(aetools.NProperty):
	"""minutes - the number of seconds in a minute"""
	which = 'min '
	want = 'TEXT'
class hours(aetools.NProperty):
	"""hours - the number of seconds in an hour"""
	which = 'hour'
	want = 'TEXT'
class days(aetools.NProperty):
	"""days - the number of seconds in a day"""
	which = 'days'
	want = 'TEXT'
class weeks(aetools.NProperty):
	"""weeks - the number of seconds in a week"""
	which = 'week'
	want = 'TEXT'
class pi(aetools.NProperty):
	"""pi - the constant pi"""
	which = 'pi  '
	want = 'doub'
class print_length(aetools.NProperty):
	"""print length - the maximum length to print"""
	which = 'prln'
	want = 'long'
class print_depth(aetools.NProperty):
	"""print depth - the maximum depth to print"""
	which = 'prdp'
	want = 'long'
class reverse(aetools.NProperty):
	"""reverse - the reverse of a list"""
	which = 'rvse'
	want = 'list'
class rest(aetools.NProperty):
	"""rest - the rest of a list"""
	which = 'rest'
	want = 'list'
class text_item_delimiters(aetools.NProperty):
	"""text item delimiters - the text item delimiters of a string"""
	which = 'txdl'
	want = 'list'
class AppleScript(aetools.NProperty):
	"""AppleScript - the top-level script object"""
	which = 'ascr'
	want = 'scpt'

applications = application

app = application

class version(aetools.ComponentItem):
	"""version - a version value"""
	want = 'vers'

class writing_code_info(aetools.ComponentItem):
	"""writing code info - Script code and language code of text run"""
	want = 'citl'
class script_code(aetools.NProperty):
	"""script code - the script code for the text"""
	which = 'pscd'
	want = 'shor'
class language_code(aetools.NProperty):
	"""language code - the language code for the text"""
	which = 'plcd'
	want = 'shor'

writing_code_infos = writing_code_info
_empty_ae_name._propdict = {
}
_empty_ae_name._elemdict = {
}
upper_case._propdict = {
}
upper_case._elemdict = {
}
machines._propdict = {
}
machines._elemdict = {
}
zones._propdict = {
}
zones._elemdict = {
}
seconds._propdict = {
}
seconds._elemdict = {
}
item._propdict = {
	'id' : id,
}
item._elemdict = {
}
text_items._propdict = {
}
text_items._elemdict = {
}
date._propdict = {
	'weekday' : weekday,
	'month' : month,
	'day' : day,
	'year' : year,
	'time' : time,
	'date_string' : date_string,
	'time_string' : time_string,
}
date._elemdict = {
}
month._propdict = {
}
month._elemdict = {
}
January._propdict = {
}
January._elemdict = {
}
February._propdict = {
}
February._elemdict = {
}
March._propdict = {
}
March._elemdict = {
}
April._propdict = {
}
April._elemdict = {
}
May._propdict = {
}
May._elemdict = {
}
June._propdict = {
}
June._elemdict = {
}
July._propdict = {
}
July._elemdict = {
}
August._propdict = {
}
August._elemdict = {
}
September._propdict = {
}
September._elemdict = {
}
October._propdict = {
}
October._elemdict = {
}
November._propdict = {
}
November._elemdict = {
}
December._propdict = {
}
December._elemdict = {
}
weekday._propdict = {
}
weekday._elemdict = {
}
Sunday._propdict = {
}
Sunday._elemdict = {
}
Monday._propdict = {
}
Monday._elemdict = {
}
Tuesday._propdict = {
}
Tuesday._elemdict = {
}
Wednesday._propdict = {
}
Wednesday._elemdict = {
}
Thursday._propdict = {
}
Thursday._elemdict = {
}
Friday._propdict = {
}
Friday._elemdict = {
}
Saturday._propdict = {
}
Saturday._elemdict = {
}
RGB_color._propdict = {
}
RGB_color._elemdict = {
}
integer._propdict = {
}
integer._elemdict = {
}
boolean._propdict = {
}
boolean._elemdict = {
}
real._propdict = {
}
real._elemdict = {
}
list._propdict = {
	'length' : length,
}
list._elemdict = {
}
linked_list._propdict = {
	'length' : length,
}
linked_list._elemdict = {
}
vector._propdict = {
	'length' : length,
}
vector._elemdict = {
}
record._propdict = {
}
record._elemdict = {
}
script._propdict = {
	'parent' : parent,
}
script._elemdict = {
}
string._propdict = {
}
string._elemdict = {
}
styled_text._propdict = {
}
styled_text._elemdict = {
}
number._propdict = {
}
number._elemdict = {
}
_class._propdict = {
	'inherits' : inherits,
}
_class._elemdict = {
}
event._propdict = {
}
event._elemdict = {
}
property._propdict = {
}
property._elemdict = {
}
constant._propdict = {
}
constant._elemdict = {
}
preposition._propdict = {
}
preposition._elemdict = {
}
key._propdict = {
}
key._elemdict = {
}
picture._propdict = {
}
picture._elemdict = {
}
reference._propdict = {
}
reference._elemdict = {
}
data._propdict = {
}
data._elemdict = {
}
handler._propdict = {
}
handler._elemdict = {
}
list_or_record._propdict = {
}
list_or_record._elemdict = {
}
list_or_string._propdict = {
}
list_or_string._elemdict = {
}
list_2c__record_or_text._propdict = {
}
list_2c__record_or_text._elemdict = {
}
number_or_date._propdict = {
}
number_or_date._elemdict = {
}
number_2c__date_or_text._propdict = {
}
number_2c__date_or_text._elemdict = {
}
alias._propdict = {
}
alias._elemdict = {
}
application._propdict = {
	'result' : result,
	'space' : space,
	'_return' : _return,
	'tab' : tab,
	'minutes' : minutes,
	'hours' : hours,
	'days' : days,
	'weeks' : weeks,
	'pi' : pi,
	'print_length' : print_length,
	'print_depth' : print_depth,
	'reverse' : reverse,
	'rest' : rest,
	'text_item_delimiters' : text_item_delimiters,
	'AppleScript' : AppleScript,
}
application._elemdict = {
}
version._propdict = {
}
version._elemdict = {
}
writing_code_info._propdict = {
	'script_code' : script_code,
	'language_code' : language_code,
}
writing_code_info._elemdict = {
}
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


#
# Indices of types declared in this module
#
_classdeclarations = {
	'jan ' : January,
	'PICT' : picture,
	'vers' : version,
	'sat ' : Saturday,
	'nov ' : November,
	'ls  ' : list_or_string,
	'list' : list,
	'cRGB' : RGB_color,
	'citl' : writing_code_info,
	'scnd' : seconds,
	'thu ' : Thursday,
	'keyf' : key,
	'sun ' : Sunday,
	'wkdy' : weekday,
	'rdat' : data,
	'vect' : vector,
	'obj ' : reference,
	'hand' : handler,
	'tue ' : Tuesday,
	'dec ' : December,
	'enum' : constant,
	'nd  ' : number_or_date,
	'wed ' : Wednesday,
	'undf' : _empty_ae_name,
	'reco' : record,
	'capp' : application,
	'cobj' : item,
	'prep' : preposition,
	'mach' : machines,
	'citm' : text_items,
	'bool' : boolean,
	'nmbr' : number,
	'prop' : property,
	'long' : integer,
	'sep ' : September,
	'scpt' : script,
	'pcls' : _class,
	'alis' : alias,
	'mon ' : Monday,
	'lr  ' : list_or_record,
	'jul ' : July,
	'fri ' : Friday,
	'oct ' : October,
	'mar ' : March,
	'ldt ' : date,
	'lrs ' : list_2c__record_or_text,
	'jun ' : June,
	'case' : upper_case,
	'doub' : real,
	'feb ' : February,
	'nds ' : number_2c__date_or_text,
	'llst' : linked_list,
	'STXT' : styled_text,
	'aug ' : August,
	'TEXT' : string,
	'apr ' : April,
	'may ' : May,
	'zone' : zones,
	'evnt' : event,
	'mnth' : month,
}

_propdeclarations = {
	'day ' : day,
	'rslt' : result,
	'time' : time,
	'prln' : print_length,
	'prdp' : print_depth,
	'txdl' : text_item_delimiters,
	'days' : days,
	'spac' : space,
	'hour' : hours,
	'pscd' : script_code,
	'plcd' : language_code,
	'ret ' : _return,
	'tstr' : time_string,
	'tab ' : tab,
	'rvse' : reverse,
	'wkdy' : weekday,
	'ID  ' : id,
	'c@#^' : inherits,
	'ascr' : AppleScript,
	'rest' : rest,
	'dstr' : date_string,
	'min ' : minutes,
	'pi  ' : pi,
	'leng' : length,
	'year' : year,
	'pare' : parent,
	'mnth' : month,
	'week' : weeks,
}

_compdeclarations = {
}

_enumdeclarations = {
	'cons' : _Enum_cons,
	'boov' : _Enum_boov,
	'misc' : _Enum_misc,
}
