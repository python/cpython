"""Suite Files and suitcases: Classes representing files and suitcases
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Files_and_suitcases_Events:

	pass


class file(aetools.ComponentItem):
	"""file - A file """
	want = 'file'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the item class """
	which = 'c@#^'
	want = 'cobj'
class file_type(aetools.NProperty):
	"""file type - the OSType identifying the type of data contained in the item """
	which = 'asty'
	want = 'type'
class creator_type(aetools.NProperty):
	"""creator type - the OSType identifying the application that created the item """
	which = 'fcrt'
	want = 'type'
class locked(aetools.NProperty):
	"""locked - Is the file locked? """
	which = 'aslk'
	want = 'bool'
class stationery(aetools.NProperty):
	"""stationery - Is the file a stationery pad? """
	which = 'pspd'
	want = 'bool'
class product_version(aetools.NProperty):
	"""product version - the version of the product (visible at the top of the –Get Info” window) """
	which = 'ver2'
	want = 'itxt'
class version(aetools.NProperty):
	"""version - the version of the file (visible at the bottom of the –Get Info” window) """
	which = 'vers'
	want = 'itxt'

files = file

class alias_file(aetools.ComponentItem):
	"""alias file - An alias file (created with –Make Alias”) """
	want = 'alia'
class original_item(aetools.NProperty):
	"""original item - the original item pointed to by the alias """
	which = 'orig'
	want = 'obj '

alias_files = alias_file

class application_file(aetools.ComponentItem):
	"""application file - An application's file on disk """
	want = 'appf'
class suggested_size(aetools.NProperty):
	"""suggested size - the memory size with which the developer recommends the application be launched """
	which = 'sprt'
	want = 'long'
class minimum_size(aetools.NProperty):
	"""minimum size - the smallest memory size with which the application can be launched """
	which = 'mprt'
	want = 'long'
class preferred_size(aetools.NProperty):
	"""preferred size - the memory size with which the application will be launched """
	which = 'appt'
	want = 'long'
class accepts_high_level_events(aetools.NProperty):
	"""accepts high level events - Is the application high-level event aware? """
	which = 'isab'
	want = 'bool'
class has_scripting_terminology(aetools.NProperty):
	"""has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
	which = 'hscr'
	want = 'bool'

application_files = application_file

class document_file(aetools.ComponentItem):
	"""document file - A document file """
	want = 'docf'

document_files = document_file

class font_file(aetools.ComponentItem):
	"""font file - A font file """
	want = 'fntf'

font_files = font_file

class desk_accessory_file(aetools.ComponentItem):
	"""desk accessory file - A desk accessory file """
	want = 'dafi'

desk_accessory_files = desk_accessory_file

class internet_location_file(aetools.ComponentItem):
	"""internet location file - An file containing an internet location """
	want = 'inlf'
class location(aetools.NProperty):
	"""location - the internet location """
	which = 'iloc'
	want = 'itxt'

internet_location_files = internet_location_file

class sound_file(aetools.ComponentItem):
	"""sound file - A sound file """
	want = 'sndf'
class sound(aetools.NProperty):
	"""sound - the sound data """
	which = 'snd '
	want = 'snd '

sound_files = sound_file

class clipping(aetools.ComponentItem):
	"""clipping - A clipping """
	want = 'clpf'

clippings = clipping

class package(aetools.ComponentItem):
	"""package - A package """
	want = 'pack'

packages = package

class suitcase(aetools.ComponentItem):
	"""suitcase - A font or desk accessory suitcase """
	want = 'stcs'
#        element 'cobj' as ['indx', 'name']

suitcases = suitcase

class font_suitcase(aetools.ComponentItem):
	"""font suitcase - A font suitcase """
	want = 'fsut'
#        element 'cobj' as ['indx', 'name']

font_suitcases = font_suitcase

class desk_accessory_suitcase(aetools.ComponentItem):
	"""desk accessory suitcase - A desk accessory suitcase """
	want = 'dsut'
#        element 'cobj' as ['indx', 'name']

desk_accessory_suitcases = desk_accessory_suitcase
file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'file_type' : file_type,
	'creator_type' : creator_type,
	'locked' : locked,
	'stationery' : stationery,
	'product_version' : product_version,
	'version' : version,
}
file._elemdict = {
}
alias_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'original_item' : original_item,
}
alias_file._elemdict = {
}
application_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'suggested_size' : suggested_size,
	'minimum_size' : minimum_size,
	'preferred_size' : preferred_size,
	'accepts_high_level_events' : accepts_high_level_events,
	'has_scripting_terminology' : has_scripting_terminology,
}
application_file._elemdict = {
}
document_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
document_file._elemdict = {
}
font_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_file._elemdict = {
}
desk_accessory_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_file._elemdict = {
}
internet_location_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'location' : location,
}
internet_location_file._elemdict = {
}
sound_file._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'sound' : sound,
}
sound_file._elemdict = {
}
clipping._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
clipping._elemdict = {
}
package._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
package._elemdict = {
}
import Earlier_terms
suitcase._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
suitcase._elemdict = {
	'item' : Earlier_terms.item,
}
font_suitcase._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_suitcase._elemdict = {
	'item' : Earlier_terms.item,
}
desk_accessory_suitcase._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_suitcase._elemdict = {
	'item' : Earlier_terms.item,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'clpf' : clipping,
	'docf' : document_file,
	'stcs' : suitcase,
	'appf' : application_file,
	'file' : file,
	'fsut' : font_suitcase,
	'pack' : package,
	'dafi' : desk_accessory_file,
	'alia' : alias_file,
	'dsut' : desk_accessory_suitcase,
	'inlf' : internet_location_file,
	'fntf' : font_file,
	'sndf' : sound_file,
}

_propdeclarations = {
	'orig' : original_item,
	'pspd' : stationery,
	'aslk' : locked,
	'iloc' : location,
	'mprt' : minimum_size,
	'fcrt' : creator_type,
	'c@#^' : _3c_Inheritance_3e_,
	'asty' : file_type,
	'hscr' : has_scripting_terminology,
	'sprt' : suggested_size,
	'appt' : preferred_size,
	'isab' : accepts_high_level_events,
	'snd ' : sound,
	'ver2' : product_version,
	'vers' : version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
