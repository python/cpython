"""
Package generated from /Volumes/Sap/System Folder/Extensions/AppleScript
Resource aeut resid 0 Standard Event Suites for English
"""

from warnings import warnpy3k
warnpy3k("In 3.x, the StdSuites package is removed.", stacklevel=2)

import aetools
Error = aetools.Error
import Text_Suite
import AppleScript_Suite
import Standard_Suite
import Macintosh_Connectivity_Clas
import QuickDraw_Graphics_Suite
import QuickDraw_Graphics_Suppleme
import Required_Suite
import Table_Suite
import Type_Names_Suite


_code_to_module = {
    'TEXT' : Text_Suite,
    'ascr' : AppleScript_Suite,
    'core' : Standard_Suite,
    'macc' : Macintosh_Connectivity_Clas,
    'qdrw' : QuickDraw_Graphics_Suite,
    'qdsp' : QuickDraw_Graphics_Suppleme,
    'reqd' : Required_Suite,
    'tbls' : Table_Suite,
    'tpnm' : Type_Names_Suite,
}



_code_to_fullname = {
    'TEXT' : ('StdSuites.Text_Suite', 'Text_Suite'),
    'ascr' : ('StdSuites.AppleScript_Suite', 'AppleScript_Suite'),
    'core' : ('StdSuites.Standard_Suite', 'Standard_Suite'),
    'macc' : ('StdSuites.Macintosh_Connectivity_Clas', 'Macintosh_Connectivity_Clas'),
    'qdrw' : ('StdSuites.QuickDraw_Graphics_Suite', 'QuickDraw_Graphics_Suite'),
    'qdsp' : ('StdSuites.QuickDraw_Graphics_Suppleme', 'QuickDraw_Graphics_Suppleme'),
    'reqd' : ('StdSuites.Required_Suite', 'Required_Suite'),
    'tbls' : ('StdSuites.Table_Suite', 'Table_Suite'),
    'tpnm' : ('StdSuites.Type_Names_Suite', 'Type_Names_Suite'),
}

from Text_Suite import *
from AppleScript_Suite import *
from Standard_Suite import *
from Macintosh_Connectivity_Clas import *
from QuickDraw_Graphics_Suite import *
from QuickDraw_Graphics_Suppleme import *
from Required_Suite import *
from Table_Suite import *
from Type_Names_Suite import *

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
getbaseclasses(graphic_group)
getbaseclasses(oval)
getbaseclasses(graphic_text)
getbaseclasses(graphic_shape)
getbaseclasses(drawing_area)
getbaseclasses(graphic_line)
getbaseclasses(polygon)
getbaseclasses(pixel)
getbaseclasses(rounded_rectangle)
getbaseclasses(graphic_object)
getbaseclasses(arc)
getbaseclasses(pixel_map)
getbaseclasses(rectangle)
getbaseclasses(selection_2d_object)
getbaseclasses(application)
getbaseclasses(document)
getbaseclasses(window)
getbaseclasses(file)
getbaseclasses(alias)
getbaseclasses(insertion_point)
getbaseclasses(character)
getbaseclasses(paragraph)
getbaseclasses(word)
getbaseclasses(text_flow)
getbaseclasses(text_style_info)
getbaseclasses(line)
getbaseclasses(text)
getbaseclasses(AppleTalk_address)
getbaseclasses(address_specification)
getbaseclasses(Token_Ring_address)
getbaseclasses(FireWire_address)
getbaseclasses(bus_slot)
getbaseclasses(SCSI_address)
getbaseclasses(ADB_address)
getbaseclasses(USB_address)
getbaseclasses(device_specification)
getbaseclasses(LocalTalk_address)
getbaseclasses(IP_address)
getbaseclasses(Ethernet_address)
getbaseclasses(graphic_group)
getbaseclasses(drawing_area)
getbaseclasses(cell)
getbaseclasses(column)
getbaseclasses(table)
getbaseclasses(row)
getbaseclasses(small_integer)
getbaseclasses(system_dictionary)
getbaseclasses(color_table)
getbaseclasses(fixed_point)
getbaseclasses(plain_text)
getbaseclasses(type_element_info)
getbaseclasses(machine_location)
getbaseclasses(PostScript_picture)
getbaseclasses(type_suite_info)
getbaseclasses(menu_item)
getbaseclasses(pixel_map_record)
getbaseclasses(small_real)
getbaseclasses(null)
getbaseclasses(rotation)
getbaseclasses(fixed)
getbaseclasses(long_point)
getbaseclasses(target_id)
getbaseclasses(type_property_info)
getbaseclasses(type_parameter_info)
getbaseclasses(long_fixed_point)
getbaseclasses(bounding_rectangle)
getbaseclasses(TIFF_picture)
getbaseclasses(long_fixed)
getbaseclasses(location_reference)
getbaseclasses(version)
getbaseclasses(RGB16_color)
getbaseclasses(double_integer)
getbaseclasses(type_event_info)
getbaseclasses(point)
getbaseclasses(application_dictionary)
getbaseclasses(unsigned_integer)
getbaseclasses(menu)
getbaseclasses(fixed_rectangle)
getbaseclasses(long_fixed_rectangle)
getbaseclasses(type_class_info)
getbaseclasses(RGB96_color)
getbaseclasses(dash_style)
getbaseclasses(scrap_styles)
getbaseclasses(extended_real)
getbaseclasses(long_rectangle)
getbaseclasses(May)
getbaseclasses(string)
getbaseclasses(miles)
getbaseclasses(number_or_date)
getbaseclasses(October)
getbaseclasses(event)
getbaseclasses(Pascal_string)
getbaseclasses(zone)
getbaseclasses(picture)
getbaseclasses(list_or_string)
getbaseclasses(number)
getbaseclasses(Tuesday)
getbaseclasses(version)
getbaseclasses(December)
getbaseclasses(square_kilometres)
getbaseclasses(reference)
getbaseclasses(vector)
getbaseclasses(weekday)
getbaseclasses(Sunday)
getbaseclasses(international_text)
getbaseclasses(seconds)
getbaseclasses(RGB_color)
getbaseclasses(kilometres)
getbaseclasses(styled_Unicode_text)
getbaseclasses(missing_value)
getbaseclasses(metres)
getbaseclasses(number_or_string)
getbaseclasses(list)
getbaseclasses(linked_list)
getbaseclasses(real)
getbaseclasses(encoded_string)
getbaseclasses(list_or_record)
getbaseclasses(Monday)
getbaseclasses(September)
getbaseclasses(anything)
getbaseclasses(property)
getbaseclasses(reference_form)
getbaseclasses(item)
getbaseclasses(grams)
getbaseclasses(record)
getbaseclasses(empty_ae_name_)
getbaseclasses(constant)
getbaseclasses(square_miles)
getbaseclasses(data)
getbaseclasses(Unicode_text)
getbaseclasses(yards)
getbaseclasses(cubic_yards)
getbaseclasses(pounds)
getbaseclasses(cubic_centimetres)
getbaseclasses(text)
getbaseclasses(July)
getbaseclasses(cubic_metres)
getbaseclasses(styled_text)
getbaseclasses(number_2c__date_or_text)
getbaseclasses(feet)
getbaseclasses(February)
getbaseclasses(degrees_Celsius)
getbaseclasses(keystroke)
getbaseclasses(integer)
getbaseclasses(degrees_Fahrenheit)
getbaseclasses(list_2c__record_or_text)
getbaseclasses(date)
getbaseclasses(degrees_Kelvin)
getbaseclasses(centimetres)
getbaseclasses(writing_code)
getbaseclasses(alias_or_string)
getbaseclasses(writing_code_info)
getbaseclasses(text_item)
getbaseclasses(machine)
getbaseclasses(type_class)
getbaseclasses(preposition)
getbaseclasses(Wednesday)
getbaseclasses(upper_case)
getbaseclasses(March)
getbaseclasses(square_feet)
getbaseclasses(November)
getbaseclasses(quarts)
getbaseclasses(alias)
getbaseclasses(January)
getbaseclasses(month)
getbaseclasses(June)
getbaseclasses(August)
getbaseclasses(styled_Clipboard_text)
getbaseclasses(gallons)
getbaseclasses(cubic_inches)
getbaseclasses(Friday)
getbaseclasses(sound)
getbaseclasses(class_)
getbaseclasses(kilograms)
getbaseclasses(script)
getbaseclasses(litres)
getbaseclasses(boolean)
getbaseclasses(square_metres)
getbaseclasses(inches)
getbaseclasses(character)
getbaseclasses(April)
getbaseclasses(ounces)
getbaseclasses(app)
getbaseclasses(handler)
getbaseclasses(C_string)
getbaseclasses(Thursday)
getbaseclasses(square_yards)
getbaseclasses(cubic_feet)
getbaseclasses(Saturday)
getbaseclasses(file_specification)

#
# Indices of types declared in this module
#
_classdeclarations = {
    'cpic' : graphic_group,
    'covl' : oval,
    'cgtx' : graphic_text,
    'cgsh' : graphic_shape,
    'cdrw' : drawing_area,
    'glin' : graphic_line,
    'cpgn' : polygon,
    'cpxl' : pixel,
    'crrc' : rounded_rectangle,
    'cgob' : graphic_object,
    'carc' : arc,
    'cpix' : pixel_map,
    'crec' : rectangle,
    'csel' : selection_2d_object,
    'capp' : application,
    'docu' : document,
    'cwin' : window,
    'file' : file,
    'alis' : alias,
    'cins' : insertion_point,
    'cha ' : character,
    'cpar' : paragraph,
    'cwor' : word,
    'cflo' : text_flow,
    'tsty' : text_style_info,
    'clin' : line,
    'ctxt' : text,
    'cat ' : AppleTalk_address,
    'cadr' : address_specification,
    'ctok' : Token_Ring_address,
    'cfw ' : FireWire_address,
    'cbus' : bus_slot,
    'cscs' : SCSI_address,
    'cadb' : ADB_address,
    'cusb' : USB_address,
    'cdev' : device_specification,
    'clt ' : LocalTalk_address,
    'cip ' : IP_address,
    'cen ' : Ethernet_address,
    'cpic' : graphic_group,
    'cdrw' : drawing_area,
    'ccel' : cell,
    'ccol' : column,
    'ctbl' : table,
    'crow' : row,
    'shor' : small_integer,
    'aeut' : system_dictionary,
    'clrt' : color_table,
    'fpnt' : fixed_point,
    'TEXT' : plain_text,
    'elin' : type_element_info,
    'mLoc' : machine_location,
    'EPS ' : PostScript_picture,
    'suin' : type_suite_info,
    'cmen' : menu_item,
    'tpmm' : pixel_map_record,
    'sing' : small_real,
    'null' : null,
    'trot' : rotation,
    'fixd' : fixed,
    'lpnt' : long_point,
    'targ' : target_id,
    'pinf' : type_property_info,
    'pmin' : type_parameter_info,
    'lfpt' : long_fixed_point,
    'qdrt' : bounding_rectangle,
    'TIFF' : TIFF_picture,
    'lfxd' : long_fixed,
    'insl' : location_reference,
    'vers' : version,
    'tr16' : RGB16_color,
    'comp' : double_integer,
    'evin' : type_event_info,
    'QDpt' : point,
    'aete' : application_dictionary,
    'magn' : unsigned_integer,
    'cmnu' : menu,
    'frct' : fixed_rectangle,
    'lfrc' : long_fixed_rectangle,
    'gcli' : type_class_info,
    'tr96' : RGB96_color,
    'tdas' : dash_style,
    'styl' : scrap_styles,
    'exte' : extended_real,
    'lrct' : long_rectangle,
    'may ' : May,
    'TEXT' : string,
    'mile' : miles,
    'nd  ' : number_or_date,
    'oct ' : October,
    'evnt' : event,
    'pstr' : Pascal_string,
    'zone' : zone,
    'PICT' : picture,
    'ls  ' : list_or_string,
    'nmbr' : number,
    'tue ' : Tuesday,
    'vers' : version,
    'dec ' : December,
    'sqkm' : square_kilometres,
    'obj ' : reference,
    'vect' : vector,
    'wkdy' : weekday,
    'sun ' : Sunday,
    'itxt' : international_text,
    'scnd' : seconds,
    'cRGB' : RGB_color,
    'kmtr' : kilometres,
    'sutx' : styled_Unicode_text,
    'msng' : missing_value,
    'metr' : metres,
    'ns  ' : number_or_string,
    'list' : list,
    'llst' : linked_list,
    'doub' : real,
    'encs' : encoded_string,
    'lr  ' : list_or_record,
    'mon ' : Monday,
    'sep ' : September,
    '****' : anything,
    'prop' : property,
    'kfrm' : reference_form,
    'cobj' : item,
    'gram' : grams,
    'reco' : record,
    'undf' : empty_ae_name_,
    'enum' : constant,
    'sqmi' : square_miles,
    'rdat' : data,
    'utxt' : Unicode_text,
    'yard' : yards,
    'cyrd' : cubic_yards,
    'lbs ' : pounds,
    'ccmt' : cubic_centimetres,
    'ctxt' : text,
    'jul ' : July,
    'cmet' : cubic_metres,
    'STXT' : styled_text,
    'nds ' : number_2c__date_or_text,
    'feet' : feet,
    'feb ' : February,
    'degc' : degrees_Celsius,
    'kprs' : keystroke,
    'long' : integer,
    'degf' : degrees_Fahrenheit,
    'lrs ' : list_2c__record_or_text,
    'ldt ' : date,
    'degk' : degrees_Kelvin,
    'cmtr' : centimetres,
    'psct' : writing_code,
    'sf  ' : alias_or_string,
    'citl' : writing_code_info,
    'citm' : text_item,
    'mach' : machine,
    'type' : type_class,
    'prep' : preposition,
    'wed ' : Wednesday,
    'case' : upper_case,
    'mar ' : March,
    'sqft' : square_feet,
    'nov ' : November,
    'qrts' : quarts,
    'alis' : alias,
    'jan ' : January,
    'mnth' : month,
    'jun ' : June,
    'aug ' : August,
    'styl' : styled_Clipboard_text,
    'galn' : gallons,
    'cuin' : cubic_inches,
    'fri ' : Friday,
    'snd ' : sound,
    'pcls' : class_,
    'kgrm' : kilograms,
    'scpt' : script,
    'litr' : litres,
    'bool' : boolean,
    'sqrm' : square_metres,
    'inch' : inches,
    'cha ' : character,
    'apr ' : April,
    'ozs ' : ounces,
    'capp' : app,
    'hand' : handler,
    'cstr' : C_string,
    'thu ' : Thursday,
    'sqyd' : square_yards,
    'cfet' : cubic_feet,
    'sat ' : Saturday,
    'fss ' : file_specification,
}


class StdSuites(Text_Suite_Events,
        AppleScript_Suite_Events,
        Standard_Suite_Events,
        Macintosh_Connectivity_Clas_Events,
        QuickDraw_Graphics_Suite_Events,
        QuickDraw_Graphics_Suppleme_Events,
        Required_Suite_Events,
        Table_Suite_Events,
        Type_Names_Suite_Events,
        aetools.TalkTo):
    _signature = 'ascr'

    _moduleName = 'StdSuites'
