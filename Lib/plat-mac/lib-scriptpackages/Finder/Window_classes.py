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
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - inherits some of its properties from the window class """
    which = 'c@#^'
    want = 'cwin'
class _Prop_current_view(aetools.NProperty):
    """current view - the current view for the container window """
    which = 'pvew'
    want = 'ecvw'
class _Prop_icon_view_options(aetools.NProperty):
    """icon view options - the icon view options for the container window """
    which = 'icop'
    want = 'icop'
class _Prop_list_view_options(aetools.NProperty):
    """list view options - the list view options for the container window """
    which = 'lvop'
    want = 'lvop'
class _Prop_target(aetools.NProperty):
    """target - the container at which this file viewer is targeted """
    which = 'fvtg'
    want = 'obj '

Finder_windows = Finder_window

class window(aetools.ComponentItem):
    """window - A window """
    want = 'cwin'
class _Prop_bounds(aetools.NProperty):
    """bounds - the boundary rectangle for the window """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_closeable(aetools.NProperty):
    """closeable - Does the window have a close box? """
    which = 'hclb'
    want = 'bool'
class _Prop_collapsed(aetools.NProperty):
    """collapsed - Is the window collapsed """
    which = 'wshd'
    want = 'bool'
class _Prop_floating(aetools.NProperty):
    """floating - Does the window have a title bar? """
    which = 'isfl'
    want = 'bool'
class _Prop_id(aetools.NProperty):
    """id - the unique id for this window """
    which = 'ID  '
    want = 'magn'
class _Prop_index(aetools.NProperty):
    """index - the number of the window in the front-to-back layer ordering """
    which = 'pidx'
    want = 'long'
class _Prop_modal(aetools.NProperty):
    """modal - Is the window modal? """
    which = 'pmod'
    want = 'bool'
class _Prop_name(aetools.NProperty):
    """name - the name of the window """
    which = 'pnam'
    want = 'utxt'
class _Prop_position(aetools.NProperty):
    """position - the upper left position of the window """
    which = 'posn'
    want = 'QDpt'
class _Prop_properties(aetools.NProperty):
    """properties - every property of a window """
    which = 'pALL'
    want = 'reco'
class _Prop_resizable(aetools.NProperty):
    """resizable - Is the window resizable? """
    which = 'prsz'
    want = 'bool'
class _Prop_titled(aetools.NProperty):
    """titled - Does the window have a title bar? """
    which = 'ptit'
    want = 'bool'
class _Prop_visible(aetools.NProperty):
    """visible - Is the window visible (always true for open Finder windows)? """
    which = 'pvis'
    want = 'bool'
class _Prop_zoomable(aetools.NProperty):
    """zoomable - Is the window zoomable? """
    which = 'iszm'
    want = 'bool'
class _Prop_zoomed(aetools.NProperty):
    """zoomed - Is the window zoomed? """
    which = 'pzum'
    want = 'bool'
class _Prop_zoomed_full_size(aetools.NProperty):
    """zoomed full size - Is the window zoomed to the full size of the screen? (can only be set, not read) """
    which = 'zumf'
    want = 'bool'

windows = window

class information_window(aetools.ComponentItem):
    """information window - An inspector window (opened by \xd2Show Info\xd3) """
    want = 'iwnd'
class _Prop_current_panel(aetools.NProperty):
    """current panel - the current panel in the information window """
    which = 'panl'
    want = 'ipnl'
class _Prop_item(aetools.NProperty):
    """item - the item from which this window was opened """
    which = 'cobj'
    want = 'obj '

class clipping_window(aetools.ComponentItem):
    """clipping window - The window containing a clipping """
    want = 'lwnd'

clipping_windows = clipping_window

class preferences_window(aetools.ComponentItem):
    """preferences window - (NOT AVAILABLE YET) The Finder Preferences window """
    want = 'pwnd'
Finder_window._superclassnames = ['window']
Finder_window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'current_view' : _Prop_current_view,
    'icon_view_options' : _Prop_icon_view_options,
    'list_view_options' : _Prop_list_view_options,
    'target' : _Prop_target,
}
Finder_window._privelemdict = {
}
window._superclassnames = []
window._privpropdict = {
    'bounds' : _Prop_bounds,
    'closeable' : _Prop_closeable,
    'collapsed' : _Prop_collapsed,
    'floating' : _Prop_floating,
    'id' : _Prop_id,
    'index' : _Prop_index,
    'modal' : _Prop_modal,
    'name' : _Prop_name,
    'position' : _Prop_position,
    'properties' : _Prop_properties,
    'resizable' : _Prop_resizable,
    'titled' : _Prop_titled,
    'visible' : _Prop_visible,
    'zoomable' : _Prop_zoomable,
    'zoomed' : _Prop_zoomed,
    'zoomed_full_size' : _Prop_zoomed_full_size,
}
window._privelemdict = {
}
information_window._superclassnames = ['window']
information_window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'current_panel' : _Prop_current_panel,
    'item' : _Prop_item,
}
information_window._privelemdict = {
}
clipping_window._superclassnames = ['window']
clipping_window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
clipping_window._privelemdict = {
}
preferences_window._superclassnames = ['window']
preferences_window._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'current_panel' : _Prop_current_panel,
}
preferences_window._privelemdict = {
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
    'ID  ' : _Prop_id,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'cobj' : _Prop_item,
    'fvtg' : _Prop_target,
    'hclb' : _Prop_closeable,
    'icop' : _Prop_icon_view_options,
    'isfl' : _Prop_floating,
    'iszm' : _Prop_zoomable,
    'lvop' : _Prop_list_view_options,
    'pALL' : _Prop_properties,
    'panl' : _Prop_current_panel,
    'pbnd' : _Prop_bounds,
    'pidx' : _Prop_index,
    'pmod' : _Prop_modal,
    'pnam' : _Prop_name,
    'posn' : _Prop_position,
    'prsz' : _Prop_resizable,
    'ptit' : _Prop_titled,
    'pvew' : _Prop_current_view,
    'pvis' : _Prop_visible,
    'pzum' : _Prop_zoomed,
    'wshd' : _Prop_collapsed,
    'zumf' : _Prop_zoomed_full_size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
