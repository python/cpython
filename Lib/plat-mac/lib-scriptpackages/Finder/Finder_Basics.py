"""Suite Finder Basics: Commonly-used Finder commands and object classes
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_Basics_Events:

	_argmap_computer = {
		'has' : 'has ',
	}

	def computer(self, _object, _attributes={}, **_arguments):
		"""computer: Test attributes of this computer
		Required argument: the attribute to test
		Keyword argument has: test specific bits of response
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the result of the query
		"""
		_code = 'fndr'
		_subcode = 'gstl'

		aetools.keysubst(_arguments, self._argmap_computer)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def copy(self, _no_object=None, _attributes={}, **_arguments):
		"""copy: Copy the selected items to the clipboard (the Finder must be the front application)
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'copy'

		if _arguments: raise TypeError, 'No optional args expected'
		if _no_object != None: raise TypeError, 'No direct arg expected'


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

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

	_argmap_sort = {
		'by' : 'by  ',
	}

	def sort(self, _object, _attributes={}, **_arguments):
		"""sort: Return the specified object(s) in a sorted list
		Required argument: a list of finder objects to sort
		Keyword argument by: the property to sort the items by (name, index, date, etc.)
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the sorted items in their new order
		"""
		_code = 'DATA'
		_subcode = 'SORT'

		aetools.keysubst(_arguments, self._argmap_sort)
		_arguments['----'] = _object


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
class Finder_preferences(aetools.NProperty):
	"""Finder preferences - Various preferences that apply to the Finder as a whole """
	which = 'pfrp'
	want = 'cprf'
class about_this_computer(aetools.NProperty):
	"""about this computer - the \xd2About this Computer\xd3 dialog and the list of running processes displayed in it """
	which = 'abbx'
	want = 'obj '
class clipboard(aetools.NProperty):
	"""clipboard - the Finder\xd5s clipboard window """
	which = 'pcli'
	want = 'obj '
class desktop(aetools.NProperty):
	"""desktop - the desktop """
	which = 'desk'
	want = 'cdsk'
class execution_state(aetools.NProperty):
	"""execution state - the current execution state of the Finder """
	which = 'exec'
	want = 'ese0'
class file_sharing(aetools.NProperty):
	"""file sharing - Is file sharing on? """
	which = 'fshr'
	want = 'bool'
class frontmost(aetools.NProperty):
	"""frontmost - Is the Finder the frontmost process? """
	which = 'pisf'
	want = 'bool'
class insertion_location(aetools.NProperty):
	"""insertion location - the container in which a new folder would appear if \xd2New Folder\xd3 was selected """
	which = 'pins'
	want = 'obj '
class largest_free_block(aetools.NProperty):
	"""largest free block - the largest free block of process memory available to launch an application """
	which = 'mfre'
	want = 'long'
class name(aetools.NProperty):
	"""name - the Finder\xd5s name """
	which = 'pnam'
	want = 'itxt'
class product_version(aetools.NProperty):
	"""product version - the version of the System software running on this computer """
	which = 'ver2'
	want = 'itxt'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user """
	which = 'sele'
	want = 'obj '
class sharing_starting_up(aetools.NProperty):
	"""sharing starting up - Is file sharing in the process of starting up? """
	which = 'fsup'
	want = 'bool'
class version(aetools.NProperty):
	"""version - the version of the Finder """
	which = 'vers'
	want = 'itxt'
class visible(aetools.NProperty):
	"""visible - Is the Finder\xd5s layer visible? """
	which = 'pvis'
	want = 'bool'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'cdis' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'cwin' as ['indx', 'name']
#        element 'cwnd' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']
#        element 'dwnd' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'iwnd' as ['indx', 'name']
#        element 'lwnd' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'pcap' as ['indx', 'name']
#        element 'pcda' as ['indx', 'name']
#        element 'prcs' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'vwnd' as ['indx', 'name']

class special_folders(aetools.ComponentItem):
	"""special folders - The special folders used by the Mac OS """
	want = 'spfl'
class apple_menu_items_folder(aetools.NProperty):
	"""apple menu items folder - the special folder named \xd2Apple Menu Items,\xd3 the contents of which appear in the Apple menu """
	which = 'amnu'
	want = 'obj '
class control_panels_folder(aetools.NProperty):
	"""control panels folder - the special folder named \xd2Control Panels\xd3 """
	which = 'ctrl'
	want = 'obj '
class extensions_folder(aetools.NProperty):
	"""extensions folder - the special folder named \xd2Extensions\xd3 """
	which = 'extn'
	want = 'obj '
class fonts_folder(aetools.NProperty):
	"""fonts folder - the special folder named \xd2Fonts\xd3 """
	which = 'font'
	want = 'obj '
class preferences_folder(aetools.NProperty):
	"""preferences folder - the special folder named \xd2Preferences\xd3 """
	which = 'pref'
	want = 'obj '
class shutdown_items_folder(aetools.NProperty):
	"""shutdown items folder - the special folder named \xd2Shutdown Items\xd3 """
	which = 'shdf'
	want = 'obj '
class startup_items_folder(aetools.NProperty):
	"""startup items folder - the special folder named \xd2Startup Items\xd3 """
	which = 'strt'
	want = 'obj '
class system_folder(aetools.NProperty):
	"""system folder - the System folder """
	which = 'macs'
	want = 'obj '
class temporary_items_folder(aetools.NProperty):
	"""temporary items folder - the special folder named \xd2Temporary Items\xd3 (invisible) """
	which = 'temp'
	want = 'obj '
application._superclassnames = []
import Files_and_suitcases
import Containers_and_folders
import Earlier_terms
import Window_classes
import Process_classes
application._privpropdict = {
	'Finder_preferences' : Finder_preferences,
	'about_this_computer' : about_this_computer,
	'clipboard' : clipboard,
	'desktop' : desktop,
	'execution_state' : execution_state,
	'file_sharing' : file_sharing,
	'frontmost' : frontmost,
	'insertion_location' : insertion_location,
	'largest_free_block' : largest_free_block,
	'name' : name,
	'product_version' : product_version,
	'selection' : selection,
	'sharing_starting_up' : sharing_starting_up,
	'version' : version,
	'visible' : visible,
}
application._privelemdict = {
	'accessory_process' : Earlier_terms.accessory_process,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Files_and_suitcases.application_file,
	'application_process' : Process_classes.application_process,
	'clipping' : Files_and_suitcases.clipping,
	'clipping_window' : Window_classes.clipping_window,
	'container' : Containers_and_folders.container,
	'container_window' : Earlier_terms.container_window,
	'content_space' : Window_classes.content_space,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'desk_accessory_suitcase' : Files_and_suitcases.desk_accessory_suitcase,
	'disk' : Containers_and_folders.disk,
	'document_file' : Files_and_suitcases.document_file,
	'file' : Files_and_suitcases.file,
	'folder' : Containers_and_folders.folder,
	'font_file' : Files_and_suitcases.font_file,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'information_window' : Earlier_terms.information_window,
	'internet_location_file' : Files_and_suitcases.internet_location_file,
	'item' : Earlier_terms.item,
	'package' : Files_and_suitcases.package,
	'process' : Earlier_terms.process,
	'sharable_container' : Containers_and_folders.sharable_container,
	'sound_file' : Files_and_suitcases.sound_file,
	'suitcase' : Files_and_suitcases.suitcase,
	'view_options_window' : Window_classes.view_options_window,
	'window' : Earlier_terms.window,
}
special_folders._superclassnames = []
special_folders._privpropdict = {
	'apple_menu_items_folder' : apple_menu_items_folder,
	'control_panels_folder' : control_panels_folder,
	'extensions_folder' : extensions_folder,
	'fonts_folder' : fonts_folder,
	'preferences_folder' : preferences_folder,
	'shutdown_items_folder' : shutdown_items_folder,
	'startup_items_folder' : startup_items_folder,
	'system_folder' : system_folder,
	'temporary_items_folder' : temporary_items_folder,
}
special_folders._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'capp' : application,
	'spfl' : special_folders,
}

_propdeclarations = {
	'abbx' : about_this_computer,
	'amnu' : apple_menu_items_folder,
	'ctrl' : control_panels_folder,
	'desk' : desktop,
	'exec' : execution_state,
	'extn' : extensions_folder,
	'font' : fonts_folder,
	'fshr' : file_sharing,
	'fsup' : sharing_starting_up,
	'macs' : system_folder,
	'mfre' : largest_free_block,
	'pcli' : clipboard,
	'pfrp' : Finder_preferences,
	'pins' : insertion_location,
	'pisf' : frontmost,
	'pnam' : name,
	'pref' : preferences_folder,
	'pvis' : visible,
	'sele' : selection,
	'shdf' : shutdown_items_folder,
	'strt' : startup_items_folder,
	'temp' : temporary_items_folder,
	'ver2' : product_version,
	'vers' : version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
