# Symbolic constants for Tk

# Booleans
NO=FALSE=OFF=0
YES=TRUE=ON=1

# -anchor and -sticky
N='n'
S='s'
W='w'
E='e'
NW='nw'
SW='sw'
NE='ne'
SE='se'
NS='ns'
EW='ew'
NSEW='nsew'
CENTER='center'

# -fill
NONE='none'
X='x'
Y='y'
BOTH='both'

# -side
LEFT='left'
TOP='top'
RIGHT='right'
BOTTOM='bottom'

# -relief
RAISED='raised'
SUNKEN='sunken'
FLAT='flat'
RIDGE='ridge'
GROOVE='groove'
SOLID = 'solid'

# -orient
HORIZONTAL='horizontal'
VERTICAL='vertical'

# -tabs
NUMERIC='numeric'

# -wrap
CHAR='char'
WORD='word'

# -align
BASELINE='baseline'

# -bordermode
INSIDE='inside'
OUTSIDE='outside'

# Special tags, marks and insert positions
SEL='sel'
SEL_FIRST='sel.first'
SEL_LAST='sel.last'
END='end'
INSERT='insert'
CURRENT='current'
ANCHOR='anchor'
ALL='all' # e.g. Canvas.delete(ALL)

# Text widget and button states
NORMAL='normal'
DISABLED='disabled'
ACTIVE='active'
# Canvas state
HIDDEN='hidden'

# Menu item types
CASCADE='cascade'
CHECKBUTTON='checkbutton'
COMMAND='command'
RADIOBUTTON='radiobutton'
SEPARATOR='separator'

# Selection modes for list boxes
SINGLE='single'
BROWSE='browse'
MULTIPLE='multiple'
EXTENDED='extended'

# Activestyle for list boxes
# NONE='none' is also valid
DOTBOX='dotbox'
UNDERLINE='underline'

# Various canvas styles
PIESLICE='pieslice'
CHORD='chord'
ARC='arc'
FIRST='first'
LAST='last'
BUTT='butt'
PROJECTING='projecting'
ROUND='round'
BEVEL='bevel'
MITER='miter'

# Arguments to xview/yview
MOVETO='moveto'
SCROLL='scroll'
UNITS='units'
PAGES='pages'

# DND constants
DND_ALL = '*'
DND_COLOR = 'DND_Color'
DND_COLOUR = DND_COLOR
DND_FILES = 'DND_Files'
DND_HTML = 'DND_HTML'
DND_RTF = 'DND_RTF'
DND_TEXT = 'DND_Text'
DND_URL = 'DND_URL'
APPLICATION_X_COLOR = 'application/x-color'
CF_HTML = 'CF_HTML'
HTML_FORMAT = 'HTML Format'
TEXT_HTML = 'text/html'
TEXT_HTML_UNICODE = 'text/html\;charset=utf-8'
NSPASTEBOARDTYPEHTML = 'NSPasteboardTypeHTML'
CF_RTF = 'CF_RTF'
CF_RTFTEXT = 'CF_RTFTEXT'
RICH_TEXT_FORMAT = 'Rich Text Format'
UNIFORMRESOURCELOCATOR = 'UniformResourceLocator'
CF_UNICODETEXT = 'CF_UNICODETEXT'
CF_TEXT = 'CF_TEXT'
CF_HDROP = 'CF_HDROP'
TEXT_PLAIN_UNICODE = 'text/plain;charset=UTF-8'
TEXT_PLAIN = 'text/plain'
URI_LIST = 'text/uri-list'
NSSTRINGPBOARDTYPE = 'NSStringPboardType'
NSFILENAMESPBOARDTYPE = 'NSFilenamesPboardType'

DND_COLOR_DROP = f'<<Drop:{DND_COLOR}>>'
DND_COLOUR_DROP = DND_COLOR_DROP
DND_FILES_DROP = f'<<Drop:{DND_FILES}>>'
DND_HTML_DROP = f'<<Drop:{DND_HTML}>>'
DND_RTF_DROP = f'<<Drop:{DND_RTF}>>'
DND_TEXT_DROP = f'<<Drop:{DND_TEXT}>>'
DND_URL_DROP = f'<<Drop:DND_URL>>'
APPLICATION_X_COLOR_DROP = f'<<Drop:{APPLICATION_X_COLOR}>>'
CF_HTML_DROP = f'<<Drop:{CF_HTML}>>'
HTML_FORMAT_DROP = f'<<Drop:{HTML_FORMAT}>>'
TEXT_HTML_DROP = f'<<Drop:{TEXT_HTML}>>'
TEXT_HTML_UNICODE_DROP = f'<<Drop:{TEXT_HTML_UNICODE}>>'
NSPASTEBOARDTYPEHTML_DROP = f'<<Drop:{NSPASTEBOARDTYPEHTML}>>'
CF_RTF_DROP = f'<<Drop:{CF_RTF}>>'
CF_RTFTEXT_DROP = f'<<Drop:{CF_RTFTEXT}>>'
RICH_TEXT_FORMAT_DROP = f'<<Drop:{RICH_TEXT_FORMAT}>>'
UNIFORMRESOURCELOCATOR_DROP = f'<<Drop:{UNIFORMRESOURCELOCATOR}>>'
CF_UNICODETEXT_DROP = f'<<Drop:{CF_UNICODETEXT}>>'
CF_TEXT_DROP = f'<<Drop:{CF_TEXT}>>'
CF_HDROP_DROP = f'<<Drop:{CF_HDROP}>>'
TEXT_PLAIN_UNICODE_DROP = f'<<Drop:{TEXT_PLAIN_UNICODE}>>'
TEXT_PLAIN_DROP = f'<<Drop:{TEXT_PLAIN}>>'
URI_LIST_DROP = f'<<Drop:{URI_LIST}>>'
NSSTRINGPBOARDTYPE_DROP = f'<<Drop:{NSSTRINGPBOARDTYPE}>>'
NSFILENAMESPBOARDTYPE_DROP = f'<<Drop:{NSFILENAMESPBOARDTYPE}>>'

ASK = 'ask'
COPY = 'copy'
LINK = 'link'
MOVE = 'move'
PRIVATE = 'private'
REFUSE_DROP = 'refuse_drop'
