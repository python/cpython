"""Suite Finder Suite: Objects and Events for the Finder
Level 1, version 1

Generated from flap:System Folder:Extensions:Finder Scripting Extension
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_Suite:

	_argmap_clean_up = {
		'by' : 'by  ',
	}

	def clean_up(self, _object, _attributes={}, **_arguments):
		"""clean up: Arrange items in window nicely
		Required argument: the window to clean up
		Keyword argument by: the order in which to clean up the objects
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fclu'

		aetools.keysubst(_arguments, self._argmap_clean_up)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

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

	def eject(self, _object=None, _attributes={}, **_arguments):
		"""eject: Eject the specified disk(s), or every ejectable disk if no parameter is specified
		Required argument: the items to eject
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'ejct'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def empty(self, _object=None, _attributes={}, **_arguments):
		"""empty: Empty the trash
		Required argument: –empty” and –empty trash” both do the same thing
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'empt'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def erase(self, _object, _attributes={}, **_arguments):
		"""erase: Erase the specified disk(s)
		Required argument: the items to erase
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fera'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_put_away = {
		'items' : 'fsel',
	}

	def put_away(self, _object, _attributes={}, **_arguments):
		"""put away: Put away the specified object(s)
		Required argument: the items to put away
		Keyword argument items: DO NOT USE: provided for backwards compatibility with old event suite.  Will be removed in future Finders
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the object put away in its put-away location
		"""
		_code = 'fndr'
		_subcode = 'ptwy'

		aetools.keysubst(_arguments, self._argmap_put_away)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def restart(self, _no_object=None, _attributes={}, **_arguments):
		"""restart: Restart the Macintosh
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

	def reveal(self, _object, _attributes={}, **_arguments):
		"""reveal: Bring the specified object(s) into view
		Required argument: the object to be made visible
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'mvis'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def select(self, _object, _attributes={}, **_arguments):
		"""select: Select the specified object(s)
		Required argument: the object to select
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'slct'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def shut_down(self, _no_object=None, _attributes={}, **_arguments):
		"""shut down: Shut Down the Macintosh
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
		"""sleep: Sleep the Macintosh
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'snoz'

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
		Keyword argument by: the property to sort the items by
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

	def update(self, _object, _attributes={}, **_arguments):
		"""update: Update the display of the specified object(s) to match their on-disk representation
		Required argument: the item to update
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fupd'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.has_key('errn'):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class accessory_process(aetools.ComponentItem):
	"""accessory process - A process launched from a desk accessory file"""
	want = 'pcda'
class desk_accessory_file(aetools.NProperty):
	"""desk accessory file - the desk accessory file from which this process was launched"""
	which = 'dafi'
	want = 'obj '

accessory_processes = accessory_process

class accessory_suitcase(aetools.ComponentItem):
	"""accessory suitcase - A desk accessory suitcase"""
	want = 'dsut'
#        element 'cobj' as ['indx', 'name']

accessory_suitcases = accessory_suitcase

class alias_file(aetools.ComponentItem):
	"""alias file - An alias file (created with –Make Alias”)"""
	want = 'alia'
class original_item(aetools.NProperty):
	"""original item - the original item pointed to by the alias"""
	which = 'orig'
	want = 'obj '

alias_files = alias_file

class application(aetools.ComponentItem):
	"""application - An application program"""
	want = 'capp'
class about_this_macintosh(aetools.NProperty):
	"""about this macintosh - the –About this Macintosh” dialog, and the list of running processes displayed in it"""
	which = 'abbx'
	want = 'obj '
class apple_menu_items_folder(aetools.NProperty):
	"""apple menu items folder - the special folder –Apple Menu Items,” the contents of which appear in the Apple menu"""
	which = 'amnu'
	want = 'obj '
class clipboard(aetools.NProperty):
	"""clipboard - the Finder's clipboard window"""
	which = 'pcli'
	want = 'obj '
class control_panels_folder(aetools.NProperty):
	"""control panels folder - the special folder –Control Panels”"""
	which = 'ctrl'
	want = 'obj '
class desktop(aetools.NProperty):
	"""desktop - the desktop"""
	which = 'desk'
	want = 'obj '
class extensions_folder(aetools.NProperty):
	"""extensions folder - the special folder –Extensions”"""
	which = 'extn'
	want = 'obj '
class file_sharing(aetools.NProperty):
	"""file sharing - Is file sharing on?"""
	which = 'fshr'
	want = 'bool'
class fonts_folder(aetools.NProperty):
	"""fonts folder - the special folder –Fonts”"""
	which = 'ffnt'
	want = 'obj '
class frontmost(aetools.NProperty):
	"""frontmost - Is this the frontmost application?"""
	which = 'pisf'
	want = 'bool'
class insertion_location(aetools.NProperty):
	"""insertion location - the container that a new folder would appear in if –New Folder” was selected"""
	which = 'pins'
	want = 'obj '
class largest_free_block(aetools.NProperty):
	"""largest free block - the largest free block of process memory available to launch an application"""
	which = 'mfre'
	want = 'long'
class preferences_folder(aetools.NProperty):
	"""preferences folder - the special folder –Preferences”"""
	which = 'pref'
	want = 'obj '
class product_version(aetools.NProperty):
	"""product version - the version of the System software running on this Macintosh"""
	which = 'ver2'
	want = 'itxt'
class selection(aetools.NProperty):
	"""selection - the selection visible to the user"""
	which = 'sele'
	want = 'obj '
class sharing_starting_up(aetools.NProperty):
	"""sharing starting up - Is File sharing in the process of starting up (still off, but soon to be on)?"""
	which = 'fsup'
	want = 'bool'
class shortcuts(aetools.NProperty):
	"""shortcuts - the –Finder Shortcuts” item in the Finder's help menu"""
	which = 'scut'
	want = 'obj '
class shutdown_items_folder(aetools.NProperty):
	"""shutdown items folder - the special folder –Shutdown Items”"""
	which = 'shdf'
	want = 'obj '
class startup_items_folder(aetools.NProperty):
	"""startup items folder - the special folder –Startup Items”"""
	which = 'strt'
	want = 'obj '
class system_folder(aetools.NProperty):
	"""system folder - the System folder"""
	which = 'macs'
	want = 'obj '
class temporary_items_folder(aetools.NProperty):
	"""temporary items folder - the special folder –Temporary Items” (invisible)"""
	which = 'temp'
	want = 'obj '
class version(aetools.NProperty):
	"""version - the version of the Finder Scripting Extension"""
	which = 'vers'
	want = 'itxt'
class view_preferences(aetools.NProperty):
	"""view preferences - the view preferences control panel"""
	which = 'pvwp'
	want = 'obj '
class visible(aetools.NProperty):
	"""visible - Is the Finder's layer visible?"""
	which = 'pvis'
	want = 'bool'
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'ctnr' as ['indx', 'name']
#        element 'cwnd' as ['indx', 'name']
#        element 'dwnd' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'cdsk' as ['indx', 'name']
#        element 'cdis' as ['indx', 'name', 'ID  ']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'iwnd' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'swnd' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']
#        element 'ctrs' as ['indx', 'name']
#        element 'cwin' as ['indx', 'name']

class application_file(aetools.ComponentItem):
	"""application file - An application's file on disk"""
	want = 'appf'
class minimum_partition_size(aetools.NProperty):
	"""minimum partition size - the smallest memory size that the application can possibly be launched with"""
	which = 'mprt'
	want = 'long'
class partition_size(aetools.NProperty):
	"""partition size - the memory size that the application will be launched with"""
	which = 'appt'
	want = 'long'
class scriptable(aetools.NProperty):
	"""scriptable - Is this application high-level event aware (accepts open application, open document, print document, and quit)?"""
	which = 'isab'
	want = 'bool'
class suggested_partition_size(aetools.NProperty):
	"""suggested partition size - the memory size that the developer recommends that the application should be launched with"""
	which = 'sprt'
	want = 'long'

application_files = application_file

class application_process(aetools.ComponentItem):
	"""application process - A process launched from an application file"""
	want = 'pcap'
class application_file(aetools.NProperty):
	"""application file - the application file from which this process was launched"""
	which = 'appf'
	want = 'appf'

application_processes = application_process

class container(aetools.ComponentItem):
	"""container - An item that contains other items"""
	want = 'ctnr'
class completely_expanded(aetools.NProperty):
	"""completely expanded - Is the container and all of its children open in outline view?"""
	which = 'pexc'
	want = 'bool'
class container_window(aetools.NProperty):
	"""container window - the main window for the container"""
	which = 'cwnd'
	want = 'obj '
class entire_contents(aetools.NProperty):
	"""entire contents - the entire contents of the container, including the contents of its children"""
	which = 'ects'
	want = 'obj '
class expandable(aetools.NProperty):
	"""expandable - Is the container capable of being expanded into outline view?"""
	which = 'pexa'
	want = 'bool'
class expanded(aetools.NProperty):
	"""expanded - Is the container open in outline view?"""
	which = 'pexp'
	want = 'bool'
class previous_list_view(aetools.NProperty):
	"""previous list view - the last non-icon view (by name, by date, etc.) selected for the container (forgotten as soon as the window is closed)"""
	which = 'svew'
	want = 'long'
# repeated property selection the selection visible to the user
class view(aetools.NProperty):
	"""view - the view selected for the container (by icon, by name, by date, etc.)"""
	which = 'pvew'
	want = 'long'
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

containers = container

class container_window(aetools.ComponentItem):
	"""container window - A window that contains items"""
	want = 'cwnd'
class container(aetools.NProperty):
	"""container - the container this window is opened from"""
	which = 'ctnr'
	want = 'obj '
class disk(aetools.NProperty):
	"""disk - the disk on which the item this window was opened from is stored"""
	which = 'cdis'
	want = 'obj '
class folder(aetools.NProperty):
	"""folder - the folder this window is opened from"""
	which = 'cfol'
	want = 'obj '
class item(aetools.NProperty):
	"""item - the item this window is opened from"""
	which = 'cobj'
	want = 'obj '
# repeated property previous_list_view the last non-icon view (by name, by date, etc.) selected for the window (forgotten as soon as the window is closed)
# repeated property selection the selection visible to the user
# repeated property view the view selected for the window (by icon, by name, by date, etc.)
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

container_windows = container_window

class content_space(aetools.ComponentItem):
	"""content space - All windows, including the desktop window (–Window” does not include the desktop window)"""
	want = 'dwnd'

content_spaces = content_space

class control_panel(aetools.ComponentItem):
	"""control panel - A control panel"""
	want = 'ccdv'
class calculate_folder_sizes(aetools.NProperty):
	"""calculate folder sizes - (Views) Are folder sizes calculated and displayed in Finder list windows?"""
	which = 'sfsz'
	want = 'bool'
class comment_heading(aetools.NProperty):
	"""comment heading - (Views) Are comments displayed in Finder list windows?"""
	which = 'scom'
	want = 'bool'
class date_heading(aetools.NProperty):
	"""date heading - (Views) Are modification dates displayed in Finder list windows?"""
	which = 'sdat'
	want = 'bool'
class disk_information_heading(aetools.NProperty):
	"""disk information heading - (Views) Is information about the volume displayed in Finder list windows?"""
	which = 'sdin'
	want = 'bool'
class icon_size(aetools.NProperty):
	"""icon size - (Views) the size of icons displayed in Finder list windows"""
	which = 'lvis'
	want = 'long'
class kind_heading(aetools.NProperty):
	"""kind heading - (Views) Are document kinds displayed in Finder list windows?"""
	which = 'sknd'
	want = 'bool'
class label_heading(aetools.NProperty):
	"""label heading - (Views) Are labels displayed in Finder list windows?"""
	which = 'slbl'
	want = 'bool'
class size_heading(aetools.NProperty):
	"""size heading - (Views) Are file sizes displayed in Finder list windows"""
	which = 'ssiz'
	want = 'bool'
class snap_to_grid(aetools.NProperty):
	"""snap to grid - (Views) Are items always snapped to the nearest grid point when they are moved?"""
	which = 'fgrd'
	want = 'bool'
class staggered_grid(aetools.NProperty):
	"""staggered grid - (Views) Are grid lines staggered?"""
	which = 'fstg'
	want = 'bool'
class version_heading(aetools.NProperty):
	"""version heading - (Views) Are file versions displayed in Finder list windows?"""
	which = 'svrs'
	want = 'bool'
class view_font(aetools.NProperty):
	"""view font - (Views) the id of the font used in Finder views"""
	which = 'vfnt'
	want = 'long'
class view_font_size(aetools.NProperty):
	"""view font size - (Views) the size of the font used in Finder views"""
	which = 'vfsz'
	want = 'long'

control_panels = control_panel

class desk_accessory_file(aetools.ComponentItem):
	"""desk accessory file - A desk accessory file"""
	want = 'dafi'

desk_accessory_files = desk_accessory_file

class desktop_2d_object(aetools.ComponentItem):
	"""desktop-object - Desktop-object is the class of the –desktop” object"""
	want = 'cdsk'
class startup_disk(aetools.NProperty):
	"""startup disk - the startup disk"""
	which = 'sdsk'
	want = 'obj '
class trash(aetools.NProperty):
	"""trash - the trash"""
	which = 'trsh'
	want = 'obj '
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

class disk(aetools.ComponentItem):
	"""disk - A disk"""
	want = 'cdis'
class capacity(aetools.NProperty):
	"""capacity - the total number of bytes (free or used) on the disk"""
	which = 'capa'
	want = 'long'
class ejectable(aetools.NProperty):
	"""ejectable - Can the media can be ejected (floppies, CD's, syquest)?"""
	which = 'isej'
	want = 'bool'
class free_space(aetools.NProperty):
	"""free space - the number of free bytes left on the disk"""
	which = 'frsp'
	want = 'long'
class local_volume(aetools.NProperty):
	"""local volume - Is the media is a local volume (rather than a file server)?"""
	which = 'isrv'
	want = 'bool'
class startup(aetools.NProperty):
	"""startup - Is this disk the boot disk?"""
	which = 'istd'
	want = 'bool'
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'ID  ', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

disks = disk

class document_file(aetools.ComponentItem):
	"""document file - A document file"""
	want = 'docf'

document_files = document_file

class file(aetools.ComponentItem):
	"""file - A file"""
	want = 'file'
class creator_type(aetools.NProperty):
	"""creator type - the OSType identifying the application that created the item"""
	which = 'fcrt'
	want = 'type'
class file_type(aetools.NProperty):
	"""file type - the OSType identifying the type of data contained in the item"""
	which = 'fitp'
	want = 'type'
class locked(aetools.NProperty):
	"""locked - Is the file locked?"""
	which = 'islk'
	want = 'bool'
# repeated property product_version the version of the product (visible at the top of the –Get Info” dialog)
class stationery(aetools.NProperty):
	"""stationery - Is the item a stationery pad?"""
	which = 'pspd'
	want = 'bool'
# repeated property version the version of the file (visible at the bottom of the –Get Info” dialog)

files = file

class folder(aetools.ComponentItem):
	"""folder - A folder"""
	want = 'cfol'
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

folders = folder

class font_file(aetools.ComponentItem):
	"""font file - A font file"""
	want = 'fntf'

font_files = font_file

class font_suitcase(aetools.ComponentItem):
	"""font suitcase - A font suitcase"""
	want = 'fsut'
#        element 'cobj' as ['indx', 'name']

font_suitcases = font_suitcase

class group(aetools.ComponentItem):
	"""group - A Group in the Users and Groups control panel"""
	want = 'sgrp'
class bounds(aetools.NProperty):
	"""bounds - the bounding rectangle of the group"""
	which = 'pbnd'
	want = 'qdrt'
class icon(aetools.NProperty):
	"""icon - the icon bitmap of the group"""
	which = 'iimg'
	want = 'ifam'
class label_index(aetools.NProperty):
	"""label index - the label of the group"""
	which = 'labi'
	want = 'long'
class name(aetools.NProperty):
	"""name - the name of the group"""
	which = 'pnam'
	want = 'itxt'
class position(aetools.NProperty):
	"""position - the position of the group within its parent window"""
	which = 'posn'
	want = 'QDpt'

groups = group

class information_window(aetools.ComponentItem):
	"""information window - An information window (opened by –Get Infoƒ”)"""
	want = 'iwnd'
class comment(aetools.NProperty):
	"""comment - the comment"""
	which = 'comt'
	want = 'itxt'
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created"""
	which = 'crtd'
	want = 'ldt '
# repeated property icon the icon bitmap of the item
# repeated property item the item this window was opened from
# repeated property locked Is the item locked?
# repeated property minimum_partition_size the smallest memory size that the application can possibly be launched with
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified"""
	which = 'modd'
	want = 'ldt '
# repeated property partition_size the memory size that the application will be launched with
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the item on disk"""
	which = 'phys'
	want = 'long'
# repeated property product_version the version of the product (visible at the top of the –Get Info” dialog)
class size(aetools.NProperty):
	"""size - the logical size of the item"""
	which = 'ptsz'
	want = 'long'
# repeated property stationery Is the item a stationery pad?
# repeated property suggested_partition_size the memory size that the developer recommends that the application should be launched with
# repeated property version the version of the file (visible at the bottom of the –Get Info” dialog)
class warn_before_emptying(aetools.NProperty):
	"""warn before emptying - Is a dialog displayed when –Empty trashƒ” is selected?"""
	which = 'warn'
	want = 'bool'

information_windows = information_window

class item(aetools.ComponentItem):
	"""item - An item"""
	want = 'cobj'
# repeated property bounds the bounding rectangle of the item
# repeated property comment the comment displayed in the –Get Info” window of the item
# repeated property container the container of this item
class content_space(aetools.NProperty):
	"""content space - the window that would open if the item was opened"""
	which = 'dwnd'
	want = 'dwnd'
# repeated property creation_date the date on which the item was created
# repeated property disk the disk on which the item is stored
# repeated property folder the folder in which the item is stored
# repeated property icon the icon bitmap of the item
class id(aetools.NProperty):
	"""id - an id that identifies the item"""
	which = 'ID  '
	want = 'long'
class information_window(aetools.NProperty):
	"""information window - the information window for the item"""
	which = 'iwnd'
	want = 'obj '
class kind(aetools.NProperty):
	"""kind - the kind of the item"""
	which = 'kind'
	want = 'itxt'
# repeated property label_index the label of the item
# repeated property modification_date the date on which the item was last modified
# repeated property name the name of the item
# repeated property physical_size the actual space used by the item on disk
# repeated property position the position of the item within its parent window
class selected(aetools.NProperty):
	"""selected - Is the item selected?"""
	which = 'issl'
	want = 'bool'
# repeated property size the logical size of the item
class window(aetools.NProperty):
	"""window - the window that would open if the item was opened"""
	which = 'cwin'
	want = 'cwin'

items = item

class process(aetools.ComponentItem):
	"""process - A process running on this Macintosh"""
	want = 'prcs'
# repeated property creator_type the creator type of this process
class file(aetools.NProperty):
	"""file - the file that launched this process"""
	which = 'file'
	want = 'obj '
# repeated property file_type the file type of the file that launched this process
# repeated property frontmost Is this the frontmost application?
# repeated property name the name of the process
# repeated property partition_size the size of the partition that this application was launched with
class partition_space_used(aetools.NProperty):
	"""partition space used - the number of bytes currently used in this partition"""
	which = 'pusd'
	want = 'long'
class remote_events(aetools.NProperty):
	"""remote events - Will this process accepts remote events?"""
	which = 'revt'
	want = 'bool'
# repeated property scriptable Is this process high-level event aware (accepts open application, open document, print document, and quit)?
# repeated property visible Is this process' layer visible?

processes = process

class sharable_container(aetools.ComponentItem):
	"""sharable container - A container that may be shared (disks and folders)"""
	want = 'sctr'
class exported(aetools.NProperty):
	"""exported - Is this folder a share point or inside a share point?"""
	which = 'sexp'
	want = 'bool'
class group(aetools.NProperty):
	"""group - the user or group that has special access to the folder"""
	which = 'sgrp'
	want = 'itxt'
class group_privileges(aetools.NProperty):
	"""group privileges - the see folders/see files/make changes privileges for the group"""
	which = 'gppr'
	want = 'priv'
class guest_privileges(aetools.NProperty):
	"""guest privileges - the see folders/see files/make changes privileges for everyone"""
	which = 'gstp'
	want = 'priv'
class inherited_privileges(aetools.NProperty):
	"""inherited privileges - Are the privileges of this item always the same as the container it is stored in?"""
	which = 'iprv'
	want = 'bool'
class mounted(aetools.NProperty):
	"""mounted - Is this folder mounted on another machine's desktop?"""
	which = 'smou'
	want = 'bool'
class owner(aetools.NProperty):
	"""owner - the user that owns this folder"""
	which = 'sown'
	want = 'itxt'
class owner_privileges(aetools.NProperty):
	"""owner privileges - the see folders/see files/make changes privileges for the owner"""
	which = 'ownr'
	want = 'priv'
class protected(aetools.NProperty):
	"""protected - Is container protected from being moved, renamed or deleted?"""
	which = 'spro'
	want = 'bool'
class shared(aetools.NProperty):
	"""shared - Is container a share point?"""
	which = 'shar'
	want = 'bool'
class sharing_window(aetools.NProperty):
	"""sharing window - the sharing window for the container"""
	which = 'swnd'
	want = 'obj '
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

sharable_containers = sharable_container

class sharing_privileges(aetools.ComponentItem):
	"""sharing privileges - A set of sharing properties"""
	want = 'priv'
class make_changes(aetools.NProperty):
	"""make changes - privileges to make changes"""
	which = 'prvw'
	want = 'bool'
class see_files(aetools.NProperty):
	"""see files - privileges to see files"""
	which = 'prvr'
	want = 'bool'
class see_folders(aetools.NProperty):
	"""see folders - privileges to see folders"""
	which = 'prvs'
	want = 'bool'

class sharing_window(aetools.ComponentItem):
	"""sharing window - A sharing window (opened by –Sharingƒ”)"""
	want = 'swnd'
# repeated property container the container that this window was opened from
# repeated property exported Is this container a share point or inside a share point?
# repeated property folder the folder that this window was opened from
# repeated property group the user or group that has special access to the container
# repeated property group_privileges the see folders/see files/make changes privileges for the group
# repeated property guest_privileges the see folders/see files/make changes privileges for everyone
# repeated property inherited_privileges Are the privileges of this item always the same as the container it is stored in?
# repeated property item the item that this window was opened from
# repeated property mounted Is this container mounted on another machine's desktop?
# repeated property owner the user that owns the container
# repeated property owner_privileges the see folders/see files/make changes privileges for the owner
# repeated property protected Is container protected from being moved, renamed or deleted?
class sharable_container(aetools.NProperty):
	"""sharable container - the sharable container that this window was opened from"""
	which = 'sctr'
	want = 'obj '
# repeated property shared Is container a share point?

sharing_windows = sharing_window

class sound_file(aetools.ComponentItem):
	"""sound file - This class represents sound files"""
	want = 'sndf'

sound_files = sound_file

class status_window(aetools.ComponentItem):
	"""status window - These windows are progress dialogs (copy window, rebuild desktop database, empty trash)"""
	want = 'qwnd'

status_windows = status_window

class suitcase(aetools.ComponentItem):
	"""suitcase - A font or desk accessory suitcase"""
	want = 'stcs'
#        element 'cobj' as ['indx', 'name']

suitcases = suitcase

class trash_2d_object(aetools.ComponentItem):
	"""trash-object - Trash-object is the class of the –trash” object"""
	want = 'ctrs'
# repeated property warn_before_emptying Is a dialog displayed when –Empty trashƒ” is selected?
#        element 'dsut' as ['indx', 'name']
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name']
#        element 'ctnr' as ['indx', 'name']
#        element 'ccdv' as ['indx', 'name']
#        element 'dafi' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'cfol' as ['indx', 'name']
#        element 'fntf' as ['indx', 'name']
#        element 'fsut' as ['indx', 'name']
#        element 'cobj' as ['indx', 'name']
#        element 'sctr' as ['indx', 'name']
#        element 'sndf' as ['indx', 'name']
#        element 'stcs' as ['indx', 'name']

class user(aetools.ComponentItem):
	"""user - A User in the Users and Groups control panel"""
	want = 'cuse'
# repeated property bounds the bounding rectangle of the user
# repeated property icon the icon bitmap of the user
# repeated property label_index the label of the user
# repeated property name the name of the user
# repeated property position the position of the user within its parent window

users = user

class window(aetools.ComponentItem):
	"""window - A window"""
	want = 'cwin'

windows = window
accessory_process._propdict = {
	'desk_accessory_file' : desk_accessory_file,
}
accessory_process._elemdict = {
}
accessory_suitcase._propdict = {
}
accessory_suitcase._elemdict = {
	'item' : item,
}
alias_file._propdict = {
	'original_item' : original_item,
}
alias_file._elemdict = {
}
application._propdict = {
	'about_this_macintosh' : about_this_macintosh,
	'apple_menu_items_folder' : apple_menu_items_folder,
	'clipboard' : clipboard,
	'control_panels_folder' : control_panels_folder,
	'desktop' : desktop,
	'extensions_folder' : extensions_folder,
	'file_sharing' : file_sharing,
	'fonts_folder' : fonts_folder,
	'frontmost' : frontmost,
	'insertion_location' : insertion_location,
	'largest_free_block' : largest_free_block,
	'preferences_folder' : preferences_folder,
	'product_version' : product_version,
	'selection' : selection,
	'sharing_starting_up' : sharing_starting_up,
	'shortcuts' : shortcuts,
	'shutdown_items_folder' : shutdown_items_folder,
	'startup_items_folder' : startup_items_folder,
	'system_folder' : system_folder,
	'temporary_items_folder' : temporary_items_folder,
	'version' : version,
	'view_preferences' : view_preferences,
	'visible' : visible,
}
application._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'container_window' : container_window,
	'content_space' : content_space,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'desktop_2d_object' : desktop_2d_object,
	'disk' : disk,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'information_window' : information_window,
	'item' : item,
	'sharable_container' : sharable_container,
	'sharing_window' : sharing_window,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
	'trash_2d_object' : trash_2d_object,
	'window' : window,
}
application_file._propdict = {
	'minimum_partition_size' : minimum_partition_size,
	'partition_size' : partition_size,
	'scriptable' : scriptable,
	'suggested_partition_size' : suggested_partition_size,
}
application_file._elemdict = {
}
application_process._propdict = {
	'application_file' : application_file,
}
application_process._elemdict = {
}
container._propdict = {
	'completely_expanded' : completely_expanded,
	'container_window' : container_window,
	'entire_contents' : entire_contents,
	'expandable' : expandable,
	'expanded' : expanded,
	'previous_list_view' : previous_list_view,
	'selection' : selection,
	'view' : view,
}
container._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
container_window._propdict = {
	'container' : container,
	'disk' : disk,
	'folder' : folder,
	'item' : item,
	'previous_list_view' : previous_list_view,
	'selection' : selection,
	'view' : view,
}
container_window._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
content_space._propdict = {
}
content_space._elemdict = {
}
control_panel._propdict = {
	'calculate_folder_sizes' : calculate_folder_sizes,
	'comment_heading' : comment_heading,
	'date_heading' : date_heading,
	'disk_information_heading' : disk_information_heading,
	'icon_size' : icon_size,
	'kind_heading' : kind_heading,
	'label_heading' : label_heading,
	'size_heading' : size_heading,
	'snap_to_grid' : snap_to_grid,
	'staggered_grid' : staggered_grid,
	'version_heading' : version_heading,
	'view_font' : view_font,
	'view_font_size' : view_font_size,
}
control_panel._elemdict = {
}
desk_accessory_file._propdict = {
}
desk_accessory_file._elemdict = {
}
desktop_2d_object._propdict = {
	'startup_disk' : startup_disk,
	'trash' : trash,
}
desktop_2d_object._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
disk._propdict = {
	'capacity' : capacity,
	'ejectable' : ejectable,
	'free_space' : free_space,
	'local_volume' : local_volume,
	'startup' : startup,
}
disk._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
document_file._propdict = {
}
document_file._elemdict = {
}
file._propdict = {
	'creator_type' : creator_type,
	'file_type' : file_type,
	'locked' : locked,
	'product_version' : product_version,
	'stationery' : stationery,
	'version' : version,
}
file._elemdict = {
}
folder._propdict = {
}
folder._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
font_file._propdict = {
}
font_file._elemdict = {
}
font_suitcase._propdict = {
}
font_suitcase._elemdict = {
	'item' : item,
}
group._propdict = {
	'bounds' : bounds,
	'icon' : icon,
	'label_index' : label_index,
	'name' : name,
	'position' : position,
}
group._elemdict = {
}
information_window._propdict = {
	'comment' : comment,
	'creation_date' : creation_date,
	'icon' : icon,
	'item' : item,
	'locked' : locked,
	'minimum_partition_size' : minimum_partition_size,
	'modification_date' : modification_date,
	'partition_size' : partition_size,
	'physical_size' : physical_size,
	'product_version' : product_version,
	'size' : size,
	'stationery' : stationery,
	'suggested_partition_size' : suggested_partition_size,
	'version' : version,
	'warn_before_emptying' : warn_before_emptying,
}
information_window._elemdict = {
}
item._propdict = {
	'bounds' : bounds,
	'comment' : comment,
	'container' : container,
	'content_space' : content_space,
	'creation_date' : creation_date,
	'disk' : disk,
	'folder' : folder,
	'icon' : icon,
	'id' : id,
	'information_window' : information_window,
	'kind' : kind,
	'label_index' : label_index,
	'modification_date' : modification_date,
	'name' : name,
	'physical_size' : physical_size,
	'position' : position,
	'selected' : selected,
	'size' : size,
	'window' : window,
}
item._elemdict = {
}
process._propdict = {
	'creator_type' : creator_type,
	'file' : file,
	'file_type' : file_type,
	'frontmost' : frontmost,
	'name' : name,
	'partition_size' : partition_size,
	'partition_space_used' : partition_space_used,
	'remote_events' : remote_events,
	'scriptable' : scriptable,
	'visible' : visible,
}
process._elemdict = {
}
sharable_container._propdict = {
	'exported' : exported,
	'group' : group,
	'group_privileges' : group_privileges,
	'guest_privileges' : guest_privileges,
	'inherited_privileges' : inherited_privileges,
	'mounted' : mounted,
	'owner' : owner,
	'owner_privileges' : owner_privileges,
	'protected' : protected,
	'shared' : shared,
	'sharing_window' : sharing_window,
}
sharable_container._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
sharing_privileges._propdict = {
	'make_changes' : make_changes,
	'see_files' : see_files,
	'see_folders' : see_folders,
}
sharing_privileges._elemdict = {
}
sharing_window._propdict = {
	'container' : container,
	'exported' : exported,
	'folder' : folder,
	'group' : group,
	'group_privileges' : group_privileges,
	'guest_privileges' : guest_privileges,
	'inherited_privileges' : inherited_privileges,
	'item' : item,
	'mounted' : mounted,
	'owner' : owner,
	'owner_privileges' : owner_privileges,
	'protected' : protected,
	'sharable_container' : sharable_container,
	'shared' : shared,
}
sharing_window._elemdict = {
}
sound_file._propdict = {
}
sound_file._elemdict = {
}
status_window._propdict = {
}
status_window._elemdict = {
}
suitcase._propdict = {
}
suitcase._elemdict = {
	'item' : item,
}
trash_2d_object._propdict = {
	'warn_before_emptying' : warn_before_emptying,
}
trash_2d_object._elemdict = {
	'accessory_suitcase' : accessory_suitcase,
	'alias_file' : alias_file,
	'application_file' : application_file,
	'container' : container,
	'control_panel' : control_panel,
	'desk_accessory_file' : desk_accessory_file,
	'document_file' : document_file,
	'file' : file,
	'folder' : folder,
	'font_file' : font_file,
	'font_suitcase' : font_suitcase,
	'item' : item,
	'sharable_container' : sharable_container,
	'sound_file' : sound_file,
	'suitcase' : suitcase,
}
user._propdict = {
	'bounds' : bounds,
	'icon' : icon,
	'label_index' : label_index,
	'name' : name,
	'position' : position,
}
user._elemdict = {
}
window._propdict = {
}
window._elemdict = {
}
_Enum_vwby = {
	'conflicts' : 'cflc',	# 
	'existing_items' : 'exsi',	# 
	'small_icon' : 'smic',	# 
	'all' : 'kyal',	# 
}

_Enum_gsen = {
	'CPU' : 'proc',	# 
	'FPU' : 'fpu ',	# 
	'MMU' : 'mmu ',	# 
	'hardware' : 'hdwr',	# 
	'operating_system' : 'os  ',	# 
	'sound_system' : 'snd ',	# 
	'memory_available' : 'lram',	# 
	'memory_installed' : 'ram ',	# 
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'swnd' : sharing_window,
	'iwnd' : information_window,
	'ccdv' : control_panel,
	'cwnd' : container_window,
	'appf' : application_file,
	'prcs' : process,
	'cdis' : disk,
	'cwin' : window,
	'dafi' : desk_accessory_file,
	'sgrp' : group,
	'alia' : alias_file,
	'ctnr' : container,
	'qwnd' : status_window,
	'fsut' : font_suitcase,
	'sndf' : sound_file,
	'priv' : sharing_privileges,
	'dwnd' : content_space,
	'pcap' : application_process,
	'stcs' : suitcase,
	'ctrs' : trash_2d_object,
	'file' : file,
	'cobj' : item,
	'cuse' : user,
	'cdsk' : desktop_2d_object,
	'pcda' : accessory_process,
	'capp' : application,
	'cfol' : folder,
	'sctr' : sharable_container,
	'dsut' : accessory_suitcase,
	'docf' : document_file,
	'fntf' : font_file,
}

_propdeclarations = {
	'swnd' : sharing_window,
	'fshr' : file_sharing,
	'pvew' : view,
	'pusd' : partition_space_used,
	'fcrt' : creator_type,
	'sdat' : date_heading,
	'sdin' : disk_information_heading,
	'strt' : startup_items_folder,
	'issl' : selected,
	'pvis' : visible,
	'slbl' : label_heading,
	'cdis' : disk,
	'fitp' : file_type,
	'smou' : mounted,
	'pexc' : completely_expanded,
	'pexa' : expandable,
	'comt' : comment,
	'svew' : previous_list_view,
	'labi' : label_index,
	'sctr' : sharable_container,
	'sknd' : kind_heading,
	'trsh' : trash,
	'fstg' : staggered_grid,
	'macs' : system_folder,
	'vfsz' : view_font_size,
	'pexp' : expanded,
	'posn' : position,
	'cobj' : item,
	'amnu' : apple_menu_items_folder,
	'pvwp' : view_preferences,
	'desk' : desktop,
	'pnam' : name,
	'mprt' : minimum_partition_size,
	'cwin' : window,
	'pcli' : clipboard,
	'spro' : protected,
	'islk' : locked,
	'sprt' : suggested_partition_size,
	'pisf' : frontmost,
	'sele' : selection,
	'ffnt' : fonts_folder,
	'istd' : startup,
	'sdsk' : startup_disk,
	'shar' : shared,
	'dwnd' : content_space,
	'file' : file,
	'sfsz' : calculate_folder_sizes,
	'ID  ' : id,
	'prvw' : make_changes,
	'iprv' : inherited_privileges,
	'prvr' : see_files,
	'prvs' : see_folders,
	'phys' : physical_size,
	'ctrl' : control_panels_folder,
	'cwnd' : container_window,
	'extn' : extensions_folder,
	'ownr' : owner_privileges,
	'modd' : modification_date,
	'dafi' : desk_accessory_file,
	'sgrp' : group,
	'temp' : temporary_items_folder,
	'fgrd' : snap_to_grid,
	'ptsz' : size,
	'kind' : kind,
	'scut' : shortcuts,
	'abbx' : about_this_macintosh,
	'ctnr' : container,
	'isej' : ejectable,
	'svrs' : version_heading,
	'vfnt' : view_font,
	'warn' : warn_before_emptying,
	'isab' : scriptable,
	'isrv' : local_volume,
	'lvis' : icon_size,
	'shdf' : shutdown_items_folder,
	'gstp' : guest_privileges,
	'vers' : version,
	'appf' : application_file,
	'iwnd' : information_window,
	'revt' : remote_events,
	'frsp' : free_space,
	'capa' : capacity,
	'pspd' : stationery,
	'scom' : comment_heading,
	'pins' : insertion_location,
	'orig' : original_item,
	'pref' : preferences_folder,
	'fsup' : sharing_starting_up,
	'sown' : owner,
	'cfol' : folder,
	'mfre' : largest_free_block,
	'ssiz' : size_heading,
	'iimg' : icon,
	'appt' : partition_size,
	'gppr' : group_privileges,
	'pbnd' : bounds,
	'ects' : entire_contents,
	'sexp' : exported,
	'ver2' : product_version,
	'crtd' : creation_date,
}

_compdeclarations = {
}

_enumdeclarations = {
	'gsen' : _Enum_gsen,
	'vwby' : _Enum_vwby,
}
