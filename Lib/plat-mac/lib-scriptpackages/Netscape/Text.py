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
class _Prop_beginning(aetools.NProperty):
    """beginning - Beginning of element """
    which = 'bgng'
    want = 'obj '
class _Prop_end(aetools.NProperty):
    """end - Ending of element """
    which = 'end '
    want = 'obj '
class _Prop_infront(aetools.NProperty):
    """infront - Immediately before element """
    which = 'pBef'
    want = 'obj '
class _Prop_justbehind(aetools.NProperty):
    """justbehind - Immediately after element """
    which = 'pAft'
    want = 'obj '
class _Prop_updateLevel(aetools.NProperty):
    """updateLevel - updating level.  Can only be incremented or decremented.  Do so only in a try block -- if the level is greater than zero, visual text updating will cease. """
    which = 'pUpL'
    want = 'long'
#        element 'stys' as ['indx', 'name']

class styleset(aetools.ComponentItem):
    """styleset - A style \xd2set\xd3 that may be used repeatedly in text objects. """
    want = 'stys'
class _Prop_color(aetools.NProperty):
    """color - the color """
    which = 'colr'
    want = 'RGB '
class _Prop_font(aetools.NProperty):
    """font - font name """
    which = 'font'
    want = 'TEXT'
class _Prop_name(aetools.NProperty):
    """name - style name """
    which = 'pnam'
    want = 'TEXT'
class _Prop_size(aetools.NProperty):
    """size - the size in points """
    which = 'ptsz'
    want = 'long'
class _Prop_style(aetools.NProperty):
    """style - the text styles or face attributes """
    which = 'txst'
    want = 'tsty'
class _Prop_writing_code(aetools.NProperty):
    """writing code - the script system and language """
    which = 'psct'
    want = 'tsty'

stylesets = styleset
text._superclassnames = []
text._privpropdict = {
    'beginning' : _Prop_beginning,
    'end' : _Prop_end,
    'infront' : _Prop_infront,
    'justbehind' : _Prop_justbehind,
    'updateLevel' : _Prop_updateLevel,
}
text._privelemdict = {
    'styleset' : styleset,
}
styleset._superclassnames = []
styleset._privpropdict = {
    'color' : _Prop_color,
    'font' : _Prop_font,
    'name' : _Prop_name,
    'size' : _Prop_size,
    'style' : _Prop_style,
    'writing_code' : _Prop_writing_code,
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
    'bgng' : _Prop_beginning,
    'colr' : _Prop_color,
    'end ' : _Prop_end,
    'font' : _Prop_font,
    'pAft' : _Prop_justbehind,
    'pBef' : _Prop_infront,
    'pUpL' : _Prop_updateLevel,
    'pnam' : _Prop_name,
    'psct' : _Prop_writing_code,
    'ptsz' : _Prop_size,
    'txst' : _Prop_style,
}

_compdeclarations = {
}

_enumdeclarations = {
}
