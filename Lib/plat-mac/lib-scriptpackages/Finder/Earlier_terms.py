"""Suite Earlier terms: Terms that have been renamed
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpnm'

from StdSuites.Type_Names_Suite import *
class Earlier_terms_Events(Type_Names_Suite_Events):

	pass


class accessory_process(aetools.ComponentItem):
	"""accessory process - A process launched from a desk accessory file """
	want = 'pcda'

accessory_processes = accessory_process

class accessory_suitcase(aetools.ComponentItem):
	"""accessory suitcase - A desk accessory suitcase """
	want = 'dsut'

accessory_suitcases = accessory_suitcase

class application(aetools.ComponentItem):
	"""application - The Finder """
	want = 'capp'
class Finder_preferences(aetools.NProperty):
	"""Finder preferences - Various preferences that apply to the Finder as a whole """
	which = 'pfrp'
	want = 'obj '
class about_this_computer(aetools.NProperty):
	"""about this computer - the \xd2About this Computer\xd3 dialog and the list of running processes displayed in it """
	which = 'abbx'
	want = 'obj '
class clipboard(aetools.NProperty):
	"""clipboard - the Finder\xd5s clipboard window """
	which = 'pcli'
	want = 'obj '
class desktop(aetools.NProperty):
	"""desktop - the desktop """
	which = 'desk'
	want = 'cdsk'
class file_sharing(aetools.NProperty):
	"""file sharing - Is file sharing on? """
	which = 'fshr'
	want = 'bool'
class frontmost(aetools.NProperty):
	"""frontmost - Is the Finder the frontmost process? """
	which = 'pisf'
	want = 'bool'
class insertion_location(aetools.NProperty):
	"""insertion location - the container in which a new folder would appear if \xd2New Folder\xd3 was selected """
	which = 'pins'
	want = 'obj '
class largest_free_block(aetools.NProperty):
	"""largest free block - the largest free block of process memory available to launch an application """
	which = 'mfre'
	want = 'long'
class name(aetools.NProperty):
	"""name - the Finder\xd5s name """
	which = 'pnam'
	want = 'itxt'
class product_version(aetools.NProperty):
	"""product version - the version of the System software running on this computer """
	which = 'ver2'
	want = 'itxt'
class properties(aetools.NProperty):
	"""properties - property that allows getting and setting of multiple properties """
	which = 'qpro'
	want = 'reco'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user """
	which = 'sele'
	want = 'obj '
class sharing_starting_up(aetools.NProperty):
	"""sharing starting up - Is file sharing in the process of starting up? """
	which = 'fsup'
	want = 'bool'
class version(aetools.NProperty):
	"""version - the version of the Finder """
	which = 'vers'
	want = 'itxt'
class visible(aetools.NProperty):
	"""visible - Is the Finder\xd5s layer visible? """
	which = 'pvis'
	want = 'bool'

class application_file(aetools.ComponentItem):
	"""application file - An application's file on disk """
	want = 'appf'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the file class """
	which = 'c@#^'
	want = 'file'
class minimum_partition_size(aetools.NProperty):
	"""minimum partition size - the smallest memory size with which the application can be launched """
	which = 'mprt'
	want = 'long'
class partition_size(aetools.NProperty):
	"""partition size - the memory size with which the application will be launched """
	which = 'appt'
	want = 'long'
class scriptable(aetools.NProperty):
	"""scriptable - Is the application high-level event aware? """
	which = 'isab'
	want = 'bool'
class suggested_partition_size(aetools.NProperty):
	"""suggested partition size - the memory size with which the developer recommends the application be launched """
	which = 'sprt'
	want = 'long'

class container_window(aetools.ComponentItem):
	"""container window - A window that contains items """
	want = 'cwnd'
class button_view_arrangement(aetools.NProperty):
	"""button view arrangement - the property by which to keep icons arranged within a button view window """
	which = 'barr'
	want = 'earr'
class calculate_folder_sizes(aetools.NProperty):
	"""calculate folder sizes - Are folder sizes calculated and displayed in the window? (does not apply to suitcase windows) """
	which = 'sfsz'
	want = 'bool'
class container(aetools.NProperty):
	"""container - the container from which the window was opened """
	which = 'ctnr'
	want = 'obj '
class has_custom_view_settings(aetools.NProperty):
	"""has custom view settings - Does the folder have custom view settings or is it using the default global settings? """
	which = 'cuss'
	want = 'long'
class item(aetools.NProperty):
	"""item - the item from which the window was opened (always returns something) """
	which = 'cobj'
	want = 'obj '
class previous_list_view(aetools.NProperty):
	"""previous list view - the last non-icon view (by name, by date, etc.) selected for the container (forgotten as soon as the window is closed and only available when the window is open) """
	which = 'svew'
	want = 'enum'
class show_comments(aetools.NProperty):
	"""show comments - Are comments displayed in the window? (does not apply to suitcases) """
	which = 'scom'
	want = 'bool'
class show_creation_date(aetools.NProperty):
	"""show creation date - Are creation dates displayed in the window? """
	which = 'scda'
	want = 'bool'
class show_kind(aetools.NProperty):
	"""show kind - Are document kinds displayed in the window? """
	which = 'sknd'
	want = 'bool'
class show_label(aetools.NProperty):
	"""show label - Are labels displayed in the window? """
	which = 'slbl'
	want = 'bool'
class show_modification_date(aetools.NProperty):
	"""show modification date - Are modification dates displayed in the window? """
	which = 'sdat'
	want = 'bool'
class show_size(aetools.NProperty):
	"""show size - Are file sizes displayed in the window? """
	which = 'ssiz'
	want = 'bool'
class show_version(aetools.NProperty):
	"""show version - Are file versions displayed in the window? (does not apply to suitcase windows) """
	which = 'svrs'
	want = 'bool'
class sort_direction(aetools.NProperty):
	"""sort direction - The direction in which the window is sorted """
	which = 'sord'
	want = 'sodr'
class spatial_view_arrangement(aetools.NProperty):
	"""spatial view arrangement - the property by which to keep icons arranged within a spatial view window """
	which = 'iarr'
	want = 'earr'
class use_relative_dates(aetools.NProperty):
	"""use relative dates - Are relative dates (e.g., today, yesterday) shown in the window? """
	which = 'urdt'
	want = 'bool'
class view(aetools.NProperty):
	"""view - the current view for the window (icon, name, date, etc.) """
	which = 'pvew'
	want = 'long'

class information_window(aetools.ComponentItem):
	"""information window - An information window (opened by \xd2Get Info\xd3) """
	want = 'iwnd'
class comment(aetools.NProperty):
	"""comment - the comment """
	which = 'comt'
	want = 'itxt'
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created """
	which = 'ascd'
	want = 'ldt '
class icon(aetools.NProperty):
	"""icon - the icon bitmap of the item """
	which = 'iimg'
	want = 'ifam'
class locked(aetools.NProperty):
	"""locked - Is the item locked (applies only to file and application information windows)? """
	which = 'aslk'
	want = 'bool'
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified """
	which = 'asmo'
	want = 'ldt '
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the item on disk """
	which = 'phys'
	want = 'long'
class size(aetools.NProperty):
	"""size - the logical size of the item """
	which = 'ptsz'
	want = 'long'
class stationery(aetools.NProperty):
	"""stationery - Is the item a stationery pad? """
	which = 'pspd'
	want = 'bool'
class warn_before_emptying(aetools.NProperty):
	"""warn before emptying - Display a dialog when emptying the trash (only valid for trash info window)? """
	which = 'warn'
	want = 'bool'

class internet_location(aetools.ComponentItem):
	"""internet location - An file containing an internet location """
	want = 'inlf'

internet_locations = internet_location

class item(aetools.ComponentItem):
	"""item - An item """
	want = 'cobj'
class bounds(aetools.NProperty):
	"""bounds - the bounding rectangle of the item (can only be set for an item in a window viewed as icons or buttons) """
	which = 'pbnd'
	want = 'qdrt'
class content_space(aetools.NProperty):
	"""content space - the window that would open if the item was opened """
	which = 'dwnd'
	want = 'obj '
class description(aetools.NProperty):
	"""description - a description of the item """
	which = 'dscr'
	want = 'itxt'
class disk(aetools.NProperty):
	"""disk - the disk on which the item is stored """
	which = 'cdis'
	want = 'obj '
class folder(aetools.NProperty):
	"""folder - the folder in which the item is stored """
	which = 'asdr'
	want = 'obj '
class id(aetools.NProperty):
	"""id - an id that identifies the item """
	which = 'ID  '
	want = 'long'
class index(aetools.NProperty):
	"""index - the index in the front-to-back ordering within its container """
	which = 'pidx'
	want = 'long'
class information_window(aetools.NProperty):
	"""information window - the information window for the item """
	which = 'iwnd'
	want = 'obj '
class kind(aetools.NProperty):
	"""kind - the kind of the item """
	which = 'kind'
	want = 'itxt'
class label_index(aetools.NProperty):
	"""label index - the label of the item """
	which = 'labi'
	want = 'long'
class position(aetools.NProperty):
	"""position - the position of the item within its parent window (can only be set for an item in a window viewed as icons or buttons) """
	which = 'posn'
	want = 'QDpt'
class selected(aetools.NProperty):
	"""selected - Is the item selected? """
	which = 'issl'
	want = 'bool'
class window(aetools.NProperty):
	"""window - the window that would open if the item was opened """
	which = 'cwin'
	want = 'obj '

class preferences(aetools.ComponentItem):
	"""preferences - The Finder Preferences """
	want = 'cprf'
class delay_before_springing(aetools.NProperty):
	"""delay before springing - the delay before springing open a container in ticks (1/60th of a second) (12 is shortest delay, 60 is longest delay) """
	which = 'dela'
	want = 'shor'
class spring_open_folders(aetools.NProperty):
	"""spring open folders - Spring open folders after the specified delay? """
	which = 'sprg'
	want = 'bool'
class use_simple_menus(aetools.NProperty):
	"""use simple menus - Use simplified Finder menus? """
	which = 'usme'
	want = 'bool'
class use_wide_grid(aetools.NProperty):
	"""use wide grid - Space icons on a wide grid? """
	which = 'uswg'
	want = 'bool'

class process(aetools.ComponentItem):
	"""process - A process running on this computer """
	want = 'prcs'
class creator_type(aetools.NProperty):
	"""creator type - the OSType of the creator of the process (the signature) """
	which = 'fcrt'
	want = 'type'
class file(aetools.NProperty):
	"""file - the file from which the process was launched """
	which = 'file'
	want = 'obj '
class file_type(aetools.NProperty):
	"""file type - the OSType of the file type of the process """
	which = 'asty'
	want = 'type'
class partition_space_used(aetools.NProperty):
	"""partition space used - the number of bytes currently used in the process' partition """
	which = 'pusd'
	want = 'long'
class remote_events(aetools.NProperty):
	"""remote events - Does the process accept remote events? """
	which = 'revt'
	want = 'bool'

class sharable_container(aetools.ComponentItem):
	"""sharable container - A container that may be shared (disks and folders) """
	want = 'sctr'
class exported(aetools.NProperty):
	"""exported - Is the container a share point or inside a share point, i.e., can the container be shared? (file sharing must be on to use this property) """
	which = 'sexp'
	want = 'bool'
class group(aetools.NProperty):
	"""group - the user or group that has special access to the container (file sharing must be on to use this property) """
	which = 'sgrp'
	want = 'itxt'
class group_privileges(aetools.NProperty):
	"""group privileges - the see folders/see files/make changes privileges for the group (file sharing must be on to use this property) """
	which = 'gppr'
	want = 'priv'
class guest_privileges(aetools.NProperty):
	"""guest privileges - the see folders/see files/make changes privileges for everyone (file sharing must be on to use this property) """
	which = 'gstp'
	want = 'priv'
class inherited_privileges(aetools.NProperty):
	"""inherited privileges - Are the privileges of the container always the same as the container in which it is stored? (file sharing must be on to use this property) """
	which = 'iprv'
	want = 'bool'
class mounted(aetools.NProperty):
	"""mounted - Is the container mounted on another machine's desktop? (file sharing must be on to use this property) """
	which = 'smou'
	want = 'bool'
class owner(aetools.NProperty):
	"""owner - the user that owns the container (file sharing must be on to use this property) """
	which = 'sown'
	want = 'itxt'
class owner_privileges(aetools.NProperty):
	"""owner privileges - the see folders/see files/make changes privileges for the owner (file sharing must be on to use this property) """
	which = 'ownr'
	want = 'priv'
class protected(aetools.NProperty):
	"""protected - Is the container protected from being moved, renamed and deleted? (file sharing must be on to use this property) """
	which = 'spro'
	want = 'bool'
class shared(aetools.NProperty):
	"""shared - Is the container a share point, i.e., is the container currently being shared? (file sharing must be on to use this property) """
	which = 'shar'
	want = 'bool'

class trash_2d_object(aetools.ComponentItem):
	"""trash-object - Trash-object is the class of the \xd2trash\xd3 object """
	want = 'ctrs'

class window(aetools.ComponentItem):
	"""window - A window """
	want = 'cwin'
class closeable(aetools.NProperty):
	"""closeable - Does the window have a close box? """
	which = 'hclb'
	want = 'bool'
class collapsed(aetools.NProperty):
	"""collapsed - Is the window collapsed (only applies to open non-pop-up windows)? """
	which = 'wshd'
	want = 'bool'
class floating(aetools.NProperty):
	"""floating - Does the window have a title bar? """
	which = 'isfl'
	want = 'bool'
class modal(aetools.NProperty):
	"""modal - Is the window modal? """
	which = 'pmod'
	want = 'bool'
class popup(aetools.NProperty):
	"""popup - Is the window is a pop-up window? (only applies to open container windows in the Finder and can only be set when the Finder is the front application) """
	which = 'drwr'
	want = 'bool'
class pulled_open(aetools.NProperty):
	"""pulled open - Is the window pulled open (only applies to pop-up windows and can only be set when the Finder is the front application)? """
	which = 'pull'
	want = 'bool'
class resizable(aetools.NProperty):
	"""resizable - Is the window resizable? """
	which = 'prsz'
	want = 'bool'
class titled(aetools.NProperty):
	"""titled - Does the window have a title bar? """
	which = 'ptit'
	want = 'bool'
class zoomable(aetools.NProperty):
	"""zoomable - Is the window zoomable? """
	which = 'iszm'
	want = 'bool'
class zoomed(aetools.NProperty):
	"""zoomed - Is the window zoomed? """
	which = 'pzum'
	want = 'bool'
class zoomed_full_size(aetools.NProperty):
	"""zoomed full size - Is the window zoomed to the full size of the screen? (can only be set, not read, and only applies to open non-pop-up windows) """
	which = 'zumf'
	want = 'bool'
accessory_process._superclassnames = []
accessory_process._privpropdict = {
}
accessory_process._privelemdict = {
}
accessory_suitcase._superclassnames = []
accessory_suitcase._privpropdict = {
}
accessory_suitcase._privelemdict = {
}
application._superclassnames = []
application._privpropdict = {
	'Finder_preferences' : Finder_preferences,
	'about_this_computer' : about_this_computer,
	'clipboard' : clipboard,
	'desktop' : desktop,
	'file_sharing' : file_sharing,
	'frontmost' : frontmost,
	'insertion_location' : insertion_location,
	'largest_free_block' : largest_free_block,
	'name' : name,
	'product_version' : product_version,
	'properties' : properties,
	'selection' : selection,
	'sharing_starting_up' : sharing_starting_up,
	'version' : version,
	'visible' : visible,
}
application._privelemdict = {
}
import Files_and_suitcases
application_file._superclassnames = ['file']
application_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'minimum_partition_size' : minimum_partition_size,
	'partition_size' : partition_size,
	'scriptable' : scriptable,
	'suggested_partition_size' : suggested_partition_size,
}
application_file._privelemdict = {
}
container_window._superclassnames = ['window']
container_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'button_view_arrangement' : button_view_arrangement,
	'calculate_folder_sizes' : calculate_folder_sizes,
	'container' : container,
	'has_custom_view_settings' : has_custom_view_settings,
	'item' : item,
	'previous_list_view' : previous_list_view,
	'show_comments' : show_comments,
	'show_creation_date' : show_creation_date,
	'show_kind' : show_kind,
	'show_label' : show_label,
	'show_modification_date' : show_modification_date,
	'show_size' : show_size,
	'show_version' : show_version,
	'sort_direction' : sort_direction,
	'spatial_view_arrangement' : spatial_view_arrangement,
	'use_relative_dates' : use_relative_dates,
	'view' : view,
}
container_window._privelemdict = {
}
information_window._superclassnames = ['window']
information_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'comment' : comment,
	'creation_date' : creation_date,
	'icon' : icon,
	'item' : item,
	'locked' : locked,
	'minimum_partition_size' : minimum_partition_size,
	'modification_date' : modification_date,
	'partition_size' : partition_size,
	'physical_size' : physical_size,
	'product_version' : product_version,
	'size' : size,
	'stationery' : stationery,
	'suggested_partition_size' : suggested_partition_size,
	'version' : version,
	'warn_before_emptying' : warn_before_emptying,
}
information_window._privelemdict = {
}
internet_location._superclassnames = []
internet_location._privpropdict = {
}
internet_location._privelemdict = {
}
item._superclassnames = []
item._privpropdict = {
	'bounds' : bounds,
	'comment' : comment,
	'container' : container,
	'content_space' : content_space,
	'creation_date' : creation_date,
	'description' : description,
	'disk' : disk,
	'folder' : folder,
	'icon' : icon,
	'id' : id,
	'index' : index,
	'information_window' : information_window,
	'kind' : kind,
	'label_index' : label_index,
	'modification_date' : modification_date,
	'name' : name,
	'physical_size' : physical_size,
	'position' : position,
	'properties' : properties,
	'selected' : selected,
	'size' : size,
	'window' : window,
}
item._privelemdict = {
}
preferences._superclassnames = []
preferences._privpropdict = {
	'calculate_folder_sizes' : calculate_folder_sizes,
	'delay_before_springing' : delay_before_springing,
	'show_comments' : show_comments,
	'show_creation_date' : show_creation_date,
	'show_kind' : show_kind,
	'show_label' : show_label,
	'show_modification_date' : show_modification_date,
	'show_size' : show_size,
	'show_version' : show_version,
	'spring_open_folders' : spring_open_folders,
	'use_relative_dates' : use_relative_dates,
	'use_simple_menus' : use_simple_menus,
	'use_wide_grid' : use_wide_grid,
	'window' : window,
}
preferences._privelemdict = {
}
process._superclassnames = []
process._privpropdict = {
	'creator_type' : creator_type,
	'file' : file,
	'file_type' : file_type,
	'frontmost' : frontmost,
	'name' : name,
	'partition_size' : partition_size,
	'partition_space_used' : partition_space_used,
	'properties' : properties,
	'remote_events' : remote_events,
	'scriptable' : scriptable,
	'visible' : visible,
}
process._privelemdict = {
}
import Containers_and_folders
sharable_container._superclassnames = ['container']
sharable_container._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'exported' : exported,
	'group' : group,
	'group_privileges' : group_privileges,
	'guest_privileges' : guest_privileges,
	'inherited_privileges' : inherited_privileges,
	'mounted' : mounted,
	'owner' : owner,
	'owner_privileges' : owner_privileges,
	'protected' : protected,
	'shared' : shared,
}
sharable_container._privelemdict = {
}
trash_2d_object._superclassnames = ['container']
trash_2d_object._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'warn_before_emptying' : warn_before_emptying,
}
trash_2d_object._privelemdict = {
}
window._superclassnames = []
window._privpropdict = {
	'bounds' : bounds,
	'closeable' : closeable,
	'collapsed' : collapsed,
	'floating' : floating,
	'index' : index,
	'modal' : modal,
	'name' : name,
	'popup' : popup,
	'position' : position,
	'properties' : properties,
	'pulled_open' : pulled_open,
	'resizable' : resizable,
	'titled' : titled,
	'visible' : visible,
	'zoomable' : zoomable,
	'zoomed' : zoomed,
	'zoomed_full_size' : zoomed_full_size,
}
window._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'appf' : application_file,
	'capp' : application,
	'cobj' : item,
	'cprf' : preferences,
	'ctrs' : trash_2d_object,
	'cwin' : window,
	'cwnd' : container_window,
	'dsut' : accessory_suitcase,
	'inlf' : internet_location,
	'iwnd' : information_window,
	'pcda' : accessory_process,
	'prcs' : process,
	'sctr' : sharable_container,
}

_propdeclarations = {
	'ID  ' : id,
	'abbx' : about_this_computer,
	'appt' : partition_size,
	'ascd' : creation_date,
	'asdr' : folder,
	'aslk' : locked,
	'asmo' : modification_date,
	'asty' : file_type,
	'barr' : button_view_arrangement,
	'c@#^' : _3c_Inheritance_3e_,
	'cdis' : disk,
	'cobj' : item,
	'comt' : comment,
	'ctnr' : container,
	'cuss' : has_custom_view_settings,
	'cwin' : window,
	'dela' : delay_before_springing,
	'desk' : desktop,
	'drwr' : popup,
	'dscr' : description,
	'dwnd' : content_space,
	'fcrt' : creator_type,
	'file' : file,
	'fshr' : file_sharing,
	'fsup' : sharing_starting_up,
	'gppr' : group_privileges,
	'gstp' : guest_privileges,
	'hclb' : closeable,
	'iarr' : spatial_view_arrangement,
	'iimg' : icon,
	'iprv' : inherited_privileges,
	'isab' : scriptable,
	'isfl' : floating,
	'issl' : selected,
	'iszm' : zoomable,
	'iwnd' : information_window,
	'kind' : kind,
	'labi' : label_index,
	'mfre' : largest_free_block,
	'mprt' : minimum_partition_size,
	'ownr' : owner_privileges,
	'pbnd' : bounds,
	'pcli' : clipboard,
	'pfrp' : Finder_preferences,
	'phys' : physical_size,
	'pidx' : index,
	'pins' : insertion_location,
	'pisf' : frontmost,
	'pmod' : modal,
	'pnam' : name,
	'posn' : position,
	'prsz' : resizable,
	'pspd' : stationery,
	'ptit' : titled,
	'ptsz' : size,
	'pull' : pulled_open,
	'pusd' : partition_space_used,
	'pvew' : view,
	'pvis' : visible,
	'pzum' : zoomed,
	'qpro' : properties,
	'revt' : remote_events,
	'scda' : show_creation_date,
	'scom' : show_comments,
	'sdat' : show_modification_date,
	'sele' : selection,
	'sexp' : exported,
	'sfsz' : calculate_folder_sizes,
	'sgrp' : group,
	'shar' : shared,
	'sknd' : show_kind,
	'slbl' : show_label,
	'smou' : mounted,
	'sord' : sort_direction,
	'sown' : owner,
	'sprg' : spring_open_folders,
	'spro' : protected,
	'sprt' : suggested_partition_size,
	'ssiz' : show_size,
	'svew' : previous_list_view,
	'svrs' : show_version,
	'urdt' : use_relative_dates,
	'usme' : use_simple_menus,
	'uswg' : use_wide_grid,
	'ver2' : product_version,
	'vers' : version,
	'warn' : warn_before_emptying,
	'wshd' : collapsed,
	'zumf' : zoomed_full_size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
