"""Suite Text Suite: A set of basic classes for text processing
Level 1, version 1

Generated from Macintosh HD:Systeemmap:Extensies:AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'TEXT'

class Text_Suite_Events:

	pass


class character(aetools.ComponentItem):
	"""character - A character """
	want = 'cha '
class _3c_inheritance_3e_(aetools.NProperty):
	"""<inheritance> - inherits some of its properties from this class """
	which = 'c@#^'
	want = 'ctxt'

class line(aetools.ComponentItem):
	"""line - A line of text """
	want = 'clin'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class justification(aetools.NProperty):
	"""justification - the justification of the text """
	which = 'pjst'
	want = 'just'

lines = line

class paragraph(aetools.ComponentItem):
	"""paragraph - A paragraph """
	want = 'cpar'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class

paragraphs = paragraph

class text(aetools.ComponentItem):
	"""text - Text """
	want = 'ctxt'
class color(aetools.NProperty):
	"""color - the color of the first character """
	which = 'colr'
	want = 'cRGB'
class font(aetools.NProperty):
	"""font - the name of the font of the first character """
	which = 'font'
	want = 'ctxt'
class size(aetools.NProperty):
	"""size - the size in points of the first character """
	which = 'ptsz'
	want = 'fixd'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language """
	which = 'psct'
	want = 'intl'
class style(aetools.NProperty):
	"""style - the text style of the first character of the first character """
	which = 'txst'
	want = 'tsty'
class uniform_styles(aetools.NProperty):
	"""uniform styles - the text styles that are uniform throughout the text """
	which = 'ustl'
	want = 'tsty'
#        element 'cha ' as ['indx']
#        element 'clin' as ['indx']
#        element 'cpar' as ['indx']
#        element 'ctxt' as ['indx']
#        element 'cwor' as ['indx']

class text_flow(aetools.ComponentItem):
	"""text flow - A contiguous block of text.  Page layout applications call this a •story.Õ """
	want = 'cflo'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class
class name(aetools.NProperty):
	"""name - the name """
	which = 'pnam'
	want = 'itxt'

text_flows = text_flow

class text_style_info(aetools.ComponentItem):
	"""text style info - On and Off styles of text run """
	want = 'tsty'
class on_styles(aetools.NProperty):
	"""on styles - the styles that are on for the text """
	which = 'onst'
	want = 'styl'
class off_styles(aetools.NProperty):
	"""off styles - the styles that are off for the text """
	which = 'ofst'
	want = 'styl'

text_style_infos = text_style_info

class word(aetools.ComponentItem):
	"""word - A word """
	want = 'cwor'
# repeated property _3c_inheritance_3e_ inherits some of its properties from this class

words = word
character._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
}
character._elemdict = {
}
line._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'justification' : justification,
}
line._elemdict = {
}
paragraph._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
}
paragraph._elemdict = {
}
text._propdict = {
	'color' : color,
	'font' : font,
	'size' : size,
	'writing_code' : writing_code,
	'style' : style,
	'uniform_styles' : uniform_styles,
}
text._elemdict = {
	'character' : character,
	'line' : line,
	'paragraph' : paragraph,
	'text' : text,
	'word' : word,
}
text_flow._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
	'name' : name,
}
text_flow._elemdict = {
}
text_style_info._propdict = {
	'on_styles' : on_styles,
	'off_styles' : off_styles,
}
text_style_info._elemdict = {
}
word._propdict = {
	'_3c_inheritance_3e_' : _3c_inheritance_3e_,
}
word._elemdict = {
}
_Enum_just = {
	'left' : 'left',	# Align with left margin
	'right' : 'rght',	# Align with right margin
	'center' : 'cent',	# Align with center
	'full' : 'full',	# Align with both left and right margins
}

_Enum_styl = {
	'plain' : 'plan',	# Plain
	'bold' : 'bold',	# Bold
	'italic' : 'ital',	# Italic
	'outline' : 'outl',	# Outline
	'shadow' : 'shad',	# Shadow
	'underline' : 'undl',	# Underline
	'superscript' : 'spsc',	# Superscript
	'subscript' : 'sbsc',	# Subscript
	'strikethrough' : 'strk',	# Strikethrough
	'small_caps' : 'smcp',	# Small caps
	'all_caps' : 'alcp',	# All capital letters
	'all_lowercase' : 'lowc',	# Lowercase
	'condensed' : 'cond',	# Condensed
	'expanded' : 'pexp',	# Expanded
	'hidden' : 'hidn',	# Hidden
}


#
# Indices of types declared in this module
#
_classdeclarations = {
	'clin' : line,
	'ctxt' : text,
	'cwor' : word,
	'cflo' : text_flow,
	'cpar' : paragraph,
	'tsty' : text_style_info,
	'cha ' : character,
}

_propdeclarations = {
	'psct' : writing_code,
	'txst' : style,
	'colr' : color,
	'font' : font,
	'pnam' : name,
	'ustl' : uniform_styles,
	'pjst' : justification,
	'ofst' : off_styles,
	'onst' : on_styles,
	'ptsz' : size,
	'c@#^' : _3c_inheritance_3e_,
}

_compdeclarations = {
}

_enumdeclarations = {
	'styl' : _Enum_styl,
	'just' : _Enum_just,
}
