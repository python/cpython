"""Suite Processes Suite: Terms and Events for controlling Processes
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'prcs'

class Processes_Suite_Events:

    def cancel(self, _object, _attributes={}, **_arguments):
        """cancel: cause the target process to behave as if the UI element were cancelled
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'cncl'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_click = {
        'at' : 'insh',
    }

    def click(self, _object, _attributes={}, **_arguments):
        """click: cause the target process to behave as if the UI element were clicked
        Required argument: the object for the command
        Keyword argument at: when sent to a "process" object, the { x, y } location at which to click, in global coordinates
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'clic'

        aetools.keysubst(_arguments, self._argmap_click)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def confirm(self, _object, _attributes={}, **_arguments):
        """confirm: cause the target process to behave as if the UI element were confirmed
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'cnfm'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def decrement(self, _object, _attributes={}, **_arguments):
        """decrement: cause the target process to behave as if the UI element were decremented
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'decr'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def increment(self, _object, _attributes={}, **_arguments):
        """increment: cause the target process to behave as if the UI element were incremented
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'incE'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def key_down(self, _object, _attributes={}, **_arguments):
        """key down: cause the target process to behave as if keys were held down
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'keyF'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def key_up(self, _object, _attributes={}, **_arguments):
        """key up: cause the target process to behave as if keys were released
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'keyU'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_keystroke = {
        'with' : 'with',
    }

    def keystroke(self, _object, _attributes={}, **_arguments):
        """keystroke: cause the target process to behave as if keystrokes were entered
        Required argument: the object for the command
        Keyword argument with: modifiers with which the keystrokes are to be entered
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'kprs'

        aetools.keysubst(_arguments, self._argmap_keystroke)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'with', _Enum_eMds)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def pick(self, _object, _attributes={}, **_arguments):
        """pick: cause the target process to behave as if the UI element were picked
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'prcs'
        _subcode = 'pick'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def select(self, _object, _attributes={}, **_arguments):
        """select: set the selected property of the UI element
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'slct'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class browser(aetools.ComponentItem):
    """browser - A browser belonging to a window """
    want = 'broW'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - All of the properties of the superclass. """
    which = 'c@#^'
    want = 'uiel'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

browsers = browser

class busy_indicator(aetools.ComponentItem):
    """busy indicator - A busy indicator belonging to a window """
    want = 'busi'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

busy_indicators = busy_indicator

class button(aetools.ComponentItem):
    """button - A button belonging to a window or scroll bar """
    want = 'butT'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

buttons = button

class application(aetools.ComponentItem):
    """application - A application presenting UI elements """
    want = 'capp'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

applications = application

class column(aetools.ComponentItem):
    """column - A column belonging to a table """
    want = 'ccol'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

columns = column

class check_box(aetools.ComponentItem):
    """check box - A check box belonging to a window """
    want = 'chbx'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

check_boxes = check_box

class color_well(aetools.ComponentItem):
    """color well - A color well belonging to a window """
    want = 'colW'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

color_wells = color_well

class combo_box(aetools.ComponentItem):
    """combo box - A combo box belonging to a window """
    want = 'comB'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

combo_boxes = combo_box

class row(aetools.ComponentItem):
    """row - A row belonging to a table """
    want = 'crow'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

rows = row

class window(aetools.ComponentItem):
    """window - A window belonging to a process """
    want = 'cwin'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

windows = window

class drawer(aetools.ComponentItem):
    """drawer - A drawer that may be extended from a window """
    want = 'draA'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

drawers = drawer

class grow_area(aetools.ComponentItem):
    """grow area - A grow area belonging to a window """
    want = 'grow'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

grow_areas = grow_area

class image(aetools.ComponentItem):
    """image - An image belonging to a static text field """
    want = 'imaA'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

images = image

class incrementor(aetools.ComponentItem):
    """incrementor - A incrementor belonging to a window """
    want = 'incr'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

incrementors = incrementor

class list(aetools.ComponentItem):
    """list - A list belonging to a window """
    want = 'list'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

lists = list

class menu_bar(aetools.ComponentItem):
    """menu bar - A menu bar belonging to a process """
    want = 'mbar'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

menu_bars = menu_bar

class menu_button(aetools.ComponentItem):
    """menu button - A menu button belonging to a window """
    want = 'menB'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

menu_buttons = menu_button

class menu(aetools.ComponentItem):
    """menu - A menu belonging to a menu bar """
    want = 'menE'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

menus = menu

class menu_item(aetools.ComponentItem):
    """menu item - A menu item belonging to a menu """
    want = 'menI'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

menu_items = menu_item

class outline(aetools.ComponentItem):
    """outline - A outline belonging to a window """
    want = 'outl'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

outlines = outline

class application_process(aetools.ComponentItem):
    """application process - A process launched from an application file """
    want = 'pcap'
class _Prop_application_file(aetools.NProperty):
    """application file - a reference to the application file from which this process was launched """
    which = 'appf'
    want = '****'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

application_processes = application_process

class desk_accessory_process(aetools.ComponentItem):
    """desk accessory process - A process launched from an desk accessory file """
    want = 'pcda'
class _Prop_desk_accessory_file(aetools.NProperty):
    """desk accessory file - a reference to the desk accessory file from which this process was launched """
    which = 'dafi'
    want = '****'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

desk_accessory_processes = desk_accessory_process

class pop_up_button(aetools.ComponentItem):
    """pop up button - A pop up button belonging to a window """
    want = 'popB'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

pop_up_buttons = pop_up_button

class process(aetools.ComponentItem):
    """process - A process running on this computer """
    want = 'prcs'
class _Prop_Classic(aetools.NProperty):
    """Classic - Is the process running in the Classic environment? """
    which = 'clsc'
    want = 'bool'
class _Prop_accepts_high_level_events(aetools.NProperty):
    """accepts high level events - Is the process high-level event aware (accepts open application, open document, print document, and quit)? """
    which = 'isab'
    want = 'bool'
class _Prop_accepts_remote_events(aetools.NProperty):
    """accepts remote events - Does the process accept remote events? """
    which = 'revt'
    want = 'bool'
class _Prop_creator_type(aetools.NProperty):
    """creator type - the OSType of the creator of the process (the signature) """
    which = 'fcrt'
    want = 'utxt'
class _Prop_displayed_name(aetools.NProperty):
    """displayed name - the name of the file from which the process was launched, as displayed in the User Interface """
    which = 'dnam'
    want = 'utxt'
class _Prop_file(aetools.NProperty):
    """file - the file from which the process was launched """
    which = 'file'
    want = '****'
class _Prop_file_type(aetools.NProperty):
    """file type - the OSType of the file type of the process """
    which = 'asty'
    want = 'utxt'
class _Prop_frontmost(aetools.NProperty):
    """frontmost - Is the process the frontmost process """
    which = 'pisf'
    want = 'bool'
class _Prop_has_scripting_terminology(aetools.NProperty):
    """has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
    which = 'hscr'
    want = 'bool'
class _Prop_name(aetools.NProperty):
    """name - the name of the process """
    which = 'pnam'
    want = 'utxt'
class _Prop_partition_space_used(aetools.NProperty):
    """partition space used - the number of bytes currently used in the process' partition """
    which = 'pusd'
    want = 'magn'
class _Prop_properties(aetools.NProperty):
    """properties - every property of the process """
    which = 'pALL'
    want = '****'
class _Prop_total_partition_size(aetools.NProperty):
    """total partition size - the size of the partition with which the process was launched """
    which = 'appt'
    want = 'magn'
class _Prop_visible(aetools.NProperty):
    """visible - Is the process' layer visible? """
    which = 'pvis'
    want = 'bool'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

processes = process

class progress_indicator(aetools.ComponentItem):
    """progress indicator - A progress indicator belonging to a window """
    want = 'proI'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

progress_indicators = progress_indicator

class radio_button(aetools.ComponentItem):
    """radio button - A radio button belonging to a window """
    want = 'radB'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

radio_buttons = radio_button

class relevance_indicator(aetools.ComponentItem):
    """relevance indicator - A relevance indicator belonging to a window """
    want = 'reli'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

relevance_indicators = relevance_indicator

class radio_group(aetools.ComponentItem):
    """radio group - A radio button group belonging to a window """
    want = 'rgrp'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

radio_groups = radio_group

class scroll_area(aetools.ComponentItem):
    """scroll area - A scroll area belonging to a window """
    want = 'scra'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

scroll_areas = scroll_area

class scroll_bar(aetools.ComponentItem):
    """scroll bar - A scroll bar belonging to a window """
    want = 'scrb'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

scroll_bars = scroll_bar

class group(aetools.ComponentItem):
    """group - A group belonging to a window """
    want = 'sgrp'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

groups = group

class sheet(aetools.ComponentItem):
    """sheet - A sheet displayed over a window """
    want = 'sheE'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

sheets = sheet

class slider(aetools.ComponentItem):
    """slider - A slider belonging to a window """
    want = 'sliI'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

sliders = slider

class splitter_group(aetools.ComponentItem):
    """splitter group - A splitter group belonging to a window """
    want = 'splg'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

splitter_groups = splitter_group

class splitter(aetools.ComponentItem):
    """splitter - A splitter belonging to a window """
    want = 'splr'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

splitters = splitter

class static_text(aetools.ComponentItem):
    """static text - A static text field belonging to a window """
    want = 'sttx'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

static_texts = static_text

class system_wide_UI_element(aetools.ComponentItem):
    """system wide UI element - The system wide UI element, a container for UI elements that do not belong to a process or application """
    want = 'sysw'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

class tab_group(aetools.ComponentItem):
    """tab group - A tab group belonging to a window """
    want = 'tab '
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

tab_groups = tab_group

class table(aetools.ComponentItem):
    """table - A table belonging to a window """
    want = 'tabB'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

tables = table

class tool_bar(aetools.ComponentItem):
    """tool bar - A tool bar belonging to a window """
    want = 'tbar'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

tool_bars = tool_bar

class text_area(aetools.ComponentItem):
    """text area - A text area belonging to a window """
    want = 'txta'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

text_areas = text_area

class text_field(aetools.ComponentItem):
    """text field - A text field belonging to a window """
    want = 'txtf'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

text_fields = text_field

class UI_element(aetools.ComponentItem):
    """UI element - A piece of the user interface of a process """
    want = 'uiel'
class _Prop_class_(aetools.NProperty):
    """class - the class of the UI Element, which identifies it function """
    which = 'pcls'
    want = 'type'
class _Prop_description(aetools.NProperty):
    """description - a more complete description of the UI element and its capabilities """
    which = 'desc'
    want = 'utxt'
class _Prop_enabled(aetools.NProperty):
    """enabled - Is the UI element enabled? ( Does it accept clicks? ) """
    which = 'enab'
    want = 'bool'
class _Prop_focused(aetools.NProperty):
    """focused - Is the focus on this UI element? """
    which = 'focu'
    want = 'bool'
class _Prop_help(aetools.NProperty):
    """help - an encoded description of the UI element and its capabilities """
    which = 'help'
    want = 'utxt'
class _Prop_maximum(aetools.NProperty):
    """maximum - the maximum vale that the UI element can take on """
    which = 'maxi'
    want = 'long'
class _Prop_minimum(aetools.NProperty):
    """minimum - the minimum vale that the UI element can take on """
    which = 'mini'
    want = 'long'
class _Prop_orientation(aetools.NProperty):
    """orientation - the orientation of the UI element """
    which = 'orie'
    want = 'utxt'
class _Prop_position(aetools.NProperty):
    """position - the position of the UI element """
    which = 'posn'
    want = 'QDpt'
class _Prop_role(aetools.NProperty):
    """role - an encoded description of the UI element and its capabilities """
    which = 'role'
    want = 'utxt'
class _Prop_selected(aetools.NProperty):
    """selected - Is the UI element selected? """
    which = 'selE'
    want = '****'
class _Prop_size(aetools.NProperty):
    """size - the size of the UI element """
    which = 'ptsz'
    want = 'QDpt'
class _Prop_subrole(aetools.NProperty):
    """subrole - an encoded description of the UI element and its capabilities """
    which = 'sbrl'
    want = 'utxt'
class _Prop_title(aetools.NProperty):
    """title - the title of the UI element as it appears on the screen """
    which = 'titl'
    want = 'utxt'
class _Prop_value(aetools.NProperty):
    """value - the current value of the UI element """
    which = 'valu'
    want = 'long'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

UI_elements = UI_element

class value_indicator(aetools.ComponentItem):
    """value indicator - A value indicator ( thumb or slider ) belonging to a scroll bar """
    want = 'vali'
#        element 'broW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'busi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'butT' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'capp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'ccol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'chbx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'colW' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'comB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'crow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'draA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'grow' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'imaA' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'incr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'list' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'mbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'menI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'outl' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'popB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'proI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'radB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'reli' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'rgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scra' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'scrb' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sgrp' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sheE' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sliI' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splg' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'splr' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sttx' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'sysw' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tab ' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tabB' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'tbar' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txta' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'txtf' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'vali' as ['name', 'indx', 'rele', 'rang', 'test']

value_indicators = value_indicator
browser._superclassnames = ['UI_element']
browser._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
browser._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
busy_indicator._superclassnames = ['UI_element']
busy_indicator._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
busy_indicator._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
button._superclassnames = ['UI_element']
button._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
button._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
application._superclassnames = ['UI_element']
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
application._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
column._superclassnames = ['UI_element']
column._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
column._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
check_box._superclassnames = ['UI_element']
check_box._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
check_box._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
color_well._superclassnames = ['UI_element']
color_well._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
color_well._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
combo_box._superclassnames = ['UI_element']
combo_box._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
combo_box._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
row._superclassnames = ['UI_element']
row._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
row._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
window._superclassnames = ['UI_element']
window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
window._privelemdict = {
    'UI_element' : UI_element,
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'button' : button,
    'check_box' : check_box,
    'check_box' : check_box,
    'color_well' : color_well,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'drawer' : drawer,
    'group' : group,
    'group' : group,
    'grow_area' : grow_area,
    'grow_area' : grow_area,
    'image' : image,
    'image' : image,
    'incrementor' : incrementor,
    'incrementor' : incrementor,
    'list' : list,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'sheet' : sheet,
    'slider' : slider,
    'slider' : slider,
    'splitter' : splitter,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'tab_group' : tab_group,
    'table' : table,
    'table' : table,
    'text_area' : text_area,
    'text_area' : text_area,
    'text_field' : text_field,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
drawer._superclassnames = ['UI_element']
drawer._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
drawer._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
grow_area._superclassnames = ['UI_element']
grow_area._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
grow_area._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
image._superclassnames = ['UI_element']
image._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
image._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
incrementor._superclassnames = ['UI_element']
incrementor._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
incrementor._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
list._superclassnames = ['UI_element']
list._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
list._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
menu_bar._superclassnames = ['UI_element']
menu_bar._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
menu_bar._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
menu_button._superclassnames = ['UI_element']
menu_button._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
menu_button._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
menu._superclassnames = ['UI_element']
menu._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
menu._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
menu_item._superclassnames = ['UI_element']
menu_item._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
menu_item._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
outline._superclassnames = ['UI_element']
outline._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
outline._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
application_process._superclassnames = ['process']
application_process._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'application_file' : _Prop_application_file,
}
application_process._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
    'window' : window,
}
desk_accessory_process._superclassnames = ['process']
desk_accessory_process._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'desk_accessory_file' : _Prop_desk_accessory_file,
}
desk_accessory_process._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
    'window' : window,
}
pop_up_button._superclassnames = ['UI_element']
pop_up_button._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
pop_up_button._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
process._superclassnames = ['UI_element']
process._privpropdict = {
    'Classic' : _Prop_Classic,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'accepts_high_level_events' : _Prop_accepts_high_level_events,
    'accepts_remote_events' : _Prop_accepts_remote_events,
    'creator_type' : _Prop_creator_type,
    'displayed_name' : _Prop_displayed_name,
    'file' : _Prop_file,
    'file_type' : _Prop_file_type,
    'frontmost' : _Prop_frontmost,
    'has_scripting_terminology' : _Prop_has_scripting_terminology,
    'name' : _Prop_name,
    'partition_space_used' : _Prop_partition_space_used,
    'properties' : _Prop_properties,
    'total_partition_size' : _Prop_total_partition_size,
    'visible' : _Prop_visible,
}
process._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
    'window' : window,
}
progress_indicator._superclassnames = ['UI_element']
progress_indicator._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
progress_indicator._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
radio_button._superclassnames = ['UI_element']
radio_button._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
radio_button._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
relevance_indicator._superclassnames = ['UI_element']
relevance_indicator._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
relevance_indicator._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
radio_group._superclassnames = ['UI_element']
radio_group._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
radio_group._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
scroll_area._superclassnames = ['UI_element']
scroll_area._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
scroll_area._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
scroll_bar._superclassnames = ['UI_element']
scroll_bar._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
scroll_bar._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'value_indicator' : value_indicator,
    'window' : window,
}
group._superclassnames = ['UI_element']
group._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
group._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
sheet._superclassnames = ['UI_element']
sheet._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
sheet._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
slider._superclassnames = ['UI_element']
slider._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
slider._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
splitter_group._superclassnames = ['UI_element']
splitter_group._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
splitter_group._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
splitter._superclassnames = ['UI_element']
splitter._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
splitter._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
static_text._superclassnames = ['UI_element']
static_text._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
static_text._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
system_wide_UI_element._superclassnames = ['UI_element']
system_wide_UI_element._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
system_wide_UI_element._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
tab_group._superclassnames = ['UI_element']
tab_group._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
tab_group._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
table._superclassnames = ['UI_element']
table._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
table._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
tool_bar._superclassnames = ['UI_element']
tool_bar._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
tool_bar._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
text_area._superclassnames = ['UI_element']
text_area._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
text_area._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
text_field._superclassnames = ['UI_element']
text_field._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
text_field._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
import Standard_Suite
UI_element._superclassnames = ['item']
UI_element._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'class_' : _Prop_class_,
    'description' : _Prop_description,
    'enabled' : _Prop_enabled,
    'focused' : _Prop_focused,
    'help' : _Prop_help,
    'maximum' : _Prop_maximum,
    'minimum' : _Prop_minimum,
    'name' : _Prop_name,
    'orientation' : _Prop_orientation,
    'position' : _Prop_position,
    'role' : _Prop_role,
    'selected' : _Prop_selected,
    'size' : _Prop_size,
    'subrole' : _Prop_subrole,
    'title' : _Prop_title,
    'value' : _Prop_value,
}
UI_element._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
value_indicator._superclassnames = ['UI_element']
value_indicator._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
value_indicator._privelemdict = {
    'UI_element' : UI_element,
    'application' : application,
    'browser' : browser,
    'busy_indicator' : busy_indicator,
    'button' : button,
    'check_box' : check_box,
    'color_well' : color_well,
    'column' : column,
    'combo_box' : combo_box,
    'drawer' : drawer,
    'group' : group,
    'grow_area' : grow_area,
    'image' : image,
    'incrementor' : incrementor,
    'list' : list,
    'menu' : menu,
    'menu_bar' : menu_bar,
    'menu_button' : menu_button,
    'menu_item' : menu_item,
    'outline' : outline,
    'pop_up_button' : pop_up_button,
    'progress_indicator' : progress_indicator,
    'radio_button' : radio_button,
    'radio_group' : radio_group,
    'relevance_indicator' : relevance_indicator,
    'row' : row,
    'scroll_area' : scroll_area,
    'scroll_bar' : scroll_bar,
    'sheet' : sheet,
    'slider' : slider,
    'splitter' : splitter,
    'splitter_group' : splitter_group,
    'static_text' : static_text,
    'system_wide_UI_element' : system_wide_UI_element,
    'tab_group' : tab_group,
    'table' : table,
    'text_area' : text_area,
    'text_field' : text_field,
    'tool_bar' : tool_bar,
    'value_indicator' : value_indicator,
    'window' : window,
}
_Enum_eMds = {
    'command_down' : 'Kcmd',	# command down
    'option_down' : 'Kopt',	# option down
    'control_down' : 'Kctl',	# control down
    'shift_down' : 'Ksft',	# shift down
}

_Enum_eMky = {
    'control' : 'eCnt',	# control
    'shift' : 'eSft',	# shift
    'command' : 'eCmd',	# command
    'option' : 'eOpt',	# option
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'broW' : browser,
    'busi' : busy_indicator,
    'butT' : button,
    'capp' : application,
    'ccol' : column,
    'chbx' : check_box,
    'colW' : color_well,
    'comB' : combo_box,
    'crow' : row,
    'cwin' : window,
    'draA' : drawer,
    'grow' : grow_area,
    'imaA' : image,
    'incr' : incrementor,
    'list' : list,
    'mbar' : menu_bar,
    'menB' : menu_button,
    'menE' : menu,
    'menI' : menu_item,
    'outl' : outline,
    'pcap' : application_process,
    'pcda' : desk_accessory_process,
    'popB' : pop_up_button,
    'prcs' : process,
    'proI' : progress_indicator,
    'radB' : radio_button,
    'reli' : relevance_indicator,
    'rgrp' : radio_group,
    'scra' : scroll_area,
    'scrb' : scroll_bar,
    'sgrp' : group,
    'sheE' : sheet,
    'sliI' : slider,
    'splg' : splitter_group,
    'splr' : splitter,
    'sttx' : static_text,
    'sysw' : system_wide_UI_element,
    'tab ' : tab_group,
    'tabB' : table,
    'tbar' : tool_bar,
    'txta' : text_area,
    'txtf' : text_field,
    'uiel' : UI_element,
    'vali' : value_indicator,
}

_propdeclarations = {
    'appf' : _Prop_application_file,
    'appt' : _Prop_total_partition_size,
    'asty' : _Prop_file_type,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'clsc' : _Prop_Classic,
    'dafi' : _Prop_desk_accessory_file,
    'desc' : _Prop_description,
    'dnam' : _Prop_displayed_name,
    'enab' : _Prop_enabled,
    'fcrt' : _Prop_creator_type,
    'file' : _Prop_file,
    'focu' : _Prop_focused,
    'help' : _Prop_help,
    'hscr' : _Prop_has_scripting_terminology,
    'isab' : _Prop_accepts_high_level_events,
    'maxi' : _Prop_maximum,
    'mini' : _Prop_minimum,
    'orie' : _Prop_orientation,
    'pALL' : _Prop_properties,
    'pcls' : _Prop_class_,
    'pisf' : _Prop_frontmost,
    'pnam' : _Prop_name,
    'posn' : _Prop_position,
    'ptsz' : _Prop_size,
    'pusd' : _Prop_partition_space_used,
    'pvis' : _Prop_visible,
    'revt' : _Prop_accepts_remote_events,
    'role' : _Prop_role,
    'sbrl' : _Prop_subrole,
    'selE' : _Prop_selected,
    'titl' : _Prop_title,
    'valu' : _Prop_value,
}

_compdeclarations = {
}

_enumdeclarations = {
    'eMds' : _Enum_eMds,
    'eMky' : _Enum_eMky,
}
