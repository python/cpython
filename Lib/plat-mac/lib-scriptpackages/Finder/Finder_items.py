"""Suite Finder items: Commands used with file system items, and basic item definition
Level 1, version 1

Generated from /System/Library/CoreServices/Finder.app
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_items_Events:

	def add_to_favorites(self, _object, _attributes={}, **_arguments):
		"""add to favorites: (NOT AVAILABLE YET) Add the items to the user\xd5s Favorites
		Required argument: the items to add to the collection of Favorites
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'ffav'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_clean_up = {
		'by' : 'by  ',
	}

	def clean_up(self, _object, _attributes={}, **_arguments):
		"""clean up: (NOT AVAILABLE YET) Arrange items in window nicely (only applies to open windows in icon view that are not kept arranged)
		Required argument: the window to clean up
		Keyword argument by: the order in which to clean up the objects (name, index, date, etc.)
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fclu'

		aetools.keysubst(_arguments, self._argmap_clean_up)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def eject(self, _object=None, _attributes={}, **_arguments):
		"""eject: Eject the specified disk(s)
		Required argument: the disk(s) to eject
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'ejct'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def empty(self, _object=None, _attributes={}, **_arguments):
		"""empty: Empty the trash
		Required argument: \xd2empty\xd3 and \xd2empty trash\xd3 both do the same thing
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'empt'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	def erase(self, _object, _attributes={}, **_arguments):
		"""erase: (NOT AVAILABLE) Erase the specified disk(s)
		Required argument: the items to erase
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fera'

		if _arguments: raise TypeError, 'No optional args expected'
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_update = {
		'necessity' : 'nec?',
		'registering_applications' : 'reg?',
	}

	def update(self, _object, _attributes={}, **_arguments):
		"""update: Update the display of the specified object(s) to match their on-disk representation
		Required argument: the item to update
		Keyword argument necessity: only update if necessary (i.e. a finder window is open).  default is false
		Keyword argument registering_applications: register applications. default is true
		Keyword argument _attributes: AppleEvent attribute dictionary
		"""
		_code = 'fndr'
		_subcode = 'fupd'

		aetools.keysubst(_arguments, self._argmap_update)
		_arguments['----'] = _object


		_reply, _arguments, _attributes = self.send(_code, _subcode,
				_arguments, _attributes)
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']


class item(aetools.ComponentItem):
	"""item - An item """
	want = 'cobj'
class bounds(aetools.NProperty):
	"""bounds - the bounding rectangle of the item (can only be set for an item in a window viewed as icons or buttons) """
	which = 'pbnd'
	want = 'qdrt'
class comment(aetools.NProperty):
	"""comment - the comment of the item, displayed in the \xd2Get Info\xd3 window """
	which = 'comt'
	want = 'utxt'
class container(aetools.NProperty):
	"""container - the container of the item """
	which = 'ctnr'
	want = 'obj '
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created """
	which = 'ascd'
	want = 'ldt '
class description(aetools.NProperty):
	"""description - a description of the item """
	which = 'dscr'
	want = 'utxt'
class disk(aetools.NProperty):
	"""disk - the disk on which the item is stored """
	which = 'cdis'
	want = 'obj '
class displayed_name(aetools.NProperty):
	"""displayed name - the user-visible name of the item """
	which = 'dnam'
	want = 'utxt'
class everyones_privileges(aetools.NProperty):
	"""everyones privileges -  """
	which = 'gstp'
	want = 'priv'
class extension_hidden(aetools.NProperty):
	"""extension hidden - Is the item's extension hidden from the user? """
	which = 'hidx'
	want = 'bool'
class group(aetools.NProperty):
	"""group - the user or group that has special access to the container """
	which = 'sgrp'
	want = 'utxt'
class group_privileges(aetools.NProperty):
	"""group privileges -  """
	which = 'gppr'
	want = 'priv'
class icon(aetools.NProperty):
	"""icon - the icon bitmap of the item """
	which = 'iimg'
	want = 'ifam'
class index(aetools.NProperty):
	"""index - the index in the front-to-back ordering within its container """
	which = 'pidx'
	want = 'long'
class information_window(aetools.NProperty):
	"""information window - the information window for the item """
	which = 'iwnd'
	want = 'obj '
class kind(aetools.NProperty):
	"""kind - the kind of the item """
	which = 'kind'
	want = 'utxt'
class label_index(aetools.NProperty):
	"""label index - the label of the item """
	which = 'labi'
	want = 'long'
class locked(aetools.NProperty):
	"""locked - Is the file locked? """
	which = 'aslk'
	want = 'bool'
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified """
	which = 'asmo'
	want = 'ldt '
class name(aetools.NProperty):
	"""name - the name of the item """
	which = 'pnam'
	want = 'utxt'
class name_extension(aetools.NProperty):
	"""name extension - the name extension of the item (such as \xd2txt\xd3) """
	which = 'nmxt'
	want = 'utxt'
class owner(aetools.NProperty):
	"""owner - the user that owns the container """
	which = 'sown'
	want = 'utxt'
class owner_privileges(aetools.NProperty):
	"""owner privileges -  """
	which = 'ownr'
	want = 'priv'
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the item on disk """
	which = 'phys'
	want = 'comp'
class position(aetools.NProperty):
	"""position - the position of the item within its parent window (can only be set for an item in a window viewed as icons or buttons) """
	which = 'posn'
	want = 'QDpt'
class properties(aetools.NProperty):
	"""properties - every property of an item """
	which = 'pALL'
	want = 'reco'
class size(aetools.NProperty):
	"""size - the logical size of the item """
	which = 'ptsz'
	want = 'comp'
class url(aetools.NProperty):
	"""url - the url of the item """
	which = 'pURL'
	want = 'utxt'

items = item
item._superclassnames = []
item._privpropdict = {
	'bounds' : bounds,
	'comment' : comment,
	'container' : container,
	'creation_date' : creation_date,
	'description' : description,
	'disk' : disk,
	'displayed_name' : displayed_name,
	'everyones_privileges' : everyones_privileges,
	'extension_hidden' : extension_hidden,
	'group' : group,
	'group_privileges' : group_privileges,
	'icon' : icon,
	'index' : index,
	'information_window' : information_window,
	'kind' : kind,
	'label_index' : label_index,
	'locked' : locked,
	'modification_date' : modification_date,
	'name' : name,
	'name_extension' : name_extension,
	'owner' : owner,
	'owner_privileges' : owner_privileges,
	'physical_size' : physical_size,
	'position' : position,
	'properties' : properties,
	'size' : size,
	'url' : url,
}
item._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cobj' : item,
}

_propdeclarations = {
	'ascd' : creation_date,
	'aslk' : locked,
	'asmo' : modification_date,
	'cdis' : disk,
	'comt' : comment,
	'ctnr' : container,
	'dnam' : displayed_name,
	'dscr' : description,
	'gppr' : group_privileges,
	'gstp' : everyones_privileges,
	'hidx' : extension_hidden,
	'iimg' : icon,
	'iwnd' : information_window,
	'kind' : kind,
	'labi' : label_index,
	'nmxt' : name_extension,
	'ownr' : owner_privileges,
	'pALL' : properties,
	'pURL' : url,
	'pbnd' : bounds,
	'phys' : physical_size,
	'pidx' : index,
	'pnam' : name,
	'posn' : position,
	'ptsz' : size,
	'sgrp' : group,
	'sown' : owner,
}

_compdeclarations = {
}

_enumdeclarations = {
}
