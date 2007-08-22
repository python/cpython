"""Suite Help Indexing Tool Suite: Special events that just the Help Indexing Tool supports.
Level 0, version 0

Generated from /Developer/Applications/Apple Help Indexing Tool.app
AETE/AEUT resource version 1/1, language 0, script 0
"""

import aetools
import MacOS

_code = 'HIT '

class Help_Indexing_Tool_Suite_Events:

    def turn_anchor_indexing(self, _object, _attributes={}, **_arguments):
        """turn anchor indexing: Turns anchor indexing on or off.
        Required argument: \xd2on\xd3 or \xd2off\xd3, to turn anchor indexing on or off
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'HIT '
        _subcode = 'tAnc'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_turn_remote_root = {
        'with_root_url' : 'rURL',
    }

    def turn_remote_root(self, _object, _attributes={}, **_arguments):
        """turn remote root: Turn usage of remote root for content on the web on or off. If turning \xd2on\xd3, supply a string as second parameter.
        Required argument: \xd2on\xd3 or \xd2off\xd3, to turn remote root on or off
        Keyword argument with_root_url: The remote root to use, in the form of \xd2http://www.apple.com/help/\xd3.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'HIT '
        _subcode = 'tRem'

        aetools.keysubst(_arguments, self._argmap_turn_remote_root)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def use_tokenizer(self, _object, _attributes={}, **_arguments):
        """use tokenizer: Tells the indexing tool which tokenizer to use.
        Required argument: Specify \xd2English\xd3, \xd2European\xd3, \xd2Japanese\xd3, \xd2Korean\xd3, or \xd2Simple\xd3.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'HIT '
        _subcode = 'uTok'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - Application class """
    want = 'capp'
class _Prop_idleStatus(aetools.NProperty):
    """idleStatus -  """
    which = 'sIdl'
    want = 'bool'
application._superclassnames = []
application._privpropdict = {
    'idleStatus' : _Prop_idleStatus,
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
    'sIdl' : _Prop_idleStatus,
}

_compdeclarations = {
}

_enumdeclarations = {
}
