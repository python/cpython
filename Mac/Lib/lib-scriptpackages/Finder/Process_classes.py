"""Suite Process classes: Classes representing processes that are running
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Process_classes_Events:

	pass


class process(aetools.ComponentItem):
	"""process - A process running on this computer """
	want = 'prcs'
class name(aetools.NProperty):
	"""name - the name of the process """
	which = 'pnam'
	want = 'itxt'
class visible(aetools.NProperty):
	"""visible - Is the process' layer visible? """
	which = 'pvis'
	want = 'bool'
class frontmost(aetools.NProperty):
	"""frontmost - Is the process the frontmost process? """
	which = 'pisf'
	want = 'bool'
class file(aetools.NProperty):
	"""file - the file from which the process was launched """
	which = 'file'
	want = 'obj '
class file_type(aetools.NProperty):
	"""file type - the OSType of the file type of the process """
	which = 'asty'
	want = 'type'
class creator_type(aetools.NProperty):
	"""creator type - the OSType of the creator of the process (the signature) """
	which = 'fcrt'
	want = 'type'
class accepts_high_level_events(aetools.NProperty):
	"""accepts high level events - Is the process high-level event aware (accepts open application, open document, print document, and quit)? """
	which = 'isab'
	want = 'bool'
class accepts_remote_events(aetools.NProperty):
	"""accepts remote events - Does the process accept remote events? """
	which = 'revt'
	want = 'bool'
class has_scripting_terminology(aetools.NProperty):
	"""has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
	which = 'hscr'
	want = 'bool'
class total_partition_size(aetools.NProperty):
	"""total partition size - the size of the partition with which the process was launched """
	which = 'appt'
	want = 'long'
class partition_space_used(aetools.NProperty):
	"""partition space used - the number of bytes currently used in the process' partition """
	which = 'pusd'
	want = 'long'

processes = process

class application_process(aetools.ComponentItem):
	"""application process - A process launched from an application file """
	want = 'pcap'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - inherits some of its properties from the process class """
	which = 'c@#^'
	want = 'prcs'
class application_file(aetools.NProperty):
	"""application file - the application file from which this process was launched """
	which = 'appf'
	want = 'appf'

application_processes = application_process

class desk_accessory_process(aetools.ComponentItem):
	"""desk accessory process - A process launched from a desk accessory file """
	want = 'pcda'
class desk_accessory_file(aetools.NProperty):
	"""desk accessory file - the desk accessory file from which this process was launched """
	which = 'dafi'
	want = 'obj '

desk_accessory_processes = desk_accessory_process
process._propdict = {
	'name' : name,
	'visible' : visible,
	'frontmost' : frontmost,
	'file' : file,
	'file_type' : file_type,
	'creator_type' : creator_type,
	'accepts_high_level_events' : accepts_high_level_events,
	'accepts_remote_events' : accepts_remote_events,
	'has_scripting_terminology' : has_scripting_terminology,
	'total_partition_size' : total_partition_size,
	'partition_space_used' : partition_space_used,
}
process._elemdict = {
}
application_process._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'application_file' : application_file,
}
application_process._elemdict = {
}
desk_accessory_process._propdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'desk_accessory_file' : desk_accessory_file,
}
desk_accessory_process._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'pcda' : desk_accessory_process,
	'pcap' : application_process,
	'prcs' : process,
}

_propdeclarations = {
	'revt' : accepts_remote_events,
	'appf' : application_file,
	'pnam' : name,
	'file' : file,
	'pusd' : partition_space_used,
	'fcrt' : creator_type,
	'c@#^' : _3c_Inheritance_3e_,
	'asty' : file_type,
	'hscr' : has_scripting_terminology,
	'dafi' : desk_accessory_file,
	'isab' : accepts_high_level_events,
	'appt' : total_partition_size,
	'pisf' : frontmost,
	'pvis' : visible,
}

_compdeclarations = {
}

_enumdeclarations = {
}
