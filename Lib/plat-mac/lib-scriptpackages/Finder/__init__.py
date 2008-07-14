"""
Package generated from /System/Library/CoreServices/Finder.app
"""

from warnings import warnpy3k
warnpy3k("In 3.x, the Finder package is removed.", stacklevel=2)

import aetools
Error = aetools.Error
import Standard_Suite
import Legacy_suite
import Containers_and_folders
import Files
import Finder_Basics
import Finder_items
import Window_classes
import Type_Definitions
import Enumerations


_code_to_module = {
    'CoRe' : Standard_Suite,
    'fleg' : Legacy_suite,
    'fndr' : Containers_and_folders,
    'fndr' : Files,
    'fndr' : Finder_Basics,
    'fndr' : Finder_items,
    'fndr' : Window_classes,
    'tpdf' : Type_Definitions,
    'tpnm' : Enumerations,
}



_code_to_fullname = {
    'CoRe' : ('Finder.Standard_Suite', 'Standard_Suite'),
    'fleg' : ('Finder.Legacy_suite', 'Legacy_suite'),
    'fndr' : ('Finder.Containers_and_folders', 'Containers_and_folders'),
    'fndr' : ('Finder.Files', 'Files'),
    'fndr' : ('Finder.Finder_Basics', 'Finder_Basics'),
    'fndr' : ('Finder.Finder_items', 'Finder_items'),
    'fndr' : ('Finder.Window_classes', 'Window_classes'),
    'tpdf' : ('Finder.Type_Definitions', 'Type_Definitions'),
    'tpnm' : ('Finder.Enumerations', 'Enumerations'),
}

from Standard_Suite import *
from Legacy_suite import *
from Containers_and_folders import *
from Files import *
from Finder_Basics import *
from Finder_items import *
from Window_classes import *
from Type_Definitions import *
from Enumerations import *

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
getbaseclasses(StdSuites.Type_Names_Suite.small_integer)
getbaseclasses(StdSuites.Type_Names_Suite.system_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.color_table)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.string)
getbaseclasses(StdSuites.Type_Names_Suite.type_element_info)
getbaseclasses(StdSuites.Type_Names_Suite.machine_location)
getbaseclasses(StdSuites.Type_Names_Suite.PostScript_picture)
getbaseclasses(StdSuites.Type_Names_Suite.type_property_info)
getbaseclasses(StdSuites.Type_Names_Suite.menu_item)
getbaseclasses(StdSuites.Type_Names_Suite.scrap_styles)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.null)
getbaseclasses(StdSuites.Type_Names_Suite.type_event_info)
getbaseclasses(StdSuites.Type_Names_Suite.rotation)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.long_point)
getbaseclasses(StdSuites.Type_Names_Suite.target_id)
getbaseclasses(StdSuites.Type_Names_Suite.type_suite_info)
getbaseclasses(StdSuites.Type_Names_Suite.type_parameter_info)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.bounding_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.TIFF_picture)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed)
getbaseclasses(StdSuites.Type_Names_Suite.version)
getbaseclasses(StdSuites.Type_Names_Suite.RGB16_color)
getbaseclasses(StdSuites.Type_Names_Suite.double_integer)
getbaseclasses(StdSuites.Type_Names_Suite.location_reference)
getbaseclasses(StdSuites.Type_Names_Suite.point)
getbaseclasses(StdSuites.Type_Names_Suite.application_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.unsigned_integer)
getbaseclasses(StdSuites.Type_Names_Suite.menu)
getbaseclasses(StdSuites.Type_Names_Suite.small_real)
getbaseclasses(StdSuites.Type_Names_Suite.fixed)
getbaseclasses(StdSuites.Type_Names_Suite.type_class_info)
getbaseclasses(StdSuites.Type_Names_Suite.RGB96_color)
getbaseclasses(StdSuites.Type_Names_Suite.dash_style)
getbaseclasses(StdSuites.Type_Names_Suite.pixel_map_record)
getbaseclasses(StdSuites.Type_Names_Suite.extended_real)
getbaseclasses(StdSuites.Type_Names_Suite.long_rectangle)
getbaseclasses(process)
getbaseclasses(application_process)
getbaseclasses(desk_accessory_process)
getbaseclasses(application)
getbaseclasses(trash_2d_object)
getbaseclasses(desktop_2d_object)
getbaseclasses(container)
getbaseclasses(folder)
getbaseclasses(disk)
getbaseclasses(application)
getbaseclasses(alias_file)
getbaseclasses(package)
getbaseclasses(file)
getbaseclasses(application_file)
getbaseclasses(internet_location_file)
getbaseclasses(document_file)
getbaseclasses(clipping)
getbaseclasses(preferences_window)
getbaseclasses(Finder_window)
getbaseclasses(window)
getbaseclasses(clipping_window)
getbaseclasses(information_window)
getbaseclasses(item)
getbaseclasses(icon_view_options)
getbaseclasses(preferences)
getbaseclasses(alias_list)
getbaseclasses(icon_family)
getbaseclasses(label)
getbaseclasses(column)
getbaseclasses(list_view_options)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'shor' : StdSuites.Type_Names_Suite.small_integer,
    'aeut' : StdSuites.Type_Names_Suite.system_dictionary,
    'clrt' : StdSuites.Type_Names_Suite.color_table,
    'fpnt' : StdSuites.Type_Names_Suite.fixed_point,
    'TEXT' : StdSuites.Type_Names_Suite.string,
    'elin' : StdSuites.Type_Names_Suite.type_element_info,
    'mLoc' : StdSuites.Type_Names_Suite.machine_location,
    'EPS ' : StdSuites.Type_Names_Suite.PostScript_picture,
    'pinf' : StdSuites.Type_Names_Suite.type_property_info,
    'cmen' : StdSuites.Type_Names_Suite.menu_item,
    'styl' : StdSuites.Type_Names_Suite.scrap_styles,
    'frct' : StdSuites.Type_Names_Suite.fixed_rectangle,
    'null' : StdSuites.Type_Names_Suite.null,
    'evin' : StdSuites.Type_Names_Suite.type_event_info,
    'trot' : StdSuites.Type_Names_Suite.rotation,
    'lfrc' : StdSuites.Type_Names_Suite.long_fixed_rectangle,
    'lpnt' : StdSuites.Type_Names_Suite.long_point,
    'targ' : StdSuites.Type_Names_Suite.target_id,
    'suin' : StdSuites.Type_Names_Suite.type_suite_info,
    'pmin' : StdSuites.Type_Names_Suite.type_parameter_info,
    'lfpt' : StdSuites.Type_Names_Suite.long_fixed_point,
    'qdrt' : StdSuites.Type_Names_Suite.bounding_rectangle,
    'TIFF' : StdSuites.Type_Names_Suite.TIFF_picture,
    'lfxd' : StdSuites.Type_Names_Suite.long_fixed,
    'vers' : StdSuites.Type_Names_Suite.version,
    'tr16' : StdSuites.Type_Names_Suite.RGB16_color,
    'comp' : StdSuites.Type_Names_Suite.double_integer,
    'insl' : StdSuites.Type_Names_Suite.location_reference,
    'QDpt' : StdSuites.Type_Names_Suite.point,
    'aete' : StdSuites.Type_Names_Suite.application_dictionary,
    'magn' : StdSuites.Type_Names_Suite.unsigned_integer,
    'cmnu' : StdSuites.Type_Names_Suite.menu,
    'sing' : StdSuites.Type_Names_Suite.small_real,
    'fixd' : StdSuites.Type_Names_Suite.fixed,
    'gcli' : StdSuites.Type_Names_Suite.type_class_info,
    'tr96' : StdSuites.Type_Names_Suite.RGB96_color,
    'tdas' : StdSuites.Type_Names_Suite.dash_style,
    'tpmm' : StdSuites.Type_Names_Suite.pixel_map_record,
    'exte' : StdSuites.Type_Names_Suite.extended_real,
    'lrct' : StdSuites.Type_Names_Suite.long_rectangle,
    'prcs' : process,
    'pcap' : application_process,
    'pcda' : desk_accessory_process,
    'capp' : application,
    'ctrs' : trash_2d_object,
    'cdsk' : desktop_2d_object,
    'ctnr' : container,
    'cfol' : folder,
    'cdis' : disk,
    'capp' : application,
    'alia' : alias_file,
    'pack' : package,
    'file' : file,
    'appf' : application_file,
    'inlf' : internet_location_file,
    'docf' : document_file,
    'clpf' : clipping,
    'pwnd' : preferences_window,
    'brow' : Finder_window,
    'cwin' : window,
    'lwnd' : clipping_window,
    'iwnd' : information_window,
    'cobj' : item,
    'icop' : icon_view_options,
    'cprf' : preferences,
    'alst' : alias_list,
    'ifam' : icon_family,
    'clbl' : label,
    'lvcl' : column,
    'lvop' : list_view_options,
}


class Finder(Standard_Suite_Events,
        Legacy_suite_Events,
        Containers_and_folders_Events,
        Files_Events,
        Finder_Basics_Events,
        Finder_items_Events,
        Window_classes_Events,
        Type_Definitions_Events,
        Enumerations_Events,
        aetools.TalkTo):
    _signature = 'MACS'

    _moduleName = 'Finder'

    _elemdict = application._elemdict
    _propdict = application._propdict
