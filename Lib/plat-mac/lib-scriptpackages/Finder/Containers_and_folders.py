"""Suite Containers and folders: Classes that can contain other file system items
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Containers_and_folders_Events:

    pass


class disk(aetools.ComponentItem):
    """disk - A disk """
    want = 'cdis'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - inherits some of its properties from the container class """
    which = 'c@#^'
    want = 'ctnr'
class _Prop_capacity(aetools.NProperty):
    """capacity - the total number of bytes (free or used) on the disk """
    which = 'capa'
    want = 'comp'
class _Prop_ejectable(aetools.NProperty):
    """ejectable - Can the media be ejected (floppies, CD's, and so on)? """
    which = 'isej'
    want = 'bool'
class _Prop_format(aetools.NProperty):
    """format - the filesystem format of this disk """
    which = 'dfmt'
    want = 'edfm'
class _Prop_free_space(aetools.NProperty):
    """free space - the number of free bytes left on the disk """
    which = 'frsp'
    want = 'comp'
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
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']

disks = disk

class desktop_2d_object(aetools.ComponentItem):
    """desktop-object - Desktop-object is the class of the \xd2desktop\xd3 object """
    want = 'cdsk'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cdis' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']

class folder(aetools.ComponentItem):
    """folder - A folder """
    want = 'cfol'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']

folders = folder

class container(aetools.ComponentItem):
    """container - An item that contains other items """
    want = 'ctnr'
class _Prop_completely_expanded(aetools.NProperty):
    """completely expanded - (NOT AVAILABLE YET) Are the container and all of its children opened as outlines? (can only be set for containers viewed as lists) """
    which = 'pexc'
    want = 'bool'
class _Prop_container_window(aetools.NProperty):
    """container window - the container window for this folder """
    which = 'cwnd'
    want = 'obj '
class _Prop_entire_contents(aetools.NProperty):
    """entire contents - the entire contents of the container, including the contents of its children """
    which = 'ects'
    want = 'obj '
class _Prop_expandable(aetools.NProperty):
    """expandable - (NOT AVAILABLE YET) Is the container capable of being expanded as an outline? """
    which = 'pexa'
    want = 'bool'
class _Prop_expanded(aetools.NProperty):
    """expanded - (NOT AVAILABLE YET) Is the container opened as an outline? (can only be set for containers viewed as lists) """
    which = 'pexp'
    want = 'bool'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']

containers = container

class trash_2d_object(aetools.ComponentItem):
    """trash-object - Trash-object is the class of the \xd2trash\xd3 object """
    want = 'ctrs'
class _Prop_warns_before_emptying(aetools.NProperty):
    """warns before emptying - Display a dialog when emptying the trash? """
    which = 'warn'
    want = 'bool'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
disk._superclassnames = ['container']
import Files
import Finder_items
disk._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'capacity' : _Prop_capacity,
    'ejectable' : _Prop_ejectable,
    'format' : _Prop_format,
    'free_space' : _Prop_free_space,
    'ignore_privileges' : _Prop_ignore_privileges,
    'local_volume' : _Prop_local_volume,
    'startup' : _Prop_startup,
}
disk._privelemdict = {
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'container' : container,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
}
desktop_2d_object._superclassnames = ['container']
desktop_2d_object._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
desktop_2d_object._privelemdict = {
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'container' : container,
    'disk' : disk,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
}
folder._superclassnames = ['container']
folder._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
folder._privelemdict = {
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'container' : container,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
}
container._superclassnames = ['item']
container._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'completely_expanded' : _Prop_completely_expanded,
    'container_window' : _Prop_container_window,
    'entire_contents' : _Prop_entire_contents,
    'expandable' : _Prop_expandable,
    'expanded' : _Prop_expanded,
}
container._privelemdict = {
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'container' : container,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
}
trash_2d_object._superclassnames = ['container']
trash_2d_object._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'warns_before_emptying' : _Prop_warns_before_emptying,
}
trash_2d_object._privelemdict = {
    'alias_file' : Files.alias_file,
    'application_file' : Files.application_file,
    'clipping' : Files.clipping,
    'container' : container,
    'document_file' : Files.document_file,
    'file' : Files.file,
    'folder' : folder,
    'internet_location_file' : Files.internet_location_file,
    'item' : Finder_items.item,
    'package' : Files.package,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cdis' : disk,
    'cdsk' : desktop_2d_object,
    'cfol' : folder,
    'ctnr' : container,
    'ctrs' : trash_2d_object,
}

_propdeclarations = {
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'capa' : _Prop_capacity,
    'cwnd' : _Prop_container_window,
    'dfmt' : _Prop_format,
    'ects' : _Prop_entire_contents,
    'frsp' : _Prop_free_space,
    'igpr' : _Prop_ignore_privileges,
    'isej' : _Prop_ejectable,
    'isrv' : _Prop_local_volume,
    'istd' : _Prop_startup,
    'pexa' : _Prop_expandable,
    'pexc' : _Prop_completely_expanded,
    'pexp' : _Prop_expanded,
    'warn' : _Prop_warns_before_emptying,
}

_compdeclarations = {
}

_enumdeclarations = {
}
