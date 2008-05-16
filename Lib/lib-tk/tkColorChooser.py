import sys
from warnings import warnpy3k

warnpy3k("the tkColorChooser module has been renamed "
         "to 'tkinter.colorchooser' in Python 3.0", stacklevel=2)

import tkinter.colorchooser
sys.modules[__name__] = tkinter.colorchooser
