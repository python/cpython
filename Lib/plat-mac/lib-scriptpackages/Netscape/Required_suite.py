"""Suite Required suite:
Level 0, version 0

Generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'reqd'

from StdSuites.Required_Suite import *
class Required_suite_Events(Required_Suite_Events):

    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: list of objects to open
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'odoc'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def print_(self, _object, _attributes={}, **_arguments):
        """print: Print the specified object(s)
        Required argument: list of objects to print
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'pdoc'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def quit(self, _no_object=None, _attributes={}, **_arguments):
        """quit: Quit Navigator
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'quit'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object is not None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def run(self, _no_object=None, _attributes={}, **_arguments):
        """run: Sent to an application when it is double-clicked
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'oapp'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object is not None: raise TypeError, 'No direct arg expected'


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
