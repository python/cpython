"""Suite Standard Suite: Common terms that most applications should support
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'CoRe'

from StdSuites.Standard_Suite import *
class Standard_Suite_Events(Standard_Suite_Events):

    def close(self, _object, _attributes={}, **_arguments):
        """close: Close an object
        Required argument: the object to close
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'core'
        _subcode = 'clos'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_count = {
        'each' : 'kocl',
    }

    def count(self, _object, _attributes={}, **_arguments):
        """count: Return the number of elements of a particular class within an object
        Required argument: the object whose elements are to be counted
        Keyword argument each: the class of the elements to be counted
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
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_data_size = {
        'as' : 'rtyp',
    }

    def data_size(self, _object, _attributes={}, **_arguments):
        """data size: Return the size in bytes of an object
        Required argument: the object whose data size is to be returned
        Keyword argument as: the data type for which the size is calculated
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the size of the object in bytes
        """
        _code = 'core'
        _subcode = 'dsiz'

        aetools.keysubst(_arguments, self._argmap_data_size)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def delete(self, _object, _attributes={}, **_arguments):
        """delete: Move an item from its container to the trash
        Required argument: the item to delete
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the item that was just deleted
        """
        _code = 'core'
        _subcode = 'delo'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_duplicate = {
        'to' : 'insh',
        'replacing' : 'alrp',
        'routing_suppressed' : 'rout',
    }

    def duplicate(self, _object, _attributes={}, **_arguments):
        """duplicate: Duplicate one or more object(s)
        Required argument: the object(s) to duplicate
        Keyword argument to: the new location for the object(s)
        Keyword argument replacing: Specifies whether or not to replace items in the destination that have the same name as items being duplicated
        Keyword argument routing_suppressed: Specifies whether or not to autoroute items (default is false). Only applies when copying to the system folder.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the duplicated object(s)
        """
        _code = 'core'
        _subcode = 'clon'

        aetools.keysubst(_arguments, self._argmap_duplicate)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'alrp', _Enum_bool)
        aetools.enumsubst(_arguments, 'rout', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def exists(self, _object, _attributes={}, **_arguments):
        """exists: Verify if an object exists
        Required argument: the object in question
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: true if it exists, false if not
        """
        _code = 'core'
        _subcode = 'doex'

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
        'to' : 'to  ',
        'with_properties' : 'prdt',
    }

    def make(self, _no_object=None, _attributes={}, **_arguments):
        """make: Make a new element
        Keyword argument new: the class of the new element
        Keyword argument at: the location at which to insert the element
        Keyword argument to: when creating an alias file, the original item to create an alias to or when creating a file viewer window, the target of the window
        Keyword argument with_properties: the initial values for the properties of the element
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the new object(s)
        """
        _code = 'core'
        _subcode = 'crel'

        aetools.keysubst(_arguments, self._argmap_make)
        if _no_object is not None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_move = {
        'to' : 'insh',
        'replacing' : 'alrp',
        'positioned_at' : 'mvpl',
        'routing_suppressed' : 'rout',
    }

    def move(self, _object, _attributes={}, **_arguments):
        """move: Move object(s) to a new location
        Required argument: the object(s) to move
        Keyword argument to: the new location for the object(s)
        Keyword argument replacing: Specifies whether or not to replace items in the destination that have the same name as items being moved
        Keyword argument positioned_at: Gives a list (in local window coordinates) of positions for the destination items
        Keyword argument routing_suppressed: Specifies whether or not to autoroute items (default is false). Only applies when moving to the system folder.
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the object(s) after they have been moved
        """
        _code = 'core'
        _subcode = 'move'

        aetools.keysubst(_arguments, self._argmap_move)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'alrp', _Enum_bool)
        aetools.enumsubst(_arguments, 'mvpl', _Enum_list)
        aetools.enumsubst(_arguments, 'rout', _Enum_bool)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_open = {
        'using' : 'usin',
        'with_properties' : 'prdt',
    }

    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: list of objects to open
        Keyword argument using: the application file to open the object with
        Keyword argument with_properties: the initial values for the properties, to be included with the open command sent to the application that opens the direct object
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'odoc'

        aetools.keysubst(_arguments, self._argmap_open)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_print_ = {
        'with_properties' : 'prdt',
    }

    def print_(self, _object, _attributes={}, **_arguments):
        """print: Print the specified object(s)
        Required argument: list of objects to print
        Keyword argument with_properties: optional properties to be included with the print command sent to the application that prints the direct object
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'pdoc'

        aetools.keysubst(_arguments, self._argmap_print_)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def quit(self, _no_object=None, _attributes={}, **_arguments):
        """quit: Quit the Finder
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'quit'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object is not None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def select(self, _object, _attributes={}, **_arguments):
        """select: Select the specified object(s)
        Required argument: the object to select
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'slct'

        if _arguments: raise TypeError, 'No optional args expected'
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

_Enum_list = None # XXXX enum list not found!!
_Enum_bool = None # XXXX enum bool not found!!

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
