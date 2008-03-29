"""Suite builtin_Suite: Every application supports open, reopen, print, run, and quit
Level 1, version 1
"""

import aetools
import MacOS

_code = 'aevt'

class builtin_Suite_Events:

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

    def run(self, _no_object=None, _attributes={}, **_arguments):
        """run: Run an application.      Most applications will open an empty, untitled window.
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

    def reopen(self, _no_object=None, _attributes={}, **_arguments):
        """reopen: Reactivate a running application.  Some applications will open a new untitled window if no window is open.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'rapp'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object is not None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
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

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_quit = {
            'saving' : 'savo',
    }

    def quit(self, _no_object=None, _attributes={}, **_arguments):
        """quit: Quit an application
        Keyword argument saving: specifies whether to save currently open documents
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'quit'

        aetools.keysubst(_arguments, self._argmap_quit)
        if _no_object is not None: raise TypeError, 'No direct arg expected'

        aetools.enumsubst(_arguments, 'savo', _Enum_savo)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                        _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_close = {
            'saving' : 'savo',
            'saving_in' : 'kfil',
    }

_Enum_savo = {
        'yes' : 'yes ', # Save objects now
        'no' : 'no      ',      # Do not save objects
        'ask' : 'ask ', # Ask the user whether to save
}

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
        'savo' : _Enum_savo,
}
