"""Suite Processes Suite: Terms and Events for controlling Processes
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'prcs'

class Processes_Suite_Events:

    pass


class application(aetools.ComponentItem):
    """application - The Processes Suite host program """
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
    """properties - every property of the Processes Suite host program """
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

class application_process(aetools.ComponentItem):
    """application process - A process launched from an application file """
    want = 'pcap'
class _Prop_application_file(aetools.NProperty):
    """application file - a reference to the application file from which this process was launched """
    which = 'appf'
    want = '****'

application_processes = application_process

class desk_accessory_process(aetools.ComponentItem):
    """desk accessory process - A process launched from an desk accessory file """
    want = 'pcda'
class _Prop_desk_accessory_file(aetools.NProperty):
    """desk accessory file - a reference to the desk accessory file from which this process was launched """
    which = 'dafi'
    want = '****'

desk_accessory_processes = desk_accessory_process

class process(aetools.ComponentItem):
    """process - A process running on this computer """
    want = 'prcs'
class _Prop_accepts_high_level_events(aetools.NProperty):
    """accepts high level events - Is the process high-level event aware (accepts open application, open document, print document, and quit)? """
    which = 'isab'
    want = 'bool'
class _Prop_accepts_remote_events(aetools.NProperty):
    """accepts remote events - Does the process accept remote events? """
    which = 'revt'
    want = 'bool'
class _Prop_classic(aetools.NProperty):
    """classic - Is the process running in the Classic environment? """
    which = 'clsc'
    want = 'bool'
class _Prop_creator_type(aetools.NProperty):
    """creator type - the OSType of the creator of the process (the signature) """
    which = 'fcrt'
    want = 'utxt'
class _Prop_file(aetools.NProperty):
    """file - the file from which the process was launched """
    which = 'file'
    want = '****'
class _Prop_file_type(aetools.NProperty):
    """file type - the OSType of the file type of the process """
    which = 'asty'
    want = 'utxt'
class _Prop_frontmost(aetools.NProperty):
    """frontmost - Is the process the frontmost process """
    which = 'pisf'
    want = 'bool'
class _Prop_has_scripting_terminology(aetools.NProperty):
    """has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
    which = 'hscr'
    want = 'bool'
class _Prop_name(aetools.NProperty):
    """name - the name of the process """
    which = 'pnam'
    want = 'utxt'
class _Prop_partition_space_used(aetools.NProperty):
    """partition space used - the number of bytes currently used in the process' partition """
    which = 'pusd'
    want = 'magn'
class _Prop_total_partition_size(aetools.NProperty):
    """total partition size - the size of the partition with which the process was launched """
    which = 'appt'
    want = 'magn'
class _Prop_visible(aetools.NProperty):
    """visible - Is the process' layer visible? """
    which = 'pvis'
    want = 'bool'

processes = process
application._superclassnames = []
import Disk_Folder_File_Suite
import Standard_Suite
import Folder_Actions_Suite
import Login_Items_Suite
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'folder_actions_enabled' : _Prop_folder_actions_enabled,
    'properties' : _Prop_properties,
}
application._privelemdict = {
    'application_process' : application_process,
    'desk_accessory_process' : desk_accessory_process,
    'disk' : Disk_Folder_File_Suite.disk,
    'document' : Standard_Suite.document,
    'file' : Disk_Folder_File_Suite.file,
    'folder' : Disk_Folder_File_Suite.folder,
    'folder_action' : Folder_Actions_Suite.folder_action,
    'item' : Disk_Folder_File_Suite.item,
    'login_item' : Login_Items_Suite.login_item,
    'process' : process,
    'window' : Standard_Suite.window,
}
application_process._superclassnames = ['process']
application_process._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'application_file' : _Prop_application_file,
}
application_process._privelemdict = {
}
desk_accessory_process._superclassnames = ['process']
desk_accessory_process._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'desk_accessory_file' : _Prop_desk_accessory_file,
}
desk_accessory_process._privelemdict = {
}
process._superclassnames = ['item']
process._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'accepts_high_level_events' : _Prop_accepts_high_level_events,
    'accepts_remote_events' : _Prop_accepts_remote_events,
    'classic' : _Prop_classic,
    'creator_type' : _Prop_creator_type,
    'file' : _Prop_file,
    'file_type' : _Prop_file_type,
    'frontmost' : _Prop_frontmost,
    'has_scripting_terminology' : _Prop_has_scripting_terminology,
    'name' : _Prop_name,
    'partition_space_used' : _Prop_partition_space_used,
    'properties' : _Prop_properties,
    'total_partition_size' : _Prop_total_partition_size,
    'visible' : _Prop_visible,
}
process._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'pcap' : application_process,
    'pcda' : desk_accessory_process,
    'prcs' : process,
}

_propdeclarations = {
    'appf' : _Prop_application_file,
    'appt' : _Prop_total_partition_size,
    'asty' : _Prop_file_type,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'clsc' : _Prop_classic,
    'dafi' : _Prop_desk_accessory_file,
    'faen' : _Prop_folder_actions_enabled,
    'fcrt' : _Prop_creator_type,
    'file' : _Prop_file,
    'hscr' : _Prop_has_scripting_terminology,
    'isab' : _Prop_accepts_high_level_events,
    'pALL' : _Prop_properties,
    'pisf' : _Prop_frontmost,
    'pnam' : _Prop_name,
    'pusd' : _Prop_partition_space_used,
    'pvis' : _Prop_visible,
    'revt' : _Prop_accepts_remote_events,
}

_compdeclarations = {
}

_enumdeclarations = {
}
