"""Suite Window classes: Classes representing windows
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Window_classes_Events:

	pass


class window(aetools.ComponentItem):
	"""window - A window """
	want = 'cwin'
class position(aetools.NProperty):
	"""position - the upper left position of the window """
	which = 'posn'
	want = 'QDpt'
class bounds(aetools.NProperty):
	"""bounds - the boundary rectangle for the window """
	which = 'pbnd'
	want = 'qdrt'
class titled(aetools.NProperty):
	"""titled - Does the window have a title bar? """
	which = 'ptit'
	want = 'bool'
class name(aetools.NProperty):
	"""name - the name of the window """
	which = 'pnam'
	want = 'itxt'
class index(aetools.NProperty):
	"""index - the number of the window in the front-to-back layer ordering """
	which = 'pidx'
	want = 'long'
class closeable(aetools.NProperty):
	"""closeable - Does the window have a close box? """
	which = 'hclb'
	want = 'bool'
class floating(aetools.NProperty):
	"""floating - Does the window have a title bar? """
	which = 'isfl'
	want = 'bool'
class modal(aetools.NProperty):
	"""modal - Is the window modal? """
	which = 'pmod'
	want = 'bool'

resizable = titled
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
class visible(aetools.NProperty):
	"""visible - Is the window visible (always true for open Finder windows)? """
	which = 'pvis'
	want = 'bool'
class popup(aetools.NProperty):
	"""popup - Is the window is a pop-up window? (only applies to open container windows in the Finder and can only be set when the Finder is the front application) """
	which = 'drwr'
	want = 'bool'
class pulled_open(aetools.NProperty):
	"""pulled open - Is the window pulled open (only applies to pop-up windows and can only be set when the Finder is the front application)? """
	which = 'pull'
	want = 'bool'
class collapsed(aetools.NProperty):
	"""collapsed - Is the window collapsed (only applies to open non-pop-up windows)? """
	which = 'wshd'
	want = 'bool'

windows = window

class container_window(aetools.ComponentItem):
	"""container window - A window that contains items """
	want = 'cwnd'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the window class """
	which = 'c@#^'
	want = 'cwin'
class container(aetools.NProperty):
	"""container - the container from which the window was opened """
	which = 'ctnr'
	want = 'obj '
class item(aetools.NProperty):
	"""item - the item from which the window was opened (always returns something) """
	which = 'cobj'
	want = 'obj '
class has_custom_view_settings(aetools.NProperty):
	"""has custom view settings - Does the folder have custom view settings or is it using the default global settings? """
	which = 'cuss'
	want = 'bool'
class view(aetools.NProperty):
	"""view - the current view for the window (icon, name, date, etc.) """
	which = 'pvew'
	want = 'long'
class previous_list_view(aetools.NProperty):
	"""previous list view - the last non-icon view (by name, by date, etc.) selected for the container (forgotten as soon as the window is closed and only available when the window is open) """
	which = 'svew'
	want = 'enum'
class button_view_arrangement(aetools.NProperty):
	"""button view arrangement - the property by which to keep icons arranged within a button view window """
	which = 'barr'
	want = 'earr'
class spatial_view_arrangement(aetools.NProperty):
	"""spatial view arrangement - the property by which to keep icons arranged within a spatial view window """
	which = 'iarr'
	want = 'earr'
class sort_direction(aetools.NProperty):
	"""sort direction - The direction in which the window is sorted """
	which = 'sord'
	want = 'sodr'
class calculates_folder_sizes(aetools.NProperty):
	"""calculates folder sizes - Are folder sizes calculated and displayed in the window? (does not apply to suitcase windows) """
	which = 'sfsz'
	want = 'bool'
class shows_comments(aetools.NProperty):
	"""shows comments - Are comments displayed in the window? (does not apply to suitcases) """
	which = 'scom'
	want = 'bool'
class shows_creation_date(aetools.NProperty):
	"""shows creation date - Are creation dates displayed in the window? """
	which = 'scda'
	want = 'bool'
class shows_kind(aetools.NProperty):
	"""shows kind - Are document kinds displayed in the window? """
	which = 'sknd'
	want = 'bool'
class shows_label(aetools.NProperty):
	"""shows label - Are labels displayed in the window? """
	which = 'slbl'
	want = 'bool'
class shows_modification_date(aetools.NProperty):
	"""shows modification date - Are modification dates displayed in the window? """
	which = 'sdat'
	want = 'bool'
class shows_size(aetools.NProperty):
	"""shows size - Are file sizes displayed in the window? """
	which = 'ssiz'
	want = 'bool'
class shows_version(aetools.NProperty):
	"""shows version - Are file versions displayed in the window? (does not apply to suitcase windows) """
	which = 'svrs'
	want = 'bool'
class uses_relative_dates(aetools.NProperty):
	"""uses relative dates - Are relative dates (e.g., today, yesterday) shown in the window? """
	which = 'urdt'
	want = 'bool'

container_windows = container_window

class information_window(aetools.ComponentItem):
	"""information window - An information window (opened by –Get Info”) """
	want = 'iwnd'
class current_panel(aetools.NProperty):
	"""current panel - the current panel in the information window """
	which = 'panl'
	want = 'ipnl'
class comment(aetools.NProperty):
	"""comment - the comment """
	which = 'comt'
	want = 'itxt'
class size(aetools.NProperty):
	"""size - the logical size of the item """
	which = 'ptsz'
	want = 'long'
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the item on disk """
	which = 'phys'
	want = 'long'
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created """
	which = 'ascd'
	want = 'ldt '
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified """
	which = 'asmo'
	want = 'ldt '
class suggested_size(aetools.NProperty):
	"""suggested size - the memory size with which the developer recommends the application be launched """
	which = 'sprt'
	want = 'long'
class minimum_size(aetools.NProperty):
	"""minimum size - the smallest memory size with which the application can be launched (only applies to information windows for applications) """
	which = 'mprt'
	want = 'long'
class preferred_size(aetools.NProperty):
	"""preferred size - the memory size with which the application will be launched (only applies to information windows for applications) """
	which = 'appt'
	want = 'long'
class icon(aetools.NProperty):
	"""icon - the icon bitmap of the item """
	which = 'iimg'
	want = 'ifam'
class locked(aetools.NProperty):
	"""locked - Is the item locked (applies only to file and application information windows)? """
	which = 'aslk'
	want = 'bool'
class stationery(aetools.NProperty):
	"""stationery - Is the item a stationery pad? """
	which = 'pspd'
	want = 'bool'
class warns_before_emptying(aetools.NProperty):
	"""warns before emptying - Display a dialog when emptying the trash (only valid for trash info window)? """
	which = 'warn'
	want = 'bool'
class product_version(aetools.NProperty):
	"""product version - the version of the product (visible at the top of the –Get Info” window) """
	which = 'ver2'
	want = 'itxt'
class version(aetools.NProperty):
	"""version - the version of the file (visible at the bottom of the –Get Info” window) """
	which = 'vers'
	want = 'itxt'

information_windows = information_window

class preferences_window(aetools.ComponentItem):
	"""preferences window - The Finder Preferences window """
	want = 'pwnd'

class clipping_window(aetools.ComponentItem):
	"""clipping window - The window containing a clipping """
	want = 'lwnd'

clipping_windows = clipping_window

class content_space(aetools.ComponentItem):
	"""content space - All windows, including the desktop window (–Window” does not include the desktop window) """
	want = 'dwnd'

content_spaces = content_space
window._propdict = {
	'position' : position,
	'bounds' : bounds,
	'titled' : titled,
	'name' : name,
	'index' : index,
	'closeable' : closeable,
	'floating' : floating,
	'modal' : modal,
	'resizable' : resizable,
	'zoomable' : zoomable,
	'zoomed' : zoomed,
	'zoomed_full_size' : zoomed_full_size,
	'visible' : visible,
	'popup' : popup,
	'pulled_open' : pulled_open,
	'collapsed' : collapsed,
}
window._elemdict = {
}
container_window._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'container' : container,
	'item' : item,
	'has_custom_view_settings' : has_custom_view_settings,
	'view' : view,
	'previous_list_view' : previous_list_view,
	'button_view_arrangement' : button_view_arrangement,
	'spatial_view_arrangement' : spatial_view_arrangement,
	'sort_direction' : sort_direction,
	'calculates_folder_sizes' : calculates_folder_sizes,
	'shows_comments' : shows_comments,
	'shows_creation_date' : shows_creation_date,
	'shows_kind' : shows_kind,
	'shows_label' : shows_label,
	'shows_modification_date' : shows_modification_date,
	'shows_size' : shows_size,
	'shows_version' : shows_version,
	'uses_relative_dates' : uses_relative_dates,
}
container_window._elemdict = {
}
information_window._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'item' : item,
	'current_panel' : current_panel,
	'comment' : comment,
	'size' : size,
	'physical_size' : physical_size,
	'creation_date' : creation_date,
	'modification_date' : modification_date,
	'suggested_size' : suggested_size,
	'minimum_size' : minimum_size,
	'preferred_size' : preferred_size,
	'icon' : icon,
	'locked' : locked,
	'stationery' : stationery,
	'warns_before_emptying' : warns_before_emptying,
	'product_version' : product_version,
	'version' : version,
}
information_window._elemdict = {
}
preferences_window._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'current_panel' : current_panel,
}
preferences_window._elemdict = {
}
clipping_window._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
clipping_window._elemdict = {
}
content_space._propdict = {
}
content_space._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'dwnd' : content_space,
	'iwnd' : information_window,
	'lwnd' : clipping_window,
	'cwnd' : container_window,
	'cwin' : window,
	'pwnd' : preferences_window,
}

_propdeclarations = {
	'pidx' : index,
	'scda' : shows_creation_date,
	'vers' : version,
	'aslk' : locked,
	'pvew' : view,
	'sdat' : shows_modification_date,
	'drwr' : popup,
	'sprt' : suggested_size,
	'pvis' : visible,
	'ptsz' : size,
	'pull' : pulled_open,
	'slbl' : shows_label,
	'wshd' : collapsed,
	'ctnr' : container,
	'ascd' : creation_date,
	'warn' : warns_before_emptying,
	'sord' : sort_direction,
	'iszm' : zoomable,
	'comt' : comment,
	'svew' : previous_list_view,
	'svrs' : shows_version,
	'sknd' : shows_kind,
	'phys' : physical_size,
	'iarr' : spatial_view_arrangement,
	'posn' : position,
	'ptit' : titled,
	'cobj' : item,
	'asmo' : modification_date,
	'ssiz' : shows_size,
	'pnam' : name,
	'pbnd' : bounds,
	'mprt' : minimum_size,
	'iimg' : icon,
	'cuss' : has_custom_view_settings,
	'appt' : preferred_size,
	'scom' : shows_comments,
	'pmod' : modal,
	'panl' : current_panel,
	'urdt' : uses_relative_dates,
	'zumf' : zoomed_full_size,
	'sfsz' : calculates_folder_sizes,
	'c@#^' : _3c_Inheritance_3e_,
	'isfl' : floating,
	'hclb' : closeable,
	'pspd' : stationery,
	'pzum' : zoomed,
	'barr' : button_view_arrangement,
	'ver2' : product_version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
