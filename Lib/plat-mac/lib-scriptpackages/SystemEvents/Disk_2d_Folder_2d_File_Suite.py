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
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - All of the properties of the superclass. """
	which = 'c@#^'
	want = 'cobj'
class properties(aetools.NProperty):
	"""properties - every property of the alias """
	which = 'pALL'
	want = '****'
class version(aetools.NProperty):
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
class capacity(aetools.NProperty):
	"""capacity - the total number of bytes (free or used) on the disk """
	which = 'capa'
	want = 'magn'
class ejectable(aetools.NProperty):
	"""ejectable - Can the media be ejected (floppies, CD's, and so on)? """
	which = 'isej'
	want = 'bool'
class format(aetools.NProperty):
	"""format - the file system format of this disk """
	which = 'dfmt'
	want = 'edfm'
class free_space(aetools.NProperty):
	"""free space - the number of free bytes left on the disk """
	which = 'frsp'
	want = 'magn'
class ignore_privileges(aetools.NProperty):
	"""ignore privileges - Ignore permissions on this disk? """
	which = 'igpr'
	want = 'bool'
class local_volume(aetools.NProperty):
	"""local volume - Is the media a local volume (as opposed to a file server)? """
	which = 'isrv'
	want = 'bool'
class startup(aetools.NProperty):
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
class POSIX_path(aetools.NProperty):
	"""POSIX path - the POSIX file system path of the item """
	which = 'posx'
	want = 'utxt'
class busy_status(aetools.NProperty):
	"""busy status - Is the item busy? """
	which = 'busy'
	want = 'bool'
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created """
	which = 'ascd'
	want = '****'
class displayed_name(aetools.NProperty):
	"""displayed name - the name of the item as displayed in the User Interface """
	which = 'dnam'
	want = 'utxt'
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified """
	which = 'asmo'
	want = '****'
class name(aetools.NProperty):
	"""name - the name of the item """
	which = 'pnam'
	want = 'utxt'
class name_extension(aetools.NProperty):
	"""name extension - the extension portion of the name """
	which = 'extn'
	want = 'utxt'
class package_folder(aetools.NProperty):
	"""package folder - Is the item a package? """
	which = 'pkgf'
	want = 'bool'
class path(aetools.NProperty):
	"""path - the file system path of the item """
	which = 'ppth'
	want = 'utxt'
class url(aetools.NProperty):
	"""url - the url of the item """
	which = 'url '
	want = 'utxt'
class visible(aetools.NProperty):
	"""visible - Is the item visible? """
	which = 'pvis'
	want = 'bool'
class volume(aetools.NProperty):
	"""volume - the volume on which the item resides """
	which = 'volu'
	want = 'utxt'

items = item

class file(aetools.ComponentItem):
	"""file - A file in the file system """
	want = 'file'
class creator_type(aetools.NProperty):
	"""creator type - the OSType identifying the application that created the file """
	which = 'fcrt'
	want = 'utxt'
class file_type(aetools.NProperty):
	"""file type - the OSType identifying the type of data contained in the file """
	which = 'asty'
	want = 'utxt'
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the file on disk """
	which = 'phys'
	want = '****'
class product_version(aetools.NProperty):
	"""product version - the version of the product (visible at the top of the "Get Info" window) """
	which = 'ver2'
	want = 'utxt'
class size(aetools.NProperty):
	"""size - the logical size of the file """
	which = 'ptsz'
	want = '****'
class stationery(aetools.NProperty):
	"""stationery - Is the file a stationery pad? """
	which = 'pspd'
	want = 'bool'

files = file
alias._superclassnames = ['item']
alias._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'properties' : properties,
	'version' : version,
}
alias._privelemdict = {
	'alias' : alias,
	'file' : file,
	'folder' : folder,
	'item' : item,
}
disk._superclassnames = ['item']
disk._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'capacity' : capacity,
	'ejectable' : ejectable,
	'format' : format,
	'free_space' : free_space,
	'ignore_privileges' : ignore_privileges,
	'local_volume' : local_volume,
	'properties' : properties,
	'startup' : startup,
}
disk._privelemdict = {
	'alias' : alias,
	'file' : file,
	'folder' : folder,
	'item' : item,
}
folder._superclassnames = ['item']
folder._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'properties' : properties,
}
folder._privelemdict = {
	'alias' : alias,
	'file' : file,
	'folder' : folder,
	'item' : item,
}
item._superclassnames = []
item._privpropdict = {
	'POSIX_path' : POSIX_path,
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'busy_status' : busy_status,
	'creation_date' : creation_date,
	'displayed_name' : displayed_name,
	'modification_date' : modification_date,
	'name' : name,
	'name_extension' : name_extension,
	'package_folder' : package_folder,
	'path' : path,
	'properties' : properties,
	'url' : url,
	'visible' : visible,
	'volume' : volume,
}
item._privelemdict = {
}
file._superclassnames = ['item']
file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'creator_type' : creator_type,
	'file_type' : file_type,
	'physical_size' : physical_size,
	'product_version' : product_version,
	'properties' : properties,
	'size' : size,
	'stationery' : stationery,
	'version' : version,
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
	'ascd' : creation_date,
	'asmo' : modification_date,
	'asty' : file_type,
	'busy' : busy_status,
	'c@#^' : _3c_Inheritance_3e_,
	'capa' : capacity,
	'dfmt' : format,
	'dnam' : displayed_name,
	'extn' : name_extension,
	'fcrt' : creator_type,
	'frsp' : free_space,
	'igpr' : ignore_privileges,
	'isej' : ejectable,
	'isrv' : local_volume,
	'istd' : startup,
	'pALL' : properties,
	'phys' : physical_size,
	'pkgf' : package_folder,
	'pnam' : name,
	'posx' : POSIX_path,
	'ppth' : path,
	'pspd' : stationery,
	'ptsz' : size,
	'pvis' : visible,
	'url ' : url,
	'ver2' : product_version,
	'vers' : version,
	'volu' : volume,
}

_compdeclarations = {
}

_enumdeclarations = {
	'edfm' : _Enum_edfm,
}
