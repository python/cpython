"""
Package generated from /Applications/Utilities/Terminal.app/Contents/Resources/Terminal.rsrc
Resource aete resid 0 Terminal Terminology
"""
import aetools
Error = aetools.Error
import Invisible_Suite
import Terminal_Suite


_code_to_module = {
	'tpnm' : Invisible_Suite,
	'trmx' : Terminal_Suite,
}



_code_to_fullname = {
	'tpnm' : ('Terminal.Invisible_Suite', 'Invisible_Suite'),
	'trmx' : ('Terminal.Terminal_Suite', 'Terminal_Suite'),
}

from Invisible_Suite import *
from Terminal_Suite import *

def getbaseclasses(v):
	if not getattr(v, '_propdict', None):
		v._propdict = {}
		v._elemdict = {}
		for superclassname in getattr(v, '_superclassnames', []):
			superclass = eval(superclassname)
			getbaseclasses(superclass)
			v._propdict.update(getattr(superclass, '_propdict', {}))
			v._elemdict.update(getattr(superclass, '_elemdict', {}))
		v._propdict.update(getattr(v, '_privpropdict', {}))
		v._elemdict.update(getattr(v, '_privelemdict', {}))

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(application)
getbaseclasses(StdSuites.Type_Names_Suite.small_integer)
getbaseclasses(StdSuites.Type_Names_Suite.RGB16_color)
getbaseclasses(StdSuites.Type_Names_Suite.system_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.color_table)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.plain_text)
getbaseclasses(StdSuites.Type_Names_Suite.type_element_info)
getbaseclasses(StdSuites.Type_Names_Suite.location_reference)
getbaseclasses(StdSuites.Type_Names_Suite.version)
getbaseclasses(StdSuites.Type_Names_Suite.PostScript_picture)
getbaseclasses(StdSuites.Type_Names_Suite.machine_location)
getbaseclasses(StdSuites.Type_Names_Suite.menu_item)
getbaseclasses(StdSuites.Type_Names_Suite.pixel_map_record)
getbaseclasses(StdSuites.Type_Names_Suite.application_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.unsigned_integer)
getbaseclasses(StdSuites.Type_Names_Suite.menu)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.type_event_info)
getbaseclasses(StdSuites.Type_Names_Suite.small_real)
getbaseclasses(StdSuites.Type_Names_Suite.type_suite_info)
getbaseclasses(StdSuites.Type_Names_Suite.rotation)
getbaseclasses(StdSuites.Type_Names_Suite.fixed)
getbaseclasses(StdSuites.Type_Names_Suite.scrap_styles)
getbaseclasses(StdSuites.Type_Names_Suite.long_point)
getbaseclasses(StdSuites.Type_Names_Suite.type_class_info)
getbaseclasses(StdSuites.Type_Names_Suite.TIFF_picture)
getbaseclasses(StdSuites.Type_Names_Suite.RGB96_color)
getbaseclasses(StdSuites.Type_Names_Suite.dash_style)
getbaseclasses(StdSuites.Type_Names_Suite.type_property_info)
getbaseclasses(StdSuites.Type_Names_Suite.type_parameter_info)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.long_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.extended_real)
getbaseclasses(StdSuites.Type_Names_Suite.double_integer)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed)
getbaseclasses(StdSuites.Type_Names_Suite.null)
getbaseclasses(StdSuites.Type_Names_Suite.target_id)
getbaseclasses(StdSuites.Type_Names_Suite.point)
getbaseclasses(StdSuites.Type_Names_Suite.bounding_rectangle)

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cwin' : window,
	'capp' : application,
	'capp' : application,
	'shor' : StdSuites.Type_Names_Suite.small_integer,
	'tr16' : StdSuites.Type_Names_Suite.RGB16_color,
	'aeut' : StdSuites.Type_Names_Suite.system_dictionary,
	'clrt' : StdSuites.Type_Names_Suite.color_table,
	'fpnt' : StdSuites.Type_Names_Suite.fixed_point,
	'TEXT' : StdSuites.Type_Names_Suite.plain_text,
	'elin' : StdSuites.Type_Names_Suite.type_element_info,
	'insl' : StdSuites.Type_Names_Suite.location_reference,
	'vers' : StdSuites.Type_Names_Suite.version,
	'EPS ' : StdSuites.Type_Names_Suite.PostScript_picture,
	'mLoc' : StdSuites.Type_Names_Suite.machine_location,
	'cmen' : StdSuites.Type_Names_Suite.menu_item,
	'tpmm' : StdSuites.Type_Names_Suite.pixel_map_record,
	'aete' : StdSuites.Type_Names_Suite.application_dictionary,
	'magn' : StdSuites.Type_Names_Suite.unsigned_integer,
	'cmnu' : StdSuites.Type_Names_Suite.menu,
	'frct' : StdSuites.Type_Names_Suite.fixed_rectangle,
	'lfrc' : StdSuites.Type_Names_Suite.long_fixed_rectangle,
	'evin' : StdSuites.Type_Names_Suite.type_event_info,
	'sing' : StdSuites.Type_Names_Suite.small_real,
	'suin' : StdSuites.Type_Names_Suite.type_suite_info,
	'trot' : StdSuites.Type_Names_Suite.rotation,
	'fixd' : StdSuites.Type_Names_Suite.fixed,
	'styl' : StdSuites.Type_Names_Suite.scrap_styles,
	'lpnt' : StdSuites.Type_Names_Suite.long_point,
	'gcli' : StdSuites.Type_Names_Suite.type_class_info,
	'TIFF' : StdSuites.Type_Names_Suite.TIFF_picture,
	'tr96' : StdSuites.Type_Names_Suite.RGB96_color,
	'tdas' : StdSuites.Type_Names_Suite.dash_style,
	'pinf' : StdSuites.Type_Names_Suite.type_property_info,
	'pmin' : StdSuites.Type_Names_Suite.type_parameter_info,
	'lfpt' : StdSuites.Type_Names_Suite.long_fixed_point,
	'lrct' : StdSuites.Type_Names_Suite.long_rectangle,
	'exte' : StdSuites.Type_Names_Suite.extended_real,
	'comp' : StdSuites.Type_Names_Suite.double_integer,
	'lfxd' : StdSuites.Type_Names_Suite.long_fixed,
	'null' : StdSuites.Type_Names_Suite.null,
	'targ' : StdSuites.Type_Names_Suite.target_id,
	'QDpt' : StdSuites.Type_Names_Suite.point,
	'qdrt' : StdSuites.Type_Names_Suite.bounding_rectangle,
}


class Terminal(Invisible_Suite_Events,
		Terminal_Suite_Events,
		aetools.TalkTo):
	_signature = 'trmx'

	_moduleName = 'Terminal'

