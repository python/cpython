import sys
from warnings import warnpy3k

warnpy3k("the Tkdnd module has been renamed "
         "to 'tkinter.dnd' in Python 3.0", stacklevel=2)

import tkinter.dnd
sys.modules[__name__] = tkinter.dnd
