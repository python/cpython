"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from /Applications/Internet Explorer.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = '****'

class Standard_Suite_Events:

    _argmap_get = {
        'as' : 'rtyp',
    }

    def get(self, _object, _attributes={}, **_arguments):
        """get:
        Required argument: an AE object reference
        Keyword argument as: undocumented, typecode 'type'
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: anything
        """
        _code = 'core'
        _subcode = 'getd'

        aetools.keysubst(_arguments, self._argmap_get)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - An application program """
    want = 'capp'
class _Prop_selected_text(aetools.NProperty):
    """selected text - the selected text """
    which = 'stxt'
    want = 'TEXT'
selected_text = _Prop_selected_text()
application._superclassnames = []
application._privpropdict = {
    'selected_text' : _Prop_selected_text,
}
application._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
}

_propdeclarations = {
    'stxt' : _Prop_selected_text,
}

_compdeclarations = {
}

_enumdeclarations = {
}
