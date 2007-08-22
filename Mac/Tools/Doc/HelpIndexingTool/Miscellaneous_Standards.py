"""Suite Miscellaneous Standards: Useful events that aren\xd5t in any other suite
Level 0, version 0

Generated from /Developer/Applications/Apple Help Indexing Tool.app
AETE/AEUT resource version 1/1, language 0, script 0
"""

import aetools
import MacOS

_code = 'misc'

class Miscellaneous_Standards_Events:

    def revert(self, _object, _attributes={}, **_arguments):
        """revert: Revert an object to the most recently saved version
        Required argument: object to revert
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'rvrt'

        if _arguments: raise TypeError('No optional args expected')
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
