import sys
from warnings import warnpy3k

warnpy3k("the tkFont module has been renamed "
         "to 'tkinter.font' in Python 3.0", stacklevel=2)

import tkinter.font
sys.modules[__name__] = tkinter.font
