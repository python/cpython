"""Suite Legacy suite: Operations formerly handled by the Finder, but now automatically delegated to other applications
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fleg'

class Legacy_suite_Events:

	def restart(self, _no_object=None, _attributes={}, **_arguments):
		"""restart: Restart the computer
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'rest'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def shut_down(self, _no_object=None, _attributes={}, **_arguments):
		"""shut down: Shut Down the computer
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'shut'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def sleep(self, _no_object=None, _attributes={}, **_arguments):
		"""sleep: Put the computer to sleep
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'slep'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - The Finder """
	want = 'capp'
class desktop_picture(aetools.NProperty):
	"""desktop picture - the desktop picture of the main monitor """
	which = 'dpic'
	want = 'file'

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

class process(aetools.ComponentItem):
	"""process - A process running on this computer """
	want = 'prcs'
class accepts_high_level_events(aetools.NProperty):
	"""accepts high level events - Is the process high-level event aware (accepts open application, open document, print document, and quit)? """
	which = 'isab'
	want = 'bool'
class accepts_remote_events(aetools.NProperty):
	"""accepts remote events - Does the process accept remote events? """
	which = 'revt'
	want = 'bool'
class creator_type(aetools.NProperty):
	"""creator type - the OSType of the creator of the process (the signature) """
	which = 'fcrt'
	want = 'type'
class file(aetools.NProperty):
	"""file - the file from which the process was launched """
	which = 'file'
	want = 'obj '
class file_type(aetools.NProperty):
	"""file type - the OSType of the file type of the process """
	which = 'asty'
	want = 'type'
class frontmost(aetools.NProperty):
	"""frontmost - Is the process the frontmost process? """
	which = 'pisf'
	want = 'bool'
class has_scripting_terminology(aetools.NProperty):
	"""has scripting terminology - Does the process have a scripting terminology, i.e., can it be scripted? """
	which = 'hscr'
	want = 'bool'
class name(aetools.NProperty):
	"""name - the name of the process """
	which = 'pnam'
	want = 'itxt'
class partition_space_used(aetools.NProperty):
	"""partition space used - the number of bytes currently used in the process' partition """
	which = 'pusd'
	want = 'long'
class total_partition_size(aetools.NProperty):
	"""total partition size - the size of the partition with which the process was launched """
	which = 'appt'
	want = 'long'
class visible(aetools.NProperty):
	"""visible - Is the process' layer visible? """
	which = 'pvis'
	want = 'bool'

processes = process
application._superclassnames = []
application._privpropdict = {
	'desktop_picture' : desktop_picture,
}
application._privelemdict = {
}
application_process._superclassnames = ['process']
application_process._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'application_file' : application_file,
}
application_process._privelemdict = {
}
desk_accessory_process._superclassnames = ['process']
desk_accessory_process._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'desk_accessory_file' : desk_accessory_file,
}
desk_accessory_process._privelemdict = {
}
process._superclassnames = []
process._privpropdict = {
	'accepts_high_level_events' : accepts_high_level_events,
	'accepts_remote_events' : accepts_remote_events,
	'creator_type' : creator_type,
	'file' : file,
	'file_type' : file_type,
	'frontmost' : frontmost,
	'has_scripting_terminology' : has_scripting_terminology,
	'name' : name,
	'partition_space_used' : partition_space_used,
	'total_partition_size' : total_partition_size,
	'visible' : visible,
}
process._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'capp' : application,
	'pcap' : application_process,
	'pcda' : desk_accessory_process,
	'prcs' : process,
}

_propdeclarations = {
	'appf' : application_file,
	'appt' : total_partition_size,
	'asty' : file_type,
	'c@#^' : _3c_Inheritance_3e_,
	'dafi' : desk_accessory_file,
	'dpic' : desktop_picture,
	'fcrt' : creator_type,
	'file' : file,
	'hscr' : has_scripting_terminology,
	'isab' : accepts_high_level_events,
	'pisf' : frontmost,
	'pnam' : name,
	'pusd' : partition_space_used,
	'pvis' : visible,
	'revt' : accepts_remote_events,
}

_compdeclarations = {
}

_enumdeclarations = {
}
