"""Suite Finder Basics: Commonly-used Finder commands and object classes
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Finder
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
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
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class application(aetools.ComponentItem):
	"""application - The Finder """
	want = 'capp'
class clipboard(aetools.NProperty):
	"""clipboard - the FinderÕs clipboard window """
	which = 'pcli'
	want = 'obj '
class largest_free_block(aetools.NProperty):
	"""largest free block - the largest free block of process memory available to launch an application """
	which = 'mfre'
	want = 'long'
class name(aetools.NProperty):
	"""name - the FinderÕs name """
	which = 'pnam'
	want = 'itxt'
class visible(aetools.NProperty):
	"""visible - Is the FinderÕs layer visible? """
	which = 'pvis'
	want = 'bool'
class frontmost(aetools.NProperty):
	"""frontmost - Is the Finder the frontmost process? """
	which = 'pisf'
	want = 'bool'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user """
	which = 'sele'
	want = 'obj '
class insertion_location(aetools.NProperty):
	"""insertion location - the container in which a new folder would appear if –New Folder” was selected """
	which = 'pins'
	want = 'obj '
class file_sharing(aetools.NProperty):
	"""file sharing - Is file sharing on? """
	which = 'fshr'
	want = 'bool'
class sharing_starting_up(aetools.NProperty):
	"""sharing starting up - Is file sharing in the process of starting up? """
	which = 'fsup'
	want = 'bool'
class product_version(aetools.NProperty):
	"""product version - the version of the System software running on this computer """
	which = 'ver2'
	want = 'itxt'
class version(aetools.NProperty):
	"""version - the version of the Finder """
	which = 'vers'
	want = 'itxt'
class about_this_computer(aetools.NProperty):
	"""about this computer - the –About this Computer” dialog and the list of running processes displayed in it """
	which = 'abbx'
	want = 'obj '
class desktop(aetools.NProperty):
	"""desktop - the desktop """
	which = 'desk'
	want = 'cdsk'
class Finder_preferences(aetools.NProperty):
	"""Finder preferences - Various preferences that apply to the Finder as a whole """
	which = 'pfrp'
	want = 'cprf'
#        element 'cobj' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'cdis' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'file' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'clpf' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'dsut' as ['indx', 'name']
#        element 'prcs' as ['indx', 'name']
#        element 'pcap' as ['indx', 'name']
#        element 'pcda' as ['indx', 'name']
#        element 'cwin' as ['indx', 'name']
#        element 'cwnd' as ['indx', 'name']
#        element 'iwnd' as ['indx', 'name']
#        element 'lwnd' as ['indx', 'name']
#        element 'dwnd' as ['indx', 'name']

class special_folders(aetools.ComponentItem):
	"""special folders - The special folders used by the Mac OS """
	want = 'spfl'
class system_folder(aetools.NProperty):
	"""system folder - the System folder """
	which = 'macs'
	want = 'obj '
class apple_menu_items_folder(aetools.NProperty):
	"""apple menu items folder - the special folder named –Apple Menu Items,” the contents of which appear in the Apple menu """
	which = 'amnu'
	want = 'obj '
class control_panels_folder(aetools.NProperty):
	"""control panels folder - the special folder named –Control Panels” """
	which = 'ctrl'
	want = 'obj '
class extensions_folder(aetools.NProperty):
	"""extensions folder - the special folder named –Extensions” """
	which = 'extn'
	want = 'obj '
class fonts_folder(aetools.NProperty):
	"""fonts folder - the special folder named –Fonts” """
	which = 'font'
	want = 'obj '
class preferences_folder(aetools.NProperty):
	"""preferences folder - the special folder named –Preferences” """
	which = 'pref'
	want = 'obj '
class shutdown_items_folder(aetools.NProperty):
	"""shutdown items folder - the special folder named –Shutdown Items” """
	which = 'shdf'
	want = 'obj '
class startup_items_folder(aetools.NProperty):
	"""startup items folder - the special folder named –Startup Items” """
	which = 'strt'
	want = 'obj '
class temporary_items_folder(aetools.NProperty):
	"""temporary items folder - the special folder named –Temporary Items” (invisible) """
	which = 'temp'
	want = 'obj '
import Earlier_terms
import Containers_and_folders
import Files_and_suitcases
import Process_classes
import Window_classes
application._propdict = {
	'clipboard' : clipboard,
	'largest_free_block' : largest_free_block,
	'name' : name,
	'visible' : visible,
	'frontmost' : frontmost,
	'selection' : selection,
	'insertion_location' : insertion_location,
	'file_sharing' : file_sharing,
	'sharing_starting_up' : sharing_starting_up,
	'product_version' : product_version,
	'version' : version,
	'about_this_computer' : about_this_computer,
	'desktop' : desktop,
	'Finder_preferences' : Finder_preferences,
}
application._elemdict = {
	'item' : Earlier_terms.item,
	'container' : Containers_and_folders.container,
	'sharable_container' : Earlier_terms.sharable_container,
	'disk' : Containers_and_folders.disk,
	'folder' : Containers_and_folders.folder,
	'file' : Files_and_suitcases.file,
	'alias_file' : Files_and_suitcases.alias_file,
	'application_file' : Earlier_terms.application_file,
	'document_file' : Files_and_suitcases.document_file,
	'font_file' : Files_and_suitcases.font_file,
	'desk_accessory_file' : Files_and_suitcases.desk_accessory_file,
	'internet_location' : Earlier_terms.internet_location,
	'sound_file' : Files_and_suitcases.sound_file,
	'clipping' : Files_and_suitcases.clipping,
	'package' : Files_and_suitcases.package,
	'suitcase' : Files_and_suitcases.suitcase,
	'font_suitcase' : Files_and_suitcases.font_suitcase,
	'accessory_suitcase' : Earlier_terms.accessory_suitcase,
	'process' : Earlier_terms.process,
	'application_process' : Process_classes.application_process,
	'accessory_process' : Earlier_terms.accessory_process,
	'window' : Earlier_terms.window,
	'container_window' : Earlier_terms.container_window,
	'information_window' : Earlier_terms.information_window,
	'clipping_window' : Window_classes.clipping_window,
	'content_space' : Window_classes.content_space,
}
special_folders._propdict = {
	'system_folder' : system_folder,
	'apple_menu_items_folder' : apple_menu_items_folder,
	'control_panels_folder' : control_panels_folder,
	'extensions_folder' : extensions_folder,
	'fonts_folder' : fonts_folder,
	'preferences_folder' : preferences_folder,
	'shutdown_items_folder' : shutdown_items_folder,
	'startup_items_folder' : startup_items_folder,
	'temporary_items_folder' : temporary_items_folder,
}
special_folders._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'spfl' : special_folders,
	'capp' : application,
}

_propdeclarations = {
	'amnu' : apple_menu_items_folder,
	'extn' : extensions_folder,
	'pnam' : name,
	'fshr' : file_sharing,
	'pcli' : clipboard,
	'strt' : startup_items_folder,
	'pref' : preferences_folder,
	'pisf' : frontmost,
	'pins' : insertion_location,
	'pvis' : visible,
	'abbx' : about_this_computer,
	'temp' : temporary_items_folder,
	'font' : fonts_folder,
	'pfrp' : Finder_preferences,
	'desk' : desktop,
	'fsup' : sharing_starting_up,
	'mfre' : largest_free_block,
	'ctrl' : control_panels_folder,
	'sele' : selection,
	'shdf' : shutdown_items_folder,
	'macs' : system_folder,
	'ver2' : product_version,
	'vers' : version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
