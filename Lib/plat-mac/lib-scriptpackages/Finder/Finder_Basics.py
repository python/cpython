"""Suite Finder Basics: Commonly-used Finder commands and object classes
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_Basics_Events:

	def copy(self, _no_object=None, _attributes={}, **_arguments):
		"""copy: (NOT AVAILABLE YET) Copy the selected items to the clipboard (the Finder must be the front application)
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

	_argmap_sort = {
		'by' : 'by  ',
	}

	def sort(self, _object, _attributes={}, **_arguments):
		"""sort: (NOT AVAILABLE YET) Return the specified object(s) in a sorted list
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
	"""Finder preferences - (NOT AVAILABLE YET) Various preferences that apply to the Finder as a whole """
	which = 'pfrp'
	want = 'cprf'
class clipboard(aetools.NProperty):
	"""clipboard - (NOT AVAILABLE YET) the Finder\xd5s clipboard window """
	which = 'pcli'
	want = 'obj '
class desktop(aetools.NProperty):
	"""desktop - the desktop """
	which = 'desk'
	want = 'cdsk'
class frontmost(aetools.NProperty):
	"""frontmost - Is the Finder the frontmost process? """
	which = 'pisf'
	want = 'bool'
class home(aetools.NProperty):
	"""home - the home directory """
	which = 'home'
	want = 'cfol'
class insertion_location(aetools.NProperty):
	"""insertion location - the container in which a new folder would appear if \xd2New Folder\xd3 was selected """
	which = 'pins'
	want = 'obj '
class name(aetools.NProperty):
	"""name - the Finder\xd5s name """
	which = 'pnam'
	want = 'itxt'
class product_version(aetools.NProperty):
	"""product version - the version of the System software running on this computer """
	which = 'ver2'
	want = 'utxt'
class selection(aetools.NProperty):
	"""selection - the selection in the frontmost Finder window """
	which = 'sele'
	want = 'obj '
class startup_disk(aetools.NProperty):
	"""startup disk - the startup disk """
	which = 'sdsk'
	want = 'cdis'
class trash(aetools.NProperty):
	"""trash - the trash """
	which = 'trsh'
	want = 'ctrs'
class version(aetools.NProperty):
	"""version - the version of the Finder """
	which = 'vers'
	want = 'utxt'
class visible(aetools.NProperty):
	"""visible - Is the Finder\xd5s layer visible? """
	which = 'pvis'
	want = 'bool'
#        element 'alia' as ['indx', 'name']
#        element 'appf' as ['indx', 'name', 'ID  ']
#        element 'brow' as ['indx', 'ID  ']
#        element 'cdis' as ['indx', 'name', 'ID  ']
#        element 'cfol' as ['indx', 'name', 'ID  ']
#        element 'clpf' as ['indx', 'name']
#        element 'cobj' as ['indx', 'rele', 'name', 'rang', 'test']
#        element 'ctnr' as ['indx', 'name']
#        element 'cwin' as ['indx', 'name']
#        element 'docf' as ['indx', 'name']
#        element 'file' as ['indx', 'name']
#        element 'inlf' as ['indx', 'name']
#        element 'lwnd' as ['indx', 'name']
#        element 'pack' as ['indx', 'name']
application._superclassnames = []
import Files
import Window_classes
import Containers_and_folders
import Finder_items
application._privpropdict = {
	'Finder_preferences' : Finder_preferences,
	'clipboard' : clipboard,
	'desktop' : desktop,
	'frontmost' : frontmost,
	'home' : home,
	'insertion_location' : insertion_location,
	'name' : name,
	'product_version' : product_version,
	'selection' : selection,
	'startup_disk' : startup_disk,
	'trash' : trash,
	'version' : version,
	'visible' : visible,
}
application._privelemdict = {
	'Finder_window' : Window_classes.Finder_window,
	'alias_file' : Files.alias_file,
	'application_file' : Files.application_file,
	'clipping' : Files.clipping,
	'clipping_window' : Window_classes.clipping_window,
	'container' : Containers_and_folders.container,
	'disk' : Containers_and_folders.disk,
	'document_file' : Files.document_file,
	'file' : Files.file,
	'folder' : Containers_and_folders.folder,
	'internet_location_file' : Files.internet_location_file,
	'item' : Finder_items.item,
	'package' : Files.package,
	'window' : Window_classes.window,
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'capp' : application,
}

_propdeclarations = {
	'desk' : desktop,
	'home' : home,
	'pcli' : clipboard,
	'pfrp' : Finder_preferences,
	'pins' : insertion_location,
	'pisf' : frontmost,
	'pnam' : name,
	'pvis' : visible,
	'sdsk' : startup_disk,
	'sele' : selection,
	'trsh' : trash,
	'ver2' : product_version,
	'vers' : version,
}

_compdeclarations = {
}

_enumdeclarations = {
}
