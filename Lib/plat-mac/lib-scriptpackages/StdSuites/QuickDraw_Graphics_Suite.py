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
class _Prop_arc_angle(aetools.NProperty):
    """arc angle - the angle of the arc in degrees """
    which = 'parc'
    want = 'fixd'
class _Prop_bounds(aetools.NProperty):
    """bounds - the smallest rectangle that contains the entire arc """
    which = 'pbnd'
    want = 'qdrt'
class _Prop_definition_rect(aetools.NProperty):
    """definition rect - the rectangle that contains the circle or oval used to define the arc """
    which = 'pdrt'
    want = 'qdrt'
class _Prop_fill_color(aetools.NProperty):
    """fill color - the fill color """
    which = 'flcl'
    want = 'cRGB'
class _Prop_fill_pattern(aetools.NProperty):
    """fill pattern - the fill pattern """
    which = 'flpt'
    want = 'cpix'
class _Prop_pen_color(aetools.NProperty):
    """pen color - the pen color """
    which = 'ppcl'
    want = 'cRGB'
class _Prop_pen_pattern(aetools.NProperty):
    """pen pattern - the pen pattern """
    which = 'pppa'
    want = 'cpix'
class _Prop_pen_width(aetools.NProperty):
    """pen width - the pen width """
    which = 'ppwd'
    want = 'shor'
class _Prop_start_angle(aetools.NProperty):
    """start angle - the angle that defines the start of the arc, in degrees """
    which = 'pang'
    want = 'fixd'
class _Prop_transfer_mode(aetools.NProperty):
    """transfer mode - the transfer mode """
    which = 'pptm'
    want = 'tran'

arcs = arc

class drawing_area(aetools.ComponentItem):
    """drawing area - Container for graphics and supporting information """
    want = 'cdrw'
class _Prop_background_color(aetools.NProperty):
    """background color - the color used to fill in unoccupied areas """
    which = 'pbcl'
    want = 'cRGB'
class _Prop_background_pattern(aetools.NProperty):
    """background pattern - the pattern used to fill in unoccupied areas """
    which = 'pbpt'
    want = 'cpix'
class _Prop_color_table(aetools.NProperty):
    """color table - the color table """
    which = 'cltb'
    want = 'clrt'
class _Prop_default_font(aetools.NProperty):
    """default font - the name of the default font for text objects """
    which = 'ptxf'
    want = 'itxt'
class _Prop_default_location(aetools.NProperty):
    """default location - the default location of each new graphic object """
    which = 'pnel'
    want = 'QDpt'
class _Prop_default_size(aetools.NProperty):
    """default size - the default size for text objects """
    which = 'ptps'
    want = 'fixd'
class _Prop_name(aetools.NProperty):
    """name - the name """
    which = 'pnam'
    want = 'itxt'
class _Prop_ordering(aetools.NProperty):
    """ordering - the ordered list of graphic objects in the drawing area """
    which = 'gobs'
    want = 'obj '
class _Prop_pixel_depth(aetools.NProperty):
    """pixel depth - the number of bits per pixel """
    which = 'pdpt'
    want = 'shor'
class _Prop_style(aetools.NProperty):
    """style - the default text style for text objects """
    which = 'txst'
    want = 'tsty'
class _Prop_text_color(aetools.NProperty):
    """text color - the default color for text objects """
    which = 'ptxc'
    want = 'cRGB'
class _Prop_update_on_change(aetools.NProperty):
    """update on change - Redraw after each change? """
    which = 'pupd'
    want = 'bool'
class _Prop_writing_code(aetools.NProperty):
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
class _Prop_color(aetools.NProperty):
    """color - the color of the first character """
    which = 'colr'
    want = 'cRGB'
class _Prop_font(aetools.NProperty):
    """font - the name of the font of the first character """
    which = 'font'
    want = 'ctxt'
class _Prop_size(aetools.NProperty):
    """size - the size in points of the first character """
    which = 'ptsz'
    want = 'fixd'
class _Prop_uniform_styles(aetools.NProperty):
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
class _Prop_point_list(aetools.NProperty):
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
class _Prop_corner_curve_height(aetools.NProperty):
    """corner curve height - the height of the oval used to define the shape of the rounded corners """
    which = 'pchd'
    want = 'shor'
class _Prop_corner_curve_width(aetools.NProperty):
    """corner curve width - the width of the oval used to define the shape of the rounded corners """
    which = 'pcwd'
    want = 'shor'

rounded_rectangles = rounded_rectangle

class graphic_line(aetools.ComponentItem):
    """graphic line - A graphic line """
    want = 'glin'
class _Prop_arrow_style(aetools.NProperty):
    """arrow style - the arrow style """
    which = 'arro'
    want = 'arro'
class _Prop_dash_style(aetools.NProperty):
    """dash style - the dash style """
    which = 'pdst'
    want = 'tdas'
class _Prop_end_point(aetools.NProperty):
    """end point - the ending point of the line """
    which = 'pend'
    want = 'QDpt'
class _Prop_start_point(aetools.NProperty):
    """start point - the starting point of the line """
    which = 'pstp'
    want = 'QDpt'

graphic_lines = graphic_line
arc._superclassnames = []
arc._privpropdict = {
    'arc_angle' : _Prop_arc_angle,
    'bounds' : _Prop_bounds,
    'definition_rect' : _Prop_definition_rect,
    'fill_color' : _Prop_fill_color,
    'fill_pattern' : _Prop_fill_pattern,
    'pen_color' : _Prop_pen_color,
    'pen_pattern' : _Prop_pen_pattern,
    'pen_width' : _Prop_pen_width,
    'start_angle' : _Prop_start_angle,
    'transfer_mode' : _Prop_transfer_mode,
}
arc._privelemdict = {
}
drawing_area._superclassnames = []
drawing_area._privpropdict = {
    'background_color' : _Prop_background_color,
    'background_pattern' : _Prop_background_pattern,
    'color_table' : _Prop_color_table,
    'default_font' : _Prop_default_font,
    'default_location' : _Prop_default_location,
    'default_size' : _Prop_default_size,
    'name' : _Prop_name,
    'ordering' : _Prop_ordering,
    'pixel_depth' : _Prop_pixel_depth,
    'style' : _Prop_style,
    'text_color' : _Prop_text_color,
    'update_on_change' : _Prop_update_on_change,
    'writing_code' : _Prop_writing_code,
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
    'color' : _Prop_color,
    'font' : _Prop_font,
    'size' : _Prop_size,
    'uniform_styles' : _Prop_uniform_styles,
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
    'point_list' : _Prop_point_list,
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
    'color' : _Prop_color,
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
    'corner_curve_height' : _Prop_corner_curve_height,
    'corner_curve_width' : _Prop_corner_curve_width,
}
rounded_rectangle._privelemdict = {
}
graphic_line._superclassnames = []
graphic_line._privpropdict = {
    'arrow_style' : _Prop_arrow_style,
    'dash_style' : _Prop_dash_style,
    'end_point' : _Prop_end_point,
    'start_point' : _Prop_start_point,
}
graphic_line._privelemdict = {
}
_Enum_arro = {
    'no_arrow' : 'arno',        # No arrow on line
    'arrow_at_start' : 'arst',  # Arrow at start of line
    'arrow_at_end' : 'aren',    # Arrow at end of line
    'arrow_at_both_ends' : 'arbo',      # Arrow at both the start and the end of the line
}

_Enum_tran = {
    'copy_pixels' : 'cpy ',     #
    'not_copy_pixels' : 'ncpy', #
    'or_pixels' : 'or  ',       #
    'not_or_pixels' : 'ntor',   #
    'bic_pixels' : 'bic ',      #
    'not_bic_pixels' : 'nbic',  #
    'xor_pixels' : 'xor ',      #
    'not_xor_pixels' : 'nxor',  #
    'add_over_pixels' : 'addo', #
    'add_pin_pixels' : 'addp',  #
    'sub_over_pixels' : 'subo', #
    'sub_pin_pixels' : 'subp',  #
    'ad_max_pixels' : 'admx',   #
    'ad_min_pixels' : 'admn',   #
    'blend_pixels' : 'blnd',    #
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
    'arro' : _Prop_arrow_style,
    'cltb' : _Prop_color_table,
    'colr' : _Prop_color,
    'flcl' : _Prop_fill_color,
    'flpt' : _Prop_fill_pattern,
    'font' : _Prop_font,
    'gobs' : _Prop_ordering,
    'pang' : _Prop_start_angle,
    'parc' : _Prop_arc_angle,
    'pbcl' : _Prop_background_color,
    'pbnd' : _Prop_bounds,
    'pbpt' : _Prop_background_pattern,
    'pchd' : _Prop_corner_curve_height,
    'pcwd' : _Prop_corner_curve_width,
    'pdpt' : _Prop_pixel_depth,
    'pdrt' : _Prop_definition_rect,
    'pdst' : _Prop_dash_style,
    'pend' : _Prop_end_point,
    'pnam' : _Prop_name,
    'pnel' : _Prop_default_location,
    'ppcl' : _Prop_pen_color,
    'pppa' : _Prop_pen_pattern,
    'pptm' : _Prop_transfer_mode,
    'ppwd' : _Prop_pen_width,
    'psct' : _Prop_writing_code,
    'pstp' : _Prop_start_point,
    'ptlt' : _Prop_point_list,
    'ptps' : _Prop_default_size,
    'ptsz' : _Prop_size,
    'ptxc' : _Prop_text_color,
    'ptxf' : _Prop_default_font,
    'pupd' : _Prop_update_on_change,
    'txst' : _Prop_style,
    'ustl' : _Prop_uniform_styles,
}

_compdeclarations = {
}

_enumdeclarations = {
    'arro' : _Enum_arro,
    'tran' : _Enum_tran,
}
