import sys
from warnings import warnpy3k

warnpy3k("the Tix module has been renamed "
         "to 'tkinter.tix' in Python 3.0", stacklevel=2)

import tkinter.tix
sys.modules[__name__] = tkinter.tix
