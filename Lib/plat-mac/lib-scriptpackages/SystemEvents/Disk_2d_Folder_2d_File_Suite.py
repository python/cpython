"""Suite Disk-Folder-File Suite: Terms and Events for controlling Disks, Folders, and Files
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'cdis'

class Disk_2d_Folder_2d_File_Suite_Events:

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


class alias(aetools.ComponentItem):
    """alias - An alias in the file system """
    want = 'alis'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - All of the properties of the superclass. """
    which = 'c@#^'
    want = 'cobj'
class _Prop_properties(aetools.NProperty):
    """properties - every property of the alias """
    which = 'pALL'
    want = '****'
class _Prop_version(aetools.NProperty):
    """version - the version of the application bundle referenced by the alias (visible at the bottom of the "Get Info" window) """
    which = 'vers'
    want = 'utxt'
#        element 'alis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

aliases = alias

class disk(aetools.ComponentItem):
    """disk - A disk in the file system """
    want = 'cdis'
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
    """local volume - Is the media a local volume (as opposed to a file server)? """
    which = 'isrv'
    want = 'bool'
class _Prop_startup(aetools.NProperty):
    """startup - Is this disk the boot disk? """
    which = 'istd'
    want = 'bool'
#        element 'alis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

disks = disk

class folder(aetools.ComponentItem):
    """folder - A folder in the file system """
    want = 'cfol'
#        element 'alis' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cfol' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'cobj' as ['name', 'indx', 'rele', 'rang', 'test']
#        element 'file' as ['name', 'indx', 'rele', 'rang', 'test']

folders = folder

class item(aetools.ComponentItem):
    """item - An item in the file system """
    want = 'cobj'
class _Prop_POSIX_path(aetools.NProperty):
    """POSIX path - the POSIX file system path of the item """
    which = 'posx'
    want = 'utxt'
class _Prop_busy_status(aetools.NProperty):
    """busy status - Is the item busy? """
    which = 'busy'
    want = 'bool'
class _Prop_creation_date(aetools.NProperty):
    """creation date - the date on which the item was created """
    which = 'ascd'
    want = '****'
class _Prop_displayed_name(aetools.NProperty):
    """displayed name - the name of the item as displayed in the User Interface """
    which = 'dnam'
    want = 'utxt'
class _Prop_modification_date(aetools.NProperty):
    """modification date - the date on which the item was last modified """
    which = 'asmo'
    want = '****'
class _Prop_name(aetools.NProperty):
    """name - the name of the item """
    which = 'pnam'
    want = 'utxt'
class _Prop_name_extension(aetools.NProperty):
    """name extension - the extension portion of the name """
    which = 'extn'
    want = 'utxt'
class _Prop_package_folder(aetools.NProperty):
    """package folder - Is the item a package? """
    which = 'pkgf'
    want = 'bool'
class _Prop_path(aetools.NProperty):
    """path - the file system path of the item """
    which = 'ppth'
    want = 'utxt'
class _Prop_url(aetools.NProperty):
    """url - the url of the item """
    which = 'url '
    want = 'utxt'
class _Prop_visible(aetools.NProperty):
    """visible - Is the item visible? """
    which = 'pvis'
    want = 'bool'
class _Prop_volume(aetools.NProperty):
    """volume - the volume on which the item resides """
    which = 'volu'
    want = 'utxt'

items = item

class file(aetools.ComponentItem):
    """file - A file in the file system """
    want = 'file'
class _Prop_creator_type(aetools.NProperty):
    """creator type - the OSType identifying the application that created the file """
    which = 'fcrt'
    want = 'utxt'
class _Prop_file_type(aetools.NProperty):
    """file type - the OSType identifying the type of data contained in the file """
    which = 'asty'
    want = 'utxt'
class _Prop_physical_size(aetools.NProperty):
    """physical size - the actual space used by the file on disk """
    which = 'phys'
    want = '****'
class _Prop_product_version(aetools.NProperty):
    """product version - the version of the product (visible at the top of the "Get Info" window) """
    which = 'ver2'
    want = 'utxt'
class _Prop_size(aetools.NProperty):
    """size - the logical size of the file """
    which = 'ptsz'
    want = '****'
class _Prop_stationery(aetools.NProperty):
    """stationery - Is the file a stationery pad? """
    which = 'pspd'
    want = 'bool'

files = file
alias._superclassnames = ['item']
alias._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'properties' : _Prop_properties,
    'version' : _Prop_version,
}
alias._privelemdict = {
    'alias' : alias,
    'file' : file,
    'folder' : folder,
    'item' : item,
}
disk._superclassnames = ['item']
disk._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'capacity' : _Prop_capacity,
    'ejectable' : _Prop_ejectable,
    'format' : _Prop_format,
    'free_space' : _Prop_free_space,
    'ignore_privileges' : _Prop_ignore_privileges,
    'local_volume' : _Prop_local_volume,
    'properties' : _Prop_properties,
    'startup' : _Prop_startup,
}
disk._privelemdict = {
    'alias' : alias,
    'file' : file,
    'folder' : folder,
    'item' : item,
}
folder._superclassnames = ['item']
folder._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'properties' : _Prop_properties,
}
folder._privelemdict = {
    'alias' : alias,
    'file' : file,
    'folder' : folder,
    'item' : item,
}
item._superclassnames = []
item._privpropdict = {
    'POSIX_path' : _Prop_POSIX_path,
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'busy_status' : _Prop_busy_status,
    'creation_date' : _Prop_creation_date,
    'displayed_name' : _Prop_displayed_name,
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
}
file._superclassnames = ['item']
file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'creator_type' : _Prop_creator_type,
    'file_type' : _Prop_file_type,
    'physical_size' : _Prop_physical_size,
    'product_version' : _Prop_product_version,
    'properties' : _Prop_properties,
    'size' : _Prop_size,
    'stationery' : _Prop_stationery,
    'version' : _Prop_version,
}
file._privelemdict = {
}
_Enum_edfm = {
    'MS_2d_DOS_format' : 'dfms',	# MS-DOS format
    'Apple_Photo_format' : 'dfph',	# Apple Photo format
    'ISO_9660_format' : 'df96',	# ISO 9660 format
    'QuickTake_format' : 'dfqt',	# QuickTake format
    'AppleShare_format' : 'dfas',	# AppleShare format
    'High_Sierra_format' : 'dfhs',	# High Sierra format
    'Mac_OS_Extended_format' : 'dfh+',	# Mac OS Extended format
    'UDF_format' : 'dfud',	# UDF format
    'unknown_format' : 'df??',	# unknown format
    'audio_format' : 'dfau',	# audio format
    'Mac_OS_format' : 'dfhf',	# Mac OS format
    'UFS_format' : 'dfuf',	# UFS format
    'NFS_format' : 'dfnf',	# NFS format
    'ProDOS_format' : 'dfpr',	# ProDOS format
    'WebDAV_format' : 'dfwd',	# WebDAV format
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'alis' : alias,
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
    'dnam' : _Prop_displayed_name,
    'extn' : _Prop_name_extension,
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
    'pvis' : _Prop_visible,
    'url ' : _Prop_url,
    'ver2' : _Prop_product_version,
    'vers' : _Prop_version,
    'volu' : _Prop_volume,
}

_compdeclarations = {
}

_enumdeclarations = {
    'edfm' : _Enum_edfm,
}
