"""Suite Finder Basics: Commonly-used Finder commands and object classes
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_Basics_Events:

    def copy(self, _no_object=None, _attributes={}, **_arguments):
        """copy: (NOT AVAILABLE YET) Copy the selected items to the clipboard (the Finder must be the front application)
        Keyword argument _attributes: AppleEvent attribute dictionary
        """
        _code = 'misc'
        _subcode = 'copy'

        if _arguments: raise TypeError, 'No optional args expected'
        if _no_object != None: raise TypeError, 'No direct arg expected'


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_sort = {
        'by' : 'by  ',
    }

    def sort(self, _object, _attributes={}, **_arguments):
        """sort: (NOT AVAILABLE YET) Return the specified object(s) in a sorted list
        Required argument: a list of finder objects to sort
        Keyword argument by: the property to sort the items by (name, index, date, etc.)
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the sorted items in their new order
        """
        _code = 'DATA'
        _subcode = 'SORT'

        aetools.keysubst(_arguments, self._argmap_sort)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error, aetools.decodeerror(_arguments)
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - The Finder """
    want = 'capp'
class _Prop_Finder_preferences(aetools.NProperty):
    """Finder preferences - (NOT AVAILABLE YET) Various preferences that apply to the Finder as a whole """
    which = 'pfrp'
    want = 'cprf'
Finder_preferences = _Prop_Finder_preferences()
class _Prop_clipboard(aetools.NProperty):
    """clipboard - (NOT AVAILABLE YET) the Finder\xd5s clipboard window """
    which = 'pcli'
    want = 'obj '
clipboard = _Prop_clipboard()
class _Prop_desktop(aetools.NProperty):
    """desktop - the desktop """
    which = 'desk'
    want = 'cdsk'
desktop = _Prop_desktop()
class _Prop_frontmost(aetools.NProperty):
    """frontmost - Is the Finder the frontmost process? """
    which = 'pisf'
    want = 'bool'
frontmost = _Prop_frontmost()
class _Prop_home(aetools.NProperty):
    """home - the home directory """
    which = 'home'
    want = 'cfol'
home = _Prop_home()
class _Prop_insertion_location(aetools.NProperty):
    """insertion location - the container in which a new folder would appear if \xd2New Folder\xd3 was selected """
    which = 'pins'
    want = 'obj '
insertion_location = _Prop_insertion_location()
class _Prop_name(aetools.NProperty):
    """name - the Finder\xd5s name """
    which = 'pnam'
    want = 'itxt'
name = _Prop_name()
class _Prop_product_version(aetools.NProperty):
    """product version - the version of the System software running on this computer """
    which = 'ver2'
    want = 'utxt'
product_version = _Prop_product_version()
class _Prop_selection(aetools.NProperty):
    """selection - the selection in the frontmost Finder window """
    which = 'sele'
    want = 'obj '
selection = _Prop_selection()
class _Prop_startup_disk(aetools.NProperty):
    """startup disk - the startup disk """
    which = 'sdsk'
    want = 'cdis'
startup_disk = _Prop_startup_disk()
class _Prop_trash(aetools.NProperty):
    """trash - the trash """
    which = 'trsh'
    want = 'ctrs'
trash = _Prop_trash()
class _Prop_version(aetools.NProperty):
    """version - the version of the Finder """
    which = 'vers'
    want = 'utxt'
version = _Prop_version()
class _Prop_visible(aetools.NProperty):
    """visible - Is the Finder\xd5s layer visible? """
    which = 'pvis'
    want = 'bool'
visible = _Prop_visible()
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'brow' as ['indx', 'ID  ']
#        element 'cdis' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'rele', 'name', 'rang', 'test']
#        element 'ctnr' as ['indx', 'name']
#        element 'cwin' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'lwnd' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
application._superclassnames = []
import Files
import Window_classes
import Containers_and_folders
import Finder_items
application._privpropdict = {
    'Finder_preferences' : _Prop_Finder_preferences,
    'clipboard' : _Prop_clipboard,
    'desktop' : _Prop_desktop,
    'frontmost' : _Prop_frontmost,
    'home' : _Prop_home,
    'insertion_location' : _Prop_insertion_location,
    'name' : _Prop_name,
    'product_version' : _Prop_product_version,
    'selection' : _Prop_selection,
    'startup_disk' : _Prop_startup_disk,
    'trash' : _Prop_trash,
    'version' : _Prop_version,
    'visible' : _Prop_visible,
}
application._privelemdict = {
    'Finder_window' : Window_classes.Finder_window,
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'clipping_window' : Window_classes.clipping_window,
    'container' : Containers_and_folders.container,
    'disk' : Containers_and_folders.disk,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : Containers_and_folders.folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
    'window' : Window_classes.window,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
}

_propdeclarations = {
    'desk' : _Prop_desktop,
    'home' : _Prop_home,
    'pcli' : _Prop_clipboard,
    'pfrp' : _Prop_Finder_preferences,
    'pins' : _Prop_insertion_location,
    'pisf' : _Prop_frontmost,
    'pnam' : _Prop_name,
    'pvis' : _Prop_visible,
    'sdsk' : _Prop_startup_disk,
    'sele' : _Prop_selection,
    'trsh' : _Prop_trash,
    'ver2' : _Prop_product_version,
    'vers' : _Prop_version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
