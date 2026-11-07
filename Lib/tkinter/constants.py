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

# -labelanchor, in addition to the values above
EN='en'
ES='es'
WN='wn'
WS='ws'

# -compound
TEXT='text'
IMAGE='image'

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
IGNORE='ignore'
OUTSIDE='outside'

# -direction
ABOVE='above'
BELOW='below'
FLUSH='flush'

# -stretch
ALWAYS='always'
MIDDLE='middle'
NEVER='never'

# -tabstyle
TABULAR='tabular'
WORDPROCESSOR='wordprocessor'

# -insertunfocussed (Tk 8.6+)
HOLLOW='hollow'

# -mode
DETERMINATE='determinate'
INDETERMINATE='indeterminate'

# -selecttype (Tk 9.0+)
ITEM='item'
CELL='cell'

# -show
TREE='tree'
HEADINGS='headings'

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
# Entry and ttk widget states
READONLY='readonly'
# Other ttk widget states
ALTERNATE='alternate'
BACKGROUND='background'
FOCUS='focus'
HOVER='hover'
INVALID='invalid'
PRESSED='pressed'
SELECTED='selected'
# ttk widget states for Treeview items and Notebook tabs (Tk 9.1+)
# FIRST='first' and LAST='last' are also used
LEAF='leaf'
OPEN='open'

# -validate
# ALL='all', FOCUS='focus' and NONE='none' are also valid
KEY='key'
FOCUSIN='focusin'
FOCUSOUT='focusout'

# Validation reasons, in addition to the values above
FORCED='forced'

# Menu item types
CASCADE='cascade'
CHECKBUTTON='checkbutton'
COMMAND='command'
RADIOBUTTON='radiobutton'
SEPARATOR='separator'

# -type for menus
MENUBAR='menubar'
TEAROFF='tearoff'

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
PIXELS='pixels'
