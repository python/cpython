"""Suite Required: Terms that every application should support
Level 1, version 1

Generated from /Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.5
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'reqd'

from StdSuites.Required_Suite import *
class Required_Events(Required_Suite_Events):

    _argmap_open = {
        'converting' : 'Conv',
    }

    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: list of objects to open
        Keyword argument converting: Whether to convert project to latest version (yes/no; default is ask).
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'odoc'

        aetools.keysubst(_arguments, self._argmap_open)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'Conv', _Enum_Conv)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

_Enum_Conv = {
    'yes' : 'yes ',     # Convert the project if necessary on open
    'no' : 'no  ',      # Do not convert the project if needed on open
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
    'Conv' : _Enum_Conv,
}
