import sys
from warnings import warnpy3k

warnpy3k("the FixTk module has been renamed "
         "to 'tkinter._fix' in Python 3.0", stacklevel=2)

import tkinter._fix
sys.modules[__name__] = tkinter._fix
