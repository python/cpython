"""Suite Text: 
Level 0, version 0

Generated from Macintosh HD:Internet:Internet-programma's:Netscape Communicatoré-map:Netscape Communicatoré
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
	"""styleset - A style ñsetî that may be used repeatedly in text objects. """
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
text._propdict = {
	'updateLevel' : updateLevel,
	'beginning' : beginning,
	'end' : end,
	'infront' : infront,
	'justbehind' : justbehind,
}
text._elemdict = {
	'styleset' : styleset,
}
styleset._propdict = {
	'name' : name,
	'color' : color,
	'font' : font,
	'size' : size,
	'writing_code' : writing_code,
	'style' : style,
}
styleset._elemdict = {
}

#
# Indices of types declared in this module
#
_classdeclarations = {
	'stys' : styleset,
	'ctxt' : text,
}

_propdeclarations = {
	'pAft' : justbehind,
	'psct' : writing_code,
	'txst' : style,
	'colr' : color,
	'pBef' : infront,
	'pnam' : name,
	'ptsz' : size,
	'pUpL' : updateLevel,
	'bgng' : beginning,
	'font' : font,
	'end ' : end,
}

_compdeclarations = {
}

_enumdeclarations = {
}
