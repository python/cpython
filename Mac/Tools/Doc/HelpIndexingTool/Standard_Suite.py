"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from /Developer/Applications/Apple Help Indexing Tool.app
AETE/AEUT resource version 1/1, language 0, script 0
"""

import aetools
import MacOS

_code = 'CoRe'

from StdSuites.Standard_Suite import *
class Standard_Suite_Events(Standard_Suite_Events):

    _argmap_close = {
        'saving' : 'savo',
        'in_' : 'kfil',
    }

    def close(self, _object, _attributes={}, **_arguments):
        """close: Close an object
        Required argument: the objects to close
        Keyword argument saving: specifies whether or not changes should be saved before closing
        Keyword argument in_: the file in which to save the object
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'clos'

        aetools.keysubst(_arguments, self._argmap_close)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'savo', _Enum_savo)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def data_size(self, _object, _attributes={}, **_arguments):
        """data size: Return the size in bytes of an object
        Required argument: the object whose data size is to be returned
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the size of the object in bytes
        """
        _code = 'core'
        _subcode = 'dsiz'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def get(self, _object, _attributes={}, **_arguments):
        """get: Get the data for an object
        Required argument: the object whose data is to be returned
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: The data from the object
        """
        _code = 'core'
        _subcode = 'getd'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_make = {
        'new' : 'kocl',
        'at' : 'insh',
        'with_data' : 'data',
        'with_properties' : 'prdt',
    }

    def make(self, _no_object=None, _attributes={}, **_arguments):
        """make: Make a new element
        Keyword argument new: the class of the new element
        Keyword argument at: the location at which to insert the element
        Keyword argument with_data: the initial data for the element
        Keyword argument with_properties: the initial values for the properties of the element
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: Object specifier for the new element
        """
        _code = 'core'
        _subcode = 'crel'

        aetools.keysubst(_arguments, self._argmap_make)
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: Objects to open. Can be a list of files or an object specifier.
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
        Required argument: Objects to print. Can be a list of files or an object specifier.
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

    _argmap_save = {
        'in_' : 'kfil',
        'as' : 'fltp',
    }

    def save(self, _object, _attributes={}, **_arguments):
        """save: save a set of objects
        Required argument: Objects to save.
        Keyword argument in_: the file in which to save the object(s)
        Keyword argument as: the file type of the document in which to save the data
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'save'

        aetools.keysubst(_arguments, self._argmap_save)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_set = {
        'to' : 'data',
    }

    def set(self, _object, _attributes={}, **_arguments):
        """set: Set an object\xd5s data
        Required argument: the object to change
        Keyword argument to: the new value
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'setd'

        aetools.keysubst(_arguments, self._argmap_set)
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
#        element 'cwin' as ['indx', 'name', 'rele']
#        element 'docu' as ['name']

class window(aetools.ComponentItem):
    """window - A Window """
    want = 'cwin'
class _Prop_bounds(aetools.NProperty):
    """bounds - the boundary rectangle for the window """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_closeable(aetools.NProperty):
    """closeable - Does the window have a close box? """
    which = 'hclb'
    want = 'bool'
class _Prop_floating(aetools.NProperty):
    """floating - Does the window float? """
    which = 'isfl'
    want = 'bool'
class _Prop_index(aetools.NProperty):
    """index - the number of the window """
    which = 'pidx'
    want = 'long'
class _Prop_modal(aetools.NProperty):
    """modal - Is the window modal? """
    which = 'pmod'
    want = 'bool'
class _Prop_name(aetools.NProperty):
    """name - the title of the window """
    which = 'pnam'
    want = 'itxt'
class _Prop_position(aetools.NProperty):
    """position - upper left coordinates of window """
    which = 'ppos'
    want = 'QDpt'
class _Prop_resizable(aetools.NProperty):
    """resizable - Is the window resizable? """
    which = 'prsz'
    want = 'bool'
class _Prop_titled(aetools.NProperty):
    """titled - Does the window have a title bar? """
    which = 'ptit'
    want = 'bool'
class _Prop_visible(aetools.NProperty):
    """visible - is the window visible? """
    which = 'pvis'
    want = 'bool'
class _Prop_zoomable(aetools.NProperty):
    """zoomable - Is the window zoomable? """
    which = 'iszm'
    want = 'bool'
class _Prop_zoomed(aetools.NProperty):
    """zoomed - Is the window zoomed? """
    which = 'pzum'
    want = 'bool'

class document(aetools.ComponentItem):
    """document - A Document """
    want = 'docu'
class _Prop_modified(aetools.NProperty):
    """modified - Has the document been modified since the last save? """
    which = 'imod'
    want = 'bool'
application._superclassnames = []
application._privpropdict = {
}
application._privelemdict = {
    'document' : document,
    'window' : window,
}
window._superclassnames = []
window._privpropdict = {
    'bounds' : _Prop_bounds,
    'closeable' : _Prop_closeable,
    'floating' : _Prop_floating,
    'index' : _Prop_index,
    'modal' : _Prop_modal,
    'name' : _Prop_name,
    'position' : _Prop_position,
    'resizable' : _Prop_resizable,
    'titled' : _Prop_titled,
    'visible' : _Prop_visible,
    'zoomable' : _Prop_zoomable,
    'zoomed' : _Prop_zoomed,
}
window._privelemdict = {
}
document._superclassnames = []
document._privpropdict = {
    'modified' : _Prop_modified,
    'name' : _Prop_name,
}
document._privelemdict = {
}
_Enum_savo = {
    'yes' : 'yes ',     # Save objects now
    'no' : 'no  ',      # Do not save objects
    'ask' : 'ask ',     # Ask the user whether to save
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'cwin' : window,
    'docu' : document,
}

_propdeclarations = {
    'hclb' : _Prop_closeable,
    'imod' : _Prop_modified,
    'isfl' : _Prop_floating,
    'iszm' : _Prop_zoomable,
    'pbnd' : _Prop_bounds,
    'pidx' : _Prop_index,
    'pmod' : _Prop_modal,
    'pnam' : _Prop_name,
    'ppos' : _Prop_position,
    'prsz' : _Prop_resizable,
    'ptit' : _Prop_titled,
    'pvis' : _Prop_visible,
    'pzum' : _Prop_zoomed,
}

_compdeclarations = {
}

_enumdeclarations = {
    'savo' : _Enum_savo,
}
