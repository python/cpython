"""Suite Standard Suite: Common terms for most applications
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'core'

from _builtinSuites.builtin_Suite import *
class Standard_Suite_Events(builtin_Suite_Events):

    _argmap_class_info = {
        'in_' : 'wrcd',
    }

    def class_info(self, _object=None, _attributes={}, **_arguments):
        """class info: (optional) Get information about an object class
        Required argument: the object class about which information is requested
        Keyword argument in_: the human language and script system in which to return information
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a record containing the object\xd5s properties and elements
        """
        _code = 'core'
        _subcode = 'qobj'

        aetools.keysubst(_arguments, self._argmap_class_info)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_close = {
        'saving' : 'savo',
        'saving_in' : 'kfil',
    }

    def close(self, _object, _attributes={}, **_arguments):
        """close: Close an object
        Required argument: the object to close
        Keyword argument saving: specifies whether changes should be saved before closing
        Keyword argument saving_in: the file or alias in which to save the object
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

    _argmap_count = {
        'each' : 'kocl',
    }

    def count(self, _object, _attributes={}, **_arguments):
        """count: Return the number of elements of an object
        Required argument: the object whose elements are to be counted
        Keyword argument each: if specified, restricts counting to objects of this class
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
        """data size: (optional) Return the size in bytes of an object
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
        """delete: Delete an object from its container. Note this does not work on script variables, only on elements of application classes.
        Required argument: the element to delete
        Keyword argument _attributes: AppleEvent attribute dictionary
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
        'with_properties' : 'prdt',
    }

    def duplicate(self, _object, _attributes={}, **_arguments):
        """duplicate: Duplicate one or more objects
        Required argument: the object(s) to duplicate
        Keyword argument to: the new location for the object(s)
        Keyword argument with_properties: the initial values for properties of the new object that are to be different from the original
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the duplicated object(s)
        """
        _code = 'core'
        _subcode = 'clon'

        aetools.keysubst(_arguments, self._argmap_duplicate)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_event_info = {
        'in_' : 'wrcd',
    }

    def event_info(self, _object, _attributes={}, **_arguments):
        """event info: (optional) Get information about the Apple events in a suite
        Required argument: the event class of the Apple events for which to return information
        Keyword argument in_: the human language and script system in which to return information
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a record containing the events and their parameters
        """
        _code = 'core'
        _subcode = 'gtei'

        aetools.keysubst(_arguments, self._argmap_event_info)
        _arguments['----'] = _object


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

    def handleBreakpoint(self, _object, _attributes={}, **_arguments):
        """handleBreakpoint: return true to stop at a breakpoint
        Required argument: the call frame of the breakpoint
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: true to stop, false if not
        """
        _code = 'core'
        _subcode = 'brak'

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
        Returns: to the new object(s)
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

    _argmap_move = {
        'to' : 'insh',
    }

    def move(self, _object, _attributes={}, **_arguments):
        """move: Move object(s) to a new location
        Required argument: the object(s) to move
        Keyword argument to: the new location for the object(s)
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: to the object(s) after they have been moved
        """
        _code = 'core'
        _subcode = 'move'

        aetools.keysubst(_arguments, self._argmap_move)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def open(self, _object, _attributes={}, **_arguments):
        """open: Open the specified object(s)
        Required argument: list of objects to open
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
        Required argument: list of objects to print
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

    _argmap_quit = {
        'saving' : 'savo',
    }

    def quit(self, _no_object=None, _attributes={}, **_arguments):
        """quit: Quit an application
        Keyword argument saving: specifies whether to save currently open documents
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'quit'

        aetools.keysubst(_arguments, self._argmap_quit)
        if _no_object != None: raise TypeError, 'No direct arg expected'

        aetools.enumsubst(_arguments, 'savo', _Enum_savo)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def reopen(self, _no_object=None, _attributes={}, **_arguments):
        """reopen: Reactivate a running application.  Some applications will open a new untitled window if no window is open.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'rapp'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def run(self, _no_object=None, _attributes={}, **_arguments):
        """run: Run an application.  Most applications will open an empty, untitled window.
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'aevt'
        _subcode = 'oapp'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


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
        """save: Save an object
        Required argument: the object to save, usually a document or window
        Keyword argument in_: the file or alias in which to save the object
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

    def select(self, _object, _attributes={}, **_arguments):
        """select: Make a selection
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

    _argmap_suite_info = {
        'in_' : 'wrcd',
    }

    def suite_info(self, _object, _attributes={}, **_arguments):
        """suite info: (optional) Get information about event suite(s)
        Required argument: the suite for which to return information
        Keyword argument in_: the human language and script system in which to return information
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: a record containing the suites and their versions
        """
        _code = 'core'
        _subcode = 'gtsi'

        aetools.keysubst(_arguments, self._argmap_suite_info)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class alias(aetools.ComponentItem):
    """alias - a file on a disk or server.  The file must exist when you check the syntax of your script. """
    want = 'alis'
class _Prop_POSIX_path(aetools.NProperty):
    """POSIX path - the POSIX path of the file """
    which = 'psxp'
    want = 'TEXT'

aliases = alias

class application(aetools.ComponentItem):
    """application - An application program """
    want = 'capp'
class _Prop_clipboard(aetools.NProperty):
    """clipboard - the contents of the clipboard for this application """
    which = 'pcli'
    want = '****'
clipboard = _Prop_clipboard()
class _Prop_frontmost(aetools.NProperty):
    """frontmost - Is this the frontmost application? """
    which = 'pisf'
    want = 'bool'
frontmost = _Prop_frontmost()
class _Prop_name(aetools.NProperty):
    """name - the name of the application """
    which = 'pnam'
    want = 'itxt'
name = _Prop_name()
class _Prop_selection(aetools.NProperty):
    """selection - the selection visible to the user.  Use the \xd4select\xd5 command to set a new selection; use \xd4contents of selection\xd5 to get or change information in the document. """
    which = 'sele'
    want = 'csel'
selection = _Prop_selection()
class _Prop_version(aetools.NProperty):
    """version - the version of the application """
    which = 'vers'
    want = 'vers'
version = _Prop_version()

applications = application

class insertion_points(aetools.ComponentItem):
    """insertion points -  """
    want = 'cins'

insertion_point = insertion_points

class selection_2d_object(aetools.ComponentItem):
    """selection-object - A way to refer to the state of the current of the selection.  Use the \xd4select\xd5 command to make a new selection. """
    want = 'csel'
class _Prop_contents(aetools.NProperty):
    """contents - the information currently selected.  Use \xd4contents of selection\xd5 to get or change information in a document. """
    which = 'pcnt'
    want = '****'

class window(aetools.ComponentItem):
    """window - A window """
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
class _Prop_resizable(aetools.NProperty):
    """resizable - Is the window resizable? """
    which = 'prsz'
    want = 'bool'
class _Prop_titled(aetools.NProperty):
    """titled - Does the window have a title bar? """
    which = 'ptit'
    want = 'bool'
class _Prop_visible(aetools.NProperty):
    """visible - Is the window visible? """
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

windows = window

class document(aetools.ComponentItem):
    """document - A document of a scriptable application """
    want = 'docu'
class _Prop_modified(aetools.NProperty):
    """modified - Has the document been modified since the last save? """
    which = 'imod'
    want = 'bool'

documents = document

class file(aetools.ComponentItem):
    """file - a file on a disk or server """
    want = 'file'

files = file
alias._superclassnames = []
alias._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
}
alias._privelemdict = {
}
application._superclassnames = []
application._privpropdict = {
    'clipboard' : _Prop_clipboard,
    'frontmost' : _Prop_frontmost,
    'name' : _Prop_name,
    'selection' : _Prop_selection,
    'version' : _Prop_version,
}
application._privelemdict = {
}
insertion_points._superclassnames = []
insertion_points._privpropdict = {
}
insertion_points._privelemdict = {
}
selection_2d_object._superclassnames = []
selection_2d_object._privpropdict = {
    'contents' : _Prop_contents,
}
selection_2d_object._privelemdict = {
}
window._superclassnames = []
window._privpropdict = {
    'bounds' : _Prop_bounds,
    'closeable' : _Prop_closeable,
    'floating' : _Prop_floating,
    'index' : _Prop_index,
    'modal' : _Prop_modal,
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
}
document._privelemdict = {
}
file._superclassnames = []
file._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
}
file._privelemdict = {
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
_Enum_kfrm = {
    'index' : 'indx',   # keyform designating indexed access
    'named' : 'name',   # keyform designating named access
    'id' : 'ID  ',      # keyform designating access by unique identifier
}

_Enum_savo = {
    'yes' : 'yes ',     # Save objects now
    'no' : 'no  ',      # Do not save objects
    'ask' : 'ask ',     # Ask the user whether to save
}

_Enum_styl = {
    'plain' : 'plan',   # Plain
    'bold' : 'bold',    # Bold
    'italic' : 'ital',  # Italic
    'outline' : 'outl', # Outline
    'shadow' : 'shad',  # Shadow
    'underline' : 'undl',       # Underline
    'superscript' : 'spsc',     # Superscript
    'subscript' : 'sbsc',       # Subscript
    'strikethrough' : 'strk',   # Strikethrough
    'small_caps' : 'smcp',      # Small caps
    'all_caps' : 'alcp',        # All capital letters
    'all_lowercase' : 'lowc',   # Lowercase
    'condensed' : 'cond',       # Condensed
    'expanded' : 'pexp',        # Expanded
    'hidden' : 'hidn',  # Hidden
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'alis' : alias,
    'capp' : application,
    'cins' : insertion_points,
    'csel' : selection_2d_object,
    'cwin' : window,
    'docu' : document,
    'file' : file,
}

_propdeclarations = {
    'hclb' : _Prop_closeable,
    'imod' : _Prop_modified,
    'isfl' : _Prop_floating,
    'iszm' : _Prop_zoomable,
    'pbnd' : _Prop_bounds,
    'pcli' : _Prop_clipboard,
    'pcnt' : _Prop_contents,
    'pidx' : _Prop_index,
    'pisf' : _Prop_frontmost,
    'pmod' : _Prop_modal,
    'pnam' : _Prop_name,
    'prsz' : _Prop_resizable,
    'psxp' : _Prop_POSIX_path,
    'ptit' : _Prop_titled,
    'pvis' : _Prop_visible,
    'pzum' : _Prop_zoomed,
    'sele' : _Prop_selection,
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
    'kfrm' : _Enum_kfrm,
    'savo' : _Enum_savo,
    'styl' : _Enum_styl,
}
