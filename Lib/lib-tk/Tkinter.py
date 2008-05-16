import sys
from warnings import warnpy3k

warnpy3k("the Tkinter module has been renamed "
         "to 'tkinter' in Python 3.0", stacklevel=2)

import tkinter
sys.modules[__name__] = tkinter
