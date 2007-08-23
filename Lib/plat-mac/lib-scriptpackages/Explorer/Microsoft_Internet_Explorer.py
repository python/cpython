"""Suite Microsoft Internet Explorer Suite: Events defined by Internet Explorer
Level 1, version 1

Generated from /Applications/Internet Explorer.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'MSIE'

class Microsoft_Internet_Explorer_Events:

    def GetSource(self, _object=None, _attributes={}, **_arguments):
        """GetSource: Get the HTML source of a browser window
        Required argument: Window Identifier of window from which to get the source. No value means get the source from the frontmost window.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: undocumented, typecode 'TEXT'
        """
        _code = 'MSIE'
        _subcode = 'SORC'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def PrintBrowserWindow(self, _object=None, _attributes={}, **_arguments):
        """PrintBrowserWindow: Print contents of browser window (HTML)
        Required argument: Window Identifier of the window to print. No value means print the frontmost browser window.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'pWND'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_do_script = {
        'window' : 'WIND',
    }

    def do_script(self, _object, _attributes={}, **_arguments):
        """do script: Execute script commands
        Required argument: JavaScript text to execute
        Keyword argument window: optional Window Identifier (as supplied by the ListWindows event) specifying context in which to execute the script
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Return value
        """
        _code = 'misc'
        _subcode = 'dosc'

        aetools.keysubst(_arguments, self._argmap_do_script)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
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
