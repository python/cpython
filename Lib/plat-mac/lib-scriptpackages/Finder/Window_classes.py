"""Suite Window classes: Classes representing windows
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Window_classes_Events:

	pass


class Finder_window(aetools.ComponentItem):
	"""Finder window - A file viewer window """
	want = 'brow'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the window class """
	which = 'c@#^'
	want = 'cwin'
class current_view(aetools.NProperty):
	"""current view - the current view for the container window """
	which = 'pvew'
	want = 'ecvw'
class icon_view_options(aetools.NProperty):
	"""icon view options - the icon view options for the container window """
	which = 'icop'
	want = 'icop'
class list_view_options(aetools.NProperty):
	"""list view options - the list view options for the container window """
	which = 'lvop'
	want = 'lvop'
class target(aetools.NProperty):
	"""target - the container at which this file viewer is targeted """
	which = 'fvtg'
	want = 'obj '

Finder_windows = Finder_window

class clipping_window(aetools.ComponentItem):
	"""clipping window - The window containing a clipping """
	want = 'lwnd'

clipping_windows = clipping_window

class information_window(aetools.ComponentItem):
	"""information window - An inspector window (opened by \xd2Show Info\xd3) """
	want = 'iwnd'
class current_panel(aetools.NProperty):
	"""current panel - the current panel in the information window """
	which = 'panl'
	want = 'ipnl'
class item(aetools.NProperty):
	"""item - the item from which this window was opened """
	which = 'cobj'
	want = 'obj '

class preferences_window(aetools.ComponentItem):
	"""preferences window - (NOT AVAILABLE YET) The Finder Preferences window """
	want = 'pwnd'

class window(aetools.ComponentItem):
	"""window - A window """
	want = 'cwin'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window """
	which = 'pbnd'
	want = 'qdrt'
class closeable(aetools.NProperty):
	"""closeable - Does the window have a close box? """
	which = 'hclb'
	want = 'bool'
class collapsed(aetools.NProperty):
	"""collapsed - Is the window collapsed """
	which = 'wshd'
	want = 'bool'
class floating(aetools.NProperty):
	"""floating - Does the window have a title bar? """
	which = 'isfl'
	want = 'bool'
class id(aetools.NProperty):
	"""id - the unique id for this window """
	which = 'ID  '
	want = 'magn'
class index(aetools.NProperty):
	"""index - the number of the window in the front-to-back layer ordering """
	which = 'pidx'
	want = 'long'
class modal(aetools.NProperty):
	"""modal - Is the window modal? """
	which = 'pmod'
	want = 'bool'
class name(aetools.NProperty):
	"""name - the name of the window """
	which = 'pnam'
	want = 'utxt'
class position(aetools.NProperty):
	"""position - the upper left position of the window """
	which = 'posn'
	want = 'QDpt'
class properties(aetools.NProperty):
	"""properties - every property of a window """
	which = 'pALL'
	want = 'reco'
class resizable(aetools.NProperty):
	"""resizable - Is the window resizable? """
	which = 'prsz'
	want = 'bool'
class titled(aetools.NProperty):
	"""titled - Does the window have a title bar? """
	which = 'ptit'
	want = 'bool'
class visible(aetools.NProperty):
	"""visible - Is the window visible (always true for open Finder windows)? """
	which = 'pvis'
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
	"""zoomed full size - Is the window zoomed to the full size of the screen? (can only be set, not read) """
	which = 'zumf'
	want = 'bool'

windows = window
Finder_window._superclassnames = ['window']
Finder_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'current_view' : current_view,
	'icon_view_options' : icon_view_options,
	'list_view_options' : list_view_options,
	'target' : target,
}
Finder_window._privelemdict = {
}
clipping_window._superclassnames = ['window']
clipping_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
clipping_window._privelemdict = {
}
information_window._superclassnames = ['window']
information_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'current_panel' : current_panel,
	'item' : item,
}
information_window._privelemdict = {
}
preferences_window._superclassnames = ['window']
preferences_window._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'current_panel' : current_panel,
}
preferences_window._privelemdict = {
}
window._superclassnames = []
window._privpropdict = {
	'bounds' : bounds,
	'closeable' : closeable,
	'collapsed' : collapsed,
	'floating' : floating,
	'id' : id,
	'index' : index,
	'modal' : modal,
	'name' : name,
	'position' : position,
	'properties' : properties,
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
	'brow' : Finder_window,
	'cwin' : window,
	'iwnd' : information_window,
	'lwnd' : clipping_window,
	'pwnd' : preferences_window,
}

_propdeclarations = {
	'ID  ' : id,
	'c@#^' : _3c_Inheritance_3e_,
	'cobj' : item,
	'fvtg' : target,
	'hclb' : closeable,
	'icop' : icon_view_options,
	'isfl' : floating,
	'iszm' : zoomable,
	'lvop' : list_view_options,
	'pALL' : properties,
	'panl' : current_panel,
	'pbnd' : bounds,
	'pidx' : index,
	'pmod' : modal,
	'pnam' : name,
	'posn' : position,
	'prsz' : resizable,
	'ptit' : titled,
	'pvew' : current_view,
	'pvis' : visible,
	'pzum' : zoomed,
	'wshd' : collapsed,
	'zumf' : zoomed_full_size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
