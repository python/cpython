"""Suite Files and suitcases: Classes representing files and suitcases
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Files_and_suitcases_Events:

	pass


class alias_file(aetools.ComponentItem):
	"""alias file - An alias file (created with \xd2Make Alias\xd3) """
	want = 'alia'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the file class """
	which = 'c@#^'
	want = 'file'
class original_item(aetools.NProperty):
	"""original item - the original item pointed to by the alias """
	which = 'orig'
	want = 'obj '

alias_files = alias_file

class application_file(aetools.ComponentItem):
	"""application file - An application's file on disk """
	want = 'appf'
class accepts_high_level_events(aetools.NProperty):
	"""accepts high level events - Is the application high-level event aware? """
	which = 'isab'
	want = 'bool'
class has_scripting_terminology(aetools.NProperty):
	"""has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
	which = 'hscr'
	want = 'bool'
class minimum_size(aetools.NProperty):
	"""minimum size - the smallest memory size with which the application can be launched """
	which = 'mprt'
	want = 'long'
class preferred_size(aetools.NProperty):
	"""preferred size - the memory size with which the application will be launched """
	which = 'appt'
	want = 'long'
class suggested_size(aetools.NProperty):
	"""suggested size - the memory size with which the developer recommends the application be launched """
	which = 'sprt'
	want = 'long'

application_files = application_file

class clipping(aetools.ComponentItem):
	"""clipping - A clipping """
	want = 'clpf'

clippings = clipping

class desk_accessory_file(aetools.ComponentItem):
	"""desk accessory file - A desk accessory file """
	want = 'dafi'

desk_accessory_files = desk_accessory_file

class desk_accessory_suitcase(aetools.ComponentItem):
	"""desk accessory suitcase - A desk accessory suitcase """
	want = 'dsut'
#        element 'cobj' as ['indx', 'name']

desk_accessory_suitcases = desk_accessory_suitcase

class document_file(aetools.ComponentItem):
	"""document file - A document file """
	want = 'docf'

document_files = document_file

class file(aetools.ComponentItem):
	"""file - A file """
	want = 'file'
class creator_type(aetools.NProperty):
	"""creator type - the OSType identifying the application that created the item """
	which = 'fcrt'
	want = 'type'
class file_type(aetools.NProperty):
	"""file type - the OSType identifying the type of data contained in the item """
	which = 'asty'
	want = 'type'
class locked(aetools.NProperty):
	"""locked - Is the file locked? """
	which = 'aslk'
	want = 'bool'
class product_version(aetools.NProperty):
	"""product version - the version of the product (visible at the top of the \xd2Get Info\xd3 window) """
	which = 'ver2'
	want = 'itxt'
class stationery(aetools.NProperty):
	"""stationery - Is the file a stationery pad? """
	which = 'pspd'
	want = 'bool'
class version(aetools.NProperty):
	"""version - the version of the file (visible at the bottom of the \xd2Get Info\xd3 window) """
	which = 'vers'
	want = 'itxt'

files = file

class font_file(aetools.ComponentItem):
	"""font file - A font file """
	want = 'fntf'

font_files = font_file

class font_suitcase(aetools.ComponentItem):
	"""font suitcase - A font suitcase """
	want = 'fsut'
#        element 'cobj' as ['indx', 'name']

font_suitcases = font_suitcase

class internet_location_file(aetools.ComponentItem):
	"""internet location file - An file containing an internet location """
	want = 'inlf'
class location(aetools.NProperty):
	"""location - the internet location """
	which = 'iloc'
	want = 'itxt'

internet_location_files = internet_location_file

class package(aetools.ComponentItem):
	"""package - A package """
	want = 'pack'

packages = package

class sound_file(aetools.ComponentItem):
	"""sound file - A sound file """
	want = 'sndf'
class sound(aetools.NProperty):
	"""sound - the sound data """
	which = 'snd '
	want = 'snd '

sound_files = sound_file

class suitcase(aetools.ComponentItem):
	"""suitcase - A font or desk accessory suitcase """
	want = 'stcs'
#        element 'cobj' as ['indx', 'name']

suitcases = suitcase
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
	'accepts_high_level_events' : accepts_high_level_events,
	'has_scripting_terminology' : has_scripting_terminology,
	'minimum_size' : minimum_size,
	'preferred_size' : preferred_size,
	'suggested_size' : suggested_size,
}
application_file._privelemdict = {
}
clipping._superclassnames = ['file']
clipping._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
clipping._privelemdict = {
}
desk_accessory_file._superclassnames = ['file']
desk_accessory_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_file._privelemdict = {
}
desk_accessory_suitcase._superclassnames = ['suitcase']
import Earlier_terms
desk_accessory_suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
desk_accessory_suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}
document_file._superclassnames = ['file']
document_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
document_file._privelemdict = {
}
file._superclassnames = ['item']
file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'creator_type' : creator_type,
	'file_type' : file_type,
	'locked' : locked,
	'product_version' : product_version,
	'stationery' : stationery,
	'version' : version,
}
file._privelemdict = {
}
font_file._superclassnames = ['file']
font_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_file._privelemdict = {
}
font_suitcase._superclassnames = ['suitcase']
font_suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
font_suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}
internet_location_file._superclassnames = ['file']
internet_location_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'location' : location,
}
internet_location_file._privelemdict = {
}
package._superclassnames = ['item']
package._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
package._privelemdict = {
}
sound_file._superclassnames = ['file']
sound_file._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'sound' : sound,
}
sound_file._privelemdict = {
}
suitcase._superclassnames = ['file']
suitcase._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
}
suitcase._privelemdict = {
	'item' : Earlier_terms.item,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'alia' : alias_file,
	'appf' : application_file,
	'clpf' : clipping,
	'dafi' : desk_accessory_file,
	'docf' : document_file,
	'dsut' : desk_accessory_suitcase,
	'file' : file,
	'fntf' : font_file,
	'fsut' : font_suitcase,
	'inlf' : internet_location_file,
	'pack' : package,
	'sndf' : sound_file,
	'stcs' : suitcase,
}

_propdeclarations = {
	'appt' : preferred_size,
	'aslk' : locked,
	'asty' : file_type,
	'c@#^' : _3c_Inheritance_3e_,
	'fcrt' : creator_type,
	'hscr' : has_scripting_terminology,
	'iloc' : location,
	'isab' : accepts_high_level_events,
	'mprt' : minimum_size,
	'orig' : original_item,
	'pspd' : stationery,
	'snd ' : sound,
	'sprt' : suggested_size,
	'ver2' : product_version,
	'vers' : version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
