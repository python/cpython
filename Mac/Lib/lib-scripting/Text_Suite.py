"""Suite Text Suite: A set of basic classes for text processing
Level 1, version 1

Generated from flap:System Folder:Extensions:Scripting Additions:Dialects:English Dialect
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'TEXT'

class Text_Suite:

	pass


class character(aetools.ComponentItem):
	"""character - A character"""
	want = 'cha '
class best_type(aetools.NProperty):
	"""best type - the best descriptor type"""
	which = 'pbst'
	want = 'type'
class _class(aetools.NProperty):
	"""class - the class"""
	which = 'pcls'
	want = 'type'
class color(aetools.NProperty):
	"""color - the color"""
	which = 'colr'
	want = 'cRGB'
class default_type(aetools.NProperty):
	"""default type - the default descriptor type"""
	which = 'deft'
	want = 'type'
class font(aetools.NProperty):
	"""font - the name of the font"""
	which = 'font'
	want = 'ctxt'
class size(aetools.NProperty):
	"""size - the size in points"""
	which = 'ptsz'
	want = 'fixd'
class writing_code(aetools.NProperty):
	"""writing code - the script system and language"""
	which = 'psct'
	want = 'intl'
class style(aetools.NProperty):
	"""style - the text style"""
	which = 'txst'
	want = 'tsty'
class uniform_styles(aetools.NProperty):
	"""uniform styles - the text style"""
	which = 'ustl'
	want = 'tsty'

class line(aetools.ComponentItem):
	"""line - A line of text"""
	want = 'clin'
class justification(aetools.NProperty):
	"""justification - Justification of the text"""
	which = 'pjst'
	want = 'just'

lines = line

class paragraph(aetools.ComponentItem):
	"""paragraph - A paragraph"""
	want = 'cpar'

paragraphs = paragraph

class text(aetools.ComponentItem):
	"""text - Text"""
	want = 'ctxt'

class text_flow(aetools.ComponentItem):
	"""text flow - A contiguous block of text"""
	want = 'cflo'
class name(aetools.NProperty):
	"""name - the name"""
	which = 'pnam'
	want = 'itxt'

text_flows = text_flow

class word(aetools.ComponentItem):
	"""word - A word"""
	want = 'cwor'

words = word
character._propdict = {
	'best_type' : best_type,
	'_class' : _class,
	'color' : color,
	'default_type' : default_type,
	'font' : font,
	'size' : size,
	'writing_code' : writing_code,
	'style' : style,
	'uniform_styles' : uniform_styles,
}
character._elemdict = {
}
line._propdict = {
	'justification' : justification,
}
line._elemdict = {
}
paragraph._propdict = {
}
paragraph._elemdict = {
}
text._propdict = {
}
text._elemdict = {
}
text_flow._propdict = {
	'name' : name,
}
text_flow._elemdict = {
}
word._propdict = {
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
	'cflo' : text_flow,
	'clin' : line,
	'ctxt' : text,
	'cha ' : character,
	'cwor' : word,
	'cpar' : paragraph,
}

_propdeclarations = {
	'pbst' : best_type,
	'psct' : writing_code,
	'txst' : style,
	'colr' : color,
	'font' : font,
	'pnam' : name,
	'pcls' : _class,
	'deft' : default_type,
	'pjst' : justification,
	'ptsz' : size,
	'ustl' : uniform_styles,
}

_compdeclarations = {
}

_enumdeclarations = {
	'styl' : _Enum_styl,
	'just' : _Enum_just,
}
