"""Suite AppleScript Suite: Standard terms for AppleScript
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Extensies/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'ascr'

class AppleScript_Suite_Events:

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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_Call_a5_subroutine = {
		'at' : 'at  ',
		'from_' : 'from',
		'for_' : 'for ',
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
		'about' : 'abou',
		'since' : 'snce',
		'given' : 'givn',
		'with' : 'with',
		'without' : 'wout',
	}

	def Call_a5_subroutine(self, _object=None, _attributes={}, **_arguments):
		"""Call\xa5subroutine: A subroutine call
		Required argument: anything
		Keyword argument at: a preposition
		Keyword argument from_: a preposition
		Keyword argument for_: a preposition
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
		Keyword argument about: a preposition
		Keyword argument since: a preposition
		Keyword argument given: parameter:value pairs, comma-separated
		Keyword argument with: formal parameter set to true if matching actual parameter is provided
		Keyword argument without: formal parameter set to false if matching actual parmeter is provided
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = 'psbr'

		aetools.keysubst(_arguments, self._argmap_Call_a5_subroutine)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def activate(self, _no_object=None, _attributes={}, **_arguments):
		"""activate: Bring the targeted application program to the front
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'actv'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def and_(self, _object, _attributes={}, **_arguments):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def end_tell(self, _no_object=None, _attributes={}, **_arguments):
		"""end tell: Record or log an \xd4end tell\xd5 statement
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'tend'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_error = {
		'number' : 'errn',
		'partial_result' : 'ptlr',
		'from_' : 'erob',
		'to' : 'errt',
	}

	def error(self, _object=None, _attributes={}, **_arguments):
		"""error: Raise an error
		Required argument: anything
		Keyword argument number: an error number
		Keyword argument partial_result: any partial result occurring before the error
		Keyword argument from_: the object that caused the error
		Keyword argument to: the desired class for a failed coercion
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'err '

		aetools.keysubst(_arguments, self._argmap_error)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def idle(self, _no_object=None, _attributes={}, **_arguments):
		"""idle: Sent to a script application when it is idle
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the number of seconds to wait for next idle event
		"""
		_code = 'misc'
		_subcode = 'idle'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def launch(self, _no_object=None, _attributes={}, **_arguments):
		"""launch: Start an application for scripting
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'noop'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def log(self, _object, _attributes={}, **_arguments):
		"""log: Cause a comment to be logged
		Required argument: undocumented, typecode 'TEXT'
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'cmnt'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def not_(self, _object, _attributes={}, **_arguments):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def or_(self, _object, _attributes={}, **_arguments):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def start_log(self, _no_object=None, _attributes={}, **_arguments):
		"""start log: Start event logging in the script editor
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log1'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def stop_log(self, _no_object=None, _attributes={}, **_arguments):
		"""stop log: Stop event logging in the script editor
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ToyS'
		_subcode = 'log0'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def tell(self, _no_object=None, _attributes={}, **_arguments):
		"""tell: Record or log a \xd4tell\xd5 statement
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'ascr'
		_subcode = 'tell'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _ad_(self, _object, _attributes={}, **_arguments):
		"""\xad: Inequality
		Required argument: an AE object reference
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: anything
		"""
		_code = 'ascr'
		_subcode = '\xad   '

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _b2_(self, _object, _attributes={}, **_arguments):
		"""\xb2: Less than or equal to
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _b3_(self, _object, _attributes={}, **_arguments):
		"""\xb3: Greater than or equal to
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def _d6_(self, _object, _attributes={}, **_arguments):
		"""\xd6: Division
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class empty_ae_name_(aetools.ComponentItem):
	""" - the undefined value """
	want = 'undf'

class April(aetools.ComponentItem):
	"""April - the month of April """
	want = 'apr '

class August(aetools.ComponentItem):
	"""August - the month of August """
	want = 'aug '

class C_string(aetools.ComponentItem):
	"""C string - text followed by a null """
	want = 'cstr'

C_strings = C_string

class December(aetools.ComponentItem):
	"""December - the month of December """
	want = 'dec '

class February(aetools.ComponentItem):
	"""February - the month of February """
	want = 'feb '

class Friday(aetools.ComponentItem):
	"""Friday - Friday """
	want = 'fri '

class January(aetools.ComponentItem):
	"""January - the month of January """
	want = 'jan '

class July(aetools.ComponentItem):
	"""July - the month of July """
	want = 'jul '

class June(aetools.ComponentItem):
	"""June - the month of June """
	want = 'jun '

class March(aetools.ComponentItem):
	"""March - the month of March """
	want = 'mar '

class May(aetools.ComponentItem):
	"""May - the month of May """
	want = 'may '

class Monday(aetools.ComponentItem):
	"""Monday - Monday """
	want = 'mon '

class November(aetools.ComponentItem):
	"""November - the month of November """
	want = 'nov '

class October(aetools.ComponentItem):
	"""October - the month of October """
	want = 'oct '

class Pascal_string(aetools.ComponentItem):
	"""Pascal string - text up to 255 characters preceded by a length byte """
	want = 'pstr'

Pascal_strings = Pascal_string

class RGB_color(aetools.ComponentItem):
	"""RGB color - Three integers specifying red, green, blue color values """
	want = 'cRGB'

RGB_colors = RGB_color

class Saturday(aetools.ComponentItem):
	"""Saturday - Saturday """
	want = 'sat '

class September(aetools.ComponentItem):
	"""September - the month of September """
	want = 'sep '

class Sunday(aetools.ComponentItem):
	"""Sunday - Sunday """
	want = 'sun '

class Thursday(aetools.ComponentItem):
	"""Thursday - Thursday """
	want = 'thu '

class Tuesday(aetools.ComponentItem):
	"""Tuesday - Tuesday """
	want = 'tue '

class Unicode_text(aetools.ComponentItem):
	"""Unicode text -  """
	want = 'utxt'

Unicode_text = Unicode_text

class Wednesday(aetools.ComponentItem):
	"""Wednesday - Wednesday """
	want = 'wed '

class alias(aetools.ComponentItem):
	"""alias - a file on a disk or server.  The file must exist when you check the syntax of your script. """
	want = 'alis'

class alias_or_string(aetools.ComponentItem):
	"""alias or string - an alias or string """
	want = 'sf  '

aliases = alias

class anything(aetools.ComponentItem):
	"""anything - any class or reference """
	want = '****'

class app(aetools.ComponentItem):
	"""app - Short name for application """
	want = 'capp'

application = app
class result(aetools.NProperty):
	"""result - the last result of evaluation """
	which = 'rslt'
	want = '****'
class space(aetools.NProperty):
	"""space - a space character """
	which = 'spac'
	want = 'cha '
class return_(aetools.NProperty):
	"""return - a return character """
	which = 'ret '
	want = 'cha '
class tab(aetools.NProperty):
	"""tab - a tab character """
	which = 'tab '
	want = 'cha '
class minutes(aetools.NProperty):
	"""minutes - the number of seconds in a minute """
	which = 'min '
	want = 'long'
class hours(aetools.NProperty):
	"""hours - the number of seconds in an hour """
	which = 'hour'
	want = 'long'
class days(aetools.NProperty):
	"""days - the number of seconds in a day """
	which = 'days'
	want = 'long'
class weeks(aetools.NProperty):
	"""weeks - the number of seconds in a week """
	which = 'week'
	want = 'long'
class pi(aetools.NProperty):
	"""pi - the constant pi """
	which = 'pi  '
	want = 'doub'
class print_length(aetools.NProperty):
	"""print length - the maximum length to print """
	which = 'prln'
	want = 'long'
class print_depth(aetools.NProperty):
	"""print depth - the maximum depth to print """
	which = 'prdp'
	want = 'long'
class text_item_delimiters(aetools.NProperty):
	"""text item delimiters - the text item delimiters of a string """
	which = 'txdl'
	want = 'list'
class AppleScript(aetools.NProperty):
	"""AppleScript - the top-level script object """
	which = 'ascr'
	want = 'scpt'

applications = app

class boolean(aetools.ComponentItem):
	"""boolean - A true or false value """
	want = 'bool'

booleans = boolean

class centimeters(aetools.ComponentItem):
	"""centimeters - a distance measurement in SI centimeters """
	want = 'cmtr'

centimetres = centimeters

class character(aetools.ComponentItem):
	"""character - an individual text character """
	want = 'cha '

characters = character

class class_(aetools.ComponentItem):
	"""class - the type of a value """
	want = 'pcls'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from this class """
	which = 'c@#^'
	want = 'type'

classes = class_

class constant(aetools.ComponentItem):
	"""constant - A constant value """
	want = 'enum'

constants = constant

class cubic_centimeters(aetools.ComponentItem):
	"""cubic centimeters - a volume measurement in SI cubic centimeters """
	want = 'ccmt'

cubic_centimetres = cubic_centimeters

class cubic_feet(aetools.ComponentItem):
	"""cubic feet - a volume measurement in Imperial cubic feet """
	want = 'cfet'

class cubic_inches(aetools.ComponentItem):
	"""cubic inches - a volume measurement in Imperial cubic inches """
	want = 'cuin'

class cubic_meters(aetools.ComponentItem):
	"""cubic meters - a volume measurement in SI cubic meters """
	want = 'cmet'

cubic_metres = cubic_meters

class cubic_yards(aetools.ComponentItem):
	"""cubic yards - a distance measurement in Imperial cubic yards """
	want = 'cyrd'

class data(aetools.ComponentItem):
	"""data - an AppleScript raw data object """
	want = 'rdat'

class date(aetools.ComponentItem):
	"""date - Absolute date and time values """
	want = 'ldt '
class weekday(aetools.NProperty):
	"""weekday - the day of a week of a date """
	which = 'wkdy'
	want = 'wkdy'
class month(aetools.NProperty):
	"""month - the month of a date """
	which = 'mnth'
	want = 'mnth'
class day(aetools.NProperty):
	"""day - the day of the month of a date """
	which = 'day '
	want = 'long'
class year(aetools.NProperty):
	"""year - the year of a date """
	which = 'year'
	want = 'long'
class time(aetools.NProperty):
	"""time - the time since midnight of a date """
	which = 'time'
	want = 'long'
class date_string(aetools.NProperty):
	"""date string - the date portion of a date-time value as text """
	which = 'dstr'
	want = 'TEXT'
class time_string(aetools.NProperty):
	"""time string - the time portion of a date-time value as text """
	which = 'tstr'
	want = 'TEXT'

dates = date

class degrees_Celsius(aetools.ComponentItem):
	"""degrees Celsius - a temperature measurement in SI degrees Celsius """
	want = 'degc'

class degrees_Fahrenheit(aetools.ComponentItem):
	"""degrees Fahrenheit - a temperature measurement in degrees Fahrenheit """
	want = 'degf'

class degrees_Kelvin(aetools.ComponentItem):
	"""degrees Kelvin - a temperature measurement in degrees Kelvin """
	want = 'degk'

class encoded_string(aetools.ComponentItem):
	"""encoded string - text encoded using the Text Encoding Converter """
	want = 'encs'

encoded_strings = encoded_string

class event(aetools.ComponentItem):
	"""event - an AppleEvents event """
	want = 'evnt'

events = event

class feet(aetools.ComponentItem):
	"""feet - a distance measurement in Imperial feet """
	want = 'feet'

class file_specification(aetools.ComponentItem):
	"""file specification - a file specification as used by the operating system """
	want = 'fss '

file_specifications = file_specification

class gallons(aetools.ComponentItem):
	"""gallons - a volume measurement in Imperial gallons """
	want = 'galn'

class grams(aetools.ComponentItem):
	"""grams - a mass measurement in SI meters """
	want = 'gram'

class handler(aetools.ComponentItem):
	"""handler - an AppleScript event or subroutine handler """
	want = 'hand'

handlers = handler

class inches(aetools.ComponentItem):
	"""inches - a distance measurement in Imperial inches """
	want = 'inch'

class integer(aetools.ComponentItem):
	"""integer - An integral number """
	want = 'long'

integers = integer

class international_text(aetools.ComponentItem):
	"""international text -  """
	want = 'itxt'

international_text = international_text

class item(aetools.ComponentItem):
	"""item - An item of any type """
	want = 'cobj'
class id(aetools.NProperty):
	"""id - the unique ID number of this object """
	which = 'ID  '
	want = 'long'

items = item

class keystroke(aetools.ComponentItem):
	"""keystroke - a press of a key combination on a Macintosh keyboard """
	want = 'kprs'
class key(aetools.NProperty):
	"""key - the character for the key was pressed (ignoring modifiers) """
	which = 'kMsg'
	want = 'cha '
class modifiers(aetools.NProperty):
	"""modifiers - the modifier keys pressed in combination """
	which = 'kMod'
	want = 'eMds'
class key_kind(aetools.NProperty):
	"""key kind - the kind of key that was pressed """
	which = 'kknd'
	want = 'ekst'

keystrokes = keystroke

class kilograms(aetools.ComponentItem):
	"""kilograms - a mass measurement in SI kilograms """
	want = 'kgrm'

class kilometers(aetools.ComponentItem):
	"""kilometers - a distance measurement in SI kilometers """
	want = 'kmtr'

kilometres = kilometers

class linked_list(aetools.ComponentItem):
	"""linked list - An ordered collection of items """
	want = 'llst'
class length(aetools.NProperty):
	"""length - the length of a list """
	which = 'leng'
	want = 'long'

linked_lists = linked_list

class list(aetools.ComponentItem):
	"""list - An ordered collection of items """
	want = 'list'
class reverse(aetools.NProperty):
	"""reverse - the items of the list in reverse order """
	which = 'rvse'
	want = 'list'
class rest(aetools.NProperty):
	"""rest - all items of the list excluding first """
	which = 'rest'
	want = 'list'

class list_or_record(aetools.ComponentItem):
	"""list or record - a list or record """
	want = 'lr  '

class list_or_string(aetools.ComponentItem):
	"""list or string - a list or string """
	want = 'ls  '

class list_2c__record_or_text(aetools.ComponentItem):
	"""list, record or text - a list, record or text """
	want = 'lrs '

lists = list

class liters(aetools.ComponentItem):
	"""liters - a volume measurement in SI liters """
	want = 'litr'

litres = liters

class machine(aetools.ComponentItem):
	"""machine - a computer """
	want = 'mach'

machines = machine

class meters(aetools.ComponentItem):
	"""meters - a distance measurement in SI meters """
	want = 'metr'

metres = meters

class miles(aetools.ComponentItem):
	"""miles - a distance measurement in Imperial miles """
	want = 'mile'

class missing_value(aetools.ComponentItem):
	"""missing value - unavailable value, such as properties missing from heterogeneous classes in a Whose clause """
	want = 'msng'

missing_values = missing_value

class month(aetools.ComponentItem):
	"""month - a month """
	want = 'mnth'

months = month

class number(aetools.ComponentItem):
	"""number - an integer or real number """
	want = 'nmbr'

class number_or_date(aetools.ComponentItem):
	"""number or date - a number or date """
	want = 'nd  '

class number_or_string(aetools.ComponentItem):
	"""number or string - a number or string """
	want = 'ns  '

class number_2c__date_or_text(aetools.ComponentItem):
	"""number, date or text - a number, date or text """
	want = 'nds '

numbers = number

class ounces(aetools.ComponentItem):
	"""ounces - a weight measurement in SI meters """
	want = 'ozs '

class picture(aetools.ComponentItem):
	"""picture - A QuickDraw picture object """
	want = 'PICT'

pictures = picture

class pounds(aetools.ComponentItem):
	"""pounds - a weight measurement in SI meters """
	want = 'lbs '

class preposition(aetools.ComponentItem):
	"""preposition - an AppleEvents preposition """
	want = 'prep'

prepositions = preposition

class properties(aetools.ComponentItem):
	"""properties -  """
	want = 'prop'

property = properties

class quarts(aetools.ComponentItem):
	"""quarts - a volume measurement in Imperial quarts """
	want = 'qrts'

class real(aetools.ComponentItem):
	"""real - A real number """
	want = 'doub'

reals = real

class record(aetools.ComponentItem):
	"""record - A set of labeled items """
	want = 'reco'

records = record

class reference(aetools.ComponentItem):
	"""reference - an AppleScript reference """
	want = 'obj '

class reference_form(aetools.ComponentItem):
	"""reference form - an AppleEvents key form """
	want = 'kfrm'

reference_forms = reference_form

references = reference

class script(aetools.ComponentItem):
	"""script - An AppleScript script """
	want = 'scpt'
class name(aetools.NProperty):
	"""name - the name of the script """
	which = 'pnam'
	want = 'TEXT'
class parent(aetools.NProperty):
	"""parent - its parent, i.e. the script that will handle events that this script doesn\xd5t """
	which = 'pare'
	want = 'scpt'

scripts = script

class seconds(aetools.ComponentItem):
	"""seconds - more than one second """
	want = 'scnd'

class sound(aetools.ComponentItem):
	"""sound - a sound object on the clipboard """
	want = 'snd '

sounds = sound

class square_feet(aetools.ComponentItem):
	"""square feet - an area measurement in Imperial square feet """
	want = 'sqft'

class square_kilometers(aetools.ComponentItem):
	"""square kilometers - an area measurement in SI square kilometers """
	want = 'sqkm'

square_kilometres = square_kilometers

class square_meters(aetools.ComponentItem):
	"""square meters - an area measurement in SI square meters """
	want = 'sqrm'

square_metres = square_meters

class square_miles(aetools.ComponentItem):
	"""square miles - an area measurement in Imperial square miles """
	want = 'sqmi'

class square_yards(aetools.ComponentItem):
	"""square yards - an area measurement in Imperial square yards """
	want = 'sqyd'

class string(aetools.ComponentItem):
	"""string - text in 8-bit Macintosh Roman format """
	want = 'TEXT'

strings = string

class styled_Clipboard_text(aetools.ComponentItem):
	"""styled Clipboard text -  """
	want = 'styl'

styled_Clipboard_text = styled_Clipboard_text

class styled_Unicode_text(aetools.ComponentItem):
	"""styled Unicode text -  """
	want = 'sutx'

styled_Unicode_text = styled_Unicode_text

class styled_text(aetools.ComponentItem):
	"""styled text -  """
	want = 'STXT'

styled_text = styled_text

class text(aetools.ComponentItem):
	"""text - text with language and style information """
	want = 'ctxt'

class text_item(aetools.ComponentItem):
	"""text item - text between delimiters """
	want = 'citm'

text_items = text_item

class type_class(aetools.ComponentItem):
	"""type class - the name of a particular class (or any four-character code) """
	want = 'type'

class upper_case(aetools.ComponentItem):
	"""upper case - Text with lower case converted to upper case """
	want = 'case'

class vector(aetools.ComponentItem):
	"""vector - An ordered collection of items """
	want = 'vect'

vectors = vector

class version(aetools.ComponentItem):
	"""version - a version value """
	want = 'vers'

class weekday(aetools.ComponentItem):
	"""weekday - a weekday """
	want = 'wkdy'

weekdays = weekday

class writing_code(aetools.ComponentItem):
	"""writing code - codes that identify the language and script system """
	want = 'psct'

class writing_code_info(aetools.ComponentItem):
	"""writing code info - script code and language code of text run """
	want = 'citl'
class script_code(aetools.NProperty):
	"""script code - the script code for the text """
	which = 'pscd'
	want = 'shor'
class language_code(aetools.NProperty):
	"""language code - the language code for the text """
	which = 'plcd'
	want = 'shor'

writing_code_infos = writing_code_info

class yards(aetools.ComponentItem):
	"""yards - a distance measurement in Imperial yards """
	want = 'yard'

class zone(aetools.ComponentItem):
	"""zone - an AppleTalk zone """
	want = 'zone'

zones = zone
empty_ae_name_._superclassnames = []
empty_ae_name_._privpropdict = {
}
empty_ae_name_._privelemdict = {
}
April._superclassnames = []
April._privpropdict = {
}
April._privelemdict = {
}
August._superclassnames = []
August._privpropdict = {
}
August._privelemdict = {
}
C_string._superclassnames = []
C_string._privpropdict = {
}
C_string._privelemdict = {
}
December._superclassnames = []
December._privpropdict = {
}
December._privelemdict = {
}
February._superclassnames = []
February._privpropdict = {
}
February._privelemdict = {
}
Friday._superclassnames = []
Friday._privpropdict = {
}
Friday._privelemdict = {
}
January._superclassnames = []
January._privpropdict = {
}
January._privelemdict = {
}
July._superclassnames = []
July._privpropdict = {
}
July._privelemdict = {
}
June._superclassnames = []
June._privpropdict = {
}
June._privelemdict = {
}
March._superclassnames = []
March._privpropdict = {
}
March._privelemdict = {
}
May._superclassnames = []
May._privpropdict = {
}
May._privelemdict = {
}
Monday._superclassnames = []
Monday._privpropdict = {
}
Monday._privelemdict = {
}
November._superclassnames = []
November._privpropdict = {
}
November._privelemdict = {
}
October._superclassnames = []
October._privpropdict = {
}
October._privelemdict = {
}
Pascal_string._superclassnames = []
Pascal_string._privpropdict = {
}
Pascal_string._privelemdict = {
}
RGB_color._superclassnames = []
RGB_color._privpropdict = {
}
RGB_color._privelemdict = {
}
Saturday._superclassnames = []
Saturday._privpropdict = {
}
Saturday._privelemdict = {
}
September._superclassnames = []
September._privpropdict = {
}
September._privelemdict = {
}
Sunday._superclassnames = []
Sunday._privpropdict = {
}
Sunday._privelemdict = {
}
Thursday._superclassnames = []
Thursday._privpropdict = {
}
Thursday._privelemdict = {
}
Tuesday._superclassnames = []
Tuesday._privpropdict = {
}
Tuesday._privelemdict = {
}
Unicode_text._superclassnames = []
Unicode_text._privpropdict = {
}
Unicode_text._privelemdict = {
}
Unicode_text._superclassnames = []
Unicode_text._privpropdict = {
}
Unicode_text._privelemdict = {
}
Wednesday._superclassnames = []
Wednesday._privpropdict = {
}
Wednesday._privelemdict = {
}
alias._superclassnames = []
alias._privpropdict = {
}
alias._privelemdict = {
}
alias_or_string._superclassnames = []
alias_or_string._privpropdict = {
}
alias_or_string._privelemdict = {
}
anything._superclassnames = []
anything._privpropdict = {
}
anything._privelemdict = {
}
app._superclassnames = []
app._privpropdict = {
}
app._privelemdict = {
}
boolean._superclassnames = []
boolean._privpropdict = {
}
boolean._privelemdict = {
}
centimeters._superclassnames = []
centimeters._privpropdict = {
}
centimeters._privelemdict = {
}
character._superclassnames = []
character._privpropdict = {
}
character._privelemdict = {
}
class_._superclassnames = ['type_class']
class_._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
class_._privelemdict = {
}
constant._superclassnames = []
constant._privpropdict = {
}
constant._privelemdict = {
}
cubic_centimeters._superclassnames = []
cubic_centimeters._privpropdict = {
}
cubic_centimeters._privelemdict = {
}
cubic_feet._superclassnames = []
cubic_feet._privpropdict = {
}
cubic_feet._privelemdict = {
}
cubic_inches._superclassnames = []
cubic_inches._privpropdict = {
}
cubic_inches._privelemdict = {
}
cubic_meters._superclassnames = []
cubic_meters._privpropdict = {
}
cubic_meters._privelemdict = {
}
cubic_yards._superclassnames = []
cubic_yards._privpropdict = {
}
cubic_yards._privelemdict = {
}
data._superclassnames = []
data._privpropdict = {
}
data._privelemdict = {
}
date._superclassnames = []
date._privpropdict = {
	'weekday' : weekday,
	'month' : month,
	'day' : day,
	'year' : year,
	'time' : time,
	'date_string' : date_string,
	'time_string' : time_string,
}
date._privelemdict = {
}
degrees_Celsius._superclassnames = []
degrees_Celsius._privpropdict = {
}
degrees_Celsius._privelemdict = {
}
degrees_Fahrenheit._superclassnames = []
degrees_Fahrenheit._privpropdict = {
}
degrees_Fahrenheit._privelemdict = {
}
degrees_Kelvin._superclassnames = []
degrees_Kelvin._privpropdict = {
}
degrees_Kelvin._privelemdict = {
}
encoded_string._superclassnames = []
encoded_string._privpropdict = {
}
encoded_string._privelemdict = {
}
event._superclassnames = []
event._privpropdict = {
}
event._privelemdict = {
}
feet._superclassnames = []
feet._privpropdict = {
}
feet._privelemdict = {
}
file_specification._superclassnames = []
file_specification._privpropdict = {
}
file_specification._privelemdict = {
}
gallons._superclassnames = []
gallons._privpropdict = {
}
gallons._privelemdict = {
}
grams._superclassnames = []
grams._privpropdict = {
}
grams._privelemdict = {
}
handler._superclassnames = []
handler._privpropdict = {
}
handler._privelemdict = {
}
inches._superclassnames = []
inches._privpropdict = {
}
inches._privelemdict = {
}
integer._superclassnames = []
integer._privpropdict = {
}
integer._privelemdict = {
}
international_text._superclassnames = []
international_text._privpropdict = {
}
international_text._privelemdict = {
}
international_text._superclassnames = []
international_text._privpropdict = {
}
international_text._privelemdict = {
}
item._superclassnames = []
item._privpropdict = {
	'id' : id,
}
item._privelemdict = {
}
keystroke._superclassnames = []
keystroke._privpropdict = {
	'key' : key,
	'modifiers' : modifiers,
	'key_kind' : key_kind,
}
keystroke._privelemdict = {
}
kilograms._superclassnames = []
kilograms._privpropdict = {
}
kilograms._privelemdict = {
}
kilometers._superclassnames = []
kilometers._privpropdict = {
}
kilometers._privelemdict = {
}
linked_list._superclassnames = []
linked_list._privpropdict = {
	'length' : length,
}
linked_list._privelemdict = {
}
list._superclassnames = []
list._privpropdict = {
	'length' : length,
	'reverse' : reverse,
	'rest' : rest,
}
list._privelemdict = {
}
list_or_record._superclassnames = []
list_or_record._privpropdict = {
}
list_or_record._privelemdict = {
}
list_or_string._superclassnames = []
list_or_string._privpropdict = {
}
list_or_string._privelemdict = {
}
list_2c__record_or_text._superclassnames = []
list_2c__record_or_text._privpropdict = {
}
list_2c__record_or_text._privelemdict = {
}
liters._superclassnames = []
liters._privpropdict = {
}
liters._privelemdict = {
}
machine._superclassnames = []
machine._privpropdict = {
}
machine._privelemdict = {
}
meters._superclassnames = []
meters._privpropdict = {
}
meters._privelemdict = {
}
miles._superclassnames = []
miles._privpropdict = {
}
miles._privelemdict = {
}
missing_value._superclassnames = []
missing_value._privpropdict = {
}
missing_value._privelemdict = {
}
month._superclassnames = []
month._privpropdict = {
}
month._privelemdict = {
}
number._superclassnames = []
number._privpropdict = {
}
number._privelemdict = {
}
number_or_date._superclassnames = []
number_or_date._privpropdict = {
}
number_or_date._privelemdict = {
}
number_or_string._superclassnames = []
number_or_string._privpropdict = {
}
number_or_string._privelemdict = {
}
number_2c__date_or_text._superclassnames = []
number_2c__date_or_text._privpropdict = {
}
number_2c__date_or_text._privelemdict = {
}
ounces._superclassnames = []
ounces._privpropdict = {
}
ounces._privelemdict = {
}
picture._superclassnames = []
picture._privpropdict = {
}
picture._privelemdict = {
}
pounds._superclassnames = []
pounds._privpropdict = {
}
pounds._privelemdict = {
}
preposition._superclassnames = []
preposition._privpropdict = {
}
preposition._privelemdict = {
}
properties._superclassnames = []
properties._privpropdict = {
}
properties._privelemdict = {
}
quarts._superclassnames = []
quarts._privpropdict = {
}
quarts._privelemdict = {
}
real._superclassnames = []
real._privpropdict = {
}
real._privelemdict = {
}
record._superclassnames = []
record._privpropdict = {
}
record._privelemdict = {
}
reference._superclassnames = []
reference._privpropdict = {
}
reference._privelemdict = {
}
reference_form._superclassnames = []
reference_form._privpropdict = {
}
reference_form._privelemdict = {
}
script._superclassnames = []
script._privpropdict = {
	'name' : name,
	'parent' : parent,
}
script._privelemdict = {
}
seconds._superclassnames = []
seconds._privpropdict = {
}
seconds._privelemdict = {
}
sound._superclassnames = []
sound._privpropdict = {
}
sound._privelemdict = {
}
square_feet._superclassnames = []
square_feet._privpropdict = {
}
square_feet._privelemdict = {
}
square_kilometers._superclassnames = []
square_kilometers._privpropdict = {
}
square_kilometers._privelemdict = {
}
square_meters._superclassnames = []
square_meters._privpropdict = {
}
square_meters._privelemdict = {
}
square_miles._superclassnames = []
square_miles._privpropdict = {
}
square_miles._privelemdict = {
}
square_yards._superclassnames = []
square_yards._privpropdict = {
}
square_yards._privelemdict = {
}
string._superclassnames = []
string._privpropdict = {
}
string._privelemdict = {
}
styled_Clipboard_text._superclassnames = []
styled_Clipboard_text._privpropdict = {
}
styled_Clipboard_text._privelemdict = {
}
styled_Clipboard_text._superclassnames = []
styled_Clipboard_text._privpropdict = {
}
styled_Clipboard_text._privelemdict = {
}
styled_Unicode_text._superclassnames = []
styled_Unicode_text._privpropdict = {
}
styled_Unicode_text._privelemdict = {
}
styled_Unicode_text._superclassnames = []
styled_Unicode_text._privpropdict = {
}
styled_Unicode_text._privelemdict = {
}
styled_text._superclassnames = []
styled_text._privpropdict = {
}
styled_text._privelemdict = {
}
styled_text._superclassnames = []
styled_text._privpropdict = {
}
styled_text._privelemdict = {
}
text._superclassnames = []
text._privpropdict = {
}
text._privelemdict = {
}
text_item._superclassnames = []
text_item._privpropdict = {
}
text_item._privelemdict = {
}
type_class._superclassnames = []
type_class._privpropdict = {
}
type_class._privelemdict = {
}
upper_case._superclassnames = []
upper_case._privpropdict = {
}
upper_case._privelemdict = {
}
vector._superclassnames = []
vector._privpropdict = {
	'length' : length,
}
vector._privelemdict = {
}
version._superclassnames = []
version._privpropdict = {
}
version._privelemdict = {
}
weekday._superclassnames = []
weekday._privpropdict = {
}
weekday._privelemdict = {
}
writing_code._superclassnames = []
writing_code._privpropdict = {
}
writing_code._privelemdict = {
}
writing_code_info._superclassnames = []
writing_code_info._privpropdict = {
	'script_code' : script_code,
	'language_code' : language_code,
}
writing_code_info._privelemdict = {
}
yards._superclassnames = []
yards._privpropdict = {
}
yards._privelemdict = {
}
zone._superclassnames = []
zone._privpropdict = {
}
zone._privelemdict = {
}
_Enum_boov = {
	'true' : 'true',	# the true boolean value
	'false' : 'fals',	# the false boolean value
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

_Enum_eMds = {
	'option_down' : 'Kopt',	# 
	'command_down' : 'Kcmd',	# 
	'control_down' : 'Kctl',	# 
	'shift_down' : 'Ksft',	# 
	'caps_lock_down' : 'Kclk',	# 
}

_Enum_ekst = {
	'escape_key' : 'ks5\x00',	# 
	'delete_key' : 'ks3\x00',	# 
	'tab_key' : 'ks0\x00',	# 
	'return_key' : 'ks$\x00',	# 
	'clear_key' : 'ksG\x00',	# 
	'enter_key' : 'ksL\x00',	# 
	'up_arrow_key' : 'ks~\x00',	# 
	'down_arrow_key' : 'ks}\x00',	# 
	'left_arrow_key' : 'ks{\x00',	# 
	'right_arrow_key' : 'ks|\x00',	# 
	'help_key' : 'ksr\x00',	# 
	'home_key' : 'kss\x00',	# 
	'page_up_key' : 'kst\x00',	# 
	'page_down_key' : 'ksy\x00',	# 
	'forward_del_key' : 'ksu\x00',	# 
	'end_key' : 'ksw\x00',	# 
	'F1_key' : 'ksz\x00',	# 
	'F2_key' : 'ksx\x00',	# 
	'F3_key' : 'ksc\x00',	# 
	'F4_key' : 'ksv\x00',	# 
	'F5_key' : 'ks`\x00',	# 
	'F6_key' : 'ksa\x00',	# 
	'F7_key' : 'ksb\x00',	# 
	'F8_key' : 'ksd\x00',	# 
	'F9_key' : 'kse\x00',	# 
	'F10_key' : 'ksm\x00',	# 
	'F11_key' : 'ksg\x00',	# 
	'F12_key' : 'kso\x00',	# 
	'F13_key' : 'ksi\x00',	# 
	'F14_key' : 'ksk\x00',	# 
	'F15_key' : 'ksq\x00',	# 
}

_Enum_misc = {
	'current_application' : 'cura',	# the current application
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'jul ' : July,
	'may ' : May,
	'TEXT' : string,
	'cmet' : cubic_meters,
	'STXT' : styled_text,
	'nds ' : number_2c__date_or_text,
	'feet' : feet,
	'feb ' : February,
	'degc' : degrees_Celsius,
	'kprs' : keystroke,
	'psct' : writing_code,
	'degf' : degrees_Fahrenheit,
	'lrs ' : list_2c__record_or_text,
	'ldt ' : date,
	'degk' : degrees_Kelvin,
	'sun ' : Sunday,
	'oct ' : October,
	'evnt' : event,
	'pstr' : Pascal_string,
	'cyrd' : cubic_yards,
	'PICT' : picture,
	'ls  ' : list_or_string,
	'long' : integer,
	'prop' : properties,
	'nmbr' : number,
	'citl' : writing_code_info,
	'citm' : text_item,
	'apr ' : April,
	'thu ' : Thursday,
	'type' : type_class,
	'prep' : preposition,
	'tue ' : Tuesday,
	'case' : upper_case,
	'vers' : version,
	'wed ' : Wednesday,
	'capp' : app,
	'sqkm' : square_kilometers,
	'obj ' : reference,
	'vect' : vector,
	'wkdy' : weekday,
	'cRGB' : RGB_color,
	'nd  ' : number_or_date,
	'itxt' : international_text,
	'scnd' : seconds,
	'mar ' : March,
	'kmtr' : kilometers,
	'sqft' : square_feet,
	'list' : list,
	'styl' : styled_Clipboard_text,
	'nov ' : November,
	'qrts' : quarts,
	'mile' : miles,
	'msng' : missing_value,
	'alis' : alias,
	'jan ' : January,
	'metr' : meters,
	'mnth' : month,
	'ns  ' : number_or_string,
	'jun ' : June,
	'aug ' : August,
	'llst' : linked_list,
	'doub' : real,
	'encs' : encoded_string,
	'galn' : gallons,
	'cuin' : cubic_inches,
	'fri ' : Friday,
	'sf  ' : alias_or_string,
	'lr  ' : list_or_record,
	'mon ' : Monday,
	'snd ' : sound,
	'sep ' : September,
	'kgrm' : kilograms,
	'scpt' : script,
	'****' : anything,
	'litr' : liters,
	'bool' : boolean,
	'cmtr' : centimeters,
	'sqrm' : square_meters,
	'inch' : inches,
	'zone' : zone,
	'kfrm' : reference_form,
	'cobj' : item,
	'gram' : grams,
	'cha ' : character,
	'reco' : record,
	'undf' : empty_ae_name_,
	'dec ' : December,
	'enum' : constant,
	'hand' : handler,
	'sqmi' : square_miles,
	'rdat' : data,
	'cstr' : C_string,
	'utxt' : Unicode_text,
	'sutx' : styled_Unicode_text,
	'mach' : machine,
	'sqyd' : square_yards,
	'yard' : yards,
	'ozs ' : ounces,
	'lbs ' : pounds,
	'cfet' : cubic_feet,
	'ccmt' : cubic_centimeters,
	'sat ' : Saturday,
	'pcls' : class_,
	'fss ' : file_specification,
	'ctxt' : text,
}

_propdeclarations = {
	'week' : weeks,
	'mnth' : month,
	'pare' : parent,
	'leng' : length,
	'pi  ' : pi,
	'kMod' : modifiers,
	'min ' : minutes,
	'wkdy' : weekday,
	'dstr' : date_string,
	'rest' : rest,
	'ascr' : AppleScript,
	'kknd' : key_kind,
	'c@#^' : _3c_Inheritance_3e_,
	'ID  ' : id,
	'year' : year,
	'rvse' : reverse,
	'tab ' : tab,
	'tstr' : time_string,
	'plcd' : language_code,
	'ret ' : return_,
	'kMsg' : key,
	'hour' : hours,
	'spac' : space,
	'days' : days,
	'txdl' : text_item_delimiters,
	'prdp' : print_depth,
	'prln' : print_length,
	'pscd' : script_code,
	'time' : time,
	'pnam' : name,
	'rslt' : result,
	'day ' : day,
}

_compdeclarations = {
}

_enumdeclarations = {
	'eMds' : _Enum_eMds,
	'cons' : _Enum_cons,
	'misc' : _Enum_misc,
	'ekst' : _Enum_ekst,
	'boov' : _Enum_boov,
}
