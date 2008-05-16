import sys
from warnings import warnpy3k

warnpy3k("the tkMessageBox module has been renamed "
         "to 'tkinter.messagebox' in Python 3.0", stacklevel=2)

import tkinter.messagebox
sys.modules[__name__] = tkinter.messagebox
