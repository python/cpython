"""Suite Standard Suite: Common classes and commands for most applications.
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = '????'

class Standard_Suite_Events:

    _argmap_close = {
        'saving_in' : 'kfil',
        'saving' : 'savo',
    }

    def close(self, _object, _attributes={}, **_arguments):
        """close: Close an object.
        Required argument: the object for the command
        Keyword argument saving_in: The file in which to save the object.
        Keyword argument saving: Specifies whether changes should be saved before closing.
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
        """count: Return the number of elements of a particular class within an object.
        Required argument: the object for the command
        Keyword argument each: The class of objects to be counted.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
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

    def delete(self, _object, _attributes={}, **_arguments):
        """delete: Delete an object.
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'delo'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_duplicate = {
        'to' : 'insh',
        'with_properties' : 'prdt',
    }

    def duplicate(self, _object, _attributes={}, **_arguments):
        """duplicate: Copy object(s) and put the copies at a new location.
        Required argument: the object for the command
        Keyword argument to: The location for the new object(s).
        Keyword argument with_properties: Properties to be set in the new duplicated object(s).
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'clon'

        aetools.keysubst(_arguments, self._argmap_duplicate)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def exists(self, _object, _attributes={}, **_arguments):
        """exists: Verify if an object exists.
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'core'
        _subcode = 'doex'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def get(self, _object, _attributes={}, **_arguments):
        """get: Get the data for an object.
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'core'
        _subcode = 'getd'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_make = {
        'at' : 'insh',
        'new' : 'kocl',
        'with_data' : 'data',
        'with_properties' : 'prdt',
    }

    def make(self, _no_object=None, _attributes={}, **_arguments):
        """make: Make a new object.
        Keyword argument at: The location at which to insert the object.
        Keyword argument new: The class of the new object.
        Keyword argument with_data: The initial data for the object.
        Keyword argument with_properties: The initial values for properties of the object.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'core'
        _subcode = 'crel'

        aetools.keysubst(_arguments, self._argmap_make)
        if _no_object is not None: raise TypeError('No direct arg expected')


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_move = {
        'to' : 'insh',
    }

    def move(self, _object, _attributes={}, **_arguments):
        """move: Move object(s) to a new location.
        Required argument: the object for the command
        Keyword argument to: The new location for the object(s).
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'move'

        aetools.keysubst(_arguments, self._argmap_move)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def open(self, _object=None, _attributes={}, **_arguments):
        """open: Open an object.
        Required argument: list of objects
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'odoc'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def print_(self, _object=None, _attributes={}, **_arguments):
        """print: Print an object.
        Required argument: list of objects
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'pdoc'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_quit = {
        'saving' : 'savo',
    }

    def quit(self, _object, _attributes={}, **_arguments):
        """quit: Quit an application.
        Required argument: the object for the command
        Keyword argument saving: Specifies whether changes should be saved before quitting.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'quit'

        aetools.keysubst(_arguments, self._argmap_quit)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'savo', _Enum_savo)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_save = {
        'in_' : 'kfil',
        'as' : 'fltp',
    }

    def save(self, _object, _attributes={}, **_arguments):
        """save: Save an object.
        Required argument: the object for the command
        Keyword argument in_: The file in which to save the object.
        Keyword argument as: The file type in which to save the data.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'save'

        aetools.keysubst(_arguments, self._argmap_save)
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
        """set: Set an object's data.
        Required argument: the object for the command
        Keyword argument to: The new value.
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
    """application - An application's top level scripting object. """
    want = 'capp'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - All of the properties of the superclass. """
    which = 'c@#^'
    want = 'cobj'
_3c_Inheritance_3e_ = _Prop__3c_Inheritance_3e_()
class _Prop_frontmost(aetools.NProperty):
    """frontmost - Is this the frontmost (active) application? """
    which = 'pisf'
    want = 'bool'
frontmost = _Prop_frontmost()
class _Prop_name(aetools.NProperty):
    """name - The name of the application. """
    which = 'pnam'
    want = 'utxt'
name = _Prop_name()
class _Prop_version(aetools.NProperty):
    """version - The version of the application. """
    which = 'vers'
    want = 'utxt'
version = _Prop_version()
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test', 'ID  ']
#        element 'docu' as ['name', 'indx', 'rele', 'rang', 'test']

applications = application

class item(aetools.ComponentItem):
    """item - A scriptable object. """
    want = 'cobj'
class _Prop_class_(aetools.NProperty):
    """class - The class of the object. """
    which = 'pcls'
    want = 'type'
class _Prop_properties(aetools.NProperty):
    """properties - All of the object's properties. """
    which = 'pALL'
    want = 'reco'

items = item

class color(aetools.ComponentItem):
    """color - A color. """
    want = 'colr'

colors = color

class window(aetools.ComponentItem):
    """window - A window. """
    want = 'cwin'
class _Prop_bounds(aetools.NProperty):
    """bounds - The bounding rectangle of the window. """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_closeable(aetools.NProperty):
    """closeable - Whether the window has a close box. """
    which = 'hclb'
    want = 'bool'
class _Prop_document(aetools.NProperty):
    """document - The document whose contents are being displayed in the window. """
    which = 'docu'
    want = 'docu'
class _Prop_floating(aetools.NProperty):
    """floating - Whether the window floats. """
    which = 'isfl'
    want = 'bool'
class _Prop_id(aetools.NProperty):
    """id - The unique identifier of the window. """
    which = 'ID  '
    want = 'long'
class _Prop_index(aetools.NProperty):
    """index - The index of the window in the back-to-front window ordering. """
    which = 'pidx'
    want = 'long'
class _Prop_miniaturizable(aetools.NProperty):
    """miniaturizable - Whether the window can be miniaturized. """
    which = 'ismn'
    want = 'bool'
class _Prop_miniaturized(aetools.NProperty):
    """miniaturized - Whether the window is currently miniaturized. """
    which = 'pmnd'
    want = 'bool'
class _Prop_modal(aetools.NProperty):
    """modal - Whether the window is the application's current modal window. """
    which = 'pmod'
    want = 'bool'
class _Prop_resizable(aetools.NProperty):
    """resizable - Whether the window can be resized. """
    which = 'prsz'
    want = 'bool'
class _Prop_titled(aetools.NProperty):
    """titled - Whether the window has a title bar. """
    which = 'ptit'
    want = 'bool'
class _Prop_visible(aetools.NProperty):
    """visible - Whether the window is currently visible. """
    which = 'pvis'
    want = 'bool'
class _Prop_zoomable(aetools.NProperty):
    """zoomable - Whether the window can be zoomed. """
    which = 'iszm'
    want = 'bool'
class _Prop_zoomed(aetools.NProperty):
    """zoomed - Whether the window is currently zoomed. """
    which = 'pzum'
    want = 'bool'

windows = window

class document(aetools.ComponentItem):
    """document - A document. """
    want = 'docu'
class _Prop_modified(aetools.NProperty):
    """modified - Has the document been modified since the last save? """
    which = 'imod'
    want = 'bool'
class _Prop_path(aetools.NProperty):
    """path - The document's path. """
    which = 'ppth'
    want = 'utxt'

documents = document
application._superclassnames = ['item']
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'frontmost' : _Prop_frontmost,
    'name' : _Prop_name,
    'version' : _Prop_version,
}
application._privelemdict = {
    'document' : document,
    'window' : window,
}
item._superclassnames = []
item._privpropdict = {
    'class_' : _Prop_class_,
    'properties' : _Prop_properties,
}
item._privelemdict = {
}
color._superclassnames = ['item']
color._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
color._privelemdict = {
}
window._superclassnames = ['item']
window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'bounds' : _Prop_bounds,
    'closeable' : _Prop_closeable,
    'document' : _Prop_document,
    'floating' : _Prop_floating,
    'id' : _Prop_id,
    'index' : _Prop_index,
    'miniaturizable' : _Prop_miniaturizable,
    'miniaturized' : _Prop_miniaturized,
    'modal' : _Prop_modal,
    'name' : _Prop_name,
    'resizable' : _Prop_resizable,
    'titled' : _Prop_titled,
    'visible' : _Prop_visible,
    'zoomable' : _Prop_zoomable,
    'zoomed' : _Prop_zoomed,
}
window._privelemdict = {
}
document._superclassnames = ['item']
document._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'modified' : _Prop_modified,
    'name' : _Prop_name,
    'path' : _Prop_path,
}
document._privelemdict = {
}
class _3c_(aetools.NComparison):
    """< - Less than """
class _3d_(aetools.NComparison):
    """= - Equal """
class _3e_(aetools.NComparison):
    """> - Greater than """
class contains(aetools.NComparison):
    """contains - Contains """
class ends_with(aetools.NComparison):
    """ends with - Ends with """
class starts_with(aetools.NComparison):
    """starts with - Starts with """
class _b2_(aetools.NComparison):
    """\xb2 - Less than or equal to """
class _b3_(aetools.NComparison):
    """\xb3 - Greater than or equal to """
_Enum_savo = {
    'ask' : 'ask ',     # Ask the user whether or not to save the file.
    'yes' : 'yes ',     # Save the file.
    'no' : 'no  ',      # Do not save the file.
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'cobj' : item,
    'colr' : color,
    'cwin' : window,
    'docu' : document,
}

_propdeclarations = {
    'ID  ' : _Prop_id,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'docu' : _Prop_document,
    'hclb' : _Prop_closeable,
    'imod' : _Prop_modified,
    'isfl' : _Prop_floating,
    'ismn' : _Prop_miniaturizable,
    'iszm' : _Prop_zoomable,
    'pALL' : _Prop_properties,
    'pbnd' : _Prop_bounds,
    'pcls' : _Prop_class_,
    'pidx' : _Prop_index,
    'pisf' : _Prop_frontmost,
    'pmnd' : _Prop_miniaturized,
    'pmod' : _Prop_modal,
    'pnam' : _Prop_name,
    'ppth' : _Prop_path,
    'prsz' : _Prop_resizable,
    'ptit' : _Prop_titled,
    'pvis' : _Prop_visible,
    'pzum' : _Prop_zoomed,
    'vers' : _Prop_version,
}

_compdeclarations = {
    '<   ' : _3c_,
    '<=  ' : _b2_,
    '=   ' : _3d_,
    '>   ' : _3e_,
    '>=  ' : _b3_,
    'bgwt' : starts_with,
    'cont' : contains,
    'ends' : ends_with,
}

_enumdeclarations = {
    'savo' : _Enum_savo,
}
