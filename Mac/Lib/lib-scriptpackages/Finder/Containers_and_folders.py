"""Suite Containers and folders: Classes that can contain other file system items
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Containers_and_folders_Events:

	pass


class container(aetools.ComponentItem):
	"""container - An item that contains other items """
	want = 'ctnr'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the item class """
	which = 'c@#^'
	want = 'cobj'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user """
	which = 'sele'
	want = 'obj '
class entire_contents(aetools.NProperty):
	"""entire contents - the entire contents of the container, including the contents of its children """
	which = 'ects'
	want = 'obj '
class expandable(aetools.NProperty):
	"""expandable - Is the container capable of being expanded as an outline? """
	which = 'pexa'
	want = 'bool'
class expanded(aetools.NProperty):
	"""expanded - Is the container opened as an outline? (can only be set for containers viewed as lists) """
	which = 'pexp'
	want = 'bool'
class completely_expanded(aetools.NProperty):
	"""completely expanded - Are the container and all of its children opened as outlines? (can only be set for containers viewed as lists) """
	which = 'pexc'
	want = 'bool'
class icon_size(aetools.NProperty):
	"""icon size - the size of icons displayed in the window. Can be specified as a number, or ... """
	which = 'lvis'
	want = 'long'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']

containers = container

class sharable_container(aetools.ComponentItem):
	"""sharable container - A container that may be shared (disks and folders) """
	want = 'sctr'
class owner(aetools.NProperty):
	"""owner - the user that owns the container (file sharing must be on to use this property) """
	which = 'sown'
	want = 'itxt'
class group(aetools.NProperty):
	"""group - the user or group that has special access to the container (file sharing must be on to use this property) """
	which = 'sgrp'
	want = 'itxt'
class owner_privileges(aetools.NProperty):
	"""owner privileges - the see folders/see files/make changes privileges for the owner (file sharing must be on to use this property) """
	which = 'ownr'
	want = 'priv'
class group_privileges(aetools.NProperty):
	"""group privileges - the see folders/see files/make changes privileges for the group (file sharing must be on to use this property) """
	which = 'gppr'
	want = 'priv'
class guest_privileges(aetools.NProperty):
	"""guest privileges - the see folders/see files/make changes privileges for everyone (file sharing must be on to use this property) """
	which = 'gstp'
	want = 'priv'
class privileges_inherited(aetools.NProperty):
	"""privileges inherited - Are the privileges of the container always the same as the container in which it is stored? (file sharing must be on to use this property) """
	which = 'iprv'
	want = 'bool'
class mounted(aetools.NProperty):
	"""mounted - Is the container mounted on another machine's desktop? (file sharing must be on to use this property) """
	which = 'smou'
	want = 'bool'
class exported(aetools.NProperty):
	"""exported - Is the container a share point or inside a share point, i.e., can the container be shared? (file sharing must be on to use this property) """
	which = 'sexp'
	want = 'bool'
class shared(aetools.NProperty):
	"""shared - Is the container a share point, i.e., is the container currently being shared? (file sharing must be on to use this property) """
	which = 'shar'
	want = 'bool'
class protected(aetools.NProperty):
	"""protected - Is the container protected from being moved, renamed and deleted? (file sharing must be on to use this property) """
	which = 'spro'
	want = 'bool'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']

sharable_containers = sharable_container

class sharing_privileges(aetools.ComponentItem):
	"""sharing privileges - A set of sharing properties (used in sharable containers) """
	want = 'priv'
class see_folders(aetools.NProperty):
	"""see folders - Can folders be seen? """
	which = 'prvs'
	want = 'bool'
class see_files(aetools.NProperty):
	"""see files - Can files be seen? """
	which = 'prvr'
	want = 'bool'
class make_changes(aetools.NProperty):
	"""make changes - Can changes be made? """
	which = 'prvw'
	want = 'bool'

class disk(aetools.ComponentItem):
	"""disk - A disk """
	want = 'cdis'
class capacity(aetools.NProperty):
	"""capacity - the total number of bytes (free or used) on the disk """
	which = 'capa'
	want = 'long'
class free_space(aetools.NProperty):
	"""free space - the number of free bytes left on the disk """
	which = 'frsp'
	want = 'long'
class ejectable(aetools.NProperty):
	"""ejectable - Can the media be ejected (floppies, CD's, and so on)? """
	which = 'isej'
	want = 'bool'
class local_volume(aetools.NProperty):
	"""local volume - Is the media a local volume (as opposed to a file server)? """
	which = 'isrv'
	want = 'bool'
class startup(aetools.NProperty):
	"""startup - Is this disk the boot disk? """
	which = 'istd'
	want = 'bool'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']

disks = disk

class folder(aetools.ComponentItem):
	"""folder - A folder """
	want = 'cfol'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']

folders = folder

class desktop_2d_object(aetools.ComponentItem):
	"""desktop-object - Desktop-object is the class of the –desktop” object """
	want = 'cdsk'
class startup_disk(aetools.NProperty):
	"""startup disk - the startup disk """
	which = 'sdsk'
	want = 'cdis'
class trash(aetools.NProperty):
	"""trash - the trash """
	which = 'trsh'
	want = 'ctrs'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cdis' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']

class trash_2d_object(aetools.ComponentItem):
	"""trash-object - Trash-object is the class of the –trash” object """
	want = 'ctrs'
class warns_before_emptying(aetools.NProperty):
	"""warns before emptying - Display a dialog when emptying the trash? """
	which = 'warn'
	want = 'bool'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']
import Earlier_terms
import Files_and_suitcases
container._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'selection' : selection,
	'entire_contents' : entire_contents,
	'expandable' : expandable,
	'expanded' : expanded,
	'completely_expanded' : completely_expanded,
	'icon_size' : icon_size,
	'icon_size' : icon_size,
}
container._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}
sharable_container._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'owner' : owner,
	'group' : group,
	'owner_privileges' : owner_privileges,
	'group_privileges' : group_privileges,
	'guest_privileges' : guest_privileges,
	'privileges_inherited' : privileges_inherited,
	'mounted' : mounted,
	'exported' : exported,
	'shared' : shared,
	'protected' : protected,
}
sharable_container._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}
sharing_privileges._propdict = {
	'see_folders' : see_folders,
	'see_files' : see_files,
	'make_changes' : make_changes,
}
sharing_privileges._elemdict = {
}
disk._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'capacity' : capacity,
	'free_space' : free_space,
	'ejectable' : ejectable,
	'local_volume' : local_volume,
	'startup' : startup,
}
disk._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}
folder._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
folder._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}
desktop_2d_object._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'startup_disk' : startup_disk,
	'trash' : trash,
}
desktop_2d_object._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'disk' : disk,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}
trash_2d_object._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'warns_before_emptying' : warns_before_emptying,
}
trash_2d_object._elemdict = {
	'item' : Earlier_terms.item,
	'container' : container,
	'sharable_container' : sharable_container,
	'folder' : folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'priv' : sharing_privileges,
	'cfol' : folder,
	'cdis' : disk,
	'sctr' : sharable_container,
	'ctnr' : container,
	'cdsk' : desktop_2d_object,
	'ctrs' : trash_2d_object,
}

_propdeclarations = {
	'ownr' : owner_privileges,
	'spro' : protected,
	'frsp' : free_space,
	'sgrp' : group,
	'pexc' : completely_expanded,
	'sele' : selection,
	'smou' : mounted,
	'pexa' : expandable,
	'istd' : startup,
	'sdsk' : startup_disk,
	'gppr' : group_privileges,
	'shar' : shared,
	'capa' : capacity,
	'isej' : ejectable,
	'gstp' : guest_privileges,
	'warn' : warns_before_emptying,
	'sown' : owner,
	'c@#^' : _3c_Inheritance_3e_,
	'sexp' : exported,
	'isrv' : local_volume,
	'iprv' : privileges_inherited,
	'lvis' : icon_size,
	'trsh' : trash,
	'prvs' : see_folders,
	'prvr' : see_files,
	'prvw' : make_changes,
	'pexp' : expanded,
	'ects' : entire_contents,
}

_compdeclarations = {
}

_enumdeclarations = {
}
