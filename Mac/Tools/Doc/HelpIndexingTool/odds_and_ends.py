"""Suite odds and ends: Things that should be in some standard suite, but aren\xd5t
Level 1, version 1

Generated from /Developer/Applications/Apple Help Indexing Tool.app
AETE/AEUT resource version 1/1, language 0, script 0
"""

import aetools
import MacOS

_code = 'Odds'

class odds_and_ends_Events:

    def select(self, _object=None, _attributes={}, **_arguments):
        """select: Select the specified object
        Required argument: the object to select
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'slct'

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
