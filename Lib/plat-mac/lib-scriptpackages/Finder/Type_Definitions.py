"""Suite Type Definitions: Definitions of records used in scripting the Finder
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpdf'

class Type_Definitions_Events:

    pass


class alias_list(aetools.ComponentItem):
    """alias list - A list of aliases.  Use \xd4as alias list\xd5 when a list of aliases is needed (instead of a list of file system item references). """
    want = 'alst'

class label(aetools.ComponentItem):
    """label - (NOT AVAILABLE YET) A Finder label (name and color) """
    want = 'clbl'
class _Prop_color(aetools.NProperty):
    """color - the color associated with the label """
    which = 'colr'
    want = 'cRGB'
class _Prop_index(aetools.NProperty):
    """index - the index in the front-to-back ordering within its container """
    which = 'pidx'
    want = 'long'
class _Prop_name(aetools.NProperty):
    """name - the name associated with the label """
    which = 'pnam'
    want = 'utxt'

class preferences(aetools.ComponentItem):
    """preferences - (NOT AVAILABLE, SUBJECT TO CHANGE) The Finder Preferences """
    want = 'cprf'
class _Prop_button_view_arrangement(aetools.NProperty):
    """button view arrangement - the method of arrangement of icons in default Finder button view windows """
    which = 'barr'
    want = 'earr'
class _Prop_button_view_icon_size(aetools.NProperty):
    """button view icon size - the size of icons displayed in Finder button view windows. """
    which = 'bisz'
    want = 'long'
class _Prop_calculates_folder_sizes(aetools.NProperty):
    """calculates folder sizes - Are folder sizes calculated and displayed in Finder list view windows? """
    which = 'sfsz'
    want = 'bool'
class _Prop_delay_before_springing(aetools.NProperty):
    """delay before springing - the delay before springing open a container in ticks (1/60th of a second) (12 is shortest delay, 60 is longest delay) """
    which = 'dela'
    want = 'shor'
class _Prop_list_view_icon_size(aetools.NProperty):
    """list view icon size - the size of icons displayed in Finder list view windows. """
    which = 'lisz'
    want = 'long'
class _Prop_shows_comments(aetools.NProperty):
    """shows comments - Are comments displayed in default Finder list view windows? """
    which = 'scom'
    want = 'bool'
class _Prop_shows_creation_date(aetools.NProperty):
    """shows creation date - Are creation dates displayed in default Finder list view windows? """
    which = 'scda'
    want = 'bool'
class _Prop_shows_kind(aetools.NProperty):
    """shows kind - Are document kinds displayed in default Finder list view windows? """
    which = 'sknd'
    want = 'bool'
class _Prop_shows_label(aetools.NProperty):
    """shows label - Are labels displayed in default Finder list view windows? """
    which = 'slbl'
    want = 'bool'
class _Prop_shows_modification_date(aetools.NProperty):
    """shows modification date - Are modification dates displayed in default Finder list view windows? """
    which = 'sdat'
    want = 'bool'
class _Prop_shows_size(aetools.NProperty):
    """shows size - Are file sizes displayed in default Finder list view windows? """
    which = 'ssiz'
    want = 'bool'
class _Prop_shows_version(aetools.NProperty):
    """shows version - Are file versions displayed in default Finder list view windows? """
    which = 'svrs'
    want = 'bool'
class _Prop_spatial_view_arrangement(aetools.NProperty):
    """spatial view arrangement - the method of arrangement of icons in default Finder spatial view windows """
    which = 'iarr'
    want = 'earr'
class _Prop_spatial_view_icon_size(aetools.NProperty):
    """spatial view icon size - the size of icons displayed in Finder spatial view windows. """
    which = 'iisz'
    want = 'long'
class _Prop_spring_open_folders(aetools.NProperty):
    """spring open folders - Spring open folders after the specified delay? """
    which = 'sprg'
    want = 'bool'
class _Prop_uses_relative_dates(aetools.NProperty):
    """uses relative dates - Are relative dates (e.g., today, yesterday) shown  in Finder list view windows? """
    which = 'urdt'
    want = 'bool'
class _Prop_uses_simple_menus(aetools.NProperty):
    """uses simple menus - Use simplified Finder menus? """
    which = 'usme'
    want = 'bool'
class _Prop_uses_wide_grid(aetools.NProperty):
    """uses wide grid - Space icons on a wide grid? """
    which = 'uswg'
    want = 'bool'
class _Prop_view_font(aetools.NProperty):
    """view font - the id of the font used in Finder views. """
    which = 'vfnt'
    want = 'long'
class _Prop_view_font_size(aetools.NProperty):
    """view font size - the size of the font used in Finder views """
    which = 'vfsz'
    want = 'long'
class _Prop_window(aetools.NProperty):
    """window - the window that would open if Finder preferences was opened """
    which = 'cwin'
    want = 'pwnd'
#        element 'clbl' as ['indx', 'name']

class icon_view_options(aetools.ComponentItem):
    """icon view options - the icon view options """
    want = 'icop'

_Prop_arrangement = _Prop_spatial_view_arrangement
class _Prop_icon_size(aetools.NProperty):
    """icon size - the size of icons displayed in the icon view """
    which = 'lvis'
    want = 'shor'

class icon_family(aetools.ComponentItem):
    """icon family - (NOT AVAILABLE YET) A family of icons """
    want = 'ifam'
class _Prop_large_32_bit_icon(aetools.NProperty):
    """large 32 bit icon - the large 32-bit color icon """
    which = 'il32'
    want = 'il32'
class _Prop_large_4_bit_icon(aetools.NProperty):
    """large 4 bit icon - the large 4-bit color icon """
    which = 'icl4'
    want = 'icl4'
class _Prop_large_8_bit_icon(aetools.NProperty):
    """large 8 bit icon - the large 8-bit color icon """
    which = 'icl8'
    want = 'icl8'
class _Prop_large_8_bit_mask(aetools.NProperty):
    """large 8 bit mask - the large 8-bit mask for large 32-bit icons """
    which = 'l8mk'
    want = 'l8mk'
class _Prop_large_monochrome_icon_and_mask(aetools.NProperty):
    """large monochrome icon and mask - the large black-and-white icon and the mask for large icons """
    which = 'ICN#'
    want = 'ICN#'
class _Prop_small_32_bit_icon(aetools.NProperty):
    """small 32 bit icon - the small 32-bit color icon """
    which = 'is32'
    want = 'is32'
class _Prop_small_4_bit_icon(aetools.NProperty):
    """small 4 bit icon - the small 4-bit color icon """
    which = 'ics4'
    want = 'ics4'
class _Prop_small_8_bit_icon(aetools.NProperty):
    """small 8 bit icon - the small 8-bit color icon """
    which = 'ics8'
    want = 'ics8'

_Prop_small_8_bit_mask = _Prop_small_8_bit_icon
class _Prop_small_monochrome_icon_and_mask(aetools.NProperty):
    """small monochrome icon and mask - the small black-and-white icon and the mask for small icons """
    which = 'ics#'
    want = 'ics#'

class column(aetools.ComponentItem):
    """column - a column of a list view """
    want = 'lvcl'
class _Prop_sort_direction(aetools.NProperty):
    """sort direction - The direction in which the window is sorted """
    which = 'sord'
    want = 'sodr'
class _Prop_visible(aetools.NProperty):
    """visible - is this column visible """
    which = 'pvis'
    want = 'bool'
class _Prop_width(aetools.NProperty):
    """width - the width of this column """
    which = 'clwd'
    want = 'shor'

columns = column

class list_view_options(aetools.ComponentItem):
    """list view options - the list view options """
    want = 'lvop'
class _Prop_sort_column(aetools.NProperty):
    """sort column - the column that the list view is sorted on """
    which = 'srtc'
    want = 'lvcl'
#        element 'lvcl' as ['indx', 'rele', 'rang', 'test']
alias_list._superclassnames = []
alias_list._privpropdict = {
}
alias_list._privelemdict = {
}
label._superclassnames = []
label._privpropdict = {
    'color' : _Prop_color,
    'index' : _Prop_index,
    'name' : _Prop_name,
}
label._privelemdict = {
}
preferences._superclassnames = []
preferences._privpropdict = {
    'button_view_arrangement' : _Prop_button_view_arrangement,
    'button_view_icon_size' : _Prop_button_view_icon_size,
    'calculates_folder_sizes' : _Prop_calculates_folder_sizes,
    'delay_before_springing' : _Prop_delay_before_springing,
    'list_view_icon_size' : _Prop_list_view_icon_size,
    'shows_comments' : _Prop_shows_comments,
    'shows_creation_date' : _Prop_shows_creation_date,
    'shows_kind' : _Prop_shows_kind,
    'shows_label' : _Prop_shows_label,
    'shows_modification_date' : _Prop_shows_modification_date,
    'shows_size' : _Prop_shows_size,
    'shows_version' : _Prop_shows_version,
    'spatial_view_arrangement' : _Prop_spatial_view_arrangement,
    'spatial_view_icon_size' : _Prop_spatial_view_icon_size,
    'spring_open_folders' : _Prop_spring_open_folders,
    'uses_relative_dates' : _Prop_uses_relative_dates,
    'uses_simple_menus' : _Prop_uses_simple_menus,
    'uses_wide_grid' : _Prop_uses_wide_grid,
    'view_font' : _Prop_view_font,
    'view_font_size' : _Prop_view_font_size,
    'window' : _Prop_window,
}
preferences._privelemdict = {
    'label' : label,
}
icon_view_options._superclassnames = []
icon_view_options._privpropdict = {
    'arrangement' : _Prop_arrangement,
    'icon_size' : _Prop_icon_size,
}
icon_view_options._privelemdict = {
}
icon_family._superclassnames = []
icon_family._privpropdict = {
    'large_32_bit_icon' : _Prop_large_32_bit_icon,
    'large_4_bit_icon' : _Prop_large_4_bit_icon,
    'large_8_bit_icon' : _Prop_large_8_bit_icon,
    'large_8_bit_mask' : _Prop_large_8_bit_mask,
    'large_monochrome_icon_and_mask' : _Prop_large_monochrome_icon_and_mask,
    'small_32_bit_icon' : _Prop_small_32_bit_icon,
    'small_4_bit_icon' : _Prop_small_4_bit_icon,
    'small_8_bit_icon' : _Prop_small_8_bit_icon,
    'small_8_bit_mask' : _Prop_small_8_bit_mask,
    'small_monochrome_icon_and_mask' : _Prop_small_monochrome_icon_and_mask,
}
icon_family._privelemdict = {
}
column._superclassnames = []
column._privpropdict = {
    'index' : _Prop_index,
    'name' : _Prop_name,
    'sort_direction' : _Prop_sort_direction,
    'visible' : _Prop_visible,
    'width' : _Prop_width,
}
column._privelemdict = {
}
list_view_options._superclassnames = []
list_view_options._privpropdict = {
    'calculates_folder_sizes' : _Prop_calculates_folder_sizes,
    'icon_size' : _Prop_icon_size,
    'sort_column' : _Prop_sort_column,
    'uses_relative_dates' : _Prop_uses_relative_dates,
}
list_view_options._privelemdict = {
    'column' : column,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'alst' : alias_list,
    'clbl' : label,
    'cprf' : preferences,
    'icop' : icon_view_options,
    'ifam' : icon_family,
    'lvcl' : column,
    'lvop' : list_view_options,
}

_propdeclarations = {
    'ICN#' : _Prop_large_monochrome_icon_and_mask,
    'barr' : _Prop_button_view_arrangement,
    'bisz' : _Prop_button_view_icon_size,
    'clwd' : _Prop_width,
    'colr' : _Prop_color,
    'cwin' : _Prop_window,
    'dela' : _Prop_delay_before_springing,
    'iarr' : _Prop_spatial_view_arrangement,
    'icl4' : _Prop_large_4_bit_icon,
    'icl8' : _Prop_large_8_bit_icon,
    'ics#' : _Prop_small_monochrome_icon_and_mask,
    'ics4' : _Prop_small_4_bit_icon,
    'ics8' : _Prop_small_8_bit_icon,
    'iisz' : _Prop_spatial_view_icon_size,
    'il32' : _Prop_large_32_bit_icon,
    'is32' : _Prop_small_32_bit_icon,
    'l8mk' : _Prop_large_8_bit_mask,
    'lisz' : _Prop_list_view_icon_size,
    'lvis' : _Prop_icon_size,
    'pidx' : _Prop_index,
    'pnam' : _Prop_name,
    'pvis' : _Prop_visible,
    'scda' : _Prop_shows_creation_date,
    'scom' : _Prop_shows_comments,
    'sdat' : _Prop_shows_modification_date,
    'sfsz' : _Prop_calculates_folder_sizes,
    'sknd' : _Prop_shows_kind,
    'slbl' : _Prop_shows_label,
    'sord' : _Prop_sort_direction,
    'sprg' : _Prop_spring_open_folders,
    'srtc' : _Prop_sort_column,
    'ssiz' : _Prop_shows_size,
    'svrs' : _Prop_shows_version,
    'urdt' : _Prop_uses_relative_dates,
    'usme' : _Prop_uses_simple_menus,
    'uswg' : _Prop_uses_wide_grid,
    'vfnt' : _Prop_view_font,
    'vfsz' : _Prop_view_font_size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
