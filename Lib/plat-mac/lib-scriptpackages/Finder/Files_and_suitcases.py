"""Suite Files and suitcases: Classes representing files and suitcases
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Finder
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
	"""product version - the version of the product (visible at the top of the \xd2Get Info\xd3 window) """
	which = 'ver2'
	want = 'itxt'
class version(aetools.NProperty):
	"""version - the version of the file (visible at the bottom of the \xd2Get Info\xd3 window) """
	which = 'vers'
	want = 'itxt'

files = file

class alias_file(aetools.ComponentItem):
	"""alias file - An alias file (created with \xd2Make Alias\xd3) """
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
import Earlier_terms
file._superclassnames = ['item']
file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'file_type' : file_type,
	'creator_type' : creator_type,
	'locked' : locked,
	'stationery' : stationery,
	'product_version' : product_version,
	'version' : version,
}
file._privelemdict = {
}
alias_file._superclassnames = ['file']
alias_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'original_item' : original_item,
}
alias_file._privelemdict = {
}
application_file._superclassnames = ['file']
application_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'suggested_size' : suggested_size,
	'minimum_size' : minimum_size,
	'preferred_size' : preferred_size,
	'accepts_high_level_events' : accepts_high_level_events,
	'has_scripting_terminology' : has_scripting_terminology,
}
application_file._privelemdict = {
}
document_file._superclassnames = ['file']
document_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
document_file._privelemdict = {
}
font_file._superclassnames = ['file']
font_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_file._privelemdict = {
}
desk_accessory_file._superclassnames = ['file']
desk_accessory_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_file._privelemdict = {
}
internet_location_file._superclassnames = ['file']
internet_location_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'location' : location,
}
internet_location_file._privelemdict = {
}
sound_file._superclassnames = ['file']
sound_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'sound' : sound,
}
sound_file._privelemdict = {
}
clipping._superclassnames = ['file']
clipping._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
clipping._privelemdict = {
}
package._superclassnames = ['item']
package._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
package._privelemdict = {
}
suitcase._superclassnames = ['file']
suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}
font_suitcase._superclassnames = ['suitcase']
font_suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}
desk_accessory_suitcase._superclassnames = ['suitcase']
desk_accessory_suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
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
}

_propdeclarations = {
	'vers' : version,
	'ver2' : product_version,
	'snd ' : sound,
	'appt' : preferred_size,
	'sprt' : suggested_size,
	'isab' : accepts_high_level_events,
	'hscr' : has_scripting_terminology,
	'asty' : file_type,
	'c@#^' : _3c_Inheritance_3e_,
	'fcrt' : creator_type,
	'mprt' : minimum_size,
	'pspd' : stationery,
	'iloc' : location,
	'aslk' : locked,
	'orig' : original_item,
}

_compdeclarations = {
}

_enumdeclarations = {
}
