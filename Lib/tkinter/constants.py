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
DND_FILES = 'DND_Files'
DND_TEXT = 'DND_Text'
CF_UNICODETEXT = 'CF_UNICODETEXT'
CF_TEXT = 'CF_TEXT'
CF_HDROP = 'CF_HDROP'
TEXT_PLAIN_UNICODE = 'text/plain;charset=UTF-8'
TEXT_PLAIN = 'text/plain'
URI_LIST = 'text/uri-list'
NSSTRINGPBOARDTYPE = 'NSStringPboardType'
NSFILENAMESPBOARDTYPE = 'NSFilenamesPboardType'

DND_FILES_DROP = '<<Drop:DND_Files>>'
DND_TEXT_DROP = '<<Drop:DND_Text>>'
CF_UNICODETEXT_DROP = '<<Drop:CF_UNICODETEXT>>'
CF_TEXT_DROP = '<<Drop:CF_TEXT>>'
CF_HDROP_DROP = '<<Drop:CF_HDROP>>'
TEXT_PLAIN_UNICODE_DROP = '<<Drop:text/plain;charset=UTF-8>>'
TEXT_PLAIN_DROP = '<<Drop:text/plain>>'
URI_LIST_DROP = '<<Drop:text/uri-list>>'
NSSTRINGPBOARDTYPE_DROP = '<<Drop:NSStringPboardType>>'
NSFILENAMESPBOARDTYPE_DROP = '<<Drop:NSFilenamesPboardType>>'

ASK = 'ask'
COPY = 'copy'
LINK = 'link'
MOVE = 'move'
PRIVATE = 'private'
REFUSE_DROP = 'refuse_drop'
