"""Suite Login Items Suite: Terms and Events for controlling the Login Items application
Level 1, version 1

Generated from /System/Library/CoreServices/System Events.app
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'logi'

class Login_Items_Suite_Events:

	pass


class login_item(aetools.ComponentItem):
	"""login item - an item to be launched or opened at login """
	want = 'logi'
class _3c_Inheritance_3e_(aetools.NProperty):
	"""<Inheritance> - All of the properties of the superclass. """
	which = 'c@#^'
	want = 'cobj'
class hidden(aetools.NProperty):
	"""hidden - Is the Login Item hidden when launched? """
	which = 'hidn'
	want = 'bool'
class kind(aetools.NProperty):
	"""kind - the file type of the Login Item """
	which = 'kind'
	want = 'utxt'
class name(aetools.NProperty):
	"""name - the name of the Login Item """
	which = 'pnam'
	want = 'utxt'
class path(aetools.NProperty):
	"""path - the file system path to the Login Item """
	which = 'ppth'
	want = 'utxt'

login_items = login_item
import Standard_Suite
login_item._superclassnames = ['item']
login_item._privpropdict = {
	'_3c_Inheritance_3e_' : _3c_Inheritance_3e_,
	'hidden' : hidden,
	'kind' : kind,
	'name' : name,
	'path' : path,
}
login_item._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'logi' : login_item,
}

_propdeclarations = {
	'c@#^' : _3c_Inheritance_3e_,
	'hidn' : hidden,
	'kind' : kind,
	'pnam' : name,
	'ppth' : path,
}

_compdeclarations = {
}

_enumdeclarations = {
}
