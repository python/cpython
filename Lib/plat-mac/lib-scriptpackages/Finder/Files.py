"""Suite Files: Classes representing files
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Files_Events:

    pass


class alias_file(aetools.ComponentItem):
    """alias file - An alias file (created with \xd2Make Alias\xd3) """
    want = 'alia'
class _Prop__3c_Inheritance_3e_(aetools.NProperty):
    """<Inheritance> - inherits some of its properties from the file class """
    which = 'c@#^'
    want = 'file'
class _Prop_original_item(aetools.NProperty):
    """original item - the original item pointed to by the alias """
    which = 'orig'
    want = 'obj '

alias_files = alias_file

class application_file(aetools.ComponentItem):
    """application file - An application's file on disk """
    want = 'appf'
class _Prop_accepts_high_level_events(aetools.NProperty):
    """accepts high level events - Is the application high-level event aware? (OBSOLETE: always returns true) """
    which = 'isab'
    want = 'bool'
class _Prop_has_scripting_terminology(aetools.NProperty):
    """has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
    which = 'hscr'
    want = 'bool'
class _Prop_minimum_size(aetools.NProperty):
    """minimum size - the smallest memory size with which the application can be launched """
    which = 'mprt'
    want = 'long'
class _Prop_opens_in_Classic(aetools.NProperty):
    """opens in Classic - Should the application launch in the Classic environment? """
    which = 'Clsc'
    want = 'bool'
class _Prop_preferred_size(aetools.NProperty):
    """preferred size - the memory size with which the application will be launched """
    which = 'appt'
    want = 'long'
class _Prop_suggested_size(aetools.NProperty):
    """suggested size - the memory size with which the developer recommends the application be launched """
    which = 'sprt'
    want = 'long'

application_files = application_file

class clipping(aetools.ComponentItem):
    """clipping - A clipping """
    want = 'clpf'
class _Prop_clipping_window(aetools.NProperty):
    """clipping window - (NOT AVAILABLE YET) the clipping window for this clipping """
    which = 'lwnd'
    want = 'obj '

clippings = clipping

class document_file(aetools.ComponentItem):
    """document file - A document file """
    want = 'docf'

document_files = document_file

class file(aetools.ComponentItem):
    """file - A file """
    want = 'file'
class _Prop_creator_type(aetools.NProperty):
    """creator type - the OSType identifying the application that created the item """
    which = 'fcrt'
    want = 'type'
class _Prop_file_type(aetools.NProperty):
    """file type - the OSType identifying the type of data contained in the item """
    which = 'asty'
    want = 'type'
class _Prop_product_version(aetools.NProperty):
    """product version - the version of the product (visible at the top of the \xd2Get Info\xd3 window) """
    which = 'ver2'
    want = 'utxt'
class _Prop_stationery(aetools.NProperty):
    """stationery - Is the file a stationery pad? """
    which = 'pspd'
    want = 'bool'
class _Prop_version(aetools.NProperty):
    """version - the version of the file (visible at the bottom of the \xd2Get Info\xd3 window) """
    which = 'vers'
    want = 'utxt'

files = file

class internet_location_file(aetools.ComponentItem):
    """internet location file - An file containing an internet location """
    want = 'inlf'
class _Prop_location(aetools.NProperty):
    """location - the internet location """
    which = 'iloc'
    want = 'utxt'

internet_location_files = internet_location_file

class package(aetools.ComponentItem):
    """package - A package """
    want = 'pack'

packages = package
alias_file._superclassnames = ['file']
alias_file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'original_item' : _Prop_original_item,
}
alias_file._privelemdict = {
}
application_file._superclassnames = ['file']
application_file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'accepts_high_level_events' : _Prop_accepts_high_level_events,
    'has_scripting_terminology' : _Prop_has_scripting_terminology,
    'minimum_size' : _Prop_minimum_size,
    'opens_in_Classic' : _Prop_opens_in_Classic,
    'preferred_size' : _Prop_preferred_size,
    'suggested_size' : _Prop_suggested_size,
}
application_file._privelemdict = {
}
clipping._superclassnames = ['file']
clipping._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'clipping_window' : _Prop_clipping_window,
}
clipping._privelemdict = {
}
document_file._superclassnames = ['file']
document_file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
document_file._privelemdict = {
}
import Finder_items
file._superclassnames = ['item']
file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'creator_type' : _Prop_creator_type,
    'file_type' : _Prop_file_type,
    'product_version' : _Prop_product_version,
    'stationery' : _Prop_stationery,
    'version' : _Prop_version,
}
file._privelemdict = {
}
internet_location_file._superclassnames = ['file']
internet_location_file._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
    'location' : _Prop_location,
}
internet_location_file._privelemdict = {
}
package._superclassnames = ['item']
package._privpropdict = {
    '_3c_Inheritance_3e_' : _Prop__3c_Inheritance_3e_,
}
package._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
    'alia' : alias_file,
    'appf' : application_file,
    'clpf' : clipping,
    'docf' : document_file,
    'file' : file,
    'inlf' : internet_location_file,
    'pack' : package,
}

_propdeclarations = {
    'Clsc' : _Prop_opens_in_Classic,
    'appt' : _Prop_preferred_size,
    'asty' : _Prop_file_type,
    'c@#^' : _Prop__3c_Inheritance_3e_,
    'fcrt' : _Prop_creator_type,
    'hscr' : _Prop_has_scripting_terminology,
    'iloc' : _Prop_location,
    'isab' : _Prop_accepts_high_level_events,
    'lwnd' : _Prop_clipping_window,
    'mprt' : _Prop_minimum_size,
    'orig' : _Prop_original_item,
    'pspd' : _Prop_stationery,
    'sprt' : _Prop_suggested_size,
    'ver2' : _Prop_product_version,
    'vers' : _Prop_version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
