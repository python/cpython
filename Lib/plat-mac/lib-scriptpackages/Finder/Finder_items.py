"""Suite Finder items: Commands used with file system items, and basic item definition
Level 1, version 1

Generated from /Volumes/Moes/Systeemmap/Finder
AETE/AEUT resource version 0/144, language 0, script 0
"""

import aetools
import MacOS

_code = 'fndr'

class Finder_items_Events:

	def add_to_favorites(self, _object, _attributes={}, **_arguments):
		"""add to favorites: Add the items to the Favorites menu in the Apple Menu and in Open and Save dialogs
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
		"""clean up: Arrange items in window nicely (only applies to open windows in icon or button views that are not kept arranged)
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
		if _arguments.get('errn', 0):
			raise aetools.Error, aetools.decodeerror(_arguments)
		# XXXX Optionally decode result
		if _arguments.has_key('----'):
			return _arguments['----']

	_argmap_put_away = {
		'asking' : 'fask',
	}

	def put_away(self, _object, _attributes={}, **_arguments):
		"""put away: Put away the specified object(s)
		Required argument: the items to put away
		Keyword argument asking: Specifies whether or not to present a dialog to confirm putting this item away.
		Keyword argument _attributes: AppleEvent attribute dictionary
		Returns: the object put away in its put-away location
		"""
		_code = 'fndr'
		_subcode = 'ptwy'

		aetools.keysubst(_arguments, self._argmap_put_away)
		_arguments['----'] = _object

		aetools.enumsubst(_arguments, 'fask', _Enum_bool)

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
	want = 'itxt'
class container(aetools.NProperty):
	"""container - the container of the item """
	which = 'ctnr'
	want = 'obj '
class content_space(aetools.NProperty):
	"""content space - the window that would open if the item was opened """
	which = 'dwnd'
	want = 'obj '
class creation_date(aetools.NProperty):
	"""creation date - the date on which the item was created """
	which = 'ascd'
	want = 'ldt '
class description(aetools.NProperty):
	"""description - a description of the item """
	which = 'dscr'
	want = 'itxt'
class disk(aetools.NProperty):
	"""disk - the disk on which the item is stored """
	which = 'cdis'
	want = 'obj '
class folder(aetools.NProperty):
	"""folder - the folder in which the item is stored """
	which = 'asdr'
	want = 'obj '
class icon(aetools.NProperty):
	"""icon - the icon bitmap of the item """
	which = 'iimg'
	want = 'ifam'
class id(aetools.NProperty):
	"""id - an id that identifies the item """
	which = 'ID  '
	want = 'long'
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
	want = 'itxt'
class label_index(aetools.NProperty):
	"""label index - the label of the item """
	which = 'labi'
	want = 'long'
class modification_date(aetools.NProperty):
	"""modification date - the date on which the item was last modified """
	which = 'asmo'
	want = 'ldt '
class name(aetools.NProperty):
	"""name - the name of the item """
	which = 'pnam'
	want = 'itxt'
class physical_size(aetools.NProperty):
	"""physical size - the actual space used by the item on disk """
	which = 'phys'
	want = 'long'
class position(aetools.NProperty):
	"""position - the position of the item within its parent window (can only be set for an item in a window viewed as icons or buttons) """
	which = 'posn'
	want = 'QDpt'
class selected(aetools.NProperty):
	"""selected - Is the item selected? """
	which = 'issl'
	want = 'bool'
class size(aetools.NProperty):
	"""size - the logical size of the item """
	which = 'ptsz'
	want = 'long'
class window(aetools.NProperty):
	"""window - the window that would open if the item was opened """
	which = 'cwin'
	want = 'obj '

items = item
item._superclassnames = []
item._privpropdict = {
	'bounds' : bounds,
	'comment' : comment,
	'container' : container,
	'content_space' : content_space,
	'creation_date' : creation_date,
	'description' : description,
	'disk' : disk,
	'folder' : folder,
	'icon' : icon,
	'id' : id,
	'index' : index,
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
item._privelemdict = {
}
_Enum_bool = None # XXXX enum bool not found!!

#
# Indices of types declared in this module
#
_classdeclarations = {
	'cobj' : item,
}

_propdeclarations = {
	'ID  ' : id,
	'ascd' : creation_date,
	'asdr' : folder,
	'asmo' : modification_date,
	'cdis' : disk,
	'comt' : comment,
	'ctnr' : container,
	'cwin' : window,
	'dscr' : description,
	'dwnd' : content_space,
	'iimg' : icon,
	'issl' : selected,
	'iwnd' : information_window,
	'kind' : kind,
	'labi' : label_index,
	'pbnd' : bounds,
	'phys' : physical_size,
	'pidx' : index,
	'pnam' : name,
	'posn' : position,
	'ptsz' : size,
}

_compdeclarations = {
}

_enumdeclarations = {
}
