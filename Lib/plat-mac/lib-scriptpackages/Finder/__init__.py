"""
Package generated from /Volumes/Sap/System Folder/Finder
Resource aete resid 0 
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Earlier_terms
import Finder_Basics
import Finder_items
import Containers_and_folders
import Files_and_suitcases
import Window_classes
import Process_classes
import Type_Definitions
import Enumerations
import Obsolete_terms


_code_to_module = {
	'CoRe' : Standard_Suite,
	'tpnm' : Earlier_terms,
	'fndr' : Finder_Basics,
	'fndr' : Finder_items,
	'fndr' : Containers_and_folders,
	'fndr' : Files_and_suitcases,
	'fndr' : Window_classes,
	'fndr' : Process_classes,
	'tpdf' : Type_Definitions,
	'tpnm' : Enumerations,
	'tpnm' : Obsolete_terms,
}



_code_to_fullname = {
	'CoRe' : ('Finder.Standard_Suite', 'Standard_Suite'),
	'tpnm' : ('Finder.Earlier_terms', 'Earlier_terms'),
	'fndr' : ('Finder.Finder_Basics', 'Finder_Basics'),
	'fndr' : ('Finder.Finder_items', 'Finder_items'),
	'fndr' : ('Finder.Containers_and_folders', 'Containers_and_folders'),
	'fndr' : ('Finder.Files_and_suitcases', 'Files_and_suitcases'),
	'fndr' : ('Finder.Window_classes', 'Window_classes'),
	'fndr' : ('Finder.Process_classes', 'Process_classes'),
	'tpdf' : ('Finder.Type_Definitions', 'Type_Definitions'),
	'tpnm' : ('Finder.Enumerations', 'Enumerations'),
	'tpnm' : ('Finder.Obsolete_terms', 'Obsolete_terms'),
}

from Standard_Suite import *
from Earlier_terms import *
from Finder_Basics import *
from Finder_items import *
from Containers_and_folders import *
from Files_and_suitcases import *
from Window_classes import *
from Process_classes import *
from Type_Definitions import *
from Enumerations import *
from Obsolete_terms import *
def getbaseclasses(v):
	if hasattr(v, '_superclassnames') and not hasattr(v, '_propdict'):
		v._propdict = {}
		v._elemdict = {}
		for superclass in v._superclassnames:
			v._propdict.update(getattr(eval(superclass), '_privpropdict', {}))
			v._elemdict.update(getattr(eval(superclass), '_privelemdict', {}))
		v._propdict.update(v._privpropdict)
		v._elemdict.update(v._privelemdict)

import StdSuites

#
# Set property and element dictionaries now that all classes have been defined
#
getbaseclasses(accessory_suitcase)
getbaseclasses(preferences)
getbaseclasses(sharable_container)
getbaseclasses(application)
getbaseclasses(trash_2d_object)
getbaseclasses(accessory_process)
getbaseclasses(window)
getbaseclasses(information_window)
getbaseclasses(process)
getbaseclasses(application_file)
getbaseclasses(internet_location)
getbaseclasses(container_window)
getbaseclasses(item)
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
getbaseclasses(application)
getbaseclasses(special_folders)
getbaseclasses(item)
getbaseclasses(trash_2d_object)
getbaseclasses(desktop_2d_object)
getbaseclasses(sharable_container)
getbaseclasses(sharing_privileges)
getbaseclasses(disk)
getbaseclasses(folder)
getbaseclasses(container)
getbaseclasses(sound_file)
getbaseclasses(font_file)
getbaseclasses(internet_location_file)
getbaseclasses(clipping)
getbaseclasses(alias_file)
getbaseclasses(desk_accessory_file)
getbaseclasses(desk_accessory_suitcase)
getbaseclasses(font_suitcase)
getbaseclasses(file)
getbaseclasses(application_file)
getbaseclasses(suitcase)
getbaseclasses(document_file)
getbaseclasses(package)
getbaseclasses(preferences_window)
getbaseclasses(view_options_window)
getbaseclasses(window)
getbaseclasses(container_window)
getbaseclasses(content_space)
getbaseclasses(information_window)
getbaseclasses(clipping_window)
getbaseclasses(process)
getbaseclasses(desk_accessory_process)
getbaseclasses(application_process)
getbaseclasses(preferences)
getbaseclasses(alias_list)
getbaseclasses(icon_family)
getbaseclasses(label)
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
getbaseclasses(status_window)
getbaseclasses(application)
getbaseclasses(sharing_window)
getbaseclasses(control_panel)
getbaseclasses(process)
getbaseclasses(item)
getbaseclasses(file)
getbaseclasses(sharable_container)
getbaseclasses(container_window)
getbaseclasses(container)
getbaseclasses(information_window)
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
	'dsut' : accessory_suitcase,
	'cprf' : preferences,
	'sctr' : sharable_container,
	'capp' : application,
	'ctrs' : trash_2d_object,
	'pcda' : accessory_process,
	'cwin' : window,
	'iwnd' : information_window,
	'prcs' : process,
	'appf' : application_file,
	'inlf' : internet_location,
	'cwnd' : container_window,
	'cobj' : item,
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
	'capp' : application,
	'spfl' : special_folders,
	'cobj' : item,
	'ctrs' : trash_2d_object,
	'cdsk' : desktop_2d_object,
	'sctr' : sharable_container,
	'priv' : sharing_privileges,
	'cdis' : disk,
	'cfol' : folder,
	'ctnr' : container,
	'sndf' : sound_file,
	'fntf' : font_file,
	'inlf' : internet_location_file,
	'clpf' : clipping,
	'alia' : alias_file,
	'dafi' : desk_accessory_file,
	'dsut' : desk_accessory_suitcase,
	'fsut' : font_suitcase,
	'file' : file,
	'appf' : application_file,
	'stcs' : suitcase,
	'docf' : document_file,
	'pack' : package,
	'pwnd' : preferences_window,
	'vwnd' : view_options_window,
	'cwin' : window,
	'cwnd' : container_window,
	'dwnd' : content_space,
	'iwnd' : information_window,
	'lwnd' : clipping_window,
	'prcs' : process,
	'pcda' : desk_accessory_process,
	'pcap' : application_process,
	'cprf' : preferences,
	'alst' : alias_list,
	'ifam' : icon_family,
	'clbl' : label,
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
	'qwnd' : status_window,
	'capp' : application,
	'swnd' : sharing_window,
	'ccdv' : control_panel,
	'prcs' : process,
	'cobj' : item,
	'file' : file,
	'sctr' : sharable_container,
	'cwnd' : container_window,
	'ctnr' : container,
	'iwnd' : information_window,
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


class Finder(Standard_Suite_Events,
		Earlier_terms_Events,
		Finder_Basics_Events,
		Finder_items_Events,
		Containers_and_folders_Events,
		Files_and_suitcases_Events,
		Window_classes_Events,
		Process_classes_Events,
		Type_Definitions_Events,
		Enumerations_Events,
		Obsolete_terms_Events,
		aetools.TalkTo):
	_signature = 'MACS'

	_moduleName = 'Finder'

