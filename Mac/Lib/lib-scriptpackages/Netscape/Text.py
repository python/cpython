"""Suite Text: 
Level 0, version 0

Generated from /Volumes/Sap/Applications (Mac OS 9)/Netscape Communicator\xe2\x84\xa2 Folder/Netscape Communicator\xe2\x84\xa2
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'TEXT'

from StdSuites.Text_Suite import *
class Text_Events(Text_Suite_Events):

	pass


class text(aetools.ComponentItem):
	"""text - independent text view objects """
	want = 'ctxt'
class updateLevel(aetools.NProperty):
	"""updateLevel - updating level.  Can only be incremented or decremented.  Do so only in a try block -- if the level is greater than zero, visual text updating will cease. """
	which = 'pUpL'
	want = 'long'
class beginning(aetools.NProperty):
	"""beginning - Beginning of element """
	which = 'bgng'
	want = 'obj '
class end(aetools.NProperty):
	"""end - Ending of element """
	which = 'end '
	want = 'obj '
class infront(aetools.NProperty):
	"""infront - Immediately before element """
	which = 'pBef'
	want = 'obj '
class justbehind(aetools.NProperty):
	"""justbehind - Immediately after element """
	which = 'pAft'
	want = 'obj '
#        element 'stys' as ['indx', 'name']

class styleset(aetools.ComponentItem):
	"""styleset - A style \xd2set\xd3 that may be used repeatedly in text objects. """
	want = 'stys'
class name(aetools.NProperty):
	"""name - style name """
	which = 'pnam'
	want = 'TEXT'
class color(aetools.NProperty):
	"""color - the color """
	which = 'colr'
	want = 'RGB '
class font(aetools.NProperty):
	"""font - font name """
	which = 'font'
	want = 'TEXT'
class size(aetools.NProperty):
	"""size - the size in points """
	which = 'ptsz'
	want = 'long'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language """
	which = 'psct'
	want = 'tsty'
class style(aetools.NProperty):
	"""style - the text styles or face attributes """
	which = 'txst'
	want = 'tsty'

stylesets = styleset
text._superclassnames = []
text._privpropdict = {
	'updateLevel' : updateLevel,
	'beginning' : beginning,
	'end' : end,
	'infront' : infront,
	'justbehind' : justbehind,
}
text._privelemdict = {
	'styleset' : styleset,
}
styleset._superclassnames = []
styleset._privpropdict = {
	'name' : name,
	'color' : color,
	'font' : font,
	'size' : size,
	'writing_code' : writing_code,
	'style' : style,
}
styleset._privelemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'ctxt' : text,
	'stys' : styleset,
}

_propdeclarations = {
	'pBef' : infront,
	'bgng' : beginning,
	'colr' : color,
	'txst' : style,
	'psct' : writing_code,
	'pAft' : justbehind,
	'end ' : end,
	'ptsz' : size,
	'pUpL' : updateLevel,
	'pnam' : name,
	'font' : font,
}

_compdeclarations = {
}

_enumdeclarations = {
}
