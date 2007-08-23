"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from /Volumes/Sap/Applications (Mac OS 9)/Metrowerks CodeWarrior 7.0/Metrowerks CodeWarrior/CodeWarrior IDE 4.2.5
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'CoRe'

from StdSuites.Standard_Suite import *
class Standard_Suite_Events(Standard_Suite_Events):

    _argmap_close = {
        'saving' : 'savo',
        'saving_in' : 'kfil',
    }

    def close(self, _object, _attributes={}, **_arguments):
        """close: close an object
        Required argument: the object to close
        Keyword argument saving: specifies whether or not changes should be saved before closing
        Keyword argument saving_in: the file in which to save the object
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
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_count = {
        'each' : 'kocl',
    }

    def count(self, _object, _attributes={}, **_arguments):
        """count: return the number of elements of a particular class within an object
        Required argument: the object whose elements are to be counted
        Keyword argument each: the class of the elements to be counted. Keyword 'each' is optional in AppleScript
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the number of elements
        """
        _code = 'core'
        _subcode = 'cnte'

        aetools.keysubst(_arguments, self._argmap_count)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_get = {
        'as' : 'rtyp',
    }

    def get(self, _object, _attributes={}, **_arguments):
        """get: get the data for an object
        Required argument: the object whose data is to be returned
        Keyword argument as: the desired types for the data, in order of preference
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: The data from the object
        """
        _code = 'core'
        _subcode = 'getd'

        aetools.keysubst(_arguments, self._argmap_get)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_make = {
        'new' : 'kocl',
        'as' : 'rtyp',
        'at' : 'insh',
        'with_data' : 'data',
        'with_properties' : 'prdt',
    }

    def make(self, _no_object=None, _attributes={}, **_arguments):
        """make: make a new element
        Keyword argument new: the class of the new element\xd1keyword 'new' is optional in AppleScript
        Keyword argument as: the desired types for the data, in order of preference
        Keyword argument at: the location at which to insert the element
        Keyword argument with_data: the initial data for the element
        Keyword argument with_properties: the initial values for the properties of the element
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the new object(s)
        """
        _code = 'core'
        _subcode = 'crel'

        aetools.keysubst(_arguments, self._argmap_make)
        if _no_object != None: raise TypeError('No direct arg expected')


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def select(self, _object=None, _attributes={}, **_arguments):
        """select: select the specified object
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

    _argmap_set = {
        'to' : 'data',
    }

    def set(self, _object, _attributes={}, **_arguments):
        """set: set an object's data
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
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - an application program """
    want = 'capp'
class _Prop_user_interaction(aetools.NProperty):
    """user interaction - user interaction level """
    which = 'inte'
    want = 'Inte'
user_interaction = _Prop_user_interaction()
#        element 'cwin' as ['indx', 'name', 'rang']
#        element 'docu' as ['indx', 'name', 'rang']

class character(aetools.ComponentItem):
    """character - a character """
    want = 'cha '
class _Prop_length(aetools.NProperty):
    """length - length in characters of this object """
    which = 'pLen'
    want = 'long'
class _Prop_offset(aetools.NProperty):
    """offset - offset of a text object from the beginning of the document (first char has offset 1) """
    which = 'pOff'
    want = 'long'

class insertion_point(aetools.ComponentItem):
    """insertion point - An insertion location between two objects """
    want = 'cins'

class line(aetools.ComponentItem):
    """line - lines of text """
    want = 'clin'
class _Prop_index(aetools.NProperty):
    """index - index of a line object from the beginning of the document (first line has index 1) """
    which = 'pidx'
    want = 'long'
#        element 'cha ' as ['indx', 'rang', 'rele']

lines = line

class selection_2d_object(aetools.ComponentItem):
    """selection-object - the selection visible to the user """
    want = 'csel'
class _Prop_contents(aetools.NProperty):
    """contents - the contents of the selection """
    which = 'pcnt'
    want = 'type'
#        element 'cha ' as ['indx', 'rele', 'rang', 'test']
#        element 'clin' as ['indx', 'rang', 'rele']
#        element 'ctxt' as ['rang']

class text(aetools.ComponentItem):
    """text - Text """
    want = 'ctxt'
#        element 'cha ' as ['indx', 'rele', 'rang']
#        element 'cins' as ['rele']
#        element 'clin' as ['indx', 'rang', 'rele']
#        element 'ctxt' as ['rang']

class window(aetools.ComponentItem):
    """window - A window """
    want = 'cwin'
class _Prop_bounds(aetools.NProperty):
    """bounds - the boundary rectangle for the window """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_document(aetools.NProperty):
    """document - the document that owns this window """
    which = 'docu'
    want = 'docu'
class _Prop_name(aetools.NProperty):
    """name - the title of the window """
    which = 'pnam'
    want = 'itxt'
class _Prop_position(aetools.NProperty):
    """position - upper left coordinates of window """
    which = 'ppos'
    want = 'QDpt'
class _Prop_visible(aetools.NProperty):
    """visible - is the window visible? """
    which = 'pvis'
    want = 'bool'
class _Prop_zoomed(aetools.NProperty):
    """zoomed - Is the window zoomed? """
    which = 'pzum'
    want = 'bool'

windows = window

class document(aetools.ComponentItem):
    """document - a document """
    want = 'docu'
class _Prop_file_permissions(aetools.NProperty):
    """file permissions - the file permissions for the document """
    which = 'PERM'
    want = 'PERM'
class _Prop_kind(aetools.NProperty):
    """kind - the kind of document """
    which = 'DKND'
    want = 'DKND'
class _Prop_location(aetools.NProperty):
    """location - the file of the document """
    which = 'FILE'
    want = 'fss '
class _Prop_window(aetools.NProperty):
    """window - the window of the document. """
    which = 'cwin'
    want = 'cwin'

documents = document

class files(aetools.ComponentItem):
    """files - Every file """
    want = 'file'

file = files
application._superclassnames = []
application._privpropdict = {
    'user_interaction' : _Prop_user_interaction,
}
application._privelemdict = {
    'document' : document,
    'window' : window,
}
character._superclassnames = []
character._privpropdict = {
    'length' : _Prop_length,
    'offset' : _Prop_offset,
}
character._privelemdict = {
}
insertion_point._superclassnames = []
insertion_point._privpropdict = {
    'length' : _Prop_length,
    'offset' : _Prop_offset,
}
insertion_point._privelemdict = {
}
line._superclassnames = []
line._privpropdict = {
    'index' : _Prop_index,
    'length' : _Prop_length,
    'offset' : _Prop_offset,
}
line._privelemdict = {
    'character' : character,
}
selection_2d_object._superclassnames = []
selection_2d_object._privpropdict = {
    'contents' : _Prop_contents,
    'length' : _Prop_length,
    'offset' : _Prop_offset,
}
selection_2d_object._privelemdict = {
    'character' : character,
    'line' : line,
    'text' : text,
}
text._superclassnames = []
text._privpropdict = {
    'length' : _Prop_length,
    'offset' : _Prop_offset,
}
text._privelemdict = {
    'character' : character,
    'insertion_point' : insertion_point,
    'line' : line,
    'text' : text,
}
window._superclassnames = []
window._privpropdict = {
    'bounds' : _Prop_bounds,
    'document' : _Prop_document,
    'index' : _Prop_index,
    'name' : _Prop_name,
    'position' : _Prop_position,
    'visible' : _Prop_visible,
    'zoomed' : _Prop_zoomed,
}
window._privelemdict = {
}
document._superclassnames = []
document._privpropdict = {
    'file_permissions' : _Prop_file_permissions,
    'index' : _Prop_index,
    'kind' : _Prop_kind,
    'location' : _Prop_location,
    'name' : _Prop_name,
    'window' : _Prop_window,
}
document._privelemdict = {
}
files._superclassnames = []
files._privpropdict = {
}
files._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'cha ' : character,
    'cins' : insertion_point,
    'clin' : line,
    'csel' : selection_2d_object,
    'ctxt' : text,
    'cwin' : window,
    'docu' : document,
    'file' : files,
}

_propdeclarations = {
    'DKND' : _Prop_kind,
    'FILE' : _Prop_location,
    'PERM' : _Prop_file_permissions,
    'cwin' : _Prop_window,
    'docu' : _Prop_document,
    'inte' : _Prop_user_interaction,
    'pLen' : _Prop_length,
    'pOff' : _Prop_offset,
    'pbnd' : _Prop_bounds,
    'pcnt' : _Prop_contents,
    'pidx' : _Prop_index,
    'pnam' : _Prop_name,
    'ppos' : _Prop_position,
    'pvis' : _Prop_visible,
    'pzum' : _Prop_zoomed,
}

_compdeclarations = {
}

_enumdeclarations = {
}
