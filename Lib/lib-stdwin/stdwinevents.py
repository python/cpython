# Module 'stdwinevents' -- Constants for stdwin event types
#
# Suggested usage:
#	from stdwinevents import *

# The function stdwin.getevent() returns a tuple containing:
#	(type, window, detail)
# where detail may be <no value> or a value depending on type, see below:

# Values for type:

WE_NULL       =  0	# not reported -- means 'no event' internally
WE_ACTIVATE   =  1	# detail is <no object>
WE_CHAR       =  2	# detail is the character
WE_COMMAND    =  3	# detail is one of the WC_* constants below
WE_MOUSE_DOWN =  4	# detail is ((h, v), clicks, button, mask)
WE_MOUSE_MOVE =  5	# ditto
WE_MOUSE_UP   =  6	# ditto
WE_MENU       =  7	# detail is (menu, item)
WE_SIZE       =  8	# detail is (width, height) [???]
WE_MOVE       =  9	# not reported -- reserved for future use
WE_DRAW       = 10	# detail is ((left, top), (right, bottom))
WE_TIMER      = 11	# detail is <no object>
WE_DEACTIVATE = 12	# detail is <no object>
WE_EXTERN     = 13	# detail is <no object>
WE_KEY        = 14	# detail is ???
WE_LOST_SEL   = 15	# detail is selection number
WE_CLOSE      = 16	# detail is <no object>

# Values for detail when type is WE_COMMAND:

WC_CLOSE      =  1	# user hit close box
WC_LEFT       =  2	# left arrow key
WC_RIGHT      =  3	# right arrow key
WC_UP         =  4	# up arrow key
WC_DOWN       =  5	# down arrow key
WC_CANCEL     =  6	# not reported -- turned into KeyboardInterrupt
WC_BACKSPACE  =  7	# backspace key
WC_TAB        =  8	# tab key
WC_RETURN     =  9	# return or enter key

# Selection numbers

WS_CLIPBOARD   = 0
WS_PRIMARY     = 1
WS_SECONDARY   = 2
