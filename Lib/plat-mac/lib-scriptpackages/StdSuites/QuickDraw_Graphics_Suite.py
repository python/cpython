"""Suite QuickDraw Graphics Suite: A set of basic classes for graphics
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Extensies/AppleScript
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
class default_font(aetools.NProperty):
	"""default font - the name of the default font for text objects """
	which = 'ptxf'
	want = 'itxt'
class default_location(aetools.NProperty):
	"""default location - the default location of each new graphic object """
	which = 'pnel'
	want = 'QDpt'
class default_size(aetools.NProperty):
	"""default size - the default size for text objects """
	which = 'ptps'
	want = 'fixd'
class name(aetools.NProperty):
	"""name - the name """
	which = 'pnam'
	want = 'itxt'
class ordering(aetools.NProperty):
	"""ordering - the ordered list of graphic objects in the drawing area """
	which = 'gobs'
	want = 'obj '
class pixel_depth(aetools.NProperty):
	"""pixel depth - the number of bits per pixel """
	which = 'pdpt'
	want = 'shor'
class style(aetools.NProperty):
	"""style - the default text style for text objects """
	which = 'txst'
	want = 'tsty'
class text_color(aetools.NProperty):
	"""text color - the default color for text objects """
	which = 'ptxc'
	want = 'cRGB'
class update_on_change(aetools.NProperty):
	"""update on change - Redraw after each change? """
	which = 'pupd'
	want = 'bool'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language of text objects in the drawing area """
	which = 'psct'
	want = 'intl'

drawing_areas = drawing_area

class graphic_objects(aetools.ComponentItem):
	"""graphic objects -  """
	want = 'cgob'

graphic_object = graphic_objects

class graphic_shapes(aetools.ComponentItem):
	"""graphic shapes -  """
	want = 'cgsh'

graphic_shape = graphic_shapes

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

class ovals(aetools.ComponentItem):
	"""ovals -  """
	want = 'covl'

oval = ovals

class polygon(aetools.ComponentItem):
	"""polygon - A polygon """
	want = 'cpgn'
class point_list(aetools.NProperty):
	"""point list - the list of points that define the polygon """
	which = 'ptlt'
	want = 'QDpt'

polygons = polygon

class graphic_groups(aetools.ComponentItem):
	"""graphic groups -  """
	want = 'cpic'

graphic_group = graphic_groups

class pixel_maps(aetools.ComponentItem):
	"""pixel maps -  """
	want = 'cpix'

pixel_map = pixel_maps

class pixel(aetools.ComponentItem):
	"""pixel - A pixel """
	want = 'cpxl'

pixels = pixel

class rectangles(aetools.ComponentItem):
	"""rectangles -  """
	want = 'crec'

rectangle = rectangles

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

class graphic_line(aetools.ComponentItem):
	"""graphic line - A graphic line """
	want = 'glin'
class arrow_style(aetools.NProperty):
	"""arrow style - the arrow style """
	which = 'arro'
	want = 'arro'
class dash_style(aetools.NProperty):
	"""dash style - the dash style """
	which = 'pdst'
	want = 'tdas'
class end_point(aetools.NProperty):
	"""end point - the ending point of the line """
	which = 'pend'
	want = 'QDpt'
class start_point(aetools.NProperty):
	"""start point - the starting point of the line """
	which = 'pstp'
	want = 'QDpt'

graphic_lines = graphic_line
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
	'default_font' : default_font,
	'default_location' : default_location,
	'default_size' : default_size,
	'name' : name,
	'ordering' : ordering,
	'pixel_depth' : pixel_depth,
	'style' : style,
	'text_color' : text_color,
	'update_on_change' : update_on_change,
	'writing_code' : writing_code,
}
drawing_area._privelemdict = {
}
graphic_objects._superclassnames = []
graphic_objects._privpropdict = {
}
graphic_objects._privelemdict = {
}
graphic_shapes._superclassnames = []
graphic_shapes._privpropdict = {
}
graphic_shapes._privelemdict = {
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
ovals._superclassnames = []
ovals._privpropdict = {
}
ovals._privelemdict = {
}
polygon._superclassnames = []
polygon._privpropdict = {
	'point_list' : point_list,
}
polygon._privelemdict = {
}
graphic_groups._superclassnames = []
graphic_groups._privpropdict = {
}
graphic_groups._privelemdict = {
}
pixel_maps._superclassnames = []
pixel_maps._privpropdict = {
}
pixel_maps._privelemdict = {
}
pixel._superclassnames = []
pixel._privpropdict = {
	'color' : color,
}
pixel._privelemdict = {
}
rectangles._superclassnames = []
rectangles._privpropdict = {
}
rectangles._privelemdict = {
}
rounded_rectangle._superclassnames = []
rounded_rectangle._privpropdict = {
	'corner_curve_height' : corner_curve_height,
	'corner_curve_width' : corner_curve_width,
}
rounded_rectangle._privelemdict = {
}
graphic_line._superclassnames = []
graphic_line._privpropdict = {
	'arrow_style' : arrow_style,
	'dash_style' : dash_style,
	'end_point' : end_point,
	'start_point' : start_point,
}
graphic_line._privelemdict = {
}
_Enum_arro = {
	'no_arrow' : 'arno',	# No arrow on line
	'arrow_at_start' : 'arst',	# Arrow at start of line
	'arrow_at_end' : 'aren',	# Arrow at end of line
	'arrow_at_both_ends' : 'arbo',	# Arrow at both the start and the end of the line
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


#
# Indices of types declared in this module
#
_classdeclarations = {
	'carc' : arc,
	'cdrw' : drawing_area,
	'cgob' : graphic_objects,
	'cgsh' : graphic_shapes,
	'cgtx' : graphic_text,
	'covl' : ovals,
	'cpgn' : polygon,
	'cpic' : graphic_groups,
	'cpix' : pixel_maps,
	'cpxl' : pixel,
	'crec' : rectangles,
	'crrc' : rounded_rectangle,
	'glin' : graphic_line,
}

_propdeclarations = {
	'arro' : arrow_style,
	'cltb' : color_table,
	'colr' : color,
	'flcl' : fill_color,
	'flpt' : fill_pattern,
	'font' : font,
	'gobs' : ordering,
	'pang' : start_angle,
	'parc' : arc_angle,
	'pbcl' : background_color,
	'pbnd' : bounds,
	'pbpt' : background_pattern,
	'pchd' : corner_curve_height,
	'pcwd' : corner_curve_width,
	'pdpt' : pixel_depth,
	'pdrt' : definition_rect,
	'pdst' : dash_style,
	'pend' : end_point,
	'pnam' : name,
	'pnel' : default_location,
	'ppcl' : pen_color,
	'pppa' : pen_pattern,
	'pptm' : transfer_mode,
	'ppwd' : pen_width,
	'psct' : writing_code,
	'pstp' : start_point,
	'ptlt' : point_list,
	'ptps' : default_size,
	'ptsz' : size,
	'ptxc' : text_color,
	'ptxf' : default_font,
	'pupd' : update_on_change,
	'txst' : style,
	'ustl' : uniform_styles,
}

_compdeclarations = {
}

_enumdeclarations = {
	'arro' : _Enum_arro,
	'tran' : _Enum_tran,
}
