"""Suite Type Definitions: Definitions of records used in scripting the Finder
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'tpdf'

class Type_Definitions_Events:

	pass


class preferences(aetools.ComponentItem):
	"""preferences - The Finder Preferences """
	want = 'cprf'
class window(aetools.NProperty):
	"""window - the window that would open if Finder preferences was opened """
	which = 'cwin'
	want = 'pwnd'
class button_view_arrangement(aetools.NProperty):
	"""button view arrangement - the method of arrangement of icons in default Finder button view windows """
	which = 'barr'
	want = 'earr'
class button_view_icon_size(aetools.NProperty):
	"""button view icon size - the size of icons displayed in Finder button view windows. """
	which = 'bisz'
	want = 'long'
class spatial_view_arrangement(aetools.NProperty):
	"""spatial view arrangement - the method of arrangement of icons in default Finder spatial view windows """
	which = 'iarr'
	want = 'earr'
class spatial_view_icon_size(aetools.NProperty):
	"""spatial view icon size - the size of icons displayed in Finder spatial view windows. """
	which = 'iisz'
	want = 'long'
class calculates_folder_sizes(aetools.NProperty):
	"""calculates folder sizes - Are folder sizes calculated and displayed in Finder list view windows? """
	which = 'sfsz'
	want = 'bool'
class list_view_icon_size(aetools.NProperty):
	"""list view icon size - the size of icons displayed in Finder list view windows. """
	which = 'lisz'
	want = 'long'
class shows_comments(aetools.NProperty):
	"""shows comments - Are comments displayed in default Finder list view windows? """
	which = 'scom'
	want = 'bool'
class shows_creation_date(aetools.NProperty):
	"""shows creation date - Are creation dates displayed in default Finder list view windows? """
	which = 'scda'
	want = 'bool'
class shows_kind(aetools.NProperty):
	"""shows kind - Are document kinds displayed in default Finder list view windows? """
	which = 'sknd'
	want = 'bool'
class shows_label(aetools.NProperty):
	"""shows label - Are labels displayed in default Finder list view windows? """
	which = 'slbl'
	want = 'bool'
class shows_modification_date(aetools.NProperty):
	"""shows modification date - Are modification dates displayed in default Finder list view windows? """
	which = 'sdat'
	want = 'bool'
class shows_size(aetools.NProperty):
	"""shows size - Are file sizes displayed in default Finder list view windows? """
	which = 'ssiz'
	want = 'bool'
class shows_version(aetools.NProperty):
	"""shows version - Are file versions displayed in default Finder list view windows? """
	which = 'svrs'
	want = 'bool'
class uses_relative_dates(aetools.NProperty):
	"""uses relative dates - Are relative dates (e.g., today, yesterday) shown  in Finder list view windows? """
	which = 'urdt'
	want = 'bool'
class uses_simple_menus(aetools.NProperty):
	"""uses simple menus - Use simplified Finder menus? """
	which = 'usme'
	want = 'bool'
class uses_wide_grid(aetools.NProperty):
	"""uses wide grid - Space icons on a wide grid? """
	which = 'uswg'
	want = 'bool'
class spring_open_folders(aetools.NProperty):
	"""spring open folders - Spring open folders after the specified delay? """
	which = 'sprg'
	want = 'bool'
class delay_before_springing(aetools.NProperty):
	"""delay before springing - the delay before springing open a container in ticks (1/60th of a second) (12 is shortest delay, 60 is longest delay) """
	which = 'dela'
	want = 'shor'
class view_font(aetools.NProperty):
	"""view font - the id of the font used in Finder views. """
	which = 'vfnt'
	want = 'long'
class view_font_size(aetools.NProperty):
	"""view font size - the size of the font used in Finder views """
	which = 'vfsz'
	want = 'long'
#        element 'clbl' as ['indx', 'name']

class label(aetools.ComponentItem):
	"""label - A Finder label (name and color) """
	want = 'clbl'
class name(aetools.NProperty):
	"""name - the name associated with the label """
	which = 'pnam'
	want = 'itxt'
class index(aetools.NProperty):
	"""index - the index in the front-to-back ordering within its container """
	which = 'pidx'
	want = 'long'
class color(aetools.NProperty):
	"""color - the color associated with the label """
	which = 'colr'
	want = 'cRGB'

class icon_family(aetools.ComponentItem):
	"""icon family - A family of icons """
	want = 'ifam'
class large_monochrome_icon_and_mask(aetools.NProperty):
	"""large monochrome icon and mask - the large black-and-white icon and the mask for large icons """
	which = 'ICN#'
	want = 'ICN#'
class large_8_bit_mask(aetools.NProperty):
	"""large 8 bit mask - the large 8-bit mask for large 32-bit icons """
	which = 'l8mk'
	want = 'l8mk'
class large_32_bit_icon(aetools.NProperty):
	"""large 32 bit icon - the large 32-bit color icon """
	which = 'il32'
	want = 'il32'
class large_8_bit_icon(aetools.NProperty):
	"""large 8 bit icon - the large 8-bit color icon """
	which = 'icl8'
	want = 'icl8'
class large_4_bit_icon(aetools.NProperty):
	"""large 4 bit icon - the large 4-bit color icon """
	which = 'icl4'
	want = 'icl4'
class small_monochrome_icon_and_mask(aetools.NProperty):
	"""small monochrome icon and mask - the small black-and-white icon and the mask for small icons """
	which = 'ics#'
	want = 'ics#'
class small_8_bit_mask(aetools.NProperty):
	"""small 8 bit mask - the small 8-bit mask for small 32-bit icons """
	which = 'ics8'
	want = 's8mk'
class small_32_bit_icon(aetools.NProperty):
	"""small 32 bit icon - the small 32-bit color icon """
	which = 'is32'
	want = 'is32'

small_8_bit_icon = small_8_bit_mask
class small_4_bit_icon(aetools.NProperty):
	"""small 4 bit icon - the small 4-bit color icon """
	which = 'ics4'
	want = 'ics4'

class alias_list(aetools.ComponentItem):
	"""alias list - A list of aliases.  Use ïas alias list’ when a list of aliases is needed (instead of a list of file system item references). """
	want = 'alst'
preferences._propdict = {
	'window' : window,
	'button_view_arrangement' : button_view_arrangement,
	'button_view_icon_size' : button_view_icon_size,
	'spatial_view_arrangement' : spatial_view_arrangement,
	'spatial_view_icon_size' : spatial_view_icon_size,
	'calculates_folder_sizes' : calculates_folder_sizes,
	'list_view_icon_size' : list_view_icon_size,
	'shows_comments' : shows_comments,
	'shows_creation_date' : shows_creation_date,
	'shows_kind' : shows_kind,
	'shows_label' : shows_label,
	'shows_modification_date' : shows_modification_date,
	'shows_size' : shows_size,
	'shows_version' : shows_version,
	'uses_relative_dates' : uses_relative_dates,
	'uses_simple_menus' : uses_simple_menus,
	'uses_wide_grid' : uses_wide_grid,
	'spring_open_folders' : spring_open_folders,
	'delay_before_springing' : delay_before_springing,
	'view_font' : view_font,
	'view_font_size' : view_font_size,
}
preferences._elemdict = {
	'label' : label,
}
label._propdict = {
	'name' : name,
	'index' : index,
	'color' : color,
}
label._elemdict = {
}
icon_family._propdict = {
	'large_monochrome_icon_and_mask' : large_monochrome_icon_and_mask,
	'large_8_bit_mask' : large_8_bit_mask,
	'large_32_bit_icon' : large_32_bit_icon,
	'large_8_bit_icon' : large_8_bit_icon,
	'large_4_bit_icon' : large_4_bit_icon,
	'small_monochrome_icon_and_mask' : small_monochrome_icon_and_mask,
	'small_8_bit_mask' : small_8_bit_mask,
	'small_32_bit_icon' : small_32_bit_icon,
	'small_8_bit_icon' : small_8_bit_icon,
	'small_4_bit_icon' : small_4_bit_icon,
}
icon_family._elemdict = {
}
alias_list._propdict = {
}
alias_list._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'clbl' : label,
	'ifam' : icon_family,
	'alst' : alias_list,
	'cprf' : preferences,
}

_propdeclarations = {
	'ics#' : small_monochrome_icon_and_mask,
	'scda' : shows_creation_date,
	'uswg' : uses_wide_grid,
	'sprg' : spring_open_folders,
	'is32' : small_32_bit_icon,
	'ICN#' : large_monochrome_icon_and_mask,
	'cwin' : window,
	'sdat' : shows_modification_date,
	'iisz' : spatial_view_icon_size,
	'barr' : button_view_arrangement,
	'il32' : large_32_bit_icon,
	'l8mk' : large_8_bit_mask,
	'scom' : shows_comments,
	'bisz' : button_view_icon_size,
	'lisz' : list_view_icon_size,
	'slbl' : shows_label,
	'icl4' : large_4_bit_icon,
	'usme' : uses_simple_menus,
	'urdt' : uses_relative_dates,
	'vfnt' : view_font,
	'sfsz' : calculates_folder_sizes,
	'pidx' : index,
	'icl8' : large_8_bit_icon,
	'ssiz' : shows_size,
	'ics8' : small_8_bit_mask,
	'colr' : color,
	'svrs' : shows_version,
	'pnam' : name,
	'sknd' : shows_kind,
	'vfsz' : view_font_size,
	'iarr' : spatial_view_arrangement,
	'ics4' : small_4_bit_icon,
	'dela' : delay_before_springing,
}

_compdeclarations = {
}

_enumdeclarations = {
}
