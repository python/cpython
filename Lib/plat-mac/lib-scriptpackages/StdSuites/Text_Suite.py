"""Suite Text Suite: A set of basic classes for text processing
Level 1, version 1

Generated from /Volumes/Sap/System Folder/Extensions/AppleScript
AETE/AEUT resource version 1/0, language 0, script 0
"""

import aetools
import MacOS

_code = 'TEXT'

class Text_Suite_Events:

    pass


class text_flow(aetools.ComponentItem):
    """text flow - A contiguous block of text.  Page layout applications call this a \xd4story.\xd5 """
    want = 'cflo'
class _Prop__3c_inheritance_3e_(aetools.NProperty):
    """<inheritance> - inherits some of its properties from this class """
    which = 'c@#^'
    want = 'ctxt'
class _Prop_name(aetools.NProperty):
    """name - the name """
    which = 'pnam'
    want = 'itxt'

text_flows = text_flow

class character(aetools.ComponentItem):
    """character - A character """
    want = 'cha '

class line(aetools.ComponentItem):
    """line - A line of text """
    want = 'clin'
class _Prop_justification(aetools.NProperty):
    """justification - the justification of the text """
    which = 'pjst'
    want = 'just'

lines = line

class paragraph(aetools.ComponentItem):
    """paragraph - A paragraph """
    want = 'cpar'

paragraphs = paragraph

class text(aetools.ComponentItem):
    """text - Text """
    want = 'ctxt'
class _Prop_color(aetools.NProperty):
    """color - the color of the first character """
    which = 'colr'
    want = 'cRGB'
class _Prop_font(aetools.NProperty):
    """font - the name of the font of the first character """
    which = 'font'
    want = 'ctxt'
class _Prop_quoted_form(aetools.NProperty):
    """quoted form - the text in quoted form """
    which = 'strq'
    want = 'ctxt'
class _Prop_size(aetools.NProperty):
    """size - the size in points of the first character """
    which = 'ptsz'
    want = 'fixd'
class _Prop_style(aetools.NProperty):
    """style - the text style of the first character of the first character """
    which = 'txst'
    want = 'tsty'
class _Prop_uniform_styles(aetools.NProperty):
    """uniform styles - the text styles that are uniform throughout the text """
    which = 'ustl'
    want = 'tsty'
class _Prop_writing_code(aetools.NProperty):
    """writing code - the script system and language """
    which = 'psct'
    want = 'intl'
#        element 'cha ' as ['indx']
#        element 'clin' as ['indx']
#        element 'cpar' as ['indx']
#        element 'ctxt' as ['indx']
#        element 'cwor' as ['indx']

class word(aetools.ComponentItem):
    """word - A word """
    want = 'cwor'

words = word

class text_style_info(aetools.ComponentItem):
    """text style info - On and Off styles of text run """
    want = 'tsty'
class _Prop_off_styles(aetools.NProperty):
    """off styles - the styles that are off for the text """
    which = 'ofst'
    want = 'styl'
class _Prop_on_styles(aetools.NProperty):
    """on styles - the styles that are on for the text """
    which = 'onst'
    want = 'styl'

text_style_infos = text_style_info
text_flow._superclassnames = ['text']
text_flow._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
    'name' : _Prop_name,
}
text_flow._privelemdict = {
}
character._superclassnames = ['text']
character._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
character._privelemdict = {
}
line._superclassnames = ['text']
line._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
    'justification' : _Prop_justification,
}
line._privelemdict = {
}
paragraph._superclassnames = ['text']
paragraph._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
paragraph._privelemdict = {
}
text._superclassnames = []
text._privpropdict = {
    'color' : _Prop_color,
    'font' : _Prop_font,
    'quoted_form' : _Prop_quoted_form,
    'size' : _Prop_size,
    'style' : _Prop_style,
    'uniform_styles' : _Prop_uniform_styles,
    'writing_code' : _Prop_writing_code,
}
text._privelemdict = {
    'character' : character,
    'line' : line,
    'paragraph' : paragraph,
    'text' : text,
    'word' : word,
}
word._superclassnames = ['text']
word._privpropdict = {
    '_3c_inheritance_3e_' : _Prop__3c_inheritance_3e_,
}
word._privelemdict = {
}
text_style_info._superclassnames = []
text_style_info._privpropdict = {
    'off_styles' : _Prop_off_styles,
    'on_styles' : _Prop_on_styles,
}
text_style_info._privelemdict = {
}
_Enum_just = {
    'left' : 'left',    # Align with left margin
    'right' : 'rght',   # Align with right margin
    'center' : 'cent',  # Align with center
    'full' : 'full',    # Align with both left and right margins
}

_Enum_styl = {
    'plain' : 'plan',   # Plain
    'bold' : 'bold',    # Bold
    'italic' : 'ital',  # Italic
    'outline' : 'outl', # Outline
    'shadow' : 'shad',  # Shadow
    'underline' : 'undl',       # Underline
    'superscript' : 'spsc',     # Superscript
    'subscript' : 'sbsc',       # Subscript
    'strikethrough' : 'strk',   # Strikethrough
    'small_caps' : 'smcp',      # Small caps
    'all_caps' : 'alcp',        # All capital letters
    'all_lowercase' : 'lowc',   # Lowercase
    'condensed' : 'cond',       # Condensed
    'expanded' : 'pexp',        # Expanded
    'hidden' : 'hidn',  # Hidden
}


#
# Indices of types declared in this module
#
_classdeclarations = {
    'cflo' : text_flow,
    'cha ' : character,
    'clin' : line,
    'cpar' : paragraph,
    'ctxt' : text,
    'cwor' : word,
    'tsty' : text_style_info,
}

_propdeclarations = {
    'c@#^' : _Prop__3c_inheritance_3e_,
    'colr' : _Prop_color,
    'font' : _Prop_font,
    'ofst' : _Prop_off_styles,
    'onst' : _Prop_on_styles,
    'pjst' : _Prop_justification,
    'pnam' : _Prop_name,
    'psct' : _Prop_writing_code,
    'ptsz' : _Prop_size,
    'strq' : _Prop_quoted_form,
    'txst' : _Prop_style,
    'ustl' : _Prop_uniform_styles,
}

_compdeclarations = {
}

_enumdeclarations = {
    'just' : _Enum_just,
    'styl' : _Enum_styl,
}
