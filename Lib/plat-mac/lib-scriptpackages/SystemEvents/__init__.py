"""
Package generated from /System/Library/CoreServices/System Events.app
"""
import aetools
Error = aetools.Error
import Standard_Suite
import Text_Suite
import Disk_2d_Folder_2d_File_Suite
import Folder_Actions_Suite
import Login_Items_Suite
import Power_Suite
import Processes_Suite
import System_Events_Suite
import Hidden_Suite


_code_to_module = {
    '????' : Standard_Suite,
    '????' : Text_Suite,
    'cdis' : Disk_2d_Folder_2d_File_Suite,
    'faco' : Folder_Actions_Suite,
    'logi' : Login_Items_Suite,
    'powr' : Power_Suite,
    'prcs' : Processes_Suite,
    'sevs' : System_Events_Suite,
    'tpnm' : Hidden_Suite,
}



_code_to_fullname = {
    '????' : ('SystemEvents.Standard_Suite', 'Standard_Suite'),
    '????' : ('SystemEvents.Text_Suite', 'Text_Suite'),
    'cdis' : ('SystemEvents.Disk_2d_Folder_2d_File_Suite', 'Disk_2d_Folder_2d_File_Suite'),
    'faco' : ('SystemEvents.Folder_Actions_Suite', 'Folder_Actions_Suite'),
    'logi' : ('SystemEvents.Login_Items_Suite', 'Login_Items_Suite'),
    'powr' : ('SystemEvents.Power_Suite', 'Power_Suite'),
    'prcs' : ('SystemEvents.Processes_Suite', 'Processes_Suite'),
    'sevs' : ('SystemEvents.System_Events_Suite', 'System_Events_Suite'),
    'tpnm' : ('SystemEvents.Hidden_Suite', 'Hidden_Suite'),
}

from Standard_Suite import *
from Text_Suite import *
from Disk_2d_Folder_2d_File_Suite import *
from Folder_Actions_Suite import *
from Login_Items_Suite import *
from Power_Suite import *
from Processes_Suite import *
from System_Events_Suite import *
from Hidden_Suite import *

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
getbaseclasses(color)
getbaseclasses(window)
getbaseclasses(application)
getbaseclasses(item)
getbaseclasses(document)
getbaseclasses(character)
getbaseclasses(attachment)
getbaseclasses(paragraph)
getbaseclasses(word)
getbaseclasses(attribute_run)
getbaseclasses(text)
getbaseclasses(login_item)
getbaseclasses(file)
getbaseclasses(alias)
getbaseclasses(item)
getbaseclasses(folder)
getbaseclasses(disk)
getbaseclasses(script)
getbaseclasses(folder_action)
getbaseclasses(StdSuites.Type_Names_Suite.double_integer)
getbaseclasses(StdSuites.Type_Names_Suite.version)
getbaseclasses(StdSuites.Type_Names_Suite.RGB16_color)
getbaseclasses(StdSuites.Type_Names_Suite.system_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.color_table)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.TIFF_picture)
getbaseclasses(StdSuites.Type_Names_Suite.type_element_info)
getbaseclasses(StdSuites.Type_Names_Suite.type_event_info)
getbaseclasses(StdSuites.Type_Names_Suite.machine_location)
getbaseclasses(StdSuites.Type_Names_Suite.PostScript_picture)
getbaseclasses(StdSuites.Type_Names_Suite.point)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_point)
getbaseclasses(StdSuites.Type_Names_Suite.menu_item)
getbaseclasses(StdSuites.Type_Names_Suite.scrap_styles)
getbaseclasses(StdSuites.Type_Names_Suite.application_dictionary)
getbaseclasses(StdSuites.Type_Names_Suite.unsigned_integer)
getbaseclasses(StdSuites.Type_Names_Suite.menu)
getbaseclasses(StdSuites.Type_Names_Suite.fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.type_property_info)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.long_fixed)
getbaseclasses(StdSuites.Type_Names_Suite.type_suite_info)
getbaseclasses(StdSuites.Type_Names_Suite.rotation)
getbaseclasses(StdSuites.Type_Names_Suite.small_integer)
getbaseclasses(StdSuites.Type_Names_Suite.fixed)
getbaseclasses(StdSuites.Type_Names_Suite.long_point)
getbaseclasses(StdSuites.Type_Names_Suite.type_class_info)
getbaseclasses(StdSuites.Type_Names_Suite.RGB96_color)
getbaseclasses(StdSuites.Type_Names_Suite.target_id)
getbaseclasses(StdSuites.Type_Names_Suite.pixel_map_record)
getbaseclasses(StdSuites.Type_Names_Suite.type_parameter_info)
getbaseclasses(StdSuites.Type_Names_Suite.extended_real)
getbaseclasses(StdSuites.Type_Names_Suite.long_rectangle)
getbaseclasses(StdSuites.Type_Names_Suite.dash_style)
getbaseclasses(StdSuites.Type_Names_Suite.string)
getbaseclasses(StdSuites.Type_Names_Suite.small_real)
getbaseclasses(StdSuites.Type_Names_Suite.null)
getbaseclasses(StdSuites.Type_Names_Suite.location_reference)
getbaseclasses(StdSuites.Type_Names_Suite.bounding_rectangle)
getbaseclasses(window)
getbaseclasses(radio_button)
getbaseclasses(list)
getbaseclasses(desk_accessory_process)
getbaseclasses(menu_item)
getbaseclasses(progress_indicator)
getbaseclasses(menu)
getbaseclasses(menu_button)
getbaseclasses(pop_up_button)
getbaseclasses(incrementor)
getbaseclasses(sheet)
getbaseclasses(tool_bar)
getbaseclasses(application_process)
getbaseclasses(text_field)
getbaseclasses(text_area)
getbaseclasses(slider)
getbaseclasses(scroll_area)
getbaseclasses(relevance_indicator)
getbaseclasses(busy_indicator)
getbaseclasses(row)
getbaseclasses(process)
getbaseclasses(table)
getbaseclasses(outline)
getbaseclasses(UI_element)
getbaseclasses(value_indicator)
getbaseclasses(system_wide_UI_element)
getbaseclasses(button)
getbaseclasses(application)
getbaseclasses(radio_group)
getbaseclasses(image)
getbaseclasses(tab_group)
getbaseclasses(menu_bar)
getbaseclasses(grow_area)
getbaseclasses(check_box)
getbaseclasses(column)
getbaseclasses(static_text)
getbaseclasses(splitter_group)
getbaseclasses(group)
getbaseclasses(splitter)
getbaseclasses(drawer)
getbaseclasses(color_well)
getbaseclasses(scroll_bar)
getbaseclasses(combo_box)
getbaseclasses(browser)
getbaseclasses(application)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'colr' : color,
    'cwin' : window,
    'capp' : application,
    'cobj' : item,
    'docu' : document,
    'cha ' : character,
    'atts' : attachment,
    'cpar' : paragraph,
    'cwor' : word,
    'catr' : attribute_run,
    'ctxt' : text,
    'logi' : login_item,
    'file' : file,
    'alis' : alias,
    'cobj' : item,
    'cfol' : folder,
    'cdis' : disk,
    'scpt' : script,
    'foac' : folder_action,
    'comp' : StdSuites.Type_Names_Suite.double_integer,
    'vers' : StdSuites.Type_Names_Suite.version,
    'tr16' : StdSuites.Type_Names_Suite.RGB16_color,
    'aeut' : StdSuites.Type_Names_Suite.system_dictionary,
    'clrt' : StdSuites.Type_Names_Suite.color_table,
    'fpnt' : StdSuites.Type_Names_Suite.fixed_point,
    'TIFF' : StdSuites.Type_Names_Suite.TIFF_picture,
    'elin' : StdSuites.Type_Names_Suite.type_element_info,
    'evin' : StdSuites.Type_Names_Suite.type_event_info,
    'mLoc' : StdSuites.Type_Names_Suite.machine_location,
    'EPS ' : StdSuites.Type_Names_Suite.PostScript_picture,
    'QDpt' : StdSuites.Type_Names_Suite.point,
    'lfpt' : StdSuites.Type_Names_Suite.long_fixed_point,
    'cmen' : StdSuites.Type_Names_Suite.menu_item,
    'styl' : StdSuites.Type_Names_Suite.scrap_styles,
    'aete' : StdSuites.Type_Names_Suite.application_dictionary,
    'magn' : StdSuites.Type_Names_Suite.unsigned_integer,
    'cmnu' : StdSuites.Type_Names_Suite.menu,
    'frct' : StdSuites.Type_Names_Suite.fixed_rectangle,
    'pinf' : StdSuites.Type_Names_Suite.type_property_info,
    'lfrc' : StdSuites.Type_Names_Suite.long_fixed_rectangle,
    'lfxd' : StdSuites.Type_Names_Suite.long_fixed,
    'suin' : StdSuites.Type_Names_Suite.type_suite_info,
    'trot' : StdSuites.Type_Names_Suite.rotation,
    'shor' : StdSuites.Type_Names_Suite.small_integer,
    'fixd' : StdSuites.Type_Names_Suite.fixed,
    'lpnt' : StdSuites.Type_Names_Suite.long_point,
    'gcli' : StdSuites.Type_Names_Suite.type_class_info,
    'tr96' : StdSuites.Type_Names_Suite.RGB96_color,
    'targ' : StdSuites.Type_Names_Suite.target_id,
    'tpmm' : StdSuites.Type_Names_Suite.pixel_map_record,
    'pmin' : StdSuites.Type_Names_Suite.type_parameter_info,
    'exte' : StdSuites.Type_Names_Suite.extended_real,
    'lrct' : StdSuites.Type_Names_Suite.long_rectangle,
    'tdas' : StdSuites.Type_Names_Suite.dash_style,
    'TEXT' : StdSuites.Type_Names_Suite.string,
    'sing' : StdSuites.Type_Names_Suite.small_real,
    'null' : StdSuites.Type_Names_Suite.null,
    'insl' : StdSuites.Type_Names_Suite.location_reference,
    'qdrt' : StdSuites.Type_Names_Suite.bounding_rectangle,
    'cwin' : window,
    'radB' : radio_button,
    'list' : list,
    'pcda' : desk_accessory_process,
    'menI' : menu_item,
    'proI' : progress_indicator,
    'menE' : menu,
    'menB' : menu_button,
    'popB' : pop_up_button,
    'incr' : incrementor,
    'sheE' : sheet,
    'tbar' : tool_bar,
    'pcap' : application_process,
    'txtf' : text_field,
    'txta' : text_area,
    'sliI' : slider,
    'scra' : scroll_area,
    'reli' : relevance_indicator,
    'busi' : busy_indicator,
    'crow' : row,
    'prcs' : process,
    'tabB' : table,
    'outl' : outline,
    'uiel' : UI_element,
    'vali' : value_indicator,
    'sysw' : system_wide_UI_element,
    'butT' : button,
    'capp' : application,
    'rgrp' : radio_group,
    'imaA' : image,
    'tab ' : tab_group,
    'mbar' : menu_bar,
    'grow' : grow_area,
    'chbx' : check_box,
    'ccol' : column,
    'sttx' : static_text,
    'splg' : splitter_group,
    'sgrp' : group,
    'splr' : splitter,
    'draA' : drawer,
    'colW' : color_well,
    'scrb' : scroll_bar,
    'comB' : combo_box,
    'broW' : browser,
    'capp' : application,
}


class SystemEvents(Standard_Suite_Events,
        Text_Suite_Events,
        Disk_2d_Folder_2d_File_Suite_Events,
        Folder_Actions_Suite_Events,
        Login_Items_Suite_Events,
        Power_Suite_Events,
        Processes_Suite_Events,
        System_Events_Suite_Events,
        Hidden_Suite_Events,
        aetools.TalkTo):
    _signature = 'sevs'

    _moduleName = 'SystemEvents'

