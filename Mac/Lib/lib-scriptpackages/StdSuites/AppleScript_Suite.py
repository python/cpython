"""Suite AppleScript Suite: Standard terms for AppleScript
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Extensies:AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'ascr'

class AppleScript_Suite_Events:

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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def tell(self, _no_object=None, _attributes={}, **_arguments):
		"""tell: Record or log a •tellÕ statement
		Keyword argument _attributes: AppleEvent attribute dictionary
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
		"""end tell: Record or log an •end tellÕ statement
		Keyword argument _attributes: AppleEvent attribute dictionary
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
		Keyword argument to: the desired class for a failed coercion
		Keyword argument _attributes: AppleEvent attribute dictionary
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
		'about' : 'abou',
		'since' : 'snce',
		'given' : 'givn',
		'with' : 'with',
		'without' : 'wout',
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


class boolean(aetools.ComponentItem):
	"""boolean - A true or false value """
	want = 'bool'

booleans = boolean

class integer(aetools.ComponentItem):
	"""integer - An integral number """
	want = 'long'

integers = integer

class real(aetools.ComponentItem):
	"""real - A real number """
	want = 'doub'

reals = real

class number(aetools.ComponentItem):
	"""number - an integer or real number """
	want = 'nmbr'

numbers = number

class list(aetools.ComponentItem):
	"""list - An ordered collection of items """
	want = 'list'
class length(aetools.NProperty):
	"""length - the length of a list """
	which = 'leng'
	want = 'long'
class reverse(aetools.NProperty):
	"""reverse - the items of the list in reverse order """
	which = 'rvse'
	want = 'list'
class rest(aetools.NProperty):
	"""rest - all items of the list excluding first """
	which = 'rest'
	want = 'list'

lists = list

class linked_list(aetools.ComponentItem):
	"""linked list - An ordered collection of items """
	want = 'llst'
# repeated property length the length of a list

linked_lists = linked_list

class vector(aetools.ComponentItem):
	"""vector - An ordered collection of items """
	want = 'vect'
# repeated property length the length of a list

vectors = vector

class record(aetools.ComponentItem):
	"""record - A set of labeled items """
	want = 'reco'

records = record

class item(aetools.ComponentItem):
	"""item - An item of any type """
	want = 'cobj'
class id(aetools.NProperty):
	"""id - the unique ID number of this object """
	which = 'ID  '
	want = 'long'

items = item

class script(aetools.ComponentItem):
	"""script - An AppleScript script """
	want = 'scpt'
class name(aetools.NProperty):
	"""name - the name of the script """
	which = 'pnam'
	want = 'TEXT'
class parent(aetools.NProperty):
	"""parent - its parent, i.e. the script that will handle events that this script doesnÕt """
	which = 'pare'
	want = 'scpt'

scripts = script

class list_or_record(aetools.ComponentItem):
	"""list or record - a list or record """
	want = 'lr  '

class list_or_string(aetools.ComponentItem):
	"""list or string - a list or string """
	want = 'ls  '

class number_or_string(aetools.ComponentItem):
	"""number or string - a number or string """
	want = 'ns  '

class alias_or_string(aetools.ComponentItem):
	"""alias or string - an alias or string """
	want = 'sf  '

class list_2c__record_or_text(aetools.ComponentItem):
	"""list, record or text - a list, record or text """
	want = 'lrs '

class number_or_date(aetools.ComponentItem):
	"""number or date - a number or date """
	want = 'nd  '

class number_2c__date_or_text(aetools.ComponentItem):
	"""number, date or text - a number, date or text """
	want = 'nds '

class _class(aetools.ComponentItem):
	"""class - the type of a value """
	want = 'pcls'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from this class """
	which = 'c@#^'
	want = 'type'

classes = _class

class event(aetools.ComponentItem):
	"""event - an AppleEvents event """
	want = 'evnt'

events = event

class property(aetools.ComponentItem):
	"""property - an AppleEvents property """
	want = 'prop'

properties = property

class constant(aetools.ComponentItem):
	"""constant - A constant value """
	want = 'enum'

constants = constant

class preposition(aetools.ComponentItem):
	"""preposition - an AppleEvents preposition """
	want = 'prep'

prepositions = preposition

class reference_form(aetools.ComponentItem):
	"""reference form - an AppleEvents key form """
	want = 'kfrm'

reference_forms = reference_form

class handler(aetools.ComponentItem):
	"""handler - an AppleScript event or subroutine handler """
	want = 'hand'

handlers = handler

class data(aetools.ComponentItem):
	"""data - an AppleScript raw data object """
	want = 'rdat'

class text(aetools.ComponentItem):
	"""text - text with language and style information """
	want = 'ctxt'

class international_text(aetools.ComponentItem):
	"""international text -  """
	want = 'itxt'

international_text = international_text

class string(aetools.ComponentItem):
	"""string - text in 8-bit Macintosh Roman format """
	want = 'TEXT'

strings = string

class styled_text(aetools.ComponentItem):
	"""styled text -  """
	want = 'STXT'

styled_text = styled_text

class styled_Clipboard_text(aetools.ComponentItem):
	"""styled Clipboard text -  """
	want = 'styl'

styled_Clipboard_text = styled_Clipboard_text

class Unicode_text(aetools.ComponentItem):
	"""Unicode text -  """
	want = 'utxt'

Unicode_text = Unicode_text

class styled_Unicode_text(aetools.ComponentItem):
	"""styled Unicode text -  """
	want = 'sutx'

styled_Unicode_text = styled_Unicode_text

class encoded_string(aetools.ComponentItem):
	"""encoded string - text encoded using the Text Encoding Converter """
	want = 'encs'

encoded_strings = encoded_string

class C_string(aetools.ComponentItem):
	"""C string - text followed by a null """
	want = 'cstr'

C_strings = C_string

class Pascal_string(aetools.ComponentItem):
	"""Pascal string - text up to 255 characters preceded by a length byte """
	want = 'pstr'

Pascal_strings = Pascal_string

class character(aetools.ComponentItem):
	"""character - an individual text character """
	want = 'cha '

characters = character

class text_item(aetools.ComponentItem):
	"""text item - text between delimiters """
	want = 'citm'

text_items = text_item

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

class _empty_ae_name(aetools.ComponentItem):
	""" - the undefined value """
	want = 'undf'

class missing_value(aetools.ComponentItem):
	"""missing value - unavailable value, such as properties missing from heterogeneous classes in a Whose clause """
	want = 'msng'

missing_values = missing_value

class reference(aetools.ComponentItem):
	"""reference - an AppleScript reference """
	want = 'obj '

references = reference

class anything(aetools.ComponentItem):
	"""anything - any class or reference """
	want = '****'

class type_class(aetools.ComponentItem):
	"""type class - the name of a particular class (or any four-character code) """
	want = 'type'

class RGB_color(aetools.ComponentItem):
	"""RGB color - Three integers specifying red, green, blue color values """
	want = 'cRGB'

RGB_colors = RGB_color

class picture(aetools.ComponentItem):
	"""picture - A QuickDraw picture object """
	want = 'PICT'

pictures = picture

class sound(aetools.ComponentItem):
	"""sound - a sound object on the clipboard """
	want = 'snd '

sounds = sound

class version(aetools.ComponentItem):
	"""version - a version value """
	want = 'vers'

class file_specification(aetools.ComponentItem):
	"""file specification - a file specification as used by the operating system """
	want = 'fss '

file_specifications = file_specification

class alias(aetools.ComponentItem):
	"""alias - a file on a disk or server.  The file must exist when you check the syntax of your script. """
	want = 'alis'

aliases = alias

class machine(aetools.ComponentItem):
	"""machine - a computer """
	want = 'mach'

machines = machine

class zone(aetools.ComponentItem):
	"""zone - an AppleTalk zone """
	want = 'zone'

zones = zone

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

class seconds(aetools.ComponentItem):
	"""seconds - more than one second """
	want = 'scnd'

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

class month(aetools.ComponentItem):
	"""month - a month """
	want = 'mnth'

months = month

class January(aetools.ComponentItem):
	"""January - the month of January """
	want = 'jan '

class February(aetools.ComponentItem):
	"""February - the month of February """
	want = 'feb '

class March(aetools.ComponentItem):
	"""March - the month of March """
	want = 'mar '

class April(aetools.ComponentItem):
	"""April - the month of April """
	want = 'apr '

class May(aetools.ComponentItem):
	"""May - the month of May """
	want = 'may '

class June(aetools.ComponentItem):
	"""June - the month of June """
	want = 'jun '

class July(aetools.ComponentItem):
	"""July - the month of July """
	want = 'jul '

class August(aetools.ComponentItem):
	"""August - the month of August """
	want = 'aug '

class September(aetools.ComponentItem):
	"""September - the month of September """
	want = 'sep '

class October(aetools.ComponentItem):
	"""October - the month of October """
	want = 'oct '

class November(aetools.ComponentItem):
	"""November - the month of November """
	want = 'nov '

class December(aetools.ComponentItem):
	"""December - the month of December """
	want = 'dec '

class weekday(aetools.ComponentItem):
	"""weekday - a weekday """
	want = 'wkdy'

weekdays = weekday

class Sunday(aetools.ComponentItem):
	"""Sunday - Sunday """
	want = 'sun '

class Monday(aetools.ComponentItem):
	"""Monday - Monday """
	want = 'mon '

class Tuesday(aetools.ComponentItem):
	"""Tuesday - Tuesday """
	want = 'tue '

class Wednesday(aetools.ComponentItem):
	"""Wednesday - Wednesday """
	want = 'wed '

class Thursday(aetools.ComponentItem):
	"""Thursday - Thursday """
	want = 'thu '

class Friday(aetools.ComponentItem):
	"""Friday - Friday """
	want = 'fri '

class Saturday(aetools.ComponentItem):
	"""Saturday - Saturday """
	want = 'sat '

class metres(aetools.ComponentItem):
	"""metres - a distance measurement in SI meters """
	want = 'metr'

meters = metres

class inches(aetools.ComponentItem):
	"""inches - a distance measurement in Imperial inches """
	want = 'inch'

class feet(aetools.ComponentItem):
	"""feet - a distance measurement in Imperial feet """
	want = 'feet'

class yards(aetools.ComponentItem):
	"""yards - a distance measurement in Imperial yards """
	want = 'yard'

class miles(aetools.ComponentItem):
	"""miles - a distance measurement in Imperial miles """
	want = 'mile'

class kilometres(aetools.ComponentItem):
	"""kilometres - a distance measurement in SI kilometers """
	want = 'kmtr'

kilometers = kilometres

class centimetres(aetools.ComponentItem):
	"""centimetres - a distance measurement in SI centimeters """
	want = 'cmtr'

centimeters = centimetres

class square_metres(aetools.ComponentItem):
	"""square metres - an area measurement in SI square meters """
	want = 'sqrm'

square_meters = square_metres

class square_feet(aetools.ComponentItem):
	"""square feet - an area measurement in Imperial square feet """
	want = 'sqft'

class square_yards(aetools.ComponentItem):
	"""square yards - an area measurement in Imperial square yards """
	want = 'sqyd'

class square_miles(aetools.ComponentItem):
	"""square miles - an area measurement in Imperial square miles """
	want = 'sqmi'

class square_kilometres(aetools.ComponentItem):
	"""square kilometres - an area measurement in SI square kilometers """
	want = 'sqkm'

square_kilometers = square_kilometres

class litres(aetools.ComponentItem):
	"""litres - a volume measurement in SI liters """
	want = 'litr'

liters = litres

class gallons(aetools.ComponentItem):
	"""gallons - a volume measurement in Imperial gallons """
	want = 'galn'

class quarts(aetools.ComponentItem):
	"""quarts - a volume measurement in Imperial quarts """
	want = 'qrts'

class cubic_metres(aetools.ComponentItem):
	"""cubic metres - a volume measurement in SI cubic meters """
	want = 'cmet'

cubic_meters = cubic_metres

class cubic_centimetres(aetools.ComponentItem):
	"""cubic centimetres - a volume measurement in SI cubic centimeters """
	want = 'ccmt'

cubic_centimeters = cubic_centimetres

class cubic_feet(aetools.ComponentItem):
	"""cubic feet - a volume measurement in Imperial cubic feet """
	want = 'cfet'

class cubic_inches(aetools.ComponentItem):
	"""cubic inches - a volume measurement in Imperial cubic inches """
	want = 'cuin'

class cubic_yards(aetools.ComponentItem):
	"""cubic yards - a distance measurement in Imperial cubic yards """
	want = 'cyrd'

class kilograms(aetools.ComponentItem):
	"""kilograms - a mass measurement in SI kilograms """
	want = 'kgrm'

class grams(aetools.ComponentItem):
	"""grams - a mass measurement in SI meters """
	want = 'gram'

class ounces(aetools.ComponentItem):
	"""ounces - a weight measurement in SI meters """
	want = 'ozs '

class pounds(aetools.ComponentItem):
	"""pounds - a weight measurement in SI meters """
	want = 'lbs '

class degrees_Celsius(aetools.ComponentItem):
	"""degrees Celsius - a temperature measurement in SI degrees Celsius """
	want = 'degc'

class degrees_Fahrenheit(aetools.ComponentItem):
	"""degrees Fahrenheit - a temperature measurement in degrees Fahrenheit """
	want = 'degf'

class degrees_Kelvin(aetools.ComponentItem):
	"""degrees Kelvin - a temperature measurement in degrees Kelvin """
	want = 'degk'

class upper_case(aetools.ComponentItem):
	"""upper case - Text with lower case converted to upper case """
	want = 'case'

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
class _return(aetools.NProperty):
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
boolean._propdict = {
}
boolean._elemdict = {
}
integer._propdict = {
}
integer._elemdict = {
}
real._propdict = {
}
real._elemdict = {
}
number._propdict = {
}
number._elemdict = {
}
list._propdict = {
	'length' : length,
	'reverse' : reverse,
	'rest' : rest,
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
item._propdict = {
	'id' : id,
}
item._elemdict = {
}
script._propdict = {
	'name' : name,
	'parent' : parent,
}
script._elemdict = {
}
list_or_record._propdict = {
}
list_or_record._elemdict = {
}
list_or_string._propdict = {
}
list_or_string._elemdict = {
}
number_or_string._propdict = {
}
number_or_string._elemdict = {
}
alias_or_string._propdict = {
}
alias_or_string._elemdict = {
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
_class._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
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
reference_form._propdict = {
}
reference_form._elemdict = {
}
handler._propdict = {
}
handler._elemdict = {
}
data._propdict = {
}
data._elemdict = {
}
text._propdict = {
}
text._elemdict = {
}
international_text._propdict = {
}
international_text._elemdict = {
}
international_text._propdict = {
}
international_text._elemdict = {
}
string._propdict = {
}
string._elemdict = {
}
styled_text._propdict = {
}
styled_text._elemdict = {
}
styled_text._propdict = {
}
styled_text._elemdict = {
}
styled_Clipboard_text._propdict = {
}
styled_Clipboard_text._elemdict = {
}
styled_Clipboard_text._propdict = {
}
styled_Clipboard_text._elemdict = {
}
Unicode_text._propdict = {
}
Unicode_text._elemdict = {
}
Unicode_text._propdict = {
}
Unicode_text._elemdict = {
}
styled_Unicode_text._propdict = {
}
styled_Unicode_text._elemdict = {
}
styled_Unicode_text._propdict = {
}
styled_Unicode_text._elemdict = {
}
encoded_string._propdict = {
}
encoded_string._elemdict = {
}
C_string._propdict = {
}
C_string._elemdict = {
}
Pascal_string._propdict = {
}
Pascal_string._elemdict = {
}
character._propdict = {
}
character._elemdict = {
}
text_item._propdict = {
}
text_item._elemdict = {
}
writing_code._propdict = {
}
writing_code._elemdict = {
}
writing_code_info._propdict = {
	'script_code' : script_code,
	'language_code' : language_code,
}
writing_code_info._elemdict = {
}
_empty_ae_name._propdict = {
}
_empty_ae_name._elemdict = {
}
missing_value._propdict = {
}
missing_value._elemdict = {
}
reference._propdict = {
}
reference._elemdict = {
}
anything._propdict = {
}
anything._elemdict = {
}
type_class._propdict = {
}
type_class._elemdict = {
}
RGB_color._propdict = {
}
RGB_color._elemdict = {
}
picture._propdict = {
}
picture._elemdict = {
}
sound._propdict = {
}
sound._elemdict = {
}
version._propdict = {
}
version._elemdict = {
}
file_specification._propdict = {
}
file_specification._elemdict = {
}
alias._propdict = {
}
alias._elemdict = {
}
machine._propdict = {
}
machine._elemdict = {
}
zone._propdict = {
}
zone._elemdict = {
}
keystroke._propdict = {
	'key' : key,
	'modifiers' : modifiers,
	'key_kind' : key_kind,
}
keystroke._elemdict = {
}
seconds._propdict = {
}
seconds._elemdict = {
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
metres._propdict = {
}
metres._elemdict = {
}
inches._propdict = {
}
inches._elemdict = {
}
feet._propdict = {
}
feet._elemdict = {
}
yards._propdict = {
}
yards._elemdict = {
}
miles._propdict = {
}
miles._elemdict = {
}
kilometres._propdict = {
}
kilometres._elemdict = {
}
centimetres._propdict = {
}
centimetres._elemdict = {
}
square_metres._propdict = {
}
square_metres._elemdict = {
}
square_feet._propdict = {
}
square_feet._elemdict = {
}
square_yards._propdict = {
}
square_yards._elemdict = {
}
square_miles._propdict = {
}
square_miles._elemdict = {
}
square_kilometres._propdict = {
}
square_kilometres._elemdict = {
}
litres._propdict = {
}
litres._elemdict = {
}
gallons._propdict = {
}
gallons._elemdict = {
}
quarts._propdict = {
}
quarts._elemdict = {
}
cubic_metres._propdict = {
}
cubic_metres._elemdict = {
}
cubic_centimetres._propdict = {
}
cubic_centimetres._elemdict = {
}
cubic_feet._propdict = {
}
cubic_feet._elemdict = {
}
cubic_inches._propdict = {
}
cubic_inches._elemdict = {
}
cubic_yards._propdict = {
}
cubic_yards._elemdict = {
}
kilograms._propdict = {
}
kilograms._elemdict = {
}
grams._propdict = {
}
grams._elemdict = {
}
ounces._propdict = {
}
ounces._elemdict = {
}
pounds._propdict = {
}
pounds._elemdict = {
}
degrees_Celsius._propdict = {
}
degrees_Celsius._elemdict = {
}
degrees_Fahrenheit._propdict = {
}
degrees_Fahrenheit._elemdict = {
}
degrees_Kelvin._propdict = {
}
degrees_Kelvin._elemdict = {
}
upper_case._propdict = {
}
upper_case._elemdict = {
}
app._propdict = {
}
app._elemdict = {
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

_Enum_eMds = {
	'option_down' : 'Kopt',	# 
	'command_down' : 'Kcmd',	# 
	'control_down' : 'Kctl',	# 
	'shift_down' : 'Ksft',	# 
	'caps_lock_down' : 'Kclk',	# 
}

_Enum_ekst = {
	'escape_key' : 'ks5\000',	# 
	'delete_key' : 'ks3\000',	# 
	'tab_key' : 'ks0\000',	# 
	'return_key' : 'ks$\000',	# 
	'clear_key' : 'ksG\000',	# 
	'enter_key' : 'ksL\000',	# 
	'up_arrow_key' : 'ks~\000',	# 
	'down_arrow_key' : 'ks}\000',	# 
	'left_arrow_key' : 'ks{\000',	# 
	'right_arrow_key' : 'ks|\000',	# 
	'help_key' : 'ksr\000',	# 
	'home_key' : 'kss\000',	# 
	'page_up_key' : 'kst\000',	# 
	'page_down_key' : 'ksy\000',	# 
	'forward_del_key' : 'ksu\000',	# 
	'end_key' : 'ksw\000',	# 
	'F1_key' : 'ksz\000',	# 
	'F2_key' : 'ksx\000',	# 
	'F3_key' : 'ksc\000',	# 
	'F4_key' : 'ksv\000',	# 
	'F5_key' : 'ks`\000',	# 
	'F6_key' : 'ksa\000',	# 
	'F7_key' : 'ksb\000',	# 
	'F8_key' : 'ksd\000',	# 
	'F9_key' : 'kse\000',	# 
	'F10_key' : 'ksm\000',	# 
	'F11_key' : 'ksg\000',	# 
	'F12_key' : 'kso\000',	# 
	'F13_key' : 'ksi\000',	# 
	'F14_key' : 'ksk\000',	# 
	'F15_key' : 'ksq\000',	# 
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'nmbr' : number,
	'ctxt' : text,
	'fss ' : file_specification,
	'sat ' : Saturday,
	'ccmt' : cubic_centimetres,
	'cfet' : cubic_feet,
	'lbs ' : pounds,
	'yard' : yards,
	'sqyd' : square_yards,
	'mach' : machine,
	'utxt' : Unicode_text,
	'cstr' : C_string,
	'rdat' : data,
	'doub' : real,
	'hand' : handler,
	'sutx' : styled_Unicode_text,
	'sqmi' : square_miles,
	'undf' : _empty_ae_name,
	'reco' : record,
	'cha ' : character,
	'cobj' : item,
	'kfrm' : reference_form,
	'enum' : constant,
	'inch' : inches,
	'sqrm' : square_metres,
	'bool' : boolean,
	'prop' : property,
	'****' : anything,
	'scpt' : script,
	'kgrm' : kilograms,
	'sep ' : September,
	'snd ' : sound,
	'mon ' : Monday,
	'capp' : app,
	'lr  ' : list_or_record,
	'fri ' : Friday,
	'cuin' : cubic_inches,
	'mar ' : March,
	'galn' : gallons,
	'encs' : encoded_string,
	'litr' : litres,
	'case' : upper_case,
	'styl' : styled_Clipboard_text,
	'llst' : linked_list,
	'pcls' : _class,
	'jun ' : June,
	'ns  ' : number_or_string,
	'ozs ' : ounces,
	'mnth' : month,
	'metr' : metres,
	'jan ' : January,
	'pstr' : Pascal_string,
	'alis' : alias,
	'gram' : grams,
	'msng' : missing_value,
	'qrts' : quarts,
	'nov ' : November,
	'list' : list,
	'sqft' : square_feet,
	'kmtr' : kilometres,
	'cRGB' : RGB_color,
	'itxt' : international_text,
	'scnd' : seconds,
	'apr ' : April,
	'nd  ' : number_or_date,
	'wkdy' : weekday,
	'vect' : vector,
	'obj ' : reference,
	'sqkm' : square_kilometres,
	'dec ' : December,
	'wed ' : Wednesday,
	'cyrd' : cubic_yards,
	'vers' : version,
	'tue ' : Tuesday,
	'prep' : preposition,
	'type' : type_class,
	'citm' : text_item,
	'citl' : writing_code_info,
	'sf  ' : alias_or_string,
	'degc' : degrees_Celsius,
	'long' : integer,
	'ls  ' : list_or_string,
	'PICT' : picture,
	'zone' : zone,
	'psct' : writing_code,
	'lrs ' : list_2c__record_or_text,
	'cmtr' : centimetres,
	'evnt' : event,
	'oct ' : October,
	'degk' : degrees_Kelvin,
	'ldt ' : date,
	'thu ' : Thursday,
	'degf' : degrees_Fahrenheit,
	'kprs' : keystroke,
	'mile' : miles,
	'feb ' : February,
	'feet' : feet,
	'nds ' : number_2c__date_or_text,
	'STXT' : styled_text,
	'cmet' : cubic_metres,
	'sun ' : Sunday,
	'aug ' : August,
	'may ' : May,
	'jul ' : July,
	'TEXT' : string,
}

_propdeclarations = {
	'pscd' : script_code,
	'rslt' : result,
	'pnam' : name,
	'time' : time,
	'txdl' : text_item_delimiters,
	'prln' : print_length,
	'prdp' : print_depth,
	'kMod' : modifiers,
	'days' : days,
	'spac' : space,
	'kMsg' : key,
	'plcd' : language_code,
	'ret ' : _return,
	'tstr' : time_string,
	'hour' : hours,
	'tab ' : tab,
	'rvse' : reverse,
	'wkdy' : weekday,
	'day ' : day,
	'ID  ' : id,
	'c@#^' : _3c_Inheritance_3e_,
	'kknd' : key_kind,
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
	'boov' : _Enum_boov,
	'ekst' : _Enum_ekst,
	'misc' : _Enum_misc,
	'cons' : _Enum_cons,
	'eMds' : _Enum_eMds,
}
