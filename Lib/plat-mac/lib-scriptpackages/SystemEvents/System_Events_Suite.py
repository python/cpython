"""Suite System Events Suite: Terms and Events for controlling the System Events application
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'sevs'

class System_Events_Suite_Events:

    pass


class application(aetools.ComponentItem):
    """application - The System Events application """
    want = 'capp'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - All of the properties of the superclass. """
    which = 'c@#^'
    want = 'capp'
class _Prop_folder_actions_enabled(aetools.NProperty):
    """folder actions enabled - Are Folder Actions currently being processed? """
    which = 'faen'
    want = 'bool'
class _Prop_properties(aetools.NProperty):
    """properties - every property of the System Events application """
    which = 'pALL'
    want = '****'
class _Prop_system_wide_UI_element(aetools.NProperty):
    """system wide UI element - the UI element for the entire system """
    which = 'swui'
    want = 'uiel'
#        element 'alis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cdis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cwin' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'docu' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'foac' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'logi' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'pcap' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'pcda' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'prcs' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'uiel' as ['name', 'indx', 'rele', 'rang', 'test']

applications = application
application._superclassnames = []
import Disk_2d_Folder_2d_File_Suite
import Standard_Suite
import Folder_Actions_Suite
import Login_Items_Suite
import Processes_Suite
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'folder_actions_enabled' : _Prop_folder_actions_enabled,
    'properties' : _Prop_properties,
    'system_wide_UI_element' : _Prop_system_wide_UI_element,
}
application._privelemdict = {
    'UI_element' : Processes_Suite.UI_element,
    'alias' : Disk_2d_Folder_2d_File_Suite.alias,
    'application_process' : Processes_Suite.application_process,
    'desk_accessory_process' : Processes_Suite.desk_accessory_process,
    'disk' : Disk_2d_Folder_2d_File_Suite.disk,
    'document' : Standard_Suite.document,
    'file' : Disk_2d_Folder_2d_File_Suite.file,
    'folder' : Disk_2d_Folder_2d_File_Suite.folder,
    'folder_action' : Folder_Actions_Suite.folder_action,
    'item' : Disk_2d_Folder_2d_File_Suite.item,
    'login_item' : Login_Items_Suite.login_item,
    'process' : Processes_Suite.process,
    'window' : Standard_Suite.window,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
}

_propdeclarations = {
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'faen' : _Prop_folder_actions_enabled,
    'pALL' : _Prop_properties,
    'swui' : _Prop_system_wide_UI_element,
}

_compdeclarations = {
}

_enumdeclarations = {
}
