"""Suite Obsolete terms: Terms that have been deprecated
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpnm'

from StdSuites.Type_Names_Suite import *
class Obsolete_terms_Events(Type_Names_Suite_Events):

	pass


class application(aetools.ComponentItem):
	"""application - The Finder """
	want = 'capp'
class view_preferences(aetools.NProperty):
	"""view preferences - backwards compatibility with Finder Scripting Extension. DEPRECATED -- not supported after Finder 8.0 """
	which = 'pvwp'
	want = 'obj '

class container(aetools.ComponentItem):
	"""container - An item that contains other items """
	want = 'ctnr'
class container_window(aetools.NProperty):
	"""container window - the main window for the container """
	which = 'cwnd'
	want = 'obj '

class container_window(aetools.ComponentItem):
	"""container window - A window that contains items """
	want = 'cwnd'
class folder_obsolete(aetools.NProperty):
	"""folder obsolete - the folder from which the window was opened (DEPRECATED - for use with scripts compiled before Finder 8.0. Will be removed in the next release.) """
	which = 'cfol'
	want = 'obj '

class control_panel(aetools.ComponentItem):
	"""control panel - A control panel """
	want = 'ccdv'

control_panels = control_panel

class file(aetools.ComponentItem):
	"""file - A file """
	want = 'file'
class file_type_obsolete(aetools.NProperty):
	"""file type obsolete - the OSType identifying the type of data contained in the item (DEPRECATED - for use with scripts compiled before Finder 8.0. Will be removed in the next release) """
	which = 'fitp'
	want = 'type'
class locked_obsolete(aetools.NProperty):
	"""locked obsolete - Is the file locked? (DEPRECATED - for use with scripts compiled before Finder 8.0. Will be removed in the next release) """
	which = 'islk'
	want = 'bool'

class information_window(aetools.ComponentItem):
	"""information window - An information window (opened by –Get Info”) """
	want = 'iwnd'
class creation_date_obsolete(aetools.NProperty):
	"""creation date obsolete - the date on which the item was created (DEPRECATED - for use with scripts compiled before Finder 8.0. Will be removed in the next release) """
	which = 'crtd'
	want = 'ldt '
class modification_date_obsolete(aetools.NProperty):
	"""modification date obsolete - the date on which the item was last modified (DEPRECATED - for use with scripts compiled before Finder 8.0. Will be removed in the next release) """
	which = 'modd'
	want = 'ldt '

class item(aetools.ComponentItem):
	"""item - An item """
	want = 'cobj'

class process(aetools.ComponentItem):
	"""process - A process running on this computer """
	want = 'prcs'

class sharable_container(aetools.ComponentItem):
	"""sharable container - A container that may be shared (disks and folders) """
	want = 'sctr'
class sharing_window(aetools.NProperty):
	"""sharing window - the sharing window for the container (file sharing must be on to use this property) """
	which = 'swnd'
	want = 'obj '

class sharing_window(aetools.ComponentItem):
	"""sharing window - A sharing window (opened by –Sharingƒ”) """
	want = 'swnd'
class sharable_container(aetools.NProperty):
	"""sharable container - the sharable container from which the window was opened """
	which = 'sctr'
	want = 'obj '
class item(aetools.NProperty):
	"""item - the item from which this window was opened """
	which = 'cobj'
	want = 'obj '
class container(aetools.NProperty):
	"""container - the container from which this window was opened """
	which = 'ctnr'
	want = 'obj '

sharing_windows = sharing_window

class status_window(aetools.ComponentItem):
	"""status window - Progress dialogs (e.g., copy window, rebuild desktop database, empty trash) """
	want = 'qwnd'

status_windows = status_window
application._propdict = {
	'view_preferences' : view_preferences,
}
application._elemdict = {
}
container._propdict = {
	'container_window' : container_window,
}
container._elemdict = {
}
container_window._propdict = {
	'folder_obsolete' : folder_obsolete,
}
container_window._elemdict = {
}
control_panel._propdict = {
}
control_panel._elemdict = {
}
file._propdict = {
	'file_type_obsolete' : file_type_obsolete,
	'locked_obsolete' : locked_obsolete,
}
file._elemdict = {
}
information_window._propdict = {
	'creation_date_obsolete' : creation_date_obsolete,
	'locked_obsolete' : locked_obsolete,
	'modification_date_obsolete' : modification_date_obsolete,
}
information_window._elemdict = {
}
item._propdict = {
	'creation_date_obsolete' : creation_date_obsolete,
	'folder_obsolete' : folder_obsolete,
	'modification_date_obsolete' : modification_date_obsolete,
}
item._elemdict = {
}
process._propdict = {
	'file_type_obsolete' : file_type_obsolete,
}
process._elemdict = {
}
sharable_container._propdict = {
	'sharing_window' : sharing_window,
}
sharable_container._elemdict = {
}
sharing_window._propdict = {
	'sharable_container' : sharable_container,
	'item' : item,
	'container' : container,
	'folder_obsolete' : folder_obsolete,
}
sharing_window._elemdict = {
}
status_window._propdict = {
}
status_window._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'ccdv' : control_panel,
	'iwnd' : information_window,
	'ctnr' : container,
	'capp' : application,
	'sctr' : sharable_container,
	'cwnd' : container_window,
	'prcs' : process,
	'file' : file,
	'cobj' : item,
	'qwnd' : status_window,
	'swnd' : sharing_window,
}

_propdeclarations = {
	'crtd' : creation_date_obsolete,
	'cfol' : folder_obsolete,
	'ctnr' : container,
	'cwnd' : container_window,
	'pvwp' : view_preferences,
	'swnd' : sharing_window,
	'sctr' : sharable_container,
	'cobj' : item,
	'modd' : modification_date_obsolete,
	'islk' : locked_obsolete,
	'fitp' : file_type_obsolete,
}

_compdeclarations = {
}

_enumdeclarations = {
}
