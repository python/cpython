"""Suite Finder Suite: Objects and Events for the Finder
Level 1, version 1

Generated from Moes:System folder:Extensions:Finder Scripting Extension
AETE/AEUT resource version 0/144, language 0, script 0
"""

import addpack
addpack.addpack('Tools')
addpack.addpack('bgen')
addpack.addpack('ae')

import aetools
import MacOS

_code = 'fndr'

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

class Finder_Suite:

	_argmap_clean_up = {
		'by' : 'by  ',
	}

	def clean_up(self, object, *arguments):
		"""clean up: Arrange items in window nicely
		Required argument: the window to clean up
		Keyword argument by: the order in which to clean up the objects
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fclu'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_clean_up)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_computer = {
		'has' : 'has ',
	}

	def computer(self, object, *arguments):
		"""computer: Test attributes of this computer
		Required argument: the attribute to test
		Keyword argument has: test specific bits of response
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the result of the query
		"""
		_code = 'fndr'
		_subcode = 'gstl'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_computer)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def eject(self, object, *arguments):
		"""eject: Eject the specified disk(s), or every ejectable disk if no parameter is specified
		Required argument: the items to eject
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'ejct'

		if len(arguments):
			object = arguments[0]
			arguments = arguments[1:]
		else:
			object = None
		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def empty(self, object, *arguments):
		"""empty: Empty the trash
		Required argument: ÒemptyÓ and Òempty trashÓ both do the same thing
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'empt'

		if len(arguments):
			object = arguments[0]
			arguments = arguments[1:]
		else:
			object = None
		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def erase(self, object, *arguments):
		"""erase: Erase the specified disk(s)
		Required argument: the items to erase
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fera'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_put_away = {
		'items' : 'fsel',
	}

	def put_away(self, object, *arguments):
		"""put away: Put away the specified object(s)
		Required argument: the items to put away
		Keyword argument items: DO NOT USE: provided for backwards compatibility with old event suite.  Will be removed in future Finders
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the object put away in its put-away location
		"""
		_code = 'fndr'
		_subcode = 'ptwy'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_put_away)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def restart(self, *arguments):
		"""restart: Restart the Macintosh
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'rest'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def reveal(self, object, *arguments):
		"""reveal: Bring the specified object(s) into view
		Required argument: the object to be made visible
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'mvis'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def select(self, object, *arguments):
		"""select: Select the specified object(s)
		Required argument: the object to select
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'misc'
		_subcode = 'slct'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def shut_down(self, *arguments):
		"""shut down: Shut Down the Macintosh
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'shut'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def sleep(self, *arguments):
		"""sleep: Sleep the Macintosh
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'snoz'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	_argmap_sort = {
		'by' : 'by  ',
	}

	def sort(self, object, *arguments):
		"""sort: Return the specified object(s) in a sorted list
		Required argument: a list of finder objects to sort
		Keyword argument by: the property to sort the items by
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the sorted items in their new order
		"""
		_code = 'DATA'
		_subcode = 'SORT'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}

		aetools.keysubst(arguments, self._argmap_sort)

		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']

	def update(self, object, *arguments):
		"""update: Update the display of the specified object(s) to match their on-disk representation
		Required argument: the item to update
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fupd'

		if len(arguments) > 1:
			raise TypeError, 'Too many arguments'
		if arguments:
			arguments = arguments[0]
			if type(arguments) <> type({}):
				raise TypeError, 'Must be a mapping'
		else:
			arguments = {}
		arguments['----'] = object

		if arguments.has_key('_attributes'):
			attributes = arguments['_attributes']
			del arguments['_attributes']
		else:
			attributes = {}


		reply, arguments, attributes = self.send(_code, _subcode,
				arguments, attributes)
		if arguments.has_key('errn'):
			raise MacOS.Error, aetools.decodeerror(arguments)
		# XXXX Optionally decode result
		if arguments.has_key('----'):
			return arguments['----']


#    Class 'accessory process' ('pcda') -- 'A process launched from a desk accessory file'
#        property 'desk accessory file' ('dafi') 'obj ' -- 'the desk accessory file from which this process was launched' []

#    Class 'accessory processes' ('pcda') -- 'every accessory process'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'accessory suitcase' ('dsut') -- 'A desk accessory suitcase'
#        element 'cobj' as ['indx', 'name']

#    Class 'accessory suitcases' ('dsut') -- 'every accessory suitcase'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'alias file' ('alia') -- 'An alias file (created with \322Make Alias\323)'
#        property 'original item' ('orig') 'obj ' -- 'the original item pointed to by the alias' []

#    Class 'alias files' ('alia') -- 'every alias file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'application' ('capp') -- 'An application program'
#        property 'about this macintosh' ('abbx') 'obj ' -- 'the \322About this Macintosh\323 dialog, and the list of running processes displayed in it' []
#        property 'apple menu items folder' ('amnu') 'obj ' -- 'the special folder \322Apple Menu Items,\323 the contents of which appear in the Apple menu' []
#        property 'clipboard' ('pcli') 'obj ' -- "the Finder's clipboard window" []
#        property 'control panels folder' ('ctrl') 'obj ' -- 'the special folder \322Control Panels\323' []
#        property 'desktop' ('desk') 'obj ' -- 'the desktop' []
#        property 'extensions folder' ('extn') 'obj ' -- 'the special folder \322Extensions\323' []
#        property 'file sharing' ('fshr') 'bool' -- 'Is file sharing on?' [mutable]
#        property 'fonts folder' ('ffnt') 'obj ' -- 'the special folder \322Fonts\323' []
#        property 'frontmost' ('pisf') 'bool' -- 'Is this the frontmost application?' [mutable]
#        property 'insertion location' ('pins') 'obj ' -- 'the container that a new folder would appear in if \322New Folder\323 was selected' []
#        property 'largest free block' ('mfre') 'long' -- 'the largest free block of process memory available to launch an application' []
#        property 'preferences folder' ('pref') 'obj ' -- 'the special folder \322Preferences\323' []
#        property 'product version' ('ver2') 'itxt' -- 'the version of the System software running on this Macintosh' []
#        property 'selection' ('sele') 'obj ' -- 'the selection visible to the user' [mutable]
#        property 'sharing starting up' ('fsup') 'bool' -- 'Is File sharing in the process of starting up (still off, but soon to be on)?' []
#        property 'shortcuts' ('scut') 'obj ' -- "the \322Finder Shortcuts\323 item in the Finder's help menu" []
#        property 'shutdown items folder' ('shdf') 'obj ' -- 'the special folder \322Shutdown Items\323' []
#        property 'startup items folder' ('strt') 'obj ' -- 'the special folder \322Startup Items\323' []
#        property 'system folder' ('macs') 'obj ' -- 'the System folder' []
#        property 'temporary items folder' ('temp') 'obj ' -- 'the special folder \322Temporary Items\323 (invisible)' []
#        property 'version' ('vers') 'itxt' -- 'the version of the Finder Scripting Extension' []
#        property 'view preferences' ('pvwp') 'obj ' -- 'the view preferences control panel' []
#        property 'visible' ('pvis') 'bool' -- "Is the Finder's layer visible?" [mutable]
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

#    Class 'application file' ('appf') -- "An application's file on disk"
#        property 'minimum partition size' ('mprt') 'long' -- 'the smallest memory size that the application can possibly be launched with' [mutable]
#        property 'partition size' ('appt') 'long' -- 'the memory size that the application will be launched with' [mutable]
#        property 'scriptable' ('isab') 'bool' -- 'Is this application high-level event aware (accepts open application, open document, print document, and quit)?' []
#        property 'suggested partition size' ('sprt') 'long' -- 'the memory size that the developer recommends that the application should be launched with' []

#    Class 'application files' ('appf') -- 'every application file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'application process' ('pcap') -- 'A process launched from an application file'
#        property 'application file' ('appf') 'appf' -- 'the application file from which this process was launched' []

#    Class 'application processes' ('pcap') -- 'every application process'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'container' ('ctnr') -- 'An item that contains other items'
#        property 'completely expanded' ('pexc') 'bool' -- 'Is the container and all of its children open in outline view?' [mutable]
#        property 'container window' ('cwnd') 'obj ' -- 'the main window for the container' []
#        property 'entire contents' ('ects') 'obj ' -- 'the entire contents of the container, including the contents of its children' []
#        property 'expandable' ('pexa') 'bool' -- 'Is the container capable of being expanded into outline view?' []
#        property 'expanded' ('pexp') 'bool' -- 'Is the container open in outline view?' [mutable]
#        property 'previous list view' ('svew') 'long' -- 'the last non-icon view (by name, by date, etc.) selected for the container (forgotten as soon as the window is closed)' []
#        property 'selection' ('sele') 'obj ' -- 'the selection visible to the user' [mutable]
#        property 'view' ('pvew') 'long' -- 'the view selected for the container (by icon, by name, by date, etc.)' [mutable]
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

#    Class 'containers' ('ctnr') -- 'every container'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'container window' ('cwnd') -- 'A window that contains items'
#        property 'container' ('ctnr') 'obj ' -- 'the container this window is opened from' []
#        property 'disk' ('cdis') 'obj ' -- 'the disk on which the item this window was opened from is stored' []
#        property 'folder' ('cfol') 'obj ' -- 'the folder this window is opened from' []
#        property 'item' ('cobj') 'obj ' -- 'the item this window is opened from' []
#        property 'previous list view' ('svew') 'long' -- 'the last non-icon view (by name, by date, etc.) selected for the window (forgotten as soon as the window is closed)' []
#        property 'selection' ('sele') 'obj ' -- 'the selection visible to the user' [mutable]
#        property 'view' ('pvew') 'long' -- 'the view selected for the window (by icon, by name, by date, etc.)' [mutable]
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

#    Class 'container windows' ('cwnd') -- 'every container window'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'content space' ('dwnd') -- 'All windows, including the desktop window (\322Window\323 does not include the desktop window)'

#    Class 'content spaces' ('dwnd') -- 'Every content space'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'control panel' ('ccdv') -- 'A control panel'
#        property 'calculate folder sizes' ('sfsz') 'bool' -- '(Views) Are folder sizes calculated and displayed in Finder list windows?' [mutable]
#        property 'comment heading' ('scom') 'bool' -- '(Views) Are comments displayed in Finder list windows?' [mutable]
#        property 'date heading' ('sdat') 'bool' -- '(Views) Are modification dates displayed in Finder list windows?' [mutable]
#        property 'disk information heading' ('sdin') 'bool' -- '(Views) Is information about the volume displayed in Finder list windows?' [mutable]
#        property 'icon size' ('lvis') 'long' -- '(Views) the size of icons displayed in Finder list windows' [mutable]
#        property 'kind heading' ('sknd') 'bool' -- '(Views) Are document kinds displayed in Finder list windows?' [mutable]
#        property 'label heading' ('slbl') 'bool' -- '(Views) Are labels displayed in Finder list windows?' [mutable]
#        property 'size heading' ('ssiz') 'bool' -- '(Views) Are file sizes displayed in Finder list windows' [mutable]
#        property 'snap to grid' ('fgrd') 'bool' -- '(Views) Are items always snapped to the nearest grid point when they are moved?' [mutable]
#        property 'staggered grid' ('fstg') 'bool' -- '(Views) Are grid lines staggered?' [mutable]
#        property 'version heading' ('svrs') 'bool' -- '(Views) Are file versions displayed in Finder list windows?' [mutable]
#        property 'view font' ('vfnt') 'long' -- '(Views) the id of the font used in Finder views' [mutable]
#        property 'view font size' ('vfsz') 'long' -- '(Views) the size of the font used in Finder views' [mutable]

#    Class 'control panels' ('ccdv') -- 'every control panel'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'desk accessory file' ('dafi') -- 'A desk accessory file'

#    Class 'desk accessory files' ('dafi') -- 'every desk accessory file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'desktop-object' ('cdsk') -- 'Desktop-object is the class of the \322desktop\323 object'
#        property 'startup disk' ('sdsk') 'obj ' -- 'the startup disk' []
#        property 'trash' ('trsh') 'obj ' -- 'the trash' []
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

#    Class 'disk' ('cdis') -- 'A disk'
#        property 'capacity' ('capa') 'long' -- 'the total number of bytes (free or used) on the disk' []
#        property 'ejectable' ('isej') 'bool' -- "Can the media can be ejected (floppies, CD's, syquest)?" []
#        property 'free space' ('frsp') 'long' -- 'the number of free bytes left on the disk' []
#        property 'local volume' ('isrv') 'bool' -- 'Is the media is a local volume (rather than a file server)?' []
#        property 'startup' ('istd') 'bool' -- 'Is this disk the boot disk?' []
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

#    Class 'disks' ('cdis') -- 'every disk'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'document file' ('docf') -- 'A document file'

#    Class 'document files' ('docf') -- 'every document file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'file' ('file') -- 'A file'
#        property 'creator type' ('fcrt') 'type' -- 'the OSType identifying the application that created the item' [mutable]
#        property 'file type' ('fitp') 'type' -- 'the OSType identifying the type of data contained in the item' [mutable]
#        property 'locked' ('islk') 'bool' -- 'Is the file locked?' [mutable]
#        property 'product version' ('ver2') 'itxt' -- 'the version of the product (visible at the top of the \322Get Info\323 dialog)' []
#        property 'stationery' ('pspd') 'bool' -- 'Is the item a stationery pad?' [mutable]
#        property 'version' ('vers') 'itxt' -- 'the version of the file (visible at the bottom of the \322Get Info\323 dialog)' []

#    Class 'files' ('file') -- 'every file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'folder' ('cfol') -- 'A folder'
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

#    Class 'folders' ('cfol') -- 'every folder'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'font file' ('fntf') -- 'A font file'

#    Class 'font files' ('fntf') -- 'every font file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'font suitcase' ('fsut') -- 'A font suitcase'
#        element 'cobj' as ['indx', 'name']

#    Class 'font suitcases' ('fsut') -- 'every font suitcase'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'group' ('sgrp') -- 'A Group in the Users and Groups control panel'
#        property 'bounds' ('pbnd') 'qdrt' -- 'the bounding rectangle of the group' [mutable]
#        property 'icon' ('iimg') 'ifam' -- 'the icon bitmap of the group' [mutable]
#        property 'label index' ('labi') 'long' -- 'the label of the group' [mutable]
#        property 'name' ('pnam') 'itxt' -- 'the name of the group' [mutable]
#        property 'position' ('posn') 'QDpt' -- 'the position of the group within its parent window' [mutable]

#    Class 'groups' ('sgrp') -- 'every group'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'information window' ('iwnd') -- 'An information window (opened by \322Get Info\311\323)'
#        property 'comment' ('comt') 'itxt' -- 'the comment' [mutable]
#        property 'creation date' ('crtd') 'ldt ' -- 'the date on which the item was created' []
#        property 'icon' ('iimg') 'ifam' -- 'the icon bitmap of the item' [mutable]
#        property 'item' ('cobj') 'obj ' -- 'the item this window was opened from' []
#        property 'locked' ('islk') 'bool' -- 'Is the item locked?' [mutable]
#        property 'minimum partition size' ('mprt') 'long' -- 'the smallest memory size that the application can possibly be launched with' [mutable]
#        property 'modification date' ('modd') 'ldt ' -- 'the date on which the item was last modified' []
#        property 'partition size' ('appt') 'long' -- 'the memory size that the application will be launched with' [mutable]
#        property 'physical size' ('phys') 'long' -- 'the actual space used by the item on disk' []
#        property 'product version' ('ver2') 'itxt' -- 'the version of the product (visible at the top of the \322Get Info\323 dialog)' []
#        property 'size' ('ptsz') 'long' -- 'the logical size of the item' []
#        property 'stationery' ('pspd') 'bool' -- 'Is the item a stationery pad?' [mutable]
#        property 'suggested partition size' ('sprt') 'long' -- 'the memory size that the developer recommends that the application should be launched with' []
#        property 'version' ('vers') 'itxt' -- 'the version of the file (visible at the bottom of the \322Get Info\323 dialog)' []
#        property 'warn before emptying' ('warn') 'bool' -- 'Is a dialog displayed when \322Empty trash\311\323 is selected?' [mutable]

#    Class 'information windows' ('iwnd') -- 'every information window'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'item' ('cobj') -- 'An item'
#        property 'bounds' ('pbnd') 'qdrt' -- 'the bounding rectangle of the item' [mutable]
#        property 'comment' ('comt') 'itxt' -- 'the comment displayed in the \322Get Info\323 window of the item' [mutable]
#        property 'container' ('ctnr') 'obj ' -- 'the container of this item' []
#        property 'content space' ('dwnd') 'dwnd' -- 'the window that would open if the item was opened' []
#        property 'creation date' ('crtd') 'ldt ' -- 'the date on which the item was created' []
#        property 'disk' ('cdis') 'obj ' -- 'the disk on which the item is stored' []
#        property 'folder' ('cfol') 'obj ' -- 'the folder in which the item is stored' []
#        property 'icon' ('iimg') 'ifam' -- 'the icon bitmap of the item' [mutable]
#        property 'id' ('ID  ') 'long' -- 'an id that identifies the item' []
#        property 'information window' ('iwnd') 'obj ' -- 'the information window for the item' []
#        property 'kind' ('kind') 'itxt' -- 'the kind of the item' []
#        property 'label index' ('labi') 'long' -- 'the label of the item' [mutable]
#        property 'modification date' ('modd') 'ldt ' -- 'the date on which the item was last modified' []
#        property 'name' ('pnam') 'itxt' -- 'the name of the item' [mutable]
#        property 'physical size' ('phys') 'long' -- 'the actual space used by the item on disk' []
#        property 'position' ('posn') 'QDpt' -- 'the position of the item within its parent window' [mutable]
#        property 'selected' ('issl') 'bool' -- 'Is the item selected?' [mutable]
#        property 'size' ('ptsz') 'long' -- 'the logical size of the item' []
#        property 'window' ('cwin') 'cwin' -- 'the window that would open if the item was opened' []

#    Class 'items' ('cobj') -- 'every item'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'process' ('prcs') -- 'A process running on this Macintosh'
#        property 'creator type' ('fcrt') 'type' -- 'the creator type of this process' []
#        property 'file' ('file') 'obj ' -- 'the file that launched this process' []
#        property 'file type' ('fitp') 'type' -- 'the file type of the file that launched this process' []
#        property 'frontmost' ('pisf') 'bool' -- 'Is this the frontmost application?' [mutable]
#        property 'name' ('pnam') 'itxt' -- 'the name of the process' []
#        property 'partition size' ('appt') 'long' -- 'the size of the partition that this application was launched with' []
#        property 'partition space used' ('pusd') 'long' -- 'the number of bytes currently used in this partition' []
#        property 'remote events' ('revt') 'bool' -- 'Will this process accepts remote events?' []
#        property 'scriptable' ('isab') 'bool' -- 'Is this process high-level event aware (accepts open application, open document, print document, and quit)?' []
#        property 'visible' ('pvis') 'bool' -- "Is this process' layer visible?" [mutable]

#    Class 'processes' ('prcs') -- 'every process'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'sharable container' ('sctr') -- 'A container that may be shared (disks and folders)'
#        property 'exported' ('sexp') 'bool' -- 'Is this folder a share point or inside a share point?' []
#        property 'group' ('sgrp') 'itxt' -- 'the user or group that has special access to the folder' [mutable]
#        property 'group privileges' ('gppr') 'priv' -- 'the see folders/see files/make changes privileges for the group' [mutable]
#        property 'guest privileges' ('gstp') 'priv' -- 'the see folders/see files/make changes privileges for everyone' [mutable]
#        property 'inherited privileges' ('iprv') 'bool' -- 'Are the privileges of this item always the same as the container it is stored in?' [mutable]
#        property 'mounted' ('smou') 'bool' -- "Is this folder mounted on another machine's desktop?" []
#        property 'owner' ('sown') 'itxt' -- 'the user that owns this folder' [mutable]
#        property 'owner privileges' ('ownr') 'priv' -- 'the see folders/see files/make changes privileges for the owner' [mutable]
#        property 'protected' ('spro') 'bool' -- 'Is container protected from being moved, renamed or deleted?' [mutable]
#        property 'shared' ('shar') 'bool' -- 'Is container a share point?' [mutable]
#        property 'sharing window' ('swnd') 'obj ' -- 'the sharing window for the container' []
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

#    Class 'sharable containers' ('sctr') -- 'every sharable container'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'sharing privileges' ('priv') -- 'A set of sharing properties'
#        property 'make changes' ('prvw') 'bool' -- 'privileges to make changes' [mutable]
#        property 'see files' ('prvr') 'bool' -- 'privileges to see files' [mutable]
#        property 'see folders' ('prvs') 'bool' -- 'privileges to see folders' [mutable]

#    Class 'sharing window' ('swnd') -- 'A sharing window (opened by \322Sharing\311\323)'
#        property 'container' ('ctnr') 'obj ' -- 'the container that this window was opened from' []
#        property 'exported' ('sexp') 'bool' -- 'Is this container a share point or inside a share point?' []
#        property 'folder' ('cfol') 'obj ' -- 'the folder that this window was opened from' []
#        property 'group' ('sgrp') 'itxt' -- 'the user or group that has special access to the container' [mutable]
#        property 'group privileges' ('gppr') 'priv' -- 'the see folders/see files/make changes privileges for the group' [mutable]
#        property 'guest privileges' ('gstp') 'priv' -- 'the see folders/see files/make changes privileges for everyone' [mutable]
#        property 'inherited privileges' ('iprv') 'bool' -- 'Are the privileges of this item always the same as the container it is stored in?' [mutable]
#        property 'item' ('cobj') 'obj ' -- 'the item that this window was opened from' []
#        property 'mounted' ('smou') 'bool' -- "Is this container mounted on another machine's desktop?" []
#        property 'owner' ('sown') 'itxt' -- 'the user that owns the container' [mutable]
#        property 'owner privileges' ('ownr') 'priv' -- 'the see folders/see files/make changes privileges for the owner' [mutable]
#        property 'protected' ('spro') 'bool' -- 'Is container protected from being moved, renamed or deleted?' [mutable]
#        property 'sharable container' ('sctr') 'obj ' -- 'the sharable container that this window was opened from' []
#        property 'shared' ('shar') 'bool' -- 'Is container a share point?' [mutable]

#    Class 'sharing windows' ('swnd') -- 'every sharing window'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'sound file' ('sndf') -- 'This class represents sound files'

#    Class 'sound files' ('sndf') -- 'every sound file'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'status window' ('qwnd') -- 'These windows are progress dialogs (copy window, rebuild desktop database, empty trash)'

#    Class 'status windows' ('qwnd') -- 'every status window'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'suitcase' ('stcs') -- 'A font or desk accessory suitcase'
#        element 'cobj' as ['indx', 'name']

#    Class 'suitcases' ('stcs') -- 'every suitcase'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'trash-object' ('ctrs') -- 'Trash-object is the class of the \322trash\323 object'
#        property 'warn before emptying' ('warn') 'bool' -- 'Is a dialog displayed when \322Empty trash\311\323 is selected?' [mutable]
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

#    Class 'user' ('cuse') -- 'A User in the Users and Groups control panel'
#        property 'bounds' ('pbnd') 'qdrt' -- 'the bounding rectangle of the user' [mutable]
#        property 'icon' ('iimg') 'ifam' -- 'the icon bitmap of the user' [mutable]
#        property 'label index' ('labi') 'long' -- 'the label of the user' [mutable]
#        property 'name' ('pnam') 'itxt' -- 'the name of the user' [mutable]
#        property 'position' ('posn') 'QDpt' -- 'the position of the user within its parent window' [mutable]

#    Class 'users' ('cuse') -- 'every user'
#        property '' ('c@#!') 'type' -- '' [0]

#    Class 'window' ('cwin') -- 'A window'

#    Class 'windows' ('cwin') -- 'every window'
#        property '' ('c@#!') 'type' -- '' [0]
