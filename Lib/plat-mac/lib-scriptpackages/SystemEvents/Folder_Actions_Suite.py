"""Suite Folder Actions Suite: Terms and Events for controlling Folder Actions
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

# Crutch for recursive import
import sys
import SystemEvents
SystemEvents.Folder_Actions_Suite = sys.modules[__name__]

import aetools
import MacOS

_code = 'faco'

class Folder_Actions_Suite_Events:

    _argmap_attach_action_to = {
        'using' : 'faal',
    }

    def attach_action_to(self, _object, _attributes={}, **_arguments):
        """attach action to: Attach an action to a folder
        Required argument: the object for the command
        Keyword argument using: a file containing the script to attach
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'faco'
        _subcode = 'atfa'

        aetools.keysubst(_arguments, self._argmap_attach_action_to)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    def attached_scripts(self, _object, _attributes={}, **_arguments):
        """attached scripts: List the actions attached to a folder
        Required argument: the object for the command
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'faco'
        _subcode = 'lact'

        if _arguments: raise TypeError('No optional args expected')
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_do_folder_action = {
        'with_window_size' : 'fnsz',
        'with_item_list' : 'flst',
        'folder_action_code' : 'actn',
    }

    def do_folder_action(self, _object, _attributes={}, **_arguments):
        """do folder action: Event the Finder sends to the Folder Actions FBA
        Required argument: the object for the command
        Keyword argument with_window_size: the new window size for the folder action message to process
        Keyword argument with_item_list: a list of items for the folder action message to process
        Keyword argument folder_action_code: the folder action message to process
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'faco'
        _subcode = 'fola'

        aetools.keysubst(_arguments, self._argmap_do_folder_action)
        _arguments['----'] = _object

        aetools.enumsubst(_arguments, 'actn', _Enum_actn)

        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_edit_action_of = {
        'using_action_name' : 'snam',
        'using_action_number' : 'indx',
    }

    def edit_action_of(self, _object, _attributes={}, **_arguments):
        """edit action of: Edit as action of a folder
        Required argument: the object for the command
        Keyword argument using_action_name: ...or the name of the action to edit
        Keyword argument using_action_number: the index number of the action to edit...
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'faco'
        _subcode = 'edfa'

        aetools.keysubst(_arguments, self._argmap_edit_action_of)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']

    _argmap_remove_action_from = {
        'using_action_name' : 'snam',
        'using_action_number' : 'indx',
    }

    def remove_action_from(self, _object, _attributes={}, **_arguments):
        """remove action from: Remove a folder action from a folder
        Required argument: the object for the command
        Keyword argument using_action_name: ...or the name of the action to remove
        Keyword argument using_action_number: the index number of the action to remove...
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
        """
        _code = 'faco'
        _subcode = 'rmfa'

        aetools.keysubst(_arguments, self._argmap_remove_action_from)
        _arguments['----'] = _object


        _reply, _arguments, _attributes = self.send(_code, _subcode,
                _arguments, _attributes)
        if _arguments.get('errn', 0):
            raise aetools.Error(aetools.decodeerror(_arguments))
        # XXXX Optionally decode result
        if _arguments.has_key('----'):
            return _arguments['----']


class application(aetools.ComponentItem):
    """application - The Folder Actions Suite host program """
    want = 'capp'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - All of the properties of the superclass. """
    which = 'c@#^'
    want = 'capp'
_3c_Inheritance_3e_ = _Prop__3c_Inheritance_3e_()
class _Prop_folder_actions_enabled(aetools.NProperty):
    """folder actions enabled - Are Folder Actions currently being processed? """
    which = 'faen'
    want = 'bool'
folder_actions_enabled = _Prop_folder_actions_enabled()
class _Prop_properties(aetools.NProperty):
    """properties - every property of the Folder Actions Suite host program """
    which = 'pALL'
    want = '****'
properties = _Prop_properties()
#        element 'cdis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test', 'ID  ']
#        element 'docu' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'foac' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'logi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'pcap' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'pcda' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'prcs' as ['name', 'indx', 'rele', 'rang', 'test']

applications = application

class folder_action(aetools.ComponentItem):
    """folder action - An action attached to a folder in the file system """
    want = 'foac'
class _Prop_name(aetools.NProperty):
    """name - the name of the folder action, which is also the name of the folder """
    which = 'pnam'
    want = 'utxt'
class _Prop_path(aetools.NProperty):
    """path - the path to the folder to which the folder action applies """
    which = 'ppth'
    want = '****'
class _Prop_volume(aetools.NProperty):
    """volume - the volume on which the folder action resides """
    which = 'volu'
    want = 'utxt'
#        element 'scpt' as ['name', 'indx', 'rele', 'rang', 'test']

folder_actions = folder_action

class script(aetools.ComponentItem):
    """script - A script invoked by a folder action """
    want = 'scpt'
class _Prop_POSIX_path(aetools.NProperty):
    """POSIX path - the POSIX file system path of the disk """
    which = 'posx'
    want = 'utxt'

scripts = script
application._superclassnames = []
import sys
from . import Disk_Folder_File_Suite
from . import Standard_Suite
from . import Login_Items_Suite
from . import Processes_Suite
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'folder_actions_enabled' : _Prop_folder_actions_enabled,
    'properties' : _Prop_properties,
}
application._privelemdict = {
    'application_process' : Processes_Suite.application_process,
    'desk_accessory_process' : Processes_Suite.desk_accessory_process,
    'disk' : Disk_Folder_File_Suite.disk,
    'document' : Standard_Suite.document,
    'file' : Disk_Folder_File_Suite.file,
    'folder' : Disk_Folder_File_Suite.folder,
    'folder_action' : folder_action,
    'item' : Disk_Folder_File_Suite.item,
    'login_item' : Login_Items_Suite.login_item,
    'process' : Processes_Suite.process,
    'window' : Standard_Suite.window,
}
folder_action._superclassnames = ['item']
folder_action._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'name' : _Prop_name,
    'path' : _Prop_path,
    'properties' : _Prop_properties,
    'volume' : _Prop_volume,
}
folder_action._privelemdict = {
    'script' : script,
}
script._superclassnames = ['item']
script._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'name' : _Prop_name,
    'path' : _Prop_path,
    'properties' : _Prop_properties,
}
script._privelemdict = {
}
_Enum_actn = {
    'items_added' : 'fget',     # items added
    'items_removed' : 'flos',   # items removed
    'window_closed' : 'fclo',   # window closed
    'window_moved' : 'fsiz',    # window moved
    'window_opened' : 'fopn',   # window opened
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'foac' : folder_action,
    'scpt' : script,
}

_propdeclarations = {
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'faen' : _Prop_folder_actions_enabled,
    'pALL' : _Prop_properties,
    'pnam' : _Prop_name,
    'posx' : _Prop_POSIX_path,
    'ppth' : _Prop_path,
    'volu' : _Prop_volume,
}

_compdeclarations = {
}

_enumdeclarations = {
    'actn' : _Enum_actn,
}
