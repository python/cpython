"""Suite QuickDraw Graphics Suite: A set of basic classes for graphics
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'qdrw'

class QuickDraw_Graphics_Suite_Events:

	pass


class arc(aetools.ComponentItem):
	"""arc - An arc """
	want = 'carc'
class arc_angle(aetools.NProperty):
	"""arc angle - the angle of the arc in degrees """
	which = 'parc'
	want = 'fixd'
class bounds(aetools.NProperty):
	"""bounds - the smallest rectangle that contains the entire arc """
	which = 'pbnd'
	want = 'qdrt'
class definition_rect(aetools.NProperty):
	"""definition rect - the rectangle that contains the circle or oval used to define the arc """
	which = 'pdrt'
	want = 'qdrt'
class fill_color(aetools.NProperty):
	"""fill color - the fill color """
	which = 'flcl'
	want = 'cRGB'
class fill_pattern(aetools.NProperty):
	"""fill pattern - the fill pattern """
	which = 'flpt'
	want = 'cpix'
class pen_color(aetools.NProperty):
	"""pen color - the pen color """
	which = 'ppcl'
	want = 'cRGB'
class pen_pattern(aetools.NProperty):
	"""pen pattern - the pen pattern """
	which = 'pppa'
	want = 'cpix'
class pen_width(aetools.NProperty):
	"""pen width - the pen width """
	which = 'ppwd'
	want = 'shor'
class start_angle(aetools.NProperty):
	"""start angle - the angle that defines the start of the arc, in degrees """
	which = 'pang'
	want = 'fixd'
class transfer_mode(aetools.NProperty):
	"""transfer mode - the transfer mode """
	which = 'pptm'
	want = 'tran'

arcs = arc

class drawing_area(aetools.ComponentItem):
	"""drawing area - Container for graphics and supporting information """
	want = 'cdrw'
class background_color(aetools.NProperty):
	"""background color - the color used to fill in unoccupied areas """
	which = 'pbcl'
	want = 'cRGB'
class background_pattern(aetools.NProperty):
	"""background pattern - the pattern used to fill in unoccupied areas """
	which = 'pbpt'
	want = 'cpix'
class color_table(aetools.NProperty):
	"""color table - the color table """
	which = 'cltb'
	want = 'clrt'
class ordering(aetools.NProperty):
	"""ordering - the ordered list of graphic objects in the drawing area """
	which = 'gobs'
	want = 'obj '
class name(aetools.NProperty):
	"""name - the name """
	which = 'pnam'
	want = 'itxt'
class default_location(aetools.NProperty):
	"""default location - the default location of each new graphic object """
	which = 'pnel'
	want = 'QDpt'
class pixel_depth(aetools.NProperty):
	"""pixel depth - the number of bits per pixel """
	which = 'pdpt'
	want = 'shor'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language of text objects in the drawing area """
	which = 'psct'
	want = 'intl'
class text_color(aetools.NProperty):
	"""text color - the default color for text objects """
	which = 'ptxc'
	want = 'cRGB'
class default_font(aetools.NProperty):
	"""default font - the name of the default font for text objects """
	which = 'ptxf'
	want = 'itxt'
class default_size(aetools.NProperty):
	"""default size - the default size for text objects """
	which = 'ptps'
	want = 'fixd'
class style(aetools.NProperty):
	"""style - the default text style for text objects """
	which = 'txst'
	want = 'tsty'
class update_on_change(aetools.NProperty):
	"""update on change - Redraw after each change? """
	which = 'pupd'
	want = 'bool'

drawing_areas = drawing_area

class graphic_line(aetools.ComponentItem):
	"""graphic line - A graphic line """
	want = 'glin'
class start_point(aetools.NProperty):
	"""start point - the starting point of the line """
	which = 'pstp'
	want = 'QDpt'
class end_point(aetools.NProperty):
	"""end point - the ending point of the line """
	which = 'pend'
	want = 'QDpt'
class dash_style(aetools.NProperty):
	"""dash style - the dash style """
	which = 'pdst'
	want = 'tdas'
class arrow_style(aetools.NProperty):
	"""arrow style - the arrow style """
	which = 'arro'
	want = 'arro'

graphic_lines = graphic_line

class graphic_object(aetools.ComponentItem):
	"""graphic object - A graphic object """
	want = 'cgob'

graphic_objects = graphic_object

class graphic_shape(aetools.ComponentItem):
	"""graphic shape - A graphic shape """
	want = 'cgsh'

graphic_shapes = graphic_shape

class graphic_text(aetools.ComponentItem):
	"""graphic text - A series of characters within a drawing area """
	want = 'cgtx'
class color(aetools.NProperty):
	"""color - the color of the first character """
	which = 'colr'
	want = 'cRGB'
class font(aetools.NProperty):
	"""font - the name of the font of the first character """
	which = 'font'
	want = 'ctxt'
class size(aetools.NProperty):
	"""size - the size in points of the first character """
	which = 'ptsz'
	want = 'fixd'
class uniform_styles(aetools.NProperty):
	"""uniform styles - the text styles that are uniform throughout the text """
	which = 'ustl'
	want = 'tsty'

class graphic_group(aetools.ComponentItem):
	"""graphic group - Group of graphics """
	want = 'cpic'

graphic_groups = graphic_group

class oval(aetools.ComponentItem):
	"""oval - An oval """
	want = 'covl'

ovals = oval

class pixel(aetools.ComponentItem):
	"""pixel - A pixel """
	want = 'cpxl'

pixels = pixel

class pixel_map(aetools.ComponentItem):
	"""pixel map - A pixel map """
	want = 'cpix'

pixel_maps = pixel_map

class polygon(aetools.ComponentItem):
	"""polygon - A polygon """
	want = 'cpgn'
class point_list(aetools.NProperty):
	"""point list - the list of points that define the polygon """
	which = 'ptlt'
	want = 'QDpt'

polygons = polygon

class rectangle(aetools.ComponentItem):
	"""rectangle - A rectangle """
	want = 'crec'

rectangles = rectangle

class rounded_rectangle(aetools.ComponentItem):
	"""rounded rectangle - A rounded rectangle """
	want = 'crrc'
class corner_curve_height(aetools.NProperty):
	"""corner curve height - the height of the oval used to define the shape of the rounded corners """
	which = 'pchd'
	want = 'shor'
class corner_curve_width(aetools.NProperty):
	"""corner curve width - the width of the oval used to define the shape of the rounded corners """
	which = 'pcwd'
	want = 'shor'

rounded_rectangles = rounded_rectangle
arc._superclassnames = []
arc._privpropdict = {
	'arc_angle' : arc_angle,
	'bounds' : bounds,
	'definition_rect' : definition_rect,
	'fill_color' : fill_color,
	'fill_pattern' : fill_pattern,
	'pen_color' : pen_color,
	'pen_pattern' : pen_pattern,
	'pen_width' : pen_width,
	'start_angle' : start_angle,
	'transfer_mode' : transfer_mode,
}
arc._privelemdict = {
}
drawing_area._superclassnames = []
drawing_area._privpropdict = {
	'background_color' : background_color,
	'background_pattern' : background_pattern,
	'color_table' : color_table,
	'ordering' : ordering,
	'name' : name,
	'default_location' : default_location,
	'pixel_depth' : pixel_depth,
	'writing_code' : writing_code,
	'text_color' : text_color,
	'default_font' : default_font,
	'default_size' : default_size,
	'style' : style,
	'update_on_change' : update_on_change,
}
drawing_area._privelemdict = {
}
graphic_line._superclassnames = []
graphic_line._privpropdict = {
	'start_point' : start_point,
	'end_point' : end_point,
	'dash_style' : dash_style,
	'arrow_style' : arrow_style,
}
graphic_line._privelemdict = {
}
graphic_object._superclassnames = []
graphic_object._privpropdict = {
}
graphic_object._privelemdict = {
}
graphic_shape._superclassnames = []
graphic_shape._privpropdict = {
}
graphic_shape._privelemdict = {
}
graphic_text._superclassnames = []
graphic_text._privpropdict = {
	'color' : color,
	'font' : font,
	'size' : size,
	'uniform_styles' : uniform_styles,
}
graphic_text._privelemdict = {
}
graphic_group._superclassnames = []
graphic_group._privpropdict = {
}
graphic_group._privelemdict = {
}
oval._superclassnames = []
oval._privpropdict = {
}
oval._privelemdict = {
}
pixel._superclassnames = []
pixel._privpropdict = {
	'color' : color,
}
pixel._privelemdict = {
}
pixel_map._superclassnames = []
pixel_map._privpropdict = {
}
pixel_map._privelemdict = {
}
polygon._superclassnames = []
polygon._privpropdict = {
	'point_list' : point_list,
}
polygon._privelemdict = {
}
rectangle._superclassnames = []
rectangle._privpropdict = {
}
rectangle._privelemdict = {
}
rounded_rectangle._superclassnames = []
rounded_rectangle._privpropdict = {
	'corner_curve_height' : corner_curve_height,
	'corner_curve_width' : corner_curve_width,
}
rounded_rectangle._privelemdict = {
}
_Enum_tran = {
	'copy_pixels' : 'cpy ',	# 
	'not_copy_pixels' : 'ncpy',	# 
	'or_pixels' : 'or  ',	# 
	'not_or_pixels' : 'ntor',	# 
	'bic_pixels' : 'bic ',	# 
	'not_bic_pixels' : 'nbic',	# 
	'xor_pixels' : 'xor ',	# 
	'not_xor_pixels' : 'nxor',	# 
	'add_over_pixels' : 'addo',	# 
	'add_pin_pixels' : 'addp',	# 
	'sub_over_pixels' : 'subo',	# 
	'sub_pin_pixels' : 'subp',	# 
	'ad_max_pixels' : 'admx',	# 
	'ad_min_pixels' : 'admn',	# 
	'blend_pixels' : 'blnd',	# 
}

_Enum_arro = {
	'no_arrow' : 'arno',	# No arrow on line
	'arrow_at_start' : 'arst',	# Arrow at start of line
	'arrow_at_end' : 'aren',	# Arrow at end of line
	'arrow_at_both_ends' : 'arbo',	# Arrow at both the start and the end of the line
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'cpic' : graphic_group,
	'covl' : oval,
	'cgtx' : graphic_text,
	'cgsh' : graphic_shape,
	'glin' : graphic_line,
	'cgob' : graphic_object,
	'cdrw' : drawing_area,
	'cpgn' : polygon,
	'cpxl' : pixel,
	'crrc' : rounded_rectangle,
	'carc' : arc,
	'cpix' : pixel_map,
	'crec' : rectangle,
}

_propdeclarations = {
	'pbpt' : background_pattern,
	'flcl' : fill_color,
	'parc' : arc_angle,
	'pbnd' : bounds,
	'colr' : color,
	'flpt' : fill_pattern,
	'ustl' : uniform_styles,
	'font' : font,
	'pend' : end_point,
	'pstp' : start_point,
	'pang' : start_angle,
	'pptm' : transfer_mode,
	'cltb' : color_table,
	'ptxc' : text_color,
	'ptxf' : default_font,
	'ppcl' : pen_color,
	'ptps' : default_size,
	'ppwd' : pen_width,
	'arro' : arrow_style,
	'pcwd' : corner_curve_width,
	'txst' : style,
	'psct' : writing_code,
	'pdst' : dash_style,
	'ptlt' : point_list,
	'gobs' : ordering,
	'pdpt' : pixel_depth,
	'pnel' : default_location,
	'pchd' : corner_curve_height,
	'pbcl' : background_color,
	'pnam' : name,
	'pdrt' : definition_rect,
	'ptsz' : size,
	'pupd' : update_on_change,
	'pppa' : pen_pattern,
}

_compdeclarations = {
}

_enumdeclarations = {
	'arro' : _Enum_arro,
	'tran' : _Enum_tran,
}
