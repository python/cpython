"""Suite Disk-Folder-File Suite: Terms and Events for controlling Disks, Folders, and Files
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'cdis'

class Disk_Folder_File_Suite_Events:

    _argmap_move = {
        'to' : 'insh',
    }

    def move(self, _object, _attributes={}, **_arguments):
        """move: Move disk item(s) to a new location.
        Required argument: the object for the command
        Keyword argument to: The new location for the disk item(s).
        Keyword argument _attributes: AppleEvent attribute dictionary
        Returns: the reply for the command
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


class application(aetools.ComponentItem):
    """application - The Disk-Folder-File Suite host program """
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
    """properties - every property of the Disk-Folder-File Suite host program """
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

class disk(aetools.ComponentItem):
    """disk - A disk in the file system """
    want = 'cdis'
class _Prop_POSIX_path(aetools.NProperty):
    """POSIX path - the POSIX file system path of the disk """
    which = 'posx'
    want = 'utxt'
class _Prop_capacity(aetools.NProperty):
    """capacity - the total number of bytes (free or used) on the disk """
    which = 'capa'
    want = 'magn'
class _Prop_ejectable(aetools.NProperty):
    """ejectable - Can the media be ejected (floppies, CD's, and so on)? """
    which = 'isej'
    want = 'bool'
class _Prop_format(aetools.NProperty):
    """format - the file system format of this disk """
    which = 'dfmt'
    want = 'edfm'
class _Prop_free_space(aetools.NProperty):
    """free space - the number of free bytes left on the disk """
    which = 'frsp'
    want = 'magn'
class _Prop_ignore_privileges(aetools.NProperty):
    """ignore privileges - Ignore permissions on this disk? """
    which = 'igpr'
    want = 'bool'
class _Prop_local_volume(aetools.NProperty):
    """local volume - Is the media a local volume (as opposed to a file server? """
    which = 'isrv'
    want = 'bool'
class _Prop_name(aetools.NProperty):
    """name - the name of the disk """
    which = 'pnam'
    want = 'utxt'
class _Prop_path(aetools.NProperty):
    """path - the file system path of the disk """
    which = 'ppth'
    want = 'utxt'
class _Prop_startup(aetools.NProperty):
    """startup - Is this disk the boot disk? """
    which = 'istd'
    want = 'bool'
class _Prop_volume(aetools.NProperty):
    """volume - the volume on which the folder resides """
    which = 'volu'
    want = 'utxt'
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

disks = disk

class folder(aetools.ComponentItem):
    """folder - A folder in the file system """
    want = 'cfol'
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

folders = folder

class item(aetools.ComponentItem):
    """item - An item in the file system """
    want = 'cobj'
class _Prop_busy_status(aetools.NProperty):
    """busy status - Is the item busy? """
    which = 'busy'
    want = 'bool'
class _Prop_creation_date(aetools.NProperty):
    """creation date - the date on which the item was created """
    which = 'ascd'
    want = '****'
class _Prop_modification_date(aetools.NProperty):
    """modification date - the date on which the item was last modified """
    which = 'asmo'
    want = '****'
class _Prop_name_extension(aetools.NProperty):
    """name extension - the extension portion of the name """
    which = 'extn'
    want = 'utxt'
class _Prop_package_folder(aetools.NProperty):
    """package folder - Is the item a package? """
    which = 'pkgf'
    want = 'bool'
class _Prop_url(aetools.NProperty):
    """url - the url of the item """
    which = 'url '
    want = 'utxt'
class _Prop_visible(aetools.NProperty):
    """visible - Is the item visible? """
    which = 'visi'
    want = 'bool'
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

items = item

class file(aetools.ComponentItem):
    """file - A file in the file system """
    want = 'file'
class _Prop_creator_type(aetools.NProperty):
    """creator type - the OSType identifying the application that created the item """
    which = 'fcrt'
    want = 'utxt'
class _Prop_file_type(aetools.NProperty):
    """file type - the OSType identifying the type of data contained in the item """
    which = 'asty'
    want = 'utxt'
class _Prop_physical_size(aetools.NProperty):
    """physical size - the actual space used by the file on disk """
    which = 'phys'
    want = 'magn'
class _Prop_product_version(aetools.NProperty):
    """product version - the version of the product (visible at the top of the ?et Info?window) """
    which = 'ver2'
    want = 'utxt'
class _Prop_size(aetools.NProperty):
    """size - the logical size of the file """
    which = 'ptsz'
    want = 'magn'
class _Prop_stationery(aetools.NProperty):
    """stationery - Is the file a stationery pad? """
    which = 'pspd'
    want = 'bool'
class _Prop_version(aetools.NProperty):
    """version - the version of the file (visible at the bottom of the ?et Info?window) """
    which = 'vers'
    want = 'utxt'
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

files = file
application._superclassnames = []
import Standard_Suite
import Folder_Actions_Suite
import Login_Items_Suite
import Processes_Suite
application._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'folder_actions_enabled' : _Prop_folder_actions_enabled,
    'properties' : _Prop_properties,
}
application._privelemdict = {
    'application_process' : Processes_Suite.application_process,
    'desk_accessory_process' : Processes_Suite.desk_accessory_process,
    'disk' : disk,
    'document' : Standard_Suite.document,
    'file' : file,
    'folder' : folder,
    'folder_action' : Folder_Actions_Suite.folder_action,
    'item' : item,
    'login_item' : Login_Items_Suite.login_item,
    'process' : Processes_Suite.process,
    'window' : Standard_Suite.window,
}
disk._superclassnames = ['item']
disk._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'capacity' : _Prop_capacity,
    'ejectable' : _Prop_ejectable,
    'format' : _Prop_format,
    'free_space' : _Prop_free_space,
    'ignore_privileges' : _Prop_ignore_privileges,
    'local_volume' : _Prop_local_volume,
    'name' : _Prop_name,
    'path' : _Prop_path,
    'properties' : _Prop_properties,
    'startup' : _Prop_startup,
    'volume' : _Prop_volume,
}
disk._privelemdict = {
    'file' : file,
    'folder' : folder,
    'item' : item,
}
folder._superclassnames = ['item']
folder._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'name' : _Prop_name,
    'path' : _Prop_path,
    'properties' : _Prop_properties,
    'volume' : _Prop_volume,
}
folder._privelemdict = {
    'file' : file,
    'file' : file,
    'folder' : folder,
    'folder' : folder,
    'item' : item,
    'item' : item,
}
item._superclassnames = []
item._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'busy_status' : _Prop_busy_status,
    'creation_date' : _Prop_creation_date,
    'modification_date' : _Prop_modification_date,
    'name' : _Prop_name,
    'name_extension' : _Prop_name_extension,
    'package_folder' : _Prop_package_folder,
    'path' : _Prop_path,
    'properties' : _Prop_properties,
    'url' : _Prop_url,
    'visible' : _Prop_visible,
    'volume' : _Prop_volume,
}
item._privelemdict = {
    'file' : file,
    'folder' : folder,
    'item' : item,
}
file._superclassnames = ['item']
file._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'creator_type' : _Prop_creator_type,
    'file_type' : _Prop_file_type,
    'name' : _Prop_name,
    'path' : _Prop_path,
    'physical_size' : _Prop_physical_size,
    'product_version' : _Prop_product_version,
    'properties' : _Prop_properties,
    'size' : _Prop_size,
    'stationery' : _Prop_stationery,
    'version' : _Prop_version,
    'volume' : _Prop_volume,
}
file._privelemdict = {
    'file' : file,
    'folder' : folder,
    'item' : item,
}
_Enum_edfm = {
    'MS_2d_DOS_format' : 'dfms',        # MS-DOS format
    'Apple_Photo_format' : 'dfph',      # Apple Photo format
    'ISO_9660_format' : 'df96', # ISO 9660 format
    'QuickTake_format' : 'dfqt',        # QuickTake format
    'AppleShare_format' : 'dfas',       # AppleShare format
    'High_Sierra_format' : 'dfhs',      # High Sierra format
    'Mac_OS_Extended_format' : 'dfh+',  # Mac OS Extended format
    'UDF_format' : 'dfud',      # UDF format
    'unknown_format' : 'df??',  # unknown format
    'audio_format' : 'dfau',    # audio format
    'Mac_OS_format' : 'dfhf',   # Mac OS format
    'UFS_format' : 'dfuf',      # UFS format
    'NFS_format' : 'dfnf',      # NFS format
    'ProDOS_format' : 'dfpr',   # ProDOS format
    'WebDAV_format' : 'dfwd',   # WebDAV format
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'capp' : application,
    'cdis' : disk,
    'cfol' : folder,
    'cobj' : item,
    'file' : file,
}

_propdeclarations = {
    'ascd' : _Prop_creation_date,
    'asmo' : _Prop_modification_date,
    'asty' : _Prop_file_type,
    'busy' : _Prop_busy_status,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'capa' : _Prop_capacity,
    'dfmt' : _Prop_format,
    'extn' : _Prop_name_extension,
    'faen' : _Prop_folder_actions_enabled,
    'fcrt' : _Prop_creator_type,
    'frsp' : _Prop_free_space,
    'igpr' : _Prop_ignore_privileges,
    'isej' : _Prop_ejectable,
    'isrv' : _Prop_local_volume,
    'istd' : _Prop_startup,
    'pALL' : _Prop_properties,
    'phys' : _Prop_physical_size,
    'pkgf' : _Prop_package_folder,
    'pnam' : _Prop_name,
    'posx' : _Prop_POSIX_path,
    'ppth' : _Prop_path,
    'pspd' : _Prop_stationery,
    'ptsz' : _Prop_size,
    'url ' : _Prop_url,
    'ver2' : _Prop_product_version,
    'vers' : _Prop_version,
    'visi' : _Prop_visible,
    'volu' : _Prop_volume,
}

_compdeclarations = {
}

_enumdeclarations = {
    'edfm' : _Enum_edfm,
}
