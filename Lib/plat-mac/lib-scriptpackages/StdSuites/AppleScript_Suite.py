"""Suite AppleScript Suite: Standard terms for AppleScript
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
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


class anything(aetools.ComponentItem):
    """anything - any class or reference """
    want = '****'

class pictures(aetools.ComponentItem):
    """pictures -  """
    want = 'PICT'

picture = pictures

class styled_text(aetools.ComponentItem):
    """styled text - text with font, size, and style information """
    want = 'STXT'

styled_text = styled_text

class strings(aetools.ComponentItem):
    """strings -  """
    want = 'TEXT'

string = strings

class alias(aetools.ComponentItem):
    """alias - a file on a disk or server.  The file must exist when you check the syntax of your script. """
    want = 'alis'
class _Prop_POSIX_path(aetools.NProperty):
    """POSIX path - the POSIX path of the file """
    which = 'psxp'
    want = 'TEXT'

aliases = alias

class April(aetools.ComponentItem):
    """April - the month of April """
    want = 'apr '

class August(aetools.ComponentItem):
    """August - the month of August """
    want = 'aug '

class booleans(aetools.ComponentItem):
    """booleans -  """
    want = 'bool'

boolean = booleans

class RGB_colors(aetools.ComponentItem):
    """RGB colors -  """
    want = 'cRGB'

RGB_color = RGB_colors

class application(aetools.ComponentItem):
    """application - specifies global properties of AppleScript """
    want = 'capp'
class _Prop_AppleScript(aetools.NProperty):
    """AppleScript - the top-level script object """
    which = 'ascr'
    want = 'scpt'
AppleScript = _Prop_AppleScript()
class _Prop_days(aetools.NProperty):
    """days - the number of seconds in a day """
    which = 'days'
    want = 'long'
days = _Prop_days()
class _Prop_hours(aetools.NProperty):
    """hours - the number of seconds in an hour """
    which = 'hour'
    want = 'long'
hours = _Prop_hours()
class _Prop_minutes(aetools.NProperty):
    """minutes - the number of seconds in a minute """
    which = 'min '
    want = 'long'
minutes = _Prop_minutes()
class _Prop_pi(aetools.NProperty):
    """pi - the constant pi """
    which = 'pi  '
    want = 'doub'
pi = _Prop_pi()
class _Prop_print_depth(aetools.NProperty):
    """print depth - the maximum depth to print """
    which = 'prdp'
    want = 'long'
print_depth = _Prop_print_depth()
class _Prop_print_length(aetools.NProperty):
    """print length - the maximum length to print """
    which = 'prln'
    want = 'long'
print_length = _Prop_print_length()
class _Prop_result(aetools.NProperty):
    """result - the last result of evaluation """
    which = 'rslt'
    want = '****'
result = _Prop_result()
class _Prop_return_(aetools.NProperty):
    """return - a return character """
    which = 'ret '
    want = 'cha '
return_ = _Prop_return_()
class _Prop_space(aetools.NProperty):
    """space - a space character """
    which = 'spac'
    want = 'cha '
space = _Prop_space()
class _Prop_tab(aetools.NProperty):
    """tab - a tab character """
    which = 'tab '
    want = 'cha '
tab = _Prop_tab()
class _Prop_text_item_delimiters(aetools.NProperty):
    """text item delimiters - the text item delimiters of a string """
    which = 'txdl'
    want = 'list'
text_item_delimiters = _Prop_text_item_delimiters()
class _Prop_weeks(aetools.NProperty):
    """weeks - the number of seconds in a week """
    which = 'week'
    want = 'long'
weeks = _Prop_weeks()

applications = application

app = application

class upper_case(aetools.ComponentItem):
    """upper case - Text with lower case converted to upper case """
    want = 'case'

class cubic_centimeters(aetools.ComponentItem):
    """cubic centimeters - a volume measurement in SI cubic centimeters """
    want = 'ccmt'

cubic_centimetres = cubic_centimeters

class cubic_feet(aetools.ComponentItem):
    """cubic feet - a volume measurement in Imperial cubic feet """
    want = 'cfet'

class characters(aetools.ComponentItem):
    """characters -  """
    want = 'cha '

character = characters

class writing_code_info(aetools.ComponentItem):
    """writing code info - script code and language code of text run """
    want = 'citl'
class _Prop_language_code(aetools.NProperty):
    """language code - the language code for the text """
    which = 'plcd'
    want = 'shor'
class _Prop_script_code(aetools.NProperty):
    """script code - the script code for the text """
    which = 'pscd'
    want = 'shor'

writing_code_infos = writing_code_info

class text_items(aetools.ComponentItem):
    """text items -  """
    want = 'citm'

text_item = text_items

class cubic_meters(aetools.ComponentItem):
    """cubic meters - a volume measurement in SI cubic meters """
    want = 'cmet'

cubic_metres = cubic_meters

class centimeters(aetools.ComponentItem):
    """centimeters - a distance measurement in SI centimeters """
    want = 'cmtr'

centimetres = centimeters

class item(aetools.ComponentItem):
    """item - An item of any type """
    want = 'cobj'
class _Prop_id(aetools.NProperty):
    """id - the unique ID number of this object """
    which = 'ID  '
    want = 'long'

items = item

class C_strings(aetools.ComponentItem):
    """C strings -  """
    want = 'cstr'

C_string = C_strings

class text(aetools.ComponentItem):
    """text - text with language and style information """
    want = 'ctxt'

class cubic_inches(aetools.ComponentItem):
    """cubic inches - a volume measurement in Imperial cubic inches """
    want = 'cuin'

class cubic_yards(aetools.ComponentItem):
    """cubic yards - a distance measurement in Imperial cubic yards """
    want = 'cyrd'

class December(aetools.ComponentItem):
    """December - the month of December """
    want = 'dec '

class degrees_Celsius(aetools.ComponentItem):
    """degrees Celsius - a temperature measurement in SI degrees Celsius """
    want = 'degc'

class degrees_Fahrenheit(aetools.ComponentItem):
    """degrees Fahrenheit - a temperature measurement in degrees Fahrenheit """
    want = 'degf'

class degrees_Kelvin(aetools.ComponentItem):
    """degrees Kelvin - a temperature measurement in degrees Kelvin """
    want = 'degk'

class reals(aetools.ComponentItem):
    """reals -  """
    want = 'doub'

real = reals

class encoded_strings(aetools.ComponentItem):
    """encoded strings -  """
    want = 'encs'

encoded_string = encoded_strings

class constants(aetools.ComponentItem):
    """constants -  """
    want = 'enum'

constant = constants

class events(aetools.ComponentItem):
    """events -  """
    want = 'evnt'

event = events

class February(aetools.ComponentItem):
    """February - the month of February """
    want = 'feb '

class feet(aetools.ComponentItem):
    """feet - a distance measurement in Imperial feet """
    want = 'feet'

class Friday(aetools.ComponentItem):
    """Friday - Friday """
    want = 'fri '

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

class handlers(aetools.ComponentItem):
    """handlers -  """
    want = 'hand'

handler = handlers

class inches(aetools.ComponentItem):
    """inches - a distance measurement in Imperial inches """
    want = 'inch'

class international_text(aetools.ComponentItem):
    """international text - text that begins with a writing code """
    want = 'itxt'

international_text = international_text

class January(aetools.ComponentItem):
    """January - the month of January """
    want = 'jan '

class July(aetools.ComponentItem):
    """July - the month of July """
    want = 'jul '

class June(aetools.ComponentItem):
    """June - the month of June """
    want = 'jun '

class reference_forms(aetools.ComponentItem):
    """reference forms -  """
    want = 'kfrm'

reference_form = reference_forms

class kilograms(aetools.ComponentItem):
    """kilograms - a mass measurement in SI kilograms """
    want = 'kgrm'

class kilometers(aetools.ComponentItem):
    """kilometers - a distance measurement in SI kilometers """
    want = 'kmtr'

kilometres = kilometers

class keystroke(aetools.ComponentItem):
    """keystroke - a press of a key combination on a Macintosh keyboard """
    want = 'kprs'
class _Prop_key(aetools.NProperty):
    """key - the character for the key was pressed (ignoring modifiers) """
    which = 'kMsg'
    want = 'cha '
class _Prop_key_kind(aetools.NProperty):
    """key kind - the kind of key that was pressed """
    which = 'kknd'
    want = 'ekst'
class _Prop_modifiers(aetools.NProperty):
    """modifiers - the modifier keys pressed in combination """
    which = 'kMod'
    want = 'eMds'

keystrokes = keystroke

class pounds(aetools.ComponentItem):
    """pounds - a weight measurement in SI meters """
    want = 'lbs '

class date(aetools.ComponentItem):
    """date - Absolute date and time values """
    want = 'ldt '
class _Prop_date_string(aetools.NProperty):
    """date string - the date portion of a date-time value as text """
    which = 'dstr'
    want = 'TEXT'
class _Prop_day(aetools.NProperty):
    """day - the day of the month of a date """
    which = 'day '
    want = 'long'
class _Prop_month(aetools.NProperty):
    """month - the month of a date """
    which = 'mnth'
    want = 'mnth'
class _Prop_time(aetools.NProperty):
    """time - the time since midnight of a date """
    which = 'time'
    want = 'long'
class _Prop_time_string(aetools.NProperty):
    """time string - the time portion of a date-time value as text """
    which = 'tstr'
    want = 'TEXT'
class _Prop_weekday(aetools.NProperty):
    """weekday - the day of a week of a date """
    which = 'wkdy'
    want = 'wkdy'
class _Prop_year(aetools.NProperty):
    """year - the year of a date """
    which = 'year'
    want = 'long'

dates = date

class list(aetools.ComponentItem):
    """list - An ordered collection of items """
    want = 'list'
class _Prop_length(aetools.NProperty):
    """length - the length of a list """
    which = 'leng'
    want = 'long'
class _Prop_rest(aetools.NProperty):
    """rest - all items of the list excluding first """
    which = 'rest'
    want = 'list'
class _Prop_reverse(aetools.NProperty):
    """reverse - the items of the list in reverse order """
    which = 'rvse'
    want = 'list'

lists = list

class liters(aetools.ComponentItem):
    """liters - a volume measurement in SI liters """
    want = 'litr'

litres = liters

class linked_list(aetools.ComponentItem):
    """linked list - An ordered collection of items """
    want = 'llst'

linked_lists = linked_list

class integers(aetools.ComponentItem):
    """integers -  """
    want = 'long'

integer = integers

class list_or_record(aetools.ComponentItem):
    """list or record - a list or record """
    want = 'lr  '

class list_2c__record_or_text(aetools.ComponentItem):
    """list, record or text - a list, record or text """
    want = 'lrs '

class list_or_string(aetools.ComponentItem):
    """list or string - a list or string """
    want = 'ls  '

class machines(aetools.ComponentItem):
    """machines -  """
    want = 'mach'

machine = machines

class March(aetools.ComponentItem):
    """March - the month of March """
    want = 'mar '

class May(aetools.ComponentItem):
    """May - the month of May """
    want = 'may '

class meters(aetools.ComponentItem):
    """meters - a distance measurement in SI meters """
    want = 'metr'

metres = meters

class miles(aetools.ComponentItem):
    """miles - a distance measurement in Imperial miles """
    want = 'mile'

class months(aetools.ComponentItem):
    """months -  """
    want = 'mnth'

month = months

class Monday(aetools.ComponentItem):
    """Monday - Monday """
    want = 'mon '

class missing_values(aetools.ComponentItem):
    """missing values -  """
    want = 'msng'

missing_value = missing_values

class number_or_date(aetools.ComponentItem):
    """number or date - a number or date """
    want = 'nd  '

class number_2c__date_or_text(aetools.ComponentItem):
    """number, date or text - a number, date or text """
    want = 'nds '

class numbers(aetools.ComponentItem):
    """numbers -  """
    want = 'nmbr'

number = numbers

class November(aetools.ComponentItem):
    """November - the month of November """
    want = 'nov '

class number_or_string(aetools.ComponentItem):
    """number or string - a number or string """
    want = 'ns  '

class references(aetools.ComponentItem):
    """references -  """
    want = 'obj '

reference = references

class October(aetools.ComponentItem):
    """October - the month of October """
    want = 'oct '

class ounces(aetools.ComponentItem):
    """ounces - a weight measurement in SI meters """
    want = 'ozs '

class class_(aetools.ComponentItem):
    """class - the type of a value """
    want = 'pcls'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - inherits some of its properties from this class """
    which = 'c@#^'
    want = 'type'

classes = class_

class prepositions(aetools.ComponentItem):
    """prepositions -  """
    want = 'prep'

preposition = prepositions

class properties(aetools.ComponentItem):
    """properties -  """
    want = 'prop'

property = properties

class writing_code(aetools.ComponentItem):
    """writing code - codes that identify the language and script system """
    want = 'psct'

class Pascal_strings(aetools.ComponentItem):
    """Pascal strings -  """
    want = 'pstr'

Pascal_string = Pascal_strings

class quarts(aetools.ComponentItem):
    """quarts - a volume measurement in Imperial quarts """
    want = 'qrts'

class data(aetools.ComponentItem):
    """data - an AppleScript raw data object """
    want = 'rdat'

class records(aetools.ComponentItem):
    """records -  """
    want = 'reco'

record = records

class Saturday(aetools.ComponentItem):
    """Saturday - Saturday """
    want = 'sat '

class seconds(aetools.ComponentItem):
    """seconds - more than one second """
    want = 'scnd'

class script(aetools.ComponentItem):
    """script - An AppleScript script """
    want = 'scpt'
class _Prop_name(aetools.NProperty):
    """name - the name of the script """
    which = 'pnam'
    want = 'TEXT'
class _Prop_parent(aetools.NProperty):
    """parent - its parent, i.e. the script that will handle events that this script doesn\xd5t """
    which = 'pare'
    want = 'scpt'

scripts = script

class September(aetools.ComponentItem):
    """September - the month of September """
    want = 'sep '

class alias_or_string(aetools.ComponentItem):
    """alias or string - an alias or string """
    want = 'sf  '

class sounds(aetools.ComponentItem):
    """sounds -  """
    want = 'snd '

sound = sounds

class square_feet(aetools.ComponentItem):
    """square feet - an area measurement in Imperial square feet """
    want = 'sqft'

class square_kilometers(aetools.ComponentItem):
    """square kilometers - an area measurement in SI square kilometers """
    want = 'sqkm'

square_kilometres = square_kilometers

class square_miles(aetools.ComponentItem):
    """square miles - an area measurement in Imperial square miles """
    want = 'sqmi'

class square_meters(aetools.ComponentItem):
    """square meters - an area measurement in SI square meters """
    want = 'sqrm'

square_metres = square_meters

class square_yards(aetools.ComponentItem):
    """square yards - an area measurement in Imperial square yards """
    want = 'sqyd'

class styled_Clipboard_text(aetools.ComponentItem):
    """styled Clipboard text - clipboard text with font, size, and style information """
    want = 'styl'

styled_Clipboard_text = styled_Clipboard_text

class Sunday(aetools.ComponentItem):
    """Sunday - Sunday """
    want = 'sun '

class styled_Unicode_text(aetools.ComponentItem):
    """styled Unicode text - styled text in the Unicode format """
    want = 'sutx'

styled_Unicode_text = styled_Unicode_text

class Thursday(aetools.ComponentItem):
    """Thursday - Thursday """
    want = 'thu '

class Tuesday(aetools.ComponentItem):
    """Tuesday - Tuesday """
    want = 'tue '

class type_class(aetools.ComponentItem):
    """type class - the name of a particular class (or any four-character code) """
    want = 'type'

class empty_ae_name_(aetools.ComponentItem):
    """ - the undefined value """
    want = 'undf'

class Unicode_text(aetools.ComponentItem):
    """Unicode text - text in the Unicode format (cannot be viewed without conversion) """
    want = 'utxt'

Unicode_text = Unicode_text

class vector(aetools.ComponentItem):
    """vector - An ordered collection of items """
    want = 'vect'

vectors = vector

class version(aetools.ComponentItem):
    """version - a version value """
    want = 'vers'

class Wednesday(aetools.ComponentItem):
    """Wednesday - Wednesday """
    want = 'wed '

class weekdays(aetools.ComponentItem):
    """weekdays -  """
    want = 'wkdy'

weekday = weekdays

class yards(aetools.ComponentItem):
    """yards - a distance measurement in Imperial yards """
    want = 'yard'

class zones(aetools.ComponentItem):
    """zones -  """
    want = 'zone'

zone = zones
anything._superclassnames = []
anything._privpropdict = {
}
anything._privelemdict = {
}
pictures._superclassnames = []
pictures._privpropdict = {
}
pictures._privelemdict = {
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
strings._superclassnames = []
strings._privpropdict = {
}
strings._privelemdict = {
}
alias._superclassnames = []
alias._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
}
alias._privelemdict = {
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
booleans._superclassnames = []
booleans._privpropdict = {
}
booleans._privelemdict = {
}
RGB_colors._superclassnames = []
RGB_colors._privpropdict = {
}
RGB_colors._privelemdict = {
}
application._superclassnames = []
application._privpropdict = {
    'AppleScript' : _Prop_AppleScript,
    'days' : _Prop_days,
    'hours' : _Prop_hours,
    'minutes' : _Prop_minutes,
    'pi' : _Prop_pi,
    'print_depth' : _Prop_print_depth,
    'print_length' : _Prop_print_length,
    'result' : _Prop_result,
    'return_' : _Prop_return_,
    'space' : _Prop_space,
    'tab' : _Prop_tab,
    'text_item_delimiters' : _Prop_text_item_delimiters,
    'weeks' : _Prop_weeks,
}
application._privelemdict = {
}
upper_case._superclassnames = []
upper_case._privpropdict = {
}
upper_case._privelemdict = {
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
characters._superclassnames = []
characters._privpropdict = {
}
characters._privelemdict = {
}
writing_code_info._superclassnames = []
writing_code_info._privpropdict = {
    'language_code' : _Prop_language_code,
    'script_code' : _Prop_script_code,
}
writing_code_info._privelemdict = {
}
text_items._superclassnames = []
text_items._privpropdict = {
}
text_items._privelemdict = {
}
cubic_meters._superclassnames = []
cubic_meters._privpropdict = {
}
cubic_meters._privelemdict = {
}
centimeters._superclassnames = []
centimeters._privpropdict = {
}
centimeters._privelemdict = {
}
item._superclassnames = []
item._privpropdict = {
    'id' : _Prop_id,
}
item._privelemdict = {
}
C_strings._superclassnames = []
C_strings._privpropdict = {
}
C_strings._privelemdict = {
}
text._superclassnames = []
text._privpropdict = {
}
text._privelemdict = {
}
cubic_inches._superclassnames = []
cubic_inches._privpropdict = {
}
cubic_inches._privelemdict = {
}
cubic_yards._superclassnames = []
cubic_yards._privpropdict = {
}
cubic_yards._privelemdict = {
}
December._superclassnames = []
December._privpropdict = {
}
December._privelemdict = {
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
reals._superclassnames = []
reals._privpropdict = {
}
reals._privelemdict = {
}
encoded_strings._superclassnames = []
encoded_strings._privpropdict = {
}
encoded_strings._privelemdict = {
}
constants._superclassnames = []
constants._privpropdict = {
}
constants._privelemdict = {
}
events._superclassnames = []
events._privpropdict = {
}
events._privelemdict = {
}
February._superclassnames = []
February._privpropdict = {
}
February._privelemdict = {
}
feet._superclassnames = []
feet._privpropdict = {
}
feet._privelemdict = {
}
Friday._superclassnames = []
Friday._privpropdict = {
}
Friday._privelemdict = {
}
file_specification._superclassnames = []
file_specification._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
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
handlers._superclassnames = []
handlers._privpropdict = {
}
handlers._privelemdict = {
}
inches._superclassnames = []
inches._privpropdict = {
}
inches._privelemdict = {
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
reference_forms._superclassnames = []
reference_forms._privpropdict = {
}
reference_forms._privelemdict = {
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
keystroke._superclassnames = []
keystroke._privpropdict = {
    'key' : _Prop_key,
    'key_kind' : _Prop_key_kind,
    'modifiers' : _Prop_modifiers,
}
keystroke._privelemdict = {
}
pounds._superclassnames = []
pounds._privpropdict = {
}
pounds._privelemdict = {
}
date._superclassnames = []
date._privpropdict = {
    'date_string' : _Prop_date_string,
    'day' : _Prop_day,
    'month' : _Prop_month,
    'time' : _Prop_time,
    'time_string' : _Prop_time_string,
    'weekday' : _Prop_weekday,
    'year' : _Prop_year,
}
date._privelemdict = {
}
list._superclassnames = []
list._privpropdict = {
    'length' : _Prop_length,
    'rest' : _Prop_rest,
    'reverse' : _Prop_reverse,
}
list._privelemdict = {
}
liters._superclassnames = []
liters._privpropdict = {
}
liters._privelemdict = {
}
linked_list._superclassnames = []
linked_list._privpropdict = {
    'length' : _Prop_length,
}
linked_list._privelemdict = {
}
integers._superclassnames = []
integers._privpropdict = {
}
integers._privelemdict = {
}
list_or_record._superclassnames = []
list_or_record._privpropdict = {
}
list_or_record._privelemdict = {
}
list_2c__record_or_text._superclassnames = []
list_2c__record_or_text._privpropdict = {
}
list_2c__record_or_text._privelemdict = {
}
list_or_string._superclassnames = []
list_or_string._privpropdict = {
}
list_or_string._privelemdict = {
}
machines._superclassnames = []
machines._privpropdict = {
}
machines._privelemdict = {
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
months._superclassnames = []
months._privpropdict = {
}
months._privelemdict = {
}
Monday._superclassnames = []
Monday._privpropdict = {
}
Monday._privelemdict = {
}
missing_values._superclassnames = []
missing_values._privpropdict = {
}
missing_values._privelemdict = {
}
number_or_date._superclassnames = []
number_or_date._privpropdict = {
}
number_or_date._privelemdict = {
}
number_2c__date_or_text._superclassnames = []
number_2c__date_or_text._privpropdict = {
}
number_2c__date_or_text._privelemdict = {
}
numbers._superclassnames = []
numbers._privpropdict = {
}
numbers._privelemdict = {
}
November._superclassnames = []
November._privpropdict = {
}
November._privelemdict = {
}
number_or_string._superclassnames = []
number_or_string._privpropdict = {
}
number_or_string._privelemdict = {
}
references._superclassnames = []
references._privpropdict = {
}
references._privelemdict = {
}
October._superclassnames = []
October._privpropdict = {
}
October._privelemdict = {
}
ounces._superclassnames = []
ounces._privpropdict = {
}
ounces._privelemdict = {
}
class_._superclassnames = ['type_class']
class_._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
class_._privelemdict = {
}
prepositions._superclassnames = []
prepositions._privpropdict = {
}
prepositions._privelemdict = {
}
properties._superclassnames = []
properties._privpropdict = {
}
properties._privelemdict = {
}
writing_code._superclassnames = []
writing_code._privpropdict = {
}
writing_code._privelemdict = {
}
Pascal_strings._superclassnames = []
Pascal_strings._privpropdict = {
}
Pascal_strings._privelemdict = {
}
quarts._superclassnames = []
quarts._privpropdict = {
}
quarts._privelemdict = {
}
data._superclassnames = []
data._privpropdict = {
}
data._privelemdict = {
}
records._superclassnames = []
records._privpropdict = {
}
records._privelemdict = {
}
Saturday._superclassnames = []
Saturday._privpropdict = {
}
Saturday._privelemdict = {
}
seconds._superclassnames = []
seconds._privpropdict = {
}
seconds._privelemdict = {
}
script._superclassnames = []
script._privpropdict = {
    'name' : _Prop_name,
    'parent' : _Prop_parent,
}
script._privelemdict = {
}
September._superclassnames = []
September._privpropdict = {
}
September._privelemdict = {
}
alias_or_string._superclassnames = []
alias_or_string._privpropdict = {
}
alias_or_string._privelemdict = {
}
sounds._superclassnames = []
sounds._privpropdict = {
}
sounds._privelemdict = {
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
square_miles._superclassnames = []
square_miles._privpropdict = {
}
square_miles._privelemdict = {
}
square_meters._superclassnames = []
square_meters._privpropdict = {
}
square_meters._privelemdict = {
}
square_yards._superclassnames = []
square_yards._privpropdict = {
}
square_yards._privelemdict = {
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
Sunday._superclassnames = []
Sunday._privpropdict = {
}
Sunday._privelemdict = {
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
type_class._superclassnames = []
type_class._privpropdict = {
}
type_class._privelemdict = {
}
empty_ae_name_._superclassnames = []
empty_ae_name_._privpropdict = {
}
empty_ae_name_._privelemdict = {
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
vector._superclassnames = []
vector._privpropdict = {
    'length' : _Prop_length,
}
vector._privelemdict = {
}
version._superclassnames = []
version._privpropdict = {
}
version._privelemdict = {
}
Wednesday._superclassnames = []
Wednesday._privpropdict = {
}
Wednesday._privelemdict = {
}
weekdays._superclassnames = []
weekdays._privpropdict = {
}
weekdays._privelemdict = {
}
yards._superclassnames = []
yards._privpropdict = {
}
yards._privelemdict = {
}
zones._superclassnames = []
zones._privpropdict = {
}
zones._privelemdict = {
}
_Enum_boov = {
    'true' : 'true',    # the true boolean value
    'false' : 'fals',   # the false boolean value
}

_Enum_cons = {
    'case' : 'case',    # case
    'diacriticals' : 'diac',    # diacriticals
    'white_space' : 'whit',     # white space
    'hyphens' : 'hyph', # hyphens
    'expansion' : 'expa',       # expansion
    'punctuation' : 'punc',     # punctuation
    'application_responses' : 'rmte',   # remote event replies
}

_Enum_eMds = {
    'option_down' : 'Kopt',     #
    'command_down' : 'Kcmd',    #
    'control_down' : 'Kctl',    #
    'shift_down' : 'Ksft',      #
    'caps_lock_down' : 'Kclk',  #
}

_Enum_ekst = {
    'escape_key' : 'ks5\x00',   #
    'delete_key' : 'ks3\x00',   #
    'tab_key' : 'ks0\x00',      #
    'return_key' : 'ks$\x00',   #
    'clear_key' : 'ksG\x00',    #
    'enter_key' : 'ksL\x00',    #
    'up_arrow_key' : 'ks~\x00', #
    'down_arrow_key' : 'ks}\x00',       #
    'left_arrow_key' : 'ks{\x00',       #
    'right_arrow_key' : 'ks|\x00',      #
    'help_key' : 'ksr\x00',     #
    'home_key' : 'kss\x00',     #
    'page_up_key' : 'kst\x00',  #
    'page_down_key' : 'ksy\x00',        #
    'forward_del_key' : 'ksu\x00',      #
    'end_key' : 'ksw\x00',      #
    'F1_key' : 'ksz\x00',       #
    'F2_key' : 'ksx\x00',       #
    'F3_key' : 'ksc\x00',       #
    'F4_key' : 'ksv\x00',       #
    'F5_key' : 'ks`\x00',       #
    'F6_key' : 'ksa\x00',       #
    'F7_key' : 'ksb\x00',       #
    'F8_key' : 'ksd\x00',       #
    'F9_key' : 'kse\x00',       #
    'F10_key' : 'ksm\x00',      #
    'F11_key' : 'ksg\x00',      #
    'F12_key' : 'kso\x00',      #
    'F13_key' : 'ksi\x00',      #
    'F14_key' : 'ksk\x00',      #
    'F15_key' : 'ksq\x00',      #
}

_Enum_misc = {
    'current_application' : 'cura',     # the current application
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    '****' : anything,
    'PICT' : pictures,
    'STXT' : styled_text,
    'TEXT' : strings,
    'alis' : alias,
    'apr ' : April,
    'aug ' : August,
    'bool' : booleans,
    'cRGB' : RGB_colors,
    'capp' : application,
    'case' : upper_case,
    'ccmt' : cubic_centimeters,
    'cfet' : cubic_feet,
    'cha ' : characters,
    'citl' : writing_code_info,
    'citm' : text_items,
    'cmet' : cubic_meters,
    'cmtr' : centimeters,
    'cobj' : item,
    'cstr' : C_strings,
    'ctxt' : text,
    'cuin' : cubic_inches,
    'cyrd' : cubic_yards,
    'dec ' : December,
    'degc' : degrees_Celsius,
    'degf' : degrees_Fahrenheit,
    'degk' : degrees_Kelvin,
    'doub' : reals,
    'encs' : encoded_strings,
    'enum' : constants,
    'evnt' : events,
    'feb ' : February,
    'feet' : feet,
    'fri ' : Friday,
    'fss ' : file_specification,
    'galn' : gallons,
    'gram' : grams,
    'hand' : handlers,
    'inch' : inches,
    'itxt' : international_text,
    'jan ' : January,
    'jul ' : July,
    'jun ' : June,
    'kfrm' : reference_forms,
    'kgrm' : kilograms,
    'kmtr' : kilometers,
    'kprs' : keystroke,
    'lbs ' : pounds,
    'ldt ' : date,
    'list' : list,
    'litr' : liters,
    'llst' : linked_list,
    'long' : integers,
    'lr  ' : list_or_record,
    'lrs ' : list_2c__record_or_text,
    'ls  ' : list_or_string,
    'mach' : machines,
    'mar ' : March,
    'may ' : May,
    'metr' : meters,
    'mile' : miles,
    'mnth' : months,
    'mon ' : Monday,
    'msng' : missing_values,
    'nd  ' : number_or_date,
    'nds ' : number_2c__date_or_text,
    'nmbr' : numbers,
    'nov ' : November,
    'ns  ' : number_or_string,
    'obj ' : references,
    'oct ' : October,
    'ozs ' : ounces,
    'pcls' : class_,
    'prep' : prepositions,
    'prop' : properties,
    'psct' : writing_code,
    'pstr' : Pascal_strings,
    'qrts' : quarts,
    'rdat' : data,
    'reco' : records,
    'sat ' : Saturday,
    'scnd' : seconds,
    'scpt' : script,
    'sep ' : September,
    'sf  ' : alias_or_string,
    'snd ' : sounds,
    'sqft' : square_feet,
    'sqkm' : square_kilometers,
    'sqmi' : square_miles,
    'sqrm' : square_meters,
    'sqyd' : square_yards,
    'styl' : styled_Clipboard_text,
    'sun ' : Sunday,
    'sutx' : styled_Unicode_text,
    'thu ' : Thursday,
    'tue ' : Tuesday,
    'type' : type_class,
    'undf' : empty_ae_name_,
    'utxt' : Unicode_text,
    'vect' : vector,
    'vers' : version,
    'wed ' : Wednesday,
    'wkdy' : weekdays,
    'yard' : yards,
    'zone' : zones,
}

_propdeclarations = {
    'ID  ' : _Prop_id,
    'ascr' : _Prop_AppleScript,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'day ' : _Prop_day,
    'days' : _Prop_days,
    'dstr' : _Prop_date_string,
    'hour' : _Prop_hours,
    'kMod' : _Prop_modifiers,
    'kMsg' : _Prop_key,
    'kknd' : _Prop_key_kind,
    'leng' : _Prop_length,
    'min ' : _Prop_minutes,
    'mnth' : _Prop_month,
    'pare' : _Prop_parent,
    'pi  ' : _Prop_pi,
    'plcd' : _Prop_language_code,
    'pnam' : _Prop_name,
    'prdp' : _Prop_print_depth,
    'prln' : _Prop_print_length,
    'pscd' : _Prop_script_code,
    'psxp' : _Prop_POSIX_path,
    'rest' : _Prop_rest,
    'ret ' : _Prop_return_,
    'rslt' : _Prop_result,
    'rvse' : _Prop_reverse,
    'spac' : _Prop_space,
    'tab ' : _Prop_tab,
    'time' : _Prop_time,
    'tstr' : _Prop_time_string,
    'txdl' : _Prop_text_item_delimiters,
    'week' : _Prop_weeks,
    'wkdy' : _Prop_weekday,
    'year' : _Prop_year,
}

_compdeclarations = {
}

_enumdeclarations = {
    'boov' : _Enum_boov,
    'cons' : _Enum_cons,
    'eMds' : _Enum_eMds,
    'ekst' : _Enum_ekst,
    'misc' : _Enum_misc,
}
