"""Suite QuickDraw Graphics Suite: A set of basic classes for graphics
Level 1, version 1

Generated from flap:System Folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'qdrw'

class QuickDraw_Graphics_Suite:

	pass


class arc(aetools.ComponentItem):
	"""arc - An arc"""
	want = 'carc'
class arc_angle(aetools.NProperty):
	"""arc angle - Angle of the arc in degrees"""
	which = 'parc'
	want = 'fixd'
class bounds(aetools.NProperty):
	"""bounds - the smallest rectangle that contains the entire arc"""
	which = 'pbnd'
	want = 'qdrt'
class _class(aetools.NProperty):
	"""class - the class"""
	which = 'pcls'
	want = 'type'
class definition_rect(aetools.NProperty):
	"""definition rect - the rectangle that contains the circle or oval used to define the arc"""
	which = 'pdrt'
	want = 'qdrt'
class fill_color(aetools.NProperty):
	"""fill color - the fill color"""
	which = 'flcl'
	want = 'cRGB'
class fill_pattern(aetools.NProperty):
	"""fill pattern - the fill pattern"""
	which = 'flpt'
	want = 'cpix'
class pen_color(aetools.NProperty):
	"""pen color - the pen color"""
	which = 'ppcl'
	want = 'cRGB'
class pen_pattern(aetools.NProperty):
	"""pen pattern - the pen pattern"""
	which = 'pppa'
	want = 'cpix'
class pen_width(aetools.NProperty):
	"""pen width - the pen width"""
	which = 'ppwd'
	want = 'shor'
class start_angle(aetools.NProperty):
	"""start angle - the angle that defines the start of the arc, in degrees"""
	which = 'pang'
	want = 'fixd'
class transfer_mode(aetools.NProperty):
	"""transfer mode - the transfer mode"""
	which = 'pptm'
	want = 'tran'

arcs = arc

class drawing_area(aetools.ComponentItem):
	"""drawing area - Container for graphics and supporting information"""
	want = 'cdrw'
class background_color(aetools.NProperty):
	"""background color - the color used to fill in unoccupied areas"""
	which = 'pbcl'
	want = 'cRGB'
class background_pattern(aetools.NProperty):
	"""background pattern - the pattern used to fill in unoccupied areas"""
	which = 'pbpt'
	want = 'cpix'
class color_table(aetools.NProperty):
	"""color table - the color table"""
	which = 'cltb'
	want = 'clrt'
class ordering(aetools.NProperty):
	"""ordering - the ordered list of graphic objects in the drawing area"""
	which = 'gobs'
	want = 'obj '
class name(aetools.NProperty):
	"""name - the name"""
	which = 'pnam'
	want = 'itxt'
class default_location(aetools.NProperty):
	"""default location - the default location of each new graphic object"""
	which = 'pnel'
	want = 'QDpt'
class pixel_depth(aetools.NProperty):
	"""pixel depth - Bits per pixel"""
	which = 'pdpt'
	want = 'shor'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language of text objects in the drawing area"""
	which = 'psct'
	want = 'intl'
class text_color(aetools.NProperty):
	"""text color - the default color for text objects"""
	which = 'ptxc'
	want = 'cRGB'
class default_font(aetools.NProperty):
	"""default font - the name of the default font for text objects"""
	which = 'ptxf'
	want = 'itxt'
class default_size(aetools.NProperty):
	"""default size - the default size for text objects"""
	which = 'ptps'
	want = 'fixd'
class style(aetools.NProperty):
	"""style - the default text style for text objects"""
	which = 'txst'
	want = 'tsty'
class update_on_change(aetools.NProperty):
	"""update on change - Redraw after each change?"""
	which = 'pupd'
	want = 'bool'

drawing_areas = drawing_area

class graphic_line(aetools.ComponentItem):
	"""graphic line - A graphic line"""
	want = 'glin'
class dash_style(aetools.NProperty):
	"""dash style - the dash style"""
	which = 'pdst'
	want = 'tdas'
class end_point(aetools.NProperty):
	"""end point - the ending point of the line"""
	which = 'pend'
	want = 'QDpt'
class arrow_style(aetools.NProperty):
	"""arrow style - the arrow style"""
	which = 'arro'
	want = 'arro'
class start_point(aetools.NProperty):
	"""start point - the starting point of the line"""
	which = 'pstp'
	want = 'QDpt'

graphic_lines = graphic_line

class graphic_object(aetools.ComponentItem):
	"""graphic object - A graphic object"""
	want = 'cgob'

graphic_objects = graphic_object

class graphic_shape(aetools.ComponentItem):
	"""graphic shape - A graphic shape"""
	want = 'cgsh'

graphic_shapes = graphic_shape

class graphic_text(aetools.ComponentItem):
	"""graphic text - A series of characters within a drawing area"""
	want = 'cgtx'
class color(aetools.NProperty):
	"""color - the color of the first character"""
	which = 'colr'
	want = 'cRGB'
class font(aetools.NProperty):
	"""font - the name of the font of the first character"""
	which = 'font'
	want = 'ctxt'
class size(aetools.NProperty):
	"""size - the size in points of the first character"""
	which = 'ptsz'
	want = 'fixd'
class uniform_styles(aetools.NProperty):
	"""uniform styles - the text styles that are uniform throughout the text"""
	which = 'ustl'
	want = 'tsty'

class graphic_group(aetools.ComponentItem):
	"""graphic group - Group of graphics"""
	want = 'cpic'

graphic_groups = graphic_group

class oval(aetools.ComponentItem):
	"""oval - An oval"""
	want = 'covl'

ovals = oval

class pixel(aetools.ComponentItem):
	"""pixel - A pixel"""
	want = 'cpxl'
# repeated property color the color

pixels = pixel

class pixel_map(aetools.ComponentItem):
	"""pixel map - A pixel map"""
	want = 'cpix'

pixel_maps = pixel_map

class polygon(aetools.ComponentItem):
	"""polygon - A polygon"""
	want = 'cpgn'
class point_list(aetools.NProperty):
	"""point list - the list of points that define the polygon"""
	which = 'ptlt'
	want = 'QDpt'

polygons = polygon

class rectangle(aetools.ComponentItem):
	"""rectangle - A rectangle"""
	want = 'crec'

rectangles = rectangle

class rounded_rectangle(aetools.ComponentItem):
	"""rounded rectangle - A rounded rectangle"""
	want = 'crrc'
class corner_curve_height(aetools.NProperty):
	"""corner curve height - the height of the oval used to define the shape of the rounded corners"""
	which = 'pchd'
	want = 'shor'
class corner_curve_width(aetools.NProperty):
	"""corner curve width - the width of the oval used to define the shape of the rounded corners"""
	which = 'pcwd'
	want = 'shor'

rounded_rectangles = rounded_rectangle
arc._propdict = {
	'arc_angle' : arc_angle,
	'bounds' : bounds,
	'_class' : _class,
	'definition_rect' : definition_rect,
	'fill_color' : fill_color,
	'fill_pattern' : fill_pattern,
	'pen_color' : pen_color,
	'pen_pattern' : pen_pattern,
	'pen_width' : pen_width,
	'start_angle' : start_angle,
	'transfer_mode' : transfer_mode,
}
arc._elemdict = {
}
drawing_area._propdict = {
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
drawing_area._elemdict = {
}
graphic_line._propdict = {
	'dash_style' : dash_style,
	'end_point' : end_point,
	'arrow_style' : arrow_style,
	'start_point' : start_point,
}
graphic_line._elemdict = {
}
graphic_object._propdict = {
}
graphic_object._elemdict = {
}
graphic_shape._propdict = {
}
graphic_shape._elemdict = {
}
graphic_text._propdict = {
	'color' : color,
	'font' : font,
	'size' : size,
	'uniform_styles' : uniform_styles,
}
graphic_text._elemdict = {
}
graphic_group._propdict = {
}
graphic_group._elemdict = {
}
oval._propdict = {
}
oval._elemdict = {
}
pixel._propdict = {
	'color' : color,
}
pixel._elemdict = {
}
pixel_map._propdict = {
}
pixel_map._elemdict = {
}
polygon._propdict = {
	'point_list' : point_list,
}
polygon._elemdict = {
}
rectangle._propdict = {
}
rectangle._elemdict = {
}
rounded_rectangle._propdict = {
	'corner_curve_height' : corner_curve_height,
	'corner_curve_width' : corner_curve_width,
}
rounded_rectangle._elemdict = {
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
	'crec' : rectangle,
	'cpix' : pixel_map,
	'carc' : arc,
	'cgsh' : graphic_shape,
	'cpxl' : pixel,
	'crrc' : rounded_rectangle,
	'cpgn' : polygon,
	'cdrw' : drawing_area,
	'cgob' : graphic_object,
	'glin' : graphic_line,
	'cgtx' : graphic_text,
	'covl' : oval,
	'cpic' : graphic_group,
}

_propdeclarations = {
	'pend' : end_point,
	'pupd' : update_on_change,
	'pstp' : start_point,
	'pdrt' : definition_rect,
	'pnam' : name,
	'pbcl' : background_color,
	'pptm' : transfer_mode,
	'pnel' : default_location,
	'pdpt' : pixel_depth,
	'gobs' : ordering,
	'ustl' : uniform_styles,
	'ptlt' : point_list,
	'pdst' : dash_style,
	'psct' : writing_code,
	'txst' : style,
	'font' : font,
	'pchd' : corner_curve_height,
	'pcls' : _class,
	'ppwd' : pen_width,
	'ptps' : default_size,
	'ppcl' : pen_color,
	'ptxf' : default_font,
	'pcwd' : corner_curve_width,
	'ptxc' : text_color,
	'cltb' : color_table,
	'pppa' : pen_pattern,
	'pang' : start_angle,
	'flpt' : fill_pattern,
	'colr' : color,
	'arro' : arrow_style,
	'pbnd' : bounds,
	'ptsz' : size,
	'parc' : arc_angle,
	'flcl' : fill_color,
	'pbpt' : background_pattern,
}

_compdeclarations = {
}

_enumdeclarations = {
	'tran' : _Enum_tran,
	'arro' : _Enum_arro,
}
