"""Suite Power Suite: Terms and Events for controlling System power
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'powr'

class Power_Suite_Events:

    def restart(self, _object, _attributes={}, **_arguments):
        """restart: Restart the computer
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'rest'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def shut_down(self, _object, _attributes={}, **_arguments):
        """shut down: Shut Down the computer
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'shut'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def sleep(self, _object, _attributes={}, **_arguments):
        """sleep: Put the computer to sleep
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'fndr'
        _subcode = 'slep'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
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
